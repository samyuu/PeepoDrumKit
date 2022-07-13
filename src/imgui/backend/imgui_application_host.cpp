﻿#include "imgui_application_host.h"
#include "imgui/3rdparty/imgui.h"
#include "imgui/3rdparty/imgui_internal.h"
#include "imgui/backend/imgui_impl_win32.h"
#include "imgui/backend/imgui_impl_d3d11.h"
#include "imgui/extension/imgui_input_binding.h"

#include "core_io.h"
#include "core_string.h"
#include "../src_res/resource.h"

#include <d3d11.h>
#include <dxgi1_3.h>

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

struct WindowRectWithDecoration { ivec2 Position, Size; };
struct ClientRect { ivec2 Position, Size; };

static WindowRectWithDecoration Win32ClientToWindowRect(HWND hwnd, DWORD windowStyle, ClientRect clientRect)
{
	RECT inClientOutWindow;
	inClientOutWindow.left = clientRect.Position.x;
	inClientOutWindow.top = clientRect.Position.y;
	inClientOutWindow.right = clientRect.Position.x + clientRect.Size.x;
	inClientOutWindow.bottom = clientRect.Position.y + clientRect.Size.y;
	if (!::AdjustWindowRect(&inClientOutWindow, windowStyle, false))
		return WindowRectWithDecoration { clientRect.Position, clientRect.Size };

	return WindowRectWithDecoration
	{
		ivec2(inClientOutWindow.left, inClientOutWindow.top),
		ivec2(inClientOutWindow.right - inClientOutWindow.left, inClientOutWindow.bottom - inClientOutWindow.top),
	};
}

namespace ApplicationHost
{
	// HACK: Color seen briefly during startup before the first frame has been drawn.
	//		 Should match the primary ImGui style window background color to create a seamless transition.
	//		 Hardcoded for now but should probably be passed in as a parameter / updated dynamically as the ImGui style changes (?)
	static constexpr u32 Win32WindowBackgroundColor = 0x001F1F1F;
	static constexpr f32 D3D11SwapchainClearColor[4] = { 0.12f, 0.12f, 0.12f, 1.0f };
	static constexpr const char* FontFilePath = "assets/NotoSansCJKjp-Regular.otf";

	static LRESULT WINAPI MainWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static ID3D11Device*            GlobalD3D11Device = nullptr;
	static ID3D11DeviceContext*     GlobalD3D11DeviceContext = nullptr;
	static IDXGISwapChain*          GlobalSwapChain = nullptr;
	static DXGI_SWAP_CHAIN_DESC		GlobalSwapChainCreationDesc = {};
	static ID3D11RenderTargetView*  GlobalMainRenderTargetView = nullptr;
	static UpdateFunc				GlobalOnUserUpdate = nullptr;
	static WindowCloseRequestFunc	GlobalOnUserWindowCloseRequest = nullptr;
	static WINDOWPLACEMENT			GlobalPreFullscreenWindowPlacement = {};
	static bool						GlobalIsWindowMinimized = false;
	static bool						GlobalIsWindowBeingDragged = false;
	static UINT_PTR					GlobalWindowRedrawTimerID = {};
	static HANDLE					GlobalSwapChainWaitableObject = NULL;
	static struct { const ImWchar *JP, *EN; } GlobalGlyphRanges = {};
	static ImGuiStyle				GlobalOriginalScaleStyle = {};
	static bool						GlobalIsFirstFrameAfterFontRebuild = true;

	static bool CreateGlobalD3D11(const StartupParam& startupParam, HWND hWnd);
	static void CleanupGlobalD3D11();
	static void CreateGlobalD3D11SwapchainRenderTarget();
	static void CleanupGlobalD3D11SwapchainRenderTarget();

