#pragma once
#include "core_types.h"
#include "imgui/imgui_include.h"

namespace PeepoDrumKit
{
	constexpr std::string_view PeepoDrumKitApplicationTitle = PEEPO_DEBUG ? "Peepo Drum Kit (Debug)" : PEEPO_RELEASE ? "Peepo Drum Kit (Release)" : "Peepo Drum Kit (Unknown)";

	// TODO: PeepoDrumKit settings.ini implement explicit default fields (and in settings window) by commenting out lines (?)

	// TODO:
	struct ApplicationSettings
	{
		// ...
	};

	struct UserSettings
	{
		Beat DrumrollHitBeatInterval = Beat::FromBars(1) / 16;

		// TODO: Adjust default values (?)
		f32 TimelineScrubAutoScrollPixelThreshold = 160.0f;
		f32 TimelineScrubAutoScrollSpeedMin = 150.0f;
		f32 TimelineScrubAutoScrollSpeedMax = 3500.0f;

		f32 TimelineSmoothScrollAnimationSpeed = 20.0f;
		f32 TimelineWaveformFadeAnimationSpeed = 20.0f;
		f32 TimelineRangeSelectionExpansionAnimationSpeed = 20.0f;
		f32 TimelineWorldSpaceCursorXAnimationSpeed = 35.0f;
		f32 TimelineGridSnapLineAnimationSpeed = 10.0f;
		f32 TimelineGoGoRangeExpansionAnimationSpeed = 20.0f;
		// TODO: Should probably also store scroll / zoom speed here

		struct InputData
		{
			MultiInputBinding Dialog_YesOrOk = { InputBinding(ImGuiKey_Enter, ImGuiModFlags_None) };
			MultiInputBinding Dialog_No = { InputBinding(ImGuiKey_Backspace, ImGuiModFlags_None) };
			MultiInputBinding Dialog_Cancel = { InputBinding(ImGuiKey_Escape, ImGuiModFlags_None) };
			MultiInputBinding Dialog_SelectNextTab = { InputBinding(ImGuiKey_Tab, ImGuiModFlags_Ctrl) };
			MultiInputBinding Dialog_SelectPreviousTab = { InputBinding(ImGuiKey_Tab, ImGuiModFlags_Ctrl | ImGuiModFlags_Shift) };

			MultiInputBinding Editor_ToggleFullscreen = { InputBinding(ImGuiKey_F11, ImGuiModFlags_None), InputBinding(ImGuiKey_Enter, ImGuiModFlags_Alt) };
			MultiInputBinding Editor_Undo = { InputBinding(ImGuiKey_Z, ImGuiModFlags_Ctrl) };
			MultiInputBinding Editor_Redo = { InputBinding(ImGuiKey_Y, ImGuiModFlags_Ctrl) };
			MultiInputBinding Editor_ChartNew = { InputBinding(ImGuiKey_N, ImGuiModFlags_Ctrl) };
			MultiInputBinding Editor_ChartOpen = { InputBinding(ImGuiKey_O, ImGuiModFlags_Ctrl) };
			MultiInputBinding Editor_ChartOpenDirectory = {};
			MultiInputBinding Editor_ChartSave = { InputBinding(ImGuiKey_S, ImGuiModFlags_Ctrl) };
			MultiInputBinding Editor_ChartSaveAs = { InputBinding(ImGuiKey_S, ImGuiModFlags_Ctrl | ImGuiModFlags_Shift) };

