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
		ImGuiTextFilter BindingsFilter = {};
		WithDefault<MultiInputBinding>* SelectedMultiBinding = nullptr;
		WithDefault<MultiInputBinding> SelectedMultiBindingOnOpenCopy = {};
		f32 MultiBindingPopupFadeCurrent = 0.0f;
		f32 MultiBindingPopupFadeTarget = 0.0f;
		InputBinding* TempAssignedBinding {};
		CPUStopwatch AssignedBindingStopwatch = {};

		void DrawGui(ChartContext& context, UserSettingsData& settings);

		void DrawTabMain(ChartContext& context, UserSettingsData& settings);
		void DrawTabInput(ChartContext& context, UserSettingsData::InputData& settings);
		void DrawTabAudio(ChartContext& context, UserSettingsData::AudioData& settings);
	};
}
