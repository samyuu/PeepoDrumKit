#pragma once
#include "core_types.h"
#include "core_string.h"
#include "chart_editor_context.h"
#include "imgui/imgui_include.h"
#include "main_settings.h"

namespace PeepoDrumKit
{
	struct ChartSettingsWindowTempActiveWidgetGroup
	{
		void* ValuePtr;
		Rect GroupRect;
	};

	struct ChartSettingsWindowTempInputState
	{
		WithDefault<MultiInputBinding>* SelectedMultiBinding = nullptr;
		WithDefault<MultiInputBinding> SelectedMultiBindingOnOpenCopy = {};
		f32 MultiBindingPopupFadeCurrent = 0.0f;
		f32 MultiBindingPopupFadeTarget = 0.0f;
		InputBinding* TempAssignedBinding {};
		CPUStopwatch AssignedBindingStopwatch = {};
	};

	struct ChartSettingsWindow
	{
		b8 DrawGui(ChartContext& context, UserSettingsData& settings);

	private:
		ImGuiTextFilter settingsFilterMain = {};
		ImGuiTextFilter settingsFilterInput = {};
		ChartSettingsWindowTempActiveWidgetGroup lastActiveGroup = {};
		ChartSettingsWindowTempInputState inputState = {};
	};
}
