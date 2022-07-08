#pragma once
#include "core_types.h"
#include "core_string.h"
#include "core_beat.h"
#include "tja_file_format.h"
#include <unordered_map>

namespace PeepoDrumKit
{
	// NOTE: Stable ID to be used by for example undoable commands to keeps references instead of indices / pointers
	enum class StableID : u32 { Null = 0 };

	inline StableID GenerateNewStableID() { static u32 globalCounter = 0; return static_cast<StableID>(++globalCounter); }

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
		BaloonSpecial,
		// ...
		Count
	};

	constexpr bool IsDonNote(NoteType v) { return (v == NoteType::Don) || (v == NoteType::DonBig); }
	constexpr bool IsKaNote(NoteType v) { return (v == NoteType::Ka) || (v == NoteType::KaBig); }
	constexpr bool IsBigNote(NoteType v) { return (v == NoteType::DonBig) || (v == NoteType::KaBig) || (v == NoteType::DrumrollBig) || (v == NoteType::BaloonSpecial); }
	constexpr bool IsDrumrollNote(NoteType v) { return (v == NoteType::Drumroll) || (v == NoteType::DrumrollBig); }
	constexpr bool IsBalloonNote(NoteType v) { return (v == NoteType::Balloon) || (v == NoteType::BaloonSpecial); }
	constexpr bool IsLongNote(NoteType v) { return IsDrumrollNote(v) || IsBalloonNote(v); }

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
		case NoteType::BaloonSpecial: return NoteType::Balloon;
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
		case NoteType::Balloon: return NoteType::BaloonSpecial;
		case NoteType::BaloonSpecial: return NoteType::BaloonSpecial;
		default: return v;
		}
	}

	constexpr NoteType ToSmallNoteIf(NoteType v, bool condition) { return condition ? ToSmallNote(v) : v; }
	constexpr NoteType ToBigNoteIf(NoteType v, bool condition) { return condition ? ToBigNote(v) : v; }

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

	constexpr const char* DifficultyTypeNames[EnumCount<DifficultyType>] =
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
		i16 BalloonPopCount;
		NoteType Type;
		bool IsSelected;
		StableID StableID;
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
		// StableID StableID;
	};

	struct BarLineChange
	{
		Beat BeatTime;
		bool IsVisible;
		// StableID StableID;
	};

	struct GoGoRange
	{
		Beat BeatTime;
		Beat BeatDuration;
		// StableID StableID;
		f32 ExpansionAnimationCurrent = 0.0f;
		f32 ExpansionAnimationTarget = 1.0f;

		constexpr Beat GetStart() const { return BeatTime; }
		constexpr Beat GetEnd() const { return BeatTime + BeatDuration; }
	};

	struct LyricChange
	{
		Beat BeatTime;
		std::string Lyric;
	};

	struct SortedNotesList
	{
		std::vector<Note> Sorted;
		std::unordered_map<StableID, i32> IDToNoteIndexMap;

	public:
		StableID Add(Note newNote);
		Note RemoveID(StableID id);
		Note RemoveIndex(i32 index);

		i32 FindIDIndex(StableID id) const;
		i32 FindExactBeatIndex(Beat beat) const;
		i32 FindOverlappingBeatIndex(Beat beatStart, Beat beatEnd) const;
		Note* TryFindID(StableID id);
		Note* TryFindExactAtBeat(Beat beat);
		Note* TryFindOverlappingBeat(Beat beat);
		Note* TryFindOverlappingBeat(Beat beatStart, Beat beatEnd);
		const Note* TryFindID(StableID id) const;
		const Note* TryFindExactAtBeat(Beat beat) const;
		const Note* TryFindOverlappingBeat(Beat beat) const;
		const Note* TryFindOverlappingBeat(Beat beatStart, Beat beatEnd) const;

		void Clear();

		inline auto begin() { return Sorted.begin(); }
		inline auto end() { return Sorted.end(); }
		inline auto begin() const { return Sorted.begin(); }
		inline auto end() const { return Sorted.end(); }
		inline size_t size() const { return Sorted.size(); }
		inline Note& operator[](size_t index) { return Sorted[index]; }
		inline const Note& operator[](size_t index) const { return Sorted[index]; }
	};

	template <typename T>
	struct BeatSortedList
	{
		std::vector<T> Sorted;

	public:
		T* TryFindLastAtBeat(Beat beat);
		T* TryFindExactAtBeat(Beat beat);
		const T* TryFindLastAtBeat(Beat beat) const;
		const T* TryFindExactAtBeat(Beat beat) const;
		void InsertOrUpdate(T changeToInsertOrUpdate);
		void RemoveAtBeat(Beat beatToFindAndRemove);
		void RemoveAtIndex(size_t indexToRemove);

		inline bool empty() const { return Sorted.empty(); }
		inline auto begin() { return Sorted.begin(); }
		inline auto end() { return Sorted.end(); }
		inline auto begin() const { return Sorted.begin(); }
		inline auto end() const { return Sorted.end(); }
		inline size_t size() const { return Sorted.size(); }
		inline T& operator[](size_t index) { return Sorted[index]; }
		inline const T& operator[](size_t index) const { return Sorted[index]; }
	};

	// NOTE: Only these template types will be explicitly instantiated to keep the header nice and clean
	using SortedScrollChangesList = BeatSortedList<ScrollChange>;
	using SortedBarLineChangesList = BeatSortedList<BarLineChange>;
	using SortedGoGoRangesList = BeatSortedList<GoGoRange>;
	using SortedLyricsList = BeatSortedList<LyricChange>;

	constexpr Beat GetBeat(const Note& v) { return v.BeatTime; }
	constexpr Beat GetBeat(const ScrollChange& v) { return v.BeatTime; }
	constexpr Beat GetBeat(const BarLineChange& v) { return v.BeatTime; }
	constexpr Beat GetBeat(const GoGoRange& v) { return v.BeatTime; }
	constexpr Beat GetBeat(const LyricChange& v) { return v.BeatTime; }

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

		inline Time GetDurationOrDefault() const { return (ChartDuration.Seconds <= 0.0) ? Time::FromMinutes(1.0) : ChartDuration; }
	};

	Beat FindCourseMaxUsedBeat(const ChartCourse& course);
	bool CreateChartProjectFromTJA(const TJA::ParsedTJA& inTJA, ChartProject& out);
	bool ConvertChartProjectToTJA(const ChartProject& in, TJA::ParsedTJA& out);
}
