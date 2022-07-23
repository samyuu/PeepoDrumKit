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
		void DrawGui(ChartContext& context, UserSettingsData& settings);
	};
}
