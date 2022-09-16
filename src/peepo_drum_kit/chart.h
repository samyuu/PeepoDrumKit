#pragma once
#include "core_types.h"
#include "core_string.h"
#include "core_beat.h"
#include "file_format_tja.h"
#include <unordered_map>

namespace PeepoDrumKit
{
	enum class NoteType : u8
	{
		// NOTE: Regular notes
		Don,
		DonBig,
		Ka,
		KaBig,
		// NOTE: Long notes
		Drumroll,
		DrumrollBig,
		Balloon,
		BalloonSpecial,
		// ...
		Count
	};

	constexpr b8 IsDonNote(NoteType v) { return (v == NoteType::Don) || (v == NoteType::DonBig); }
	constexpr b8 IsKaNote(NoteType v) { return (v == NoteType::Ka) || (v == NoteType::KaBig); }
	constexpr b8 IsSmallNote(NoteType v) { return (v == NoteType::Don) || (v == NoteType::Ka) || (v == NoteType::Drumroll) || (v == NoteType::Balloon); }
	constexpr b8 IsBigNote(NoteType v) { return !IsSmallNote(v); }
	constexpr b8 IsDrumrollNote(NoteType v) { return (v == NoteType::Drumroll) || (v == NoteType::DrumrollBig); }
	constexpr b8 IsBalloonNote(NoteType v) { return (v == NoteType::Balloon) || (v == NoteType::BalloonSpecial); }
	constexpr b8 IsLongNote(NoteType v) { return IsDrumrollNote(v) || IsBalloonNote(v); }
	constexpr b8 IsRegularNote(NoteType v) { return !IsLongNote(v); }
	constexpr NoteType ToSmallNote(NoteType v)
	{
		switch (v)
		{
		case NoteType::Don: return NoteType::Don;
		case NoteType::DonBig: return NoteType::Don;
		case NoteType::Ka: return NoteType::Ka;
		case NoteType::KaBig: return NoteType::Ka;
		case NoteType::Drumroll: return NoteType::Drumroll;
		case NoteType::DrumrollBig: return NoteType::Drumroll;
		case NoteType::Balloon: return NoteType::Balloon;
		case NoteType::BalloonSpecial: return NoteType::Balloon;
		default: return v;
		}
	}
	constexpr NoteType ToBigNote(NoteType v)
	{
		switch (v)
		{
		case NoteType::Don: return NoteType::DonBig;
		case NoteType::DonBig: return NoteType::DonBig;
		case NoteType::Ka: return NoteType::KaBig;
		case NoteType::KaBig: return NoteType::KaBig;
		case NoteType::Drumroll: return NoteType::DrumrollBig;
		case NoteType::DrumrollBig: return NoteType::DrumrollBig;
		case NoteType::Balloon: return NoteType::BalloonSpecial;
		case NoteType::BalloonSpecial: return NoteType::BalloonSpecial;
		default: return v;
		}
	}
	constexpr NoteType ToggleNoteSize(NoteType v) { return IsSmallNote(v) ? ToBigNote(v) : ToSmallNote(v); }
	constexpr NoteType ToSmallNoteIf(NoteType v, b8 condition) { return condition ? ToSmallNote(v) : v; }
	constexpr NoteType ToBigNoteIf(NoteType v, b8 condition) { return condition ? ToBigNote(v) : v; }
	constexpr NoteType FlipNote(NoteType v)
	{
		switch (v)
		{
		case NoteType::Don: return NoteType::Ka;
		case NoteType::DonBig: return NoteType::KaBig;
		case NoteType::Ka: return NoteType::Don;
		case NoteType::KaBig: return NoteType::DonBig;
		default: return v;
		}
	}
	constexpr bool IsNoteFlippable(NoteType v) { return FlipNote(v) != v; }

	constexpr i32 DefaultBalloonPopCount(Beat beatDuration, i32 gridBarDivision) { return (beatDuration.Ticks / GetGridBeatSnap(gridBarDivision).Ticks); }

	enum class DifficultyType : u8
	{
		Easy,
		Normal,
		Hard,
		Oni,
		OniUra,
		Count
	};

