#pragma once
#include "core_types.h"
#include "core_beat.h"
#include "chart_editor_settings.h"
#include "chart_editor_context.h"
#include "chart_editor_sound.h"
#include "imgui/imgui_include.h"

namespace PeepoDrumKit
{
	// NOTE: Coordinate Systems:
	//		 Screen Space	-> Absolute position starting at the top left of the OS window / desktop (mouse cursor position, etc.)
	//		 Local Space	-> Relative position to the top left of the timeline content region
	//		 World Space	-> Virtual position within the timeline
	struct TimelineCamera
	{
		vec2 PositionCurrent = vec2(0.0f);
		vec2 PositionTarget = vec2(0.0f);

		vec2 ZoomCurrent = vec2(1.0f);
		vec2 ZoomTarget = vec2(1.0f);

		static constexpr f32 WorldSpaceXUnitsPerSecond = 300.0f;

		inline void UpdateAnimations()
		{
			// NOTE: Smooth zooming only looks correct when the smooth scroll and zoom speeds are the same
			Gui::AnimateExponential(&PositionCurrent, PositionTarget, *Settings.Animation.TimelineSmoothScrollSpeed);
			Gui::AnimateExponential(&ZoomCurrent, ZoomTarget, *Settings.Animation.TimelineSmoothScrollSpeed);
		}

		constexpr void SetZoomTargetAroundWorldPivot(vec2 newZoom, vec2 worldSpacePivot)
		{
			const vec2 localSpacePrev = WorldToLocalSpace_AtTarget(worldSpacePivot);
			ZoomTarget = Clamp(newZoom, vec2(0.001f), vec2(100.0f));
			const vec2 localSpaceNow = WorldToLocalSpace_AtTarget(worldSpacePivot);
			PositionTarget += (localSpaceNow - localSpacePrev);
		}

		constexpr void SetZoomTargetAroundLocalPivot(vec2 newZoom, vec2 localSpacePivot)
		{
			SetZoomTargetAroundWorldPivot(newZoom, LocalToWorldSpace_AtTarget(localSpacePivot));
		}

		constexpr vec2 LocalToWorldSpace(vec2 localSpace) const { return (localSpace + PositionCurrent) / ZoomCurrent; }
		constexpr vec2 LocalToWorldSpace_AtTarget(vec2 localSpace) const { return (localSpace + PositionTarget) / ZoomTarget; }
		constexpr vec2 WorldToLocalSpace(vec2 worldSpace) const { return (worldSpace * ZoomCurrent) - PositionCurrent; }
		constexpr vec2 WorldToLocalSpace_AtTarget(vec2 worldSpace) const { return (worldSpace * ZoomTarget) - PositionTarget; }
		constexpr vec2 LocalToWorldSpaceScale(vec2 localSpaceScale) const { return localSpaceScale / ZoomCurrent; }
		constexpr vec2 LocalToWorldSpaceScale_AtTarget(vec2 localSpaceScale) const { return localSpaceScale / ZoomTarget; }
		constexpr vec2 WorldToLocalSpaceScale(vec2 worldSpaceScale) const { return worldSpaceScale * ZoomCurrent; }
		constexpr vec2 WorldToLocalSpaceScale_AtTarget(vec2 worldSpaceScale) const { return worldSpaceScale * ZoomTarget; }

		constexpr f32 TimeToWorldSpaceX(Time time) const { return static_cast<f32>(time.Seconds * WorldSpaceXUnitsPerSecond); }
		constexpr f32 TimeToLocalSpaceX(Time time) const { return (TimeToWorldSpaceX(time) * ZoomCurrent.x) - PositionCurrent.x; }
		constexpr f32 TimeToLocalSpaceX_AtTarget(Time time) const { return (TimeToWorldSpaceX(time) * ZoomTarget.x) - PositionTarget.x; }
		constexpr Time WorldSpaceXToTime(f32 worldSpaceX) const { return Time::FromSec(worldSpaceX / WorldSpaceXUnitsPerSecond); }
		constexpr Time LocalSpaceXToTime(f32 localSpaceX) const { return WorldSpaceXToTime((localSpaceX + PositionCurrent.x) / ZoomCurrent.x); }
		constexpr Time LocalSpaceXToTime_AtTarget(f32 localSpaceX) const { return WorldSpaceXToTime((localSpaceX + PositionTarget.x) / ZoomTarget.x); }

