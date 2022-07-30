#pragma once
#include "core_types.h"
#include "core_io.h"
#include "core_string.h"
#include "core_beat.h"
#include <vector>
#include <cstdarg>

// NOTE: "Token" -> smallest atomic piece of data.
//		 A list of tokens basically losslessly represents a TJA file and exists to make parsing easier.
//
//		 "Parsing" a list of tokens results in plain metadata structs with all of the property keys resolved
//		 as well as a list of chart commands per difficulty course. (comments however are not stored)
//		 Parsed data can represent any well formatted TJA (given that it doesn't contain unknown commands, metadata or ill formatted parameters)
//		 even if it contains bad chart commands such as improper branches.
//
//		 "Converting" a parsed TJA then results in an easy to access / modify chart data representation
//		 which has beat timings converted to fixed point units, empty notes removed and tempo changes etc. are no longer interleaved.
namespace TJA
{
	static constexpr std::string_view Extension = ".tja";
	static constexpr std::string_view FilterName = "TJA Taiko Chart";
	static constexpr std::string_view FilterSpec = "*.tja";

	enum class Encoding : u8 { Unknown, UTF8, ShiftJIS };

	enum class TokenType : u8
	{
		Unknown,

		// NOTE: ...
		EmptyLine,
		// NOTE: // comment
		Comment,
		// NOTE: KEY:VALUE
		KeyColonValue,
		// NOTE: #COMMAND
		HashChartCommand,
		// NOTE: 10101010,
		ChartData,

		Count
	};

	enum class Key : u8
	{
		Unknown,

		// NOTE: Once per file
		Main_TITLE,
		Main_TITLEJA,
		Main_TITLEEN,
		Main_TITLECN,
		Main_TITLETW,
		Main_TITLEKO,
		Main_SUBTITLE,
		Main_SUBTITLEJA,
		Main_SUBTITLEEN,
		Main_SUBTITLECN,
		Main_SUBTITLETW,
		Main_SUBTITLEKO,
		Main_BPM,
		Main_WAVE,
		Main_OFFSET,
		Main_DEMOSTART,
		Main_GENRE,
		Main_SCOREMODE,
		Main_MAKER,
		Main_LYRICS,
		Main_SONGVOL,
		Main_SEVOL,
		Main_SIDE,
		Main_LIFE,
		Main_GAME,
		Main_HEADSCROLL,
		Main_BGIMAGE,
		Main_BGMOVIE,
		Main_MOVIEOFFSET,
		Main_TAIKOWEBSKIN,

		// NOTE: Once per difficulty course
		Course_COURSE,
		Course_LEVEL,
		Course_BALLOON,
		Course_SCOREINIT,
		Course_SCOREDIFF,
		Course_BALLOONNOR,
		Course_BALLOONEXP,
		Course_BALLOONMAS,
		Course_STYLE,
		Course_EXAM1,
		Course_EXAM2,
		Course_EXAM3,
		Course_GAUGEINCR,
		Course_TOTAL,
		Course_HIDDENBRANCH,

		// NOTE: Chart song notation
		Chart_START,
		Chart_END,
		Chart_MEASURE,
		Chart_BPMCHANGE,
		Chart_DELAY,
		Chart_SCROLL,
		Chart_GOGOSTART,
		Chart_GOGOEND,
		Chart_BARLINEOFF,
		Chart_BARLINEON,
		Chart_BRANCHSTART,
		Chart_N,
		Chart_E,
		Chart_M,
		Chart_BRANCHEND,
		Chart_SECTION,
		Chart_LYRIC,
		Chart_LEVELHOLD,
		Chart_BMSCROLL,
		Chart_HBSCROLL,
		Chart_SENOTECHANGE,
		Chart_NEXTSONG,
		Chart_DIRECTION,
		Chart_SUDDEN,
		Chart_JPOSSCROLL,
		Count,

		Main_First = Main_TITLE,
		Main_Last = Main_TAIKOWEBSKIN,
		Course_First = Course_COURSE,
		Course_Last = Course_HIDDENBRANCH,
		Chart_First = Chart_START,
		Chart_Last = Chart_JPOSSCROLL,

		KeyColonValue_First = Main_First,
		KeyColonValue_Last = Course_Last,
		HashCommand_First = Chart_First,
		HashCommand_Last = Chart_Last,
	};

	struct Token
	{
		TokenType Type;
		Key Key;
		i16 LineIndex;
		std::string_view Line;
		std::string_view KeyString;
		std::string_view ValueString;
	};

	constexpr Tempo DefaultTempo = Tempo(120.0f);

	constexpr TimeSignature DefaultTimeSignature = TimeSignature(4, 4);

	enum class DifficultyType : u8
	{
		Easy,
		Normal,
		Hard,
		Oni,
		OniUra,
		Tower,
		Dan,
		Count
	};