	constexpr cstr DifficultyTypeNames[EnumCount<DifficultyType>] =
	{
		"Easy",
		"Normal",
		"Hard",
		"Oni",
		"Oni-Ura",
	};

	enum class DifficultyLevel : u8
	{
		Min = 1,
		Max = 10
	};

	enum class BranchType : u8
	{
		Normal,
		Expert,
		Master,
		Count
	};

	// TODO: Animations for create / delete AND for moving left / right (?)
	struct Note
	{
		Beat BeatTime;
		Beat BeatDuration;
		Time TimeOffset;
		NoteType Type;
		b8 IsSelected;
		i16 BalloonPopCount;
		f32 ClickAnimationTimeRemaining;
		f32 ClickAnimationTimeDuration;

		constexpr Beat GetStart() const { return BeatTime; }
		constexpr Beat GetEnd() const { return BeatTime + BeatDuration; }
	};

	static_assert(sizeof(Note) == 32, "Accidentally introduced padding to Note struct (?)");

	struct ScrollChange
	{
		Beat BeatTime;
		f32 ScrollSpeed;
		b8 IsSelected;
	};

	struct BarLineChange
	{
		Beat BeatTime;
		b8 IsVisible;
		b8 IsSelected;
	};

	struct GoGoRange
	{
		Beat BeatTime;
		Beat BeatDuration;
		b8 IsSelected;
		f32 ExpansionAnimationCurrent = 0.0f;
		f32 ExpansionAnimationTarget = 1.0f;

		constexpr Beat GetStart() const { return BeatTime; }
		constexpr Beat GetEnd() const { return BeatTime + BeatDuration; }
	};

	struct LyricChange
	{
		Beat BeatTime;
		std::string Lyric;
		b8 IsSelected;
	};

	using SortedNotesList = BeatSortedList<Note>;
	using SortedScrollChangesList = BeatSortedList<ScrollChange>;
	using SortedBarLineChangesList = BeatSortedList<BarLineChange>;
	using SortedGoGoRangesList = BeatSortedList<GoGoRange>;
	using SortedLyricsList = BeatSortedList<LyricChange>;

	constexpr Beat GetBeat(const Note& v) { return v.BeatTime; }
	constexpr Beat GetBeat(const ScrollChange& v) { return v.BeatTime; }
	constexpr Beat GetBeat(const BarLineChange& v) { return v.BeatTime; }
	constexpr Beat GetBeat(const GoGoRange& v) { return v.BeatTime; }
	constexpr Beat GetBeat(const LyricChange& v) { return v.BeatTime; }
	constexpr Beat GetBeatDuration(const Note& v) { return v.BeatDuration; }
	constexpr Beat GetBeatDuration(const ScrollChange& v) { return Beat::Zero(); }
	constexpr Beat GetBeatDuration(const BarLineChange& v) { return Beat::Zero(); }
	constexpr Beat GetBeatDuration(const GoGoRange& v) { return v.BeatDuration; }
	constexpr Beat GetBeatDuration(const LyricChange& v) { return Beat::Zero(); }

	constexpr Tempo ScrollSpeedToTempo(f32 scrollSpeed, Tempo baseTempo) { return Tempo(scrollSpeed * baseTempo.BPM); }
	constexpr f32 ScrollTempoToSpeed(Tempo scrollTempo, Tempo baseTempo) { return (baseTempo.BPM == 0.0f) ? 0.0f : (scrollTempo.BPM / baseTempo.BPM); }

	constexpr b8 VisibleOrDefault(const BarLineChange* v) { return (v == nullptr) ? true : v->IsVisible; }
	constexpr f32 ScrollOrDefault(const ScrollChange* v) { return (v == nullptr) ? 1.0f : v->ScrollSpeed; }
	constexpr Tempo TempoOrDefault(const TempoChange* v) { return (v == nullptr) ? FallbackTempo : v->Tempo; }

	enum class Language : u8 { Base, JA, EN, CN, TW, KO, Count };
	struct PerLanguageString
	{
		std::string Slots[EnumCount<Language>];

