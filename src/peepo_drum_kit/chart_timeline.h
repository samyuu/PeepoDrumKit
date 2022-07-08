#pragma once
#include "core_types.h"
#include "core_beat.h"
#include "main_settings.h"
#include "chart_editor_context.h"
#include "chart_sound_effects.h"
#include "imgui/imgui_include.h"

// BUG: TIMELINE PERFORMANCE BUG:
//		-> zoom in as close as possible
//		-> zoom out as far as possible
//		-> performance goes to shit??

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
			Gui::AnimateExponential(&PositionCurrent, PositionTarget, GlobalSettings.TimelineSmoothScrollAnimationSpeed);
			Gui::AnimateExponential(&ZoomCurrent, ZoomTarget, GlobalSettings.TimelineSmoothScrollAnimationSpeed);

			// BUG: Result in weird jittering..?
			// static constexpr f32 snapThreshold = 0.01f;
			//if (ApproxmiatelySame(PositionCurrent, PositionTarget, snapThreshold)) PositionCurrent = PositionTarget;
			//if (ApproxmiatelySame(ZoomCurrent, ZoomTarget, snapThreshold)) ZoomCurrent = ZoomTarget;
		}

		// NOTE: Only *looks* correct when the smooth scroll and zoom speeds are similar
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
		constexpr Time WorldSpaceXToTime(f32 worldSpaceX) const { return Time::FromSeconds(worldSpaceX / WorldSpaceXUnitsPerSecond); }
		constexpr Time LocalSpaceXToTime(f32 localSpaceX) const { return WorldSpaceXToTime((localSpaceX + PositionCurrent.x) / ZoomCurrent.x); }
		constexpr Time LocalSpaceXToTime_AtTarget(f32 localSpaceX) const { return WorldSpaceXToTime((localSpaceX + PositionTarget.x) / ZoomTarget.x); }

		constexpr Time TimePerScreenPixel() const { return WorldSpaceXToTime(1.0f / ZoomCurrent.x); }
	};

	// NOTE: Making it so the Gui::DragScalar() screen space mouse movement matches the equivalent distance on the timeline
	constexpr f32 TimelineDragScalarSpeedAtZoomSec(const TimelineCamera& camera) { return static_cast<f32>(camera.TimePerScreenPixel().TotalSeconds()); }
	constexpr f32 TimelineDragScalarSpeedAtZoomMS(const TimelineCamera& camera) { return static_cast<f32>(camera.TimePerScreenPixel().TotalMilliseconds()); }

	// TODO: If branches can only change notes (??) then could have multiple rows of notes for each branch with all but the active branch being grayed out (?)
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

		bool IsAnyChildWindowFocused = false;
		bool IsContentHeaderWindowHovered = false;
		// bool IsContentHeaderWindowFocused = false;
		bool IsContentWindowHovered = false;
		bool IsContentWindowFocused = false;
		bool IsSidebarWindowHovered = false;
		// bool IsSidebarWindowFocused = false;

		vec2 MousePosThisFrame = {};
		vec2 MousePosLastFrame = {};

		bool IsCameraMouseGrabActive = false;
		bool IsCursorMouseScrubActive = false;

		struct SelectedItemDragData
		{
			bool IsActive;
			bool IsHovering;
			Beat MouseBeatThisFrame;
			Beat MouseBeatLastFrame;
			Beat BeatOnMouseDown;
			Beat BeatDistanceMovedSoFar;
		} SelectedItemDrag = {};

		struct LongNotePlacementData
		{
			bool IsActive;
			Beat CursorBeatHead;
			Beat CursorBeatTail;
			NoteType NoteType;
			inline Beat GetMin() const { return ClampBot(Min(CursorBeatHead, CursorBeatTail), Beat::Zero()); }
			inline Beat GetMax() const { return ClampBot(Max(CursorBeatHead, CursorBeatTail), Beat::Zero()); }
		} LongNotePlacement = {};
		bool PlaceBalloonBindingDownThisFrame = false, PlaceBalloonBindingDownLastFrame = false;
		bool PlaceDrumrollBindingDownThisFrame = false, PlaceDrumrollBindingDownLastFrame = false;

		enum class BoxSelectionAction : u8 { Clear, Add, Remove };
		struct BoxSelectionData
		{
			bool IsActive;
			BoxSelectionAction Action;
			Rect WorldSpaceRect;
		} BoxSelection = {};

		struct RangeSelectionData
		{
			Beat Start, End;
			bool HasEnd;
			bool IsActive;
			inline Beat GetMin() const { return ClampBot(Min(Start, End), Beat::Zero()); }
			inline Beat GetMax() const { return ClampBot(Max(Start, End), Beat::Zero()); }
		} RangeSelection = {};
		f32 RangeSelectionExpansionAnimationCurrent = 0.0f;
		f32 RangeSelectionExpansionAnimationTarget = 0.0f;

		f32 WorldSpaceCursorXAnimationCurrent = 0.0f;
		f32 GridSnapLineAnimationCurrent = 1.0f;

		bool PlaybackSoundsEnabled = true;
		struct MetronomeData
		{
			bool IsEnabled = false;
			bool HasOnPlaybackStartTimeBeenPlayed = false;
			Time LastProvidedNonSmoothCursorTime = {};
			Time LastPlayedBeatTime = {};
		} Metronome = {};

		// TODO: Implement properly and make it so the draw order stays correct by binary searching through sorted list (?)
		struct DeletedNoteAnimation { Note OriginalNote; BranchType Branch; f32 ElapsedTimeSec; };
		std::vector<DeletedNoteAnimation> TempDeletedNoteAnimationsBuffer;

		struct TempDrawSelectionBox { Rect ScreenSpaceRect; u32 FillColor, BorderColor; };
		std::vector<TempDrawSelectionBox> TempSelectionBoxesDrawBuffer;

	public:
		// TODO: Make sure this'll work correctly with popup windows too (context menu, etc.)
		inline bool HasKeyboardFocus() const { return IsAnyChildWindowFocused /*IsContentWindowFocused | IsContentHeaderWindowFocused*/; }

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

		// BUG: Using a Time overdraw param means different behavior at different zoom levels
		struct MinMaxTime { Time Min, Max; };
		inline MinMaxTime GetMinMaxVisibleTime(const Time overdraw = Time::Zero()) const
		{
			const Time minVisibleTime = Camera.LocalSpaceXToTime(0.0f) - overdraw;
			const Time maxVisibleTime = Camera.LocalSpaceXToTime(Regions.Content.GetWidth()) + overdraw;
			return { minVisibleTime, maxVisibleTime };
		}

		void DrawGui(ChartContext& context);

		void PlayNoteSoundAndHitAnimationsAtBeat(ChartContext& context, Beat cursorBeat);

		enum class ClipboardAction : u8 { Cut, Copy, Paste, Delete };
		void ExecuteClipboardAction(ChartContext& context, ClipboardAction action);

	private:
		// NOTE: Must update input *before* drawing so that the scroll positions won't change
		//		 between having drawn the timeline header and drawing the timeline content.
		//		 But this also means we have to store any of the window focus / active / hover states across frame boundary
		void UpdateInputAtStartOfFrame(ChartContext& context);

		void DrawAllAtEndOfFrame(ChartContext& context);
	};
}
