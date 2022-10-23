#pragma once
#include "core_types.h"
#include "core_string.h"
#include "chart.h"
#include "chart_editor_timeline.h"
#include "chart_editor_context.h"
#include "chart_editor_theme.h"
#include "imgui/imgui_include.h"

namespace PeepoDrumKit
{
	struct LoadingTextAnimation
	{
		b8 WasLoadingLastFrame = false;
		u8 RingIndex = 0;
		f32 AccumulatedTimeSec = 0.0f;

		cstr UpdateFrameAndGetText(b8 isLoadingThisFrame, f32 deltaTimeSec);
	};

	struct TempoTapCalculator
	{
		i32 TapCount = 0;
		Tempo LastTempo = Tempo(0.0f);
		Tempo LastTempoMin = Tempo(0.0f), LastTempoMax = Tempo(0.0f);
		CPUStopwatch FirstTap = CPUStopwatch::StartNew();
		CPUStopwatch LastTap = CPUStopwatch::StartNew();
		Time ResetThreshold = Time::FromSec(2.0);

		inline b8 HasTimedOut() const { return ResetThreshold > Time::Zero() && LastTap.GetElapsed() >= ResetThreshold; }
		inline void Reset() { FirstTap.Stop(); TapCount = 0; LastTempo = LastTempoMin = LastTempoMax = Tempo(0.0f); }
		inline void Tap()
		{
			if (ResetThreshold > Time::Zero() && LastTap.Restart() >= ResetThreshold)
				Reset();
			FirstTap.Start();
			LastTempo = CalculateTempo(TapCount++, FirstTap.GetElapsed());
			LastTempoMin.BPM = (TapCount <= 2) ? LastTempo.BPM : Min(LastTempo.BPM, LastTempoMin.BPM);
			LastTempoMax.BPM = (TapCount <= 2) ? LastTempo.BPM : Max(LastTempo.BPM, LastTempoMax.BPM);
		}

		static constexpr Tempo CalculateTempo(i32 tapCount, Time elapsed) { return Tempo((tapCount <= 0) ? 0.0f : static_cast<f32>(60.0 * tapCount / elapsed.ToSec())); }
	};

	struct ChartHelpWindow
	{
		void DrawGui(ChartContext& context);
	};

	struct ChartUndoHistoryWindow
	{
		void DrawGui(ChartContext& context);
	};

	struct TempoCalculatorWindow
	{
		TempoTapCalculator Calculator = {};
		void DrawGui(ChartContext& context);
	};

	struct ChartInspectorWindow
	{
		// NOTE: Temp buffer only valid within the scope of the function
		struct TempChartItem { GenericList List; size_t Index; GenericMemberFlags AvailableMembers; AllGenericMembersUnionArray MemberValues; Tempo BaseScrollTempo; };
		std::vector<TempChartItem> SelectedItems;

		void DrawGui(ChartContext& context);
	};

	struct ChartPropertiesWindowIn { b8 IsSongAsyncLoading; };
	struct ChartPropertiesWindowOut { b8 BrowseOpenSong; b8 LoadNewSong; std::string NewSongFilePath; };
	struct ChartPropertiesWindow
	{
		std::string SongFileNameInputBuffer;
		LoadingTextAnimation SongLoadingTextAnimation {};
		b8 DifficultySliderStarsFitOnScreenLastFrame = false;
		b8 DifficultySliderStarsWasHoveredLastFrame = false;

		void DrawGui(ChartContext& context, const ChartPropertiesWindowIn& in, ChartPropertiesWindowOut& out);
	};

	struct ChartTempoWindow
	{
		void DrawGui(ChartContext& context, ChartTimeline& timeline);
	};

	struct ChartLyricsWindow
	{
		std::string LyricInputBuffer;
		std::string AllLyricsBuffer, AllLyricsCopyOnMadeActive;
		b8 IsLyricInputActiveThisFrame, IsLyricInputActiveLastFrame;
		b8 IsAllLyricsInputActiveThisFrame, IsAllLyricsInputActiveLastFrame;

		void DrawGui(ChartContext& context, ChartTimeline& timeline);
	};

	struct GameCamera
	{
		Rect ScreenSpaceViewportRect {};
		f32 WorldToScreenScaleFactor = 1.0f;
		vec2 WorldSpaceSize {};
		Rect LaneRect {};

		constexpr f32 LaneWidth() const { return LaneRect.GetWidth(); }
		constexpr f32 ExtendedLaneWidthFactor() const { return LaneRect.GetWidth() / GameLaneStandardWidth; }

		constexpr f32 WorldToScreenScale(f32 worldScale) const { return worldScale * WorldToScreenScaleFactor; }
		constexpr vec2 WorldToScreenScale(vec2 worldScale) const { return worldScale * WorldToScreenScaleFactor; }
		constexpr vec2 WorldToScreenSpace(vec2 worldSpace) const { return ScreenSpaceViewportRect.TL + (worldSpace * WorldToScreenScaleFactor); }

		// NOTE: Same scale as world space but with (0,0) starting at the hit-circle center point
		constexpr f32 TimeToLaneSpaceX(Time cursorTime, Time noteTime, Tempo tempo, f32 scrollSpeed) const { return ((tempo.BPM * scrollSpeed) / 60.0f) * (noteTime - cursorTime).ToSec_F32() * GameWorldSpaceDistancePerLaneBeat; }
		constexpr vec2 LaneXToWorldSpace(f32 laneX) { return (LaneRect.TL + GameHitCircle.Center + vec2(laneX, 0.0f)); }

		constexpr b8 IsPointVisibleOnLane(f32 laneX, f32 threshold = 280.0f) const { return (laneX >= -threshold) && (laneX <= (LaneWidth() + threshold)); }
		constexpr b8 IsRangeVisibleOnLane(f32 laneHeadX, f32 laneTailX, f32 threshold = 280.0f) const { return (laneTailX >= -threshold) && (laneHeadX <= (LaneWidth() + threshold)); }
	};

	struct ChartGamePreview
	{
		GameCamera Camera = {};

		struct DeferredNoteDrawData { f32 LaneHeadX, LaneTailX; const Note* OriginalNote; Time NoteStartTime, NoteEndTime; };
		std::vector<DeferredNoteDrawData> ReverseNoteDrawBuffer;

		void DrawGui(ChartContext& context, Time animatedCursorTime);
	};
}
