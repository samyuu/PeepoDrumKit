#include "chart.h"
#include <algorithm>

namespace PeepoDrumKit
{
	static constexpr NoteType ConvertTJANoteType(TJA::NoteType tjaNoteType)
	{
		switch (tjaNoteType)
		{
		case TJA::NoteType::None: return NoteType::Count;
		case TJA::NoteType::Don: return NoteType::Don;
		case TJA::NoteType::Ka: return NoteType::Ka;
		case TJA::NoteType::DonBig: return NoteType::DonBig;
		case TJA::NoteType::KaBig: return NoteType::KaBig;
		case TJA::NoteType::Start_Drumroll: return NoteType::Drumroll;
		case TJA::NoteType::Start_DrumrollBig: return NoteType::DrumrollBig;
		case TJA::NoteType::Start_Balloon: return NoteType::Balloon;
		case TJA::NoteType::End_BalloonOrDrumroll: return NoteType::Count;
		case TJA::NoteType::Start_BaloonSpecial: return NoteType::BalloonSpecial;
		case TJA::NoteType::DonBigBoth: return NoteType::Count;
		case TJA::NoteType::KaBigBoth: return NoteType::Count;
		case TJA::NoteType::Hidden: return NoteType::Count;
		default: return NoteType::Count;
		}
	}

	static constexpr TJA::NoteType ConvertTJANoteType(NoteType noteType)
	{
		switch (noteType)
		{
		case NoteType::Don: return TJA::NoteType::Don;
		case NoteType::DonBig: return TJA::NoteType::DonBig;
		case NoteType::Ka: return TJA::NoteType::Ka;
		case NoteType::KaBig: return TJA::NoteType::KaBig;
		case NoteType::Drumroll: return TJA::NoteType::Start_Drumroll;
		case NoteType::DrumrollBig: return TJA::NoteType::Start_DrumrollBig;
		case NoteType::Balloon: return TJA::NoteType::Start_Balloon;
		case NoteType::BalloonSpecial: return TJA::NoteType::Start_BaloonSpecial;
		default: return TJA::NoteType::None;
		}
	}

