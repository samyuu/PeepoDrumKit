#pragma once
#include "core_types.h"
#include "imgui/imgui_include.h"

namespace PeepoDrumKit
{
	inline f32 TimelineRowHeight = 32.0f; // 28.0f; // 26.0f;
	inline f32 TimelineRowHeightNotes = 48.0f; // 46.0f; // 38.0f; // 32.0f; // 26.0f;
	inline f32 TimelineAutoScrollLockContentWidthFactor = 0.35f;
	inline f32 TimelineCursorHeadHeight = 10.0f;
	inline f32 TimelineCursorHeadWidth = 11.0f;
	inline f32 TimelineBoxSelectionRadius = 9.0f;
	inline f32 TimelineBoxSelectionLineThickness = 2.0f;
	inline f32 TimelineBoxSelectionLinePadding = 4.0f;
	inline f32 TimelineSongDemoStartMarkerWidth = 9.0f;
	inline f32 TimelineSongDemoStartMarkerHeight = 9.0f;
	inline f32 TimelineSelectedNoteHitBoxSizeSmall = (16.0f * 2.0f) + 2.0f;
	inline f32 TimelineSelectedNoteHitBoxSizeBig = (22.0f * 2.0f) + 2.0f;

	struct NoteRadii { f32 BlackOuter, WhiteInner, ColorInner; };
	inline NoteRadii TimelineNoteRadiiSmall = { 16.0f, 14.0f, 10.0f };
	inline NoteRadii TimelineNoteRadiiBig = { 22.0f, 19.0f, 15.0f };
	inline NoteRadii GameRefNoteRadiiSmall = { 54.0f, 48.0f, 38.0f };
	inline NoteRadii GameRefNoteRadiiBig = { 81.0f, 75.0f, 60.0f }; // { 80.0f, 75.0f, 60.0f }; // { 81.0f, 76.0f, 61.0f }; // { 81.0f, 75.0f, 65.0f };

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

	// TODO: Define somewhese else... maybe even in "imgui_common.h" (?)
	inline u32 DragTextHoveredColor = 0xFFBCDDDB;
	inline u32 DragTextActiveColor = 0xFFC3F5F2;

	inline u32 TimelineBackgroundColor = 0xFF282828;
	inline u32 TimelineOutOfBoundsDimColor = 0x731F1F1F;
	inline u32 TimelineWaveformBaseColor = 0x807D7D7D;

	inline u32 TimelineCursorColor = 0xB375AD85; // 0xB37FDFAB; // 0xB375AD85;
	inline u32 TimelineItemTextColor = 0xFFF0F0F0;
	inline u32 TimelineItemTextColorShadow = 0xFF000000;
	inline u32 TimelineItemTextColorWarning = 0xFF4C4CFF;

	inline u32 TimelineGoGoBackgroundColorBorder = 0xFF1E1E1E;
	inline u32 TimelineGoGoBackgroundColorOuter = 0xFF46494D; // 0xFF46494D;
	inline u32 TimelineGoGoBackgroundColorInner = 0xFF2D2D33; // 0xFF2A2B2D; // 0xFF8C8C99; // 0x2C8C8C99; // 0x448787FF;

	inline u32 TimelineLyricsTextColor = 0xFFDEE5F0; // 0xFFC2D3EB; // 0xFFC9DBE6; // 0xFFF0F0F0;
	inline u32 TimelineLyricsTextColorShadow = 0xFF131415; // 0xFF222527; // 0xFF000000;
	inline u32 TimelineLyricsBackgroundColorBorder = 0xFF1E1E1E;
	inline u32 TimelineLyricsBackgroundColorOuter = 0xFF46494D; // 0xFF46494D;
	inline u32 TimelineLyricsBackgroundColorInner = 0xFF323438; // 0xFF2A2B2D;

	inline u32 TimelineHorizontalRowLineColor = 0x2D7D7D7D;
	inline u32 TimelineGridBarLineColor = 0x807D7D7D;
	inline u32 TimelineGridBeatLineColor = 0x2D7D7D7D;
	inline u32 TimelineGridSnapLineColor = 0x1A7D7D7D;
	inline u32 TimelineGridSnapTupletLineColor = 0x1A22BBBC; // 0x1A7D7D7D;

