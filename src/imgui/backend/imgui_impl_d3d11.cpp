// dear imgui: Renderer Backend for DirectX11
// This needs to be used along with a Platform Backend (e.g. Win32)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'ID3D11ShaderResourceView*' as ImTextureID. Read the FAQ about ImTextureID!
//  [X] Renderer: Multi-viewport support. Enable with 'io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable'.
//  [X] Renderer: Support for large meshes (64k+ vertices) with 16-bit indices.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2022-XX-XX: Platform: Added support for multiple windows via the ImGuiPlatformIO interface.
//  2021-06-29: Reorganized backend to pull data from a single structure to facilitate usage with multiple-contexts (all g_XXXX access changed to bd->XXXX).
//  2021-05-19: DirectX11: Replaced direct access to ImDrawCmd::TextureId with a call to ImDrawCmd::GetTexID(). (will become a requirement)
//  2021-02-18: DirectX11: Change blending equation to preserve alpha in output buffer.
//  2019-08-01: DirectX11: Fixed code querying the Geometry Shader state (would generally error with Debug layer enabled).
//  2019-07-21: DirectX11: Backup, clear and restore Geometry Shader is any is bound when calling ImGui_ImplDX10_RenderDrawData. Clearing Hull/Domain/Compute shaders without backup/restore.
//  2019-05-29: DirectX11: Added support for large mesh (64K+ vertices), enable ImGuiBackendFlags_RendererHasVtxOffset flag.
//  2019-04-30: DirectX11: Added support for special ImDrawCallback_ResetRenderState callback to reset render state.
//  2018-12-03: Misc: Added #pragma comment statement to automatically link with d3dcompiler.lib when using D3DCompile().
//  2018-11-30: Misc: Setting up io.BackendRendererName so it can be displayed in the About Window.
//  2018-08-01: DirectX11: Querying for IDXGIFactory instead of IDXGIFactory1 to increase compatibility.
//  2018-07-13: DirectX11: Fixed unreleased resources in Init and Shutdown functions.
//  2018-06-08: Misc: Extracted imgui_impl_dx11.cpp/.h away from the old combined DX11+Win32 example.
//  2018-06-08: DirectX11: Use draw_data->DisplayPos and draw_data->DisplaySize to setup projection matrix and clipping rectangle.
//  2018-02-16: Misc: Obsoleted the io.RenderDrawListsFn callback and exposed ImGui_ImplDX11_RenderDrawData() in the .h file so you can call it yourself.
//  2018-02-06: Misc: Removed call to ImGui::Shutdown() which is not available from 1.60 WIP, user needs to call CreateContext/DestroyContext themselves.
//  2016-05-07: DirectX11: Disabling depth-write.

#include "imgui/3rdparty/imgui.h"
#include "imgui/backend/imgui_custom_draw.h"
#include "imgui/backend/imgui_impl_d3d11.h"
#include <vector>

// DirectX
#include <stdio.h>
#include <d3d11.h>

#include "shader_imgui_default_ps.h"
#include "shader_imgui_default_vs.h"
#include "shader_imgui_waveform_ps.h"
#include "shader_imgui_waveform_vs.h"

// DirectX11 data
struct ImGui_ImplDX11_Data
{
	ID3D11Device*               D3DDevice;
	ID3D11DeviceContext*        D3DDeviceContext;
	IDXGIFactory*               DXGIFactory;
	ID3D11Buffer*               VertexBuffer;
	ID3D11Buffer*               IndexBuffer;
	ID3D11VertexShader*         DefaultVS;
	ID3D11VertexShader*         WaveformVS;
	ID3D11InputLayout*          InputLayout;
	ID3D11Buffer*               VertexConstantBuffer;
	ID3D11Buffer*               WaveformConstantBuffer;
	ID3D11PixelShader*          DefaultPS;
	ID3D11PixelShader*          WaveformPS;
	ID3D11SamplerState*         FontSampler;
	ID3D11ShaderResourceView*   FontTextureView;
	ID3D11RasterizerState*      RasterizerState;
	ID3D11BlendState*           BlendState;
	ID3D11DepthStencilState*    DepthStencilState;
	int                         VertexBufferSize;
	int                         IndexBufferSize;

	ImGui_ImplDX11_Data() { memset((void*)this, 0, sizeof(*this)); VertexBufferSize = 5000; IndexBufferSize = 10000; }
};

struct VERTEX_CONSTANT_BUFFER
{
	float mvp[4][4];
};

