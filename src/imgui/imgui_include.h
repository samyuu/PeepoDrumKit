#pragma once

#include "3rdparty/imgui.h"
#include "3rdparty/imgui_internal.h"
#include "backend/imgui_application_host.h"
#include "backend/imgui_custom_draw.h"
#include "extension/imgui_common.h"
#include "extension/imgui_input_binding.h"

// NOTE: This should be the only public Dear Imgui header included by all other src files (except for 3rdparty_extension to prevent include polution)
//		 All external code should only reference the ImGui namespace using this alias
namespace Gui = ImGui;