	inline u32 TimelineBoxSelectionBackgroundColor = 0x262E8F5E;
	inline u32 TimelineBoxSelectionBorderColor = 0xFF2E8F5E;
	inline u32 TimelineBoxSelectionFillColor = 0xFF293730;
	inline u32 TimelineBoxSelectionInnerColor = 0xFFF0F0F0;
	inline u32 TimelineRangeSelectionBackgroundColor = 0x0AD0D0D0;
	inline u32 TimelineRangeSelectionBorderColor = 0x60E0E0E0;
	inline u32 TimelineRangeSelectionHeaderHighlightColor = 0x6075AD85;
	inline u32 TimelineSelectedNoteBoxBackgroundColor = 0x0AD0D0D0;
	inline u32 TimelineSelectedNoteBoxBorderColor = 0x60E0E0E0;

	inline u32 TimelineTempoChangeLineColor = 0xDC314CD8; // 0xAA2A7DFF; // 0xFF0000FF;
	inline u32 TimelineSignatureChangeLineColor = 0xDC22BEE2; // 0xAA2AFFB4; // 0xFF00FF00;
	inline u32 TimelineScrollChangeLineColor = 0xDC6CA71E; // 0xDC548119;
	inline u32 TimelineBarLineChangeLineColor = 0xDCBE9E2C;

	inline u32 TimelineSongDemoStartMarkerColorFill = 0x3B75AD85; // 0x3A63D5AE;
	inline u32 TimelineSongDemoStartMarkerColorBorder = 0xB375AD85; // 0xC863D5AE;

	inline u32 GameLaneBorderColor = 0xFF000000;
	inline u32 GameLaneBarLineColor = 0xBFDADADA; // 0xFFDADADA;
	inline u32 GameLaneContentBackgroundColor = 0xFF282828;
	inline u32 GameLaneFooterBackgroundColor = 0xFF848484;
	inline u32 GameLaneHitCircleInnerFillColor = 0xFF525252;
	inline u32 GameLaneHitCircleInnerOutlineColor = 0xFF888888;
	inline u32 GameLaneHitCircleOuterOutlineColor = 0xFF646464;

	inline u32 NoteColorRed = 0xFF2746F5;
	inline u32 NoteColorBlue = 0xFFC0C044;
	inline u32 NoteColorOrange = 0xFF0078FD;
	inline u32 NoteColorYellow = 0xFF00B7F6;
	inline u32 NoteColorWhite = 0xFFDEEDF5;
	inline u32 NoteColorBlack = 0xFF1E1E1E; // Alternatively to make it a bit darker: 0xFF161616;
	inline u32 NoteBalloonTextColor = 0xFF000000;
	inline u32 NoteBalloonTextColorShadow = 0xFFFFFFFF;
	inline u32* NoteTypeToColorMap[EnumCount<NoteType>] =
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

	struct GameCamera
	{
		Rect ScreenSpaceContentRect {};
		Rect ScreenSpaceLaneRect {};
		f32 RefToScreenScaleFactor = 1.0f;

		inline f32 RefToScreenScale(f32 refScale) const { return refScale * RefToScreenScaleFactor; }
		inline vec2 RefToScreenScale(vec2 refSpace) const { return refSpace * RefToScreenScaleFactor; }
		inline vec2 RefToScreenSpace(vec2 refSpace) const { return ScreenSpaceLaneRect.TL + (refSpace * RefToScreenScaleFactor); }
		inline bool IsPointVisibleOnLane(f32 refX, f32 refThreshold = 280.0f) const { return (refX >= -refThreshold) && (refX <= (GameRefLaneSize.x + refThreshold)); }
		inline bool IsRangeVisibleOnLane(f32 refHeadX, f32 refTailX, f32 refThreshold = 280.0f) const { return (refTailX >= -refThreshold) && (refHeadX <= (GameRefLaneSize.x + refThreshold)); }
	};

	constexpr f32 TimeToNoteLaneRefSpaceX(Time cursorTime, Time noteTime, Tempo tempo, f32 scrollSpeed)
	{
		return ((tempo.BPM * scrollSpeed) / 60.0f) * static_cast<f32>((noteTime - cursorTime).TotalSeconds()) * GameRefLaneDistancePerBeat;
	}

	inline NoteRadii GuiScaleNoteRadii(NoteRadii radii) { return NoteRadii { GuiScale(radii.BlackOuter), GuiScale(radii.WhiteInner), GuiScale(radii.ColorInner) }; }