	static void ImGuiUpdateBuildFonts()
	{
		// TODO: Fonts should probably be set up by the application itself instead of being tucked away here but it doesn't really matter too much for now..
		ImGuiIO& io = ImGui::GetIO();

		if (GlobalGlyphRanges.JP == nullptr)
		{
			// NOTE: Using the glyph ranges builder here takes around ~0.15ms in release and ~2ms in debug builds
			static ImVector<ImWchar>		globalRangesJP;
			static ImFontGlyphRangesBuilder globalRangesBuilderJP;

			// HACK: Somewhat arbitrary non-exhaustive list of glyphs sometimes seen in song names etc.
			static constexpr const char additionalGlyphs[] =
				u8"ΔΨαλμχд‐’“”…′※™ⅠⅡⅤⅥⅦⅩ↑→↓∞∫≠⊆⑨▼◆◇"
				u8"○◎★☆♂♡♢♥♦♨♪亰什儚兩凋區叩吠吼咄哭嗚嘘"
				u8"噛囃堡姐孩學對弩彡彷徨怎怯愴戀戈捌掴撥擺朧朶"
				u8"杓棍棕檄欅洩涵渕溟滾漾漿潘澤濤濱炸焉焔爛狗獨"
				u8"琲甜睛筐篭繋繚繧翡舘芒范蔀蔔蔡薇薔薛蘋蘿號蛻"
				u8"裙訶譚變賽逅邂郢雙霍霖靡韶餃驢髭魄麹麼鼠﻿";

			// HACK: The mere 常用 + 人名 kanji of course aren't anywhere near sufficient, **especially** for song names and file paths.
			//		 This *should* include *at least* the ~6000 漢字漢検１級 + common "fancy" unicode characters used as variations of the regular ASCII set.
			//		 Creating a font atlas that big upfront however absolutely kills startup times so the only sane solution is to use dynamic glyph rasterization
			//		 which will hopefully be fully implemented in the not too distant future :Copium: (https://github.com/ocornut/imgui/pull/3471)
			globalRangesBuilderJP.AddText(additionalGlyphs, additionalGlyphs + (ArrayCount(additionalGlyphs) - sizeof('\0')));
			globalRangesBuilderJP.AddRanges(io.Fonts->GetGlyphRangesJapanese());
			globalRangesBuilderJP.BuildRanges(&globalRangesJP);

			// GlobalGlyphRanges.JP = io.Fonts->GetGlyphRangesJapanese();
			GlobalGlyphRanges.JP = globalRangesJP.Data;
			GlobalGlyphRanges.EN = io.Fonts->GetGlyphRangesDefault();
		}

		const std::string_view fontFileName = Path::GetFileName(FontFilePath);

		enum class Ownership : u8 { Copy, Move };
		auto addFont = [&](i32 fontSizePixels, const ImWchar* glyphRanges, Ownership ownership) -> ImFont*
		{
			ImFontConfig fontConfig = {};
			fontConfig.FontDataOwnedByAtlas = (ownership == Ownership::Move) ? true : false;
			fontConfig.EllipsisChar = '\0';
			sprintf_s(fontConfig.Name, "%.*s, %dpx", FmtStrViewArgs(fontFileName), fontSizePixels);

			return io.Fonts->AddFontFromMemoryTTF(GlobalState.FontFileContent, static_cast<int>(GlobalState.FontFileContentSize), static_cast<f32>(fontSizePixels), &fontConfig, glyphRanges);
		};

		const bool rebuild = !io.Fonts->Fonts.empty();
		if (rebuild)
			io.Fonts->Clear();

		// NOTE: Unfortunately Dear ImGui does not allow avoiding these copies at the moment as far as I can tell (except for maybe some super hacky "inject nullptrs before shutdown")
		FontMain_JP = addFont(GuiScaleI32(16), GlobalGlyphRanges.JP, Ownership::Copy);
		FontMedium_EN = addFont(GuiScaleI32(18), GlobalGlyphRanges.EN, Ownership::Copy);
		FontLarge_EN = addFont(GuiScaleI32(22), GlobalGlyphRanges.EN, Ownership::Copy);

		if (rebuild)
			ImGui_ImplDX11_RecreateFontTexture();

		GlobalIsFirstFrameAfterFontRebuild = true;
	}