	bool CreateChartProjectFromTJA(const TJA::ParsedTJA& inTJA, ChartProject& out)
	{
		out.ChartDuration = Time::Zero();
		out.ChartTitle[Language::Base] = inTJA.Metadata.TITLE;
		out.ChartTitle[Language::JA] = inTJA.Metadata.TITLE_JA;
		out.ChartTitle[Language::EN] = inTJA.Metadata.TITLE_EN;
		out.ChartTitle[Language::CN] = inTJA.Metadata.TITLE_CN;
		out.ChartTitle[Language::TW] = inTJA.Metadata.TITLE_TW;
		out.ChartTitle[Language::KO] = inTJA.Metadata.TITLE_KO;
		out.ChartSubtitle[Language::Base] = inTJA.Metadata.SUBTITLE;
		out.ChartSubtitle[Language::JA] = inTJA.Metadata.SUBTITLE_JA;
		out.ChartSubtitle[Language::EN] = inTJA.Metadata.SUBTITLE_EN;
		out.ChartSubtitle[Language::CN] = inTJA.Metadata.SUBTITLE_CN;
		out.ChartSubtitle[Language::TW] = inTJA.Metadata.SUBTITLE_TW;
		out.ChartSubtitle[Language::KO] = inTJA.Metadata.SUBTITLE_KO;
		out.ChartCreator = inTJA.Metadata.MAKER;
		out.ChartGenre = inTJA.Metadata.GENRE;
		out.ChartLyricsFileName = inTJA.Metadata.LYRICS;
		out.SongOffset = inTJA.Metadata.OFFSET;
		out.SongDemoStartTime = inTJA.Metadata.DEMOSTART;
		out.SongFileName = inTJA.Metadata.WAVE;
		out.SongVolume = inTJA.Metadata.SONGVOL;
		out.SoundEffectVolume = inTJA.Metadata.SEVOL;
		out.BackgroundImageFileName = inTJA.Metadata.BGIMAGE;
		out.BackgroundMovieFileName = inTJA.Metadata.BGMOVIE;
		out.MovieOffset = inTJA.Metadata.MOVIEOFFSET;
		for (size_t i = 0; i < inTJA.Courses.size(); i++)
		{
			const TJA::ConvertedCourse& inCourse = TJA::ConvertParsedToConvertedCourse(inTJA, inTJA.Courses[i]);
			ChartCourse& outCourse = *out.Courses.emplace_back(std::make_unique<ChartCourse>());

			// HACK: Write proper enum conversion functions
			outCourse.Type = Clamp(static_cast<DifficultyType>(inCourse.CourseMetadata.COURSE), DifficultyType {}, DifficultyType::Count);
			outCourse.Level = Clamp(static_cast<DifficultyLevel>(inCourse.CourseMetadata.LEVEL), DifficultyLevel::Min, DifficultyLevel::Max);

			outCourse.TempoMap.Tempo.Sorted = { TempoChange(Beat::Zero(), inTJA.Metadata.BPM) };
			outCourse.TempoMap.Signature.Sorted = { TimeSignatureChange(Beat::Zero(), TimeSignature(4, 4)) };
			TimeSignature lastSignature = TimeSignature(4, 4);

			i32 currentBalloonIndex = 0;

			for (const TJA::ConvertedMeasure& inMeasure : inCourse.Measures)
			{
				for (const TJA::ConvertedNote& inNote : inMeasure.Notes)
				{
					if (inNote.Type == TJA::NoteType::End_BalloonOrDrumroll)
					{
						// TODO: Proper handling
						if (!outCourse.Notes_Normal.Sorted.empty())
							outCourse.Notes_Normal.Sorted.back().BeatDuration = (inMeasure.StartTime + inNote.TimeWithinMeasure) - outCourse.Notes_Normal.Sorted.back().BeatTime;
						continue;
					}

					const NoteType outNoteType = ConvertTJANoteType(inNote.Type);
					if (outNoteType == NoteType::Count)
						continue;

					Note& outNote = outCourse.Notes_Normal.Sorted.emplace_back();
					outNote.BeatTime = (inMeasure.StartTime + inNote.TimeWithinMeasure);
					outNote.Type = outNoteType;

					if (inNote.Type == TJA::NoteType::Start_Balloon || inNote.Type == TJA::NoteType::Start_BaloonSpecial)
					{
						// TODO: Implement properly with correct branch handling
						if (InBounds(currentBalloonIndex, inCourse.CourseMetadata.BALLOON))
							outNote.BalloonPopCount = inCourse.CourseMetadata.BALLOON[currentBalloonIndex];
						currentBalloonIndex++;
					}
				}

				if (inMeasure.TimeSignature != lastSignature)
				{
					outCourse.TempoMap.Signature.InsertOrUpdate(TimeSignatureChange(inMeasure.StartTime, inMeasure.TimeSignature));
					lastSignature = inMeasure.TimeSignature;
				}

				for (const TJA::ConvertedTempoChange& inTempoChange : inMeasure.TempoChanges)
					outCourse.TempoMap.Tempo.InsertOrUpdate(TempoChange(inMeasure.StartTime + inTempoChange.TimeWithinMeasure, inTempoChange.Tempo));

				for (const TJA::ConvertedScrollChange& inScrollChange : inMeasure.ScrollChanges)
					outCourse.ScrollChanges.Sorted.push_back(ScrollChange { (inMeasure.StartTime + inScrollChange.TimeWithinMeasure), inScrollChange.ScrollSpeed });

				for (const TJA::ConvertedBarLineChange& barLineChange : inMeasure.BarLineChanges)
					outCourse.BarLineChanges.Sorted.push_back(BarLineChange { (inMeasure.StartTime + barLineChange.TimeWithinMeasure), barLineChange.Visibile });

				for (const TJA::ConvertedLyricChange& lyricChange : inMeasure.LyricChanges)
					outCourse.Lyrics.Sorted.push_back(LyricChange { (inMeasure.StartTime + lyricChange.TimeWithinMeasure), lyricChange.Lyric });
			}

			for (const TJA::ConvertedGoGoRange& inGoGoRange : inCourse.GoGoRanges)
				outCourse.GoGoRanges.Sorted.push_back(GoGoRange { inGoGoRange.StartTime, (inGoGoRange.EndTime - inGoGoRange.StartTime) });

			//outCourse.TempoMap.SetTempoChange(TempoChange());
			//outCourse.TempoMap = inCourse.GoGoRanges;

			outCourse.ScoreInit = inCourse.CourseMetadata.SCOREINIT;
			outCourse.ScoreDiff = inCourse.CourseMetadata.SCOREDIFF;

			outCourse.TempoMap.RebuildAccelerationStructure();

			if (!inCourse.Measures.empty())
				out.ChartDuration = Max(out.ChartDuration, outCourse.TempoMap.BeatToTime(inCourse.Measures.back().StartTime /*+ inCourse.Measures.back().TimeSignature.GetDurationPerBar()*/));
		}

		return true;
	}

