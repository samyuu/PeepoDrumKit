#include "chart_editor_widgets.h"

namespace PeepoDrumKit
{
	struct BezierKeyFrame { f32 Time, Value, HandleL, HandleR; };

	static constexpr f32 InterpolateCubicBezier(f32 pointA, f32 pointB, f32 pointC, f32 pointD, f32 t)
	{
		const f32 invT = (1.0f - t);
		return
			(pointA * invT * invT * invT) +
			(pointB * 3.0f * invT * invT * t) +
			(pointC * 3.0f * invT * t * t) +
			(pointD *  t * t * t);
	}

	static constexpr f32 InterpolateCubicBezier(const BezierKeyFrame& left, const BezierKeyFrame& right, f32 time)
	{
		const f32 t = (time - left.Time) / ClampBot((right.Time - left.Time), 0.00001f);
		return InterpolateCubicBezier(left.Value, left.HandleR, right.HandleL, right.Value, t);
	}

	struct LeftRightBezierKeyFrames { const BezierKeyFrame *L, *R; };
	static constexpr LeftRightBezierKeyFrames FindClosestKeyFramesAt(const BezierKeyFrame* keys, size_t keyCount, f32 time)
	{
		const auto* keyL = &keys[0];
		const auto* keyR = keyL;
		for (size_t i = 1; i < keyCount; i++)
		{
			keyR = &keys[i];
			if (keyR->Time >= time)
				break;
			keyL = keyR;
		}
		return { keyL, keyR };
	}

	static constexpr f32 SampleBezierFCurve(const BezierKeyFrame* keys, size_t keyCount, f32 time)
	{
		if (keyCount == 0) return 0.0f;
		if (keyCount == 1) return keys[0].Value;

		const auto& first = keys[0];
		const auto& last = keys[keyCount - 1];
		if (time <= first.Time) return first.Value;
		if (time >= last.Time) return last.Value;

		const auto closest = FindClosestKeyFramesAt(keys, keyCount, time);
		return InterpolateCubicBezier(*closest.L, *closest.R, time);
	}

	template <size_t Size>
	static constexpr f32 SampleBezierFCurve(const BezierKeyFrame(&keys)[Size], f32 time) { return SampleBezierFCurve(keys, Size, time); }
}

namespace PeepoDrumKit
{
	static constexpr f32 FrameToTime(f32 frame, f32 fps = 60.0f) { return (frame / fps); }
	struct TimeLeftCenterRight { f32 Time; vec2 Left, Center, Right; };
#if 0
	static constexpr TimeLeftCenterRight HitPath[] =
	{
		{ FrameToTime(0.0f),  vec2(),				 vec2(618.0f, 386.0f),  vec2(663.0f, 293.0f) },
		{ FrameToTime(6.0f),  vec2(695.0f, 229.0f),	 vec2(792.0f, 153.0f),  vec2(850.0f, 99.0f) },
		{ FrameToTime(14.0f), vec2(1039.0f, -13.0f), vec2(1188.0f, -22.0f), vec2(1268.0f, -23.0f) },
		{ FrameToTime(23.0f), vec2(1420.0f, -30.0f), vec2(1568.0f, 41.0f),  vec2(1661.0f, 90.0f) },
		{ FrameToTime(29.0f), vec2(1815.0f, 189.0f), vec2(1836.0f, 244.0f), vec2() },
	};
	static constexpr BezierKeyFrame RefSpaceNoteHitPathX[] =
	{
		{ HitPath[0].Time, HitPath[0].Center.x, HitPath[0].Left.x, HitPath[0].Right.x },
		{ HitPath[1].Time, HitPath[1].Center.x, HitPath[1].Left.x, HitPath[1].Right.x },
		{ HitPath[2].Time, HitPath[2].Center.x, HitPath[2].Left.x, HitPath[2].Right.x },
		{ HitPath[3].Time, HitPath[3].Center.x, HitPath[3].Left.x, HitPath[3].Right.x },
		{ HitPath[4].Time, HitPath[4].Center.x, HitPath[4].Left.x, HitPath[4].Right.x },
	};
	static constexpr BezierKeyFrame RefSpaceNoteHitPathY[] =
	{
		{ HitPath[0].Time, HitPath[0].Center.y, HitPath[0].Left.y, HitPath[0].Right.y },
		{ HitPath[1].Time, HitPath[1].Center.y, HitPath[1].Left.y, HitPath[1].Right.y },
		{ HitPath[2].Time, HitPath[2].Center.y, HitPath[2].Left.y, HitPath[2].Right.y },
		{ HitPath[3].Time, HitPath[3].Center.y, HitPath[3].Left.y, HitPath[3].Right.y },
		{ HitPath[4].Time, HitPath[4].Center.y, HitPath[4].Left.y, HitPath[4].Right.y },
	};
#elif 0
	static constexpr TimeLeftCenterRight HitPath[] =
	{
		{ FrameToTime(0),  vec2(), vec2(616, 386),  vec2(692, 222) },
		{ FrameToTime(10), vec2(818, 118), vec2(954, 43),  vec2(1200, -50) },
		{ FrameToTime(22), vec2(1420, -30), vec2(1526, 16), vec2(1676, 72) },
		{ FrameToTime(30), vec2(1774, 169), vec2(1836, 246), vec2() },
	};
	static constexpr BezierKeyFrame RefSpaceNoteHitPathX[] =
	{
		{ HitPath[0].Time, HitPath[0].Center.x, HitPath[0].Left.x, HitPath[0].Right.x },
		{ HitPath[1].Time, HitPath[1].Center.x, HitPath[1].Left.x, HitPath[1].Right.x },
		{ HitPath[2].Time, HitPath[2].Center.x, HitPath[2].Left.x, HitPath[2].Right.x },
		{ HitPath[3].Time, HitPath[3].Center.x, HitPath[3].Left.x, HitPath[3].Right.x },
	};
	static constexpr BezierKeyFrame RefSpaceNoteHitPathY[] =
	{
		{ HitPath[0].Time, HitPath[0].Center.y, HitPath[0].Left.y, HitPath[0].Right.y },
		{ HitPath[1].Time, HitPath[1].Center.y, HitPath[1].Left.y, HitPath[1].Right.y },
		{ HitPath[2].Time, HitPath[2].Center.y, HitPath[2].Left.y, HitPath[2].Right.y },
		{ HitPath[3].Time, HitPath[3].Center.y, HitPath[3].Left.y, HitPath[3].Right.y },
	};
#elif 1