		inline auto& Base() { return Slots[EnumToIndex(Language::Base)]; }
		inline auto& Base() const { return Slots[EnumToIndex(Language::Base)]; }
		inline auto* begin() { return &Slots[0]; }
		inline auto* end() { return &Slots[0] + ArrayCount(Slots); }
		inline auto* begin() const { return &Slots[0]; }
		inline auto* end() const { return &Slots[0] + ArrayCount(Slots); }
		inline auto& operator[](Language v) { return Slots[EnumToIndex(v)]; }
		inline auto& operator[](Language v) const { return Slots[EnumToIndex(v)]; }
	};

	struct ChartCourse
	{
		DifficultyType Type = DifficultyType::Oni;
		DifficultyLevel Level = DifficultyLevel { 1 };

		SortedTempoMap TempoMap;

		// TODO: Have per-branch scroll speed changes (?)
		SortedNotesList Notes_Normal;
		SortedNotesList Notes_Expert;
		SortedNotesList Notes_Master;

		SortedScrollChangesList ScrollChanges;
		SortedBarLineChangesList BarLineChanges;
		SortedGoGoRangesList GoGoRanges;
		SortedLyricsList Lyrics;

		i32 ScoreInit = 0;
		i32 ScoreDiff = 0;

		inline auto& GetNotes(BranchType branch) { assert(branch < BranchType::Count); return (&Notes_Normal)[EnumToIndex(branch)]; }
		inline auto& GetNotes(BranchType branch) const { assert(branch < BranchType::Count); return (&Notes_Normal)[EnumToIndex(branch)]; }
	};

	// NOTE: Internal representation of a chart. Can then be imported / exported as .tja (and maybe as the native fumen binary format too eventually?)
	struct ChartProject
	{
		std::vector<std::unique_ptr<ChartCourse>> Courses;

		Time ChartDuration = {};
		PerLanguageString ChartTitle;
		PerLanguageString ChartSubtitle;
		std::string ChartCreator;
		std::string ChartGenre;
		std::string ChartLyricsFileName;

		Time SongOffset = {};
		Time SongDemoStartTime = {};
		std::string SongFileName;

		f32 SongVolume = 1.0f;
		f32 SoundEffectVolume = 1.0f;

		std::string BackgroundImageFileName;
		std::string BackgroundMovieFileName;
		Time MovieOffset = {};

		// TODO: Maybe change to GetDurationOr(Time defaultDuration) and always pass in context.SongDuration (?)
		inline Time GetDurationOrDefault() const { return (ChartDuration.Seconds <= 0.0) ? Time::FromMin(1.0) : ChartDuration; }
	};

	// NOTE: Chart Space -> Starting at 00:00.000 (as most internal calculations are done in)
	//		  Song Space -> Starting relative to Song Offset (sometimes useful for displaying to the user)
	enum class TimeSpace : u8 { Chart, Song };
	constexpr Time ChartToSongTimeSpace(Time inTime, Time songOffset) { return (inTime - songOffset); }
	constexpr Time SongToChartTimeSpace(Time inTime, Time songOffset) { return (inTime + songOffset); }
	constexpr Time ConvertTimeSpace(Time v, TimeSpace in, TimeSpace out, Time songOffset) { v = (in == out) ? v : (in == TimeSpace::Chart) ? (v - songOffset) : (v + songOffset); return (v == Time { -0.0 }) ? Time {} : v; }
	constexpr Time ConvertTimeSpace(Time v, TimeSpace in, TimeSpace out, const ChartProject& chart) { return ConvertTimeSpace(v, in, out, chart.SongOffset); }

	Beat FindCourseMaxUsedBeat(const ChartCourse& course);
	b8 CreateChartProjectFromTJA(const TJA::ParsedTJA& inTJA, ChartProject& out);
	b8 ConvertChartProjectToTJA(const ChartProject& in, TJA::ParsedTJA& out, b8 includePeepoDrumKitComment = true);
}

namespace PeepoDrumKit
{
	enum class GenericList : u8
	{
		TempoChanges,
		SignatureChanges,
		Notes_Normal,
		Notes_Expert,
		Notes_Master,
		ScrollChanges,
		BarLineChanges,
		GoGoRanges,
		Lyrics,
		Count
	};