	Beat FindCourseMaxUsedBeat(const ChartCourse& course)
	{
		// NOTE: Technically only need to look at the last item of each sorted list **but just to be sure**, in case there is something wonky going on with out-of-order durations or something
		Beat maxBeat = Beat::Zero();
		for (const auto& v : course.TempoMap.Tempo) maxBeat = Max(maxBeat, v.Beat);
		for (const auto& v : course.TempoMap.Signature) maxBeat = Max(maxBeat, v.Beat);
		for (size_t i = 0; i < EnumCount<BranchType>; i++)
			for (const auto& v : course.GetNotes(static_cast<BranchType>(i))) maxBeat = Max(maxBeat, v.BeatTime + Max(Beat::Zero(), v.BeatDuration));
		for (const auto& v : course.GoGoRanges) maxBeat = Max(maxBeat, v.BeatTime + Max(Beat::Zero(), v.BeatDuration));
		for (const auto& v : course.ScrollChanges) maxBeat = Max(maxBeat, v.BeatTime);
		for (const auto& v : course.BarLineChanges) maxBeat = Max(maxBeat, v.BeatTime);
		for (const auto& v : course.Lyrics) maxBeat = Max(maxBeat, v.BeatTime);
		return maxBeat;
	}

	bool ConvertChartProjectToTJA(const ChartProject& in, TJA::ParsedTJA& out)
	{
		static constexpr const char* FallbackTJAChartTitle = "Untitled Chart";
		out.Metadata.TITLE = !in.ChartTitle[Language::Base].empty() ? in.ChartTitle[Language::Base] : FallbackTJAChartTitle;
		out.Metadata.TITLE_JA = in.ChartTitle[Language::JA];
		out.Metadata.TITLE_EN = in.ChartTitle[Language::EN];
		out.Metadata.TITLE_CN = in.ChartTitle[Language::CN];
		out.Metadata.TITLE_TW = in.ChartTitle[Language::TW];
		out.Metadata.TITLE_KO = in.ChartTitle[Language::KO];
		out.Metadata.SUBTITLE = in.ChartSubtitle[Language::Base];
		out.Metadata.SUBTITLE_JA = in.ChartSubtitle[Language::JA];
		out.Metadata.SUBTITLE_EN = in.ChartSubtitle[Language::EN];
		out.Metadata.SUBTITLE_CN = in.ChartSubtitle[Language::CN];
		out.Metadata.SUBTITLE_TW = in.ChartSubtitle[Language::TW];
		out.Metadata.SUBTITLE_KO = in.ChartSubtitle[Language::KO];
		out.Metadata.MAKER = in.ChartCreator;
		out.Metadata.GENRE = in.ChartGenre;
		out.Metadata.LYRICS = in.ChartLyricsFileName;
		out.Metadata.OFFSET = in.SongOffset;
		out.Metadata.DEMOSTART = in.SongDemoStartTime;
		out.Metadata.WAVE = in.SongFileName;
		out.Metadata.SONGVOL = in.SongVolume;
		out.Metadata.SEVOL = in.SoundEffectVolume;
		out.Metadata.BGIMAGE = in.BackgroundImageFileName;
		out.Metadata.BGMOVIE = in.BackgroundMovieFileName;
		out.Metadata.MOVIEOFFSET = in.MovieOffset;

		if (!in.Courses.empty())
		{
			if (!in.Courses[0]->TempoMap.Tempo.empty())
			{
				const TempoChange* initialTempo = in.Courses[0]->TempoMap.Tempo.TryFindLastAtBeat(Beat::Zero());
				out.Metadata.BPM = (initialTempo != nullptr) ? initialTempo->Tempo : FallbackTempo;
			}
		}

		out.Courses.reserve(in.Courses.size());
		for (const std::unique_ptr<ChartCourse>& inCourseIt : in.Courses)
		{
			const ChartCourse& inCourse = *inCourseIt;
			TJA::ParsedCourse& outCourse = out.Courses.emplace_back();

			// HACK: Write proper enum conversion functions
			outCourse.Metadata.COURSE = static_cast<TJA::DifficultyType>(inCourse.Type);
			outCourse.Metadata.LEVEL = static_cast<i32>(inCourse.Level);
			for (const Note& inNote : inCourse.Notes_Normal) if (IsBalloonNote(inNote.Type)) { outCourse.Metadata.BALLOON.push_back(inNote.BalloonPopCount); }
			outCourse.Metadata.SCOREINIT = inCourse.ScoreInit;
			outCourse.Metadata.SCOREDIFF = inCourse.ScoreDiff;

			// TODO: Is this implemented correctly..? Need to have enough measures to cover every note/command and pad with empty measures up to the chart duration
			// BUG: NOPE! "07 ゲームミュージック/003D. MagiCatz/MagiCatz.tja" for example still gets rounded up and then increased by a measure each time it gets saved
			// ... and even so does "Heat Haze Shadow 2.tja" without any weird time signatures..??
			const Beat inChartMaxUsedBeat = FindCourseMaxUsedBeat(inCourse);
			const Beat inChartBeatDuration = inCourse.TempoMap.TimeToBeat(in.GetDurationOrDefault());
			std::vector<TJA::ConvertedMeasure> outConvertedMeasures;

			inCourse.TempoMap.ForEachBeatBar([&](const SortedTempoMap::ForEachBeatBarData& it)
			{
				if (inChartBeatDuration > inChartMaxUsedBeat && (it.Beat >= inChartBeatDuration))
					return ControlFlow::Break;
				if (it.IsBar)
				{
					TJA::ConvertedMeasure& outConvertedMeasure = outConvertedMeasures.emplace_back();
					outConvertedMeasure.StartTime = it.Beat;
					outConvertedMeasure.TimeSignature = it.Signature;
				}
				return (it.Beat >= Max(inChartBeatDuration, inChartMaxUsedBeat)) ? ControlFlow::Break : ControlFlow::Continue;
			});

			if (outConvertedMeasures.empty())
				outConvertedMeasures.push_back(TJA::ConvertedMeasure { Beat::Zero(), TimeSignature(4, 4) });

			static constexpr auto tryFindMeasureForBeat = [](std::vector<TJA::ConvertedMeasure>& measures, Beat beatToFind) -> TJA::ConvertedMeasure*
			{
				// TODO: Optimize using binary search
				for (auto& measure : measures)
					if (beatToFind >= measure.StartTime && beatToFind < (measure.StartTime + measure.TimeSignature.GetDurationPerBar()))
						return &measure;
				return nullptr;
			};

			for (const TempoChange& inTempoChange : inCourse.TempoMap.Tempo)
			{
				if (!(&inTempoChange == &inCourse.TempoMap.Tempo[0] && inTempoChange.Tempo.BPM == out.Metadata.BPM.BPM))
				{
					TJA::ConvertedMeasure* outConvertedMeasure = tryFindMeasureForBeat(outConvertedMeasures, inTempoChange.Beat);
					if (assert(outConvertedMeasure != nullptr); outConvertedMeasure != nullptr)
						outConvertedMeasure->TempoChanges.push_back(TJA::ConvertedTempoChange { (inTempoChange.Beat - outConvertedMeasure->StartTime), inTempoChange.Tempo });
				}
			}

			for (const Note& inNote : inCourse.Notes_Normal)
			{
				TJA::ConvertedMeasure* outConvertedMeasure = tryFindMeasureForBeat(outConvertedMeasures, inNote.BeatTime);
				if (assert(outConvertedMeasure != nullptr); outConvertedMeasure != nullptr)
					outConvertedMeasure->Notes.push_back(TJA::ConvertedNote { (inNote.BeatTime - outConvertedMeasure->StartTime), ConvertTJANoteType(inNote.Type) });

				if (inNote.BeatDuration > Beat::Zero())
				{
					TJA::ConvertedMeasure* outConvertedMeasure = tryFindMeasureForBeat(outConvertedMeasures, inNote.BeatTime + inNote.BeatDuration);
					if (assert(outConvertedMeasure != nullptr); outConvertedMeasure != nullptr)
						outConvertedMeasure->Notes.push_back(TJA::ConvertedNote { ((inNote.BeatTime + inNote.BeatDuration) - outConvertedMeasure->StartTime), TJA::NoteType::End_BalloonOrDrumroll });
				}
			}

			for (const ScrollChange& inScroll : inCourse.ScrollChanges)
			{
				TJA::ConvertedMeasure* outConvertedMeasure = tryFindMeasureForBeat(outConvertedMeasures, inScroll.BeatTime);
				if (assert(outConvertedMeasure != nullptr); outConvertedMeasure != nullptr)
					outConvertedMeasure->ScrollChanges.push_back(TJA::ConvertedScrollChange { (inScroll.BeatTime - outConvertedMeasure->StartTime), inScroll.ScrollSpeed });
			}

			for (const BarLineChange& barLineChange : inCourse.BarLineChanges)
			{
				TJA::ConvertedMeasure* outConvertedMeasure = tryFindMeasureForBeat(outConvertedMeasures, barLineChange.BeatTime);
				if (assert(outConvertedMeasure != nullptr); outConvertedMeasure != nullptr)
					outConvertedMeasure->BarLineChanges.push_back(TJA::ConvertedBarLineChange { (barLineChange.BeatTime - outConvertedMeasure->StartTime), barLineChange.IsVisible });
				// assert(outConvertedMeasure->BarLineChanges.size() == 1 && "This shouldn't be a vector at all (?)");
			}

			for (const LyricChange& inLyric : inCourse.Lyrics)
			{
				TJA::ConvertedMeasure* outConvertedMeasure = tryFindMeasureForBeat(outConvertedMeasures, inLyric.BeatTime);
				if (assert(outConvertedMeasure != nullptr); outConvertedMeasure != nullptr)
					outConvertedMeasure->LyricChanges.push_back(TJA::ConvertedLyricChange { (inLyric.BeatTime - outConvertedMeasure->StartTime), inLyric.Lyric });
			}

			std::vector<TJA::ConvertedGoGoRange> outConvertedGoGoRanges;
			outConvertedGoGoRanges.reserve(inCourse.GoGoRanges.size());
			for (const GoGoRange& gogo : inCourse.GoGoRanges)
				outConvertedGoGoRanges.push_back(TJA::ConvertedGoGoRange { gogo.BeatTime, (gogo.BeatTime + Max(Beat::Zero(), gogo.BeatDuration)) });

			TJA::ConvertConvertedMeasuresToParsedCommands(outConvertedMeasures, outConvertedGoGoRanges, outCourse.ChartCommands);
		}

		return true;
	}
}
