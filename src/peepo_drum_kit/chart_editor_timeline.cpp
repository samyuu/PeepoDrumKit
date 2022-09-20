#include "chart_editor_timeline.h"
#include "chart_editor_undo.h"
#include "chart_editor_common.h"
#include "chart_editor_i18n.h"

namespace PeepoDrumKit
{
	static void DrawTimelineDebugWindowContent(ChartTimeline& timeline, ChartContext& context)
	{
		static constexpr ImGuiColorEditFlags colorEditFlags = ImGuiColorEditFlags_AlphaPreviewHalf;
		static constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInner;

		Gui::PushID(&timeline);
		defer { Gui::PopID(); };

		Gui::PushStyleColor(ImGuiCol_Text, 0xFFABE1E0);
		Gui::PushStyleColor(ImGuiCol_Border, 0x3EABE1E0);
		Gui::BeginChild("BackgroundChild", Gui::GetContentRegionAvail(), true);
		defer { Gui::EndChild(); Gui::PopStyleColor(2); };

		Gui::UpdateSmoothScrollWindow();

		if (Gui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (Gui::Property::BeginTable(tableFlags))
			{
				Gui::Property::PropertyTextValueFunc("Camera.PositionCurrent", [&] { Gui::SetNextItemWidth(-1.0f); Gui::DragFloat2("##PositionCurrent", timeline.Camera.PositionCurrent.data()); });
				Gui::Property::PropertyTextValueFunc("Camera.PositionTarget", [&] { Gui::SetNextItemWidth(-1.0f); Gui::DragFloat2("##PositionTarget", timeline.Camera.PositionTarget.data()); });
				Gui::Property::PropertyTextValueFunc("Camera.ZoomCurrent", [&] { Gui::SetNextItemWidth(-1.0f); Gui::DragFloat2("##ZoomCurrent", timeline.Camera.ZoomCurrent.data(), 0.01f); });
				Gui::Property::PropertyTextValueFunc("Camera.ZoomTarget", [&]
				{
					Gui::SetNextItemWidth(-1.0f);
					if (auto v = timeline.Camera.ZoomTarget; Gui::DragFloat2("##ZoomTarget", v.data(), 0.01f))
						timeline.Camera.SetZoomTargetAroundLocalPivot(v, timeline.ScreenToLocalSpace(vec2(timeline.Regions.Content.GetCenter().x, 0.0f)));
				});
				Gui::Property::EndTable();
			}
		}

		if (Gui::CollapsingHeader("Colors", ImGuiTreeNodeFlags_DefaultOpen))
		{
			struct NamedColorU32Pointer { cstr Label; u32* ColorPtr; u32 Default; };
			static NamedColorU32Pointer namedColors[] =
			{
				{ "Timeline Background", &TimelineBackgroundColor },
				{ "Timeline Out Of Bounds Dim", &TimelineOutOfBoundsDimColor },
				{ "Timeline Waveform Base", &TimelineWaveformBaseColor },
				NamedColorU32Pointer {},
				{ "Timeline Cursor", &TimelineCursorColor },
				{ "Timeline Item Text", &TimelineItemTextColor },
				{ "Timeline Item Text (Shadow)", &TimelineItemTextColorShadow },
				{ "Timeline Item Text (Warning)", &TimelineItemTextColorWarning },
				{ "Timeline Item Text Underline", &TimelineItemTextUnderlineColor },
				{ "Timeline Item Text Underline (Shadow)", &TimelineItemTextUnderlineColorShadow },
				NamedColorU32Pointer {},
				{ "Timeline GoGo Background Border", &TimelineGoGoBackgroundColorBorder },
				{ "Timeline GoGo Background Border (Selected)", &TimelineGoGoBackgroundColorBorderSelected },
				{ "Timeline GoGo Background Outer", &TimelineGoGoBackgroundColorOuter },
				{ "Timeline GoGo Background Inner", &TimelineGoGoBackgroundColorInner },
				NamedColorU32Pointer {},
				{ "Timeline Lyrics Text", &TimelineLyricsTextColor },
				{ "Timeline Lyrics Text (Shadow)", &TimelineLyricsTextColorShadow },
				{ "Timeline Lyrics Background Border", &TimelineLyricsBackgroundColorBorder },
				{ "Timeline Lyrics Background Border (Selected)", &TimelineLyricsBackgroundColorBorderSelected },
				{ "Timeline Lyrics Background Outer", &TimelineLyricsBackgroundColorOuter },
				{ "Timeline Lyrics Background Inner", &TimelineLyricsBackgroundColorInner },
				NamedColorU32Pointer {},
				{ "Timeline Horizontal Row Line", &TimelineHorizontalRowLineColor },
				{ "Grid Bar Line", &TimelineGridBarLineColor },
				{ "Grid Beat Line", &TimelineGridBeatLineColor },
				{ "Grid Snap Line", &TimelineGridSnapLineColor },
				{ "Grid Snap Line (Tuplet)", &TimelineGridSnapTupletLineColor },
				NamedColorU32Pointer {},
				{ "Box-Selection Background", &TimelineBoxSelectionBackgroundColor },
				{ "Box-Selection Border", &TimelineBoxSelectionBorderColor },
				{ "Box-Selection Fill", &TimelineBoxSelectionFillColor },
				{ "Box-Selection Inner", &TimelineBoxSelectionInnerColor },
				{ "Range-Selection Background", &TimelineRangeSelectionBackgroundColor },
				{ "Range-Selection Border", &TimelineRangeSelectionBorderColor },
				{ "Range-Selection Header Highlight", &TimelineRangeSelectionHeaderHighlightColor },
				{ "Selected Note Box Background", &TimelineSelectedNoteBoxBackgroundColor },
				{ "Selected Note Box Border", &TimelineSelectedNoteBoxBorderColor },
				NamedColorU32Pointer {},
				{ "Timeline Tempo Change Line", &TimelineTempoChangeLineColor },
				{ "Timeline Signature Change Line", &TimelineSignatureChangeLineColor },
				{ "Timeline Scroll Change Line", &TimelineScrollChangeLineColor },
				{ "Timeline Bar Line Change Line", &TimelineBarLineChangeLineColor },
				{ "Timeline Selected Item Line", &TimelineSelectedItemLineColor },
				NamedColorU32Pointer {},
				{ "Timeline Song Demo Start Marker Fill", &TimelineSongDemoStartMarkerColorFill },
				{ "Timeline Song Demo Start Marker Border", &TimelineSongDemoStartMarkerColorBorder },
				NamedColorU32Pointer {},
				{ "Game Lane Border", &GameLaneBorderColor },
				{ "Game Lane Bar Line", &GameLaneBarLineColor },
				{ "Game Lane Content Background", &GameLaneContentBackgroundColor },
				{ "Game Lane Footer Background", &GameLaneFooterBackgroundColor },
				{ "Game Lane Hit Circle Inner Fill", &GameLaneHitCircleInnerFillColor },
				{ "Game Lane Hit Circle Inner Outline", &GameLaneHitCircleInnerOutlineColor },
				{ "Game Lane Hit Circle Outer Outline", &GameLaneHitCircleOuterOutlineColor },
				NamedColorU32Pointer {},
				{ "Note Red", &NoteColorRed },
				{ "Note Blue", &NoteColorBlue },
				{ "Note Orange", &NoteColorOrange },
				{ "Note Yellow", &NoteColorYellow },
				{ "Note White", &NoteColorWhite },
				{ "Note Black", &NoteColorBlack },
				{ "Note Drumroll Hit", &NoteColorDrumrollHit },
				{ "Note Balloon Text", &NoteBalloonTextColor },
				{ "Note Balloon Text (Shadow)", &NoteBalloonTextColorShadow },
				NamedColorU32Pointer {},
				{ "Drag Text (Hovered)", &DragTextHoveredColor },
				{ "Drag Text (Active)", &DragTextActiveColor },
				{ "Input Text Warning Text", &InputTextWarningTextColor },
			};

			if (static b8 firstFrame = true; firstFrame) { for (auto& e : namedColors) { e.Default = (e.ColorPtr != nullptr) ? *e.ColorPtr : 0xFFFF00FF; } firstFrame = false; }

			static ImGuiTextFilter colorFilter;
			Gui::SetNextItemWidth(-1.0f);
			if (Gui::InputTextWithHint("##ColorFilter", "Color Filter...", colorFilter.InputBuf, ArrayCount(colorFilter.InputBuf)))
				colorFilter.Build();

			if (Gui::Property::BeginTable(tableFlags))
			{
				i32 passedColorCount = 0;
				for (const auto& namedColor : namedColors)
				{
					if (namedColor.ColorPtr == nullptr || !colorFilter.PassFilter(namedColor.Label))
						continue;

					passedColorCount++;
					Gui::PushID(namedColor.ColorPtr);
					Gui::Property::Property([&]()
					{
						Gui::AlignTextToFramePadding();
						Gui::TextUnformatted(namedColor.Label);
						if (*namedColor.ColorPtr != namedColor.Default)
						{
							Gui::SameLine(ClampBot(Gui::GetContentRegionAvail().x - Gui::GetFontSize(), 1.0f), -1.0f);
							if (Gui::ColorButton("Reset Default", ImColor(namedColor.Default), ImGuiColorEditFlags_None))
								*namedColor.ColorPtr = namedColor.Default;
						}
					});
					Gui::Property::Value([&]() { Gui::SetNextItemWidth(-1.0f); Gui::ColorEdit4_U32(namedColor.Label, namedColor.ColorPtr, colorEditFlags); });
					Gui::PopID();
				}

				if (passedColorCount <= 0) { Gui::PushStyleColor(ImGuiCol_Text, Gui::GetColorU32(ImGuiCol_TextDisabled)); Gui::Property::PropertyText("No matching color found :Sadge:"); Gui::PopStyleColor(); }

				Gui::Property::EndTable();
			}
		}

		if (Gui::CollapsingHeader("Size Variables", ImGuiTreeNodeFlags_DefaultOpen))
		{
			struct NamedVariablePtr { cstr Label; f32* ValuePtr; f32 Default; };
			static NamedVariablePtr namedVariables[] =
			{
				{ "TimelineRowHeight", &TimelineRowHeight, },
				{ "TimelineRowHeightNotes", &TimelineRowHeightNotes, },
				{ "TimelineCursorHeadHeight", &TimelineCursorHeadHeight, },
				{ "TimelineCursorHeadWidth", &TimelineCursorHeadWidth, },
				NamedVariablePtr {},
				{ "TimelineSongDemoStartMarkerWidth", &TimelineSongDemoStartMarkerWidth, },
				{ "TimelineSongDemoStartMarkerHeight", &TimelineSongDemoStartMarkerHeight, },
				{ "TimelineSelectedNoteHitBoxSizeSmall", &TimelineSelectedNoteHitBoxSizeSmall, },
				{ "TimelineSelectedNoteHitBoxSizeBig", &TimelineSelectedNoteHitBoxSizeBig, },
				NamedVariablePtr {},
				{ "TimelineNoteRadiiSmall.BlackOuter", &TimelineNoteRadiiSmall.BlackOuter, },
				{ "TimelineNoteRadiiSmall.WhiteInner", &TimelineNoteRadiiSmall.WhiteInner, },
				{ "TimelineNoteRadiiSmall.ColorInner", &TimelineNoteRadiiSmall.ColorInner, },
				{ "TimelineNoteRadiiBig.BlackOuter", &TimelineNoteRadiiBig.BlackOuter,	 },
				{ "TimelineNoteRadiiBig.WhiteInner", &TimelineNoteRadiiBig.WhiteInner,	 },
				{ "TimelineNoteRadiiBig.ColorInner", &TimelineNoteRadiiBig.ColorInner,	 },
				NamedVariablePtr {},
				{ "GameRefNoteRadiiSmall.BlackOuter", &GameRefNoteRadiiSmall.BlackOuter, },
				{ "GameRefNoteRadiiSmall.WhiteInner", &GameRefNoteRadiiSmall.WhiteInner, },
				{ "GameRefNoteRadiiSmall.ColorInner", &GameRefNoteRadiiSmall.ColorInner, },
				{ "GameRefNoteRadiiBig.BlackOuter", &GameRefNoteRadiiBig.BlackOuter,	 },
				{ "GameRefNoteRadiiBig.WhiteInner", &GameRefNoteRadiiBig.WhiteInner,	 },
				{ "GameRefNoteRadiiBig.ColorInner", &GameRefNoteRadiiBig.ColorInner,	 },
			};

			if (static b8 firstFrame = true; firstFrame) { for (auto& e : namedVariables) { e.Default = (e.ValuePtr != nullptr) ? *e.ValuePtr : 0.0f; } firstFrame = false; }

			static ImGuiTextFilter variableFilter;
			Gui::SetNextItemWidth(-1.0f);
			if (Gui::InputTextWithHint("##VariableFilter", "Variable Filter...", variableFilter.InputBuf, ArrayCount(variableFilter.InputBuf)))
				variableFilter.Build();

			if (Gui::Property::BeginTable(tableFlags))
			{
				i32 passedVariableCount = 0;
				for (const auto& namedVariable : namedVariables)
				{
					if (namedVariable.ValuePtr == nullptr || !variableFilter.PassFilter(namedVariable.Label))
						continue;

					passedVariableCount++;
					Gui::PushID(namedVariable.ValuePtr);
					Gui::Property::Property([&]()
					{
						Gui::AlignTextToFramePadding();
						Gui::TextUnformatted(namedVariable.Label);
						if (*namedVariable.ValuePtr != namedVariable.Default)
						{
							Gui::SameLine(ClampBot(Gui::GetContentRegionAvail().x - Gui::GetFontSize(), 1.0f), -1.0f);
							if (Gui::ColorButton("Reset Default", ImColor(0xFF6384AA), ImGuiColorEditFlags_NoDragDrop))
								*namedVariable.ValuePtr = namedVariable.Default;
						}
					});
					Gui::Property::Value([&]() { Gui::SetNextItemWidth(-1.0f); Gui::DragFloat(namedVariable.Label, namedVariable.ValuePtr, 0.25f); });
					Gui::PopID();
				}

				if (passedVariableCount <= 0) { Gui::PushStyleColor(ImGuiCol_Text, Gui::GetColorU32(ImGuiCol_TextDisabled)); Gui::Property::PropertyText("No matching variable found :Sadge:"); Gui::PopStyleColor(); }

				Gui::Property::EndTable();
			}
		}

		if (Gui::CollapsingHeader("Highlight Region"))
		{
			auto region = [](cstr name, Rect region) { Gui::Selectable(name); if (Gui::IsItemHovered()) { Gui::GetForegroundDrawList()->AddRect(region.TL, region.BR, ImColor(1.0f, 0.0f, 1.0f)); } };
			region("Regions.Window", timeline.Regions.Window);
			region("Regions.SidebarHeader", timeline.Regions.SidebarHeader);
			region("Regions.Sidebar", timeline.Regions.Sidebar);
			region("Regions.ContentHeader", timeline.Regions.ContentHeader);
			region("Regions.Content", timeline.Regions.Content);
			region("Regions.ContentScrollbarX", timeline.Regions.ContentScrollbarX);
		}
	}
}

namespace PeepoDrumKit
{
	// TODO: Maybe use f64 for all timeline units? "dvec2"?
	struct ForEachRowData
	{
		TimelineRowType RowType;
		f32 LocalY;
		f32 LocalHeight;
		std::string_view Label;
	};

	template <typename Func>
	static void ForEachTimelineRow(ChartTimeline& timeline, Func perRowFunc)
	{
		f32 localY = 0.0f;
		for (TimelineRowType rowType = {}; rowType < TimelineRowType::Count; IncrementEnum(rowType))
		{
			// HACK: Draw the non default branches smaller for now to waste less space (might wanna rethink all of this...)
			const b8 isNotesRow =
				(rowType >= TimelineRowType::NoteBranches_First && rowType <= TimelineRowType::NoteBranches_Last) &&
				(TimelineRowToBranchType(rowType) == BranchType::Normal);

			const f32 localHeight = GuiScale(isNotesRow ? TimelineRowHeightNotes : TimelineRowHeight);

			perRowFunc(ForEachRowData { rowType, localY, localHeight, UI_StrRuntime(TimelineRowTypeNames[EnumToIndex(rowType)]) });
			localY += localHeight;
		}
	}

	static f32 GetTotalTimelineRowsHeight(const ChartTimeline& timeline)
	{
		f32 totalHeight = 0.0f;
		ForEachTimelineRow(*const_cast<ChartTimeline*>(&timeline), [&](const ForEachRowData& it) { totalHeight += it.LocalHeight; });
		return totalHeight;
	}

	struct ForEachGridLineData
	{
		Time Time;
		i32 BarIndex;
		b8 IsBar;
	};

	template <typename Func>
	static void ForEachTimelineVisibleGridLine(ChartTimeline& timeline, ChartContext& context, Time visibleTimeOverdraw, Func perGridFunc)
	{
		// TODO: Rewrite all of this to correctly handle tempo map changes and take GuiScaleFactor text size into account
		/* // TEMP: */ static constexpr Time GridTimeStep = Time::FromSec(1.0);
		if (GridTimeStep.Seconds <= 0.0)
			return;

		// TODO: Not too sure about the exact values here, these just came from randomly trying out numbers until it looked about right
		const f32 minAllowedSpacing = GuiScale(128.0f);
		const i32 maxSubDivisions = 6;

		// TODO: Go the extra step and add a fade animation for these too
		i32 gridLineSubDivisions = 0;
		for (gridLineSubDivisions = 0; gridLineSubDivisions < maxSubDivisions; gridLineSubDivisions++)
		{
			const f32 localSpaceXPerGridLine = timeline.Camera.WorldToLocalSpaceScale(vec2(timeline.Camera.TimeToWorldSpaceX(GridTimeStep), 0.0f)).x;
			const f32 localSpaceXPerGridLineSubDivided = localSpaceXPerGridLine * static_cast<f32>(gridLineSubDivisions + 1);

			if (localSpaceXPerGridLineSubDivided >= (minAllowedSpacing / (gridLineSubDivisions + 1)))
				break;
		}

		const auto minMaxVisibleTime = timeline.GetMinMaxVisibleTime(visibleTimeOverdraw);
		const i32 gridLineModToSkip = (1 << gridLineSubDivisions);
		i32 gridLineIndex = 0;

		const Time chartDuration = context.Chart.GetDurationOrDefault();
		context.ChartSelectedCourse->TempoMap.ForEachBeatBar([&](const SortedTempoMap::ForEachBeatBarData& it)
		{
			const Time timeIt = context.ChartSelectedCourse->TempoMap.BeatToTime(it.Beat);

			if ((gridLineIndex++ % gridLineModToSkip) == 0)
			{
				if (timeIt >= minMaxVisibleTime.Min && timeIt <= minMaxVisibleTime.Max)
					perGridFunc(ForEachGridLineData { timeIt, it.BarIndex, it.IsBar });
			}

			if (timeIt >= minMaxVisibleTime.Max || (it.IsBar && timeIt >= chartDuration))
				return ControlFlow::Break;
			else
				return ControlFlow::Continue;
		});
	}

	// NOTE: Drawing the waveform on top was originally intended to ensure it is always visible, though it can admittedly looks a bit weird at times...
	enum class WaveformDrawOrder { Background, Foreground };
	static constexpr WaveformDrawOrder TimelineWaveformDrawOrder = WaveformDrawOrder::Background;

	static constexpr f32 NoteHitAnimationDuration = 0.06f;
	static constexpr f32 NoteHitAnimationScaleStart = 1.35f, NoteHitAnimationScaleEnd = 1.0f;
	static constexpr f32 NoteDeleteAnimationDuration = 0.04f;

	static constexpr SoundEffectType SoundEffectTypeForNoteType(NoteType noteType)
	{
		return IsKaNote(noteType) ? SoundEffectType::TaikoKa : SoundEffectType::TaikoDon;
	}

	static b8 IsTimelineCursorVisibleOnScreen(const TimelineCamera& camera, const TimelineRegions& regions, const Time cursorTime, const f32 edgePixelThreshold = 0.0f)
	{
		assert(edgePixelThreshold >= 0.0f);
		const f32 cursorLocalSpaceX = camera.TimeToLocalSpaceX(cursorTime);
		return (cursorLocalSpaceX >= edgePixelThreshold && cursorLocalSpaceX <= ClampBot(regions.Content.GetWidth() - edgePixelThreshold, edgePixelThreshold));
	}