	enum class GenericMember : u8
	{
		B8_IsSelected,
		B8_BarLineVisible,
		I16_BalloonPopCount,
		F32_ScrollSpeed,
		Beat_Start,
		Beat_Duration,
		Time_Offset,
		NoteType_V,
		Tempo_V,
		TimeSignature_V,
		CStr_Lyric,
		Count
	};

	using GenericMemberFlags = u32;
	constexpr GenericMemberFlags EnumToFlag(GenericMember type) { return (1u << static_cast<u32>(type)); }
	enum GenericMemberFlagsEnum : GenericMemberFlags
	{
		GenericMemberFlags_None = 0,
		GenericMemberFlags_IsSelected = EnumToFlag(GenericMember::B8_IsSelected),
		GenericMemberFlags_BarLineVisible = EnumToFlag(GenericMember::B8_BarLineVisible),
		GenericMemberFlags_BalloonPopCount = EnumToFlag(GenericMember::I16_BalloonPopCount),
		GenericMemberFlags_ScrollSpeed = EnumToFlag(GenericMember::F32_ScrollSpeed),
		GenericMemberFlags_Start = EnumToFlag(GenericMember::Beat_Start),
		GenericMemberFlags_Duration = EnumToFlag(GenericMember::Beat_Duration),
		GenericMemberFlags_Offset = EnumToFlag(GenericMember::Time_Offset),
		GenericMemberFlags_NoteType = EnumToFlag(GenericMember::NoteType_V),
		GenericMemberFlags_Tempo = EnumToFlag(GenericMember::Tempo_V),
		GenericMemberFlags_TimeSignature = EnumToFlag(GenericMember::TimeSignature_V),
		GenericMemberFlags_Lyric = EnumToFlag(GenericMember::CStr_Lyric),
		GenericMemberFlags_All = 0b11111111111,
	};

	static_assert(GenericMemberFlags_All & (1u << (static_cast<u32>(GenericMember::Count) - 1)));
	static_assert(!(GenericMemberFlags_All & (1u << static_cast<u32>(GenericMember::Count))));

	constexpr cstr GenericListNames[] = { "TempoChanges", "SignatureChanges", "Notes_Normal", "Notes_Expert", "Notes_Master", "ScrollChanges", "BarLineChanges", "GoGoRanges", "Lyrics", };
	constexpr cstr GenericMemberNames[] = { "IsSelected", "BarLineVisible", "BalloonPopCount", "ScrollSpeed", "Start", "Duration", "Offset", "NoteType", "Tempo", "TimeSignature", "Lyric", };

	union GenericMemberUnion
	{
		b8 B8;
		i16 I16;
		f32 F32;
		Beat Beat;
		Time Time;
		NoteType NoteType;
		Tempo Tempo;
		TimeSignature TimeSignature;
		cstr CStr;

		inline GenericMemberUnion() { ::memset(this, 0, sizeof(*this)); }
		inline b8 operator==(const GenericMemberUnion& other) const { return (::memcmp(this, &other, sizeof(*this)) == 0); }
		inline b8 operator!=(const GenericMemberUnion& other) const { return !(*this == other); }
	};

	static_assert(sizeof(GenericMemberUnion) == 8);

	struct AllGenericMembersUnionArray
	{
		GenericMemberUnion V[EnumCount<GenericMember>];

		inline GenericMemberUnion& operator[](GenericMember member) { return V[EnumToIndex(member)]; }
		inline const GenericMemberUnion& operator[](GenericMember member) const { return V[EnumToIndex(member)]; }