struct WAVEFORM_CONSTANT_BUFFER
{
	struct { vec2 Pos, UV; } PerVertex[6];
	struct { vec2 Size, SizeInv; } CB_RectSize;
	struct { f32 R, G, B, A; } Color;
	float Amplitudes[CustomDraw::WaveformPixelsPerChunk];
};

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static ImGui_ImplDX11_Data* ImGui_ImplDX11_GetBackendData()
{
	return ImGui::GetCurrentContext() ? (ImGui_ImplDX11_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

// Forward Declarations
static void ImGui_ImplDX11_InitPlatformInterface();
static void ImGui_ImplDX11_ShutdownPlatformInterface();
namespace CustomDraw
{
	static void DX11RenderInit(ImGui_ImplDX11_Data* bd);
	static void DX11BeginRenderDrawData(ImDrawData* drawData);
	static void DX11EndRenderDrawData(ImDrawData* drawData);
	static void DX11ReleaseDeferedResources(ImGui_ImplDX11_Data* bd);
}

// Functions
static void ImGui_ImplDX11_SetupRenderState(ImDrawData* draw_data, ID3D11DeviceContext* ctx)
{
	ImGui_ImplDX11_Data* bd = ImGui_ImplDX11_GetBackendData();

	// Setup viewport
	D3D11_VIEWPORT vp = {};
	vp.Width = draw_data->DisplaySize.x;
	vp.Height = draw_data->DisplaySize.y;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = vp.TopLeftY = 0;
	ctx->RSSetViewports(1, &vp);

	// Setup shader and vertex buffers
	unsigned int stride = sizeof(ImDrawVert);
	unsigned int offset = 0;
	ctx->IASetInputLayout(bd->InputLayout);
	ctx->IASetVertexBuffers(0, 1, &bd->VertexBuffer, &stride, &offset);
	ctx->IASetIndexBuffer(bd->IndexBuffer, sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ctx->VSSetShader(bd->DefaultVS, nullptr, 0);
	ID3D11Buffer* constantBuffers[] = { bd->VertexConstantBuffer, bd->WaveformConstantBuffer };
	ctx->VSSetConstantBuffers(0, 2, constantBuffers);
	ctx->PSSetConstantBuffers(0, 2, constantBuffers);
	ctx->PSSetShader(bd->DefaultPS, nullptr, 0);
	ctx->PSSetSamplers(0, 1, &bd->FontSampler);
	ctx->GSSetShader(nullptr, nullptr, 0);
	ctx->HSSetShader(nullptr, nullptr, 0); // In theory we should backup and restore this as well.. very infrequently used..
	ctx->DSSetShader(nullptr, nullptr, 0); // In theory we should backup and restore this as well.. very infrequently used..
	ctx->CSSetShader(nullptr, nullptr, 0); // In theory we should backup and restore this as well.. very infrequently used..

	// Setup blend state
	const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
	ctx->OMSetBlendState(bd->BlendState, blend_factor, 0xffffffff);
	ctx->OMSetDepthStencilState(bd->DepthStencilState, 0);
	ctx->RSSetState(bd->RasterizerState);
}

// Render function
void ImGui_ImplDX11_RenderDrawData(ImDrawData* draw_data)
{
	// Avoid rendering when minimized
	if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
		return;

	ImGui_ImplDX11_Data* bd = ImGui_ImplDX11_GetBackendData();
	ID3D11DeviceContext* ctx = bd->D3DDeviceContext;

	// Create and grow vertex/index buffers if needed
	if (!bd->VertexBuffer || bd->VertexBufferSize < draw_data->TotalVtxCount)
	{
		if (bd->VertexBuffer) { bd->VertexBuffer->Release(); bd->VertexBuffer = nullptr; }
		bd->VertexBufferSize = draw_data->TotalVtxCount + 5000;
		D3D11_BUFFER_DESC desc = {};
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.ByteWidth = bd->VertexBufferSize * sizeof(ImDrawVert);
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		if (bd->D3DDevice->CreateBuffer(&desc, nullptr, &bd->VertexBuffer) < 0)
			return;
	}
	if (!bd->IndexBuffer || bd->IndexBufferSize < draw_data->TotalIdxCount)
	{
		if (bd->IndexBuffer) { bd->IndexBuffer->Release(); bd->IndexBuffer = nullptr; }
		bd->IndexBufferSize = draw_data->TotalIdxCount + 10000;
		D3D11_BUFFER_DESC desc = {};
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.ByteWidth = bd->IndexBufferSize * sizeof(ImDrawIdx);
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		if (bd->D3DDevice->CreateBuffer(&desc, nullptr, &bd->IndexBuffer) < 0)
			return;
	}

	// Upload vertex/index data into a single contiguous GPU buffer
	D3D11_MAPPED_SUBRESOURCE vtx_resource, idx_resource;
	if (ctx->Map(bd->VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &vtx_resource) != S_OK)
		return;
	if (ctx->Map(bd->IndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &idx_resource) != S_OK)
		return;
	ImDrawVert* vtx_dst = (ImDrawVert*)vtx_resource.pData;
	ImDrawIdx* idx_dst = (ImDrawIdx*)idx_resource.pData;
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtx_dst += cmd_list->VtxBuffer.Size;
		idx_dst += cmd_list->IdxBuffer.Size;
	}
	ctx->Unmap(bd->VertexBuffer, 0);
	ctx->Unmap(bd->IndexBuffer, 0);

	// Setup orthographic projection matrix into our constant buffer
	// Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
	{
		D3D11_MAPPED_SUBRESOURCE mapped_resource;
		if (ctx->Map(bd->VertexConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource) != S_OK)
			return;
		VERTEX_CONSTANT_BUFFER* constant_buffer = (VERTEX_CONSTANT_BUFFER*)mapped_resource.pData;
		float L = draw_data->DisplayPos.x;
		float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
		float T = draw_data->DisplayPos.y;
		float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
		float mvp[4][4] =
		{
			{ 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
			{ 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
			{ 0.0f,         0.0f,           0.5f,       0.0f },
			{ (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
		};
		memcpy(&constant_buffer->mvp, mvp, sizeof(mvp));
		ctx->Unmap(bd->VertexConstantBuffer, 0);
	}

	// Backup DX state that will be modified to restore it afterwards (unfortunately this is very ugly looking and verbose. Close your eyes!)
	struct BACKUP_DX11_STATE
	{
		UINT                        ScissorRectsCount, ViewportsCount;
		D3D11_RECT                  ScissorRects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
		D3D11_VIEWPORT              Viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
		ID3D11RasterizerState*      RS;
		ID3D11BlendState*           BlendState;
		FLOAT                       BlendFactor[4];
		UINT                        SampleMask;
		UINT                        StencilRef;
		ID3D11DepthStencilState*    DepthStencilState;
		ID3D11ShaderResourceView*   PSShaderResource;
		ID3D11SamplerState*         PSSampler;
		ID3D11PixelShader*          PS;
		ID3D11VertexShader*         VS;
		ID3D11GeometryShader*       GS;
		UINT                        PSInstancesCount, VSInstancesCount, GSInstancesCount;
		ID3D11ClassInstance         *PSInstances[256], *VSInstances[256], *GSInstances[256];   // 256 is max according to PSSetShader documentation
		D3D11_PRIMITIVE_TOPOLOGY    PrimitiveTopology;
		ID3D11Buffer*               IndexBuffer, *VertexBuffer, *VSConstantBuffer;
		UINT                        IndexBufferOffset, VertexBufferStride, VertexBufferOffset;
		DXGI_FORMAT                 IndexBufferFormat;
		ID3D11InputLayout*          InputLayout;
	};
	BACKUP_DX11_STATE old = {};
	old.ScissorRectsCount = old.ViewportsCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
	ctx->RSGetScissorRects(&old.ScissorRectsCount, old.ScissorRects);
	ctx->RSGetViewports(&old.ViewportsCount, old.Viewports);
	ctx->RSGetState(&old.RS);
	ctx->OMGetBlendState(&old.BlendState, old.BlendFactor, &old.SampleMask);
	ctx->OMGetDepthStencilState(&old.DepthStencilState, &old.StencilRef);
	ctx->PSGetShaderResources(0, 1, &old.PSShaderResource);
	ctx->PSGetSamplers(0, 1, &old.PSSampler);
	old.PSInstancesCount = old.VSInstancesCount = old.GSInstancesCount = 256;
	ctx->PSGetShader(&old.PS, old.PSInstances, &old.PSInstancesCount);
	ctx->VSGetShader(&old.VS, old.VSInstances, &old.VSInstancesCount);
	ctx->VSGetConstantBuffers(0, 1, &old.VSConstantBuffer);
	ctx->GSGetShader(&old.GS, old.GSInstances, &old.GSInstancesCount);

	ctx->IAGetPrimitiveTopology(&old.PrimitiveTopology);
	ctx->IAGetIndexBuffer(&old.IndexBuffer, &old.IndexBufferFormat, &old.IndexBufferOffset);
	ctx->IAGetVertexBuffers(0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset);
	ctx->IAGetInputLayout(&old.InputLayout);

	// Setup desired DX state
	ImGui_ImplDX11_SetupRenderState(draw_data, ctx);
	CustomDraw::DX11BeginRenderDrawData(draw_data);

	// Render command lists
	// (Because we merged all buffers into a single one, we maintain our own offset into them)
	int global_idx_offset = 0;
	int global_vtx_offset = 0;
	ImVec2 clip_off = draw_data->DisplayPos;
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback != nullptr)
			{
				// User callback, registered via ImDrawList::AddCallback()
				// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
				if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
					ImGui_ImplDX11_SetupRenderState(draw_data, ctx);
				else
					pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				// Project scissor/clipping rectangles into framebuffer space
				ImVec2 clip_min(pcmd->ClipRect.x - clip_off.x, pcmd->ClipRect.y - clip_off.y);
				ImVec2 clip_max(pcmd->ClipRect.z - clip_off.x, pcmd->ClipRect.w - clip_off.y);
				if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
					continue;

				// Apply scissor/clipping rectangle
				const D3D11_RECT r = { (LONG)clip_min.x, (LONG)clip_min.y, (LONG)clip_max.x, (LONG)clip_max.y };
				ctx->RSSetScissorRects(1, &r);

				// Bind texture, Draw
				ID3D11ShaderResourceView* texture_srv = (ID3D11ShaderResourceView*)pcmd->GetTexID();
				ctx->PSSetShaderResources(0, 1, &texture_srv);
				ctx->DrawIndexed(pcmd->ElemCount, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset);
			}
		}
		global_idx_offset += cmd_list->IdxBuffer.Size;
		global_vtx_offset += cmd_list->VtxBuffer.Size;
	}

	CustomDraw::DX11EndRenderDrawData(draw_data);

	// Restore modified DX state
	ctx->RSSetScissorRects(old.ScissorRectsCount, old.ScissorRects);
	ctx->RSSetViewports(old.ViewportsCount, old.Viewports);
	ctx->RSSetState(old.RS); if (old.RS) old.RS->Release();
	ctx->OMSetBlendState(old.BlendState, old.BlendFactor, old.SampleMask); if (old.BlendState) old.BlendState->Release();
	ctx->OMSetDepthStencilState(old.DepthStencilState, old.StencilRef); if (old.DepthStencilState) old.DepthStencilState->Release();
	ctx->PSSetShaderResources(0, 1, &old.PSShaderResource); if (old.PSShaderResource) old.PSShaderResource->Release();
	ctx->PSSetSamplers(0, 1, &old.PSSampler); if (old.PSSampler) old.PSSampler->Release();
	ctx->PSSetShader(old.PS, old.PSInstances, old.PSInstancesCount); if (old.PS) old.PS->Release();
	for (UINT i = 0; i < old.PSInstancesCount; i++) if (old.PSInstances[i]) old.PSInstances[i]->Release();
	ctx->VSSetShader(old.VS, old.VSInstances, old.VSInstancesCount); if (old.VS) old.VS->Release();
	ctx->VSSetConstantBuffers(0, 1, &old.VSConstantBuffer); if (old.VSConstantBuffer) old.VSConstantBuffer->Release();
	ctx->GSSetShader(old.GS, old.GSInstances, old.GSInstancesCount); if (old.GS) old.GS->Release();
	for (UINT i = 0; i < old.VSInstancesCount; i++) if (old.VSInstances[i]) old.VSInstances[i]->Release();
	ctx->IASetPrimitiveTopology(old.PrimitiveTopology);
	ctx->IASetIndexBuffer(old.IndexBuffer, old.IndexBufferFormat, old.IndexBufferOffset); if (old.IndexBuffer) old.IndexBuffer->Release();
	ctx->IASetVertexBuffers(0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset); if (old.VertexBuffer) old.VertexBuffer->Release();
	ctx->IASetInputLayout(old.InputLayout); if (old.InputLayout) old.InputLayout->Release();
}

static void ImGui_ImplDX11_CreateFontsTexture()
{
	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplDX11_Data* bd = ImGui_ImplDX11_GetBackendData();
	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	// Upload texture to graphics system
	{
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;

		ID3D11Texture2D* pTexture = nullptr;
		D3D11_SUBRESOURCE_DATA subResource;
		subResource.pSysMem = pixels;
		subResource.SysMemPitch = desc.Width * 4;
		subResource.SysMemSlicePitch = 0;
		bd->D3DDevice->CreateTexture2D(&desc, &subResource, &pTexture);
		IM_ASSERT(pTexture != nullptr);

		// Create texture view
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = desc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		bd->D3DDevice->CreateShaderResourceView(pTexture, &srvDesc, &bd->FontTextureView);
		pTexture->Release();
	}

	// Store our identifier
	io.Fonts->SetTexID((ImTextureID)bd->FontTextureView);

	// Create texture sampler
	{
		D3D11_SAMPLER_DESC desc = {};
		desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.MipLODBias = 0.f;
		desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		desc.MinLOD = 0.f;
		desc.MaxLOD = 0.f;
		bd->D3DDevice->CreateSamplerState(&desc, &bd->FontSampler);
	}
}

bool ImGui_ImplDX11_CreateDeviceObjects()
{
	ImGui_ImplDX11_Data* bd = ImGui_ImplDX11_GetBackendData();
	if (!bd->D3DDevice)
		return false;
	if (bd->FontSampler)
		ImGui_ImplDX11_InvalidateDeviceObjects();

	// Create the vertex shader
	{
		if (bd->D3DDevice->CreateVertexShader(shader_imgui_default_vs_bytecode, sizeof(shader_imgui_default_vs_bytecode), nullptr, &bd->DefaultVS) != S_OK)
			return false;

		if (bd->D3DDevice->CreateVertexShader(shader_imgui_waveform_vs_bytecode, sizeof(shader_imgui_waveform_vs_bytecode), nullptr, &bd->WaveformVS) != S_OK)
			return false;

		// Create the input layout
		D3D11_INPUT_ELEMENT_DESC local_layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, uv),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)IM_OFFSETOF(ImDrawVert, col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		if (bd->D3DDevice->CreateInputLayout(local_layout, 3, shader_imgui_default_vs_bytecode, sizeof(shader_imgui_default_vs_bytecode), &bd->InputLayout) != S_OK)
			return false;

		// Create the constant buffer
		{
			D3D11_BUFFER_DESC desc;
			desc.ByteWidth = sizeof(VERTEX_CONSTANT_BUFFER);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			bd->D3DDevice->CreateBuffer(&desc, nullptr, &bd->VertexConstantBuffer);

			desc.ByteWidth = sizeof(WAVEFORM_CONSTANT_BUFFER);
			bd->D3DDevice->CreateBuffer(&desc, nullptr, &bd->WaveformConstantBuffer);
		}
	}

	// Create the pixel shader
	{
		if (bd->D3DDevice->CreatePixelShader(shader_imgui_default_ps_bytecode, sizeof(shader_imgui_default_ps_bytecode), nullptr, &bd->DefaultPS) != S_OK)
			return false;

		if (bd->D3DDevice->CreatePixelShader(shader_imgui_waveform_ps_bytecode, sizeof(shader_imgui_waveform_ps_bytecode), nullptr, &bd->WaveformPS) != S_OK)
			return false;
	}

	// Create the blending setup
	{
		D3D11_BLEND_DESC desc = {};
		desc.AlphaToCoverageEnable = false;
		desc.RenderTarget[0].BlendEnable = true;
		desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		bd->D3DDevice->CreateBlendState(&desc, &bd->BlendState);
	}

	// Create the rasterizer state
	{
		D3D11_RASTERIZER_DESC desc = {};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_NONE;
		desc.ScissorEnable = true;
		desc.DepthClipEnable = true;
		bd->D3DDevice->CreateRasterizerState(&desc, &bd->RasterizerState);
	}

	// Create depth-stencil State
	{
		D3D11_DEPTH_STENCIL_DESC desc = {};
		desc.DepthEnable = false;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
		desc.StencilEnable = false;
		desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		desc.BackFace = desc.FrontFace;
		bd->D3DDevice->CreateDepthStencilState(&desc, &bd->DepthStencilState);
	}

	ImGui_ImplDX11_CreateFontsTexture();

	return true;
}

void ImGui_ImplDX11_InvalidateDeviceObjects()
{
	ImGui_ImplDX11_Data* bd = ImGui_ImplDX11_GetBackendData();
	if (!bd->D3DDevice)
		return;

	if (bd->FontSampler) { bd->FontSampler->Release(); bd->FontSampler = nullptr; }
	if (bd->FontTextureView) { bd->FontTextureView->Release(); bd->FontTextureView = nullptr; ImGui::GetIO().Fonts->SetTexID(nullptr); } // We copied data->pFontTextureView to io.Fonts->TexID so let's clear that as well.
	if (bd->IndexBuffer) { bd->IndexBuffer->Release(); bd->IndexBuffer = nullptr; }
	if (bd->VertexBuffer) { bd->VertexBuffer->Release(); bd->VertexBuffer = nullptr; }
	if (bd->BlendState) { bd->BlendState->Release(); bd->BlendState = nullptr; }
	if (bd->DepthStencilState) { bd->DepthStencilState->Release(); bd->DepthStencilState = nullptr; }
	if (bd->RasterizerState) { bd->RasterizerState->Release(); bd->RasterizerState = nullptr; }
	if (bd->DefaultPS) { bd->DefaultPS->Release(); bd->DefaultPS = nullptr; }
	if (bd->WaveformPS) { bd->WaveformPS->Release(); bd->WaveformPS = nullptr; }
	if (bd->WaveformConstantBuffer) { bd->WaveformConstantBuffer->Release(); bd->WaveformConstantBuffer = nullptr; }
	if (bd->VertexConstantBuffer) { bd->VertexConstantBuffer->Release(); bd->VertexConstantBuffer = nullptr; }
	if (bd->InputLayout) { bd->InputLayout->Release(); bd->InputLayout = nullptr; }
	if (bd->DefaultVS) { bd->DefaultVS->Release(); bd->DefaultVS = nullptr; }
	if (bd->WaveformVS) { bd->WaveformVS->Release(); bd->WaveformVS = nullptr; }
}

bool ImGui_ImplDX11_RecreateFontTexture()
{
	ImGui_ImplDX11_Data* bd = ImGui_ImplDX11_GetBackendData();
	if (!bd->D3DDevice)
		return false;

	if (bd->FontSampler) { bd->FontSampler->Release(); bd->FontSampler = nullptr; }
	if (bd->FontTextureView) { bd->FontTextureView->Release(); bd->FontTextureView = nullptr; ImGui::GetIO().Fonts->SetTexID(nullptr); } // We copied data->pFontTextureView to io.Fonts->TexID so let's clear that as well.
	ImGui_ImplDX11_CreateFontsTexture();
	return true;
}

bool ImGui_ImplDX11_Init(ID3D11Device* device, ID3D11DeviceContext* device_context)
{
	ImGuiIO& io = ImGui::GetIO();
	IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

	// Setup backend capabilities flags
	ImGui_ImplDX11_Data* bd = IM_NEW(ImGui_ImplDX11_Data)();
	io.BackendRendererUserData = (void*)bd;
	io.BackendRendererName = "imgui_impl_dx11";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
	io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;  // We can create multi-viewports on the Renderer side (optional)

	// Get factory from device
	IDXGIDevice* pDXGIDevice = nullptr;
	IDXGIAdapter* pDXGIAdapter = nullptr;
	IDXGIFactory* pFactory = nullptr;

	if (device->QueryInterface(IID_PPV_ARGS(&pDXGIDevice)) == S_OK)
		if (pDXGIDevice->GetParent(IID_PPV_ARGS(&pDXGIAdapter)) == S_OK)
			if (pDXGIAdapter->GetParent(IID_PPV_ARGS(&pFactory)) == S_OK)
			{
				bd->D3DDevice = device;
				bd->D3DDeviceContext = device_context;
				bd->DXGIFactory = pFactory;
			}
	if (pDXGIDevice) pDXGIDevice->Release();
	if (pDXGIAdapter) pDXGIAdapter->Release();
	bd->D3DDevice->AddRef();
	bd->D3DDeviceContext->AddRef();

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		ImGui_ImplDX11_InitPlatformInterface();

	CustomDraw::DX11RenderInit(bd);

	return true;
}

void ImGui_ImplDX11_Shutdown()
{
	ImGui_ImplDX11_Data* bd = ImGui_ImplDX11_GetBackendData();
	IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
	ImGuiIO& io = ImGui::GetIO();

	CustomDraw::DX11ReleaseDeferedResources(bd);

	ImGui_ImplDX11_ShutdownPlatformInterface();
	ImGui_ImplDX11_InvalidateDeviceObjects();
	if (bd->DXGIFactory) { bd->DXGIFactory->Release(); }
	if (bd->D3DDevice) { bd->D3DDevice->Release(); }
	if (bd->D3DDeviceContext) { bd->D3DDeviceContext->Release(); }
	io.BackendRendererName = nullptr;
	io.BackendRendererUserData = nullptr;
	IM_DELETE(bd);
}

void ImGui_ImplDX11_NewFrame()
{
	ImGui_ImplDX11_Data* bd = ImGui_ImplDX11_GetBackendData();
	IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplDX11_Init()?");

	if (!bd->FontSampler)
		ImGui_ImplDX11_CreateDeviceObjects();

	CustomDraw::DX11ReleaseDeferedResources(bd);
}

//--------------------------------------------------------------------------------------------------------
// MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
// This is an _advanced_ and _optional_ feature, allowing the backend to create and handle multiple viewports simultaneously.
// If you are new to dear imgui or creating a new binding for dear imgui, it is recommended that you completely ignore this section first..
//--------------------------------------------------------------------------------------------------------

// Helper structure we store in the void* RenderUserData field of each ImGuiViewport to easily retrieve our backend data.
struct ImGui_ImplDX11_ViewportData
{
	IDXGISwapChain*                 SwapChain;
	ID3D11RenderTargetView*         RTView;

	ImGui_ImplDX11_ViewportData() { SwapChain = nullptr; RTView = nullptr; }
	~ImGui_ImplDX11_ViewportData() { IM_ASSERT(SwapChain == nullptr && RTView == nullptr); }
};

static void ImGui_ImplDX11_CreateWindow(ImGuiViewport* viewport)
{
	ImGui_ImplDX11_Data* bd = ImGui_ImplDX11_GetBackendData();
	ImGui_ImplDX11_ViewportData* vd = IM_NEW(ImGui_ImplDX11_ViewportData)();
	viewport->RendererUserData = vd;

	// PlatformHandleRaw should always be a HWND, whereas PlatformHandle might be a higher-level handle (e.g. GLFWWindow*, SDL_Window*).
	// Some backend will leave PlatformHandleRaw NULL, in which case we assume PlatformHandle will contain the HWND.
	HWND hwnd = viewport->PlatformHandleRaw ? (HWND)viewport->PlatformHandleRaw : (HWND)viewport->PlatformHandle;
	IM_ASSERT(hwnd != 0);

	// Create swap chain
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferDesc.Width = (UINT)viewport->Size.x;
	sd.BufferDesc.Height = (UINT)viewport->Size.y;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 1;
	sd.OutputWindow = hwnd;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = 0;

	IM_ASSERT(vd->SwapChain == nullptr && vd->RTView == nullptr);
	bd->DXGIFactory->CreateSwapChain(bd->D3DDevice, &sd, &vd->SwapChain);
	bd->DXGIFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

	// Create the render target
	if (vd->SwapChain)
	{
		ID3D11Texture2D* pBackBuffer;
		vd->SwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
		bd->D3DDevice->CreateRenderTargetView(pBackBuffer, nullptr, &vd->RTView);
		pBackBuffer->Release();
	}
}

static void ImGui_ImplDX11_DestroyWindow(ImGuiViewport* viewport)
{
	// The main viewport (owned by the application) will always have RendererUserData == NULL since we didn't create the data for it.
	if (ImGui_ImplDX11_ViewportData* vd = (ImGui_ImplDX11_ViewportData*)viewport->RendererUserData)
	{
		if (vd->SwapChain)
			vd->SwapChain->Release();
		vd->SwapChain = nullptr;
		if (vd->RTView)
			vd->RTView->Release();
		vd->RTView = nullptr;
		IM_DELETE(vd);
	}
	viewport->RendererUserData = nullptr;
}

static void ImGui_ImplDX11_SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
	ImGui_ImplDX11_Data* bd = ImGui_ImplDX11_GetBackendData();
	ImGui_ImplDX11_ViewportData* vd = (ImGui_ImplDX11_ViewportData*)viewport->RendererUserData;
	if (vd->RTView)
	{
		vd->RTView->Release();
		vd->RTView = nullptr;
	}
	if (vd->SwapChain)
	{
		ID3D11Texture2D* pBackBuffer = nullptr;
		vd->SwapChain->ResizeBuffers(0, (UINT)size.x, (UINT)size.y, DXGI_FORMAT_UNKNOWN, 0);
		vd->SwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
		if (pBackBuffer == nullptr) { fprintf(stderr, "ImGui_ImplDX11_SetWindowSize() failed creating buffers.\n"); return; }
		bd->D3DDevice->CreateRenderTargetView(pBackBuffer, nullptr, &vd->RTView);
		pBackBuffer->Release();
	}
}

static void ImGui_ImplDX11_RenderWindow(ImGuiViewport* viewport, void*)
{
	ImGui_ImplDX11_Data* bd = ImGui_ImplDX11_GetBackendData();
	ImGui_ImplDX11_ViewportData* vd = (ImGui_ImplDX11_ViewportData*)viewport->RendererUserData;
	ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	bd->D3DDeviceContext->OMSetRenderTargets(1, &vd->RTView, nullptr);
	if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear))
		bd->D3DDeviceContext->ClearRenderTargetView(vd->RTView, (float*)&clear_color);
	ImGui_ImplDX11_RenderDrawData(viewport->DrawData);
}

