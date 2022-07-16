#include "chart_timeline.h"
#include "chart_undo_commands.h"
#include "chart_draw_common.h"

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
			struct NamedColorU32Pointer { const char* Label; u32* ColorPtr; u32 Default; };
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
				NamedColorU32Pointer {},
				{ "Timeline GoGo Background Border", &TimelineGoGoBackgroundColorBorder },
				{ "Timeline GoGo Background Outer", &TimelineGoGoBackgroundColorOuter },
				{ "Timeline GoGo Background Inner", &TimelineGoGoBackgroundColorInner },
				NamedColorU32Pointer {},
				{ "Timeline Lyrics Text", &TimelineLyricsTextColor },
				{ "Timeline Lyrics Text (Shadow)", &TimelineLyricsTextColorShadow },
				{ "Timeline Lyrics Background Border", &TimelineLyricsBackgroundColorBorder },
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
				{ "Note Balloon Text", &NoteBalloonTextColor },
				{ "Note Balloon Text (Shadow)", &NoteBalloonTextColorShadow },
				NamedColorU32Pointer {},
				{ "Drag Text (Hovered)", &DragTextHoveredColor },
				{ "Drag Text (Active)", &DragTextActiveColor },
			};

			if (static bool firstFrame = true; firstFrame) { for (auto& e : namedColors) { e.Default = (e.ColorPtr != nullptr) ? *e.ColorPtr : 0xFFFF00FF; } firstFrame = false; }

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
			struct NamedVariablePtr { const char* Label; f32* ValuePtr; f32 Default; };
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

			if (static bool firstFrame = true; firstFrame) { for (auto& e : namedVariables) { e.Default = (e.ValuePtr != nullptr) ? *e.ValuePtr : 0.0f; } firstFrame = false; }

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

		if (Gui::CollapsingHeader("User Settings", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (Gui::Property::BeginTable(tableFlags))
			{
				static constexpr auto animationSpeed = [](const char* label, f32* inOutSpeed)
				{
					Gui::Property::PropertyTextValueFunc(label, [&] { Gui::SetNextItemWidth(-1.0f); Gui::DragFloat(label, inOutSpeed, 0.1f, 0.0f, 100.0f); });
				};
				animationSpeed("TimelineSmoothScrollAnimationSpeed", &Settings.TimelineSmoothScrollAnimationSpeed.Value);
				animationSpeed("TimelineWaveformFadeAnimationSpeed", &Settings.TimelineWaveformFadeAnimationSpeed.Value);
				animationSpeed("TimelineRangeSelectionExpansionAnimationSpeed", &Settings.TimelineRangeSelectionExpansionAnimationSpeed.Value);
				animationSpeed("TimelineWorldSpaceCursorXAnimationSpeed", &Settings.TimelineWorldSpaceCursorXAnimationSpeed.Value);
				animationSpeed("TimelineGridSnapLineAnimationSpeed", &Settings.TimelineGridSnapLineAnimationSpeed.Value);
				animationSpeed("TimelineGoGoRangeExpansionAnimationSpeed", &Settings.TimelineGoGoRangeExpansionAnimationSpeed.Value);
				Gui::Property::EndTable();
			}
		}

		if (Gui::CollapsingHeader("Highlight Region"))
		{
			auto region = [](const char* name, Rect region) { Gui::Selectable(name); if (Gui::IsItemHovered()) { Gui::GetForegroundDrawList()->AddRect(region.TL, region.BR, ImColor(1.0f, 0.0f, 1.0f)); } };
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
		for (size_t i = 0; i < EnumCount<TimelineRowType>; i++)
		{
			const auto rowType = static_cast<TimelineRowType>(i);
			const f32 localHeight = GuiScale((rowType >= TimelineRowType::NoteBranches_First && rowType <= TimelineRowType::NoteBranches_Last) ? TimelineRowHeightNotes : TimelineRowHeight);

			perRowFunc(ForEachRowData { rowType, localY, localHeight, TimelineRowTypeNames[i] });
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
		bool IsBar;
	};

	template <typename Func>
	static void ForEachTimelineVisibleGridLine(ChartTimeline& timeline, ChartContext& context, Func perGridFunc)
	{
		// TODO: Rewrite all of this to correctly handle tempo map changes and take GuiScaleFactor text size into account
		/* // TEMP: */ static constexpr Time GridTimeStep = Time::FromSeconds(1.0);
		if (GridTimeStep.Seconds <= 0.0)
			return;

		// TODO: Not too sure about the exact values here, these just came from randomly trying out numbers until it looked about right
		static constexpr f32 minAllowedSpacing = 128.0f;
		static constexpr i32 maxSubDivisions = 6;

		// TODO: Go the extra step and add a fade animation for these too
		i32 gridLineSubDivisions = 0;
		for (gridLineSubDivisions = 0; gridLineSubDivisions < maxSubDivisions; gridLineSubDivisions++)
		{
			const f32 localSpaceXPerGridLine = timeline.Camera.WorldToLocalSpaceScale(vec2(timeline.Camera.TimeToWorldSpaceX(GridTimeStep), 0.0f)).x;
			const f32 localSpaceXPerGridLineSubDivided = localSpaceXPerGridLine * static_cast<f32>(gridLineSubDivisions + 1);

			if (localSpaceXPerGridLineSubDivided >= (minAllowedSpacing / (gridLineSubDivisions + 1)))
				break;
		}

		const auto minMaxVisibleTime = timeline.GetMinMaxVisibleTime(Time::FromSeconds(1.0));
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

	struct PlaybackSpeedsArrayView
	{
		i32 Count; const f32* V;
		template <size_t Size>
		constexpr PlaybackSpeedsArrayView(const f32(&v)[Size]) : Count(static_cast<i32>(Size)), V(v) {}
		constexpr const f32* begin() const { return V; }
		constexpr const f32* end() const { return V + Count; }
	};

	// TODO: Maybe make user configurable (?)
	static constexpr f32 PresetPlaybackSpeedsFine[] = { 1.0f, 0.95f, 0.9f, 0.85f, 0.8f, 0.75f, 0.7f, 0.65f, 0.6f, 0.55f, 0.5f, 0.45f, 0.4f, 0.35f, 0.30f, 0.25f, 0.2f };
	static constexpr f32 PresetPlaybackSpeedsRough[] = { 1.0f, 0.75f, 0.5f, 0.25f };
	static constexpr f32 PresetPlaybackSpeeds[] = { 1.0f, 0.9f, 0.8f, 0.7f, 0.6f, 0.5f, 0.4f, 0.3f, 0.2f };
	static constexpr PlaybackSpeedsArrayView PresetPlaybackSpeedsFine_View = PresetPlaybackSpeedsFine;
	static constexpr PlaybackSpeedsArrayView PresetPlaybackSpeedsRough_View = PresetPlaybackSpeedsRough;
	static constexpr PlaybackSpeedsArrayView PresetPlaybackSpeeds_View = PresetPlaybackSpeeds;

	static constexpr f32 NoteHitAnimationDuration = 0.06f;
	static constexpr f32 NoteHitAnimationScaleStart = 1.35f, NoteHitAnimationScaleEnd = 1.0f;
	static constexpr f32 NoteDeleteAnimationDuration = 0.04f;

	static constexpr SoundEffectType SoundEffectTypeForNoteType(NoteType noteType)
	{
		return IsKaNote(noteType) ? SoundEffectType::TaikoKa : SoundEffectType::TaikoDon;
	}

	static bool IsTimelineCursorVisibleOnScreen(const TimelineCamera& camera, const TimelineRegions& regions, const Time cursorTime, const f32 edgePixelThreshold = 0.0f)
	{
		assert(edgePixelThreshold >= 0.0f);
		const f32 cursorLocalSpaceX = camera.TimeToLocalSpaceX(cursorTime);
		return (cursorLocalSpaceX >= edgePixelThreshold && cursorLocalSpaceX <= ClampBot(regions.Content.GetWidth() - edgePixelThreshold, edgePixelThreshold));
	}

	static void SetNotesWaveAnimationTimes(std::vector<Note>& notesToAnimate, i32 direction = +1)
	{
		assert(direction != 0);
		for (i32 i = 0; i < static_cast<i32>(notesToAnimate.size()); i++)
		{
			// TODO: Proper "wave" placement animation (also maybe scale by beat instead of index?)
			const i32 animationIndex = (direction > 0) ? i : (static_cast<i32>(notesToAnimate.size()) - i);
			const f32 noteCountAnimationFactor = (1.0f / static_cast<i32>(notesToAnimate.size())) * 2.0f;
			notesToAnimate[i].ClickAnimationTimeRemaining = NoteHitAnimationDuration + (animationIndex * (NoteHitAnimationDuration * /*0.08f **/ noteCountAnimationFactor));
			notesToAnimate[i].ClickAnimationTimeDuration = notesToAnimate[i].ClickAnimationTimeRemaining;
		}
	}

	static f32 GetTimelineNoteScaleFactor(bool isPlayback, Time cursorTime, Beat cursorBeatOnPlaybackStart, const Note& note, Time noteTime)
	{
		// TODO: Handle AnimationTime > AnimationDuration differently so that range selected multi note placement can have a nice "wave propagation" effect
		if (note.ClickAnimationTimeRemaining > 0.0f)
			return ConvertRange<f32>(note.ClickAnimationTimeDuration, 0.0f, NoteHitAnimationScaleStart, NoteHitAnimationScaleEnd, note.ClickAnimationTimeRemaining);

		if (isPlayback && note.BeatTime >= cursorBeatOnPlaybackStart)
		{
			// TODO: Rewirte cleanup using ConvertRange
			const f32 timeUntilNote = static_cast<f32>((noteTime - cursorTime).TotalSeconds());
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

	static constexpr f32 TimeToScrollbarLocalSpaceX(Time time, const TimelineRegions& regions, Time chartDuration)
	{
		return static_cast<f32>(ConvertRange<f64>(0.0, chartDuration.TotalSeconds(), 0.0, regions.ContentScrollbarX.GetWidth(), time.TotalSeconds()));
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

		const Time waveformTimePerPixel = Time::FromSeconds(chartDuration.TotalSeconds() / ClampBot(timeline.Regions.ContentScrollbarX.GetWidth(), 1.0f));
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

	static void UpdateTimelinePlaybackAndMetronomneSounds(ChartContext& context, bool playbackSoundsEnabled, ChartTimeline::MetronomeData& metronome)
	{
		static constexpr Time frameTimeThresholdAtWhichPlayingSoundsMakesNoSense = Time::FromMilliseconds(250.0);
		static constexpr Time playbackSoundFutureOffset = Time::FromSeconds(1.0 / 25.0);

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
						for (Beat subBeat = Beat::Zero(); subBeat <= note.BeatDuration; subBeat += *Settings.DrumrollHitBeatInterval)
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
				metronome.LastPlayedBeatTime = Time::FromSeconds(F64Min);
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
		// BUG: Freaks out when zoomed in too far / the timeline is too long (16min song for example) float precision issue (?)
		Camera.UpdateAnimations();
		Gui::AnimateExponential(&context.SongWaveformFadeAnimationCurrent, context.SongWaveformFadeAnimationTarget, *Settings.TimelineWaveformFadeAnimationSpeed);
		Gui::AnimateExponential(&RangeSelectionExpansionAnimationCurrent, RangeSelectionExpansionAnimationTarget, *Settings.TimelineRangeSelectionExpansionAnimationSpeed);

		const f32 worldSpaceCursorXAnimationTarget = Camera.TimeToWorldSpaceX(context.GetCursorTime());
		Gui::AnimateExponential(&WorldSpaceCursorXAnimationCurrent, worldSpaceCursorXAnimationTarget, *Settings.TimelineWorldSpaceCursorXAnimationSpeed);

		const f32 elapsedAnimationTimeSec = Gui::DeltaTime();
		for (size_t i = 0; i < EnumCount<BranchType>; i++)
		{
			for (Note& note : context.ChartSelectedCourse->GetNotes(static_cast<BranchType>(i)))
				note.ClickAnimationTimeRemaining = ClampBot(note.ClickAnimationTimeRemaining - elapsedAnimationTimeSec, 0.0f);
		}

		if (!TempDeletedNoteAnimationsBuffer.empty())
		{
			for (auto& data : TempDeletedNoteAnimationsBuffer)
				data.ElapsedTimeSec += Gui::DeltaTime();
			erase_remove_if(TempDeletedNoteAnimationsBuffer, [](auto& v) { return (v.ElapsedTimeSec >= NoteDeleteAnimationDuration); });
		}

		for (auto& gogo : context.ChartSelectedCourse->GoGoRanges)
			Gui::AnimateExponential(&gogo.ExpansionAnimationCurrent, gogo.ExpansionAnimationTarget, *Settings.TimelineGoGoRangeExpansionAnimationSpeed);

		UpdateInputAtStartOfFrame(context);

		// DEBUG:
		if (Gui::Begin("Chart Timeline - Debug")) { DrawTimelineDebugWindowContent(*this, context); } Gui::End();

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

		auto timelineRegionBegin = [](const Rect region, const char* name, bool padding = false)
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
						Gui::Button(ClampBot(context.GetCursorTime(), Time::Zero()).ToString().Data, perButtonSize);
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
					const bool isPlayback = context.GetIsPlayback();

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

	void ChartTimeline::PlayNoteSoundAndHitAnimationsAtBeat(ChartContext& context, Beat cursorBeat)
	{
		bool soundHasBeenPlayed = false;
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
		static constexpr const char* clipboardTextHeader = "// PeepoDrumKit Clipboard";
		static constexpr auto notesToClipboardText = [](const std::vector<Note>& notes, Beat baseBeat) -> std::string
		{
			std::string out; out.reserve(512); out += clipboardTextHeader; out += '\n';
			for (const Note& note : notes)
			{
				char buffer[256];
				out += std::string_view(buffer, sprintf_s(buffer,
					"Note { %d, %d, %d, %d };%c",
					(note.BeatTime - baseBeat).Ticks,
					note.BeatDuration.Ticks,
					static_cast<i32>(note.Type),
					note.BalloonPopCount,
					(&note != &notes.back()) ? '\n' : '\0'));
			}
			return out;
		};
		static constexpr auto notesFromClipboatdText = [](std::string_view clipboardText) -> std::vector<Note>
		{
			std::vector<Note> out;
			if (ASCII::StartsWith(clipboardText, clipboardTextHeader))
			{
				// TODO: Split and parse by ';' instead of '\n' (?)
				ASCII::ForEachLineInMultiLineString(clipboardText, false, [&](std::string_view line)
				{
					if (line.empty() || ASCII::StartsWith(line, "//"))
						return;

					line = ASCII::TrimSuffix(ASCII::TrimSuffix(line, "\r"), "\n");
					if (ASCII::StartsWith(line, "Note {") && ASCII::EndsWith(line, "};"))
					{
						Note& newNote = out.emplace_back();

						line = ASCII::Trim(line.substr(ArrayCount("Note {") - 1, line.size() - ArrayCount("Note {}")));
						ASCII::ForEachInCommaSeparatedList(line, [&, commaIndex = 0](std::string_view commaSeparated) mutable
						{
							if (commaSeparated = ASCII::Trim(commaSeparated); !commaSeparated.empty())
							{
								switch (commaIndex)
								{
								case 0: { if (i32 v {}; ASCII::TryParseI32(commaSeparated, v)) { newNote.BeatTime.Ticks = v; } } break;
								case 1: { if (i32 v {}; ASCII::TryParseI32(commaSeparated, v)) { newNote.BeatDuration.Ticks = v; } } break;
								case 2: { if (i32 v {}; ASCII::TryParseI32(commaSeparated, v)) { newNote.Type = static_cast<NoteType>(Clamp(v, 0, EnumCountI32<NoteType>-1)); } } break;
								case 3: { if (i32 v {}; ASCII::TryParseI32(commaSeparated, v)) { newNote.BalloonPopCount = v; } } break;
								}
							}
							commaIndex++;
						});
					}
				});
			}
			return out;
		};
		static constexpr auto copyAllSelectedNotes = [](const SortedNotesList& selectedBranchNotes) -> std::vector<Note>
		{
			i32 selectionCount = 0; for (const Note& note : selectedBranchNotes) { if (note.IsSelected) selectionCount++; }
			std::vector<Note> out;
			if (selectionCount > 0)
			{
				out.reserve(selectionCount);
				for (const Note& note : selectedBranchNotes) { if (note.IsSelected) out.push_back(note); }
			}
			return out;
		};

		auto& selectedBranchNotes = context.ChartSelectedCourse->GetNotes(context.ChartSelectedBranch);
		switch (action)
		{
		case ClipboardAction::Cut:
		{
			if (auto selectedNotes = copyAllSelectedNotes(selectedBranchNotes); !selectedNotes.empty())
			{
				for (const Note& note : selectedNotes) TempDeletedNoteAnimationsBuffer.push_back(
					DeletedNoteAnimation { note, context.ChartSelectedBranch, ConvertRange(0.0f, static_cast<f32>(selectedNotes.size()), 0.0f, -0.08f, static_cast<f32>(ArrayItToIndexI32(&note, &selectedNotes[0]))) });

				Gui::SetClipboardText(notesToClipboardText(selectedNotes, selectedNotes[0].BeatTime).c_str());
				context.Undo.Execute<Commands::RemoveMultipleNotes_Cut>(&selectedBranchNotes, std::move(selectedNotes));
			}
		} break;
		case ClipboardAction::Copy:
		{
			if (auto selectedNotes = copyAllSelectedNotes(selectedBranchNotes); !selectedNotes.empty())
			{
				// TODO: Maybe also animate original notes being copied (?)
				Gui::SetClipboardText(notesToClipboardText(selectedNotes, selectedNotes[0].BeatTime).c_str());
			}
		} break;
		case ClipboardAction::Paste:
		{
			std::vector<Note> clipboardNotes = notesFromClipboatdText(Gui::GetClipboardTextView());
			if (!clipboardNotes.empty())
			{
				const Beat baseBeat = FloorBeatToCurrentGrid(context.GetCursorBeat()) - clipboardNotes[0].BeatTime;
				for (Note& note : clipboardNotes) { note.BeatTime += baseBeat; }

				auto noteAlreadyExistsOrBad = [&](const Note& t) { return selectedBranchNotes.TryFindOverlappingBeat(t.GetStart(), t.GetEnd()) != nullptr || (t.BeatTime < Beat::Zero()); };
				erase_remove_if(clipboardNotes, noteAlreadyExistsOrBad);

				if (!clipboardNotes.empty())
				{
					SetNotesWaveAnimationTimes(clipboardNotes, +1);
					context.SfxVoicePool.PlaySound(SoundEffectTypeForNoteType(clipboardNotes[0].Type));
					context.Undo.Execute<Commands::AddMultipleNotes_Paste>(&selectedBranchNotes, std::move(clipboardNotes));
				}
			}
		} break;
		case ClipboardAction::Delete:
		{
			if (auto selectedNotes = copyAllSelectedNotes(selectedBranchNotes); !selectedNotes.empty())
			{
				for (const Note& note : selectedNotes) { TempDeletedNoteAnimationsBuffer.push_back(DeletedNoteAnimation { note, context.ChartSelectedBranch, 0.0f }); }
				context.Undo.Execute<Commands::RemoveMultipleNotes>(&selectedBranchNotes, std::move(selectedNotes));
			}
		} break;
		default: { assert(false); } break;
		}
	}

	void ChartTimeline::UpdateInputAtStartOfFrame(ChartContext& context)
	{
		MousePosLastFrame = MousePosThisFrame;
		MousePosThisFrame = Gui::GetMousePos();

		// NOTE: Mouse scroll / zoom
		if (!ApproxmiatelySame(Gui::GetIO().MouseWheel, 0.0f))
		{
			// BUG: Can't scroll while holding Ctrl...
			const vec2 mouseScrollSpeed = vec2(Gui::GetIO().KeyShift ? 250.0f : 100.0f) * vec2(1.0f, 0.75f);
			const vec2 mouseZoomSpeed = vec2(1.2f, 1.0f);
			const vec2 newZoom = (Gui::GetIO().MouseWheel > 0.0f) ? (Camera.ZoomTarget * mouseZoomSpeed) : (Camera.ZoomTarget / mouseZoomSpeed);

			if (IsSidebarWindowHovered)
			{
#if 0 // NOTE: Not needed at the moment for small number of rows
				if (!Gui::GetIO().KeyAlt)
					Camera.PositionTarget.y -= (Gui::GetIO().MouseWheel * mouseScrollSpeed.y);
#endif
			}
			else if (IsContentHeaderWindowHovered || IsContentWindowHovered)
			{
				if (Gui::GetIO().KeyAlt)
					Camera.SetZoomTargetAroundLocalPivot(newZoom, ScreenToLocalSpace(Gui::GetMousePos()));
				else if (Gui::GetIO().KeyCtrl)
					Camera.PositionTarget.y -= (Gui::GetIO().MouseWheel * mouseScrollSpeed.y);
				else
					Camera.PositionTarget.x += (Gui::GetIO().MouseWheel * mouseScrollSpeed.x);
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
					const Time elapsedCursorTime = Time::FromSeconds(Gui::DeltaTime()) * context.GetPlaybackSpeed();
					const f32 cameraScrollIncrement = Camera.TimeToWorldSpaceX(elapsedCursorTime) * Camera.ZoomCurrent.x;
					Camera.PositionCurrent.x += cameraScrollIncrement;
					Camera.PositionTarget.x += cameraScrollIncrement;
				}
			}
		}

		// NOTE: Cursor controls
		{
			// NOTE: Selected notes mouse drag
			{
				auto& notes = context.ChartSelectedCourse->GetNotes(context.ChartSelectedBranch);
				size_t selectedNoteCount = 0; for (const Note& note : notes) { if (note.IsSelected) selectedNoteCount++; }

				if (SelectedItemDrag.IsActive && !Gui::IsMouseDown(ImGuiMouseButton_Left))
					SelectedItemDrag = {};

				SelectedItemDrag.IsHovering = false;
				SelectedItemDrag.MouseBeatLastFrame = SelectedItemDrag.MouseBeatThisFrame;
				SelectedItemDrag.MouseBeatThisFrame = FloorBeatToCurrentGrid(context.TimeToBeat(Camera.LocalSpaceXToTime(ScreenToLocalSpace(MousePosThisFrame).x)));

				if (selectedNoteCount > 0 && IsContentWindowHovered && !SelectedItemDrag.IsActive)
				{
					Rect localNotesRowRect = {};
					ForEachTimelineRow(*this, [&](const ForEachRowData& rowIt)
					{
						const BranchType branchForThisRow =
							(rowIt.RowType == TimelineRowType::Notes_Normal) ? BranchType::Normal :
							(rowIt.RowType == TimelineRowType::Notes_Expert) ? BranchType::Expert :
							(rowIt.RowType == TimelineRowType::Notes_Master) ? BranchType::Master : BranchType::Count;

						if (branchForThisRow == context.ChartSelectedBranch)
							localNotesRowRect = Rect(vec2(0.0f, rowIt.LocalY), vec2(Regions.Content.GetWidth(), rowIt.LocalY + rowIt.LocalHeight));
					});

					for (const Note& note : notes)
					{
						if (note.IsSelected)
						{
							const vec2 center = LocalToScreenSpace(vec2(Camera.TimeToLocalSpaceX(context.BeatToTime(note.BeatTime)), localNotesRowRect.GetCenter().y));
							const Rect hitbox = Rect::FromCenterSize(center, vec2(GuiScale(IsBigNote(note.Type) ? TimelineSelectedNoteHitBoxSizeBig : TimelineSelectedNoteHitBoxSizeSmall)));
							if (hitbox.Contains(MousePosThisFrame))
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
					static constexpr auto checkCanSelectedNotesBeMoved = [](const SortedNotesList& notes, Beat beatIncrement) -> bool
					{
						if (beatIncrement > Beat::Zero())
						{
							for (i32 i = 0; i < static_cast<i32>(notes.size()); i++)
							{
								const Note& thisNote = notes[i];
								const Note* nextNote = IndexOrNull(i + 1, notes);
								if (thisNote.IsSelected && nextNote != nullptr && !nextNote->IsSelected)
								{
									if (thisNote.GetEnd() + beatIncrement >= nextNote->GetStart())
										return false;
								}
							}
						}
						else
						{
							for (i32 i = static_cast<i32>(notes.size()) - 1; i >= 0; i--)
							{
								const Note& thisNote = notes[i];
								const Note* prevNote = IndexOrNull(i - 1, notes);
								if (thisNote.IsSelected)
								{
									if (thisNote.GetStart() + beatIncrement < Beat::Zero())
										return false;

									if (prevNote != nullptr && !prevNote->IsSelected)
									{
										if (thisNote.GetStart() + beatIncrement <= prevNote->GetEnd())
											return false;
									}
								}
							}
						}
						return true;
					};

					const Beat cursorBeat = context.GetCursorBeat();
					const Beat dragBeatIncrement = FloorBeatToCurrentGrid(SelectedItemDrag.BeatDistanceMovedSoFar);
					if (dragBeatIncrement != Beat::Zero() && checkCanSelectedNotesBeMoved(notes, dragBeatIncrement))
					{
						SelectedItemDrag.BeatDistanceMovedSoFar -= dragBeatIncrement;

						std::vector<Commands::ChangeMultipleNoteBeats::Data> noteBeatsToChange;
						noteBeatsToChange.reserve(selectedNoteCount);
						for (const Note& note : notes)
							if (note.IsSelected) { auto& data = noteBeatsToChange.emplace_back(); data.StableID = note.StableID; data.NewBeat = (note.BeatTime + dragBeatIncrement); }

						context.Undo.Execute<Commands::ChangeMultipleNoteBeats_MoveNotes>(&notes, std::move(noteBeatsToChange));

						for (const Note& note : notes)
							if (note.IsSelected) { if (note.BeatTime == cursorBeat) PlayNoteSoundAndHitAnimationsAtBeat(context, note.BeatTime); }
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
					// const bool clickedForward = (newCursorTime >= oldCursorTime);

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

					const f32 threshold = *Settings.TimelineScrubAutoScrollPixelThreshold;
					const f32 speedMin = *Settings.TimelineScrubAutoScrollSpeedMin, speedMax = *Settings.TimelineScrubAutoScrollSpeedMax;

					const f32 modifier = Gui::GetIO().KeyAlt ? 0.25f : Gui::GetIO().KeyShift ? 2.0f : 1.0f;
					if (const f32 left = threshold; mouseLocalSpaceX < left)
					{
						const f32 scrollIncrementThisFrame = ConvertRange(threshold, 0.0f, speedMin, speedMax, mouseLocalSpaceX) * modifier * Gui::DeltaTime();
						Camera.PositionCurrent.x -= scrollIncrementThisFrame;
						Camera.PositionTarget.x -= scrollIncrementThisFrame;
					}
					if (const f32 right = (Regions.ContentHeader.GetWidth() - threshold); mouseLocalSpaceX >= right)
					{
						const f32 scrollIncrementThisFrame = ConvertRange(0.0f, threshold, speedMin, speedMax, mouseLocalSpaceX - right) * modifier * Gui::DeltaTime();
						Camera.PositionCurrent.x += scrollIncrementThisFrame;
						Camera.PositionTarget.x += scrollIncrementThisFrame;
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

				if (Gui::IsAnyPressed(*Settings.Input.Timeline_StartEndRangeSelection, false))
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
			}

			// NOTE: Grid snap controls
			if (IsContentWindowHovered || HasKeyboardFocus())
			{
				i32 currentGridDivisionIndex = 0;
				for (const i32& it : AllowedGridBarDivisions) if (it == CurrentGridBarDivision) currentGridDivisionIndex = ArrayItToIndexI32(&it, &AllowedGridBarDivisions[0]);

				const bool increaseGrid = (IsContentWindowHovered && Gui::IsMouseClicked(ImGuiMouseButton_X2, true)) || (HasKeyboardFocus() && Gui::IsAnyPressed(*Settings.Input.Timeline_IncreaseGridDivision, true, InputModifierBehavior::Relaxed));
				const bool decreaseGrid = (IsContentWindowHovered && Gui::IsMouseClicked(ImGuiMouseButton_X1, true)) || (HasKeyboardFocus() && Gui::IsAnyPressed(*Settings.Input.Timeline_DecreaseGridDivision, true, InputModifierBehavior::Relaxed));
				if (increaseGrid) CurrentGridBarDivision = AllowedGridBarDivisions[Clamp(currentGridDivisionIndex + 1, 0, ArrayCountI32(AllowedGridBarDivisions) - 1)];
				if (decreaseGrid) CurrentGridBarDivision = AllowedGridBarDivisions[Clamp(currentGridDivisionIndex - 1, 0, ArrayCountI32(AllowedGridBarDivisions) - 1)];
			}

			// NOTE: Playback speed controls
			if (const auto& io = Gui::GetIO(); HasKeyboardFocus() && !io.KeyCtrl)
			{
				const auto speeds = io.KeyAlt ? PresetPlaybackSpeedsFine_View : io.KeyShift ? PresetPlaybackSpeedsRough_View : PresetPlaybackSpeeds_View;

				i32 closetPlaybackSpeedIndex = 0;
				const f32 currentPlaybackSpeed = context.GetPlaybackSpeed();
				for (const f32& it : speeds) if (it >= currentPlaybackSpeed) closetPlaybackSpeedIndex = ArrayItToIndexI32(&it, &speeds.V[0]);

				if (Gui::IsAnyPressed(*Settings.Input.Timeline_IncreasePlaybackSpeed, true, InputModifierBehavior::Relaxed)) context.SetPlaybackSpeed(speeds.V[Clamp(closetPlaybackSpeedIndex - 1, 0, speeds.Count - 1)]);
				if (Gui::IsAnyPressed(*Settings.Input.Timeline_DecreasePlaybackSpeed, true, InputModifierBehavior::Relaxed)) context.SetPlaybackSpeed(speeds.V[Clamp(closetPlaybackSpeedIndex + 1, 0, speeds.Count - 1)]);
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

					if (Gui::GetIO().KeyShift && RangeSelection.IsActive && RangeSelection.HasEnd)
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
							if (notes.TryFindOverlappingBeat(beatForThisNote) == nullptr)
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
						const bool isPlayback = context.GetIsPlayback();
						const Beat cursorBeat = isPlayback ? RoundBeatToCurrentGrid(context.GetCursorBeat()) : FloorBeatToCurrentGrid(context.GetCursorBeat());

						Note* existingNoteAtCursor = notes.TryFindOverlappingBeat(cursorBeat);
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
										context.Undo.Execute<Commands::ChangeSingleNoteType>(&notes, Commands::ChangeSingleNoteType::Data { existingNoteAtCursor->StableID, noteTypeToInsert });
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

			const bool activeFocusedAndHasLength = HasKeyboardFocus() && LongNotePlacement.IsActive && (LongNotePlacement.CursorBeatHead != LongNotePlacement.CursorBeatTail);
			if (PlaceBalloonBindingDownLastFrame && !PlaceBalloonBindingDownThisFrame) { if (activeFocusedAndHasLength) placeLongNoteOnBindingRelease(LongNotePlacement.NoteType); LongNotePlacement = {}; }
			if (PlaceDrumrollBindingDownLastFrame && !PlaceDrumrollBindingDownThisFrame) { if (activeFocusedAndHasLength) placeLongNoteOnBindingRelease(LongNotePlacement.NoteType); LongNotePlacement = {}; }
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
			static constexpr auto getBoxSelectionAction = [](const ImGuiIO& io) { return io.KeyShift ? BoxSelectionAction::Add : io.KeyAlt ? BoxSelectionAction::Remove : BoxSelectionAction::Clear; };

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
						Rect screenNotesRowRect = {};
						ForEachTimelineRow(*this, [&](const ForEachRowData& rowIt)
						{
							const BranchType branchForThisRow =
								(rowIt.RowType == TimelineRowType::Notes_Normal) ? BranchType::Normal :
								(rowIt.RowType == TimelineRowType::Notes_Expert) ? BranchType::Expert :
								(rowIt.RowType == TimelineRowType::Notes_Master) ? BranchType::Master : BranchType::Count;

							if (branchForThisRow == context.ChartSelectedBranch)
								screenNotesRowRect = Rect(LocalToScreenSpace(vec2(0.0f, rowIt.LocalY)), LocalToScreenSpace(vec2(Regions.Content.GetWidth(), rowIt.LocalY + rowIt.LocalHeight)));
						});

						const vec2 screenNotesRowCenter = screenNotesRowRect.GetCenter();
						const vec2 screenSelectionMin = LocalToScreenSpace(Camera.WorldToLocalSpace(BoxSelection.WorldSpaceRect.GetMin()));
						const vec2 screenSelectionMax = LocalToScreenSpace(Camera.WorldToLocalSpace(BoxSelection.WorldSpaceRect.GetMax()));
						const Beat selectionBeatMin = context.TimeToBeat(Camera.WorldSpaceXToTime(BoxSelection.WorldSpaceRect.GetMin().x));
						const Beat selectionBeatMax = context.TimeToBeat(Camera.WorldSpaceXToTime(BoxSelection.WorldSpaceRect.GetMax().x));
						auto isNoteInsideSelectionBox = [&](const Note& note)
						{
							return (note.BeatTime >= selectionBeatMin && note.BeatTime <= selectionBeatMax) && (screenNotesRowCenter.y >= screenSelectionMin.y && screenNotesRowCenter.y <= screenSelectionMax.y);
						};

						auto& notes = context.ChartSelectedCourse->GetNotes(context.ChartSelectedBranch);
						switch (BoxSelection.Action)
						{
						case BoxSelectionAction::Clear: { for (Note& n : notes) n.IsSelected = isNoteInsideSelectionBox(n); } break;
						case BoxSelectionAction::Add: { for (Note& n : notes) if (isNoteInsideSelectionBox(n)) n.IsSelected = true; } break;
						case BoxSelectionAction::Remove: { for (Note& n : notes) if (isNoteInsideSelectionBox(n)) n.IsSelected = false; } break;
						}
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

	void ChartTimeline::DrawAllAtEndOfFrame(ChartContext& context)
	{
		const bool isPlayback = context.GetIsPlayback();
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

		context.ElapsedProgramTimeSincePlaybackStarted = isPlayback ? context.ElapsedProgramTimeSincePlaybackStarted + Time::FromSeconds(Gui::DeltaTime()) : Time::Zero();
		context.ElapsedProgramTimeSincePlaybackStopped = !isPlayback ? context.ElapsedProgramTimeSincePlaybackStopped + Time::FromSeconds(Gui::DeltaTime()) : Time::Zero();
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

			Gui::PushFont(FontMedium_EN);
			ForEachTimelineVisibleGridLine(*this, context, [&](const ForEachGridLineData& gridIt)
			{
				const u32 lineColor = gridIt.IsBar ? TimelineGridBarLineColor : TimelineGridBeatLineColor;

				const vec2 screenSpaceTL = LocalToScreenSpace(vec2(Camera.TimeToLocalSpaceX(gridIt.Time), 0.0f));
				DrawListContent->AddLine(screenSpaceTL, screenSpaceTL + vec2(0.0f, Regions.Content.GetHeight()), lineColor);

				const vec2 headerScreenSpaceTL = LocalToScreenSpace_ContentHeader(vec2(Camera.TimeToLocalSpaceX(gridIt.Time), 0.0f));
				DrawListContentHeader->AddLine(headerScreenSpaceTL, headerScreenSpaceTL + vec2(0.0f, Regions.ContentHeader.GetHeight()), lineColor);

				// BUG: WHERE THE FUCK DID THE TIME / BAR LABELS GO
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
							gridIt.Time.ToString().Data);

						lastDrawnScreenSpaceTextTL = headerScreenSpaceTL;
						Gui::DisableFontPixelSnap(false);
					}
				}
			});
			Gui::PopFont();
		}

		// NOTE: Grid snap beat lines
		{
			static constexpr f32 maxZoomLevelAtWhichToFadeOutGridBeatSnapLines = 0.5f;
			const f32 gridBeatSnapLineAnimationTarget = (Camera.ZoomCurrent.x <= maxZoomLevelAtWhichToFadeOutGridBeatSnapLines) ? 0.0f : 1.0f;
			Gui::AnimateExponential(&GridSnapLineAnimationCurrent, gridBeatSnapLineAnimationTarget, *Settings.TimelineGridSnapLineAnimationSpeed);
			GridSnapLineAnimationCurrent = Clamp(GridSnapLineAnimationCurrent, 0.0f, 1.0f);

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
			Gui::PushFont(FontMedium_EN);
			const MinMaxTime visibleTime = GetMinMaxVisibleTime(Time::FromSeconds(1.0));
			ForEachTimelineRow(*this, [&](const ForEachRowData& rowIt)
			{
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
					const bool isThisRowImplemented = !(rowIt.RowType == TimelineRowType::Notes_Expert || rowIt.RowType == TimelineRowType::Notes_Master);

					Gui::DisableFontPixelSnap(true);
					DrawListSidebar->AddText(screenSpaceTextPosition, Gui::GetColorU32(isThisRowImplemented ? ImGuiCol_Text : ImGuiCol_TextDisabled), Gui::StringViewStart(rowIt.Label), Gui::StringViewEnd(rowIt.Label));
					Gui::DisableFontPixelSnap(false);
				}

				{
					const vec2 screenSpaceBL = LocalToScreenSpace(vec2(0.0f, rowIt.LocalY + rowIt.LocalHeight));
					DrawListContent->AddLine(screenSpaceBL, screenSpaceBL + vec2(Regions.Content.GetWidth(), 0.0f), TimelineHorizontalRowLineColor);

					static constexpr f32 compactFormatStringZoomLevelThreshold = 0.25f;
					const bool useCompactFormat = (Camera.ZoomTarget.x < compactFormatStringZoomLevelThreshold);

					// TODO: Do culling by first determining min/max visible beat times then use those to continue / break inside loop (although problematic with TimeOffset?)
					switch (rowIt.RowType)
					{
					case TimelineRowType::Tempo:
					{
						for (const auto& tempoChange : context.ChartSelectedCourse->TempoMap.TempoChanges)
						{
							const Time startTime = context.BeatToTime(tempoChange.Beat);
							if (startTime < visibleTime.Min || startTime > visibleTime.Max)
								continue;

							const vec2 localSpaceTL = vec2(Camera.TimeToLocalSpaceX(startTime), rowIt.LocalY);
							const vec2 localSpaceBL = localSpaceTL + vec2(0.0f, rowIt.LocalHeight);
							const vec2 localSpaceCenter = localSpaceTL + vec2(0.0f, rowIt.LocalHeight * 0.5f);

							const f32 textHeight = Gui::GetFontSize();
							const vec2 textPosition = LocalToScreenSpace(localSpaceCenter + vec2(3.0f, textHeight * -0.5f));

							Gui::DisableFontPixelSnap(true);
							char buffer[32]; const auto text = std::string_view(buffer, sprintf_s(buffer, useCompactFormat ? "%.0f BPM" : "%g BPM", tempoChange.Tempo.BPM));
							const vec2 textSize = Gui::CalcTextSize(text);
							DrawListContent->AddRectFilled(vec2(LocalToScreenSpace(localSpaceTL).x, textPosition.y), textPosition + textSize, TimelineBackgroundColor);

							DrawListContent->AddLine(LocalToScreenSpace(localSpaceTL + vec2(0.0f, 1.0f)), LocalToScreenSpace(localSpaceBL), TimelineTempoChangeLineColor);
							Gui::AddTextWithDropShadow(DrawListContent, textPosition, TimelineItemTextColor, text, TimelineItemTextColorShadow);
							Gui::DisableFontPixelSnap(false);
						}
					} break;

					case TimelineRowType::TimeSignature:
					{
						for (const auto& signatureChange : context.ChartSelectedCourse->TempoMap.SignatureChanges)
						{
							const Time startTime = context.BeatToTime(signatureChange.Beat);
							if (startTime < visibleTime.Min || startTime > visibleTime.Max)
								continue;

							const vec2 localSpaceTL = vec2(Camera.TimeToLocalSpaceX(startTime), rowIt.LocalY);
							const vec2 localSpaceBL = localSpaceTL + vec2(0.0f, rowIt.LocalHeight);
							const vec2 localSpaceCenter = localSpaceTL + vec2(0.0f, rowIt.LocalHeight * 0.5f);

							const f32 textHeight = Gui::GetFontSize();
							const vec2 textPosition = LocalToScreenSpace(localSpaceCenter + vec2(3.0f, textHeight * -0.5f));

							Gui::DisableFontPixelSnap(true);
							char buffer[32]; const auto text = std::string_view(buffer, sprintf_s(buffer, "%d/%d", signatureChange.Signature.Numerator, signatureChange.Signature.Denominator));
							const vec2 textSize = Gui::CalcTextSize(text);
							DrawListContent->AddRectFilled(vec2(LocalToScreenSpace(localSpaceTL).x, textPosition.y), textPosition + textSize, TimelineBackgroundColor);

							DrawListContent->AddLine(LocalToScreenSpace(localSpaceTL + vec2(0.0f, 1.0f)), LocalToScreenSpace(localSpaceBL), TimelineSignatureChangeLineColor);
							Gui::AddTextWithDropShadow(DrawListContent, textPosition, IsTimeSignatureSupported(signatureChange.Signature) ? TimelineItemTextColor : TimelineItemTextColorWarning, text, TimelineItemTextColorShadow);
							Gui::DisableFontPixelSnap(false);
						}
					} break;

					case TimelineRowType::Notes_Normal:
					case TimelineRowType::Notes_Expert:
					case TimelineRowType::Notes_Master:
					{
						// TODO: Draw unselected branch notes grayed and at a slightly smaller scale (also nicely animate between selecting different branched!)

						// TODO: It looks like there'll also have to be one scroll speed lane per branch type
						//		 which means the scroll speed change line should probably extend all to the way down to its corresponding note lane (?)
						const BranchType branchForThisRow =
							(rowIt.RowType == TimelineRowType::Notes_Normal) ? BranchType::Normal :
							(rowIt.RowType == TimelineRowType::Notes_Expert) ? BranchType::Expert : BranchType::Master;

						for (const Note& note : context.ChartSelectedCourse->GetNotes(branchForThisRow))
						{
							const Time startTime = context.BeatToTime(note.GetStart()) + note.TimeOffset;
							const Time endTime = (note.BeatDuration > Beat::Zero()) ? context.BeatToTime(note.GetEnd()) + note.TimeOffset : startTime;
							if (endTime < visibleTime.Min || startTime > visibleTime.Max)
								continue;

							const vec2 localTL = vec2(Camera.TimeToLocalSpaceX(startTime), rowIt.LocalY);
							const vec2 localCenter = localTL + vec2(0.0f, rowIt.LocalHeight * 0.5f);

							if (note.BeatDuration > Beat::Zero())
							{
								const vec2 localTR = vec2(Camera.TimeToLocalSpaceX(endTime), rowIt.LocalY);
								const vec2 localCenterEnd = localTR + vec2(0.0f, rowIt.LocalHeight * 0.5f);
								DrawTimelineNoteDuration(DrawListContent, LocalToScreenSpace(localCenter), LocalToScreenSpace(localCenterEnd), note.Type);
							}

							const f32 noteScaleFactor = GetTimelineNoteScaleFactor(isPlayback, cursorTime, cursorBeatOnPlaybackStart, note, startTime);
							DrawTimelineNote(DrawListContent, LocalToScreenSpace(localCenter), noteScaleFactor, note.Type);

							if (IsBalloonNote(note.Type) || note.BalloonPopCount > 0)
								DrawTimelineNoteBalloonPopCount(DrawListContent, LocalToScreenSpace(localCenter), noteScaleFactor, note.BalloonPopCount);

							if (note.IsSelected)
							{
								const vec2 hitBoxSize = vec2(GuiScale((IsBigNote(note.Type) ? TimelineSelectedNoteHitBoxSizeBig : TimelineSelectedNoteHitBoxSizeSmall)));
								TempSelectionBoxesDrawBuffer.push_back(TempDrawSelectionBox { Rect::FromCenterSize(LocalToScreenSpace(localCenter), hitBoxSize), TimelineSelectedNoteBoxBackgroundColor, TimelineSelectedNoteBoxBorderColor });
							}
						}

						if (!TempDeletedNoteAnimationsBuffer.empty())
						{
							for (const auto& data : TempDeletedNoteAnimationsBuffer)
							{
								if (data.Branch != branchForThisRow)
									continue;

								const Time startTime = context.BeatToTime(data.OriginalNote.BeatTime) + data.OriginalNote.TimeOffset;
								const Time endTime = startTime;
								if (endTime < visibleTime.Min || startTime > visibleTime.Max)
									continue;

								const vec2 localTL = vec2(Camera.TimeToLocalSpaceX(startTime), rowIt.LocalY);
								const vec2 localCenter = localTL + vec2(0.0f, rowIt.LocalHeight * 0.5f);

								const f32 noteScaleFactor = ConvertRange(0.0f, NoteDeleteAnimationDuration, 1.0f, 0.0f, ClampBot(data.ElapsedTimeSec, 0.0f));
								DrawTimelineNote(DrawListContent, LocalToScreenSpace(localCenter), noteScaleFactor, data.OriginalNote.Type);
								// TODO: Also animate duration fading out or "collapsing" some other way (?)
							}
						}

						if (!TempSelectionBoxesDrawBuffer.empty())
						{
							for (const auto& box : TempSelectionBoxesDrawBuffer)
							{
								DrawListContent->AddRectFilled(box.ScreenSpaceRect.TL, box.ScreenSpaceRect.BR, box.FillColor);
								DrawListContent->AddRect(box.ScreenSpaceRect.TL, box.ScreenSpaceRect.BR, box.BorderColor);
							}
							TempSelectionBoxesDrawBuffer.clear();
						}

						// NOTE: Long note placement preview
						if (LongNotePlacement.IsActive && context.ChartSelectedBranch == branchForThisRow)
						{
							const Beat minBeat = LongNotePlacement.GetMin(), maxBeat = LongNotePlacement.GetMax();
							const vec2 localTL = vec2(Camera.TimeToLocalSpaceX(context.BeatToTime(minBeat)), rowIt.LocalY);
							const vec2 localCenter = localTL + vec2(0.0f, rowIt.LocalHeight * 0.5f);
							const vec2 localTR = vec2(Camera.TimeToLocalSpaceX(context.BeatToTime(maxBeat)), rowIt.LocalY);
							const vec2 localCenterEnd = localTR + vec2(0.0f, rowIt.LocalHeight * 0.5f);
							DrawTimelineNoteDuration(DrawListContent, LocalToScreenSpace(localCenter), LocalToScreenSpace(localCenterEnd), LongNotePlacement.NoteType, 0.7f);
							DrawTimelineNote(DrawListContent, LocalToScreenSpace(localCenter), 1.0f, LongNotePlacement.NoteType, 0.7f);

							if (IsBalloonNote(LongNotePlacement.NoteType))
								DrawTimelineNoteBalloonPopCount(DrawListContent, LocalToScreenSpace(localCenter), 1.0f, DefaultBalloonPopCount(maxBeat - minBeat, CurrentGridBarDivision));
						}
					} break;

					case TimelineRowType::ScrollSpeed:
					{
						for (const auto& scroll : context.ChartSelectedCourse->ScrollChanges)
						{
							const Time startTime = context.BeatToTime(scroll.BeatTime);
							if (startTime < visibleTime.Min || startTime > visibleTime.Max)
								continue;

							const vec2 localSpaceTL = vec2(Camera.TimeToLocalSpaceX(startTime), rowIt.LocalY);
							const vec2 localSpaceBL = localSpaceTL + vec2(0.0f, rowIt.LocalHeight);
							const vec2 localSpaceCenter = localSpaceTL + vec2(0.0f, rowIt.LocalHeight * 0.5f);

							const f32 textHeight = Gui::GetFontSize();
							const vec2 textPosition = LocalToScreenSpace(localSpaceCenter + vec2(3.0f, textHeight * -0.5f));

							Gui::DisableFontPixelSnap(true);
							char buffer[32]; const auto text = std::string_view(buffer, sprintf_s(buffer, "%gx", scroll.ScrollSpeed));
							const vec2 textSize = Gui::CalcTextSize(text);
							DrawListContent->AddRectFilled(vec2(LocalToScreenSpace(localSpaceTL).x, textPosition.y), textPosition + textSize, TimelineBackgroundColor);
							DrawListContent->AddLine(LocalToScreenSpace(localSpaceTL + vec2(0.0f, 1.0f)), LocalToScreenSpace(localSpaceBL), TimelineScrollChangeLineColor);
							Gui::AddTextWithDropShadow(DrawListContent, textPosition, TimelineItemTextColor, text, TimelineItemTextColorShadow);
							Gui::DisableFontPixelSnap(false);
						}
					} break;

					case TimelineRowType::BarLineVisibility:
					{
						for (const auto& barLine : context.ChartSelectedCourse->BarLineChanges)
						{
							const Time startTime = context.BeatToTime(barLine.BeatTime);
							if (startTime < visibleTime.Min || startTime > visibleTime.Max)
								continue;

							const vec2 localSpaceTL = vec2(Camera.TimeToLocalSpaceX(startTime), rowIt.LocalY);
							const vec2 localSpaceBL = localSpaceTL + vec2(0.0f, rowIt.LocalHeight);
							const vec2 localSpaceCenter = localSpaceTL + vec2(0.0f, rowIt.LocalHeight * 0.5f);

							const f32 textHeight = Gui::GetFontSize();
							const vec2 textPosition = LocalToScreenSpace(localSpaceCenter + vec2(3.0f, textHeight * -0.5f));

							Gui::DisableFontPixelSnap(true);
							const std::string_view text = barLine.IsVisible ? "On" : "Off";
							const vec2 textSize = Gui::CalcTextSize(text);
							DrawListContent->AddRectFilled(vec2(LocalToScreenSpace(localSpaceTL).x, textPosition.y), textPosition + textSize, TimelineBackgroundColor);
							DrawListContent->AddLine(LocalToScreenSpace(localSpaceTL + vec2(0.0f, 1.0f)), LocalToScreenSpace(localSpaceBL), TimelineBarLineChangeLineColor);
							Gui::AddTextWithDropShadow(DrawListContent, textPosition, TimelineItemTextColor, text, TimelineItemTextColorShadow);
							Gui::DisableFontPixelSnap(false);
						}
					} break;

					case TimelineRowType::GoGoTime:
					{
						for (const auto& gogo : context.ChartSelectedCourse->GoGoRanges)
						{
							const Time startTime = context.BeatToTime(gogo.GetStart());
							const Time endTime = context.BeatToTime(gogo.GetEnd());
							if (endTime < visibleTime.Min || startTime > visibleTime.Max)
								continue;

							static constexpr f32 margin = 1.0f;
							const vec2 localTL = vec2(Camera.TimeToLocalSpaceX(startTime), 0.0f) + vec2(0.0f, rowIt.LocalY + margin);
							const vec2 localBR = vec2(Camera.TimeToLocalSpaceX(endTime), 0.0f) + vec2(0.0f, rowIt.LocalY + rowIt.LocalHeight - (margin * 2.0f));
							DrawTimelineGoGoTimeBackground(DrawListContent, LocalToScreenSpace(localTL) + vec2(0.0f, 2.0f), LocalToScreenSpace(localBR), gogo.ExpansionAnimationCurrent);
						}
					} break;

					case TimelineRowType::Lyrics:
					{
						if (const auto& lyrics = context.ChartSelectedCourse->Lyrics; !lyrics.empty())
						{
							const Beat chartBeatDuration = context.TimeToBeat(context.Chart.GetDurationOrDefault());

							Gui::PushFont(FontMain_JP);
							for (size_t i = 0; i < lyrics.size(); i++)
							{
								const auto* prevLyric = IndexOrNull(static_cast<i32>(i) - 1, lyrics);
								const auto& thisLyric = lyrics[i];
								const auto* nextLyric = IndexOrNull(i + 1, lyrics);
								if (thisLyric.Lyric.empty() && prevLyric != nullptr && !prevLyric->Lyric.empty())
									continue;

								const Beat lastBeat = (thisLyric.BeatTime <= chartBeatDuration) ? chartBeatDuration : Beat::FromTicks(I32Max);
								const Time startTime = context.BeatToTime(thisLyric.BeatTime);
								const Time endTime = context.BeatToTime(thisLyric.Lyric.empty() ? thisLyric.BeatTime : (nextLyric != nullptr) ? nextLyric->BeatTime : lastBeat);
								if (endTime < visibleTime.Min || startTime > visibleTime.Max)
									continue;

								const vec2 localSpaceTL = vec2(Camera.TimeToLocalSpaceX(startTime), rowIt.LocalY);
								const vec2 localSpaceBR = vec2(Camera.TimeToLocalSpaceX(endTime), rowIt.LocalY + rowIt.LocalHeight);
								const vec2 localSpaceCenter = localSpaceTL + vec2(0.0f, rowIt.LocalHeight * 0.5f);

								const f32 textHeight = Gui::GetFontSize();
								const vec2 textPosition = LocalToScreenSpace(localSpaceCenter + vec2(4.0f, textHeight * -0.5f));

								Gui::DisableFontPixelSnap(true);
								{
									const Rect lyricsBarRect = Rect(LocalToScreenSpace(localSpaceTL + vec2(0.0f, 1.0f)), LocalToScreenSpace(localSpaceBR));
									DrawTimelineLyricsBackground(DrawListContent, lyricsBarRect.TL, lyricsBarRect.BR);

									static constexpr f32 borderLeft = 2.0f, borderRight = 5.0f;
									const ImVec4 clipRect = { lyricsBarRect.TL.x + borderLeft, lyricsBarRect.TL.y, lyricsBarRect.BR.x - borderRight, lyricsBarRect.BR.y };

									if (Absolute(clipRect.z - clipRect.x) > (borderLeft + borderRight))
										Gui::AddTextWithDropShadow(DrawListContent, nullptr, 0.0f, textPosition, TimelineLyricsTextColor, thisLyric.Lyric, 0.0f, &clipRect, TimelineLyricsTextColorShadow);
								}
								Gui::DisableFontPixelSnap(false);
							}
							Gui::PopFont();
						}
					} break;

					default: { assert(!"Missing TimelineRowType switch case"); } break;
					}
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
				Gui::DisableFontPixelSnap(true);
				const vec2 center = LocalToScreenSpace(Camera.WorldToLocalSpace(BoxSelection.WorldSpaceRect.TL));
				DrawListContent->AddCircleFilled(center, TimelineBoxSelectionRadius, TimelineBoxSelectionFillColor);
				DrawListContent->AddCircle(center, TimelineBoxSelectionRadius, TimelineBoxSelectionBorderColor);
				{
					const Rect horizontal = Rect::FromCenterSize(center, vec2((TimelineBoxSelectionRadius - TimelineBoxSelectionLinePadding) * 2.0f, TimelineBoxSelectionLineThickness));
					const Rect vertical = Rect::FromCenterSize(center, vec2(TimelineBoxSelectionLineThickness, (TimelineBoxSelectionRadius - TimelineBoxSelectionLinePadding) * 2.0f));
					DrawListContent->AddRectFilled(horizontal.TL, horizontal.BR, TimelineBoxSelectionInnerColor);
					if (BoxSelection.Action == BoxSelectionAction::Add)
						DrawListContent->AddRectFilled(vertical.TL, vertical.BR, TimelineBoxSelectionInnerColor);
				}
				Gui::DisableFontPixelSnap(false);
			}
		}
	}
}