	constexpr TimeLeftCenterRight LinearKey(f32 frame, vec2 c) { return TimeLeftCenterRight { FrameToTime(frame), c, c, c }; }
	static constexpr TimeLeftCenterRight HitPath[] =
	{
		LinearKey({  0 }, vec2 {  615, 386 }),
		LinearKey({  1 }, vec2 {  639, 342 }),
		LinearKey({  2 }, vec2 {  664, 300 }),
		LinearKey({  3 }, vec2 {  693, 260 }),
		LinearKey({  4 }, vec2 {  725, 222 }),
		LinearKey({  5 }, vec2 {  758, 186 }),
		LinearKey({  6 }, vec2 {  793, 153 }),
		LinearKey({  7 }, vec2 {  830, 122 }),
		LinearKey({  8 }, vec2 {  870, 93  }),
		LinearKey({  9 }, vec2 {  912, 66  }),
		LinearKey({ 10 }, vec2 {  954, 43  }),
		LinearKey({ 11 }, vec2 { 1001, 27  }),
		LinearKey({ 12 }, vec2 { 1046, 11  }),
		LinearKey({ 13 }, vec2 { 1094, -2  }),
		LinearKey({ 14 }, vec2 { 1142, -14 }),
		LinearKey({ 15 }, vec2 { 1192, -18 }),
		LinearKey({ 16 }, vec2 { 1240, -22 }),
		LinearKey({ 17 }, vec2 { 1292, -23 }),
		LinearKey({ 18 }, vec2 { 1336, -22 }),
		LinearKey({ 19 }, vec2 { 1385, -16 }),
		LinearKey({ 20 }, vec2 { 1435, -8  }),
		LinearKey({ 21 }, vec2 { 1479, 3   }),
		LinearKey({ 22 }, vec2 { 1526, 16  }),
		LinearKey({ 23 }, vec2 { 1570, 36  }),
		LinearKey({ 24 }, vec2 { 1612, 56  }),
		LinearKey({ 25 }, vec2 { 1658, 83  }),
		LinearKey({ 26 }, vec2 { 1696, 115 }),
		LinearKey({ 27 }, vec2 { 1734, 144 }),
		LinearKey({ 28 }, vec2 { 1770, 176 }),
		LinearKey({ 29 }, vec2 { 1803, 210 }),
		LinearKey({ 30 }, vec2 { 1836, 247 }),
	};

