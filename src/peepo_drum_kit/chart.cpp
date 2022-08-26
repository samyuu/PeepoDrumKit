#include "chart.h"
#include "core_build_info.h"
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

	b8 CreateChartProjectFromTJA(const TJA::ParsedTJA& inTJA, ChartProject& out)
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

	b8 ConvertChartProjectToTJA(const ChartProject& in, TJA::ParsedTJA& out, b8 includePeepoDrumKitComment)
	{
		static constexpr cstr FallbackTJAChartTitle = "Untitled Chart";
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

		if (includePeepoDrumKitComment)
		{
			out.HasPeepoDrumKitComment = true;
			out.PeepoDrumKitCommentDate = BuildInfo::CompilationDateParsed;
		}

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

namespace PeepoDrumKit
{
	struct BeatStartAndDurationPtrs { Beat* Start; Beat* Duration; };
	inline BeatStartAndDurationPtrs GetGenericListStructRawBeatPtr(GenericListStruct& in, GenericList list)
	{
		switch (list)
		{
		case GenericList::TempoChanges: return { &in.POD.Tempo.Beat, nullptr };
		case GenericList::SignatureChanges: return { &in.POD.Signature.Beat, nullptr };
		case GenericList::Notes_Normal: return { &in.POD.Note.BeatTime, &in.POD.Note.BeatDuration };
		case GenericList::Notes_Expert: return { &in.POD.Note.BeatTime, &in.POD.Note.BeatDuration };
		case GenericList::Notes_Master: return { &in.POD.Note.BeatTime, &in.POD.Note.BeatDuration };
		case GenericList::ScrollChanges: return { &in.POD.Scroll.BeatTime, nullptr };
		case GenericList::BarLineChanges: return { &in.POD.BarLine.BeatTime, nullptr };
		case GenericList::GoGoRanges: return { &in.POD.GoGo.BeatTime, &in.POD.GoGo.BeatDuration };
		case GenericList::Lyrics: return { &in.NonTrivial.Lyric.BeatTime, nullptr };
		default: assert(false); return { nullptr, nullptr };
		}
	}

	Beat GenericListStruct::GetBeat(GenericList list) const
	{
		return *GetGenericListStructRawBeatPtr(*const_cast<GenericListStruct*>(this), list).Start;
	}

	Beat GenericListStruct::GetBeatDuration(GenericList list) const
	{
		const auto ptrs = GetGenericListStructRawBeatPtr(*const_cast<GenericListStruct*>(this), list);
		return (ptrs.Duration != nullptr) ? *ptrs.Duration : Beat::Zero();
	}

	void GenericListStruct::SetBeat(GenericList list, Beat newValue)
	{
		*GetGenericListStructRawBeatPtr(*this, list).Start = newValue;
	}

	size_t GetGenericMember_RawByteSize(GenericMember member)
	{
		switch (member)
		{
		case GenericMember::B8_IsSelected: return sizeof(b8);
		case GenericMember::B8_BarLineVisible: return sizeof(b8);
		case GenericMember::I16_BalloonPopCount: return sizeof(i16);
		case GenericMember::F32_ScrollSpeed: return sizeof(f32);
		case GenericMember::Beat_Start: return sizeof(Beat);
		case GenericMember::Beat_Duration: return sizeof(Beat);
		case GenericMember::Time_Offset: return sizeof(Time);
		case GenericMember::NoteType_V: return sizeof(NoteType);
		case GenericMember::Tempo_V: return sizeof(Tempo);
		case GenericMember::TimeSignature_V: return sizeof(TimeSignature);
		case GenericMember::CStr_Lyric: return sizeof(cstr);
		default: assert(false); return 0;
		}
	}

	size_t GetGenericListCount(const ChartCourse& course, GenericList list)
	{
		switch (list)
		{
		case GenericList::TempoChanges: return course.TempoMap.Tempo.size();
		case GenericList::SignatureChanges: return course.TempoMap.Signature.size();
		case GenericList::Notes_Normal: return course.Notes_Normal.size();
		case GenericList::Notes_Expert: return course.Notes_Expert.size();
		case GenericList::Notes_Master: return course.Notes_Master.size();
		case GenericList::ScrollChanges: return course.ScrollChanges.size();
		case GenericList::BarLineChanges: return course.BarLineChanges.size();
		case GenericList::GoGoRanges: return course.GoGoRanges.size();
		case GenericList::Lyrics: return course.Lyrics.size();
		default: assert(false); return 0;
		}
	}

	GenericMemberFlags GetAvailableMemberFlags(GenericList list)
	{
		switch (list)
		{
		case GenericList::TempoChanges:
			return GenericMemberFlags_IsSelected | GenericMemberFlags_Start | GenericMemberFlags_Tempo;
		case GenericList::SignatureChanges:
			return GenericMemberFlags_IsSelected | GenericMemberFlags_Start | GenericMemberFlags_TimeSignature;
		case GenericList::Notes_Normal:
		case GenericList::Notes_Expert:
		case GenericList::Notes_Master:
			return GenericMemberFlags_IsSelected | GenericMemberFlags_BalloonPopCount | GenericMemberFlags_Start | GenericMemberFlags_Duration | GenericMemberFlags_Offset | GenericMemberFlags_NoteType;
		case GenericList::ScrollChanges:
			return GenericMemberFlags_IsSelected | GenericMemberFlags_ScrollSpeed | GenericMemberFlags_Start;
		case GenericList::BarLineChanges:
			return GenericMemberFlags_IsSelected | GenericMemberFlags_BarLineVisible | GenericMemberFlags_Start;
		case GenericList::GoGoRanges:
			return GenericMemberFlags_IsSelected | GenericMemberFlags_Start | GenericMemberFlags_Duration;
		case GenericList::Lyrics:
			return GenericMemberFlags_IsSelected | GenericMemberFlags_Start | GenericMemberFlags_Lyric;
		default:
			assert(false); return GenericMemberFlags_None;
		}
	}

	void* TryGetGeneric_RawVoidPtr(const ChartCourse& course, GenericList list, size_t index, GenericMember member)
	{
		switch (list)
		{
		case GenericList::TempoChanges:
			if (auto& vector = *const_cast<SortedTempoChangesList*>(&course.TempoMap.Tempo); index < vector.size())
			{
				switch (member)
				{
				case GenericMember::B8_IsSelected: return &vector[index].IsSelected;
				case GenericMember::Beat_Start: return &vector[index].Beat;
				case GenericMember::Tempo_V: return &vector[index].Tempo;
				}
			} break;
		case GenericList::SignatureChanges:
			if (auto& vector = *const_cast<SortedSignatureChangesList*>(&course.TempoMap.Signature); index < vector.size())
			{
				switch (member)
				{
				case GenericMember::B8_IsSelected: return &vector[index].IsSelected;
				case GenericMember::Beat_Start: return &vector[index].Beat;
				case GenericMember::TimeSignature_V: return &vector[index].Signature;
				}
			} break;
		case GenericList::Notes_Normal:
		case GenericList::Notes_Expert:
		case GenericList::Notes_Master:
		{
			auto& vector = *const_cast<SortedNotesList*>(&(
				list == GenericList::Notes_Normal ? course.Notes_Normal :
				list == GenericList::Notes_Expert ? course.Notes_Expert : course.Notes_Master));

			if (index < vector.size())
			{
				switch (member)
				{
				case GenericMember::B8_IsSelected: return &vector[index].IsSelected;
				case GenericMember::I16_BalloonPopCount: return &vector[index].BalloonPopCount;
				case GenericMember::Beat_Start: return &vector[index].BeatTime;
				case GenericMember::Beat_Duration: return &vector[index].BeatDuration;
				case GenericMember::Time_Offset: return &vector[index].TimeOffset;
				case GenericMember::NoteType_V: return &vector[index].Type;
				}
			}
		} break;
		case GenericList::ScrollChanges:
			if (auto& vector = *const_cast<SortedScrollChangesList*>(&course.ScrollChanges); index < vector.size())
			{
				switch (member)
				{
				case GenericMember::B8_IsSelected: return &vector[index].IsSelected;
				case GenericMember::F32_ScrollSpeed: return &vector[index].ScrollSpeed;
				case GenericMember::Beat_Start: return &vector[index].BeatTime;
				}
			} break;
		case GenericList::BarLineChanges:
			if (auto& vector = *const_cast<SortedBarLineChangesList*>(&course.BarLineChanges); index < vector.size())
			{
				switch (member)
				{
				case GenericMember::B8_IsSelected: return &vector[index].IsSelected;
				case GenericMember::B8_BarLineVisible: return &vector[index].IsVisible;
				case GenericMember::Beat_Start: return &vector[index].BeatTime;
				}
			} break;
		case GenericList::GoGoRanges:
			if (auto& vector = *const_cast<SortedGoGoRangesList*>(&course.GoGoRanges); index < vector.size())
			{
				switch (member)
				{
				case GenericMember::B8_IsSelected: return &vector[index].IsSelected;
				case GenericMember::Beat_Start: return &vector[index].BeatTime;
				case GenericMember::Beat_Duration: return &vector[index].BeatDuration;
				}
			} break;
		case GenericList::Lyrics:
			if (auto& vector = *const_cast<SortedLyricsList*>(&course.Lyrics); index < vector.size())
			{
				switch (member)
				{
				case GenericMember::B8_IsSelected: return &vector[index].IsSelected;
				case GenericMember::Beat_Start: return &vector[index].BeatTime;
				case GenericMember::CStr_Lyric: return vector[index].Lyric.data();
				}
			} break;
		default:
			assert(false); break;
		}
		return nullptr;
	}

	b8 TryGetGeneric(const ChartCourse& course, GenericList list, size_t index, GenericMember member, GenericMemberUnion& outValue)
	{
		const void* voidMember = TryGetGeneric_RawVoidPtr(course, list, index, member);
		if (voidMember == nullptr)
			return false;

		if (member == GenericMember::CStr_Lyric)
			outValue.CStr = static_cast<cstr>(voidMember);
		else
			memcpy(&outValue, voidMember, GetGenericMember_RawByteSize(member));
		return true;
	}

	b8 TrySetGeneric(ChartCourse& course, GenericList list, size_t index, GenericMember member, const GenericMemberUnion& inValue)
	{
		void* voidMember = TryGetGeneric_RawVoidPtr(course, list, index, member);
		if (voidMember == nullptr)
			return false;

		if (member == GenericMember::CStr_Lyric)
			course.Lyrics[index].Lyric.assign(inValue.CStr);
		else
			memcpy(voidMember, &inValue, GetGenericMember_RawByteSize(member));
		return true;
	}

	b8 TryGetGenericStruct(const ChartCourse& course, GenericList list, size_t index, GenericListStruct& outValue)
	{
		switch (list)
		{
		case GenericList::TempoChanges: if (InBounds(index, course.TempoMap.Tempo)) { outValue.POD.Tempo = course.TempoMap.Tempo[index]; return true; } break;
		case GenericList::SignatureChanges: if (InBounds(index, course.TempoMap.Signature)) { outValue.POD.Signature = course.TempoMap.Signature[index]; return true; } break;
		case GenericList::Notes_Normal: if (InBounds(index, course.Notes_Normal)) { outValue.POD.Note = course.Notes_Normal[index]; return true; } break;
		case GenericList::Notes_Expert: if (InBounds(index, course.Notes_Expert)) { outValue.POD.Note = course.Notes_Expert[index]; return true; } break;
		case GenericList::Notes_Master: if (InBounds(index, course.Notes_Master)) { outValue.POD.Note = course.Notes_Master[index]; return true; } break;
		case GenericList::ScrollChanges: if (InBounds(index, course.ScrollChanges)) { outValue.POD.Scroll = course.ScrollChanges[index]; return true; } break;
		case GenericList::BarLineChanges: if (InBounds(index, course.BarLineChanges)) { outValue.POD.BarLine = course.BarLineChanges[index]; return true; } break;
		case GenericList::GoGoRanges: if (InBounds(index, course.GoGoRanges)) { outValue.POD.GoGo = course.GoGoRanges[index]; return true; } break;
		case GenericList::Lyrics: if (InBounds(index, course.Lyrics)) { outValue.NonTrivial.Lyric = course.Lyrics[index]; return true; } break;
		default: assert(false); break;
		}
		return false;
	}

	b8 TrySetGenericStruct(ChartCourse& course, GenericList list, size_t index, const GenericListStruct& inValue)
	{
		switch (list)
		{
		case GenericList::TempoChanges: if (InBounds(index, course.TempoMap.Tempo)) { course.TempoMap.Tempo[index] = inValue.POD.Tempo; return true; } break;
		case GenericList::SignatureChanges: if (InBounds(index, course.TempoMap.Signature)) { course.TempoMap.Signature[index] = inValue.POD.Signature; return true; } break;
		case GenericList::Notes_Normal: if (InBounds(index, course.Notes_Normal)) { course.Notes_Normal[index] = inValue.POD.Note; return true; } break;
		case GenericList::Notes_Expert: if (InBounds(index, course.Notes_Expert)) { course.Notes_Expert[index] = inValue.POD.Note; return true; } break;
		case GenericList::Notes_Master: if (InBounds(index, course.Notes_Master)) { course.Notes_Master[index] = inValue.POD.Note; return true; } break;
		case GenericList::ScrollChanges: if (InBounds(index, course.ScrollChanges)) { course.ScrollChanges[index] = inValue.POD.Scroll; return true; } break;
		case GenericList::BarLineChanges: if (InBounds(index, course.BarLineChanges)) { course.BarLineChanges[index] = inValue.POD.BarLine; return true; } break;
		case GenericList::GoGoRanges: if (InBounds(index, course.GoGoRanges)) { course.GoGoRanges[index] = inValue.POD.GoGo; return true; } break;
		case GenericList::Lyrics: if (InBounds(index, course.Lyrics)) { course.Lyrics[index] = inValue.NonTrivial.Lyric; return true; } break;
		default: assert(false); break;
		}
		return false;
	}

	b8 TryAddGenericStruct(ChartCourse& course, GenericList list, GenericListStruct inValue)
	{
		switch (list)
		{
		case GenericList::TempoChanges: { course.TempoMap.Tempo.InsertOrUpdate(inValue.POD.Tempo); return true; } break;
		case GenericList::SignatureChanges: { course.TempoMap.Signature.InsertOrUpdate(inValue.POD.Signature); return true; } break;
		case GenericList::Notes_Normal: { course.Notes_Normal.InsertOrUpdate(inValue.POD.Note); return true; } break;
		case GenericList::Notes_Expert: { course.Notes_Expert.InsertOrUpdate(inValue.POD.Note); return true; } break;
		case GenericList::Notes_Master: { course.Notes_Master.InsertOrUpdate(inValue.POD.Note); return true; } break;
		case GenericList::ScrollChanges: { course.ScrollChanges.InsertOrUpdate(inValue.POD.Scroll); return true; } break;
		case GenericList::BarLineChanges: { course.BarLineChanges.InsertOrUpdate(inValue.POD.BarLine); return true; } break;
		case GenericList::GoGoRanges: { course.GoGoRanges.InsertOrUpdate(inValue.POD.GoGo); return true; } break;
		case GenericList::Lyrics: { course.Lyrics.InsertOrUpdate(std::move(inValue.NonTrivial.Lyric)); return true; } break;
		default: assert(false); break;
		}
		return false;
	}

	b8 TryRemoveGenericStruct(ChartCourse& course, GenericList list, const GenericListStruct& inValueToRemove)
	{
		return TryRemoveGenericStruct(course, list, inValueToRemove.GetBeat(list));
	}

	b8 TryRemoveGenericStruct(ChartCourse& course, GenericList list, Beat beatToRemove)
	{
		switch (list)
		{
		case GenericList::TempoChanges: { course.TempoMap.Tempo.RemoveAtBeat(beatToRemove); return true; } break;
		case GenericList::SignatureChanges: { course.TempoMap.Signature.RemoveAtBeat(beatToRemove); return true; } break;
		case GenericList::Notes_Normal: { course.Notes_Normal.RemoveAtBeat(beatToRemove); return true; } break;
		case GenericList::Notes_Expert: { course.Notes_Expert.RemoveAtBeat(beatToRemove); return true; } break;
		case GenericList::Notes_Master: { course.Notes_Master.RemoveAtBeat(beatToRemove); return true; } break;
		case GenericList::ScrollChanges: { course.ScrollChanges.RemoveAtBeat(beatToRemove); return true; } break;
		case GenericList::BarLineChanges: { course.BarLineChanges.RemoveAtBeat(beatToRemove); return true; } break;
		case GenericList::GoGoRanges: { course.GoGoRanges.RemoveAtBeat(beatToRemove); return true; } break;
		case GenericList::Lyrics: { course.Lyrics.RemoveAtBeat(beatToRemove); return true; } break;
		default: assert(false); break;
		}
		return false;
	}
}