		constexpr Time TimePerScreenPixel() const { return WorldSpaceXToTime(1.0f / ZoomCurrent.x); }
	};

	// NOTE: Making it so the Gui::DragScalar() screen space mouse movement matches the equivalent distance on the timeline
	constexpr f32 TimelineDragScalarSpeedAtZoomSec(const TimelineCamera& camera) { return camera.TimePerScreenPixel().ToSec_F32(); }
	constexpr f32 TimelineDragScalarSpeedAtZoomMS(const TimelineCamera& camera) { return camera.TimePerScreenPixel().ToMS_F32(); }

	enum class TimelineRowType : u8
	{
		Tempo,
		TimeSignature,
		Notes_Normal,
		Notes_Expert,
		Notes_Master,
		ScrollSpeed,
		BarLineVisibility,
		GoGoTime,
		Lyrics,
		Count,

		NoteBranches_First = Notes_Normal,
		NoteBranches_Last = Notes_Master,
	};

	constexpr std::string_view TimelineRowTypeNames[EnumCount<TimelineRowType>] =
	{
		"Tempo",
		"Time Signature",
		"Notes",
		"Notes (Expert)",
		"Notes (Master)",
		"Scroll Speed",
		"Bar Line Visibility",
		"Go-Go Time",
		"Lyrics",
	};

	constexpr GenericList TimelineRowToGenericList(TimelineRowType row)
	{
		switch (row)
		{
		case TimelineRowType::Tempo: return GenericList::TempoChanges;
		case TimelineRowType::TimeSignature: return GenericList::SignatureChanges;
		case TimelineRowType::Notes_Normal: return GenericList::Notes_Normal;
		case TimelineRowType::Notes_Expert: return GenericList::Notes_Expert;
		case TimelineRowType::Notes_Master: return GenericList::Notes_Expert;
		case TimelineRowType::ScrollSpeed: return GenericList::ScrollChanges;
		case TimelineRowType::BarLineVisibility: return GenericList::BarLineChanges;
		case TimelineRowType::GoGoTime: return GenericList::GoGoRanges;
		case TimelineRowType::Lyrics: return GenericList::Lyrics;
		default: assert(false); return GenericList::Count;
		}
	}

	constexpr TimelineRowType GenericListToTimelineRow(GenericList list)
	{
		switch (list)
		{
		case GenericList::TempoChanges: return TimelineRowType::Tempo;
		case GenericList::SignatureChanges: return TimelineRowType::TimeSignature;
		case GenericList::Notes_Normal: return TimelineRowType::Notes_Normal;
		case GenericList::Notes_Expert: return TimelineRowType::Notes_Expert;
		case GenericList::Notes_Master: return TimelineRowType::Notes_Master;
		case GenericList::ScrollChanges: return TimelineRowType::ScrollSpeed;
		case GenericList::BarLineChanges: return TimelineRowType::BarLineVisibility;
		case GenericList::GoGoRanges: return TimelineRowType::GoGoTime;
		case GenericList::Lyrics: return TimelineRowType::Lyrics;
		default: assert(false); return TimelineRowType::Count;
		}
	}

	constexpr BranchType TimelineRowToBranchType(TimelineRowType rowType)
	{
		return
			(rowType == TimelineRowType::Notes_Normal) ? BranchType::Normal :
			(rowType == TimelineRowType::Notes_Expert) ? BranchType::Expert :
			(rowType == TimelineRowType::Notes_Master) ? BranchType::Master : BranchType::Count;
	}

	struct TimelineRegions
	{
		// NOTE: Includes the entire window
		Rect Window;

		// NOTE: Left side
		Rect SidebarHeader;
		Rect Sidebar;

		// NOTE: Right side
		Rect ContentHeader;
		Rect Content;
		Rect ContentScrollbarX;
	};

	static_assert((Beat::TicksPerBeat * 4) == 192);
	constexpr i32 AllowedGridBarDivisions[] = { 4, 8, 12, 16, 24, 32, 48, 64, 96, 192 };

	// NOTE: Nudge the startup camera position and min scrollbar.x slightly to the left
	//		 so that a line drawn at worldspace position { x = 0.0f } is fully visible
	static constexpr f32 TimelineCameraBaseScrollX = -32.0f;

