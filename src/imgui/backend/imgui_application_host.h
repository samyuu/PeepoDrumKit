#pragma once
#include "core_types.h"
#include <string_view>
#include <vector>
#include <optional>

// NOTE: Exposed here specifically to be used with PushFont() / PopFont()
struct ImFont;
inline ImFont* FontMain_JP = nullptr;
inline ImFont* FontMedium_EN = nullptr;
inline ImFont* FontLarge_EN = nullptr;

inline f32 GuiScaleFactor = 1.0f;
inline f32 GuiScaleFactorToSetNextFrame = GuiScaleFactor;
inline f32 GuiScale(f32 value) { return Floor(value * GuiScaleFactor); }
inline vec2 GuiScale(vec2 value) { return Floor(value * GuiScaleFactor); }
inline i32 GuiScaleI32(i32 value) { return static_cast<i32>(Floor(static_cast<f32>(value) * GuiScaleFactor)); }

constexpr f32 GuiScaleFactorMin = FromPercent(50.0f);
constexpr f32 GuiScaleFactorMax = FromPercent(400.0f);
constexpr f32 RoundAndClampGuiScaleFactor(f32 scaleFactor) { return Clamp(FromPercent(Round(ToPercent(scaleFactor))), GuiScaleFactorMin, GuiScaleFactorMax); }

namespace ApplicationHost
{
	struct StartupParam
	{
		std::string_view WindowTitle = {};
		std::optional<ivec2> WindowPosition = {};
		std::optional<ivec2> WindowSize = {};
		bool WaitableSwapChain = true;
		bool AllowSwapChainTearing = false;
	};

	struct State
	{
		// NOTE: READ ONLY
		// --------------------------------
		void* NativeWindowHandle;
		ivec2 WindowPosition;
		ivec2 WindowSize;
		bool IsBorderlessFullscreen;
		// TODO: Implement the notion of "handling" a dropped file event
		std::vector<std::string> FilePathsDroppedThisFrame;
		std::string WindowTitle;
		void* FontFileContent;
		size_t FontFileContentSize;
		// --------------------------------

		// NOTE: READ + WRITE
		// --------------------------------
		i32 SwapInterval = 1;
		std::string SetWindowTitleNextFrame;
		std::optional<ivec2> SetWindowPositionNextFrame;
		std::optional<ivec2> SetWindowSizeNextFrame;
		std::optional<ivec2> MinWindowSizeRestraints = ivec2(640, 360);
		std::optional<bool> SetBorderlessFullscreenNextFrame;
		std::optional<i32> RequestExitNextFrame;
		// --------------------------------
	};

	inline State GlobalState = {};

	// NOTE: Specifically to handle the case of unsaved user data
	enum class CloseResponse : u8 { Exit, SupressExit };

	using StartupFunc = void(*)();
	using UpdateFunc = void(*)();
	using ShutdownFunc = void(*)();
	using WindowCloseRequestFunc = CloseResponse(*)();

	struct UserCallbacks
	{
		StartupFunc OnStartup;
		UpdateFunc OnUpdate;
		ShutdownFunc OnShutdown;
		WindowCloseRequestFunc OnWindowCloseRequest;
	};

	i32 EnterProgramLoop(const StartupParam& startupParam, UserCallbacks userCallbacks);
}