	static constexpr BezierKeyFrame NoteHitPathBezierKeyAt(size_t i, i32 dimension)
	{
		return (dimension == 0) ?
			BezierKeyFrame { HitPath[i].Time, HitPath[i].Center.x, HitPath[i].Left.x, HitPath[i].Right.x } :
			BezierKeyFrame { HitPath[i].Time, HitPath[i].Center.y, HitPath[i].Left.y, HitPath[i].Right.y };
	}

	static constexpr BezierKeyFrame RefSpaceNoteHitPathX[] =
	{
		NoteHitPathBezierKeyAt({  0 }, 0), NoteHitPathBezierKeyAt({  1 }, 0), NoteHitPathBezierKeyAt({  2 }, 0), NoteHitPathBezierKeyAt({  3 }, 0), NoteHitPathBezierKeyAt({  4 }, 0),
		NoteHitPathBezierKeyAt({  5 }, 0), NoteHitPathBezierKeyAt({  6 }, 0), NoteHitPathBezierKeyAt({  7 }, 0), NoteHitPathBezierKeyAt({  8 }, 0), NoteHitPathBezierKeyAt({  9 }, 0),
		NoteHitPathBezierKeyAt({ 10 }, 0), NoteHitPathBezierKeyAt({ 11 }, 0), NoteHitPathBezierKeyAt({ 12 }, 0), NoteHitPathBezierKeyAt({ 13 }, 0), NoteHitPathBezierKeyAt({ 14 }, 0),
		NoteHitPathBezierKeyAt({ 15 }, 0), NoteHitPathBezierKeyAt({ 16 }, 0), NoteHitPathBezierKeyAt({ 17 }, 0), NoteHitPathBezierKeyAt({ 18 }, 0), NoteHitPathBezierKeyAt({ 19 }, 0),
		NoteHitPathBezierKeyAt({ 20 }, 0), NoteHitPathBezierKeyAt({ 21 }, 0), NoteHitPathBezierKeyAt({ 22 }, 0), NoteHitPathBezierKeyAt({ 23 }, 0), NoteHitPathBezierKeyAt({ 24 }, 0),
		NoteHitPathBezierKeyAt({ 25 }, 0), NoteHitPathBezierKeyAt({ 26 }, 0), NoteHitPathBezierKeyAt({ 27 }, 0), NoteHitPathBezierKeyAt({ 28 }, 0), NoteHitPathBezierKeyAt({ 29 }, 0),
		NoteHitPathBezierKeyAt({ 30 }, 0),
	};
	static constexpr BezierKeyFrame RefSpaceNoteHitPathY[] =
	{
		NoteHitPathBezierKeyAt({  0 }, 1), NoteHitPathBezierKeyAt({  1 }, 1), NoteHitPathBezierKeyAt({  2 }, 1), NoteHitPathBezierKeyAt({  3 }, 1), NoteHitPathBezierKeyAt({  4 }, 1),
		NoteHitPathBezierKeyAt({  5 }, 1), NoteHitPathBezierKeyAt({  6 }, 1), NoteHitPathBezierKeyAt({  7 }, 1), NoteHitPathBezierKeyAt({  8 }, 1), NoteHitPathBezierKeyAt({  9 }, 1),
		NoteHitPathBezierKeyAt({ 10 }, 1), NoteHitPathBezierKeyAt({ 11 }, 1), NoteHitPathBezierKeyAt({ 12 }, 1), NoteHitPathBezierKeyAt({ 13 }, 1), NoteHitPathBezierKeyAt({ 14 }, 1),
		NoteHitPathBezierKeyAt({ 15 }, 1), NoteHitPathBezierKeyAt({ 16 }, 1), NoteHitPathBezierKeyAt({ 17 }, 1), NoteHitPathBezierKeyAt({ 18 }, 1), NoteHitPathBezierKeyAt({ 19 }, 1),
		NoteHitPathBezierKeyAt({ 20 }, 1), NoteHitPathBezierKeyAt({ 21 }, 1), NoteHitPathBezierKeyAt({ 22 }, 1), NoteHitPathBezierKeyAt({ 23 }, 1), NoteHitPathBezierKeyAt({ 24 }, 1),
		NoteHitPathBezierKeyAt({ 25 }, 1), NoteHitPathBezierKeyAt({ 26 }, 1), NoteHitPathBezierKeyAt({ 27 }, 1), NoteHitPathBezierKeyAt({ 28 }, 1), NoteHitPathBezierKeyAt({ 29 }, 1),
		NoteHitPathBezierKeyAt({ 30 }, 1),
	};
#endif
	static constexpr BezierKeyFrame NoteHitFadeIn[] =
	{
		{ FrameToTime(30.0f), 0.0f, 0.0f, 0.0f },
		{ FrameToTime(39.0f), 1.0f, 1.0f, 1.0f },
	};
	static constexpr BezierKeyFrame NoteHitFadeOut[] =
	{
		{ FrameToTime(39.0f), 1.0f, 1.0f, 1.0f },
		{ FrameToTime(46.0f), 0.0f, 0.0f, 0.0f },
	};
	static constexpr Time NoteHitAnimationDuration = Time::FromSec(HitPath[ArrayCount(HitPath) - 1].Time); // Time::FromFrames(46.0);