	static void ScrollToTimelinePosition(TimelineCamera& camera, const TimelineRegions& regions, const ChartProject& chart, f32 normalizedTargetPosition)
	{
		const f32 visibleWidth = regions.Content.GetWidth();
		const f32 totalTimelineWidth = camera.WorldToLocalSpaceScale(vec2(camera.TimeToWorldSpaceX(chart.GetDurationOrDefault()), 0.0f)).x + 2.0f;

		const f32 cameraTargetPosition = TimelineCameraBaseScrollX + (totalTimelineWidth - visibleWidth - TimelineCameraBaseScrollX) * normalizedTargetPosition;
		camera.PositionTarget.x = ClampBot(cameraTargetPosition, TimelineCameraBaseScrollX);
	}

	static f32 GetNotesWaveAnimationTimeAtIndex(i32 noteIndex, i32 notesCount, i32 direction)
	{
		// TODO: Proper "wave" placement animation (also maybe scale by beat instead of index?)
		const i32 animationIndex = (direction > 0) ? noteIndex : (notesCount - noteIndex);
		const f32 noteCountAnimationFactor = (1.0f / notesCount) * 2.0f;
		return NoteHitAnimationDuration + (animationIndex * (NoteHitAnimationDuration * noteCountAnimationFactor));
	}

	static void SetNotesWaveAnimationTimes(std::vector<Note>& notesToAnimate, i32 direction = +1)
	{
		const i32 notesCount = static_cast<i32>(notesToAnimate.size());
		for (i32 noteIndex = 0; noteIndex < notesCount; noteIndex++)
			notesToAnimate[noteIndex].ClickAnimationTimeDuration = notesToAnimate[noteIndex].ClickAnimationTimeRemaining = GetNotesWaveAnimationTimeAtIndex(noteIndex, notesCount, direction);
	}

	static void SetNotesWaveAnimationTimes(std::vector<GenericListStructWithType>& noteItemsToAnimate, i32 direction = +1)
	{
		i32 notesCount = 0, noteIndex = 0;
		for (auto& item : noteItemsToAnimate) { if (IsNotesList(item.List)) notesCount++; }

		for (auto& item : noteItemsToAnimate)
			if (IsNotesList(item.List))
				item.Value.POD.Note.ClickAnimationTimeDuration = item.Value.POD.Note.ClickAnimationTimeRemaining = GetNotesWaveAnimationTimeAtIndex(noteIndex++, notesCount, direction);
	}

	static f32 GetTimelineNoteScaleFactor(b8 isPlayback, Time cursorTime, Beat cursorBeatOnPlaybackStart, const Note& note, Time noteTime)
	{
		// TODO: Handle AnimationTime > AnimationDuration differently so that range selected multi note placement can have a nice "wave propagation" effect
		if (note.ClickAnimationTimeRemaining > 0.0f)
			return ConvertRange<f32>(note.ClickAnimationTimeDuration, 0.0f, NoteHitAnimationScaleStart, NoteHitAnimationScaleEnd, note.ClickAnimationTimeRemaining);

		if (isPlayback && note.BeatTime >= cursorBeatOnPlaybackStart)
		{
			// TODO: Rewirte cleanup using ConvertRange
			const f32 timeUntilNote = (noteTime - cursorTime).ToSec_F32();
			if (timeUntilNote <= 0.0f && timeUntilNote >= -NoteHitAnimationDuration)
			{
				const f32 delta = (timeUntilNote / -NoteHitAnimationDuration);
				return Lerp(NoteHitAnimationScaleStart, NoteHitAnimationScaleEnd, delta);
			}
		}

		return 1.0f;
	}

	static void DrawSingleWaveformLine(ImDrawList* drawList, vec2 screenSpaceCenter, f32 halfLineHeight, u32 waveformColor)
	{
		drawList->AddLine(screenSpaceCenter - vec2(0.0f, halfLineHeight), screenSpaceCenter + vec2(0.0f, halfLineHeight), waveformColor);
	}

	static void DrawTimelineContentWaveform(const ChartTimeline& timeline, ImDrawList* drawList, Time chartSongOffset, const Audio::WaveformMipChain& waveformL, const Audio::WaveformMipChain& waveformR, f32 waveformAnimation)
	{
		const f32 waveformAnimationScale = Clamp(waveformAnimation, 0.0f, 1.0f);
		const f32 waveformAnimationAlpha = (waveformAnimationScale * waveformAnimationScale);
		const u32 waveformColor = Gui::ColorU32WithAlpha(TimelineWaveformBaseColor, waveformAnimationAlpha * 0.215f * (waveformR.IsEmpty() ? 2.0f : 1.0f));

		const Time waveformTimePerPixel = timeline.Camera.LocalSpaceXToTime(1.0f) - timeline.Camera.LocalSpaceXToTime(0.0f);
		const Time waveformDuration = waveformL.GetDuration();

		const Rect contentRect = timeline.Regions.Content;
		const f32 halfHeight = GetTotalTimelineRowsHeight(timeline) * 0.5f;

		for (size_t waveformIndex = 0; waveformIndex < 2; waveformIndex++)
		{
			const auto& waveform = (waveformIndex == 0) ? waveformL : waveformR;
			if (waveform.IsEmpty())
				continue;

			const auto& waveformMip = waveform.FindClosestMip(waveformTimePerPixel);
			for (i32 visiblePixel = 0; visiblePixel < contentRect.GetWidth(); visiblePixel++)
			{
				const vec2 localCenter = vec2(static_cast<f32>(visiblePixel), halfHeight);
				const vec2 screenCenter = timeline.LocalToScreenSpace(localCenter);
				const Time timeAtPixel = timeline.Camera.LocalSpaceXToTime(localCenter.x) - chartSongOffset;
				if (timeAtPixel < Time::Zero()) continue;
				if (timeAtPixel > waveformDuration) break;
				DrawSingleWaveformLine(drawList,
					screenCenter, waveformAnimationScale * Clamp(waveform.GetAmplitudeAt(waveformMip, timeAtPixel, waveformTimePerPixel) * halfHeight, 1.0f, halfHeight), waveformColor);
			}
		}
	}

	struct DrawTimelineContentItemRowParam
	{
		ChartTimeline& Timeline;
		ChartContext& Context;
		ImDrawList* DrawListContent;
		ChartTimeline::MinMaxTime VisibleTime;
		b8 IsPlayback;
		Time CursorTime;
		Beat CursorBeatOnPlaybackStart;
	};

	template <typename T, TimelineRowType RowType>
	static void DrawTimelineContentItemRowT(DrawTimelineContentItemRowParam param, const ForEachRowData& rowIt, const BeatSortedList<T>& list)
	{
		ChartTimeline& timeline = param.Timeline;
		ChartContext& context = param.Context;
		ImDrawList* drawListContent = param.DrawListContent;
		const TimelineCamera& camera = timeline.Camera;
		const ChartTimeline::MinMaxTime visibleTime = param.VisibleTime;

		// TODO: Do culling by first determining min/max visible beat times then use those to continue / break inside loop (although problematic with TimeOffset?)
		if constexpr (std::is_same_v<T, Note>)
		{
			// TODO: Draw unselected branch notes grayed and at a slightly smaller scale (also nicely animate between selecting different branched!)

			// TODO: It looks like there'll also have to be one scroll speed lane per branch type
			//		 which means the scroll speed change line should probably extend all to the way down to its corresponding note lane (?)

			for (const Note& it : list)
			{
				const Time startTime = context.BeatToTime(it.GetStart()) + it.TimeOffset;
				const Time endTime = (it.BeatDuration > Beat::Zero()) ? context.BeatToTime(it.GetEnd()) + it.TimeOffset : startTime;
				if (endTime < visibleTime.Min || startTime > visibleTime.Max)
					continue;

				const vec2 localTL = vec2(timeline.Camera.TimeToLocalSpaceX(startTime), rowIt.LocalY);
				const vec2 localCenter = localTL + vec2(0.0f, rowIt.LocalHeight * 0.5f);

				if (it.BeatDuration > Beat::Zero())
				{
					const vec2 localTR = vec2(timeline.Camera.TimeToLocalSpaceX(endTime), rowIt.LocalY);
					const vec2 localCenterEnd = localTR + vec2(0.0f, rowIt.LocalHeight * 0.5f);
					DrawTimelineNoteDuration(drawListContent, timeline.LocalToScreenSpace(localCenter), timeline.LocalToScreenSpace(localCenterEnd), it.Type);
				}

				const f32 noteScaleFactor = GetTimelineNoteScaleFactor(param.IsPlayback, param.CursorTime, param.CursorBeatOnPlaybackStart, it, startTime);
				DrawTimelineNote(drawListContent, timeline.LocalToScreenSpace(localCenter), noteScaleFactor, it.Type);

				if (IsBalloonNote(it.Type) || it.BalloonPopCount > 0)
					DrawTimelineNoteBalloonPopCount(drawListContent, timeline.LocalToScreenSpace(localCenter), noteScaleFactor, it.BalloonPopCount);

				if (it.IsSelected)
				{
					// NOTE: Draw the note itself with the time offset applied but draw the hitbox at the original beat center
					const f32 localSpaceTimeOffsetX = timeline.Camera.WorldToLocalSpaceScale(vec2(timeline.Camera.TimeToWorldSpaceX(it.TimeOffset), 0.0f)).x;

					const vec2 hitBoxSize = vec2(GuiScale((IsBigNote(it.Type) ? TimelineSelectedNoteHitBoxSizeBig : TimelineSelectedNoteHitBoxSizeSmall)));
					timeline.TempSelectionBoxesDrawBuffer.push_back(ChartTimeline::TempDrawSelectionBox { Rect::FromCenterSize(timeline.LocalToScreenSpace(localCenter - vec2(localSpaceTimeOffsetX, 0.0f)), hitBoxSize), TimelineSelectedNoteBoxBackgroundColor, TimelineSelectedNoteBoxBorderColor });
				}
			}

			static constexpr BranchType branchForThisRow = TimelineRowToBranchType(RowType);
			if (!timeline.TempDeletedNoteAnimationsBuffer.empty())
			{
				for (const auto& data : timeline.TempDeletedNoteAnimationsBuffer)
				{
					if (data.Branch != branchForThisRow)
						continue;

					const Time startTime = context.BeatToTime(data.OriginalNote.BeatTime) + data.OriginalNote.TimeOffset;
					const Time endTime = startTime;
					if (endTime < visibleTime.Min || startTime > visibleTime.Max)
						continue;

					const vec2 localTL = vec2(camera.TimeToLocalSpaceX(startTime), rowIt.LocalY);
					const vec2 localCenter = localTL + vec2(0.0f, rowIt.LocalHeight * 0.5f);

					const f32 noteScaleFactor = ConvertRange(0.0f, NoteDeleteAnimationDuration, 1.0f, 0.0f, ClampBot(data.ElapsedTimeSec, 0.0f));
					DrawTimelineNote(drawListContent, timeline.LocalToScreenSpace(localCenter), noteScaleFactor, data.OriginalNote.Type);
					// TODO: Also animate duration fading out or "collapsing" some other way (?)
				}
			}

			if (!timeline.TempSelectionBoxesDrawBuffer.empty())
			{
				for (const auto& box : timeline.TempSelectionBoxesDrawBuffer)
				{
					drawListContent->AddRectFilled(box.ScreenSpaceRect.TL, box.ScreenSpaceRect.BR, box.FillColor);
					drawListContent->AddRect(box.ScreenSpaceRect.TL, box.ScreenSpaceRect.BR, box.BorderColor);
				}
				timeline.TempSelectionBoxesDrawBuffer.clear();
			}

			// NOTE: Long note placement preview
			if (timeline.LongNotePlacement.IsActive && context.ChartSelectedBranch == branchForThisRow)
			{
				const Beat minBeat = timeline.LongNotePlacement.GetMin(), maxBeat = timeline.LongNotePlacement.GetMax();
				const vec2 localTL = vec2(timeline.Camera.TimeToLocalSpaceX(context.BeatToTime(minBeat)), rowIt.LocalY);
				const vec2 localCenter = localTL + vec2(0.0f, rowIt.LocalHeight * 0.5f);
				const vec2 localTR = vec2(timeline.Camera.TimeToLocalSpaceX(context.BeatToTime(maxBeat)), rowIt.LocalY);
				const vec2 localCenterEnd = localTR + vec2(0.0f, rowIt.LocalHeight * 0.5f);
				DrawTimelineNoteDuration(drawListContent, timeline.LocalToScreenSpace(localCenter), timeline.LocalToScreenSpace(localCenterEnd), timeline.LongNotePlacement.NoteType, 0.7f);
				DrawTimelineNote(drawListContent, timeline.LocalToScreenSpace(localCenter), 1.0f, timeline.LongNotePlacement.NoteType, 0.7f);

				if (IsBalloonNote(timeline.LongNotePlacement.NoteType))
					DrawTimelineNoteBalloonPopCount(drawListContent, timeline.LocalToScreenSpace(localCenter), 1.0f, DefaultBalloonPopCount(maxBeat - minBeat, timeline.CurrentGridBarDivision));
			}
		}
		else if constexpr (std::is_same_v<T, GoGoRange>)
		{
			for (const GoGoRange& it : list)
			{
				const Time startTime = context.BeatToTime(it.GetStart());
				const Time endTime = context.BeatToTime(it.GetEnd());
				if (endTime < visibleTime.Min || startTime > visibleTime.Max)
					continue;

				static constexpr f32 margin = 1.0f;
				const vec2 localTL = vec2(camera.TimeToLocalSpaceX(startTime), 0.0f) + vec2(0.0f, rowIt.LocalY + margin);
				const vec2 localBR = vec2(camera.TimeToLocalSpaceX(endTime), 0.0f) + vec2(0.0f, rowIt.LocalY + rowIt.LocalHeight - (margin * 2.0f));
				DrawTimelineGoGoTimeBackground(drawListContent, timeline.LocalToScreenSpace(localTL) + vec2(0.0f, 2.0f), timeline.LocalToScreenSpace(localBR), it.ExpansionAnimationCurrent, it.IsSelected);
			}
		}
		else if constexpr (std::is_same_v<T, LyricChange>)
		{
			const Beat chartBeatDuration = context.TimeToBeat(context.Chart.GetDurationOrDefault());

			Gui::PushFont(FontMain_JP);
			for (size_t i = 0; i < list.size(); i++)
			{
				const LyricChange* prevLyric = IndexOrNull(static_cast<i32>(i) - 1, list);
				const LyricChange& thisLyric = list[i];
				const LyricChange* nextLyric = IndexOrNull(i + 1, list);
				if (thisLyric.Lyric.empty() && prevLyric != nullptr && !prevLyric->Lyric.empty())
					continue;

				const Beat lastBeat = (thisLyric.BeatTime <= chartBeatDuration) ? chartBeatDuration : Beat::FromTicks(I32Max);
				const Time startTime = context.BeatToTime(thisLyric.BeatTime);
				const Time endTime = context.BeatToTime(thisLyric.Lyric.empty() ? thisLyric.BeatTime : (nextLyric != nullptr) ? nextLyric->BeatTime : lastBeat);
				if (endTime < visibleTime.Min || startTime > visibleTime.Max)
					continue;

				const vec2 localSpaceTL = vec2(camera.TimeToLocalSpaceX(startTime), rowIt.LocalY);
				const vec2 localSpaceBR = vec2(camera.TimeToLocalSpaceX(endTime), rowIt.LocalY + rowIt.LocalHeight);
				const vec2 localSpaceCenter = localSpaceTL + vec2(0.0f, rowIt.LocalHeight * 0.5f);

				const f32 textHeight = Gui::GetFontSize();
				const vec2 textPosition = timeline.LocalToScreenSpace(localSpaceCenter + vec2(4.0f, textHeight * -0.5f));

				Gui::DisableFontPixelSnap(true);
				{
					const Rect lyricsBarRect = Rect(timeline.LocalToScreenSpace(localSpaceTL + vec2(0.0f, 1.0f)), timeline.LocalToScreenSpace(localSpaceBR));

					DrawTimelineLyricsBackground(drawListContent, lyricsBarRect.TL, lyricsBarRect.BR, thisLyric.IsSelected);

					// HACK: Try to at least somewhat visalize the tail being selected, for now
					if (thisLyric.IsSelected || (nextLyric != nullptr && nextLyric->IsSelected))
					{
						const vec2 width = vec2(GuiScale(1.0f), 0.0f);
						drawListContent->AddRectFilled(lyricsBarRect.GetTR() - width, lyricsBarRect.GetBR() + width, (nextLyric != nullptr && nextLyric->IsSelected) ? TimelineLyricsBackgroundColorBorderSelected : TimelineLyricsBackgroundColorBorder);
					}

					static constexpr f32 borderLeft = 2.0f, borderRight = 5.0f;
					const ImVec4 clipRect = { lyricsBarRect.TL.x + borderLeft, lyricsBarRect.TL.y, lyricsBarRect.BR.x - borderRight, lyricsBarRect.BR.y };

					if (Absolute(clipRect.z - clipRect.x) > (borderLeft + borderRight))
						Gui::AddTextWithDropShadow(drawListContent, nullptr, 0.0f, textPosition, TimelineLyricsTextColor, thisLyric.Lyric, 0.0f, &clipRect, TimelineLyricsTextColorShadow);
				}
				Gui::DisableFontPixelSnap(false);
			}
			Gui::PopFont();
		}
		else
		{
			static constexpr f32 compactFormatStringZoomLevelThreshold = 0.25f;
			const b8 useCompactFormat = (camera.ZoomTarget.x < compactFormatStringZoomLevelThreshold);
			const f32 textHeight = Gui::GetFontSize();

			for (const auto& it : list)
			{
				const Time startTime = context.BeatToTime(GetBeat(it));
				if (startTime < visibleTime.Min || startTime > visibleTime.Max)
					continue;

				const vec2 localSpaceTL = vec2(camera.TimeToLocalSpaceX(startTime), rowIt.LocalY);
				const vec2 localSpaceBL = localSpaceTL + vec2(0.0f, rowIt.LocalHeight);
				const vec2 localSpaceCenter = localSpaceTL + vec2(0.0f, rowIt.LocalHeight * 0.5f);
				const vec2 textPosition = timeline.LocalToScreenSpace(localSpaceCenter + vec2(3.0f, textHeight * -0.5f));

				Gui::DisableFontPixelSnap(true);
				{
					[[maybe_unused]] char b[32]; std::string_view text; u32 lineColor; u32 textColor = TimelineItemTextColor;
					if constexpr (std::is_same_v<T, TempoChange>) { text = std::string_view(b, sprintf_s(b, useCompactFormat ? "%.0f BPM" : "%g BPM", it.Tempo.BPM)); lineColor = TimelineTempoChangeLineColor; }
					if constexpr (std::is_same_v<T, TimeSignatureChange>) { text = std::string_view(b, sprintf_s(b, "%d/%d", it.Signature.Numerator, it.Signature.Denominator)); lineColor = TimelineSignatureChangeLineColor; textColor = IsTimeSignatureSupported(it.Signature) ? TimelineItemTextColor : TimelineItemTextColorWarning; }
					if constexpr (std::is_same_v<T, ScrollChange>) { text = std::string_view(b, sprintf_s(b, "%gx", it.ScrollSpeed)); lineColor = TimelineScrollChangeLineColor; }
					if constexpr (std::is_same_v<T, BarLineChange>) { text = it.IsVisible ? "On" : "Off"; lineColor = TimelineBarLineChangeLineColor; }

					const vec2 textSize = Gui::CalcTextSize(text);
					drawListContent->AddRectFilled(vec2(timeline.LocalToScreenSpace(localSpaceTL).x, textPosition.y), textPosition + textSize, TimelineBackgroundColor);
					drawListContent->AddLine(timeline.LocalToScreenSpace(localSpaceTL + vec2(0.0f, 1.0f)), timeline.LocalToScreenSpace(localSpaceBL), it.IsSelected ? TimelineSelectedItemLineColor : lineColor);
					Gui::AddTextWithDropShadow(drawListContent, textPosition, textColor, text, TimelineItemTextColorShadow);

					if (it.IsSelected)
					{
						const vec2 lineStartEnd[2] = { textPosition + vec2(0.0f, textSize.y), textPosition + textSize };
						drawListContent->AddLine(lineStartEnd[0] + vec2(1.0f), lineStartEnd[1] + vec2(1.0f), TimelineItemTextUnderlineColorShadow);
						drawListContent->AddLine(lineStartEnd[0], lineStartEnd[1], TimelineItemTextUnderlineColor);
					}
				}
				Gui::DisableFontPixelSnap(false);
			}
		}
	}