	static void ImGuiAndUserUpdateThenRenderAndPresentFrame()
	{
		if (!GlobalIsWindowMinimized && GlobalSwapChainWaitableObject != NULL)
			::WaitForSingleObjectEx(GlobalSwapChainWaitableObject, 1000, true);

		if (!ApproxmiatelySame(GuiScaleFactor, GuiScaleFactorToSetNextFrame))
		{
			GuiScaleFactor = RoundAndClampGuiScaleFactor(GuiScaleFactorToSetNextFrame);
			GuiScaleFactorToSetNextFrame = GuiScaleFactor;
			ImGuiUpdateBuildFonts();

			ImGui::GetStyle() = GlobalOriginalScaleStyle;
			if (!ApproxmiatelySame(GuiScaleFactor, 1.0f))
				ImGui::GetStyle().ScaleAllSizes(GuiScaleFactor);
		}

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGui_UpdateInternalInputExtraDataAtStartOfFrame();

		if (GlobalIsFirstFrameAfterFontRebuild)
		{
			// HACK: First stub out with '\0' so that ImFont::BuildLookupTable() doesn't try to overwrite it using FindFirstExistingGlyph()
			//		 then disable using -1 so that the built in ellipsis glyph won't being used (since it doesn't look too great, with "Noto Sans CJK JP" at least)
			for (ImFont* font : ImGui::GetIO().Fonts->Fonts)
			{
				if (font->EllipsisChar == '\0')
					font->EllipsisChar = static_cast<ImWchar>(-1);
			}
			GlobalIsFirstFrameAfterFontRebuild = false;
		}

		assert(GlobalOnUserUpdate != nullptr);
		GlobalOnUserUpdate();

		ImGui::Render();
		ImGui_UpdateInternalInputExtraDataAtEndOfFrame();

		GlobalD3D11DeviceContext->OMSetRenderTargets(1, &GlobalMainRenderTargetView, nullptr);
		GlobalD3D11DeviceContext->ClearRenderTargetView(GlobalMainRenderTargetView, D3D11SwapchainClearColor);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		// TODO: Maybe handle this better somehow, not sure...
		if (!GlobalIsWindowMinimized)
			GlobalSwapChain->Present(GlobalState.SwapInterval, 0);
		else
			::Sleep(33);
	}