		inline auto& IsSelected() { return (*this)[GenericMember::B8_IsSelected].B8; }
		inline auto& BarLineVisible() { return (*this)[GenericMember::B8_BarLineVisible].B8; }
		inline auto& BalloonPopCount() { return (*this)[GenericMember::I16_BalloonPopCount].I16; }
		inline auto& ScrollSpeed() { return (*this)[GenericMember::F32_ScrollSpeed].F32; }
		inline auto& BeatStart() { return (*this)[GenericMember::Beat_Start].Beat; }
		inline auto& BeatDuration() { return (*this)[GenericMember::Beat_Duration].Beat; }
		inline auto& TimeOffset() { return (*this)[GenericMember::Time_Offset].Time; }
		inline auto& NoteType() { return (*this)[GenericMember::NoteType_V].NoteType; }
		inline auto& Tempo() { return (*this)[GenericMember::Tempo_V].Tempo; }
		inline auto& TimeSignature() { return (*this)[GenericMember::TimeSignature_V].TimeSignature; }
		inline auto& Lyric() { return (*this)[GenericMember::CStr_Lyric].CStr; }
		inline const auto& IsSelected() const { return (*this)[GenericMember::B8_IsSelected].B8; }
		inline const auto& BarLineVisible() const { return (*this)[GenericMember::B8_BarLineVisible].B8; }
		inline const auto& BalloonPopCount() const { return (*this)[GenericMember::I16_BalloonPopCount].I16; }
		inline const auto& ScrollSpeed() const { return (*this)[GenericMember::F32_ScrollSpeed].F32; }
		inline const auto& BeatStart() const { return (*this)[GenericMember::Beat_Start].Beat; }
		inline const auto& BeatDuration() const { return (*this)[GenericMember::Beat_Duration].Beat; }
		inline const auto& TimeOffset() const { return (*this)[GenericMember::Time_Offset].Time; }
		inline const auto& NoteType() const { return (*this)[GenericMember::NoteType_V].NoteType; }
		inline const auto& Tempo() const { return (*this)[GenericMember::Tempo_V].Tempo; }
		inline const auto& TimeSignature() const { return (*this)[GenericMember::TimeSignature_V].TimeSignature; }
		inline const auto& Lyric() const { return (*this)[GenericMember::CStr_Lyric].CStr; }
	};

	struct GenericListStruct
	{
		union PODData
		{
			TempoChange Tempo;
			TimeSignatureChange Signature;
			Note Note;
			ScrollChange Scroll;
			BarLineChange BarLine;
			GoGoRange GoGo;

			inline PODData() { ::memset(this, 0, sizeof(*this)); }
		} POD;

		// NOTE: Handle separately due to constructor / destructor requirement
		struct NonTrivialData
		{
			LyricChange Lyric;
		} NonTrivial {};

		// NOTE: Little helpers here just for convenience
		Beat GetBeat(GenericList list) const;
		Beat GetBeatDuration(GenericList list) const;
		void SetBeat(GenericList list, Beat newValue);
	};

	struct GenericListStructWithType
	{
		GenericList List;
		GenericListStruct Value;

		inline Beat GetBeat() const { return Value.GetBeat(List); }
		inline Beat GetBeatDuration() const { return Value.GetBeatDuration(List); }
		inline void SetBeat(Beat newValue) { Value.SetBeat(List, newValue); }
	};

	constexpr b8 IsNotesList(GenericList list) { return (list == GenericList::Notes_Normal) || (list == GenericList::Notes_Expert) || (list == GenericList::Notes_Master); }
	constexpr b8 ListHasDurations(GenericList list) { return IsNotesList(list) || (list == GenericList::GoGoRanges); }
	constexpr b8 ListUsesInclusiveBeatCheck(GenericList list) { return IsNotesList(list) || (list != GenericList::GoGoRanges && list != GenericList::Lyrics); }

	size_t GetGenericMember_RawByteSize(GenericMember member);
	size_t GetGenericListCount(const ChartCourse& course, GenericList list);
	GenericMemberFlags GetAvailableMemberFlags(GenericList list);

	void* TryGetGeneric_RawVoidPtr(const ChartCourse& course, GenericList list, size_t index, GenericMember member);
	b8 TryGetGeneric(const ChartCourse& course, GenericList list, size_t index, GenericMember member, GenericMemberUnion& outValue);
	b8 TrySetGeneric(ChartCourse& course, GenericList list, size_t index, GenericMember member, const GenericMemberUnion& inValue);

	b8 TryGetGenericStruct(const ChartCourse& course, GenericList list, size_t index, GenericListStruct& outValue);
	b8 TrySetGenericStruct(ChartCourse& course, GenericList list, size_t index, const GenericListStruct& inValue);
	b8 TryAddGenericStruct(ChartCourse& course, GenericList list, GenericListStruct inValue);
	b8 TryRemoveGenericStruct(ChartCourse& course, GenericList list, const GenericListStruct& inValueToRemove);
	b8 TryRemoveGenericStruct(ChartCourse& course, GenericList list, Beat beatToRemove);