	struct NoteHitPathAnimationData
	{
		vec2 RefSpaceOffset = {};
		f32 WhiteFadeIn = 0.0f;
		f32 AlphaFadeOut = 1.0f;
	};

	static NoteHitPathAnimationData GetNoteHitPathAnimation(Time timeSinceHit)
	{
		NoteHitPathAnimationData out {};
		if (timeSinceHit >= Time::Zero())
		{
			const f32 animationTime = timeSinceHit.ToSec_F32();
			out.RefSpaceOffset.x = SampleBezierFCurve(RefSpaceNoteHitPathX, animationTime) - HitPath[0].Center.x;
			out.RefSpaceOffset.y = SampleBezierFCurve(RefSpaceNoteHitPathY, animationTime) - HitPath[0].Center.y;
#if 0 // TODO: Just doesn't really look that great... maybe also needs the other hit effects too
			out.WhiteFadeIn = SampleBezierFCurve(NoteHitFadeIn, animationTime);
			out.AlphaFadeOut = SampleBezierFCurve(NoteHitFadeOut, animationTime);
#endif
		}
		return out;
	}

	static constexpr Time TimeSinceNoteHit(Time noteTime, Time cursorTime) { return (cursorTime - noteTime); }
}

namespace PeepoDrumKit
{
	struct ForEachBarLaneData
	{
		Time Time;
		Tempo Tempo;
		f32 ScrollSpeed;
	};

	template <typename Func>
	static void ForEachBarOnNoteLane(ChartCourse& course, BranchType branch, Beat maxBeatDuration, Func perBarFunc)
	{
		BeatSortedForwardIterator<TempoChange> tempoChangeIt {};
		BeatSortedForwardIterator<ScrollChange> scrollChangeIt {};
		BeatSortedForwardIterator<BarLineChange> barLineChangeIt {};

		course.TempoMap.ForEachBeatBar([&](const SortedTempoMap::ForEachBeatBarData& it)
		{
			if (it.Beat > maxBeatDuration)
				return ControlFlow::Break;

			if (!it.IsBar)
				return ControlFlow::Continue;

			if (!VisibleOrDefault(barLineChangeIt.Next(course.BarLineChanges.Sorted, it.Beat)))
				return ControlFlow::Continue;

			const Time time = course.TempoMap.BeatToTime(it.Beat);
			perBarFunc(ForEachBarLaneData { time,
				TempoOrDefault(tempoChangeIt.Next(course.TempoMap.Tempo.Sorted, it.Beat)),
				ScrollOrDefault(scrollChangeIt.Next(course.ScrollChanges.Sorted, it.Beat)) });

			return ControlFlow::Continue;
		});
	}

	struct ForEachNoteLaneData
	{
		const Note* OriginalNote;
		Time TimeHead;
		Time TimeTail;
		Tempo Tempo;
		f32 ScrollSpeed;
	};

	template <typename Func>
	static void ForEachNoteOnNoteLane(ChartCourse& course, BranchType branch, Func perNoteFunc)
	{
		BeatSortedForwardIterator<TempoChange> tempoChangeIt {};
		BeatSortedForwardIterator<ScrollChange> scrollChangeIt {};

		for (const Note& note : course.GetNotes(branch))
		{
			const Beat beat = note.BeatTime;
			const Time head = (course.TempoMap.BeatToTime(beat) + note.TimeOffset);
			const Time tail = (note.BeatDuration > Beat::Zero()) ? (course.TempoMap.BeatToTime(beat + note.BeatDuration) + note.TimeOffset) : head;
			perNoteFunc(ForEachNoteLaneData { &note, head, tail,
				TempoOrDefault(tempoChangeIt.Next(course.TempoMap.Tempo.Sorted, beat)),
				ScrollOrDefault(scrollChangeIt.Next(course.ScrollChanges.Sorted, beat))
				});
		}
	}

