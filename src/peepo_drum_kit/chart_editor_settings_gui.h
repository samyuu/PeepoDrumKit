#pragma once
#include "core_types.h"
#include "core_string.h"
#include "chart_editor_context.h"
#include "imgui/imgui_include.h"
#include "main_settings.h"

namespace PeepoDrumKit
{
	struct ChartSettingsWindow
	{
		b8 DrawGui(ChartContext& context, UserSettingsData& settings);

	private:
		b8 DrawTabMain(ChartContext& context, UserSettingsData& settings);
		b8 DrawTabInput(ChartContext& context, UserSettingsData::InputData& settings);
		b8 DrawTabAudio(ChartContext& context, UserSettingsData::AudioData& settings);

		ImGuiTextFilter mainSettingsFilter = {};
		struct LastActiveGroup { void* ValuePtr; Rect GroupRect; } lastActiveGroup = {};

		ImGuiTextFilter inputSettingsFilter = {};
		WithDefault<MultiInputBinding>* selectedMultiBinding = nullptr;
		WithDefault<MultiInputBinding> selectedMultiBindingOnOpenCopy = {};
		f32 multiBindingPopupFadeCurrent = 0.0f;
		f32 multiBindingPopupFadeTarget = 0.0f;
		InputBinding* tempAssignedBinding {};
		CPUStopwatch assignedBindingStopwatch = {};
	};
}
