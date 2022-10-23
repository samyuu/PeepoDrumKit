#pragma once
#include "core_types.h"
#include "chart.h"
#include "imgui/imgui_include.h"

namespace PeepoDrumKit
{
	inline f32 TimelineRowHeight = 32.0f;
	inline f32 TimelineRowHeightNotes = 48.0f;
	inline f32 TimelineAutoScrollLockContentWidthFactor = 0.35f;
	inline f32 TimelineCursorHeadHeight = 10.0f;
	inline f32 TimelineCursorHeadWidth = 11.0f;
	inline f32 TimelineBoxSelectionRadius = 9.0f;
	inline f32 TimelineBoxSelectionLineThickness = 2.0f;
	inline f32 TimelineBoxSelectionLinePadding = 4.0f;
	inline f32 TimelineBoxSelectionXorDotRadius = 3.0f;
	inline f32 TimelineSongDemoStartMarkerWidth = 9.0f;
	inline f32 TimelineSongDemoStartMarkerHeight = 9.0f;
	inline f32 TimelineSelectedNoteHitBoxSizeSmall = (16.0f * 2.0f) + 2.0f;
	inline f32 TimelineSelectedNoteHitBoxSizeBig = (22.0f * 2.0f) + 2.0f;

	struct NoteRadii { f32 BlackOuter, WhiteInner, ColorInner; };
	inline NoteRadii GameRefNoteRadiiSmall = { 54.0f, 48.0f, 38.0f };
	inline NoteRadii GameRefNoteRadiiBig = { 81.0f, 75.0f, 60.0f };
	inline NoteRadii GuiScaleNoteRadii(NoteRadii radii) { return NoteRadii { GuiScale(radii.BlackOuter), GuiScale(radii.WhiteInner), GuiScale(radii.ColorInner) }; }

	// NOTE: All "Game Preview" coordinates are defined in this "Ref-Space" and then scaled to Screen-Space before rendering
	constexpr vec2 GameRefScale = vec2(1920.0f, 1080.0f);
	constexpr vec2 GameRefLanePerSideBorderSize = vec2(6.0f, 12.0f);
	constexpr vec2 GameRefLaneSize = vec2(1428.0f - GameRefLanePerSideBorderSize.x, 264.0f - (GameRefLanePerSideBorderSize.y * 2.0f));
	constexpr f32 GameRefLaneContentHeight = 195.0f;
	constexpr f32 GameRefLaneFooterHeight = 39.0f;
	constexpr f32 GameRefLaneDistancePerBeat = 356.0f;
	constexpr f32 GameRefBarLineThickness = 3.0f;
	constexpr vec2 GameRefHitCircleCenter = vec2(120.0f, 98.0f);
	constexpr f32 GameRefHitCircleInnerFillRadius = (78.0f / 2.0f);
	constexpr f32 GameRefHitCircleInnerOutlineRadius = (100.0f / 2.0f);
	constexpr f32 GameRefHitCircleInnerOutlineThickness = 5.0f;
	constexpr f32 GameRefHitCircleOuterOutlineRadius = (154.0f / 2.0f);
	constexpr f32 GameRefHitCircleOuterOutlineThickness = 5.0f;

	inline u32 DragTextHoveredColor = 0xFFBCDDDB;
	inline u32 DragTextActiveColor = 0xFFC3F5F2;
	inline u32 InputTextWarningTextColor = 0xFF4C4CFF;

	inline u32 TimelineBackgroundColor = 0xFF282828;
	inline u32 TimelineOutOfBoundsDimColor = 0x731F1F1F;
	inline u32 TimelineWaveformBaseColor = 0x807D7D7D;

	inline u32 TimelineCursorColor = 0xB375AD85;
	inline u32 TimelineItemTextColor = 0xFFF0F0F0;
	inline u32 TimelineItemTextColorShadow = 0xFF000000;
	inline u32 TimelineItemTextColorWarning = 0xFF4C4CFF;
	inline u32 TimelineItemTextUnderlineColor = 0xFFF0F0F0;
	inline u32 TimelineItemTextUnderlineColorShadow = 0xFF000000;

	inline u32 TimelineGoGoBackgroundColorBorder = 0xFF1E1E1E;
	inline u32 TimelineGoGoBackgroundColorBorderSelected = 0xFF979797;
	inline u32 TimelineGoGoBackgroundColorOuter = 0xFF46494D;
	inline u32 TimelineGoGoBackgroundColorInner = 0xFF2D2D33;