static void ImGui_ImplDX11_SwapBuffers(ImGuiViewport* viewport, void*)
{
	ImGui_ImplDX11_ViewportData* vd = (ImGui_ImplDX11_ViewportData*)viewport->RendererUserData;
	vd->SwapChain->Present(0, 0); // Present without vsync
}

static void ImGui_ImplDX11_InitPlatformInterface()
{
	ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
	platform_io.Renderer_CreateWindow = ImGui_ImplDX11_CreateWindow;
	platform_io.Renderer_DestroyWindow = ImGui_ImplDX11_DestroyWindow;
	platform_io.Renderer_SetWindowSize = ImGui_ImplDX11_SetWindowSize;
	platform_io.Renderer_RenderWindow = ImGui_ImplDX11_RenderWindow;
	platform_io.Renderer_SwapBuffers = ImGui_ImplDX11_SwapBuffers;
}

static void ImGui_ImplDX11_ShutdownPlatformInterface()
{
	ImGui::DestroyPlatformWindows();
}

namespace CustomDraw
{
	struct DX11GPUTextureData
	{
		// NOTE: An ID of 0 denotes an empty slot
		u32 GenerationID;
		GPUTextureDesc Desc;
		ID3D11Texture2D* Texture2D;
		ID3D11ShaderResourceView* ResourceView;
	};