			MultiInputBinding Timeline_PlaceNoteDon = { InputBinding(ImGuiKey_F, ImGuiModFlags_None), InputBinding(ImGuiKey_J, ImGuiModFlags_None) };
			MultiInputBinding Timeline_PlaceNoteKa = { InputBinding(ImGuiKey_D, ImGuiModFlags_None), InputBinding(ImGuiKey_K, ImGuiModFlags_None) };
			MultiInputBinding Timeline_PlaceNoteBalloon = { InputBinding(ImGuiKey_E, ImGuiModFlags_None), InputBinding(ImGuiKey_I, ImGuiModFlags_None) };
			MultiInputBinding Timeline_PlaceNoteDrumroll = { InputBinding(ImGuiKey_R, ImGuiModFlags_None), InputBinding(ImGuiKey_U, ImGuiModFlags_None) };
			MultiInputBinding Timeline_Cut = { InputBinding(ImGuiKey_X, ImGuiModFlags_Ctrl) };
			MultiInputBinding Timeline_Copy = { InputBinding(ImGuiKey_C, ImGuiModFlags_Ctrl) };
			MultiInputBinding Timeline_Paste = { InputBinding(ImGuiKey_V, ImGuiModFlags_Ctrl) };
			MultiInputBinding Timeline_DeleteSelection = { InputBinding(ImGuiKey_Delete, ImGuiModFlags_None) };
			MultiInputBinding Timeline_StartEndRangeSelection = { InputBinding(ImGuiKey_Tab, ImGuiModFlags_None) };
			MultiInputBinding Timeline_StepCursorLeft = { InputBinding(ImGuiKey_LeftArrow, ImGuiModFlags_None) };
			MultiInputBinding Timeline_StepCursorRight = { InputBinding(ImGuiKey_RightArrow, ImGuiModFlags_None) };
			MultiInputBinding Timeline_IncreaseGridDivision = { InputBinding(ImGuiKey_UpArrow, ImGuiModFlags_None) };
			MultiInputBinding Timeline_DecreaseGridDivision = { InputBinding(ImGuiKey_DownArrow, ImGuiModFlags_None) };
			MultiInputBinding Timeline_IncreasePlaybackSpeed = { InputBinding(ImGuiKey_C, ImGuiModFlags_None) };
			MultiInputBinding Timeline_DecreasePlaybackSpeed = { InputBinding(ImGuiKey_Z, ImGuiModFlags_None) };
			MultiInputBinding Timeline_TogglePlayback = { InputBinding(ImGuiKey_Space, ImGuiModFlags_None) };
			MultiInputBinding Timeline_ToggleMetronome = { InputBinding(ImGuiKey_F1, ImGuiModFlags_None) };
		} Input;

#if 0  // TODO: (Should probably partially be inside TJA namespace)
		struct TJAWriteData
		{
			enum class TJATextEncoding { UTF8WithBom, ShiftJIS };
			enum class StringOrInt { String, Int };
			TJATextEncoding Encoding;
			StringOrInt DifficultyTypeFormat;
		} TJAWrite;
#endif
	};

	inline UserSettings GlobalSettings = {};

	inline void GuiStyleColorNiceLimeGreen(ImGuiStyle* destination = nullptr)
	{
		ImVec4* colors = (destination != nullptr) ? destination->Colors : Gui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		colors[ImGuiCol_Border] = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.21f, 0.21f, 0.21f, 0.54f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.27f, 0.27f, 0.27f, 0.64f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 0.54f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.12f, 0.12f, 0.12f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.37f, 0.56f, 0.18f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.52f, 0.18f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.33f, 0.49f, 0.17f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.45f, 0.45f, 0.45f, 0.40f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.59f, 0.59f, 0.59f, 0.40f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.36f, 0.36f, 0.36f, 0.40f);
		colors[ImGuiCol_Header] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.23f, 0.23f, 0.23f, 1.00f);
		colors[ImGuiCol_Separator] = ImVec4(0.49f, 0.49f, 0.49f, 0.50f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.32f, 0.49f, 0.21f, 0.78f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.39f, 0.67f, 0.18f, 0.78f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.44f, 0.44f, 0.44f, 0.20f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.39f, 0.60f, 0.24f, 0.20f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.43f, 0.65f, 0.27f, 0.20f);
		colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.22f, 0.25f, 0.18f, 1.00f);
		colors[ImGuiCol_TabActive] = ImVec4(0.30f, 0.41f, 0.16f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
		colors[ImGuiCol_DockingPreview] = ImVec4(0.52f, 0.68f, 0.46f, 0.70f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.34f, 0.45f, 0.20f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.35f, 0.49f, 0.18f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.34f, 0.45f, 0.20f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.23f, 1.00f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0.41f, 0.41f, 0.41f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.05f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.76f, 0.76f, 0.76f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(0.50f, 0.75f, 0.21f, 1.00f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.50f, 0.75f, 0.21f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.50f);
	}
}