	inline u32 TimelineLyricsTextColor = 0xFFDEE5F0;
	inline u32 TimelineLyricsTextColorShadow = 0xFF131415;
	inline u32 TimelineLyricsBackgroundColorBorder = 0xFF1E1E1E;
	inline u32 TimelineLyricsBackgroundColorBorderSelected = 0xFF979797;
	inline u32 TimelineLyricsBackgroundColorOuter = 0xFF46494D;
	inline u32 TimelineLyricsBackgroundColorInner = 0xFF323438;

	inline u32 TimelineHorizontalRowLineColor = 0x2D7D7D7D;
	inline u32 TimelineGridBarLineColor = 0x807D7D7D;
	inline u32 TimelineGridBeatLineColor = 0x2D7D7D7D;
	inline u32 TimelineGridSnapLineColor = 0x1A7D7D7D;
	inline u32 TimelineGridSnapTupletLineColor = 0x1A22BBBC;

	inline u32 TimelineBoxSelectionBackgroundColor = 0x262E8F5E;
	inline u32 TimelineBoxSelectionBorderColor = 0xFF2E8F5E;
	inline u32 TimelineBoxSelectionFillColor = 0xFF293730;
	inline u32 TimelineBoxSelectionInnerColor = 0xFFF0F0F0;
	inline u32 TimelineRangeSelectionBackgroundColor = 0x0AD0D0D0;
	inline u32 TimelineRangeSelectionBorderColor = 0x60E0E0E0;
	inline u32 TimelineRangeSelectionHeaderHighlightColor = 0x6075AD85;
	inline u32 TimelineSelectedNoteBoxBackgroundColor = 0x0AD0D0D0;
	inline u32 TimelineSelectedNoteBoxBorderColor = 0x60E0E0E0;

	inline u32 TimelineTempoChangeLineColor = 0xDC314CD8;
	inline u32 TimelineSignatureChangeLineColor = 0xDC22BEE2;
	inline u32 TimelineScrollChangeLineColor = 0xDC6CA71E;
	inline u32 TimelineBarLineChangeLineColor = 0xDCBE9E2C;
	inline u32 TimelineSelectedItemLineColor = 0xDCFFFFFF;

	inline u32 TimelineSongDemoStartMarkerColorFill = 0x3B75AD85;
	inline u32 TimelineSongDemoStartMarkerColorBorder = 0xB375AD85;

	inline u32 GameLaneBorderColor = 0xFF000000;
	inline u32 GameLaneBarLineColor = 0xBFDADADA;
	inline u32 GameLaneContentBackgroundColor = 0xFF282828;
	inline u32 GameLaneFooterBackgroundColor = 0xFF848484;
	inline u32 GameLaneHitCircleInnerFillColor = 0xFF525252;
	inline u32 GameLaneHitCircleInnerOutlineColor = 0xFF888888;
	inline u32 GameLaneHitCircleOuterOutlineColor = 0xFF646464;

	inline u32 NoteColorRed = 0xFF2B41F3;
	inline u32 NoteColorBlue = 0xFFC2C351;
	inline u32 NoteColorOrange = 0xFF0078FD;
	inline u32 NoteColorYellow = 0xFF13B7F4;
	inline u32 NoteColorWhite = 0xFFDEEDF5;
	inline u32 NoteColorBlack = 0xFF1E1E1E;
	inline u32 NoteColorDrumrollHit = 0xFF0000F6;
	inline u32 NoteBalloonTextColor = 0xFF000000;
	inline u32 NoteBalloonTextColorShadow = 0xFFFFFFFF;
	inline u32* NoteTypeToColorMap[] =
	{
		&NoteColorRed,		// Don
		&NoteColorRed,		// DonBig
		&NoteColorBlue,		// Ka
		&NoteColorBlue,		// KaBig
		&NoteColorYellow,	// Start_Drumroll
		&NoteColorYellow,	// Start_DrumrollBig
		&NoteColorOrange,	// Start_Balloon
		&NoteColorOrange,	// Start_SpecialBaloon
	};
	static_assert(ArrayCount(NoteTypeToColorMap) == EnumCount<NoteType>);

	constexpr void GuiStyleColorPeepoDrumKit(ImGuiStyle* destination = &Gui::GetStyle())
	{
		ImVec4* colors = destination->Colors;
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
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.31f, 0.78f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.23f, 0.78f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.76f, 0.76f, 0.76f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(0.50f, 0.75f, 0.21f, 1.00f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.50f, 0.75f, 0.21f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.50f);
	}
}
