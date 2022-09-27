#pragma once
#include "core_types.h"
#include "imgui/3rdparty/imgui.h"
#include "imgui/3rdparty/imgui_internal.h"

// NOTE: Special custom drawing related routines not usually possible via standard Dear ImGui
//		 but explictiply implemented for each render backend and made accessible here
namespace CustomDraw
{
	enum class GPUPixelFormat : u8 { RGBA, BGRA, Count };
	enum class GPUAccessType : u8 { Static, Dynamic, Count };

	struct GPUTextureDesc
	{
		GPUPixelFormat Format;
		GPUAccessType Access;
		ivec2 Size;
		const void* InitialPixels;
	};

	struct GPUTextureHandle { u32 SlotIndex, GenerationID; };

	struct GPUTexture
	{
		GPUTextureHandle Handle = {};

		void Load(const GPUTextureDesc& desc);
		void Unload();
		void UpdateDynamic(ivec2 size, const void* newPixels);
		b8 IsValid() const;
		ivec2 GetSize() const;
		vec2 GetSizeF32() const;
		GPUPixelFormat GetFormat() const;
		ImTextureID GetTexID() const;
	};

	constexpr i32 WaveformPixelsPerChunk = 256;
	struct WaveformChunk { f32 PerPixelAmplitude[WaveformPixelsPerChunk]; };
	void DrawWaveformChunk(ImDrawList* drawList, Rect rect, u32 color, const WaveformChunk& chunk);
}