	i32 EnterProgramLoop(const StartupParam& startupParam, UserCallbacks userCallbacks)
	{
		ImGui_ImplWin32_EnableDpiAwareness();
		const HICON windowIcon = ::LoadIconW(::GetModuleHandleW(nullptr), MAKEINTRESOURCEW(PEEPO_DRUM_KIT_ICON));

		WNDCLASSEXW windowClass = { sizeof(windowClass), CS_CLASSDC, MainWindowProc, 0L, 0L, ::GetModuleHandleW(nullptr), windowIcon, NULL, ::CreateSolidBrush(Win32WindowBackgroundColor), nullptr, L"ImGuiApplicationHost", NULL };
		::RegisterClassExW(&windowClass);

		static constexpr DWORD windowStyle = WS_OVERLAPPEDWINDOW;
		const HWND hwnd = ::CreateWindowExW(0, windowClass.lpszClassName, UTF8::WideArg(startupParam.WindowTitle).c_str(),
			windowStyle,
			startupParam.WindowPosition.has_value() ? startupParam.WindowPosition->x : CW_USEDEFAULT,
			startupParam.WindowPosition.has_value() ? startupParam.WindowPosition->y : CW_USEDEFAULT,
			startupParam.WindowSize.has_value() ? startupParam.WindowSize->x : CW_USEDEFAULT,
			startupParam.WindowSize.has_value() ? startupParam.WindowSize->y : CW_USEDEFAULT,
			NULL,
			NULL,
			windowClass.hInstance,
			nullptr);

		GlobalState.NativeWindowHandle = hwnd;
		GlobalState.WindowTitle = startupParam.WindowTitle;

		if (!CreateGlobalD3D11(startupParam, hwnd))
		{
			CleanupGlobalD3D11();
			::UnregisterClassW(windowClass.lpszClassName, windowClass.hInstance);
			return 1;
		}

		::ShowWindow(hwnd, SW_SHOWDEFAULT);
		::UpdateWindow(hwnd);
		::DragAcceptFiles(hwnd, true);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.IniFilename = "settings_imgui.ini";

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		ImGui_ImplWin32_Init(hwnd, windowIcon);
		ImGui_ImplDX11_Init(GlobalD3D11Device, GlobalD3D11DeviceContext);

		userCallbacks.OnStartup();

		// TODO: Proper error handling
		GlobalState.FontFileContent = ImFileLoadToMemory(FontFilePath, "rb", &GlobalState.FontFileContentSize, 0);
		assert(GlobalState.FontFileContent != nullptr && GlobalState.FontFileContentSize > 0);
		ImGuiUpdateBuildFonts();

		GuiScaleFactorToSetNextFrame = GuiScaleFactor;
		GlobalOriginalScaleStyle = ImGui::GetStyle();
		if (!ApproxmiatelySame(GuiScaleFactor, 1.0f))
			ImGui::GetStyle().ScaleAllSizes(GuiScaleFactor);

		bool done = false;
		while (!done)
		{
			if (GlobalState.RequestExitNextFrame.has_value())
			{
				const CloseResponse response = (GlobalOnUserWindowCloseRequest != nullptr) ? GlobalOnUserWindowCloseRequest() : CloseResponse::Exit;
				if (response != CloseResponse::SupressExit)
					::PostQuitMessage(GlobalState.RequestExitNextFrame.value());
				GlobalState.RequestExitNextFrame = std::nullopt;
			}

			if (!GlobalState.FilePathsDroppedThisFrame.empty())
				GlobalState.FilePathsDroppedThisFrame.clear();

			MSG msg = {};
			while (::PeekMessageW(&msg, NULL, 0U, 0U, PM_REMOVE))
			{
				::TranslateMessage(&msg);
				::DispatchMessageW(&msg);
				if (msg.message == WM_QUIT)
					done = true;
			}
			if (done)
				break;

			if (!GlobalState.SetWindowTitleNextFrame.empty())
			{
				if (GlobalState.SetWindowTitleNextFrame != GlobalState.WindowTitle)
				{
					::SetWindowTextW(hwnd, UTF8::WideArg(GlobalState.SetWindowTitleNextFrame).c_str());
					GlobalState.WindowTitle = GlobalState.SetWindowTitleNextFrame;
				}
				GlobalState.SetWindowTitleNextFrame.clear();
			}

			if (GlobalState.SetWindowPositionNextFrame.has_value() || GlobalState.SetWindowSizeNextFrame.has_value())
			{
				const ivec2 newPosition = GlobalState.SetWindowPositionNextFrame.value_or(GlobalState.WindowPosition);
				const ivec2 newSize = GlobalState.SetWindowSizeNextFrame.value_or(GlobalState.WindowSize);

				const WindowRectWithDecoration windowRect = Win32ClientToWindowRect(hwnd, windowStyle, ClientRect { newPosition, newSize });
				::SetWindowPos(hwnd, NULL, windowRect.Position.x, windowRect.Position.y, windowRect.Size.x, windowRect.Size.y, SWP_NOZORDER | SWP_NOACTIVATE);

				GlobalState.SetWindowPositionNextFrame = std::nullopt;
				GlobalState.SetWindowSizeNextFrame = std::nullopt;
			}

			if (GlobalState.SetBorderlessFullscreenNextFrame.has_value())
			{
				if (GlobalState.SetBorderlessFullscreenNextFrame.value() != GlobalState.IsBorderlessFullscreen)
				{
					// NOTE: https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
					const DWORD windowStyle = ::GetWindowLongW(hwnd, GWL_STYLE);
					if (windowStyle & WS_OVERLAPPEDWINDOW)
					{
						MONITORINFO monitor = { sizeof(monitor) };
						if (::GetWindowPlacement(hwnd, &GlobalPreFullscreenWindowPlacement) && ::GetMonitorInfoW(::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor))
						{
							::SetWindowLongW(hwnd, GWL_STYLE, windowStyle & ~WS_OVERLAPPEDWINDOW);
							::SetWindowPos(hwnd, HWND_TOP,
								monitor.rcMonitor.left, monitor.rcMonitor.top,
								monitor.rcMonitor.right - monitor.rcMonitor.left,
								monitor.rcMonitor.bottom - monitor.rcMonitor.top,
								SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
							GlobalState.IsBorderlessFullscreen = true;
						}
					}
					else
					{
						::SetWindowLongW(hwnd, GWL_STYLE, windowStyle | WS_OVERLAPPEDWINDOW);
						::SetWindowPlacement(hwnd, &GlobalPreFullscreenWindowPlacement);
						::SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
						GlobalState.IsBorderlessFullscreen = false;
					}
				}
				GlobalState.SetBorderlessFullscreenNextFrame = std::nullopt;
			}

			GlobalOnUserUpdate = userCallbacks.OnUpdate;
			GlobalOnUserWindowCloseRequest = userCallbacks.OnWindowCloseRequest;
			ImGuiAndUserUpdateThenRenderAndPresentFrame();
		}

		userCallbacks.OnShutdown();

		IM_FREE(GlobalState.FontFileContent);
		GlobalState.FontFileContent = nullptr;
		GlobalState.FontFileContentSize = 0;

		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();

		CleanupGlobalD3D11();
		::DestroyWindow(hwnd);
		::UnregisterClassW(windowClass.lpszClassName, windowClass.hInstance);

		return 0;
	}

	static bool CreateGlobalD3D11(const StartupParam& startupParam, HWND hWnd)
	{
		DXGI_SWAP_CHAIN_DESC sd = {};
		sd.BufferCount = 2;
		sd.BufferDesc.Width = 0;
		sd.BufferDesc.Height = 0;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = hWnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = true;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		sd.Flags = 0;

		if (startupParam.WaitableSwapChain)
			sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
		if (startupParam.AllowSwapChainTearing)
			sd.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

		static constexpr UINT deviceFlags = D3D11_CREATE_DEVICE_SINGLETHREADED | (PEEPO_DEBUG ? D3D11_CREATE_DEVICE_DEBUG : 0);
		static constexpr D3D_FEATURE_LEVEL inFeatureLevels[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
		D3D_FEATURE_LEVEL outFeatureLevel = {};

		HRESULT hr = S_OK;
		if (FAILED(hr = ::D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, deviceFlags, inFeatureLevels, ArrayCountI32(inFeatureLevels), D3D11_SDK_VERSION, &sd, &GlobalSwapChain, &GlobalD3D11Device, &outFeatureLevel, &GlobalD3D11DeviceContext)))
		{
			// NOTE: In case FLIP_DISCARD and or FRAME_LATENCY_WAITABLE_OBJECT aren't supported (<= win7?) try again using the regular bitblt model.
			//		 For windows 8.1 specifically it might be better to first check for FLIP_SEQUENTIAL but whatever
			sd.BufferCount = 1;
			sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
			sd.Flags = 0;

			if (FAILED(hr = ::D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, deviceFlags, inFeatureLevels, ArrayCountI32(inFeatureLevels), D3D11_SDK_VERSION, &sd, &GlobalSwapChain, &GlobalD3D11Device, &outFeatureLevel, &GlobalD3D11DeviceContext)))
				return false;
		}

		// NOTE: Disable silly ALT+ENTER fullscreen toggle default behavior
		{
			IDXGIDevice* dxgiDevice = nullptr;
			IDXGIAdapter* dxgiAdapter = nullptr;
			IDXGIFactory* dxgiFactory = nullptr;

			hr = GlobalD3D11Device ? GlobalD3D11Device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)) : E_INVALIDARG;
			hr = dxgiDevice ? dxgiDevice->GetAdapter(&dxgiAdapter) : hr;
			hr = dxgiAdapter ? dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory)) : hr;
			hr = dxgiFactory ? dxgiFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER) : hr;

			if (dxgiFactory) { dxgiFactory->Release(); dxgiFactory = nullptr; }
			if (dxgiAdapter) { dxgiAdapter->Release(); dxgiAdapter = nullptr; }
			if (dxgiDevice) { dxgiDevice->Release(); dxgiDevice = nullptr; }
		}

		if (sd.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
		{
			IDXGISwapChain2* swapChain2 = nullptr;
			hr = GlobalSwapChain->QueryInterface(IID_PPV_ARGS(&swapChain2));
			if (swapChain2 != nullptr)
			{
				GlobalSwapChainWaitableObject = swapChain2->GetFrameLatencyWaitableObject();
				hr = swapChain2->SetMaximumFrameLatency(1);
				hr = swapChain2->Release();
			}
		}

		GlobalSwapChainCreationDesc = sd;
		CreateGlobalD3D11SwapchainRenderTarget();
		return true;
	}

	static void CleanupGlobalD3D11()
	{
		CleanupGlobalD3D11SwapchainRenderTarget();
		if (GlobalSwapChainWaitableObject) { ::CloseHandle(GlobalSwapChainWaitableObject); GlobalSwapChainWaitableObject = NULL; }
		if (GlobalSwapChain) { GlobalSwapChain->Release(); GlobalSwapChain = nullptr; }
		if (GlobalD3D11DeviceContext) { GlobalD3D11DeviceContext->Release(); GlobalD3D11DeviceContext = nullptr; }
		if (GlobalD3D11Device) { GlobalD3D11Device->Release(); GlobalD3D11Device = nullptr; }
	}

	static void CreateGlobalD3D11SwapchainRenderTarget()
	{
		ID3D11Texture2D* pBackBuffer;
		GlobalSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
		GlobalD3D11Device->CreateRenderTargetView(pBackBuffer, nullptr, &GlobalMainRenderTargetView);
		pBackBuffer->Release();
	}

	static void CleanupGlobalD3D11SwapchainRenderTarget()
	{
		if (GlobalMainRenderTargetView) { GlobalMainRenderTargetView->Release(); GlobalMainRenderTargetView = nullptr; }
	}

	// Win32 message handler
	// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
	// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
	// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
	// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
	static LRESULT WINAPI MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
			return true;

		switch (msg)
		{
		case WM_CREATE:
			// NOTE: "It's hardly a coincidence that the timer ID space is the same size as the address space" - Raymond Chen
			GlobalWindowRedrawTimerID = ::SetTimer(hwnd, reinterpret_cast<UINT_PTR>(&GlobalWindowRedrawTimerID), USER_TIMER_MINIMUM, nullptr);
			break;

		case WM_TIMER:
			if (GlobalIsWindowBeingDragged && wParam == GlobalWindowRedrawTimerID)
			{
				// HACK: Again not great but still better than completly freezing while dragging the window
				if (GlobalD3D11Device != nullptr && GlobalOnUserUpdate != nullptr)
					ImGuiAndUserUpdateThenRenderAndPresentFrame();
				return 0;
			}
			break;

		case WM_ENTERSIZEMOVE:
			GlobalIsWindowBeingDragged = true;
			break;

		case WM_EXITSIZEMOVE:
			GlobalIsWindowBeingDragged = false;
			break;

		case WM_MOVE:
			GlobalState.WindowPosition = ivec2(static_cast<i32>(LOWORD(lParam)), static_cast<i32>(HIWORD(lParam)));
			return 0;

		case WM_SIZE:
			GlobalIsWindowMinimized = (wParam == SIZE_MINIMIZED);
			if (GlobalD3D11Device != nullptr && wParam != SIZE_MINIMIZED)
			{
				GlobalState.WindowSize = ivec2(static_cast<i32>(LOWORD(lParam)), static_cast<i32>(HIWORD(lParam)));

				CleanupGlobalD3D11SwapchainRenderTarget();

				GlobalSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, GlobalSwapChainCreationDesc.Flags);
				CreateGlobalD3D11SwapchainRenderTarget();

				// HACK: Rendering during a resize is far from perfect but should still be much better than freezing completely
				if (GlobalOnUserUpdate != nullptr)
					ImGuiAndUserUpdateThenRenderAndPresentFrame();
			}
			return 0;

		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
				return 0;
			break;

		case WM_DESTROY:
			::PostQuitMessage(0);
			return 0;

		case WM_DPICHANGED:
			if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
			{
				// const int dpi = HIWORD(wParam);
				// printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
				const RECT* suggested_rect = (RECT*)lParam;
				::SetWindowPos(hwnd, NULL, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
			}
			break;

		case WM_DROPFILES:
			if (HDROP dropHandle = reinterpret_cast<HDROP>(wParam); dropHandle != NULL)
			{
				const UINT droppedFileCount = ::DragQueryFileW(dropHandle, 0xFFFFFFFF, nullptr, 0u);
				std::wstring utf16Buffer;

				GlobalState.FilePathsDroppedThisFrame.clear();
				GlobalState.FilePathsDroppedThisFrame.reserve(droppedFileCount);
				for (UINT i = 0; i < droppedFileCount; i++)
				{
					const UINT requiredBufferSize = ::DragQueryFileW(dropHandle, i, nullptr, 0u);
					if (requiredBufferSize > 0)
					{
						utf16Buffer.resize(requiredBufferSize + 1);
						if (const UINT success = ::DragQueryFileW(dropHandle, i, utf16Buffer.data(), static_cast<UINT>(utf16Buffer.size())); success != 0)
							GlobalState.FilePathsDroppedThisFrame.push_back(UTF8::Narrow(std::wstring_view(utf16Buffer.data(), requiredBufferSize)));
					}
				}

				::DragFinish(dropHandle);
			}
			break;

		case WM_GETMINMAXINFO:
			if (GlobalState.MinWindowSizeRestraints.has_value())
			{
				MINMAXINFO* outMinMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
				outMinMaxInfo->ptMinTrackSize.x = GlobalState.MinWindowSizeRestraints->x;
				outMinMaxInfo->ptMinTrackSize.y = GlobalState.MinWindowSizeRestraints->y;
			}
			break;

		case WM_CLOSE:
			if (GlobalOnUserWindowCloseRequest != nullptr)
			{
				const CloseResponse response = GlobalOnUserWindowCloseRequest();
				if (response == CloseResponse::SupressExit)
					return 0;
			}
			break;

		}
		return ::DefWindowProcW(hwnd, msg, wParam, lParam);
	}
}