	inline void DrawTimelineNote(ImDrawList* drawList, vec2 center, f32 scale, NoteType noteType, f32 alpha = 1.0f)
	{
		const auto radii = GuiScaleNoteRadii(IsBigNote(noteType) ? TimelineNoteRadiiBig : TimelineNoteRadiiSmall);
		drawList->AddCircleFilled(center, scale * radii.BlackOuter, Gui::ColorU32WithAlpha(NoteColorBlack, alpha));
		drawList->AddCircleFilled(center, scale * radii.WhiteInner, Gui::ColorU32WithAlpha(NoteColorWhite, alpha));
		drawList->AddCircleFilled(center, scale * radii.ColorInner, Gui::ColorU32WithAlpha(*NoteTypeToColorMap[EnumToIndex(noteType)], alpha));
	}

	// TODO: Maybe animate hitting during playback and transition color from yellow -> orange -> red (?)
	inline void DrawTimelineNoteDuration(ImDrawList* drawList, vec2 centerHead, vec2 centerTail, NoteType noteType, f32 alpha = 1.0f)
	{
		// BUG: Ugly full-circle clipping when zoomed out and not drawing a regular head note to cover it up
		const auto radii = GuiScaleNoteRadii(IsBigNote(noteType) ? TimelineNoteRadiiBig : TimelineNoteRadiiSmall);
		DrawTimelineNote(drawList, centerHead, 1.0f, noteType, alpha);
		DrawTimelineNote(drawList, centerTail, 1.0f, noteType, alpha);
		drawList->AddRectFilled(centerHead - vec2(0.0f, radii.BlackOuter), centerTail + vec2(0.0f, radii.BlackOuter), Gui::ColorU32WithAlpha(NoteColorBlack, alpha));
		drawList->AddRectFilled(centerHead - vec2(0.0f, radii.WhiteInner), centerTail + vec2(0.0f, radii.WhiteInner), Gui::ColorU32WithAlpha(NoteColorWhite, alpha));
		drawList->AddRectFilled(centerHead - vec2(0.0f, radii.ColorInner), centerTail + vec2(0.0f, radii.ColorInner), Gui::ColorU32WithAlpha(*NoteTypeToColorMap[EnumToIndex(noteType)], alpha));
	}

	inline void DrawTimelineNoteBalloonPopCount(ImDrawList* drawList, vec2 center, f32 scale, i32 popCount)
	{
		char buffer[32]; const auto text = std::string_view(buffer, sprintf_s(buffer, "%d", popCount));
		const ImFont* font = FontLarge_EN;
		const f32 fontSize = (font->FontSize * scale);
		const vec2 textSize = font->CalcTextSizeA(fontSize, F32Max, -1.0f, Gui::StringViewStart(text), Gui::StringViewEnd(text));
		const vec2 textPosition = (center - (textSize * 0.5f)) - vec2(0.0f, 1.0f);

		Gui::DisableFontPixelSnap(true);
		Gui::AddTextWithDropShadow(drawList, font, fontSize, textPosition, NoteBalloonTextColor, text, 0.0f, nullptr, NoteBalloonTextColorShadow);
		Gui::DisableFontPixelSnap(false);
	}

	struct DrawTimelineRectBaseParam { vec2 TL, BR; f32 TriScaleL, TriScaleR; u32 ColorBorder, ColorOuter, ColorInner; };
	inline void DrawTimelineRectBaseWithStartEndTriangles(ImDrawList* drawList, DrawTimelineRectBaseParam param)
	{
		const f32 outerOffset = ClampBot(GuiScale(2.0f), 1.0f);
		const f32 innerOffset = ClampBot(GuiScale(5.0f), 1.0f);
		const Rect borderRect = Rect(param.TL, param.BR);
		Rect outerRect = Rect(param.TL + vec2(outerOffset), param.BR - vec2(outerOffset)); if (outerRect.GetWidth() < outerOffset) outerRect.BR.x = outerRect.TL.x + outerOffset;
		Rect innerRect = Rect(param.TL + vec2(innerOffset), param.BR - vec2(innerOffset)); if (innerRect.GetWidth() < outerOffset) innerRect.BR.x = innerRect.TL.x + outerOffset;

		drawList->AddRectFilled(borderRect.TL, borderRect.BR, param.ColorBorder);
		drawList->AddRectFilled(outerRect.TL, outerRect.BR, param.ColorOuter);
		drawList->AddRectFilled(innerRect.TL, innerRect.BR, param.ColorInner);

		if (outerRect.GetWidth() > outerOffset)
		{
			const f32 triH = (param.BR.y - param.TL.y) - innerOffset;
			const f32 triW = ClampTop(triH, (param.BR.x - param.TL.x) - innerOffset);
			if (param.TriScaleL > 0.0f)
				drawList->AddTriangleFilled(outerRect.TL, outerRect.TL + vec2(triW, triH), outerRect.TL + vec2(0.0f, triH), param.ColorOuter);
			else if (param.TriScaleL < 0.0f)
				drawList->AddTriangleFilled(outerRect.TL, outerRect.TL + vec2(triW, 0.0f), outerRect.TL + vec2(0.0f, triH), param.ColorOuter);

			if (param.TriScaleR > 0.0f)
				drawList->AddTriangleFilled(outerRect.BR, outerRect.BR - vec2(triW, 0.0f), outerRect.BR - vec2(0.0f, triH), param.ColorOuter);
			else if (param.TriScaleR < 0.0f)
				drawList->AddTriangleFilled(outerRect.BR, outerRect.BR - vec2(triW, triH), outerRect.BR - vec2(0.0f, triH), param.ColorOuter);
		}
	}