	static constexpr f32 TimeToScrollbarLocalSpaceX(Time time, const TimelineRegions& regions, Time chartDuration)
	{
		return static_cast<f32>(ConvertRange<f64>(0.0, chartDuration.ToSec(), 0.0, regions.ContentScrollbarX.GetWidth(), time.ToSec()));
	}

	static constexpr f32 TimeToScrollbarLocalSpaceXClamped(Time time, const TimelineRegions& regions, Time chartDuration)
	{
		return Clamp(TimeToScrollbarLocalSpaceX(time, regions, chartDuration), 1.0f, regions.ContentScrollbarX.GetWidth() - 2.0f);
	}

	static void DrawTimelineScrollbarXWaveform(const ChartTimeline& timeline, ImDrawList* drawList, Time chartSongOffset, Time chartDuration, const Audio::WaveformMipChain& waveformL, const Audio::WaveformMipChain& waveformR, f32 waveformAnimation)
	{
		assert(!waveformL.IsEmpty());
		const f32 waveformAnimationScale = Clamp(waveformAnimation, 0.0f, 1.0f);
		const f32 waveformAnimationAlpha = (waveformAnimationScale * waveformAnimationScale);
		const u32 waveformColor = Gui::ColorU32WithAlpha(TimelineWaveformBaseColor, waveformAnimationAlpha * 0.5f * (waveformR.IsEmpty() ? 2.0f : 1.0f));

		const Time waveformTimePerPixel = Time::FromSec(chartDuration.ToSec() / ClampBot(timeline.Regions.ContentScrollbarX.GetWidth(), 1.0f));
		const Time waveformDuration = waveformL.GetDuration();

		const Rect scrollbarRect = timeline.Regions.ContentScrollbarX;
		const f32 scrollbarHalfHeight = scrollbarRect.GetHeight() * 0.5f;

		for (size_t waveformIndex = 0; waveformIndex < 2; waveformIndex++)
		{
			const auto& waveform = (waveformIndex == 0) ? waveformL : waveformR;
			if (waveform.IsEmpty())
				continue;

			const auto& waveformMip = waveform.FindClosestMip(waveformTimePerPixel);
			for (i32 visiblePixel = 0; visiblePixel < scrollbarRect.GetWidth(); visiblePixel++)
			{
				const vec2 localCenter = vec2(static_cast<f32>(visiblePixel), scrollbarHalfHeight);
				const vec2 screenCenter = timeline.LocalToScreenSpace_ScrollbarX(localCenter);
				const Time timeAtPixel = (waveformTimePerPixel * static_cast<f64>(visiblePixel)) - chartSongOffset;
				if (timeAtPixel < Time::Zero()) continue;
				if (timeAtPixel > waveformDuration) break;
				DrawSingleWaveformLine(drawList,
					screenCenter, Clamp(waveform.GetAmplitudeAt(waveformMip, timeAtPixel, waveformTimePerPixel) * scrollbarHalfHeight, 1.0f, scrollbarHalfHeight), waveformColor);
			}
		}
	}

	static void DrawTimelineScrollbarXMinimap(const ChartTimeline& timeline, ImDrawList* drawList, const ChartCourse& course, BranchType branch, Time chartDuration)
	{
		const vec2 localNoteRectSize = GuiScale(vec2(2.0f, 4.0f)); // timeline.Regions.ContentScrollbarX.GetHeight() * 0.25f;
		const f32 localNoteCenterY = timeline.Regions.ContentScrollbarX.GetHeight() * /*0.5f*//*0.75f*/0.25f;

		// TODO: Also draw other timeline items... tempo / signature changes, gogo-time etc. (?)
		for (const Note& note : course.GetNotes(branch))
		{
			const f32 localHeadX = TimeToScrollbarLocalSpaceX(course.TempoMap.BeatToTime(note.GetStart()) + note.TimeOffset, timeline.Regions, chartDuration);
			Rect screenNoteRect = Rect::FromCenterSize(timeline.LocalToScreenSpace_ScrollbarX(vec2(localHeadX, localNoteCenterY)), localNoteRectSize);

			if (note.BeatDuration > Beat::Zero())
			{
				const f32 localTailX = TimeToScrollbarLocalSpaceX(course.TempoMap.BeatToTime(note.GetEnd()) + note.TimeOffset, timeline.Regions, chartDuration);
				screenNoteRect.BR.x += (localTailX - localHeadX);
			}

			drawList->AddRectFilled(screenNoteRect.TL, screenNoteRect.BR, note.IsSelected ? NoteColorWhite : *NoteTypeToColorMap[EnumToIndex(note.Type)]);
			// drawList->AddRect(screenNoteRect.TL, screenNoteRect.BR, NoteColorWhite, 0.0f, ImDrawFlags_None, 2.0f);
		}
	}

	static void UpdateTimelinePlaybackAndMetronomneSounds(ChartContext& context, b8 playbackSoundsEnabled, ChartTimeline::MetronomeData& metronome)
	{
		static constexpr Time frameTimeThresholdAtWhichPlayingSoundsMakesNoSense = Time::FromMS(250.0);
		static constexpr Time playbackSoundFutureOffset = Time::FromSec(1.0 / 25.0);

		Time& nonSmoothCursorThisFrame = context.CursorNonSmoothTimeThisFrame;
		Time& nonSmoothCursorLastFrame = context.CursorNonSmoothTimeLastFrame;

		nonSmoothCursorLastFrame = nonSmoothCursorThisFrame;
		nonSmoothCursorThisFrame = (context.SongVoice.GetPosition() + context.Chart.SongOffset);

		const Time elapsedCursorTimeSinceLastUpdate = (nonSmoothCursorLastFrame - nonSmoothCursorThisFrame);
		if (elapsedCursorTimeSinceLastUpdate >= frameTimeThresholdAtWhichPlayingSoundsMakesNoSense)
			return;

		const Time futureOffset = (playbackSoundFutureOffset * Min(context.SongVoice.GetPlaybackSpeed(), 1.0f));

		if (playbackSoundsEnabled)
		{
			auto checkAndPlayNoteSound = [&](Time noteTime, NoteType noteType)
			{
				const Time offsetNoteTime = noteTime - futureOffset;
				if (offsetNoteTime >= nonSmoothCursorLastFrame && offsetNoteTime < nonSmoothCursorThisFrame)
				{
					// NOTE: Don't wanna cause any audio cutoffs. If this happens the future threshold is either set too low for the current frame time
					//		 or playback was started on top of an existing note
					const Time startTime = Min((nonSmoothCursorThisFrame - noteTime), Time::Zero());
					const Time externalClock = noteTime;

					context.SfxVoicePool.PlaySound(SoundEffectTypeForNoteType(noteType), startTime, externalClock);
				}
			};

			for (const Note& note : context.ChartSelectedCourse->GetNotes(context.ChartSelectedBranch))
			{
				if (note.BeatDuration > Beat::Zero())
				{
					if (IsBalloonNote(note.Type))
					{
						checkAndPlayNoteSound(context.BeatToTime(note.BeatTime) + note.TimeOffset, note.Type);

						const Beat balloonBeatInterval = (note.BalloonPopCount > 0) ? (note.BeatDuration / note.BalloonPopCount) : Beat::Zero();
						if (balloonBeatInterval > Beat::Zero())
						{
							i32 remainingPops = note.BalloonPopCount;
							for (Beat subBeat = balloonBeatInterval; (subBeat < note.BeatDuration) && (--remainingPops > 0); subBeat += balloonBeatInterval)
								checkAndPlayNoteSound(context.BeatToTime(note.BeatTime + subBeat) + note.TimeOffset, note.Type);
						}
					}
					else
					{
						const Beat drummrollBeatInterval = GetGridBeatSnap(*Settings.General.DrumrollAutoHitBarDivision);
						for (Beat subBeat = Beat::Zero(); subBeat <= note.BeatDuration; subBeat += drummrollBeatInterval)
							checkAndPlayNoteSound(context.BeatToTime(note.BeatTime + subBeat) + note.TimeOffset, note.Type);
					}
				}
				else
				{
					checkAndPlayNoteSound(context.BeatToTime(note.BeatTime) + note.TimeOffset, note.Type);
				}
			}
		}

		if (metronome.IsEnabled)
		{
			if (nonSmoothCursorLastFrame != metronome.LastProvidedNonSmoothCursorTime)
			{
				metronome.LastPlayedBeatTime = Time::FromSec(F64Min);
				metronome.HasOnPlaybackStartTimeBeenPlayed = false;
			}
			metronome.LastProvidedNonSmoothCursorTime = nonSmoothCursorThisFrame;

			const Beat cursorBeatStart = context.TimeToBeat(nonSmoothCursorThisFrame);
			const Beat cursorBeatEnd = cursorBeatStart + Beat::FromBars(1);
			const Time cursorTimeOnPlaybackStart = context.CursorTimeOnPlaybackStart;

			context.ChartSelectedCourse->TempoMap.ForEachBeatBar([&](const SortedTempoMap::ForEachBeatBarData& it)
			{
				if (it.Beat >= cursorBeatEnd)
					return ControlFlow::Break;

				const Time beatTime = context.BeatToTime(it.Beat);
				const Time offsetBeatTime = (beatTime - futureOffset);

				// BUG: This is definitely too lenient and easily falsely triggers 1/16th after a bar start
				// HACK: Imperfect solution for making up for the future offset skipping past the first beat(s) if they are too close to the playback start time
				if (!metronome.HasOnPlaybackStartTimeBeenPlayed && ApproxmiatelySame(beatTime.Seconds, cursorTimeOnPlaybackStart.Seconds, 0.01))
				{
					metronome.HasOnPlaybackStartTimeBeenPlayed = true;
					metronome.LastPlayedBeatTime = beatTime;
					context.SfxVoicePool.PlaySound(it.IsBar ? SoundEffectType::MetronomeBar : SoundEffectType::MetronomeBeat);
					return ControlFlow::Break;
				}

				if (offsetBeatTime >= nonSmoothCursorLastFrame && offsetBeatTime < nonSmoothCursorThisFrame)
				{
					if (metronome.LastPlayedBeatTime != beatTime)
					{
						metronome.LastPlayedBeatTime = beatTime;
						const Time startTime = Min((nonSmoothCursorThisFrame - beatTime), Time::Zero());
						context.SfxVoicePool.PlaySound(it.IsBar ? SoundEffectType::MetronomeBar : SoundEffectType::MetronomeBeat, startTime);
					}
					return ControlFlow::Break;
				}

				return ControlFlow::Continue;
			});
		}
	}

	void ChartTimeline::DrawGui(ChartContext& context)
	{
		UpdateInputAtStartOfFrame(context);
		UpdateAllAnimationsAfterUserInput(context);

#if PEEPO_DEBUG // DEBUG: Submit empty window first for more natural tab order sorting
		if (Gui::Begin(UI_WindowName("Chart Timeline - Debug"))) { DrawTimelineDebugWindowContent(*this, context); } Gui::End();
#endif

		const auto& style = Gui::GetStyle();

		// NOTE: Big enough to fit both the bar-index, bar-time and some padding
		static constexpr f32 contentAndSidebarHeaderHeight = 34.0f;
		const f32 sidebarWidth = GuiScale(CurrentSidebarWidth);
		const f32 sidebarHeight = GuiScale(contentAndSidebarHeaderHeight);

		const f32 scrollbarHeight = Gui::GetStyle().ScrollbarSize * 3.0f;
		Regions.Window = Rect::FromTLSize(Gui::GetCursorScreenPos(), Gui::GetContentRegionAvail());
		Regions.SidebarHeader = Rect::FromTLSize(Regions.Window.TL, vec2(sidebarWidth, sidebarHeight));
		Regions.Sidebar = Rect::FromTLSize(Regions.SidebarHeader.GetBL(), vec2(sidebarWidth, Regions.Window.GetHeight() - sidebarHeight));
		Regions.ContentHeader = Rect(Regions.SidebarHeader.GetTR(), vec2(Regions.Window.BR.x, Regions.SidebarHeader.BR.y));
		Regions.Content = Rect::FromTLSize(Regions.ContentHeader.GetBL(), Max(vec2(0.0f), vec2(Regions.Window.GetWidth() - sidebarWidth, Regions.Window.GetHeight() - sidebarHeight - scrollbarHeight)));
		Regions.ContentScrollbarX = Rect::FromTLSize(Regions.Content.GetBL(), vec2(Regions.Content.GetWidth(), scrollbarHeight));

		auto timelineRegionBegin = [](const Rect region, cstr name, b8 padding = false)
		{
			if (!padding) Gui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });

			Gui::SetCursorScreenPos(region.GetTL());
			Gui::BeginChild(name, region.GetSize(), true, ImGuiWindowFlags_NoScrollbar);