	void ChartGamePreview::DrawGui(ChartContext& context, Time animatedCursorTime)
	{
		Gui::BeginDisabled();
		Gui::PushStyleColor(ImGuiCol_Button, Gui::GetColorU32(ImGuiCol_Button, 0.25f));
		Gui::Button("##GamePreview", Gui::GetContentRegionAvail());
		Gui::PopStyleColor();
		Gui::EndDisabled();

		static constexpr f32 buttonMargin = 8.0f;;
		Camera.ScreenSpaceContentRect = Gui::GetItemRect();
		Camera.RefToScreenScaleFactor = ClampBot((Camera.ScreenSpaceContentRect.GetWidth() - (buttonMargin * 2.0f)) / GameRefLaneSize.x, 0.1f);
		Camera.ScreenSpaceLaneRect = Rect::FromTLSize(vec2(Camera.ScreenSpaceContentRect.TL.x + buttonMargin, Camera.ScreenSpaceContentRect.GetCenter().y - Camera.RefToScreenScale(GameRefLaneContentHeight * 0.5f)), Camera.RefToScreenScale(GameRefLaneSize));

		const Rect baseClipRect = Gui::GetItemRect();
		const Rect laneClipRect = Rect(Camera.RefToScreenSpace(vec2(0.0f, 0.0f)), Camera.RefToScreenSpace(GameRefLaneSize));
		const Rect laneClipRectExtended = Rect::FromCenterSize(laneClipRect.GetCenter(), vec2(laneClipRect.GetSize().x, Gui::GetWindowSize().y));

		ImDrawList* drawList = Gui::GetWindowDrawList();
		drawList->PushClipRect(baseClipRect.TL, baseClipRect.BR, false);
		{
			const b8 isPlayback = context.GetIsPlayback();
			const BeatAndTime exactCursorBeatAndTime = context.GetCursorBeatAndTime();
			const Time cursorTimeOrAnimated = isPlayback ? exactCursorBeatAndTime.Time : animatedCursorTime;
			const Beat cursorBeatOrAnimated = isPlayback ? exactCursorBeatAndTime.Beat : context.TimeToBeat(animatedCursorTime);
			const Beat chartBeatDuration = context.TimeToBeat(context.Chart.GetDurationOrDefault());

			{
				// TODO: Scan for last hit note and draw "hit" animation inner circle / background color change (?)

				// NOTE: Lane background and borders
				drawList->AddRectFilled(Camera.RefToScreenSpace(-GameRefLanePerSideBorderSize), Camera.RefToScreenSpace(GameRefLaneSize + GameRefLanePerSideBorderSize), GameLaneBorderColor);
				drawList->AddRectFilled(Camera.RefToScreenSpace(vec2(0.0f, 0.0f)), Camera.RefToScreenSpace(GameRefLaneSize), GameLaneBorderColor);
				drawList->AddRectFilled(Camera.RefToScreenSpace(vec2(0.0f, 0.0f)), Camera.RefToScreenSpace(vec2(GameRefLaneSize.x, GameRefLaneContentHeight)), GameLaneContentBackgroundColor);
				drawList->AddRectFilled(Camera.RefToScreenSpace(vec2(0.0f, GameRefLaneSize.y - GameRefLaneFooterHeight)), Camera.RefToScreenSpace(vec2(GameRefLaneSize.x, GameRefLaneSize.y)), GameLaneFooterBackgroundColor);

				// NOTE: Hit indicator circle
				drawList->AddCircleFilled(Camera.RefToScreenSpace(GameRefHitCircleCenter), Camera.RefToScreenScale(GameRefHitCircleInnerFillRadius), GameLaneHitCircleInnerFillColor);
				drawList->AddCircle(Camera.RefToScreenSpace(GameRefHitCircleCenter), Camera.RefToScreenScale(GameRefHitCircleInnerOutlineRadius), GameLaneHitCircleInnerOutlineColor, 0, Camera.RefToScreenScale(GameRefHitCircleInnerOutlineThickness));
				drawList->AddCircle(Camera.RefToScreenSpace(GameRefHitCircleCenter), Camera.RefToScreenScale(GameRefHitCircleOuterOutlineRadius), GameLaneHitCircleOuterOutlineColor, 0, Camera.RefToScreenScale(GameRefHitCircleOuterOutlineThickness));
			}

			drawList->PushClipRect(laneClipRect.TL, laneClipRect.BR, true);
			{
				ForEachBarOnNoteLane(*context.ChartSelectedCourse, context.ChartSelectedBranch, chartBeatDuration, [&](const ForEachBarLaneData& it)
				{
					const f32 refLaneX = TimeToNoteLaneRefSpaceX(cursorTimeOrAnimated, it.Time, it.Tempo, it.ScrollSpeed);
					if (Camera.IsPointVisibleOnLane(refLaneX))
					{
						const vec2 tl = Camera.RefToScreenSpace(vec2(GameRefHitCircleCenter.x + refLaneX, -1.0f));
						const vec2 br = Camera.RefToScreenSpace(vec2(GameRefHitCircleCenter.x + refLaneX, GameRefLaneContentHeight - 1.0f));
						drawList->AddLine(tl, br, GameLaneBarLineColor, Camera.RefToScreenScale(GameRefBarLineThickness));
					}
				});
			}
			drawList->PopClipRect();

			drawList->PushClipRect(laneClipRectExtended.TL, laneClipRectExtended.BR, true);
			{
				ForEachNoteOnNoteLane(*context.ChartSelectedCourse, context.ChartSelectedBranch, [&](const ForEachNoteLaneData& it)
				{
					const f32 refLaneHeadX = TimeToNoteLaneRefSpaceX(cursorTimeOrAnimated, it.TimeHead, it.Tempo, it.ScrollSpeed);
					const f32 refLaneTailX = TimeToNoteLaneRefSpaceX(cursorTimeOrAnimated, it.TimeTail, it.Tempo, it.ScrollSpeed);
					const Time timeSinceHit = TimeSinceNoteHit(it.TimeHead, cursorTimeOrAnimated);
					if (it.OriginalNote->BeatDuration <= Beat::Zero() && timeSinceHit >= Time::Zero())
					{
						if (timeSinceHit <= NoteHitAnimationDuration)
							ReverseNoteDrawBuffer.push_back(DeferredNoteDrawData { ClampBot(refLaneHeadX, 0.0f), ClampBot(refLaneTailX, 0.0f), it.OriginalNote, it.TimeHead });
					}
					else
					{
						if (Camera.IsRangeVisibleOnLane(refLaneHeadX, refLaneTailX))
							ReverseNoteDrawBuffer.push_back(DeferredNoteDrawData { refLaneHeadX, refLaneTailX, it.OriginalNote, it.TimeHead });
					}}
				);

				for (auto it = ReverseNoteDrawBuffer.rbegin(); it != ReverseNoteDrawBuffer.rend(); it++)
				{
					// BUG: Wrong drawing order for long notes? (noticable at high scroll speeds)
					if (it->OriginalNote->BeatDuration > Beat::Zero())
					{
						DrawGamePreviewNoteDuration(Camera, drawList,
							GameRefHitCircleCenter + vec2(it->RefLaneHeadX, 0.0f),
							GameRefHitCircleCenter + vec2(it->RefLaneTailX, 0.0f), it->OriginalNote->Type);
						DrawGamePreviewNote(Camera, drawList, GameRefHitCircleCenter + vec2(it->RefLaneHeadX, 0.0f), it->OriginalNote->Type);
					}
					else
					{
						const auto hitAnimation = GetNoteHitPathAnimation(TimeSinceNoteHit(it->NoteTime, cursorTimeOrAnimated));
						const vec2 refSpaceCenter = GameRefHitCircleCenter + vec2(it->RefLaneHeadX, 0.0f) + hitAnimation.RefSpaceOffset;

						if (hitAnimation.AlphaFadeOut >= 1.0f)
							DrawGamePreviewNote(Camera, drawList, refSpaceCenter, it->OriginalNote->Type);

						if (const f32 whiteAlpha = (hitAnimation.WhiteFadeIn * hitAnimation.AlphaFadeOut); whiteAlpha > 0.0f)
						{
							const auto radii = IsBigNote(it->OriginalNote->Type) ? GameRefNoteRadiiBig : GameRefNoteRadiiSmall;
							drawList->AddCircleFilled(Camera.RefToScreenSpace(refSpaceCenter), Camera.RefToScreenScale(radii.BlackOuter), ImColor(1.0f, 1.0f, 1.0f, whiteAlpha));
						}
					}
				}
				ReverseNoteDrawBuffer.clear();
			}
			drawList->PopClipRect();
		}
		drawList->PopClipRect();
	}
}