	// NOTE: First valid ID starts at 1
	static u32 LastTextureGenreationID;
	static std::vector<DX11GPUTextureData> LoadedTextureSlots;
	static std::vector<ID3D11DeviceChild*> DeviceResourcesToDeferRelease;

	inline DX11GPUTextureData* ResolveHandle(GPUTextureHandle handle)
	{
		auto& slots = LoadedTextureSlots;
		return (handle.GenerationID != 0) && (handle.SlotIndex < slots.size()) && (slots[handle.SlotIndex].GenerationID == handle.GenerationID) ? &slots[handle.SlotIndex] : nullptr;
	}

	void GPUTexture::Load(const GPUTextureDesc& desc)
	{
		ImGui_ImplDX11_Data* bd = ImGui_ImplDX11_GetBackendData();
		assert(ResolveHandle(Handle) == nullptr);

		DX11GPUTextureData* slot = nullptr;
		for (auto& it : LoadedTextureSlots) { if (it.GenerationID == 0) { slot = &it; break; } }
		if (slot == nullptr) { slot = &LoadedTextureSlots.emplace_back(); }

		slot->GenerationID = ++LastTextureGenreationID;
		slot->Desc = desc;

		HRESULT result = bd->D3DDevice->CreateTexture2D(PtrArg(D3D11_TEXTURE2D_DESC
			{
				static_cast<UINT>(desc.Size.x), static_cast<UINT>(desc.Size.y), 1u, 1u,
				(desc.Format == GPUPixelFormat::RGBA) ? DXGI_FORMAT_R8G8B8A8_UNORM : (desc.Format == GPUPixelFormat::BGRA) ? DXGI_FORMAT_B8G8R8A8_UNORM : DXGI_FORMAT_UNKNOWN,
				DXGI_SAMPLE_DESC { 1u, 0u },
				(desc.Access == GPUAccessType::Dynamic) ? D3D11_USAGE_DYNAMIC : (desc.Access == GPUAccessType::Static) ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT,
				D3D11_BIND_SHADER_RESOURCE,
				(desc.Access == GPUAccessType::Dynamic) ? D3D11_CPU_ACCESS_WRITE : 0u, 0u
			}),
			(desc.InitialPixels != nullptr) ? PtrArg(D3D11_SUBRESOURCE_DATA { desc.InitialPixels, static_cast<UINT>(desc.Size.x * 4), 0u }) : nullptr,
			&slot->Texture2D);
		assert(SUCCEEDED(result));

		result = bd->D3DDevice->CreateShaderResourceView(slot->Texture2D, nullptr, &slot->ResourceView);
		assert(SUCCEEDED(result));

		Handle = GPUTextureHandle { static_cast<u32>(ArrayItToIndex(slot, &LoadedTextureSlots[0])), slot->GenerationID };
	}