			if (!padding) Gui::PopStyleVar(1);
		};

		auto timelineRegionEnd = []()
		{
			Gui::EndChild();
		};

		IsAnyChildWindowFocused = Gui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);

		timelineRegionBegin(Regions.Window, "TimelineWindow");
		{
			timelineRegionBegin(Regions.SidebarHeader, "TimelineSideBarHeader");
			{
				DrawListSidebarHeader = Gui::GetWindowDrawList();
				char buffer[64];

				// TODO: ...
				static constexpr f32 buttonAlpha = 0.35f;
				Gui::PushFont(FontMedium_EN);
				Gui::PushStyleColor(ImGuiCol_Button, Gui::GetColorU32(ImGuiCol_Button, buttonAlpha));
				Gui::PushStyleColor(ImGuiCol_ButtonHovered, Gui::GetColorU32(ImGuiCol_ButtonHovered, buttonAlpha));
				Gui::PushStyleColor(ImGuiCol_ButtonActive, Gui::GetColorU32(ImGuiCol_ButtonActive, buttonAlpha));
				{
					Gui::BeginDisabled();
					const vec2 perButtonSize = vec2(Gui::GetContentRegionAvail()) * vec2(1.0f / 3.0f, 1.0f);
					{
						Time displayTime = {};
						if (*Settings.General.DisplayTimeInSongSpace)
							displayTime = ClampBot(context.GetCursorTime() - context.Chart.SongOffset, Min(-context.Chart.SongOffset, Time::Zero()));
						else
							displayTime = ClampBot(context.GetCursorTime(), Time::Zero());

						Gui::Button(displayTime.ToString().Data, perButtonSize);
					}
					Gui::SameLine(0.0f, 0.0f);
					{
						sprintf_s(buffer, "%g%%", ToPercent(context.GetPlaybackSpeed()));
						Gui::Button(buffer, perButtonSize);
					}
					Gui::SameLine(0.0f, 0.0f);
					{
						sprintf_s(buffer, "1 / %d", CurrentGridBarDivision);
						Gui::Button(buffer, perButtonSize);
					}
					Gui::EndDisabled();
				}
				Gui::PopStyleColor(3);
				Gui::PopFont();
			}
			timelineRegionEnd();

			const ImGuiHoveredFlags hoveredFlags = BoxSelection.IsActive ? ImGuiHoveredFlags_AllowWhenBlockedByActiveItem : ImGuiHoveredFlags_None;

			timelineRegionBegin(Regions.Sidebar, "TimelineSideBar");
			{
				DrawListSidebar = Gui::GetWindowDrawList();
				IsSidebarWindowHovered = Gui::IsWindowHovered(hoveredFlags);
			}
			timelineRegionEnd();

			timelineRegionBegin(Regions.ContentHeader, "TimelineContentHeader");
			{
				DrawListContentHeader = Gui::GetWindowDrawList();
				IsContentHeaderWindowHovered = Gui::IsWindowHovered(hoveredFlags);
			}
			timelineRegionEnd();

			timelineRegionBegin(Regions.Content, "TimelineContent");
			{
				if (Gui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && !Gui::IsWindowFocused())
				{
					if (Gui::IsMouseClicked(ImGuiMouseButton_Middle) || Gui::IsMouseClicked(ImGuiMouseButton_Right))
						Gui::SetWindowFocus();
				}

				DrawListContent = Gui::GetWindowDrawList();
				IsContentWindowHovered = Gui::IsWindowHovered(hoveredFlags);
				IsContentWindowFocused = Gui::IsWindowFocused();
			}
			timelineRegionEnd();

			timelineRegionBegin(Regions.ContentScrollbarX, "TimelineContentScrollbarX");
			{
				Gui::PushStyleColor(ImGuiCol_ScrollbarGrab, Gui::GetColorU32(ImGuiCol_ScrollbarGrab, 0.2f));
				Gui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, Gui::GetColorU32(ImGuiCol_ScrollbarGrabHovered, 0.2f));
				Gui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, Gui::GetColorU32(ImGuiCol_ScrollbarGrabActive, 0.2f));

				// NOTE: Waveform and cursor on top of scrollbar!
				{
					const Time cursorTime = context.GetCursorTime();
					const Time chartDuration = context.Chart.GetDurationOrDefault();
					const b8 isPlayback = context.GetIsPlayback();

					if (!context.SongWaveformL.IsEmpty())
						DrawTimelineScrollbarXWaveform(*this, Gui::GetWindowDrawList(), context.Chart.SongOffset, chartDuration, context.SongWaveformL, context.SongWaveformR, context.SongWaveformFadeAnimationCurrent);

					DrawTimelineScrollbarXMinimap(*this, Gui::GetWindowDrawList(), *context.ChartSelectedCourse, context.ChartSelectedBranch, chartDuration);

					const f32 animatedCursorLocalSpaceX = TimeToScrollbarLocalSpaceXClamped(Camera.WorldSpaceXToTime(WorldSpaceCursorXAnimationCurrent), Regions, chartDuration);
					const f32 currentCursorLocalSpaceX = TimeToScrollbarLocalSpaceXClamped(cursorTime, Regions, chartDuration);
					const f32 cursorLocalSpaceX = isPlayback ? currentCursorLocalSpaceX : animatedCursorLocalSpaceX;

					// BUG: Drawn cursor doesn't perfectly line up on the left side with the scroll grab hand (?)
					Gui::GetWindowDrawList()->AddLine(
						LocalToScreenSpace_ScrollbarX(vec2(cursorLocalSpaceX, 2.0f)),
						LocalToScreenSpace_ScrollbarX(vec2(cursorLocalSpaceX, Regions.ContentScrollbarX.GetHeight() - 2.0f)),
						TimelineCursorColor);
				}

				// NOTE: Nice extra bit of user feedback
				if (IsCameraMouseGrabActive) Gui::PushStyleColor(ImGuiCol_ScrollbarGrab, Gui::GetStyleColorVec4(ImGuiCol_ScrollbarGrabHovered));

				const f32 localSpaceVisibleWidth = Regions.Content.GetWidth();
				const f32 localSpaceTimelineWidth = Camera.WorldToLocalSpaceScale(vec2(Camera.TimeToWorldSpaceX(context.Chart.GetDurationOrDefault()), 0.0f)).x + 2.0f;

				// BUG: Scrollbar should still be interactable while box selecting
				static constexpr ImS64 padding = 1;
				ImS64 inOutScrollValue = static_cast<ImS64>(Camera.PositionCurrent.x - TimelineCameraBaseScrollX);
				const ImS64 inSizeAvail = static_cast<ImS64>(localSpaceVisibleWidth + TimelineCameraBaseScrollX);
				const ImS64 inContentSize = static_cast<ImS64>(localSpaceTimelineWidth);
				if (Gui::ScrollbarEx(ImRect(Regions.ContentScrollbarX.TL, Regions.ContentScrollbarX.BR), Gui::GetID("ContentScrollbarX"), ImGuiAxis_X,
					&inOutScrollValue, inSizeAvail, inContentSize, ImDrawFlags_RoundCornersNone))
				{
					// BUG: Only setting PositionTarget results in glitchy behavior when clicking somewhere on the scrollbar for shorter than the animation duration
					//		however setting both PositionCurrent and PositionTarget means no smooth scrolling while dragging around
					// Camera.PositionTarget.x = static_cast<f32>(inOutScrollValue);
					Camera.PositionCurrent.x = Camera.PositionTarget.x = static_cast<f32>(inOutScrollValue) + TimelineCameraBaseScrollX;
				}

				if (IsCameraMouseGrabActive) Gui::PopStyleColor();
				Gui::PopStyleColor(3);
			}
			timelineRegionEnd();

			DrawAllAtEndOfFrame(context);
			DrawListSidebarHeader = DrawListSidebar = DrawListContentHeader = DrawListContent = nullptr;
		}
		timelineRegionEnd();
	}

	void ChartTimeline::StartEndRangeSelectionAtCursor(ChartContext& context)
	{
		if (!RangeSelection.IsActive || RangeSelection.HasEnd)
		{
			RangeSelection.Start = RangeSelection.End = RoundBeatToCurrentGrid(context.GetCursorBeat());
			RangeSelection.HasEnd = false;
			RangeSelection.IsActive = true;
			RangeSelectionExpansionAnimationTarget = 0.0f;
		}
		else
		{
			RangeSelection.End = RoundBeatToCurrentGrid(context.GetCursorBeat());
			RangeSelection.HasEnd = true;
			RangeSelectionExpansionAnimationTarget = 1.0f;
			if (RangeSelection.End == RangeSelection.Start)
				RangeSelection = {};
		}
	}

	void ChartTimeline::PlayNoteSoundAndHitAnimationsAtBeat(ChartContext& context, Beat cursorBeat)
	{
		b8 soundHasBeenPlayed = false;
		for (Note& note : context.ChartSelectedCourse->GetNotes(context.ChartSelectedBranch))
		{
			if (note.BeatTime == cursorBeat)
			{
				note.ClickAnimationTimeRemaining = note.ClickAnimationTimeDuration = NoteHitAnimationDuration;
				if (!soundHasBeenPlayed) { context.SfxVoicePool.PlaySound(SoundEffectTypeForNoteType(note.Type)); soundHasBeenPlayed = true; }
			}
		}
	}

	void ChartTimeline::ExecuteClipboardAction(ChartContext& context, ClipboardAction action)
	{
		static constexpr cstr clipboardTextHeader = "// PeepoDrumKit Clipboard";
		static constexpr auto itemsToClipboardText = [](const std::vector<GenericListStructWithType>& inItems, Beat baseBeat) -> std::string
		{
			std::string out; out.reserve(512); out += clipboardTextHeader; out += '\n';
			for (const auto& item : inItems)
			{
				char buffer[256]; i32 bufferLength = 0;
				switch (item.List)
				{
				case GenericList::TempoChanges:
				{
					const auto& in = item.Value.POD.Tempo;
					bufferLength = sprintf_s(buffer, "Tempo { %d, %g };\n", (in.Beat - baseBeat).Ticks, in.Tempo.BPM);
				} break;
				case GenericList::SignatureChanges:
				{
					const auto& in = item.Value.POD.Signature;
					bufferLength = sprintf_s(buffer, "TimeSignature { %d, %d, %d };\n", (in.Beat - baseBeat).Ticks, in.Signature.Numerator, in.Signature.Denominator);
				} break;
				case GenericList::Notes_Normal:
				case GenericList::Notes_Expert:
				case GenericList::Notes_Master:
				{
					const auto& in = item.Value.POD.Note;
					bufferLength = sprintf_s(buffer, "Note { %d, %d, %d, %d, %g };\n", (in.BeatTime - baseBeat).Ticks, in.BeatDuration.Ticks, static_cast<i32>(in.Type), in.BalloonPopCount, in.TimeOffset.ToMS());
				} break;
				case GenericList::ScrollChanges:
				{
					const auto& in = item.Value.POD.Scroll;
					bufferLength = sprintf_s(buffer, "ScrollSpeed { %d, %g };\n", (in.BeatTime - baseBeat).Ticks, in.ScrollSpeed);
				} break;
				case GenericList::BarLineChanges:
				{
					const auto& in = item.Value.POD.BarLine;
					bufferLength = sprintf_s(buffer, "BarLine { %d, %d };\n", (in.BeatTime - baseBeat).Ticks, in.IsVisible ? 1 : 0);
				} break;
				case GenericList::GoGoRanges:
				{
					const auto& in = item.Value.POD.GoGo;
					bufferLength = sprintf_s(buffer, "GoGo { %d, %d };\n", (in.BeatTime - baseBeat).Ticks, in.BeatDuration.Ticks);
				} break;
				case GenericList::Lyrics:
				{
					// TODO: Properly handle escape characters (?)
					const auto& in = item.Value.NonTrivial.Lyric;
					out += "Lyric { ";
					out += std::string_view(buffer, sprintf_s(buffer, "%d, ", (in.BeatTime - baseBeat).Ticks));
					out += in.Lyric;
					out += " };\n";
				} break;
				default: { assert(false); } break;
				}

				if (bufferLength > 0)
					out += std::string_view(buffer, bufferLength);
			}

			if (!out.empty() && out.back() == '\n')
				out.erase(out.end() - 1);

			return out;
		};
		static constexpr auto itemsFromClipboatdText = [](std::string_view clipboardText) -> std::vector<GenericListStructWithType>
		{
			std::vector<GenericListStructWithType> out;
			if (!ASCII::StartsWith(clipboardText, clipboardTextHeader))
				return out;

			// TODO: Split and parse by ';' instead of '\n' (?)
			ASCII::ForEachLineInMultiLineString(clipboardText, false, [&](std::string_view line)
			{
				if (line.empty() || ASCII::StartsWith(line, "//"))
					return;

				line = ASCII::TrimSuffix(ASCII::TrimSuffix(line, "\r"), "\n");
				const size_t openIndex = line.find_first_of('{');
				const size_t closeIndex = line.find_last_of('}');
				if (openIndex == std::string_view::npos || closeIndex == std::string_view::npos || closeIndex <= openIndex)
					return;

				const std::string_view itemType = ASCII::Trim(line.substr(0, openIndex));
				const std::string_view itemParam = ASCII::Trim(line.substr(openIndex + sizeof('{'), (closeIndex - openIndex) - sizeof('}')));

				if (itemType == "Lyric")
				{
					auto& newItem = out.emplace_back(); newItem.List = GenericList::Lyrics;
					auto& newItemValue = newItem.Value.NonTrivial.Lyric;

					const size_t commaIndex = itemParam.find_first_of(',');
					if (commaIndex != std::string_view::npos)
					{
						const std::string_view beatSubStr = ASCII::Trim(itemParam.substr(0, commaIndex));
						const std::string_view lyricSubStr = ASCII::Trim(itemParam.substr(commaIndex + sizeof(',')));

						ASCII::TryParseI32(beatSubStr, newItemValue.BeatTime.Ticks);
						newItemValue.Lyric = lyricSubStr;
					}
				}
				else
				{
					struct { i32 I32; f32 F32; b8 IsValidI32, IsValidF32; } parsedParams[6] = {};
					ASCII::ForEachInCommaSeparatedList(itemParam, [&, paramIndex = 0](std::string_view v) mutable
					{
						if (paramIndex < ArrayCount(parsedParams))
						{
							if (v = ASCII::Trim(v); !v.empty())
							{
								parsedParams[paramIndex].IsValidI32 = ASCII::TryParseI32(v, parsedParams[paramIndex].I32);
								parsedParams[paramIndex].IsValidF32 = ASCII::TryParseF32(v, parsedParams[paramIndex].F32);
							}
						}
						paramIndex++;
					});

					if (itemType == "Tempo")
					{
						auto& newItem = out.emplace_back(); newItem.List = GenericList::TempoChanges;
						auto& newItemValue = newItem.Value.POD.Tempo;
						newItemValue = TempoChange {};
						newItemValue.Beat.Ticks = parsedParams[0].I32;
						newItemValue.Tempo.BPM = parsedParams[1].F32;
					}
					else if (itemType == "TimeSignature")
					{
						auto& newItem = out.emplace_back(); newItem.List = GenericList::SignatureChanges;
						auto& newItemValue = newItem.Value.POD.Signature;
						newItemValue = TimeSignatureChange {};
						newItemValue.Beat.Ticks = parsedParams[0].I32;
						newItemValue.Signature.Numerator = parsedParams[1].I32;
						newItemValue.Signature.Denominator = parsedParams[2].I32;
					}
					else if (itemType == "Note")
					{
						auto& newItem = out.emplace_back(); newItem.List = GenericList::Notes_Normal;
						auto& newItemValue = newItem.Value.POD.Note;
						newItemValue = Note {};
						newItemValue.BeatTime.Ticks = parsedParams[0].I32;
						newItemValue.BeatDuration.Ticks = parsedParams[1].I32;
						newItemValue.Type = static_cast<NoteType>(parsedParams[2].I32);
						newItemValue.BalloonPopCount = static_cast<i16>(parsedParams[3].I32);
						newItemValue.TimeOffset = Time::FromMS(parsedParams[4].F32);
					}
					else if (itemType == "ScrollSpeed")
					{
						auto& newItem = out.emplace_back(); newItem.List = GenericList::ScrollChanges;
						auto& newItemValue = newItem.Value.POD.Scroll;
						newItemValue = ScrollChange {};
						newItemValue.BeatTime.Ticks = parsedParams[0].I32;
						newItemValue.ScrollSpeed = parsedParams[1].F32;
					}
					else if (itemType == "BarLine")
					{
						auto& newItem = out.emplace_back(); newItem.List = GenericList::BarLineChanges;
						auto& newItemValue = newItem.Value.POD.BarLine;
						newItemValue = BarLineChange {};
						newItemValue.BeatTime.Ticks = parsedParams[0].I32;
						newItemValue.IsVisible = (parsedParams[1].I32 != 0);
					}
					else if (itemType == "GoGo")
					{
						auto& newItem = out.emplace_back(); newItem.List = GenericList::GoGoRanges;
						auto& newItemValue = newItem.Value.POD.GoGo;
						newItemValue = GoGoRange {};
						newItemValue.BeatTime.Ticks = parsedParams[0].I32;
						newItemValue.BeatDuration.Ticks = parsedParams[1].I32;
					}
#if PEEPO_DEBUG
					else { assert(false); }
#endif
				}
			});
			return out;
		};
		static constexpr auto copyAllSelectedItems = [](const ChartCourse& course) -> std::vector<GenericListStructWithType>
		{
			std::vector<GenericListStructWithType> out;
			size_t selectionCount = 0; ForEachSelectedChartItem(course, [&](const ForEachChartItemData& it) { selectionCount++; });
			if (selectionCount > 0)
			{
				out.reserve(selectionCount);
				ForEachSelectedChartItem(course, [&](const ForEachChartItemData& it)
				{
					auto& itemValue = out.emplace_back();
					itemValue.List = it.List;
					TryGetGenericStruct(course, it.List, it.Index, itemValue.Value);
				});
			}
			return out;
		};
		static constexpr auto findBaseBeat = [](const std::vector<GenericListStructWithType>& items)
		{
			Beat minBase = Beat::FromTicks(I32Max);
			for (const auto& item : items)
				minBase = Min(item.GetBeat(), minBase);
			return (minBase.Ticks != I32Max) ? minBase : Beat::Zero();
		};

		ChartCourse& course = *context.ChartSelectedCourse;
		switch (action)
		{
		case ClipboardAction::Cut:
		{
			if (auto selectedItems = copyAllSelectedItems(course); !selectedItems.empty())
			{
				size_t selectedNoteIndex = 0;
				for (const auto& item : selectedItems)
				{
					if (IsNotesList(item.List))
					{
						TempDeletedNoteAnimationsBuffer.push_back(
							DeletedNoteAnimation { item.Value.POD.Note, context.ChartSelectedBranch, ConvertRange(0.0f, static_cast<f32>(selectedItems.size()), 0.0f, -0.08f, static_cast<f32>(selectedNoteIndex)) });
						selectedNoteIndex++;
					}
				}

				Gui::SetClipboardText(itemsToClipboardText(selectedItems, findBaseBeat(selectedItems)).c_str());
				context.Undo.Execute<Commands::RemoveMultipleGenericItems_Cut>(&course, std::move(selectedItems));
			}
		} break;
		case ClipboardAction::Copy:
		{
			if (auto selectedItems = copyAllSelectedItems(course); !selectedItems.empty())
			{
				// TODO: Maybe also animate original notes being copied (?)
				Gui::SetClipboardText(itemsToClipboardText(selectedItems, findBaseBeat(selectedItems)).c_str());
			}
		} break;
		case ClipboardAction::Paste:
		{
			std::vector<GenericListStructWithType> clipboardItems = itemsFromClipboatdText(Gui::GetClipboardTextView());
			if (!clipboardItems.empty())
			{
				const Beat baseBeat = FloorBeatToCurrentGrid(context.GetCursorBeat()) - findBaseBeat(clipboardItems);
				for (auto& item : clipboardItems) { item.SetBeat(item.GetBeat() + baseBeat); }

				auto itemAlreadyExistsOrIsBad = [&](const GenericListStructWithType& item)
				{
					const b8 inclusiveBeatCheck = ListUsesInclusiveBeatCheck(item.List);
					auto check = [&](auto& list, auto& i) { return (GetBeat(i) < Beat::Zero()) || (list.TryFindOverlappingBeat(GetBeat(i), GetBeat(i) + GetBeatDuration(i), inclusiveBeatCheck) != nullptr); };
					switch (item.List)
					{
					case GenericList::TempoChanges: return check(course.TempoMap.Tempo, item.Value.POD.Tempo);
					case GenericList::SignatureChanges: return check(course.TempoMap.Signature, item.Value.POD.Signature);
					case GenericList::Notes_Normal: return check(course.Notes_Normal, item.Value.POD.Note);
					case GenericList::Notes_Expert: return check(course.Notes_Expert, item.Value.POD.Note);
					case GenericList::Notes_Master: return check(course.Notes_Master, item.Value.POD.Note);
					case GenericList::ScrollChanges: return check(course.ScrollChanges, item.Value.POD.Scroll);
					case GenericList::BarLineChanges: return check(course.BarLineChanges, item.Value.POD.BarLine);
					case GenericList::GoGoRanges: return check(course.GoGoRanges, item.Value.POD.GoGo);
					case GenericList::Lyrics: return check(course.Lyrics, item.Value.NonTrivial.Lyric);
					default: assert(false); return false;
					}
				};
				erase_remove_if(clipboardItems, itemAlreadyExistsOrIsBad);

				if (!clipboardItems.empty())
				{
					SetNotesWaveAnimationTimes(clipboardItems, +1);

					b8 isFirstNote = true;
					for (const auto& item : clipboardItems)
						if (isFirstNote && IsNotesList(item.List)) { context.SfxVoicePool.PlaySound(SoundEffectTypeForNoteType(item.Value.POD.Note.Type)); isFirstNote = false; }

					context.Undo.Execute<Commands::AddMultipleGenericItems_Paste>(&course, std::move(clipboardItems));
				}
			}
		} break;
		case ClipboardAction::Delete:
		{
			if (auto selectedItems = copyAllSelectedItems(course); !selectedItems.empty())
			{
				for (const auto& item : selectedItems)
				{
					if (IsNotesList(item.List))
						TempDeletedNoteAnimationsBuffer.push_back(DeletedNoteAnimation { item.Value.POD.Note, context.ChartSelectedBranch, 0.0f });
				}

				context.Undo.Execute<Commands::RemoveMultipleGenericItems>(&course, std::move(selectedItems));
			}
		} break;
		default: { assert(false); } break;
		}
	}

	void ChartTimeline::ExecuteSelectionAction(ChartContext& context, SelectionAction action, const SelectionActionParam& param)
	{
		ChartCourse& course = *context.ChartSelectedCourse;
		switch (action)
		{
		default: { assert(false); } break;
		case SelectionAction::SelectAll: { ForEachChartItem(course, [&](const ForEachChartItemData& it) { it.SetIsSelected(course, true); }); } break;
		case SelectionAction::UnselectAll: { ForEachChartItem(course, [&](const ForEachChartItemData& it) { it.SetIsSelected(course, false); }); } break;
		case SelectionAction::InvertAll: { ForEachChartItem(course, [&](const ForEachChartItemData& it) { it.SetIsSelected(course, !it.GetIsSelected(course)); }); } break;
		case SelectionAction::SelectAllWithinRangeSelection:
		{
			if (RangeSelection.IsActiveAndHasEnd())
			{
				const Beat rangeSelectionMin = RoundBeatToCurrentGrid(RangeSelection.GetMin());
				const Beat rangeSelectionMax = RoundBeatToCurrentGrid(RangeSelection.GetMax());
				ForEachChartItem(course, [&](const ForEachChartItemData& it)
				{
					const Beat itStart = it.GetBeat(course);
					const Beat itEnd = itStart + it.GetBeatDuration(course);
					if ((itStart <= rangeSelectionMax) && (itEnd >= rangeSelectionMin))
						it.SetIsSelected(course, true);
				});
			}
		} break;
		case SelectionAction::PerRowShiftSelected:
		{
			struct ItemProxy
			{
				ChartCourse& Course; GenericList List; i32 Index;
				inline b8 Exists() const { return (Index >= 0); }
				inline b8 IsSelected() const { GenericMemberUnion v {}; TryGetGeneric(Course, List, Index, GenericMember::B8_IsSelected, v); return v.B8; }
				inline void IsSelected(b8 isSelected) { GenericMemberUnion v {}; v.B8 = isSelected; TrySetGeneric(Course, List, Index, GenericMember::B8_IsSelected, v); }
				inline static ItemProxy At(ChartCourse& course, GenericList list, i32 listCount, i32 i) { return ItemProxy { course, list, (i >= 0) && (i < listCount) ? i : -1 }; }
			};

			for (GenericList list = {}; list < GenericList::Count; IncrementEnum(list))
			{
				const i32 listCount = static_cast<i32>(GetGenericListCount(course, list));
				if (param.ShiftDelta > 0)
				{
					for (i32 i = listCount - 1; i >= 0; i--)
					{
						ItemProxy thisIt = ItemProxy::At(course, list, listCount, i);
						ItemProxy nextIt = ItemProxy::At(course, list, listCount, i + 1);
						if (nextIt.Exists() && thisIt.IsSelected())
							nextIt.IsSelected(true);
						thisIt.IsSelected(false);
					}
				}
				else if (param.ShiftDelta < 0)
				{
					for (i32 i = 0; i < listCount; i++)
					{
						ItemProxy thisIt = ItemProxy::At(course, list, listCount, i);
						ItemProxy prevIt = ItemProxy::At(course, list, listCount, i - 1);
						if (prevIt.Exists() && thisIt.IsSelected())
							prevIt.IsSelected(true);
						thisIt.IsSelected(false);
					}
				}
			}
		} break;
		case SelectionAction::PerRowSelectPattern:
		{
			if (param.Pattern == nullptr || param.Pattern[0] == '\0')
				break;

			const std::string_view pattern = param.Pattern;
			for (GenericList list = {}; list < GenericList::Count; IncrementEnum(list))
			{
				for (size_t i = 0, patternIndex = 0; i < GetGenericListCount(course, list); i++)
				{
					if (const ForEachChartItemData it = { list, i }; it.GetIsSelected(course))
					{
						if (pattern[patternIndex] != 'x')
							it.SetIsSelected(course, false);
						if (++patternIndex >= pattern.size())
							patternIndex = 0;
					}
				}
			}
		} break;
		}
	}

	void ChartTimeline::ExecuteTransformAction(ChartContext& context, TransformAction action, const TransformActionParam& param)
	{
		ChartCourse& course = *context.ChartSelectedCourse;
		switch (action)
		{
		default: { assert(false); } break;
		case TransformAction::FlipNoteType:
		{
			for (BranchType branch = {}; branch < BranchType::Count; IncrementEnum(branch))
			{
				size_t selectedNoteCount = 0;
				SortedNotesList& notes = context.ChartSelectedCourse->GetNotes(branch);
				for (const Note& note : notes) { if (note.IsSelected && IsNoteFlippable(note.Type)) selectedNoteCount++; }
				if (selectedNoteCount <= 0)
					continue;

				std::vector<Commands::ChangeMultipleNoteTypes::Data> noteTypesToChange;
				noteTypesToChange.reserve(selectedNoteCount);

				for (Note& note : notes)
				{
					if (note.IsSelected && IsNoteFlippable(note.Type))
					{
						auto& data = noteTypesToChange.emplace_back();
						data.Index = ArrayItToIndex(&note, &notes[0]);
						data.NewType = FlipNote(note.Type);
						note.ClickAnimationTimeRemaining = note.ClickAnimationTimeDuration = NoteHitAnimationDuration;
					}
				}

				context.SfxVoicePool.PlaySound(SoundEffectTypeForNoteType(noteTypesToChange[0].NewType));
				context.Undo.Execute<Commands::ChangeMultipleNoteTypes_FlipTypes>(&notes, std::move(noteTypesToChange));
				context.Undo.DisallowMergeForLastCommand();
			}
		} break;
		case TransformAction::ToggleNoteSize:
		{
			for (BranchType branch = {}; branch < BranchType::Count; IncrementEnum(branch))
			{
				size_t selectedNoteCount = 0;
				SortedNotesList& notes = context.ChartSelectedCourse->GetNotes(branch);
				for (const Note& note : notes) { if (note.IsSelected) selectedNoteCount++; }
				if (selectedNoteCount <= 0)
					continue;

				std::vector<Commands::ChangeMultipleNoteTypes::Data> noteTypesToChange;
				noteTypesToChange.reserve(selectedNoteCount);

				for (Note& note : notes)
				{
					if (note.IsSelected)
					{
						auto& data = noteTypesToChange.emplace_back();
						data.Index = ArrayItToIndex(&note, &notes[0]);
						data.NewType = ToggleNoteSize(note.Type);
						note.ClickAnimationTimeRemaining = note.ClickAnimationTimeDuration = NoteHitAnimationDuration;
					}
				}

				context.SfxVoicePool.PlaySound(SoundEffectTypeForNoteType(noteTypesToChange[0].NewType));
				context.Undo.Execute<Commands::ChangeMultipleNoteTypes_ToggleSizes>(&notes, std::move(noteTypesToChange));
				context.Undo.DisallowMergeForLastCommand();
			}
		} break;
		case TransformAction::ScaleItemTime:
		{
			assert(param.TimeRatio[0] > 0 && param.TimeRatio[1] > 0 && param.TimeRatio[0] != param.TimeRatio[1]);
			size_t selectedItemCount = 0; ForEachSelectedChartItem(course, [&](const ForEachChartItemData& it) { selectedItemCount++; });
			if (selectedItemCount <= 0)
				return;

			b8 isFirst = true; Beat firstBeat = {};
			std::vector<GenericListStructWithType> itemsToRemove; itemsToRemove.reserve(selectedItemCount);
			std::vector<GenericListStructWithType> itemsToAdd; itemsToAdd.reserve(selectedItemCount);
			ForEachSelectedChartItem(course, [&](const ForEachChartItemData& it)
			{
				if (isFirst) { firstBeat = it.GetBeat(course); isFirst = false; }

				auto& itemToRemove = itemsToRemove.emplace_back();
				itemToRemove.List = it.List;
				TryGetGenericStruct(course, it.List, it.Index, itemToRemove.Value);

				auto& itemToAdd = itemsToAdd.emplace_back(itemToRemove);
				itemToAdd.SetBeat((((itemToAdd.GetBeat() - firstBeat) / param.TimeRatio[1]) * param.TimeRatio[0]) + firstBeat);

				if (IsNotesList(itemToAdd.List))
					itemToAdd.Value.POD.Note.ClickAnimationTimeRemaining = itemToAdd.Value.POD.Note.ClickAnimationTimeDuration = NoteHitAnimationDuration;
			});

			// BUG: Resolve item duration intersections (only *add* notes if they don't interect another non-selected long item (?))
			// BUG: Overwritten items not correctly restored on undo (?)
			if (!itemsToRemove.empty() || !itemsToAdd.empty())
			{
				for (auto& it : itemsToAdd) if (IsNotesList(it.List)) { context.SfxVoicePool.PlaySound(SoundEffectTypeForNoteType(it.Value.POD.Note.Type)); break; }

				if (param.TimeRatio[0] < param.TimeRatio[1])
					context.Undo.Execute<Commands::RemoveThenAddMultipleGenericItems_CompressItems>(&course, std::move(itemsToRemove), std::move(itemsToAdd));
				else
					context.Undo.Execute<Commands::RemoveThenAddMultipleGenericItems_ExpandItems>(&course, std::move(itemsToRemove), std::move(itemsToAdd));
				context.Undo.DisallowMergeForLastCommand();
			}
		} break;
		}
	}

	void ChartTimeline::ExecuteConvertSelectionToScrollChanges(ChartContext& context)
	{
		ChartCourse& course = *context.ChartSelectedCourse;

		size_t nonScrollChangeSelectedItemCount = 0;
		ForEachSelectedChartItem(course, [&](const ForEachChartItemData& it) { nonScrollChangeSelectedItemCount += (it.List != GenericList::ScrollChanges); });
		if (nonScrollChangeSelectedItemCount <= 0)
			return;

		std::vector<ScrollChange*> scrollChangesThatAlreadyExist; scrollChangesThatAlreadyExist.reserve(nonScrollChangeSelectedItemCount);
		std::vector<ScrollChange> scrollChangesToAdd; scrollChangesToAdd.reserve(nonScrollChangeSelectedItemCount);
		ForEachSelectedChartItem(course, [&](const ForEachChartItemData& it)
		{
			if (it.List != GenericList::ScrollChanges)
			{
				const Beat itBeat = it.GetBeat(course);
				if (ScrollChange* lastScrollChange = course.ScrollChanges.TryFindLastAtBeat(itBeat); lastScrollChange != nullptr && lastScrollChange->BeatTime == itBeat)
					scrollChangesThatAlreadyExist.push_back(lastScrollChange);
				else
					scrollChangesToAdd.push_back(ScrollChange { itBeat, (lastScrollChange != nullptr) ? lastScrollChange->ScrollSpeed : 1.0f });
			}
		});

		if (!scrollChangesThatAlreadyExist.empty() || !scrollChangesToAdd.empty())
		{
			if (*Settings.General.ConvertSelectionToScrollChanges_UnselectOld)
				ForEachSelectedChartItem(course, [&](const ForEachChartItemData& it) { it.SetIsSelected(course, false); });

			if (*Settings.General.ConvertSelectionToScrollChanges_SelectNew)
			{
				for (auto* it : scrollChangesThatAlreadyExist) { it->IsSelected = true; }
				for (auto& it : scrollChangesToAdd) { it.IsSelected = true; }
			}

			if (!scrollChangesToAdd.empty())
				context.Undo.Execute<Commands::AddMultipleScrollChanges>(&course.ScrollChanges, std::move(scrollChangesToAdd));
		}
	}

	void ChartTimeline::UpdateInputAtStartOfFrame(ChartContext& context)
	{
		MousePosLastFrame = MousePosThisFrame;
		MousePosThisFrame = Gui::GetMousePos();

		// NOTE: Mouse scroll / zoom
		if (!ApproxmiatelySame(Gui::GetIO().MouseWheel, 0.0f))
		{
			vec2 scrollStep = vec2(Gui::GetIO().KeyShift ? *Settings.General.TimelineScrollDistancePerMouseWheelTickFast : *Settings.General.TimelineScrollDistancePerMouseWheelTick);
			scrollStep.y *= 0.75f;
			if (*Settings.General.TimelineScrollInvertMouseWheel)
				scrollStep.x *= -1.0f;

			if (IsSidebarWindowHovered)
			{
#if 0 // NOTE: Not needed at the moment for small number of rows
				if (!Gui::GetIO().KeyAlt)
					Camera.PositionTarget.y -= (Gui::GetIO().MouseWheel * scrollStep.y);
#endif
			}
			else if (IsContentHeaderWindowHovered || IsContentWindowHovered)
			{
				if (Gui::GetIO().KeyAlt)
				{
					const f32 zoomFactorX = *Settings.General.TimelineZoomFactorPerMouseWheelTick;
					const f32 newZoomX = (Gui::GetIO().MouseWheel > 0.0f) ? (Camera.ZoomTarget.x * zoomFactorX) : (Camera.ZoomTarget.x / zoomFactorX);
					Camera.SetZoomTargetAroundLocalPivot(vec2(newZoomX, Camera.ZoomTarget.y), ScreenToLocalSpace(Gui::GetMousePos()));
				}
				else
				{
					if (Gui::GetIO().KeyCtrl)
						Camera.PositionTarget.y -= (Gui::GetIO().MouseWheel * scrollStep.y);
					else
						Camera.PositionTarget.x += (Gui::GetIO().MouseWheel * scrollStep.x);
				}
			}
		}

		// NOTE: Mouse wheel grab scroll
		{
			if (IsContentWindowHovered && Gui::IsMouseClicked(ImGuiMouseButton_Middle))
				IsCameraMouseGrabActive = true;
			else if (IsCameraMouseGrabActive && !Gui::IsMouseDown(ImGuiMouseButton_Middle))
				IsCameraMouseGrabActive = false;

			if (IsCameraMouseGrabActive)
			{
				Gui::SetActiveID(Gui::GetID(&IsCameraMouseGrabActive), Gui::GetCurrentWindow());
				Gui::SetMouseCursor(ImGuiMouseCursor_Hand);

				// BUG: Kinda freaks out when mouse grabbing and smooth zooming at the same time
				Camera.PositionTarget.x = Camera.PositionCurrent.x += (MousePosLastFrame.x - MousePosThisFrame.x);
				Camera.ZoomTarget.x = Camera.ZoomCurrent.x;
			}
		}

		// NOTE: Auto playback scroll (needs to be updated before the toggle playback input check is run for better result (?))
		{
			if (context.GetIsPlayback() && Audio::Engine.GetIsStreamOpenRunning())
			{
				const Time cursorTime = context.GetCursorTime();
				if (IsTimelineCursorVisibleOnScreen(Camera, Regions, cursorTime) && Camera.TimeToLocalSpaceX(cursorTime) >= Round(Regions.Content.GetWidth() * TimelineAutoScrollLockContentWidthFactor))
				{
					const Time elapsedCursorTime = Time::FromSec(Gui::DeltaTime()) * context.GetPlaybackSpeed();
					const f32 cameraScrollIncrement = Camera.TimeToWorldSpaceX(elapsedCursorTime) * Camera.ZoomCurrent.x;
					Camera.PositionCurrent.x += cameraScrollIncrement;
					Camera.PositionTarget.x += cameraScrollIncrement;
				}
			}
		}

		// NOTE: Cursor controls
		{
			// NOTE: Selected items mouse drag
			{
				ChartCourse& selectedCourse = *context.ChartSelectedCourse;

				size_t selectedItemCount = 0; b8 allSelectedItemsAreNotes = true; b8 atLeastOneSelectedItemIsTempoChange = false;
				ForEachSelectedChartItem(selectedCourse, [&](const ForEachChartItemData& it)
				{
					selectedItemCount++;
					allSelectedItemsAreNotes &= IsNotesList(it.List);
					atLeastOneSelectedItemIsTempoChange |= (it.List == GenericList::TempoChanges);
				});

				if (SelectedItemDrag.IsActive && !Gui::IsMouseDown(ImGuiMouseButton_Left))
					SelectedItemDrag = {};

				SelectedItemDrag.IsHovering = false;
				SelectedItemDrag.MouseBeatLastFrame = SelectedItemDrag.MouseBeatThisFrame;
				SelectedItemDrag.MouseBeatThisFrame = FloorBeatToCurrentGrid(context.TimeToBeat(Camera.LocalSpaceXToTime(ScreenToLocalSpace(MousePosThisFrame).x)));

				if (selectedItemCount > 0 && IsContentWindowHovered && !SelectedItemDrag.IsActive)
				{
					ForEachTimelineRow(*this, [&](const ForEachRowData& rowIt)
					{
						const GenericList list = TimelineRowToGenericList(rowIt.RowType);
						const b8 isNotesRow = IsNotesList(list);

						const Rect screenRowRect = Rect(LocalToScreenSpace(vec2(0.0f, rowIt.LocalY)), LocalToScreenSpace(vec2(Regions.Content.GetWidth(), rowIt.LocalY + rowIt.LocalHeight)));
						const vec2 screenRectCenter = screenRowRect.GetCenter();

						for (size_t i = 0; i < GetGenericListCount(selectedCourse, list); i++)
						{
							GenericMemberUnion beatStart, beatDuration, isSelected, noteType;
							if (TryGetGeneric(selectedCourse, list, i, GenericMember::B8_IsSelected, isSelected) && isSelected.B8)
							{
								const b8 hasBeatStart = TryGetGeneric(selectedCourse, list, i, GenericMember::Beat_Start, beatStart);
								const b8 hasBeatDuration = TryGetGeneric(selectedCourse, list, i, GenericMember::Beat_Duration, beatDuration);

								Rect screenHitbox = {};
								if (isNotesRow)
								{
									TryGetGeneric(selectedCourse, list, i, GenericMember::NoteType_V, noteType);

									const vec2 center = vec2(LocalToScreenSpace(vec2(Camera.TimeToLocalSpaceX(context.BeatToTime(beatStart.Beat)), 0.0f)).x, screenRectCenter.y);
									screenHitbox = Rect::FromCenterSize(center, vec2(GuiScale(IsBigNote(noteType.NoteType) ? TimelineSelectedNoteHitBoxSizeBig : TimelineSelectedNoteHitBoxSizeSmall)));
								}
								else
								{
									// TODO: Proper hitboxses (at least for gogo range and lyrics?)
									const vec2 center = vec2(LocalToScreenSpace(vec2(Camera.TimeToLocalSpaceX(context.BeatToTime(beatStart.Beat)), 0.0f)).x, screenRectCenter.y);
									screenHitbox = Rect::FromCenterSize(center, vec2(GuiScale(TimelineSelectedNoteHitBoxSizeSmall)));
								}

								if (screenHitbox.Contains(MousePosThisFrame))
								{
									SelectedItemDrag.IsHovering = true;
									if (Gui::IsMouseClicked(ImGuiMouseButton_Left))
									{
										SelectedItemDrag.IsActive = true;
										SelectedItemDrag.BeatOnMouseDown = SelectedItemDrag.MouseBeatThisFrame;
										SelectedItemDrag.BeatDistanceMovedSoFar = Beat::Zero();
										context.Undo.DisallowMergeForLastCommand();
									}
								}
							}
						}
					});
				}

				if (SelectedItemDrag.IsActive || SelectedItemDrag.IsHovering)
					Gui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

				if (SelectedItemDrag.IsActive)
				{
					// TODO: Maybe should set active ID here too... thought that then conflicts with WindowIsHovered and other actions
					// Gui::SetActiveID(Gui::GetID(&SelectedItemDrag), Gui::GetCurrentWindow());
					SelectedItemDrag.BeatDistanceMovedSoFar += (SelectedItemDrag.MouseBeatThisFrame - SelectedItemDrag.MouseBeatLastFrame);
					context.Undo.ResetMergeTimeThresholdStopwatch();
				}

				if (SelectedItemDrag.IsActive)
				{
					auto itemSelected = [&](GenericList list, size_t i) { GenericMemberUnion out; TryGetGeneric(selectedCourse, list, i, GenericMember::B8_IsSelected, out); return out.B8; };
					auto itemStart = [&](GenericList list, size_t i) { GenericMemberUnion out; TryGetGeneric(selectedCourse, list, i, GenericMember::Beat_Start, out); return out.Beat; };
					auto itemDuration = [&](GenericList list, size_t i) { GenericMemberUnion out; TryGetGeneric(selectedCourse, list, i, GenericMember::Beat_Duration, out); return out.Beat; };
					auto checkCanSelectedItemsBeMoved = [&](GenericList list, Beat beatIncrement) -> b8
					{
						const i32 listCount = static_cast<i32>(GetGenericListCount(selectedCourse, list));
						if (listCount <= 0)
							return true;

						// BUG: It's possible to move two non-empty lyric changes on top of each other, which isn't a critical bug (as it's still handled correctly) 
						//		but definitely annoying and a bit confusing
						// TODO: Rework LyricChange to be Beat+Duration based to avoid the problem all together and also simplify the rest of the code
						const b8 inclusiveBeatCheck = ListUsesInclusiveBeatCheck(list);
						if (beatIncrement > Beat::Zero())
						{
							for (i32 thisIndex = 0; thisIndex < listCount; thisIndex++)
							{
								const i32 nextIndex = thisIndex + 1;
								const b8 hasNext = (nextIndex < listCount);
								if (itemSelected(list, thisIndex) && hasNext && !itemSelected(list, nextIndex))
								{
									const Beat thisEnd = itemStart(list, thisIndex) + itemDuration(list, thisIndex);
									const Beat nextStart = itemStart(list, nextIndex);
									if (inclusiveBeatCheck)
									{
										if (thisEnd + beatIncrement >= nextStart)
											return false;
									}
									else
									{
										if (thisEnd + beatIncrement > nextStart)
											return false;
									}
								}
							}
						}
						else
						{
							for (i32 thisIndex = (listCount - 1); thisIndex >= 0; thisIndex--)
							{
								const i32 prevIndex = thisIndex - 1;
								const b8 hasPrev = (prevIndex >= 0);
								if (itemSelected(list, thisIndex))
								{
									const Beat thisStart = itemStart(list, thisIndex);
									if (thisStart + beatIncrement < Beat::Zero())
										return false;

									if (hasPrev && !itemSelected(list, prevIndex))
									{
										const Beat prevEnd = itemStart(list, prevIndex) + itemDuration(list, prevIndex);
										if (inclusiveBeatCheck)
										{
											if (thisStart + beatIncrement <= prevEnd)
												return false;
										}
										else
										{
											if (thisStart + beatIncrement < prevEnd)
												return false;
										}
									}
								}
							}
						}
						return true;
					};

					const Beat cursorBeat = context.GetCursorBeat();
					const Beat dragBeatIncrement = FloorBeatToCurrentGrid(SelectedItemDrag.BeatDistanceMovedSoFar);

					// BUG: Doesn't account for smooth scroll delay and playback auto scrolling
					const b8 wasMouseMovedOrScrolled = (!ApproxmiatelySame(Gui::GetIO().MouseDelta.x, 0.0f) || !ApproxmiatelySame(Gui::GetIO().MouseWheel, 0.0f));

					if (dragBeatIncrement != Beat::Zero() && wasMouseMovedOrScrolled)
					{
						b8 allItemsCanBeMoved = true;
						for (GenericList list = {}; list < GenericList::Count; IncrementEnum(list))
							allItemsCanBeMoved &= checkCanSelectedItemsBeMoved(list, dragBeatIncrement);

						if (allItemsCanBeMoved)
						{
							SelectedItemDrag.BeatDistanceMovedSoFar -= dragBeatIncrement;

							auto& notes = selectedCourse.GetNotes(context.ChartSelectedBranch);
							if (allSelectedItemsAreNotes)
							{
								std::vector<Commands::ChangeMultipleNoteBeats::Data> noteBeatsToChange;
								noteBeatsToChange.reserve(selectedItemCount);
								for (const Note& note : notes)
									if (note.IsSelected) { auto& data = noteBeatsToChange.emplace_back(); data.Index = ArrayItToIndex(&note, &notes[0]); data.NewBeat = (note.BeatTime + dragBeatIncrement); }

								context.Undo.Execute<Commands::ChangeMultipleNoteBeats_MoveNotes>(&notes, std::move(noteBeatsToChange));
							}
							else
							{
								std::vector<Commands::ChangeMultipleGenericProperties::Data> itemsToChange;
								itemsToChange.reserve(selectedItemCount);

								ForEachSelectedChartItem(selectedCourse, [&](const ForEachChartItemData& it)
								{
									auto& data = itemsToChange.emplace_back();
									data.Index = it.Index;
									data.List = it.List;
									data.Member = GenericMember::Beat_Start;
									data.NewValue.Beat = it.GetBeat(selectedCourse) + dragBeatIncrement;
								});

								context.Undo.Execute<Commands::ChangeMultipleGenericProperties_MoveItems>(&selectedCourse, std::move(itemsToChange));
							}

							for (const Note& note : notes)
								if (note.IsSelected) { if (note.BeatTime == cursorBeat) PlayNoteSoundAndHitAnimationsAtBeat(context, note.BeatTime); }

							// NOTE: Set again to account for a changes in cursor time
							if (atLeastOneSelectedItemIsTempoChange)
								context.SetCursorBeat(cursorBeat);
						}
					}
				}
			}

			if (IsContentWindowHovered && !SelectedItemDrag.IsHovering && Gui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				const Time oldCursorTime = context.GetCursorTime();
				const f32 oldCursorLocalSpaceX = Camera.TimeToLocalSpaceX(oldCursorTime);

				const Time timeAtMouseX = Camera.LocalSpaceXToTime(ScreenToLocalSpace(MousePosThisFrame).x);
				const Beat newCursorBeat = FloorBeatToCurrentGrid(context.TimeToBeat(timeAtMouseX));

				context.SetCursorBeat(newCursorBeat);
				PlayNoteSoundAndHitAnimationsAtBeat(context, newCursorBeat);

				if (context.GetIsPlayback())
				{
					const Time newCursorTime = context.BeatToTime(newCursorBeat);
					const f32 localCursorX = Camera.TimeToLocalSpaceX(newCursorTime);
					const f32 localAutoScrollLockX = Round(Regions.Content.GetWidth() * TimelineAutoScrollLockContentWidthFactor);

					// BUG: *Sometimes* ugly ~1px jutter/lag when jumping forward..? (?)
					// printf("localCursorX: %f\n\n", localCursorX);
					// printf("localAutoScrollLockX: %f\n", localAutoScrollLockX);

					// TODO: Should this be a condition too..?
					// const b8 clickedForward = (newCursorTime >= oldCursorTime);

					const f32 distanceFromAutoScrollPositionX = (localCursorX - localAutoScrollLockX);
					if (/*clickedForward &&*/ distanceFromAutoScrollPositionX > 0.0f)
						Camera.PositionTarget.x += distanceFromAutoScrollPositionX;
				}
			}

			// NOTE: Header bar cursor scrubbing
			{
				if (IsContentHeaderWindowHovered && !SelectedItemDrag.IsHovering && Gui::IsMouseClicked(ImGuiMouseButton_Left))
					IsCursorMouseScrubActive = true;

				if (!Gui::IsMouseDown(ImGuiMouseButton_Left) || !Gui::IsMousePosValid())
					IsCursorMouseScrubActive = false;

				if (IsCursorMouseScrubActive)
				{
					const f32 mouseLocalSpaceX = ScreenToLocalSpace(MousePosThisFrame).x;
					const Time mouseCursorTime = Camera.LocalSpaceXToTime(mouseLocalSpaceX);
					const Beat oldCursorBeat = context.GetCursorBeat();
					const Beat newCursorBeat = context.TimeToBeat(mouseCursorTime);

					const f32 threshold = ClampBot(GuiScale(*Settings.General.TimelineScrubAutoScrollPixelThreshold), 1.0f);
					const f32 speedMin = *Settings.General.TimelineScrubAutoScrollSpeedMin, speedMax = *Settings.General.TimelineScrubAutoScrollSpeedMax;

					const f32 modifier = Gui::GetIO().KeyAlt ? 0.25f : Gui::GetIO().KeyShift ? 2.0f : 1.0f;
					if (const f32 left = threshold; mouseLocalSpaceX < left)
					{
						const f32 scrollIncrementThisFrame = ConvertRange(threshold, 0.0f, speedMin, speedMax, mouseLocalSpaceX) * modifier * Gui::DeltaTime();
						if (*Settings.General.TimelineScrubAutoScrollEnableClamp)
						{
							const f32 minScrollX = TimelineCameraBaseScrollX;
							Camera.PositionCurrent.x = ClampBot(Camera.PositionCurrent.x - scrollIncrementThisFrame, ClampTop(Camera.PositionCurrent.x, minScrollX));
							Camera.PositionTarget.x = ClampBot(Camera.PositionTarget.x - scrollIncrementThisFrame, ClampTop(Camera.PositionTarget.x, minScrollX));
						}
						else
						{
							Camera.PositionCurrent.x -= scrollIncrementThisFrame;
							Camera.PositionTarget.x -= scrollIncrementThisFrame;
						}
					}
					if (const f32 right = (Regions.ContentHeader.GetWidth() - threshold); mouseLocalSpaceX >= right)
					{
						const f32 scrollIncrementThisFrame = ConvertRange(0.0f, threshold, speedMin, speedMax, mouseLocalSpaceX - right) * modifier * Gui::DeltaTime();
						if (*Settings.General.TimelineScrubAutoScrollEnableClamp)
						{
							const f32 maxScrollX = Camera.WorldToLocalSpaceScale(vec2(Camera.TimeToWorldSpaceX(context.Chart.GetDurationOrDefault()), 0.0f)).x - Regions.ContentHeader.GetWidth() + 1.0f;
							Camera.PositionCurrent.x = ClampTop(Camera.PositionCurrent.x + scrollIncrementThisFrame, ClampBot(Camera.PositionCurrent.x, maxScrollX));
							Camera.PositionTarget.x = ClampTop(Camera.PositionTarget.x + scrollIncrementThisFrame, ClampBot(Camera.PositionTarget.x, maxScrollX));
						}
						else
						{
							Camera.PositionCurrent.x += scrollIncrementThisFrame;
							Camera.PositionTarget.x += scrollIncrementThisFrame;
						}
					}

					if (newCursorBeat != oldCursorBeat)
					{
						context.SetCursorBeat(newCursorBeat);
						WorldSpaceCursorXAnimationCurrent = Camera.LocalToWorldSpace(vec2(mouseLocalSpaceX, 0.0f)).x;
						if (MousePosThisFrame.x != MousePosLastFrame.x) PlayNoteSoundAndHitAnimationsAtBeat(context, newCursorBeat);
					}
				}
			}

			if (HasKeyboardFocus())
			{
				if (Gui::GetActiveID() == 0)
				{
					if (Gui::IsAnyPressed(*Settings.Input.Timeline_Cut, false)) ExecuteClipboardAction(context, ClipboardAction::Cut);
					if (Gui::IsAnyPressed(*Settings.Input.Timeline_Copy, false)) ExecuteClipboardAction(context, ClipboardAction::Copy);
					if (Gui::IsAnyPressed(*Settings.Input.Timeline_Paste, false)) ExecuteClipboardAction(context, ClipboardAction::Paste);
					if (Gui::IsAnyPressed(*Settings.Input.Timeline_DeleteSelection, false)) ExecuteClipboardAction(context, ClipboardAction::Delete);

					SelectionActionParam param {};
					if (Gui::IsAnyPressed(*Settings.Input.Timeline_SelectAll, false)) ExecuteSelectionAction(context, SelectionAction::SelectAll, param);
					if (Gui::IsAnyPressed(*Settings.Input.Timeline_ClearSelection, false)) ExecuteSelectionAction(context, SelectionAction::UnselectAll, param);
					if (Gui::IsAnyPressed(*Settings.Input.Timeline_InvertSelection, false)) ExecuteSelectionAction(context, SelectionAction::InvertAll, param);
					if (Gui::IsAnyPressed(*Settings.Input.Timeline_SelectAllWithinRangeSelection, false)) ExecuteSelectionAction(context, SelectionAction::SelectAllWithinRangeSelection, param);
					if (Gui::IsAnyPressed(*Settings.Input.Timeline_ShiftSelectionLeft, true)) ExecuteSelectionAction(context, SelectionAction::PerRowShiftSelected, param.SetShiftDelta(-1));
					if (Gui::IsAnyPressed(*Settings.Input.Timeline_ShiftSelectionRight, true)) ExecuteSelectionAction(context, SelectionAction::PerRowShiftSelected, param.SetShiftDelta(+1));
					if (Gui::IsAnyPressed(*Settings.Input.Timeline_SelectItemPattern_xo, false)) ExecuteSelectionAction(context, SelectionAction::PerRowSelectPattern, param.SetPattern("xo"));
					if (Gui::IsAnyPressed(*Settings.Input.Timeline_SelectItemPattern_xoo, false)) ExecuteSelectionAction(context, SelectionAction::PerRowSelectPattern, param.SetPattern("xoo"));
					if (Gui::IsAnyPressed(*Settings.Input.Timeline_SelectItemPattern_xooo, false)) ExecuteSelectionAction(context, SelectionAction::PerRowSelectPattern, param.SetPattern("xooo"));
					if (Gui::IsAnyPressed(*Settings.Input.Timeline_SelectItemPattern_xxoo, false)) ExecuteSelectionAction(context, SelectionAction::PerRowSelectPattern, param.SetPattern("xxoo"));

					const MultiInputBinding* customBindings[] =
					{
						&*Settings.Input.Timeline_SelectItemPattern_CustomA, &*Settings.Input.Timeline_SelectItemPattern_CustomB, &*Settings.Input.Timeline_SelectItemPattern_CustomC,
						&*Settings.Input.Timeline_SelectItemPattern_CustomD, &*Settings.Input.Timeline_SelectItemPattern_CustomE, &*Settings.Input.Timeline_SelectItemPattern_CustomF,
					};

					for (size_t i = 0; i < ArrayCount(customBindings); i++)
					{
						if (i < Settings.General.CustomSelectionPatterns->V.size() && Gui::IsAnyPressed(*customBindings[i], false))
							ExecuteSelectionAction(context, SelectionAction::PerRowSelectPattern, param.SetPattern(Settings.General.CustomSelectionPatterns->V[i].Data));
					}

					if (Gui::IsAnyPressed(*Settings.Input.Timeline_ConvertSelectionToScrollChanges, false)) ExecuteConvertSelectionToScrollChanges(context);
				}

				if (const auto& io = Gui::GetIO(); !io.KeyCtrl)
				{
					auto stepCursorByBeat = [&](Beat beatIncrement)
					{
						const auto oldCursorBeatAndTime = context.GetCursorBeatAndTime();
						const Beat oldCursorBeat = RoundBeatToCurrentGrid(oldCursorBeatAndTime.Beat);
						const Beat newCursorBeat = oldCursorBeat + beatIncrement;

						context.SetCursorBeat(newCursorBeat);
						PlayNoteSoundAndHitAnimationsAtBeat(context, newCursorBeat);

						// BUG: It's still possible to move the cursor off screen without it auto-scrolling if the cursor is on the far right edge of the screen
						const f32 cursorLocalSpaceXOld = Camera.TimeToLocalSpaceX_AtTarget(oldCursorBeatAndTime.Time);
						if (cursorLocalSpaceXOld >= 0.0f && cursorLocalSpaceXOld <= Regions.Content.GetWidth())
						{
							const f32 cursorLocalSpaceX = Camera.TimeToLocalSpaceX(context.BeatToTime(newCursorBeat));
							Camera.PositionTarget.x += (cursorLocalSpaceX - Camera.TimeToLocalSpaceX(oldCursorBeatAndTime.Time));
							WorldSpaceCursorXAnimationCurrent = Camera.LocalToWorldSpace(vec2(cursorLocalSpaceX, 0.0f)).x;
						}
					};

					// TODO: Maybe refine further...
					const Beat cursorStepKeyDistance = context.GetIsPlayback() ?
						(io.KeyAlt ? GetGridBeatSnap(CurrentGridBarDivision) : io.KeyShift ? Beat::FromBeats(2) : Beat::FromBeats(1)) :
						(io.KeyShift ? Beat::FromBeats(1) : GetGridBeatSnap(CurrentGridBarDivision));

					// NOTE: Separate down checks for right priority, although ideally the *last held* binding should probably be prioritized instead (which would be a bit tricky)
					if (Gui::IsAnyDown(*Settings.Input.Timeline_StepCursorRight, InputModifierBehavior::Relaxed))
					{
						if (Gui::IsAnyPressed(*Settings.Input.Timeline_StepCursorRight, true, InputModifierBehavior::Relaxed)) { stepCursorByBeat(cursorStepKeyDistance * +1); }
					}
					else if (Gui::IsAnyDown(*Settings.Input.Timeline_StepCursorLeft, InputModifierBehavior::Relaxed))
					{
						if (Gui::IsAnyPressed(*Settings.Input.Timeline_StepCursorLeft, true, InputModifierBehavior::Relaxed)) { stepCursorByBeat(cursorStepKeyDistance * -1); }
					}
				}

				if (Gui::IsAnyPressed(*Settings.Input.Timeline_JumpToTimelineStart, false)) ScrollToTimelinePosition(Camera, Regions, context.Chart, 0.0f);
				if (Gui::IsAnyPressed(*Settings.Input.Timeline_JumpToTimelineEnd, false)) ScrollToTimelinePosition(Camera, Regions, context.Chart, 1.0f);
				if (Gui::IsAnyPressed(*Settings.Input.Timeline_StartEndRangeSelection, false)) StartEndRangeSelectionAtCursor(context);
			}

			// NOTE: Grid snap controls
			if (IsContentWindowHovered || HasKeyboardFocus())
			{
				i32 currentGridDivisionIndex = 0;
				for (const i32& it : AllowedGridBarDivisions) if (it == CurrentGridBarDivision) currentGridDivisionIndex = ArrayItToIndexI32(&it, &AllowedGridBarDivisions[0]);

				const b8 keyboardFocus = HasKeyboardFocus() && !Gui::GetIO().KeyCtrl;
				const b8 increaseGrid = (IsContentWindowHovered && Gui::IsMouseClicked(ImGuiMouseButton_X2, true)) || (keyboardFocus && Gui::IsAnyPressed(*Settings.Input.Timeline_IncreaseGridDivision, true, InputModifierBehavior::Relaxed));
				const b8 decreaseGrid = (IsContentWindowHovered && Gui::IsMouseClicked(ImGuiMouseButton_X1, true)) || (keyboardFocus && Gui::IsAnyPressed(*Settings.Input.Timeline_DecreaseGridDivision, true, InputModifierBehavior::Relaxed));
				if (increaseGrid) CurrentGridBarDivision = AllowedGridBarDivisions[Clamp(currentGridDivisionIndex + 1, 0, ArrayCountI32(AllowedGridBarDivisions) - 1)];
				if (decreaseGrid) CurrentGridBarDivision = AllowedGridBarDivisions[Clamp(currentGridDivisionIndex - 1, 0, ArrayCountI32(AllowedGridBarDivisions) - 1)];
			}
			if (HasKeyboardFocus())
			{
				const struct { const WithDefault<MultiInputBinding>& V; i32 BarDivision; } allBindings[] =
				{
					{ Settings.Input.Timeline_SetGridDivision_1_4, 4 },
					{ Settings.Input.Timeline_SetGridDivision_1_8, 8 },
					{ Settings.Input.Timeline_SetGridDivision_1_12, 12 },
					{ Settings.Input.Timeline_SetGridDivision_1_16, 16 },
					{ Settings.Input.Timeline_SetGridDivision_1_24, 24 },
					{ Settings.Input.Timeline_SetGridDivision_1_32, 32 },
					{ Settings.Input.Timeline_SetGridDivision_1_48, 48 },
					{ Settings.Input.Timeline_SetGridDivision_1_64, 64 },
					{ Settings.Input.Timeline_SetGridDivision_1_96, 96 },
					{ Settings.Input.Timeline_SetGridDivision_1_192, 192 },
				};

				for (const auto& binding : allBindings)
				{
					if (Gui::IsAnyPressed(*binding.V, false))
						CurrentGridBarDivision = binding.BarDivision;
				}
			}

			// NOTE: Playback speed controls
			if (HasKeyboardFocus())
			{
				if (const auto& io = Gui::GetIO(); !io.KeyCtrl)
				{
					const std::vector<f32>& speeds = io.KeyAlt ? Settings.General.PlaybackSpeedStepsPrecise->V : io.KeyShift ? Settings.General.PlaybackSpeedStepsRough->V : Settings.General.PlaybackSpeedSteps->V;

					i32 closetPlaybackSpeedIndex = 0;
					const f32 currentPlaybackSpeed = context.GetPlaybackSpeed();
					for (const f32& it : speeds) if (it >= currentPlaybackSpeed) closetPlaybackSpeedIndex = ArrayItToIndexI32(&it, &speeds[0]);

					if (Gui::IsAnyPressed(*Settings.Input.Timeline_IncreasePlaybackSpeed, true, InputModifierBehavior::Relaxed)) context.SetPlaybackSpeed(speeds[Clamp(closetPlaybackSpeedIndex - 1, 0, speeds.empty() ? 0 : static_cast<i32>(speeds.size() - 1))]);
					if (Gui::IsAnyPressed(*Settings.Input.Timeline_DecreasePlaybackSpeed, true, InputModifierBehavior::Relaxed)) context.SetPlaybackSpeed(speeds[Clamp(closetPlaybackSpeedIndex + 1, 0, speeds.empty() ? 0 : static_cast<i32>(speeds.size() - 1))]);
				}

				if (Gui::IsAnyPressed(*Settings.Input.Timeline_SetPlaybackSpeed_100, false)) context.SetPlaybackSpeed(FromPercent(100.0f));
				if (Gui::IsAnyPressed(*Settings.Input.Timeline_SetPlaybackSpeed_75, false)) context.SetPlaybackSpeed(FromPercent(75.0f));
				if (Gui::IsAnyPressed(*Settings.Input.Timeline_SetPlaybackSpeed_50, false)) context.SetPlaybackSpeed(FromPercent(50.0f));
				if (Gui::IsAnyPressed(*Settings.Input.Timeline_SetPlaybackSpeed_25, false)) context.SetPlaybackSpeed(FromPercent(25.0f));
			}

			if (HasKeyboardFocus() && Gui::IsAnyPressed(*Settings.Input.Timeline_TogglePlayback, false, InputModifierBehavior::Relaxed))
			{
				if (context.GetIsPlayback())
				{
					context.SetIsPlayback(false);
					WorldSpaceCursorXAnimationCurrent = Camera.TimeToWorldSpaceX(context.GetCursorTime());
				}
				else
				{
					Audio::Engine.EnsureStreamRunning();
					const Beat cursorBeat = context.GetCursorBeat();
					PlayNoteSoundAndHitAnimationsAtBeat(context, cursorBeat);

					context.SetIsPlayback(true);
				}
			}

			auto updateNotePlacementBinding = [this, &context](const MultiInputBinding& inputBinding, NoteType noteTypeToInsert)
			{
				if (Gui::IsAnyPressed(inputBinding, false, InputModifierBehavior::Relaxed))
				{
					SortedNotesList& notes = context.ChartSelectedCourse->GetNotes(context.ChartSelectedBranch);

					if (Gui::GetIO().KeyShift && RangeSelection.IsActiveAndHasEnd())
					{
						const Beat startTick = RoundBeatToCurrentGrid(RangeSelection.GetMin());
						const Beat endTick = RoundBeatToCurrentGrid(RangeSelection.GetMax());
						const Beat beatPerNote = GetGridBeatSnap(CurrentGridBarDivision);
						const i32 maxExpectedNoteCountToAdd = ((endTick - startTick).Ticks / beatPerNote.Ticks) + 1;

						std::vector<Note> newNotesToAdd;
						newNotesToAdd.reserve(maxExpectedNoteCountToAdd);

						for (i32 i = 0; i < maxExpectedNoteCountToAdd; i++)
						{
							const Beat beatForThisNote = Beat(Min(startTick, endTick).Ticks + (i * beatPerNote.Ticks));
							if (notes.TryFindOverlappingBeat(beatForThisNote, beatForThisNote) == nullptr)
							{
								Note& newNote = newNotesToAdd.emplace_back();
								newNote.BeatTime = beatForThisNote;
								newNote.Type = noteTypeToInsert;
							}
						}

						if (!newNotesToAdd.empty())
						{
							SetNotesWaveAnimationTimes(newNotesToAdd, (RangeSelection.Start < RangeSelection.End) ? +1 : -1);
							context.SfxVoicePool.PlaySound(SoundEffectTypeForNoteType(newNotesToAdd.front().Type));
							context.Undo.Execute<Commands::AddMultipleNotes>(&notes, std::move(newNotesToAdd));
						}
					}
					else
					{
						const b8 isPlayback = context.GetIsPlayback();
						const Beat cursorBeat = isPlayback ? RoundBeatToCurrentGrid(context.GetCursorBeat()) : FloorBeatToCurrentGrid(context.GetCursorBeat());

						Note* existingNoteAtCursor = notes.TryFindOverlappingBeat(cursorBeat, cursorBeat);
						if (existingNoteAtCursor != nullptr)
						{
							if (existingNoteAtCursor->BeatTime == cursorBeat)
							{
								existingNoteAtCursor->ClickAnimationTimeRemaining = existingNoteAtCursor->ClickAnimationTimeDuration = NoteHitAnimationDuration;
								if (!isPlayback)
								{
									if (ToSmallNote(existingNoteAtCursor->Type) == ToSmallNote(noteTypeToInsert) || (existingNoteAtCursor->BeatDuration > Beat::Zero()))
									{
										TempDeletedNoteAnimationsBuffer.push_back(DeletedNoteAnimation { *existingNoteAtCursor, context.ChartSelectedBranch, 0.0f });
										context.Undo.Execute<Commands::RemoveSingleNote>(&notes, *existingNoteAtCursor);
									}
									else
									{
										context.Undo.Execute<Commands::ChangeSingleNoteType>(&notes, Commands::ChangeSingleNoteType::Data { ArrayItToIndex(existingNoteAtCursor, &notes[0]), noteTypeToInsert });
										context.Undo.DisallowMergeForLastCommand();
									}
								}
							}
							context.SfxVoicePool.PlaySound(SoundEffectTypeForNoteType(existingNoteAtCursor->Type));
						}
						else if (cursorBeat >= Beat::Zero())
						{
							Note newNote {};
							newNote.BeatTime = cursorBeat;
							newNote.Type = noteTypeToInsert;
							newNote.ClickAnimationTimeRemaining = newNote.ClickAnimationTimeDuration = NoteHitAnimationDuration;
							context.Undo.Execute<Commands::AddSingleNote>(&notes, newNote);
							context.SfxVoicePool.PlaySound(SoundEffectTypeForNoteType(noteTypeToInsert));
						}
					}
				}
			};

			PlaceBalloonBindingDownLastFrame = PlaceBalloonBindingDownThisFrame;
			PlaceBalloonBindingDownThisFrame = HasKeyboardFocus() && Gui::IsAnyDown(*Settings.Input.Timeline_PlaceNoteBalloon, InputModifierBehavior::Relaxed);
			PlaceDrumrollBindingDownLastFrame = PlaceDrumrollBindingDownThisFrame;
			PlaceDrumrollBindingDownThisFrame = HasKeyboardFocus() && Gui::IsAnyDown(*Settings.Input.Timeline_PlaceNoteDrumroll, InputModifierBehavior::Relaxed);
			if (HasKeyboardFocus())
			{
				updateNotePlacementBinding(*Settings.Input.Timeline_PlaceNoteDon, ToBigNoteIf(NoteType::Don, Gui::GetIO().KeyAlt));
				updateNotePlacementBinding(*Settings.Input.Timeline_PlaceNoteKa, ToBigNoteIf(NoteType::Ka, Gui::GetIO().KeyAlt));

				if (PlaceBalloonBindingDownThisFrame || PlaceDrumrollBindingDownThisFrame)
				{
					const Beat cursorBeat = context.GetIsPlayback() ? RoundBeatToCurrentGrid(context.GetCursorBeat()) : FloorBeatToCurrentGrid(context.GetCursorBeat());
					LongNotePlacement.NoteType = ToBigNoteIf(PlaceBalloonBindingDownThisFrame ? NoteType::Balloon : NoteType::Drumroll, Gui::GetIO().KeyAlt);
					if (!LongNotePlacement.IsActive)
					{
						LongNotePlacement.IsActive = true;
						LongNotePlacement.CursorBeatHead = cursorBeat;
						context.SfxVoicePool.PlaySound(SoundEffectTypeForNoteType(LongNotePlacement.NoteType));
					}
					LongNotePlacement.CursorBeatTail = cursorBeat;
				}
			}
			else
			{
				LongNotePlacement = {};
			}

			auto placeLongNoteOnBindingRelease = [this, &context](NoteType longNoteType)
			{
				SortedNotesList& notes = context.ChartSelectedCourse->GetNotes(context.ChartSelectedBranch);
				const Beat minBeat = LongNotePlacement.GetMin();
				const Beat maxBeat = LongNotePlacement.GetMax();

				std::vector<Note> notesToRemove;
				for (const Note& existingNote : notes)
				{
					if (existingNote.GetStart() <= maxBeat && minBeat <= existingNote.GetEnd())
						notesToRemove.push_back(existingNote);
				}

				Note newLongNote {};
				newLongNote.BeatTime = minBeat;
				newLongNote.BeatDuration = (maxBeat - minBeat);
				newLongNote.BalloonPopCount = IsBalloonNote(longNoteType) ? DefaultBalloonPopCount(newLongNote.BeatDuration, CurrentGridBarDivision) : 0;
				newLongNote.Type = longNoteType;
				newLongNote.ClickAnimationTimeRemaining = newLongNote.ClickAnimationTimeDuration = NoteHitAnimationDuration;
				context.Undo.Execute<Commands::AddSingleLongNote>(&notes, newLongNote, std::move(notesToRemove));

				context.SfxVoicePool.PlaySound(SoundEffectTypeForNoteType(longNoteType));
			};

			const b8 activeFocusedAndHasLength = HasKeyboardFocus() && LongNotePlacement.IsActive && (LongNotePlacement.CursorBeatHead != LongNotePlacement.CursorBeatTail);
			if (PlaceBalloonBindingDownLastFrame && !PlaceBalloonBindingDownThisFrame) { if (activeFocusedAndHasLength) placeLongNoteOnBindingRelease(LongNotePlacement.NoteType); LongNotePlacement = {}; }
			if (PlaceDrumrollBindingDownLastFrame && !PlaceDrumrollBindingDownThisFrame) { if (activeFocusedAndHasLength) placeLongNoteOnBindingRelease(LongNotePlacement.NoteType); LongNotePlacement = {}; }

			if (HasKeyboardFocus())
			{
				TransformActionParam param {};
				if (Gui::IsAnyPressed(*Settings.Input.Timeline_FlipNoteType, false)) ExecuteTransformAction(context, TransformAction::FlipNoteType, param);
				if (Gui::IsAnyPressed(*Settings.Input.Timeline_ToggleNoteSize, false)) ExecuteTransformAction(context, TransformAction::ToggleNoteSize, param);
				if (Gui::IsAnyPressed(*Settings.Input.Timeline_ExpandItemTime_2To1, false)) ExecuteTransformAction(context, TransformAction::ScaleItemTime, param.SetTimeRatio(2, 1));
				if (Gui::IsAnyPressed(*Settings.Input.Timeline_ExpandItemTime_3To2, false)) ExecuteTransformAction(context, TransformAction::ScaleItemTime, param.SetTimeRatio(3, 2));
				if (Gui::IsAnyPressed(*Settings.Input.Timeline_ExpandItemTime_4To3, false)) ExecuteTransformAction(context, TransformAction::ScaleItemTime, param.SetTimeRatio(4, 3));
				if (Gui::IsAnyPressed(*Settings.Input.Timeline_CompressItemTime_1To2, false)) ExecuteTransformAction(context, TransformAction::ScaleItemTime, param.SetTimeRatio(1, 2));
				if (Gui::IsAnyPressed(*Settings.Input.Timeline_CompressItemTime_2To3, false)) ExecuteTransformAction(context, TransformAction::ScaleItemTime, param.SetTimeRatio(2, 3));
				if (Gui::IsAnyPressed(*Settings.Input.Timeline_CompressItemTime_3To4, false)) ExecuteTransformAction(context, TransformAction::ScaleItemTime, param.SetTimeRatio(3, 4));
			}
		}

		if (HasKeyboardFocus() && Gui::IsAnyPressed(*Settings.Input.Timeline_ToggleMetronome))
		{
			Metronome.IsEnabled ^= true;
			if (!context.GetIsPlayback())
				context.SfxVoicePool.PlaySound(SoundEffectType::MetronomeBeat);
		}

		// NOTE: Playback preview sounds / metronome
		if (context.GetIsPlayback() && (PlaybackSoundsEnabled || Metronome.IsEnabled))
			UpdateTimelinePlaybackAndMetronomneSounds(context, PlaybackSoundsEnabled, Metronome);

		// NOTE: Mouse selection box
		{
			static constexpr auto getBoxSelectionAction = [](const ImGuiIO& io)
			{
				const b8 shift = io.KeyShift, alt = io.KeyAlt;
				return (shift && alt) ? BoxSelectionAction::XOR : shift ? BoxSelectionAction::Add : alt ? BoxSelectionAction::Sub : BoxSelectionAction::Clear;
			};

			if (IsContentWindowHovered && Gui::IsMouseClicked(ImGuiMouseButton_Right))
			{
				BoxSelection.IsActive = true;
				BoxSelection.Action = getBoxSelectionAction(Gui::GetIO());
				BoxSelection.WorldSpaceRect.TL = BoxSelection.WorldSpaceRect.BR = Camera.LocalToWorldSpace(ScreenToLocalSpace(Gui::GetMousePos()));
			}
			else if (BoxSelection.IsActive && Gui::IsMouseDown(ImGuiMouseButton_Right))
			{
				BoxSelection.WorldSpaceRect.BR = Camera.LocalToWorldSpace(ScreenToLocalSpace(Gui::GetMousePos()));
				BoxSelection.Action = getBoxSelectionAction(Gui::GetIO());
				RangeSelection = {};
			}
			else
			{
				if (BoxSelection.IsActive)
					BoxSelection.Action = getBoxSelectionAction(Gui::GetIO());

				if (BoxSelection.IsActive && Gui::IsMouseReleased(ImGuiMouseButton_Right))
				{
					static constexpr f32 minBoxSizeThreshold = 4.0f;
					const Rect screenSpaceRect = Rect(Camera.WorldToLocalSpace(BoxSelection.WorldSpaceRect.TL), Camera.WorldToLocalSpace(BoxSelection.WorldSpaceRect.BR));
					if (Absolute(screenSpaceRect.GetWidth()) >= minBoxSizeThreshold && Absolute(screenSpaceRect.GetHeight()) >= minBoxSizeThreshold)
					{
						const vec2 screenSelectionMin = LocalToScreenSpace(Camera.WorldToLocalSpace(BoxSelection.WorldSpaceRect.GetMin()));
						const vec2 screenSelectionMax = LocalToScreenSpace(Camera.WorldToLocalSpace(BoxSelection.WorldSpaceRect.GetMax()));
						const Beat selectionBeatMin = context.TimeToBeat(Camera.WorldSpaceXToTime(BoxSelection.WorldSpaceRect.GetMin().x));
						const Beat selectionBeatMax = context.TimeToBeat(Camera.WorldSpaceXToTime(BoxSelection.WorldSpaceRect.GetMax().x));

						ForEachTimelineRow(*this, [&](const ForEachRowData& rowIt)
						{
							const GenericList list = TimelineRowToGenericList(rowIt.RowType);
							const b8 isNotesRow = IsNotesList(list);

							enum class IntersectionTest : u8 { Center, FullRow };
							const IntersectionTest intersectionTest = (list == GenericList::GoGoRanges || list == GenericList::Lyrics) ? IntersectionTest::FullRow : IntersectionTest::Center;

							const Rect screenRowRect = Rect(LocalToScreenSpace(vec2(0.0f, rowIt.LocalY)), LocalToScreenSpace(vec2(Regions.Content.GetWidth(), rowIt.LocalY + rowIt.LocalHeight)));
							const f32 screenMinY = (intersectionTest == IntersectionTest::Center) ? screenRowRect.GetCenter().y : screenRowRect.TL.y;
							const f32 screenMaxY = (intersectionTest == IntersectionTest::Center) ? screenRowRect.GetCenter().y : screenRowRect.BR.y;

							for (size_t i = 0; i < GetGenericListCount(*context.ChartSelectedCourse, list); i++)
							{
								GenericMemberUnion beatStart, beatDuration, isSelected;
								const b8 hasBeatStart = TryGetGeneric(*context.ChartSelectedCourse, list, i, GenericMember::Beat_Start, beatStart);
								const b8 hasBeatDuration = TryGetGeneric(*context.ChartSelectedCourse, list, i, GenericMember::Beat_Duration, beatDuration);
								const b8 hasIsSelected = TryGetGeneric(*context.ChartSelectedCourse, list, i, GenericMember::B8_IsSelected, isSelected);
								assert(hasBeatStart && hasIsSelected);

								const Beat beatMin = beatStart.Beat;
								const Beat beatMax = (hasBeatDuration && !isNotesRow) ? (beatStart.Beat + beatDuration.Beat) : beatStart.Beat;
								const b8 isInsideSelectionBox = (beatMin <= selectionBeatMax) && (beatMax >= selectionBeatMin) && (screenMinY <= screenSelectionMax.y) && (screenMaxY >= screenSelectionMin.y);

								switch (BoxSelection.Action)
								{
								case BoxSelectionAction::Clear: { isSelected.B8 = isInsideSelectionBox; } break;
								case BoxSelectionAction::Add: { if (isInsideSelectionBox) isSelected.B8 = true; } break;
								case BoxSelectionAction::Sub: { if (isInsideSelectionBox) isSelected.B8 = false; } break;
								case BoxSelectionAction::XOR: { isSelected.B8 ^= isInsideSelectionBox; } break;
								}

								TrySetGeneric(*context.ChartSelectedCourse, list, i, GenericMember::B8_IsSelected, isSelected);
							}
						});
					}
				}

				BoxSelection.IsActive = false;
				BoxSelection.WorldSpaceRect.TL = BoxSelection.WorldSpaceRect.BR = vec2(0.0f);
			}

			// TODO: Animate notes when including them in selection box (?)
			if (BoxSelection.IsActive)
			{
				// BUG: Doesn't work perfectly (mostly with the scrollbar) but need to prevent other window widgets from being interactable
				Gui::SetActiveID(Gui::GetID(&BoxSelection), Gui::GetCurrentWindow());

				// NOTE: Auto scroll timeline if selection offscreen and mouse moved (similar to how it works in reaper)
				if ((MousePosThisFrame.x < Regions.Content.TL.x) || (MousePosThisFrame.x > Regions.Content.BR.x))
				{
					const vec2 mouseDelta = (MousePosLastFrame - MousePosThisFrame);
					if (!ApproxmiatelySame(mouseDelta.x, 0.0f) || !ApproxmiatelySame(mouseDelta.y, 0.0f))
					{
						// TODO: Make global setting and adjust default value
						static constexpr f32 scrollSpeed = 3.0f; // 4.0f;

						// NOTE: Using the max axis feels more natural than the vector length
						const f32 mouseDistanceMoved = Max(Absolute(mouseDelta.x), Absolute(mouseDelta.y));
						const f32 scrollDirection = (MousePosThisFrame.x < Regions.Content.TL.x) ? -1.0f : +1.0f;
						Camera.PositionTarget.x = ClampBot(Min(TimelineCameraBaseScrollX, Camera.PositionTarget.x), Camera.PositionTarget.x + (scrollDirection * mouseDistanceMoved * scrollSpeed));
					}
				}
			}
		}
	}

	void ChartTimeline::UpdateAllAnimationsAfterUserInput(ChartContext& context)
	{
		// BUG: Freaks out when zoomed in too far / the timeline is too long (16min song for example) float precision issue (?)
		Camera.UpdateAnimations();
		Gui::AnimateExponential(&context.SongWaveformFadeAnimationCurrent, context.SongWaveformFadeAnimationTarget, *Settings.Animation.TimelineWaveformFadeSpeed);
		Gui::AnimateExponential(&RangeSelectionExpansionAnimationCurrent, RangeSelectionExpansionAnimationTarget, *Settings.Animation.TimelineRangeSelectionExpansionSpeed);

		const f32 worldSpaceCursorXAnimationTarget = Camera.TimeToWorldSpaceX(context.GetCursorTime());
		Gui::AnimateExponential(&WorldSpaceCursorXAnimationCurrent, worldSpaceCursorXAnimationTarget, *Settings.Animation.TimelineWorldSpaceCursorXSpeed);

		for (auto& course : context.Chart.Courses)
		{
			const f32 elapsedAnimationTimeSec = Gui::DeltaTime();
			for (BranchType branch = {}; branch < BranchType::Count; IncrementEnum(branch))
			{
				for (Note& note : course->GetNotes(branch))
					note.ClickAnimationTimeRemaining = ClampBot(note.ClickAnimationTimeRemaining - elapsedAnimationTimeSec, 0.0f);
			}

			for (auto& gogo : course->GoGoRanges)
				Gui::AnimateExponential(&gogo.ExpansionAnimationCurrent, gogo.ExpansionAnimationTarget, *Settings.Animation.TimelineGoGoRangeExpansionSpeed);
		}

		if (!TempDeletedNoteAnimationsBuffer.empty())
		{
			for (auto& data : TempDeletedNoteAnimationsBuffer)
				data.ElapsedTimeSec += Gui::DeltaTime();
			erase_remove_if(TempDeletedNoteAnimationsBuffer, [](auto& v) { return (v.ElapsedTimeSec >= NoteDeleteAnimationDuration); });
		}

		static constexpr f32 maxZoomLevelAtWhichToFadeOutGridBeatSnapLines = 0.5f;
		const f32 gridBeatSnapLineAnimationTarget = (Camera.ZoomCurrent.x <= maxZoomLevelAtWhichToFadeOutGridBeatSnapLines) ? 0.0f : 1.0f;
		Gui::AnimateExponential(&GridSnapLineAnimationCurrent, gridBeatSnapLineAnimationTarget, *Settings.Animation.TimelineGridSnapLineSpeed);
		GridSnapLineAnimationCurrent = Clamp(GridSnapLineAnimationCurrent, 0.0f, 1.0f);
	}

	void ChartTimeline::DrawAllAtEndOfFrame(ChartContext& context)
	{
		const b8 isPlayback = context.GetIsPlayback();
		const BeatAndTime cursorBeatAndTime = context.GetCursorBeatAndTime();
		const Time cursorTime = cursorBeatAndTime.Time;
		const Beat cursorBeat = cursorBeatAndTime.Beat;
		const Beat cursorBeatOnPlaybackStart = context.TimeToBeat(context.CursorTimeOnPlaybackStart);

		// HACK: Drawing outside the BeginChild() / EndChild() means the clipping rect has already been popped...
		DrawListSidebarHeader->PushClipRect(Regions.SidebarHeader.TL, Regions.SidebarHeader.BR);
		DrawListSidebar->PushClipRect(Regions.Sidebar.TL, Regions.Sidebar.BR);
		DrawListContentHeader->PushClipRect(Regions.ContentHeader.TL, Regions.ContentHeader.BR);
		DrawListContent->PushClipRect(Regions.Content.TL, Regions.Content.BR);
		defer { DrawListContent->PopClipRect(); DrawListContentHeader->PopClipRect(); DrawListSidebar->PopClipRect(); DrawListSidebarHeader->PopClipRect(); };

		context.ElapsedProgramTimeSincePlaybackStarted = isPlayback ? context.ElapsedProgramTimeSincePlaybackStarted + Time::FromSec(Gui::DeltaTime()) : Time::Zero();
		context.ElapsedProgramTimeSincePlaybackStopped = !isPlayback ? context.ElapsedProgramTimeSincePlaybackStopped + Time::FromSec(Gui::DeltaTime()) : Time::Zero();
		const f32 animatedCursorLocalSpaceX = Camera.WorldToLocalSpace(vec2(WorldSpaceCursorXAnimationCurrent, 0.0f)).x;
		const f32 currentCursorLocalSpaceX = Camera.TimeToLocalSpaceX(cursorTime);

		const f32 cursorLocalSpaceX = isPlayback ? currentCursorLocalSpaceX : animatedCursorLocalSpaceX;
		const f32 cursorHeaderTriangleLocalSpaceX = cursorLocalSpaceX + 0.5f;

		// NOTE: Base background
		{
			static constexpr f32 borders = 1.0f;
			DrawListContent->AddRectFilled(Regions.Content.TL + vec2(borders), Regions.Content.BR - vec2(borders), TimelineBackgroundColor);
		}

		// NOTE: Demo start time marker
		{
			if (context.Chart.SongDemoStartTime > Time::Zero())
			{
				const Time songDemoStartTimeChartSpace = context.Chart.SongDemoStartTime + context.Chart.SongOffset;
				const f32 songDemoStartTimeLocalSpaceX = Camera.TimeToLocalSpaceX(songDemoStartTimeChartSpace);

				const vec2 headerScreenSpaceTL = LocalToScreenSpace_ContentHeader(vec2(songDemoStartTimeLocalSpaceX, 0.0f));
				const vec2 triangle[3]
				{
					headerScreenSpaceTL + vec2(0.0f, Regions.ContentHeader.GetHeight() - GuiScale(TimelineSongDemoStartMarkerHeight)),
					headerScreenSpaceTL + vec2(GuiScale(TimelineSongDemoStartMarkerWidth), Regions.ContentHeader.GetHeight()),
					headerScreenSpaceTL + vec2(0.0f, Regions.ContentHeader.GetHeight()),
				};

				DrawListContentHeader->AddTriangleFilled(triangle[0], triangle[1], triangle[2], TimelineSongDemoStartMarkerColorFill);
				DrawListContentHeader->AddTriangle(triangle[0], triangle[1], triangle[2], TimelineSongDemoStartMarkerColorBorder);
			}
		}

		// NOTE: Tempo map bar/beat lines and bar-index/time text
		{
			const f32 screenSpaceTimeTextWidth = Gui::CalcTextSize(Time::Zero().ToString().Data).x;
			vec2 lastDrawnScreenSpaceTextTL = vec2(F32Min);

			const vec2 screenSpaceTextOffsetBarIndex = GuiScale(vec2(4.0f, 1.0f));
			const vec2 screenSpaceTextOffsetBarTime = GuiScale(vec2(4.0f, 13.0f));

			const b8 displayTimeInSongSpace = (*Settings.General.DisplayTimeInSongSpace && Absolute(context.Chart.SongOffset.ToMS()) > 0.5);
			const Time timeLabelDisplayOffset = displayTimeInSongSpace ? -context.Chart.SongOffset : Time::Zero();

			Gui::PushFont(FontMedium_EN);
			const Time visibleTimeOverdraw = Camera.TimePerScreenPixel() * (Gui::CalcTextSize("00:00.000").x + Gui::GetFrameHeight());
			ForEachTimelineVisibleGridLine(*this, context, visibleTimeOverdraw, [&](const ForEachGridLineData& gridIt)
			{
				const u32 lineColor = gridIt.IsBar ? TimelineGridBarLineColor : TimelineGridBeatLineColor;

				const vec2 screenSpaceTL = LocalToScreenSpace(vec2(Camera.TimeToLocalSpaceX(gridIt.Time), 0.0f));
				DrawListContent->AddLine(screenSpaceTL, screenSpaceTL + vec2(0.0f, Regions.Content.GetHeight()), lineColor);

				const vec2 headerScreenSpaceTL = LocalToScreenSpace_ContentHeader(vec2(Camera.TimeToLocalSpaceX(gridIt.Time), 0.0f));
				DrawListContentHeader->AddLine(headerScreenSpaceTL, headerScreenSpaceTL + vec2(0.0f, Regions.ContentHeader.GetHeight()), lineColor);

				// HACK: Try to prevent overlapping text for the very last grid line at least
				//if (!ApproxmiatelySame(gridIt.Time.Seconds, context.GetDurationOrDefault().Seconds) && (lastDrawnScreenSpaceTextTL.x + screenSpaceTimeTextWidth) > headerScreenSpaceTL.x)
				{
					// TODO: Fix overlapping text when zoomed out really far (while still keeping the grid lines)
					if (gridIt.IsBar)
					{
						Gui::DisableFontPixelSnap(true);

						char buffer[32];
						Gui::AddTextWithDropShadow(DrawListContentHeader, headerScreenSpaceTL + screenSpaceTextOffsetBarIndex, Gui::GetColorU32(ImGuiCol_Text),
							std::string_view(buffer, sprintf_s(buffer, "%d", gridIt.BarIndex)));
						Gui::AddTextWithDropShadow(DrawListContentHeader, headerScreenSpaceTL + screenSpaceTextOffsetBarTime, Gui::GetColorU32(ImGuiCol_Text, 0.5f),
							(gridIt.Time + timeLabelDisplayOffset).ToString().Data);

						lastDrawnScreenSpaceTextTL = headerScreenSpaceTL;
						Gui::DisableFontPixelSnap(false);
					}
				}
			});
			Gui::PopFont();
		}

		// NOTE: Grid snap beat lines
		{
			if (GridSnapLineAnimationCurrent > 0.0f)
			{
				const auto minMaxVisibleTime = GetMinMaxVisibleTime();
				const Beat gridBeatSnap = GetGridBeatSnap(CurrentGridBarDivision);
				const Beat minVisibleBeat = FloorBeatToGrid(context.TimeToBeat(minMaxVisibleTime.Min), gridBeatSnap) - gridBeatSnap;
				const Beat maxVisibleBeat = CeilBeatToGrid(context.TimeToBeat(minMaxVisibleTime.Max), gridBeatSnap) + gridBeatSnap;

				const u32 gridSnapLineColor = Gui::ColorU32WithAlpha(IsTupletBarDivision(CurrentGridBarDivision) ? TimelineGridSnapTupletLineColor : TimelineGridSnapLineColor, GridSnapLineAnimationCurrent);
				for (Beat beatIt = ClampBot(minVisibleBeat, Beat::Zero()); beatIt <= ClampTop(maxVisibleBeat, context.TimeToBeat(context.Chart.GetDurationOrDefault())); beatIt += gridBeatSnap)
				{
					const vec2 screenSpaceTL = LocalToScreenSpace(vec2(Camera.TimeToLocalSpaceX(context.BeatToTime(beatIt)), 0.0f));
					DrawListContent->AddLine(screenSpaceTL, screenSpaceTL + vec2(0.0f, Regions.Content.GetHeight()), gridSnapLineColor);
				}
			}
		}

		// NOTE: Out of bounds darkening
		{
			const Time gridTimeMin = ClampTop(context.Chart.SongOffset, Time::Zero()), gridTimeMax = context.Chart.GetDurationOrDefault();
			const f32 localSpaceMinL = Clamp(Camera.WorldToLocalSpace(vec2(Camera.TimeToWorldSpaceX(gridTimeMin), 0.0f)).x, 0.0f, Regions.Content.GetWidth());
			const f32 localSpaceMaxR = Clamp(Camera.WorldToLocalSpace(vec2(Camera.TimeToWorldSpaceX(gridTimeMax), 0.0f)).x, 0.0f, Regions.Content.GetWidth());

			const Rect screenSpaceRectL = Rect(Regions.Content.TL, vec2(LocalToScreenSpace(vec2(localSpaceMinL, 0.0f)).x, Regions.Content.BR.y));
			const Rect screenSpaceRectR = Rect(vec2(LocalToScreenSpace(vec2(localSpaceMaxR, 0.0f)).x, Regions.Content.TL.y), Regions.Content.BR);

			if (screenSpaceRectL.GetWidth() > 0.0f) DrawListContent->AddRectFilled(screenSpaceRectL.TL, screenSpaceRectL.BR, TimelineOutOfBoundsDimColor);
			if (screenSpaceRectR.GetWidth() > 0.0f) DrawListContent->AddRectFilled(screenSpaceRectR.TL, screenSpaceRectR.BR, TimelineOutOfBoundsDimColor);
		}

		// NOTE: Range selection rect
		if (RangeSelection.IsActive)
		{
			vec2 localTL = vec2(Camera.TimeToLocalSpaceX(context.BeatToTime(RangeSelection.Start)), 1.0f);
			vec2 localBR = vec2(Camera.TimeToLocalSpaceX(context.BeatToTime(RangeSelection.End)), Regions.Content.GetHeight() - 1.0f);
			localBR.x = LerpClamped(localTL.x, localBR.x, RangeSelectionExpansionAnimationCurrent);
			const vec2 screenSpaceTL = LocalToScreenSpace(localTL) + vec2(!RangeSelection.HasEnd ? -1.0f : 0.0f, 0.0f);
			const vec2 screenSpaceBR = LocalToScreenSpace(localBR) + vec2(!RangeSelection.HasEnd ? +1.0f : 0.0f, 0.0f);

			if ((TimelineRangeSelectionHeaderHighlightColor & IM_COL32_A_MASK) > 0)
			{
				const f32 headerHighlightRectHeight = GuiScale(3.0f);
				const Rect headerHighlightRect = Rect(
					LocalToScreenSpace_ContentHeader(vec2(localTL.x, Regions.ContentHeader.GetHeight() - headerHighlightRectHeight)),
					LocalToScreenSpace_ContentHeader(vec2(localBR.x, Regions.ContentHeader.GetHeight() + 2.0f)));

				DrawListContentHeader->AddRectFilled(headerHighlightRect.TL, headerHighlightRect.BR, TimelineRangeSelectionHeaderHighlightColor);
				DrawListContentHeader->AddRect(headerHighlightRect.TL, headerHighlightRect.BR, TimelineRangeSelectionBorderColor);
			}

			DrawListContent->AddRectFilled(screenSpaceTL, screenSpaceBR, TimelineRangeSelectionBackgroundColor);
			DrawListContent->AddRect(screenSpaceTL, screenSpaceBR, TimelineRangeSelectionBorderColor);
		}

		// NOTE: Background waveform
		if (TimelineWaveformDrawOrder == WaveformDrawOrder::Background && !context.SongWaveformL.IsEmpty())
			DrawTimelineContentWaveform(*this, DrawListContent, context.Chart.SongOffset, context.SongWaveformL, context.SongWaveformR, context.SongWaveformFadeAnimationCurrent);

		// NOTE: Row labels, lines and items
		{
			const Time visibleTimeOverdraw = Camera.TimePerScreenPixel() * (Gui::GetFrameHeight() * 4.0f);
			const DrawTimelineContentItemRowParam rowParam = { *this, context, DrawListContent, GetMinMaxVisibleTime(visibleTimeOverdraw), isPlayback, cursorTime, cursorBeatOnPlaybackStart };
			Gui::PushFont(FontMedium_EN);
			ForEachTimelineRow(*this, [&](const ForEachRowData& rowIt)
			{
				// NOTE: Row label text
				{
					const vec2 sidebarScreenSpaceTL = LocalToScreenSpace_Sidebar(vec2(0.0f, rowIt.LocalY));
					const vec2 sidebarScreenSpaceBL = LocalToScreenSpace_Sidebar(vec2(0.0f, rowIt.LocalY + rowIt.LocalHeight));

					DrawListSidebar->AddLine(
						sidebarScreenSpaceBL,
						sidebarScreenSpaceBL + vec2(Regions.Sidebar.GetWidth(), 0.0f),
						Gui::GetColorU32(ImGuiCol_Separator, 0.35f));

					const f32 textHeight = Gui::GetFontSize();
					const vec2 screenSpaceTextPosition = sidebarScreenSpaceTL + vec2((Gui::GetStyle().FramePadding.x * 2.0f), Floor((rowIt.LocalHeight * 0.5f) - (textHeight * 0.5f)));

					// HACK: Use TextDisable for now to make it clear that these aren't really implemented yet
					const b8 isThisRowImplemented = !(rowIt.RowType == TimelineRowType::Notes_Expert || rowIt.RowType == TimelineRowType::Notes_Master);

					Gui::DisableFontPixelSnap(true);
					DrawListSidebar->AddText(screenSpaceTextPosition, Gui::GetColorU32(isThisRowImplemented ? ImGuiCol_Text : ImGuiCol_TextDisabled), Gui::StringViewStart(rowIt.Label), Gui::StringViewEnd(rowIt.Label));
					Gui::DisableFontPixelSnap(false);
				}

				// NOTE: Row separator line
				{
					const vec2 screenSpaceBL = LocalToScreenSpace(vec2(0.0f, rowIt.LocalY + rowIt.LocalHeight));
					DrawListContent->AddLine(screenSpaceBL, screenSpaceBL + vec2(Regions.Content.GetWidth(), 0.0f), TimelineHorizontalRowLineColor);
				}

				// NOTE: Row items
				switch (rowIt.RowType)
				{
				case TimelineRowType::Tempo: DrawTimelineContentItemRowT<TempoChange, TimelineRowType::Tempo>(rowParam, rowIt, context.ChartSelectedCourse->TempoMap.Tempo); break;
				case TimelineRowType::TimeSignature: DrawTimelineContentItemRowT<TimeSignatureChange, TimelineRowType::TimeSignature>(rowParam, rowIt, context.ChartSelectedCourse->TempoMap.Signature); break;
				case TimelineRowType::Notes_Normal: DrawTimelineContentItemRowT<Note, TimelineRowType::Notes_Normal>(rowParam, rowIt, context.ChartSelectedCourse->Notes_Normal); break;
				case TimelineRowType::Notes_Expert: DrawTimelineContentItemRowT<Note, TimelineRowType::Notes_Expert>(rowParam, rowIt, context.ChartSelectedCourse->Notes_Expert); break;
				case TimelineRowType::Notes_Master: DrawTimelineContentItemRowT<Note, TimelineRowType::Notes_Master>(rowParam, rowIt, context.ChartSelectedCourse->Notes_Master); break;
				case TimelineRowType::ScrollSpeed: DrawTimelineContentItemRowT<ScrollChange, TimelineRowType::ScrollSpeed>(rowParam, rowIt, context.ChartSelectedCourse->ScrollChanges); break;
				case TimelineRowType::BarLineVisibility: DrawTimelineContentItemRowT<BarLineChange, TimelineRowType::BarLineVisibility>(rowParam, rowIt, context.ChartSelectedCourse->BarLineChanges); break;
				case TimelineRowType::GoGoTime: DrawTimelineContentItemRowT<GoGoRange, TimelineRowType::GoGoTime>(rowParam, rowIt, context.ChartSelectedCourse->GoGoRanges); break;
				case TimelineRowType::Lyrics: DrawTimelineContentItemRowT<LyricChange, TimelineRowType::Lyrics>(rowParam, rowIt, context.ChartSelectedCourse->Lyrics); break;
				default: { assert(!"Missing TimelineRowType switch case"); } break;
				}
			});
			Gui::PopFont();
		}

		// NOTE: Background waveform overlay
		if (TimelineWaveformDrawOrder == WaveformDrawOrder::Foreground && !context.SongWaveformL.IsEmpty())
			DrawTimelineContentWaveform(*this, DrawListContent, context.Chart.SongOffset, context.SongWaveformL, context.SongWaveformR, context.SongWaveformFadeAnimationCurrent);

		// NOTE: Cursor foreground
		{
			const f32 cursorWidth = GuiScale(TimelineCursorHeadWidth);
			const f32 cursorHeight = GuiScale(TimelineCursorHeadHeight);

			// TODO: Maybe animate between triangle and something else depending on isPlayback (?)
			DrawListContentHeader->AddTriangleFilled(
				LocalToScreenSpace_ContentHeader(vec2(cursorHeaderTriangleLocalSpaceX - cursorWidth, 0.0f)),
				LocalToScreenSpace_ContentHeader(vec2(cursorHeaderTriangleLocalSpaceX + cursorWidth, 0.0f)),
				LocalToScreenSpace_ContentHeader(vec2(cursorHeaderTriangleLocalSpaceX, cursorHeight)), TimelineCursorColor);
			DrawListContentHeader->AddLine(
				LocalToScreenSpace_ContentHeader(vec2(cursorLocalSpaceX, cursorHeight)),
				LocalToScreenSpace_ContentHeader(vec2(cursorLocalSpaceX, Regions.ContentHeader.GetHeight())), TimelineCursorColor);

			// TODO: Maybe draw different cursor color or "type" depending on grid snap (?) (see "ArrowVortex/assets/icons snap.png")
			DrawListContent->AddLine(
				LocalToScreenSpace(vec2(cursorLocalSpaceX, 0.0f)),
				LocalToScreenSpace(vec2(cursorLocalSpaceX, Regions.Content.GetHeight())), TimelineCursorColor);

			// NOTE: Immediately draw destination cursor line for better responsiveness
			if (!isPlayback && !ApproxmiatelySame(animatedCursorLocalSpaceX, currentCursorLocalSpaceX, 0.5f))
			{
				DrawListContent->AddLine(
					LocalToScreenSpace(vec2(currentCursorLocalSpaceX, 0.0f)),
					LocalToScreenSpace(vec2(currentCursorLocalSpaceX, Regions.Content.GetHeight())), Gui::ColorU32WithAlpha(TimelineCursorColor, 0.25f));
			}
		}

		// NOTE: Mouse selection box
		if (BoxSelection.IsActive)
		{
			auto clampVisibleScreenSpace = [&](vec2 screenSpace) { return vec2(screenSpace.x, Clamp(screenSpace.y, Regions.Content.TL.y + 2.0f, Regions.Content.BR.y - 2.0f)); };

			DrawListContent->AddRectFilled(
				clampVisibleScreenSpace(LocalToScreenSpace(Camera.WorldToLocalSpace(BoxSelection.WorldSpaceRect.TL))),
				clampVisibleScreenSpace(LocalToScreenSpace(Camera.WorldToLocalSpace(BoxSelection.WorldSpaceRect.BR))), TimelineBoxSelectionBackgroundColor);
			DrawListContent->AddRect(
				clampVisibleScreenSpace(LocalToScreenSpace(Camera.WorldToLocalSpace(BoxSelection.WorldSpaceRect.TL))),
				clampVisibleScreenSpace(LocalToScreenSpace(Camera.WorldToLocalSpace(BoxSelection.WorldSpaceRect.BR))), TimelineBoxSelectionBorderColor);

			if (BoxSelection.Action != BoxSelectionAction::Clear)
			{
				const f32 radius = GuiScale(TimelineBoxSelectionRadius);
				const f32 linePadding = GuiScale(TimelineBoxSelectionLinePadding);
				const f32 lineThickness = ClampBot(Round(TimelineBoxSelectionLineThickness * GuiScaleFactorCurrent / 2.0f) * 2.0f, TimelineBoxSelectionLineThickness);

				const vec2 center = LocalToScreenSpace(Camera.WorldToLocalSpace(BoxSelection.WorldSpaceRect.TL));
				DrawListContent->AddCircleFilled(center, radius, TimelineBoxSelectionFillColor);
				DrawListContent->AddCircle(center, radius, TimelineBoxSelectionBorderColor);
				{
					const Rect horizontal = Rect::FromCenterSize(center, vec2((radius - linePadding) * 2.0f, lineThickness));
					const Rect vertical = Rect::FromCenterSize(center, vec2(lineThickness, (radius - linePadding) * 2.0f));

					if (BoxSelection.Action == BoxSelectionAction::Add)
					{
						DrawListContent->AddRectFilled(horizontal.TL, horizontal.BR, TimelineBoxSelectionInnerColor);
						DrawListContent->AddRectFilled(vertical.TL, vertical.BR, TimelineBoxSelectionInnerColor);
					}
					else if (BoxSelection.Action == BoxSelectionAction::Sub)
					{
						DrawListContent->AddRectFilled(horizontal.TL, horizontal.BR, TimelineBoxSelectionInnerColor);
					}
					else if (BoxSelection.Action == BoxSelectionAction::XOR)
					{
						DrawListContent->AddCircleFilled(center, GuiScale(TimelineBoxSelectionXorDotRadius), TimelineBoxSelectionInnerColor);
					}
				}
			}
		}
	}
}