	enum class NoteType : u8
	{
		// NOTE: 休符
		None,

		// NOTE: ドン
		Don,
		// NOTE: カツ
		Ka,
		// NOTE: ドン（大）
		DonBig,
		// NOTE: カツ（大）
		KaBig,

		// NOTE: 連打開始
		Start_Drumroll,
		// NOTE: 連打（大）開始
		Start_DrumrollBig,
		// NOTE: ゲキ連打開始
		Start_Balloon,
		// NOTE: 連打終了
		End_BalloonOrDrumroll,
		// NOTE: イモ連打開始
		Start_BaloonSpecial,

		// NOTE: Multiplayer
		DonBigBoth,
		KaBigBoth,
		Hidden,

		Count
	};

	constexpr b8 IsBigNote(NoteType note)
	{
		return
			(note == NoteType::DonBig) || (note == NoteType::KaBig) ||
			(note == NoteType::Start_DrumrollBig) || (note == NoteType::Start_BaloonSpecial) ||
			(note == NoteType::DonBigBoth) || (note == NoteType::KaBigBoth);
	}

	enum class ScrollDirection : u8
	{
		FromRight,
		FromAbove,
		FromBelow,
		FromTopRight,
		FromBottomRight,
		FromLeft,
		FromTopLeft,
		FromBottomLeft,
		Count
	};

	enum class BranchCondition : u8
	{
		Roll,
		Precise,
		Score,
		Count
	};

	enum class StyleMode : u8
	{
		Single,
		Double,
		Count
	};

	enum class GaugeIncrementMethod : u8
	{
		Normal,
		Floor,
		Round,
		NotFix,
		Ceiling,
		Count
	};

	enum class ScoreMode : u8
	{
		// NOTE: ドンだフル配点（AC7以前）
		AC1_To_AC7,
		// NOTE: 旧配点（AC14以前）
		AC8_To_AC14,
		// NOTE: 新配点
		AC0,
		Count
	};

	enum class SongSelectSide : u8
	{
		Normal,
		Ex,
		Both,
		Count
	};

	enum class GameType : u8
	{
		Taiko,
		Jubeat,
		Count
	};

	struct ParsedMainMetadata
	{
		std::string TITLE;
		std::string TITLE_JA;
		std::string TITLE_EN;
		std::string TITLE_CN;
		std::string TITLE_TW;
		std::string TITLE_KO;
		std::string SUBTITLE;
		std::string SUBTITLE_JA;
		std::string SUBTITLE_EN;
		std::string SUBTITLE_CN;
		std::string SUBTITLE_TW;
		std::string SUBTITLE_KO;
		std::string WAVE;
		std::string BGIMAGE;
		std::string BGMOVIE;
		std::string LYRICS;
		std::string MAKER;
		Tempo BPM = DefaultTempo;
		f32 HEADSCROLL = 1.0f;
		Time OFFSET = Time::Zero();
		Time MOVIEOFFSET = Time::Zero();
		Time DEMOSTART = Time::Zero();
		f32 SONGVOL = 1.0f;
		f32 SEVOL = 1.0f;
		ScoreMode SCOREMODE = ScoreMode::AC1_To_AC7;
		SongSelectSide SIDE = SongSelectSide::Both;
		i32 LIFE = 0;
		std::string GENRE;
		GameType GAME = GameType::Taiko;
		std::string TAIKOWEBSKIN;
	};

	struct ParsedCourseMetadata
	{
		DifficultyType COURSE = DifficultyType::Oni;
		i32 LEVEL = 1;
		std::vector<i32> BALLOON;
		std::vector<i32> BALLOON_Normal;
		std::vector<i32> BALLOON_Expert;
		std::vector<i32> BALLOON_Master;
		i32 SCOREINIT = 0;
		i32 SCOREDIFF = 0;
		StyleMode STYLE = StyleMode::Single;
		std::string EXAM1;
		std::string EXAM2;
		std::string EXAM3;
		GaugeIncrementMethod GAUGEINCR = GaugeIncrementMethod::Normal;
		i32 TOTAL = 0;
		i32 HIDDENBRANCH = 0;
	};

	enum class ParsedChartCommandType : u8
	{
		Unknown,

		MeasureNotes,
		MeasureEnd,

		ChangeTimeSignature,
		ChangeTempo,
		ChangeDelay,
		ChangeScrollSpeed,
		ChangeBarLine,

		GoGoStart,
		GoGoEnd,

		BranchStart,
		BranchNormal,
		BranchExpert,
		BranchMaster,
		BranchEnd,
		BranchLevelHold,

		ResetAccuracyValues,
		SetLyricLine,

		BMScroll,
		HBScroll,
		SENoteChange,
		SetNextSong,
		ChangeDirection,

		SetSudden,
		SetScrollTransition,

		Count
	};