	enum class ClipboardAction : u8 { Cut, Copy, Paste, Delete };
	enum class SelectionAction : u8 { SelectAll, UnselectAll, InvertAll, SelectAllWithinRangeSelection, PerRowShiftSelected, PerRowSelectPattern };
	union SelectionActionParam
	{
		struct { i32 ShiftDelta; };
		struct { cstr Pattern; }; // NOTE: In the format: "xo", "xoo", "xooo", "xxoo", etc.
		inline SelectionActionParam& SetShiftDelta(i32 v) { ShiftDelta = v; return *this; }
		inline SelectionActionParam& SetPattern(cstr pattern) { Pattern = pattern; return *this; }
	};
	enum class TransformAction : u8 { FlipNoteType, ToggleNoteSize, ScaleItemTime };
	union TransformActionParam
	{
		struct { i32 TimeRatio[2]; };
		inline TransformActionParam& SetTimeRatio(i32 numerator, i32 denominator) { TimeRatio[0] = numerator; TimeRatio[1] = denominator; return *this; }
	};

	struct ChartTimeline
	{
		TimelineCamera Camera = []() { TimelineCamera out {}; out.PositionCurrent.x = out.PositionTarget.x = TimelineCameraBaseScrollX; return out; }();

		// NOTE: All in screen space
		TimelineRegions Regions = {};

		// TODO: Add splitter behavior to allow resizing (?)
		f32 CurrentSidebarWidth = 240.0f;

		// NOTE: Defined as a fraction of a 4/4 bar
		i32 CurrentGridBarDivision = 16;

		// NOTE: Only valid inside the window drawing scope
		ImDrawList* DrawListSidebarHeader = nullptr;
		ImDrawList* DrawListSidebar = nullptr;
		ImDrawList* DrawListContentHeader = nullptr;
		ImDrawList* DrawListContent = nullptr;

		b8 IsAnyChildWindowFocused = false;
		b8 IsContentHeaderWindowHovered = false;
		b8 IsContentWindowHovered = false;
		b8 IsContentWindowFocused = false;
		b8 IsSidebarWindowHovered = false;

		vec2 MousePosThisFrame = {};
		vec2 MousePosLastFrame = {};

		b8 IsCameraMouseGrabActive = false;
		b8 IsCursorMouseScrubActive = false;

		struct SelectedItemDragData
		{
			b8 IsActive;
			b8 IsHovering;
			Beat MouseBeatThisFrame;
			Beat MouseBeatLastFrame;
			Beat BeatOnMouseDown;
			Beat BeatDistanceMovedSoFar;
		} SelectedItemDrag = {};

		struct LongNotePlacementData
		{
			b8 IsActive;
			Beat CursorBeatHead;
			Beat CursorBeatTail;
			NoteType NoteType;
			inline Beat GetMin() const { return ClampBot(Min(CursorBeatHead, CursorBeatTail), Beat::Zero()); }
			inline Beat GetMax() const { return ClampBot(Max(CursorBeatHead, CursorBeatTail), Beat::Zero()); }
		} LongNotePlacement = {};
		b8 PlaceBalloonBindingDownThisFrame = false, PlaceBalloonBindingDownLastFrame = false;
		b8 PlaceDrumrollBindingDownThisFrame = false, PlaceDrumrollBindingDownLastFrame = false;

		enum class BoxSelectionAction : u8 { Clear, Add, Sub, XOR };
		struct BoxSelectionData
		{
			b8 IsActive;
			BoxSelectionAction Action;
			Rect WorldSpaceRect;
		} BoxSelection = {};

		struct RangeSelectionData
		{
			Beat Start, End;
			b8 HasEnd;
			b8 IsActive;
			inline b8 IsActiveAndHasEnd() { return (IsActive && HasEnd); }
			inline Beat GetMin() const { return ClampBot(Min(Start, End), Beat::Zero()); }
			inline Beat GetMax() const { return ClampBot(Max(Start, End), Beat::Zero()); }
		} RangeSelection = {};
		f32 RangeSelectionExpansionAnimationCurrent = 0.0f;
		f32 RangeSelectionExpansionAnimationTarget = 0.0f;

