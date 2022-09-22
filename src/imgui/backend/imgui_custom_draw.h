#pragma once
#include "core_types.h"
#include "imgui/3rdparty/imgui.h"
#include "imgui/3rdparty/imgui_internal.h"

// NOTE: Special custom drawing related routines not usually possible via standard Dear ImGui
//		 but explictiply implemented for each render backend and made accessible here
namespace CustomDraw
{
	// TODO: Implement custom dynamic streaming texture API here too

	constexpr i32 WaveformPixelsPerChunk = 256;
	struct WaveformChunk { f32 PerPixelAmplitude[WaveformPixelsPerChunk]; };
	void DrawWaveformChunk(ImDrawList* drawList, Rect rect, u32 color, const WaveformChunk& chunk);
}