	struct ParsedChartCommand
	{
		// TODO: Maybe store notes as index + count into single vector (?)
		ParsedChartCommandType Type;
		struct ParamData
		{
			struct { std::vector<NoteType> Notes; } MeasureNotes;
			struct { TimeSignature Value; } ChangeTimeSignature;
			struct { Tempo Value; } ChangeTempo;
			struct { Time Value; } ChangeDelay;
			struct { f32 Value; } ChangeScrollSpeed;
			struct { b8 Visible; } ChangeBarLine;
			struct { BranchCondition Condition; i32 RequirementExpert; i32 RequirementMaster; } BranchStart;
			struct { std::string Value; } SetLyricLine;
			struct { i32 Type; } SENoteChange;
			struct { std::string CommaSeparatedList; } SetNextSong;
			struct { ScrollDirection Direction; } ChangeDirection;
			struct { Time AppearanceOffset, MovementWaitDelay; } SetSudden;
			struct { Time Duration; f32 MovementDistance; ScrollDirection Direction; } SetScrollTransition;
		} Param;
	};

	struct ParsedCourse
	{
		ParsedCourseMetadata Metadata;
		std::vector<ParsedChartCommand> ChartCommands;
	};

	struct ParsedTJA
	{
		ParsedMainMetadata Metadata;
		std::vector<ParsedCourse> Courses;
	};

	std::vector<std::string_view> SplitLines(std::string_view fileContent);

	// NOTE: Designed to never fail, invalid input data just means a different arrangements of (unknown / bad) tokens
	std::vector<Token> TokenizeLines(const std::vector<std::string_view>& lines);

	struct ErrorList
	{
		// TODO: Have error enum type instead + std::string_view of the offending data (?)
		struct ErrorLine { i32 LineIndex; std::string Description; };
		std::vector<ErrorLine> Errors;

		inline void Push(i32 lineIndex, cstr fmt, ...)
		{
			char buffer[0xFF];
			va_list args;
			va_start(args, fmt);
			Errors.push_back(ErrorLine { lineIndex, std::string(buffer, vsprintf_s(buffer, fmt, args)) });
			va_end(args);
		}
		inline void Clear() { Errors.clear(); }
	};

	ParsedTJA ParseTokens(const std::vector<Token>& tokens, ErrorList& outErrors);

	void ConvertParsedToText(const ParsedTJA& inContent, std::string& out, Encoding encoding);

	struct ConvertedNote
	{
		Beat TimeWithinMeasure;
		NoteType Type;
		// TODO: Also need to store balloon pop counts
		// i16 BalloonPopCount;
	};

	struct ConvertedTempoChange
	{
		Beat TimeWithinMeasure;
		Tempo Tempo;
	};

	struct ConvertedDelayChange
	{
		Beat TimeWithinMeasure;
		Time Delay;
	};

	struct ConvertedScrollChange
	{
		Beat TimeWithinMeasure;
		f32 ScrollSpeed;
	};

	struct ConvertedBarLineChange
	{
		Beat TimeWithinMeasure;
		b8 Visibile;
	};

	struct ConvertedLyricChange
	{
		Beat TimeWithinMeasure;
		std::string Lyric;
	};

	struct ConvertedMeasure
	{
		Beat StartTime;
		TimeSignature TimeSignature;
		std::vector<ConvertedNote> Notes;
		std::vector<ConvertedTempoChange> TempoChanges;
		std::vector<ConvertedDelayChange> DelayChanges;
		std::vector<ConvertedScrollChange> ScrollChanges;
		// BUG: Can't actually change inbetween measures..?
		std::vector<ConvertedBarLineChange> BarLineChanges;
		std::vector<ConvertedLyricChange> LyricChanges;
	};

	struct ConvertedGoGoRange
	{
		Beat StartTime;
		Beat EndTime;
	};

	// TODO: Handle multi player tracks and branches ?!
	// TODO: Should probably also rewrite SortedTempoMap for taiko to not be combined but instead have separate TempoMap, TimeSignatureChanges, ScrollSpeedChanges, etc.
	// TODO: Have one TempoMap per branch then for each note have a flag for which branches it applies to (?)
	struct ConvertedCourse
	{
		// TODO: Restructure this to not duplicate main metadata for each course
		ParsedMainMetadata MainMetadata;
		ParsedCourseMetadata CourseMetadata;

		std::vector<ConvertedMeasure> Measures;
		std::vector<ConvertedGoGoRange> GoGoRanges;
	};

	void ConvertConvertedMeasuresToParsedCommands(const std::vector<TJA::ConvertedMeasure>& inMeasures, const std::vector<ConvertedGoGoRange>& inGoGo, std::vector<TJA::ParsedChartCommand>& outCommands);

	ConvertedCourse ConvertParsedToConvertedCourse(const ParsedTJA& inContent, const ParsedCourse& inCourse);
}