	inline void DrawTimelineGoGoTimeBackground(ImDrawList* drawList, vec2 tl, vec2 br, f32 animationScale)
	{
		const f32 centerX = (br.x + tl.x) * 0.5f;
		tl.x = Lerp(centerX, tl.x, animationScale);
		br.x = Lerp(centerX, br.x, animationScale);
		DrawTimelineRectBaseWithStartEndTriangles(drawList, DrawTimelineRectBaseParam { tl, br, 1.0f, 1.0f, TimelineGoGoBackgroundColorBorder, TimelineGoGoBackgroundColorOuter, TimelineGoGoBackgroundColorInner });
	}

	inline void DrawTimelineLyricsBackground(ImDrawList* drawList, vec2 tl, vec2 br)
	{
		DrawTimelineRectBaseWithStartEndTriangles(drawList, DrawTimelineRectBaseParam { tl, br, 1.0f, 0.0f, TimelineLyricsBackgroundColorBorder, TimelineLyricsBackgroundColorOuter, TimelineLyricsBackgroundColorInner });
	}

	inline void DrawGamePreviewNote(const GameCamera& camera, ImDrawList* drawList, vec2 refSpaceCenter, NoteType noteType)
	{
		const auto radii = IsBigNote(noteType) ? GameRefNoteRadiiBig : GameRefNoteRadiiSmall;
		drawList->AddCircleFilled(camera.RefToScreenSpace(refSpaceCenter), camera.RefToScreenScale(radii.BlackOuter), NoteColorBlack);
		drawList->AddCircleFilled(camera.RefToScreenSpace(refSpaceCenter), camera.RefToScreenScale(radii.WhiteInner), NoteColorWhite);
		drawList->AddCircleFilled(camera.RefToScreenSpace(refSpaceCenter), camera.RefToScreenScale(radii.ColorInner), *NoteTypeToColorMap[EnumToIndex(noteType)]);
	}

	inline void DrawGamePreviewNoteDuration(const GameCamera& camera, ImDrawList* drawList, vec2 refSpaceCenterHead, vec2 refSpaceCenterTail, NoteType noteType)
	{
		const auto radii = IsBigNote(noteType) ? GameRefNoteRadiiBig : GameRefNoteRadiiSmall;
		DrawGamePreviewNote(camera, drawList, refSpaceCenterHead, noteType);
		DrawGamePreviewNote(camera, drawList, refSpaceCenterTail, noteType);
		drawList->AddRectFilled(camera.RefToScreenSpace(refSpaceCenterHead - vec2(0.0f, radii.BlackOuter)), camera.RefToScreenSpace(refSpaceCenterTail + vec2(0.0f, radii.BlackOuter)), NoteColorBlack);
		drawList->AddRectFilled(camera.RefToScreenSpace(refSpaceCenterHead - vec2(0.0f, radii.WhiteInner)), camera.RefToScreenSpace(refSpaceCenterTail + vec2(0.0f, radii.WhiteInner)), NoteColorWhite);
		drawList->AddRectFilled(camera.RefToScreenSpace(refSpaceCenterHead - vec2(0.0f, radii.ColorInner)), camera.RefToScreenSpace(refSpaceCenterTail + vec2(0.0f, radii.ColorInner)), *NoteTypeToColorMap[EnumToIndex(noteType)]);
	}
}