	void GPUTexture::Unload()
	{
		if (auto* data = ResolveHandle(Handle); data != nullptr)
		{
			DeviceResourcesToDeferRelease.push_back(data->Texture2D);
			DeviceResourcesToDeferRelease.push_back(data->ResourceView);
			*data = DX11GPUTextureData {};
		}
		Handle = GPUTextureHandle {};
	}

	void GPUTexture::UpdateDynamic(ivec2 size, const void* newPixels)
	{
		auto* data = ResolveHandle(Handle);
		if (data == nullptr)
			return;

		assert(data->Desc.Access == GPUAccessType::Dynamic);
		assert(data->Desc.Size == size);
		ImGui_ImplDX11_Data* bd = ImGui_ImplDX11_GetBackendData();

		if (D3D11_MAPPED_SUBRESOURCE mapped; SUCCEEDED(bd->D3DDeviceContext->Map(data->Texture2D, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			const size_t outStride = mapped.RowPitch;
			const size_t outByteSize = mapped.DepthPitch;
			u8* outData = static_cast<u8*>(mapped.pData);
			assert(outData != nullptr);

			assert(data->Desc.Format == GPUPixelFormat::RGBA || data->Desc.Format == GPUPixelFormat::BGRA);
			static constexpr u32 rgbaBitsPerPixel = (sizeof(u32) * BitsPerByte);
			const size_t inStride = (size.x * rgbaBitsPerPixel) / BitsPerByte;
			const size_t inByteSize = (size.y * inStride);
			const u8* inData = static_cast<const u8*>(newPixels);

			if (outByteSize == inByteSize)
			{
				memcpy(outData, inData, inByteSize);
			}
			else
			{
				assert(outByteSize == (outStride * size.x));
				for (size_t y = 0; y < size.y; y++)
					memcpy(&outData[outStride * y], &inData[inStride * y], inStride);
			}

			bd->D3DDeviceContext->Unmap(data->Texture2D, 0);
		}
	}

	b8 GPUTexture::IsValid() const { return (ResolveHandle(Handle) != nullptr); }
	ivec2 GPUTexture::GetSize() const { auto* data = ResolveHandle(Handle); return data ? data->Desc.Size : ivec2(0, 0); }
	vec2 GPUTexture::GetSizeF32() const { return vec2(GetSize()); }
	GPUPixelFormat GPUTexture::GetFormat() const { auto* data = ResolveHandle(Handle); return data ? data->Desc.Format : GPUPixelFormat {}; }
	ImTextureID GPUTexture::GetTexID() const { auto* data = ResolveHandle(Handle); return data ? data->ResourceView : nullptr; }

	// NOTE: The most obvious way to extend this would be to either add an enum command type + a union of parameters
	//		 or (better?) a per command type commands vector with the render callback userdata storing a packed type+index
	struct DX11CustomDrawCommand
	{
		Rect Rect;
		ImVec4 Color;
		CustomDraw::WaveformChunk WaveformChunk;
	};

	static ImDrawData* ThisFrameImDrawData = nullptr;
	static std::vector<DX11CustomDrawCommand> CustomDrawCommandsThisFrame;

	static void DX11RenderInit(ImGui_ImplDX11_Data* bd)
	{
		CustomDrawCommandsThisFrame.reserve(64);
		LoadedTextureSlots.reserve(8);
		DeviceResourcesToDeferRelease.reserve(LoadedTextureSlots.capacity() * 2);
	}

	static void DX11BeginRenderDrawData(ImDrawData* drawData)
	{
		ThisFrameImDrawData = drawData;
	}

	static void DX11EndRenderDrawData(ImDrawData* drawData)
	{
		assert(drawData == ThisFrameImDrawData);
		ThisFrameImDrawData = nullptr;
		CustomDrawCommandsThisFrame.clear();
	}

	static void DX11ReleaseDeferedResources(ImGui_ImplDX11_Data* bd)
	{
		assert(bd != nullptr && bd->D3DDevice != nullptr);

		if (!DeviceResourcesToDeferRelease.empty())
		{
			for (ID3D11DeviceChild* it : DeviceResourcesToDeferRelease) { if (it != nullptr) it->Release(); }
			DeviceResourcesToDeferRelease.clear();
		}
	}

	void DrawWaveformChunk(ImDrawList* drawList, Rect rect, u32 color, const WaveformChunk& chunk)
	{
		ImDrawCallback callback = [](const ImDrawList* parentList, const ImDrawCmd* cmd)
		{
			static_assert(sizeof(WAVEFORM_CONSTANT_BUFFER::Amplitudes) == sizeof(WaveformChunk::PerPixelAmplitude));
			const DX11CustomDrawCommand& customCommand = CustomDrawCommandsThisFrame[reinterpret_cast<size_t>(cmd->UserCallbackData)];

			ImGui_ImplDX11_Data* bd = ImGui_ImplDX11_GetBackendData();
			ID3D11DeviceContext* ctx = bd->D3DDeviceContext;

			if (D3D11_MAPPED_SUBRESOURCE mapped; ctx->Map(bd->WaveformConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped) == S_OK)
			{
				constexpr vec2 uvTL = vec2(0.0f, 0.0f); constexpr vec2 uvTR = vec2(1.0f, 0.0f);
				constexpr vec2 uvBL = vec2(0.0f, 1.0f); constexpr vec2 uvBR = vec2(1.0f, 1.0f);

				WAVEFORM_CONSTANT_BUFFER* data = static_cast<WAVEFORM_CONSTANT_BUFFER*>(mapped.pData);
				data->PerVertex[0] = { customCommand.Rect.GetBL(), uvBL };
				data->PerVertex[1] = { customCommand.Rect.GetBR(), uvBR };
				data->PerVertex[2] = { customCommand.Rect.GetTR(), uvTR };
				data->PerVertex[3] = { customCommand.Rect.GetBL(), uvBL };
				data->PerVertex[4] = { customCommand.Rect.GetTR(), uvTR };
				data->PerVertex[5] = { customCommand.Rect.GetTL(), uvTL };
				data->CB_RectSize = { customCommand.Rect.GetSize(), vec2(1.0f) / customCommand.Rect.GetSize() };
				data->Color = { customCommand.Color.x, customCommand.Color.y, customCommand.Color.z, customCommand.Color.w };
				memcpy(&data->Amplitudes, &customCommand.WaveformChunk.PerPixelAmplitude, sizeof(data->Amplitudes));
				ctx->Unmap(bd->WaveformConstantBuffer, 0);
			}

			// HACK: Duplicated from regular render command loop
			ImVec2 clip_off = ThisFrameImDrawData->DisplayPos;
			ImVec2 clip_min(cmd->ClipRect.x - clip_off.x, cmd->ClipRect.y - clip_off.y);
			ImVec2 clip_max(cmd->ClipRect.z - clip_off.x, cmd->ClipRect.w - clip_off.y);
			if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
				return;

			const D3D11_RECT r = { (LONG)clip_min.x, (LONG)clip_min.y, (LONG)clip_max.x, (LONG)clip_max.y };
			ctx->RSSetScissorRects(1, &r);

			ctx->VSSetShader(bd->WaveformVS, nullptr, 0);
			ctx->PSSetShader(bd->WaveformPS, nullptr, 0);
			ctx->Draw(6, 0);

			// HACK: Always reset state for now even if it's immediately set back by the next render command
			ctx->VSSetShader(bd->DefaultVS, nullptr, 0);
			ctx->PSSetShader(bd->DefaultPS, nullptr, 0);
		};

		void* userData = reinterpret_cast<void*>(CustomDrawCommandsThisFrame.size());
		CustomDrawCommandsThisFrame.push_back(DX11CustomDrawCommand { rect, ImGui::ColorConvertU32ToFloat4(color), chunk });

		drawList->AddCallback(callback, userData);
	}
}