		f32 WorldSpaceCursorXAnimationCurrent = 0.0f;
		f32 GridSnapLineAnimationCurrent = 1.0f;

		b8 PlaybackSoundsEnabled = true;
		struct MetronomeData
		{
			b8 IsEnabled = false;
			b8 HasOnPlaybackStartTimeBeenPlayed = false;
			Time LastProvidedNonSmoothCursorTime = {};
			Time LastPlayedBeatTime = {};
		} Metronome = {};

		// TODO: Implement properly and make it so the draw order stays correct by binary searching through sorted list (?)
		struct DeletedNoteAnimation { Note OriginalNote; BranchType Branch; f32 ElapsedTimeSec; };
		std::vector<DeletedNoteAnimation> TempDeletedNoteAnimationsBuffer;

		struct TempDrawSelectionBox { Rect ScreenSpaceRect; u32 FillColor, BorderColor; };
		std::vector<TempDrawSelectionBox> TempSelectionBoxesDrawBuffer;

	public:
		inline b8 HasKeyboardFocus() const { return IsAnyChildWindowFocused; }

		inline Beat FloorBeatToCurrentGrid(Beat beat) const { return FloorBeatToGrid(beat, GetGridBeatSnap(CurrentGridBarDivision)); }
		inline Beat RoundBeatToCurrentGrid(Beat beat) const { return RoundBeatToGrid(beat, GetGridBeatSnap(CurrentGridBarDivision)); }
		inline Beat CeilBeatToCurrentGrid(Beat beat) const { return CeilBeatToGrid(beat, GetGridBeatSnap(CurrentGridBarDivision)); }

		inline vec2 ScreenToLocalSpace(vec2 screenSpace) const { return screenSpace - Regions.Content.TL; }
		inline vec2 LocalToScreenSpace(vec2 localSpace) const { return localSpace + Regions.Content.TL; }

		inline vec2 ScreenToLocalSpace_ContentHeader(vec2 screenSpace) const { return screenSpace - Regions.ContentHeader.TL; }
		inline vec2 LocalToScreenSpace_ContentHeader(vec2 localSpace) const { return localSpace + Regions.ContentHeader.TL; }

		inline vec2 ScreenToLocalSpace_Sidebar(vec2 screenSpace) const { return screenSpace - Regions.Sidebar.TL; }
		inline vec2 LocalToScreenSpace_Sidebar(vec2 localSpace) const { return localSpace + Regions.Sidebar.TL; }

		inline vec2 ScreenToLocalSpace_ScrollbarX(vec2 screenSpace) const { return screenSpace - Regions.ContentScrollbarX.TL; }
		inline vec2 LocalToScreenSpace_ScrollbarX(vec2 localSpace) const { return localSpace + Regions.ContentScrollbarX.TL; }

		struct MinMaxTime { Time Min, Max; };
		inline MinMaxTime GetMinMaxVisibleTime(const Time overdraw = Time::Zero()) const
		{
			const Time minVisibleTime = Camera.LocalSpaceXToTime(0.0f) - overdraw;
			const Time maxVisibleTime = Camera.LocalSpaceXToTime(Regions.Content.GetWidth()) + overdraw;
			return { minVisibleTime, maxVisibleTime };
		}

		void DrawGui(ChartContext& context);

		void StartEndRangeSelectionAtCursor(ChartContext& context);
		void PlayNoteSoundAndHitAnimationsAtBeat(ChartContext& context, Beat cursorBeat);

		void ExecuteClipboardAction(ChartContext& context, ClipboardAction action);
		void ExecuteSelectionAction(ChartContext& context, SelectionAction action, const SelectionActionParam& param);
		void ExecuteTransformAction(ChartContext& context, TransformAction action, const TransformActionParam& param);

	private:
		// NOTE: Must update input *before* drawing so that the scroll positions won't change
		//		 between having drawn the timeline header and drawing the timeline content.
		//		 But this also means we have to store any of the window focus / active / hover states across frame boundary
		void UpdateInputAtStartOfFrame(ChartContext& context);

		// NOTE: Not entirely sure about this but updating *after* all user input seems to make the most sense..?
		void UpdateAllAnimationsAfterUserInput(ChartContext& context);

		void DrawAllAtEndOfFrame(ChartContext& context);
	};
}