	struct ForEachChartItemData
	{
		GenericList List;
		size_t Index;

		// NOTE: Again just little accessor helpers for the members that should always be available for each list type
		inline b8 GetIsSelected(const ChartCourse& c) const { GenericMemberUnion v {}; TryGetGeneric(c, List, Index, GenericMember::B8_IsSelected, v); return v.B8; }
		inline void SetIsSelected(ChartCourse& c, b8 isSelected) const { GenericMemberUnion v {}; v.B8 = isSelected; TrySetGeneric(c, List, Index, GenericMember::B8_IsSelected, v); }
		inline Beat GetBeat(const ChartCourse& c) const { GenericMemberUnion v {}; TryGetGeneric(c, List, Index, GenericMember::Beat_Start, v); return v.Beat; }
		inline Beat GetBeatDuration(const ChartCourse& c) const { GenericMemberUnion v {}; TryGetGeneric(c, List, Index, GenericMember::Beat_Duration, v); return v.Beat; }
		inline void SetBeat(ChartCourse& c, Beat beat) const { GenericMemberUnion v {}; v.Beat = beat; TrySetGeneric(c, List, Index, GenericMember::Beat_Start, v); }
	};

	template <typename Func>
	void ForEachChartItem(const ChartCourse& course, Func perItemFunc)
	{
		for (GenericList list = {}; list < GenericList::Count; IncrementEnum(list))
		{
			for (size_t i = 0; i < GetGenericListCount(course, list); i++)
				perItemFunc(ForEachChartItemData { list, i });
		}
	}

	template <typename Func>
	void ForEachSelectedChartItem(const ChartCourse& course, Func perSelectedItemFunc)
	{
#if 0
		for (GenericList list = {}; list < GenericList::Count; IncrementEnum(list))
		{
			for (size_t i = 0; i < GetGenericListCount(course, list); i++)
				if (GenericMemberUnion value; TryGetGeneric(course, list, i, GenericMember::B8_IsSelected, value) && value.B8)
					perSelectedItemFunc(ForEachChartItemData { list, i });
		}
#else // NOTE: Manually unrolled to avoid redundant inner branches
		for (size_t i = 0; i < course.TempoMap.Tempo.size(); i++) if (course.TempoMap.Tempo[i].IsSelected) perSelectedItemFunc(ForEachChartItemData { GenericList::TempoChanges, i });
		for (size_t i = 0; i < course.TempoMap.Signature.size(); i++) if (course.TempoMap.Signature[i].IsSelected) perSelectedItemFunc(ForEachChartItemData { GenericList::SignatureChanges, i });
		for (size_t i = 0; i < course.Notes_Normal.size(); i++) if (course.Notes_Normal[i].IsSelected) perSelectedItemFunc(ForEachChartItemData { GenericList::Notes_Normal, i });
		for (size_t i = 0; i < course.Notes_Expert.size(); i++) if (course.Notes_Expert[i].IsSelected) perSelectedItemFunc(ForEachChartItemData { GenericList::Notes_Expert, i });
		for (size_t i = 0; i < course.Notes_Master.size(); i++) if (course.Notes_Master[i].IsSelected) perSelectedItemFunc(ForEachChartItemData { GenericList::Notes_Master, i });
		for (size_t i = 0; i < course.ScrollChanges.size(); i++) if (course.ScrollChanges[i].IsSelected) perSelectedItemFunc(ForEachChartItemData { GenericList::ScrollChanges, i });
		for (size_t i = 0; i < course.BarLineChanges.size(); i++) if (course.BarLineChanges[i].IsSelected) perSelectedItemFunc(ForEachChartItemData { GenericList::BarLineChanges, i });
		for (size_t i = 0; i < course.GoGoRanges.size(); i++) if (course.GoGoRanges[i].IsSelected) perSelectedItemFunc(ForEachChartItemData { GenericList::GoGoRanges, i });
		for (size_t i = 0; i < course.Lyrics.size(); i++) if (course.Lyrics[i].IsSelected) perSelectedItemFunc(ForEachChartItemData { GenericList::Lyrics, i });
		static_assert(EnumCount<GenericList> == 9);
#endif
	}
}
