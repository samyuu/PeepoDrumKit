#include "file_format_tja.h"
#include <algorithm>

namespace TJA
{
	// NOTE: Comment marker stored in the format: "// PeepoDrumKit yyyy/MM/dd"
	static constexpr std::string_view PeepoDrumKitCommentMarkerPrefix = "PeepoDrumKit";

	static constexpr std::string_view KeyStrings[EnumCount<Key>] =
	{
		"",

		// NOTE: Main_
		"TITLE",
		"TITLEJA",
		"TITLEEN",
		"TITLECN",
		"TITLETW",
		"TITLEKO",
		"SUBTITLE",
		"SUBTITLEJA",
		"SUBTITLEEN",
		"SUBTITLECN",
		"SUBTITLETW",
		"SUBTITLEKO",
		"BPM",
		"WAVE",
		"OFFSET",
		"DEMOSTART",
		"GENRE",
		"SCOREMODE",
		"MAKER",
		"LYRICS",
		"SONGVOL",
		"SEVOL",
		"SIDE",
		"LIFE",
		"GAME",
		"HEADSCROLL",
		"BGIMAGE",
		"BGMOVIE",
		"MOVIEOFFSET",
		"TAIKOWEBSKIN",

		// NOTE: Course_
		"COURSE",
		"LEVEL",
		"BALLOON",
		"SCOREINIT",
		"SCOREDIFF",
		"BALLOONNOR",
		"BALLOONEXP",
		"BALLOONMAS",
		"STYLE",
		"EXAM1",
		"EXAM2",
		"EXAM3",
		"GAUGEINCR",
		"TOTAL",
		"HIDDENBRANCH",

		// NOTE: Chart_
		"START",
		"END",
		"MEASURE",
		"BPMCHANGE",
		"DELAY",
		"SCROLL",
		"GOGOSTART",
		"GOGOEND",
		"BARLINEOFF",
		"BARLINEON",
		"BRANCHSTART",
		"N",
		"E",
		"M",
		"BRANCHEND",
		"SECTION",
		"LYRIC",
		"LEVELHOLD",
		"BMSCROLL",
		"HBSCROLL",
		"SENOTECHANGE",
		"NEXTSONG",
		"DIRECTION",
		"SUDDEN",
		"JPOSSCROLL",
	};

	struct LinePrefixCommentSuffixSplit { std::string_view LinePrefix, CommentSuffix; };

	static constexpr LinePrefixCommentSuffixSplit SplitLineIntoPrefixAndCommentSuffix(std::string_view line)
	{
		if (line.size() >= (sizeof('/') * 2))
		{
			for (size_t i = 0; i < line.size() - 1; i++)
			{
				if (line[i] == '/' && line[i + 1] == '/')
					return { line.substr(0, i), line.substr(i) };
			}
		}

		return { line, line.substr(0, 0) };
	}

	std::vector<std::string_view> SplitLines(std::string_view fileContent)
	{
		i32 lineCount = 0;
		ASCII::ForEachLineInMultiLineString(fileContent, false, [&](std::string_view) { lineCount++; });

		std::vector<std::string_view> outLines;
		outLines.reserve(lineCount);
		ASCII::ForEachLineInMultiLineString(fileContent, false, [&](std::string_view line) { outLines.push_back(line); });

		return outLines;
	}

	std::vector<Token> TokenizeLines(const std::vector<std::string_view>& lines)
	{
		std::vector<Token> outTokens;
		outTokens.reserve(lines.size());

		b8 currentlyBetweenChartStartAndEnd = false;

		for (size_t lineIndex = 0; lineIndex < lines.size(); lineIndex++)
		{
			const std::string_view lineFull = lines[lineIndex];
			std::string_view lineTrimmed = ASCII::Trim(lineFull);

			if (lineTrimmed.empty() || ASCII::IsAllWhitespace(lineTrimmed))
			{
				Token& newToken = outTokens.emplace_back();
				newToken.Type = TokenType::EmptyLine;
				newToken.LineIndex = static_cast<i16>(lineIndex);
				newToken.Line = lineTrimmed;
			}
			else
			{
				const LinePrefixCommentSuffixSplit lineCommentSplit = SplitLineIntoPrefixAndCommentSuffix(lineTrimmed);
				if (!lineCommentSplit.CommentSuffix.empty())
					lineTrimmed = ASCII::Trim(lineCommentSplit.LinePrefix);

				if (!lineTrimmed.empty())
				{
					Token& newToken = outTokens.emplace_back();
					newToken.Type = TokenType::Unknown;
					newToken.LineIndex = static_cast<i16>(lineIndex);
					newToken.Line = lineTrimmed;

					if (lineTrimmed[0] == '#')
					{
						newToken.Type = TokenType::HashChartCommand;
						if (const size_t spaceSeparator = lineTrimmed.find_first_of(' '); spaceSeparator != std::string_view::npos)
						{
							newToken.KeyString = lineTrimmed.substr(sizeof('#'), spaceSeparator - sizeof('#'));
							newToken.ValueString = lineTrimmed.substr(spaceSeparator + sizeof(' '));
						}
						else
						{
							newToken.KeyString = lineTrimmed.substr(sizeof('#'), lineTrimmed.size() - sizeof('#'));
							newToken.ValueString = {};
						}

						for (size_t i = EnumToIndex(Key::HashCommand_First); i <= EnumToIndex(Key::HashCommand_Last); i++)
							if (newToken.KeyString == KeyStrings[i]) { newToken.Key = static_cast<Key>(i); break; }

#if 0
						{
							if (newToken.KeyString == "SCROLL3") newToken.Key = Key::Chart_SCROLL;		// HACK: For またさいたま2000.tja
							if (newToken.KeyString == "SCROL") newToken.Key = Key::Chart_SCROLL;		// HACK: シンフォニック ドルアーガ.tja
							if (newToken.KeyString == "BPCMAHNEG") newToken.Key = Key::Chart_BPMCHANGE; // HACK: 太鼓ラブ！.tja
						}
#endif

						if (newToken.Key == Key::Chart_START)
							currentlyBetweenChartStartAndEnd = true;
						else if (newToken.Key == Key::Chart_END)
							currentlyBetweenChartStartAndEnd = false;
					}
					else if (const size_t colonSeparator = lineTrimmed.find_first_of(':'); colonSeparator != std::string_view::npos)
					{
						newToken.Type = TokenType::KeyColonValue;
						newToken.KeyString = lineTrimmed.substr(0, colonSeparator);
						newToken.ValueString = lineTrimmed.substr(colonSeparator + sizeof(':'));

						for (size_t i = EnumToIndex(Key::KeyColonValue_First); i <= EnumToIndex(Key::KeyColonValue_Last); i++)
							if (newToken.KeyString == KeyStrings[i]) { newToken.Key = static_cast<Key>(i); break; }
					}
					else
					{
						newToken.Type = currentlyBetweenChartStartAndEnd ? TokenType::ChartData : TokenType::Unknown;
						newToken.KeyString = {};
						newToken.ValueString = lineTrimmed;
					}
				}

				if (!lineCommentSplit.CommentSuffix.empty())
				{
					Token& newCommentToken = outTokens.emplace_back();
					newCommentToken.Type = TokenType::Comment;
					newCommentToken.LineIndex = static_cast<i16>(lineIndex);
					newCommentToken.Line = lineTrimmed;
					newCommentToken.ValueString = ASCII::Trim(lineCommentSplit.CommentSuffix.substr(sizeof('/') * 2));
				}
			}
		}

		return outTokens;
	}

	ParsedTJA ParseTokens(const std::vector<Token>& tokens, ErrorList& outErrors)
	{
		static constexpr auto tryParseI32 = [](std::string_view in, i32* out) -> b8 { if (in.empty()) { *out = 0; return true; } else { return ASCII::TryParseI32(in, *out); } };
		static constexpr auto tryParseF32 = [](std::string_view in, f32* out) -> b8 { if (in.empty()) { *out = 0.0f; return true; } else { return ASCII::TryParseF32(in, *out); } };
		static constexpr auto tryParseCommaSeapratedI32s = [](std::string_view in, std::vector<i32>* out) -> b8
		{
			i32 count = 0;
			ASCII::ForEachInCommaSeparatedList(in, [&](std::string_view) { count++; });
			out->reserve(count);
			b8 allSuccessful = true;
			ASCII::ForEachInCommaSeparatedList(in, [&](std::string_view i32String)
			{
				i32 v = 0;
				allSuccessful &= ASCII::TryParseI32(ASCII::Trim(i32String), v);
				out->push_back(v);
			});
			return allSuccessful;
		};
		static constexpr auto tryParseDifficultyType = [](std::string_view in, DifficultyType* out) -> b8
		{
			if (i32 v; ASCII::TryParseI32(in, v)) { *out = static_cast<DifficultyType>(v); return true; }
			if (ASCII::MatchesInsensitive(in, "easy")) { *out = DifficultyType::Easy; return true; }
			if (ASCII::MatchesInsensitive(in, "normal")) { *out = DifficultyType::Normal; return true; }
			if (ASCII::MatchesInsensitive(in, "hard")) { *out = DifficultyType::Hard; return true; }
			if (ASCII::MatchesInsensitive(in, "oni")) { *out = DifficultyType::Oni; return true; }
			if (ASCII::MatchesInsensitive(in, "edit")) { *out = DifficultyType::OniUra; return true; }
			if (ASCII::MatchesInsensitive(in, "tower")) { *out = DifficultyType::Tower; return true; }
			if (ASCII::MatchesInsensitive(in, "dan")) { *out = DifficultyType::Dan; return true; }
			if (ASCII::MatchesInsensitive(in, "ura")) { *out = DifficultyType::OniUra; return true; }
			return false;
		};
		static constexpr auto tryParseTime = [](std::string_view in, Time* out) -> b8
		{
			if (in.empty()) { *out = Time::Zero(); return true; }
			if (f32 v; ASCII::TryParseF32(in, v)) { *out = Time::FromSec(v); return true; }
			return false;
		};
		static constexpr auto tryParseTempo = [](std::string_view in, Tempo* out) -> b8
		{
			if (in.empty()) { *out = Tempo(0.0f); return true; }
			if (f32 v; ASCII::TryParseF32(in, v)) { *out = Tempo(v); return true; }
			return false;
		};
		static constexpr auto tryParsePercent = [](std::string_view in, f32* out) -> b8
		{
			if (in.empty()) { *out = 0.0f; return true; }
			if (f32 v; ASCII::TryParseF32(in, v)) { *out = FromPercent(v); return true; }
			return false;
		};
		static constexpr auto tryParseTimeSignature = [](std::string_view in, TimeSignature* out) -> b8
		{
			const size_t splitIndex = in.find_first_of("/");
			if (splitIndex == std::string_view::npos)
				return false;

			const std::string_view inNum = ASCII::Trim(in.substr(0, splitIndex));
			const std::string_view inDen = ASCII::Trim(in.substr(splitIndex + 1));
			if (i32 outNum, outDen; ASCII::TryParseI32(inNum, outNum) && ASCII::TryParseI32(inDen, outDen))
			{
				*out = TimeSignature(Max(1, outNum), Max(1, outDen));
				return true;
			}
			else
			{
				return false;
			}
		};
		static constexpr auto tryParseNoteTypeChar = [](char in, NoteType* out) -> b8
		{
			switch (in)
			{
			case '0': *out = NoteType::None; return true;
			case '1': *out = NoteType::Don; return true;
			case '2': *out = NoteType::Ka; return true;
			case '3': *out = NoteType::DonBig; return true;
			case '4': *out = NoteType::KaBig; return true;
			case '5': *out = NoteType::Start_Drumroll; return true;
			case '6': *out = NoteType::Start_DrumrollBig; return true;
			case '7': *out = NoteType::Start_Balloon; return true;
			case '8': *out = NoteType::End_BalloonOrDrumroll; return true;
			case '9': *out = NoteType::Start_BaloonSpecial; return true;
			case 'A': *out = NoteType::DonBigBoth; return true;
			case 'B': *out = NoteType::KaBigBoth; return true;
			case 'F': *out = NoteType::Hidden; return true;
			default: return false;
			}
		};
		static constexpr auto tryParseScoreMode = [](std::string_view in, ScoreMode* out) -> b8
		{
			if (in == "0") { *out = ScoreMode::AC1_To_AC7; return true; }
			if (in == "1") { *out = ScoreMode::AC8_To_AC14; return true; }
			if (in == "2") { *out = ScoreMode::AC0; return true; }
			return false;
		};
		static constexpr auto tryParseSongSelectSide = [](std::string_view in, SongSelectSide* out) -> b8
		{
			if (ASCII::MatchesInsensitive(in, "Normal") || in == "1") { *out = SongSelectSide::Normal; return true; }
			if (ASCII::MatchesInsensitive(in, "Ex") || in == "2") { *out = SongSelectSide::Normal; return true; }
			if (ASCII::MatchesInsensitive(in, "Both") || in == "3") { *out = SongSelectSide::Normal; return true; }
			return false;
		};
		static constexpr auto tryParseGameType = [](std::string_view in, GameType* out) -> b8
		{
			if (ASCII::MatchesInsensitive(in, "Taiko")) { *out = GameType::Taiko; return true; }
			if (ASCII::MatchesInsensitive(in, "Jube")) { *out = GameType::Jubeat; return true; }
			return false;
		};
		static constexpr auto tryParseScrollDirection = [](std::string_view in, ScrollDirection* out) -> b8
		{
			if (i32 v; ASCII::TryParseI32(in, v)) { *out = static_cast<ScrollDirection>(v); return true; }
			*out = ScrollDirection::FromRight; return false;
		};
		static constexpr auto tryParseBranchCondition = [](std::string_view in, BranchCondition* out) -> b8
		{
			if (ASCII::MatchesInsensitive(in, "r")) { *out = BranchCondition::Roll; return true; }
			if (ASCII::MatchesInsensitive(in, "p")) { *out = BranchCondition::Precise; return true; }
			if (ASCII::MatchesInsensitive(in, "s")) { *out = BranchCondition::Score; return true; }
			*out = BranchCondition::Precise; return false;
		};
		static constexpr auto tryParseStyleMode = [](std::string_view in, StyleMode* out) -> b8
		{
			if (ASCII::MatchesInsensitive(in, "Single") || in == "1") { *out = StyleMode::Single; return true; }
			if (ASCII::MatchesInsensitive(in, "Double") || ASCII::MatchesInsensitive(in, "Couple") || in == "2") { *out = StyleMode::Double; return true; }
			*out = StyleMode::Single; return false;
		};
		static constexpr auto tryParseGaugeIncrementMethod = [](std::string_view in, GaugeIncrementMethod* out) -> b8
		{
			if (ASCII::MatchesInsensitive(in, "NORMAL")) { *out = GaugeIncrementMethod::Normal; return true; }
			if (ASCII::MatchesInsensitive(in, "FLOOR")) { *out = GaugeIncrementMethod::Floor; return true; }
			if (ASCII::MatchesInsensitive(in, "ROUND")) { *out = GaugeIncrementMethod::Round; return true; }
			if (ASCII::MatchesInsensitive(in, "NOTFIX")) { *out = GaugeIncrementMethod::NotFix; return true; }
			if (ASCII::MatchesInsensitive(in, "CEILING")) { *out = GaugeIncrementMethod::Ceiling; return true; }
			return true;
		};

		static constexpr auto validateEndOfMeasureNoteCount = [](i32 noteCountAtEndOfMeasure, i32 lineIndex, ErrorList& outErrors)
		{
			if (noteCountAtEndOfMeasure <= 1)
				return;

#if 0 // TODO: JUMP HERE
			static constexpr i32 supported4By4BarDivisions[] = { 1, 2, 4, 8, 12, 16, 24, 32, 48, 64, 96, 192 };
			for (const i32 supportedDivision : supported4By4BarDivisions)
			{
				if (noteCountAtEndOfMeasure == supportedDivision)
					return;
			}

			// TODO: Also needs to take time signature into account..?
			// TODO: Report error if measure note count not nicely divisible assuming max 1/192nd (?)
			outErrors.Push(lineIndex, "Unusual note count measure division (?)");
#endif
		};

		ParsedTJA outTJA = {};
		ParsedCourse* currentCourse = nullptr;

		i32 currentMeasureNoteCount = 0;
		b8 currentlyBetweenChartStartAndEnd = false;
		b8 currentlyBetweenGoGoStartAndEnd = false;
		b8 currentlyInBetweenMeasure = false;

		auto pushChartCommand = [&outTJA, &currentCourse](ParsedChartCommandType type) -> ParsedChartCommand&
		{
			if (currentCourse == nullptr)
				currentCourse = &outTJA.Courses.emplace_back();

			auto& newCommand = currentCourse->ChartCommands.emplace_back();
			newCommand.Type = type;
			return newCommand;
		};

		for (const Token& token : tokens)
		{
			const i16 lineIndex = token.LineIndex;
			switch (token.Type)
			{
			case TokenType::Unknown:
			{
				outErrors.Push(lineIndex, "Unknown data '%.*s'", FmtStrViewArgs(token.Line));
			} break;

			case TokenType::EmptyLine:
			{
				// ...
			} break;

			case TokenType::Comment:
			{
				// ...
				if (!outTJA.HasPeepoDrumKitComment && ASCII::StartsWith(token.ValueString, PeepoDrumKitCommentMarkerPrefix))
				{
					outTJA.HasPeepoDrumKitComment = true;
					outTJA.PeepoDrumKitCommentDate = Date::Zero();

					std::string_view comment = token.ValueString.substr(PeepoDrumKitCommentMarkerPrefix.size());
					comment = ASCII::Trim(comment);
					comment = ASCII::TrimSuffix(ASCII::TrimPrefix(comment, "("), ")");
					if (!comment.empty())
					{
						Date::FormatBuffer dateBuffer {};
						CopyStringViewIntoFixedBuffer(dateBuffer.Data, comment);
						outTJA.PeepoDrumKitCommentDate = Date::FromString(dateBuffer.Data, '/');
					}
				}
			} break;

			case TokenType::KeyColonValue:
			{
				if (token.Key >= Key::Main_First && token.Key <= Key::Main_Last)
				{
					const std::string_view in = ASCII::Trim(token.ValueString);
					ParsedMainMetadata& out = outTJA.Metadata;
					switch (token.Key)
					{
					case Key::Main_TITLE: { out.TITLE = in; } break;
					case Key::Main_TITLEJA: { out.TITLE_JA = in; } break;
					case Key::Main_TITLEEN: { out.TITLE_EN = in; } break;
					case Key::Main_TITLECN: { out.TITLE_CN = in; } break;
					case Key::Main_TITLETW: { out.TITLE_TW = in; } break;
					case Key::Main_TITLEKO: { out.TITLE_KO = in; } break;
					case Key::Main_SUBTITLE: { out.SUBTITLE = in; } break;
					case Key::Main_SUBTITLEJA: { out.SUBTITLE_JA = in; } break;
					case Key::Main_SUBTITLEEN: { out.SUBTITLE_EN = in; } break;
					case Key::Main_SUBTITLECN: { out.SUBTITLE_CN = in; } break;
					case Key::Main_SUBTITLETW: { out.SUBTITLE_TW = in; } break;
					case Key::Main_SUBTITLEKO: { out.SUBTITLE_KO = in; } break;
					case Key::Main_BPM: { if (!tryParseTempo(in, &out.BPM)) { outErrors.Push(lineIndex, "Invalid tempo '%.*s'", FmtStrViewArgs(in)); } } break;
					case Key::Main_WAVE: { out.WAVE = in; } break;
					case Key::Main_OFFSET: { if (!tryParseTime(in, &out.OFFSET)) { outErrors.Push(lineIndex, "Invalid float '%.*s'", FmtStrViewArgs(in)); } } break;
					case Key::Main_DEMOSTART: { if (!tryParseTime(in, &out.DEMOSTART)) { outErrors.Push(lineIndex, "Invalid float '%.*s'", FmtStrViewArgs(in)); } } break;
					case Key::Main_GENRE: { out.GENRE = in; } break;
					case Key::Main_SCOREMODE: { if (!tryParseScoreMode(in, &out.SCOREMODE)) { outErrors.Push(lineIndex, "Unknown score mode '%.*s'", FmtStrViewArgs(in)); } } break;
					case Key::Main_MAKER: { out.MAKER = in; } break;
					case Key::Main_LYRICS: { out.LYRICS = in; } break;
					case Key::Main_SONGVOL: { if (!tryParsePercent(in, &out.SONGVOL)) { outErrors.Push(lineIndex, "Invalid float '%.*s'", FmtStrViewArgs(in)); } } break;
					case Key::Main_SEVOL: { if (!tryParsePercent(in, &out.SEVOL)) { outErrors.Push(lineIndex, "Invalid float '%.*s'", FmtStrViewArgs(in)); } } break;
					case Key::Main_SIDE: { if (!tryParseSongSelectSide(in, &out.SIDE)) { outErrors.Push(lineIndex, "Unknown side '%.*s'", FmtStrViewArgs(in)); } } break;
					case Key::Main_LIFE: { if (!tryParseI32(in, &out.LIFE)) { outErrors.Push(lineIndex, "Invalid int '%.*s'", FmtStrViewArgs(in)); } } break;
					case Key::Main_GAME: { if (!tryParseGameType(in, &out.GAME)) { outErrors.Push(lineIndex, "Unknown game type '%.*s'", FmtStrViewArgs(in)); } } break;
					case Key::Main_HEADSCROLL: { if (!tryParseF32(in, &out.HEADSCROLL)) { outErrors.Push(lineIndex, "Invalid float '%.*s'", FmtStrViewArgs(in)); } } break;
					case Key::Main_BGIMAGE: { out.BGIMAGE = in; } break;
					case Key::Main_BGMOVIE: { out.BGMOVIE = in; } break;
					case Key::Main_MOVIEOFFSET: { if (!tryParseTime(in, &out.MOVIEOFFSET)) { outErrors.Push(lineIndex, "Invalid float '%.*s'", FmtStrViewArgs(in)); } } break;
					case Key::Main_TAIKOWEBSKIN: { out.TAIKOWEBSKIN = in; } break;
					default: { assert(!"Unhandled Key::Main_ switch case despite (Key::Main_First to Key::Main_Last) range check"); } break;
					}
				}
				else if (token.Key >= Key::Course_First && token.Key <= Key::Course_Last)
				{
					if (currentCourse == nullptr)
						currentCourse = &outTJA.Courses.emplace_back();

					const std::string_view in = ASCII::Trim(token.ValueString);
					ParsedCourseMetadata& out = currentCourse->Metadata;
					switch (token.Key)
					{
					case Key::Course_COURSE: { if (!tryParseDifficultyType(in, &out.COURSE)) { outErrors.Push(lineIndex, "Invalid difficulty '%.*s'", FmtStrViewArgs(in)); } } break;
					case Key::Course_LEVEL: { if (!tryParseI32(in, &out.LEVEL)) { outErrors.Push(lineIndex, "Invalid int '%.*s'", FmtStrViewArgs(in)); } } break;
					case Key::Course_BALLOON: { if (!tryParseCommaSeapratedI32s(in, &out.BALLOON)) { outErrors.Push(lineIndex, "Invalid int in comma separated list '%.*s'", FmtStrViewArgs(in)); } } break;
					case Key::Course_SCOREINIT: { if (!tryParseI32(in, &out.SCOREINIT)) { outErrors.Push(lineIndex, "Invalid int '%.*s'", FmtStrViewArgs(in)); } } break;
					case Key::Course_SCOREDIFF: { if (!tryParseI32(in, &out.SCOREDIFF)) { outErrors.Push(lineIndex, "Invalid int '%.*s'", FmtStrViewArgs(in)); } } break;
					case Key::Course_BALLOONNOR: { if (!tryParseCommaSeapratedI32s(in, &out.BALLOON_Normal)) { outErrors.Push(lineIndex, "Invalid int in comma separated list '%.*s'", FmtStrViewArgs(in)); } } break;
					case Key::Course_BALLOONEXP: { if (!tryParseCommaSeapratedI32s(in, &out.BALLOON_Expert)) { outErrors.Push(lineIndex, "Invalid int in comma separated list '%.*s'", FmtStrViewArgs(in)); } } break;
					case Key::Course_BALLOONMAS: { if (!tryParseCommaSeapratedI32s(in, &out.BALLOON_Master)) { outErrors.Push(lineIndex, "Invalid int in comma separated list '%.*s'", FmtStrViewArgs(in)); } } break;
					case Key::Course_STYLE: { if (!tryParseStyleMode(in, &out.STYLE)) { outErrors.Push(lineIndex, "Unknown style mode '%.*s'", FmtStrViewArgs(in)); } } break;
					case Key::Course_EXAM1: { out.EXAM1 = in; } break;
					case Key::Course_EXAM2: { out.EXAM2 = in; } break;
					case Key::Course_EXAM3: { out.EXAM3 = in; } break;
					case Key::Course_GAUGEINCR: { if (!tryParseGaugeIncrementMethod(in, &out.GAUGEINCR)) { outErrors.Push(lineIndex, "Unknown gauge increment method '%.*s'", FmtStrViewArgs(in)); } } break;
					case Key::Course_TOTAL: { out.TOTAL; } break;
					case Key::Course_HIDDENBRANCH: { if (!tryParseI32(in, &out.HIDDENBRANCH)) { outErrors.Push(lineIndex, "Invalid int '%.*s'", FmtStrViewArgs(in)); } } break;
					default: { assert(!"Unhandled Key::Course_ switch case despite (Key::Course_First to Key::Course_Last) range check"); } break;
					}
				}
				else
				{
					outErrors.Push(lineIndex, "Unknown property '%.*s'", FmtStrViewArgs(token.KeyString));
				}
			} break;

			case TokenType::HashChartCommand:
			{
				if (token.Key >= Key::Chart_First && token.Key <= Key::Chart_Last)
				{
					if (token.Key != Key::Chart_START && token.Key != Key::Chart_END && !currentlyBetweenChartStartAndEnd)
						outErrors.Push(lineIndex, "Chart commands must be placed between #START and #END");

					const std::string_view in = ASCII::Trim(token.ValueString);
					switch (token.Key)
					{
					case Key::Chart_START:
					{
						if (currentlyBetweenChartStartAndEnd)
							outErrors.Push(lineIndex, "Missing #END command");

						currentlyBetweenChartStartAndEnd = true;
					} break;
					case Key::Chart_END:
					{
						if (!currentlyBetweenChartStartAndEnd)
							outErrors.Push(lineIndex, "Missing #START command");

						if (currentlyBetweenGoGoStartAndEnd)
						{
							outErrors.Push(lineIndex, "Missing #GOGOEND command");
							pushChartCommand(ParsedChartCommandType::GoGoEnd);
							currentlyBetweenGoGoStartAndEnd = false;
						}

						if (currentlyInBetweenMeasure)
						{
#if 0 // NOTE: Ending on an open measure feels like it should be supported..? Otherwise it forces any chart to have at least one full empty measure at the end (?)
							outErrors.Error(lineIndex, "Missing end-of-measure comma");
							pushChartCommand(ParsedChartCommandType::MeasureEnd);
#endif
							validateEndOfMeasureNoteCount(currentMeasureNoteCount, lineIndex, outErrors);
							currentMeasureNoteCount = 0;
							currentlyInBetweenMeasure = false;
						}

						currentlyBetweenChartStartAndEnd = false;
						currentCourse = nullptr;
					} break;
					case Key::Chart_MEASURE:
					{
						if (currentlyInBetweenMeasure)
						{
							outErrors.Push(lineIndex, "Cannot change time signature in the middle of a measure");
						}
						else
						{
							if (!tryParseTimeSignature(in, &pushChartCommand(ParsedChartCommandType::ChangeTimeSignature).Param.ChangeTimeSignature.Value) || in.empty())
								outErrors.Push(lineIndex, "Invalid time signature '%.*s'", FmtStrViewArgs(in));

							// NOTE: Just a limitation of the fixed point Beat implementation, arguably not a problem with the file itself..?
							// if (((Beat::TicksPerBeat * 4) % currentCourse->ChartCommands.back().Param.ChangeTimeSignature.Value.Denominator) != 0)
							if (!IsTimeSignatureSupported(currentCourse->ChartCommands.back().Param.ChangeTimeSignature.Value))
								outErrors.Push(lineIndex, "Unsupported time signature denominator '%.*s'", FmtStrViewArgs(in));
						}
					} break;
					case Key::Chart_BPMCHANGE:
					{
						if (!tryParseTempo(in, &pushChartCommand(ParsedChartCommandType::ChangeTempo).Param.ChangeTempo.Value) || in.empty())
							outErrors.Push(lineIndex, "Invalid tempo '%.*s'", FmtStrViewArgs(in));
					} break;
					case Key::Chart_DELAY:
					{
						if (!tryParseTime(in, &pushChartCommand(ParsedChartCommandType::ChangeDelay).Param.ChangeDelay.Value) || in.empty())
							outErrors.Push(lineIndex, "Invalid delay '%.*s'", FmtStrViewArgs(in));
					} break;
					case Key::Chart_SCROLL:
					{
						if (!tryParseF32(in, &pushChartCommand(ParsedChartCommandType::ChangeScrollSpeed).Param.ChangeScrollSpeed.Value) || in.empty())
							outErrors.Push(lineIndex, "Invalid scroll speed '%.*s'", FmtStrViewArgs(in));
					} break;
					case Key::Chart_GOGOSTART:
					{
						if (currentlyBetweenGoGoStartAndEnd)
							outErrors.Push(lineIndex, "Missing #GOGOEND command");

						pushChartCommand(ParsedChartCommandType::GoGoStart);
						currentlyBetweenGoGoStartAndEnd = true;
					} break;
					case Key::Chart_GOGOEND:
					{
						if (!currentlyBetweenGoGoStartAndEnd)
							outErrors.Push(lineIndex, "Missing #GOGOSTART command");

						pushChartCommand(ParsedChartCommandType::GoGoEnd);
						currentlyBetweenGoGoStartAndEnd = false;
					} break;
					case Key::Chart_BARLINEOFF:
					{
						pushChartCommand(ParsedChartCommandType::ChangeBarLine).Param.ChangeBarLine.Visible = false;
					} break;
					case Key::Chart_BARLINEON:
					{
						pushChartCommand(ParsedChartCommandType::ChangeBarLine).Param.ChangeBarLine.Visible = true;
					} break;
					case Key::Chart_BRANCHSTART:
					{
						ParsedChartCommand& newCommand = pushChartCommand(ParsedChartCommandType::BranchStart);
						ASCII::ForEachInCommaSeparatedList(in, [&, valueIndex = 0](std::string_view value) mutable
						{
							value = ASCII::Trim(value);
							if (valueIndex == 0) tryParseBranchCondition(value, &newCommand.Param.BranchStart.Condition);
							if (valueIndex == 1) tryParseI32(value, &newCommand.Param.BranchStart.RequirementExpert);
							if (valueIndex == 2) tryParseI32(value, &newCommand.Param.BranchStart.RequirementMaster);
							valueIndex++;
						});
					} break;
					case Key::Chart_N:
					{
						// TODO: Bunch of branch validation etc.
						pushChartCommand(ParsedChartCommandType::BranchNormal);
					} break;
					case Key::Chart_E:
					{
						pushChartCommand(ParsedChartCommandType::BranchExpert);
					} break;
					case Key::Chart_M:
					{
						pushChartCommand(ParsedChartCommandType::BranchMaster);
					} break;
					case Key::Chart_BRANCHEND:
					{
						pushChartCommand(ParsedChartCommandType::BranchEnd);
					} break;
					case Key::Chart_SECTION: { pushChartCommand(ParsedChartCommandType::ResetAccuracyValues); } break;
					case Key::Chart_LYRIC: { pushChartCommand(ParsedChartCommandType::SetLyricLine).Param.SetLyricLine.Value = in; } break;
					case Key::Chart_LEVELHOLD: { pushChartCommand(ParsedChartCommandType::BranchLevelHold); } break;
					case Key::Chart_BMSCROLL: { pushChartCommand(ParsedChartCommandType::BMScroll); } break;
					case Key::Chart_HBSCROLL: { pushChartCommand(ParsedChartCommandType::HBScroll); } break;
					case Key::Chart_SENOTECHANGE: { tryParseI32(in, &pushChartCommand(ParsedChartCommandType::SENoteChange).Param.SENoteChange.Type); } break;
					case Key::Chart_NEXTSONG: { pushChartCommand(ParsedChartCommandType::SetNextSong).Param.SetNextSong.CommaSeparatedList = in; } break;
					case Key::Chart_DIRECTION: { tryParseScrollDirection(in, &pushChartCommand(ParsedChartCommandType::ChangeDirection).Param.ChangeDirection.Direction); } break;
					case Key::Chart_SUDDEN:
					{
						ParsedChartCommand& newCommand = pushChartCommand(ParsedChartCommandType::SetSudden);
						ASCII::ForEachInSpaceSeparatedList(in, [&, valueIndex = 0](std::string_view value) mutable
						{
							value = ASCII::Trim(value);
							if (valueIndex == 0) tryParseTime(value, &newCommand.Param.SetSudden.AppearanceOffset);
							if (valueIndex == 1) tryParseTime(value, &newCommand.Param.SetSudden.MovementWaitDelay);
							valueIndex++;
						});
					} break;
					case Key::Chart_JPOSSCROLL:
					{
						ParsedChartCommand& newCommand = pushChartCommand(ParsedChartCommandType::SetScrollTransition);
						ASCII::ForEachInCommaSeparatedList(in, [&, valueIndex = 0](std::string_view value) mutable
						{
							value = ASCII::Trim(value);
							if (valueIndex == 0) tryParseTime(value, &newCommand.Param.SetScrollTransition.Duration);
							if (valueIndex == 1) tryParseF32(value, &newCommand.Param.SetScrollTransition.MovementDistance);
							if (valueIndex == 2) tryParseScrollDirection(value, &newCommand.Param.SetScrollTransition.Direction);
							valueIndex++;
						});
					} break;
					default: { assert(!"Unhandled Key::Chart_ switch case despite (Key::Chart_First to Key::Chart_Last) range check"); } break;
					}
				}
				else
				{
					outErrors.Push(lineIndex, "Unknown chart command '%.*s'", FmtStrViewArgs(token.KeyString));
				}
			} break;

			case TokenType::ChartData:
			{
				if (currentlyBetweenChartStartAndEnd)
				{
					currentlyInBetweenMeasure = true;

					ParsedChartCommand& newCommand = pushChartCommand(ParsedChartCommandType::MeasureNotes);
					newCommand.Param.MeasureNotes.Notes.reserve(token.ValueString.size());
					for (const char& c : token.ValueString)
					{
						if (c == ',')
						{
							pushChartCommand(ParsedChartCommandType::MeasureEnd);
							if (&c != &token.ValueString.back())
								outErrors.Push(lineIndex, "Unexpected trailing data after end-of-measure comma");

							validateEndOfMeasureNoteCount(currentMeasureNoteCount, lineIndex, outErrors);

							currentMeasureNoteCount = 0;
							currentlyInBetweenMeasure = false;
							break;
						}
						else
						{
							// NOTE: Treating unknown note types as empty spaces makes the most sense because that way timing won't be messed up 
							//		 in case it's somehow a genuine note that just happens to be unsupported by this parser (?)
							NoteType parsedNoteTypeOrNone = NoteType::None;
							if (!tryParseNoteTypeChar(c, &parsedNoteTypeOrNone))
								outErrors.Push(lineIndex, "Unknown note type '%c'", c);

							// NOTE: Except for whitespace I guess, those never make sense to be valid notes (?)
							//		 The format just doesn't really describe how to handle this...
							if (!ASCII::IsWhitespace(c))
							{
								newCommand.Param.MeasureNotes.Notes.push_back(parsedNoteTypeOrNone);
								currentMeasureNoteCount++;
							}
						}
					}
				}
				else
				{
					outErrors.Push(lineIndex, "Chart note data can only be added between #START and #END");
				}
			} break;

			default:
			{
				assert(!"OutOfBounds TokenType enum. TokenizeLines() should have output TokenType::Unknown in case of invalid input data");
			} break;
			}
		}

		if (currentlyBetweenChartStartAndEnd)
			outErrors.Push(tokens.empty() ? 0 : tokens.back().LineIndex, "Missing #END command");

#if 1 // DEBUG: Always add at least one course for now so the debug gui has something to display
		if (outTJA.Courses.empty())
			outTJA.Courses.emplace_back();
#endif

		return outTJA;
	}

	void ConvertParsedToText(const ParsedTJA& inContent, std::string& out, Encoding encoding)
	{
		// TODO: ... or maybe tokenize first instead of going right to text..?
		out.reserve(out.size() + 0x4000);
		if (encoding == Encoding::UTF8)
			out += std::string_view(UTF8::BOM_UTF8, sizeof(UTF8::BOM_UTF8));

		static constexpr auto appendLine = [](std::string& out, std::string_view line) { out += line; out += '\n'; };
		static constexpr auto appendProperyLine = [](std::string& out, Key key, std::string_view value) { out += KeyStrings[EnumToIndex(key)]; out += ':'; out += value; out += '\n'; };
		static constexpr auto appendProperyLineIfNotEmpty = [](std::string& out, Key key, std::string_view value) { if (!value.empty()) { appendProperyLine(out, key, value); } };
		static constexpr auto appendCommandLine = [](std::string& out, Key key, std::string_view value) { out += '#'; out += KeyStrings[EnumToIndex(key)]; if (!value.empty()) { out += ' '; out += value; }out += '\n'; };
		static constexpr auto appendBalloonProperyLine = [](std::string& out, Key key, const std::vector<i32>& popCounts)
		{
			out += KeyStrings[EnumToIndex(key)];
			out += ':';
			char buffer[16];
			for (size_t i = 0; i < popCounts.size(); i++) { if (i != 0) { out += ','; } out += std::string_view(buffer, sprintf_s(buffer, "%d", popCounts[i])); }
			out += '\n';
		};
		char buffer[512];

		static constexpr auto noteTypeToChar = [](NoteType in) -> char
		{
			switch (in)
			{
			case NoteType::None: return '0';
			case NoteType::Don: return '1';
			case NoteType::Ka: return '2';
			case NoteType::DonBig: return '3';
			case NoteType::KaBig: return '4';
			case NoteType::Start_Drumroll: return '5';
			case NoteType::Start_DrumrollBig: return '6';
			case NoteType::Start_Balloon: return '7';
			case NoteType::End_BalloonOrDrumroll: return '8';
			case NoteType::Start_BaloonSpecial: return '9';
			case NoteType::DonBigBoth: return 'A';
			case NoteType::KaBigBoth: return 'B';
			case NoteType::Hidden: return 'F';
			default: return ' ';
			}
		};
		static constexpr auto difficultyTypeToString = [](DifficultyType in) -> cstr
		{
			switch (in)
			{
			case DifficultyType::Easy: return "Easy";
			case DifficultyType::Normal: return "Normal";
			case DifficultyType::Hard: return "Hard";
			case DifficultyType::Oni: return "Oni";
			case DifficultyType::OniUra: return "Edit";
			case DifficultyType::Tower: return "Tower";
			case DifficultyType::Dan: return "Dan";
			default: return "";
			}
		};

		if (inContent.HasPeepoDrumKitComment)
		{
			out += "// ";
			out += PeepoDrumKitCommentMarkerPrefix;
			if (inContent.PeepoDrumKitCommentDate != Date::Zero())
			{
				out += " ";
				out += inContent.PeepoDrumKitCommentDate.ToString().Data;
			}
			out += '\n';
		}

		appendProperyLine(out, Key::Main_TITLE, inContent.Metadata.TITLE);
		appendProperyLineIfNotEmpty(out, Key::Main_TITLEJA, inContent.Metadata.TITLE_JA);
		appendProperyLineIfNotEmpty(out, Key::Main_TITLEEN, inContent.Metadata.TITLE_EN);
		appendProperyLineIfNotEmpty(out, Key::Main_TITLECN, inContent.Metadata.TITLE_CN);
		appendProperyLineIfNotEmpty(out, Key::Main_TITLETW, inContent.Metadata.TITLE_TW);
		appendProperyLineIfNotEmpty(out, Key::Main_TITLEKO, inContent.Metadata.TITLE_KO);
		appendProperyLine(out, Key::Main_SUBTITLE, inContent.Metadata.SUBTITLE);
		appendProperyLineIfNotEmpty(out, Key::Main_SUBTITLEJA, inContent.Metadata.SUBTITLE_JA);
		appendProperyLineIfNotEmpty(out, Key::Main_SUBTITLEEN, inContent.Metadata.SUBTITLE_EN);
		appendProperyLineIfNotEmpty(out, Key::Main_SUBTITLECN, inContent.Metadata.SUBTITLE_CN);
		appendProperyLineIfNotEmpty(out, Key::Main_SUBTITLETW, inContent.Metadata.SUBTITLE_TW);
		appendProperyLineIfNotEmpty(out, Key::Main_SUBTITLEKO, inContent.Metadata.SUBTITLE_KO);
		appendProperyLine(out, Key::Main_BPM, std::string_view(buffer, sprintf_s(buffer, "%g", inContent.Metadata.BPM.BPM)));
		appendProperyLine(out, Key::Main_WAVE, inContent.Metadata.WAVE);
		appendProperyLine(out, Key::Main_OFFSET, std::string_view(buffer, sprintf_s(buffer, "%g", inContent.Metadata.OFFSET.Seconds)));
		appendProperyLine(out, Key::Main_DEMOSTART, std::string_view(buffer, sprintf_s(buffer, "%g", inContent.Metadata.DEMOSTART.Seconds)));
		appendProperyLineIfNotEmpty(out, Key::Main_GENRE, inContent.Metadata.GENRE);
		if (inContent.Metadata.SCOREMODE != ScoreMode {}) appendProperyLine(out, Key::Main_SCOREMODE, std::string_view(buffer, sprintf_s(buffer, "%d", static_cast<i32>(inContent.Metadata.SCOREMODE))));
		appendProperyLineIfNotEmpty(out, Key::Main_MAKER, inContent.Metadata.MAKER);
		appendProperyLineIfNotEmpty(out, Key::Main_LYRICS, inContent.Metadata.LYRICS);
		if (!ApproxmiatelySame(inContent.Metadata.SONGVOL, 1.0f)) appendProperyLine(out, Key::Main_SONGVOL, std::string_view(buffer, sprintf_s(buffer, "%g", ToPercent(inContent.Metadata.SONGVOL))));
		if (!ApproxmiatelySame(inContent.Metadata.SEVOL, 1.0f)) appendProperyLine(out, Key::Main_SEVOL, std::string_view(buffer, sprintf_s(buffer, "%g", ToPercent(inContent.Metadata.SEVOL))));
		// TODO: Key::Main_SIDE;
		if (inContent.Metadata.LIFE != 0) appendProperyLine(out, Key::Main_LIFE, std::string_view(buffer, sprintf_s(buffer, "%d", inContent.Metadata.LIFE)));
		// TODO: Key::Main_GAME;
		if (inContent.Metadata.HEADSCROLL != 1.0f) appendProperyLine(out, Key::Main_HEADSCROLL, std::string_view(buffer, sprintf_s(buffer, "%g", inContent.Metadata.HEADSCROLL)));
		appendProperyLineIfNotEmpty(out, Key::Main_BGIMAGE, inContent.Metadata.BGIMAGE);
		appendProperyLineIfNotEmpty(out, Key::Main_BGMOVIE, inContent.Metadata.BGMOVIE);
		if (inContent.Metadata.MOVIEOFFSET != Time::Zero()) appendProperyLine(out, Key::Main_MOVIEOFFSET, std::string_view(buffer, sprintf_s(buffer, "%g", inContent.Metadata.MOVIEOFFSET.Seconds)));
		appendProperyLineIfNotEmpty(out, Key::Main_TAIKOWEBSKIN, inContent.Metadata.TAIKOWEBSKIN);
		appendLine(out, "");

		for (const ParsedCourse& course : inContent.Courses)
		{
			if (&course != &inContent.Courses[0])
				appendLine(out, "");

			appendProperyLine(out, Key::Course_COURSE, difficultyTypeToString(course.Metadata.COURSE));
			appendProperyLine(out, Key::Course_LEVEL, std::string_view(buffer, sprintf_s(buffer, "%d", course.Metadata.LEVEL)));
			if (!course.Metadata.BALLOON.empty() || !course.Metadata.BALLOON_Normal.empty() || !course.Metadata.BALLOON_Expert.empty() || !course.Metadata.BALLOON_Master.empty())
				appendBalloonProperyLine(out, Key::Course_BALLOON, course.Metadata.BALLOON);
			if (!course.Metadata.BALLOON_Normal.empty() || !course.Metadata.BALLOON_Expert.empty() || !course.Metadata.BALLOON_Master.empty())
			{
				appendBalloonProperyLine(out, Key::Course_BALLOONNOR, course.Metadata.BALLOON_Normal);
				appendBalloonProperyLine(out, Key::Course_BALLOONEXP, course.Metadata.BALLOON_Expert);
				appendBalloonProperyLine(out, Key::Course_BALLOONMAS, course.Metadata.BALLOON_Master);
			}
			appendProperyLine(out, Key::Course_SCOREINIT, (course.Metadata.SCOREINIT == 0) ? "" : std::string_view(buffer, sprintf_s(buffer, "%d", course.Metadata.SCOREINIT)));
			appendProperyLine(out, Key::Course_SCOREDIFF, (course.Metadata.SCOREDIFF == 0) ? "" : std::string_view(buffer, sprintf_s(buffer, "%d", course.Metadata.SCOREDIFF)));
			// TODO: Key::Course_STYLE;
			// TODO: Key::Course_EXAM1;
			// TODO: Key::Course_EXAM2;
			// TODO: Key::Course_EXAM3;
			// TODO: Key::Course_GAUGEINCR;
			// TODO: Key::Course_TOTAL;
			// TODO: Key::Course_HIDDENBRANCH;
			appendLine(out, "");

			appendCommandLine(out, Key::Chart_START, "");
			for (const ParsedChartCommand& command : course.ChartCommands)
			{
				switch (command.Type)
				{
				case ParsedChartCommandType::MeasureNotes:
				{
					for (const NoteType note : command.Param.MeasureNotes.Notes)
						out += noteTypeToChar(note);

					if (ArrayItToIndex(&command, &course.ChartCommands[0]) + 1 < course.ChartCommands.size())
					{
						if ((&command + 1)->Type != ParsedChartCommandType::MeasureEnd)
							appendLine(out, "");
					}
				} break;
				case ParsedChartCommandType::MeasureEnd: { appendLine(out, ","); } break;
				case ParsedChartCommandType::ChangeTimeSignature:
				{
					appendCommandLine(out, Key::Chart_MEASURE, std::string_view(buffer, sprintf_s(buffer, "%d/%d", command.Param.ChangeTimeSignature.Value.Numerator, command.Param.ChangeTimeSignature.Value.Denominator)));
				} break;
				case ParsedChartCommandType::ChangeTempo:
				{
					appendCommandLine(out, Key::Chart_BPMCHANGE, std::string_view(buffer, sprintf_s(buffer, "%g", command.Param.ChangeTempo.Value.BPM)));
				} break;
				case ParsedChartCommandType::ChangeDelay:
				{
					appendCommandLine(out, Key::Chart_DELAY, std::string_view(buffer, sprintf_s(buffer, "%g", command.Param.ChangeDelay.Value.ToSec())));
				} break;
				case ParsedChartCommandType::ChangeScrollSpeed:
				{
					appendCommandLine(out, Key::Chart_SCROLL, std::string_view(buffer, sprintf_s(buffer, "%g", command.Param.ChangeScrollSpeed.Value)));
				} break;
				case ParsedChartCommandType::ChangeBarLine:
				{
					appendCommandLine(out, command.Param.ChangeBarLine.Visible ? Key::Chart_BARLINEON : Key::Chart_BARLINEOFF, "");
				} break;
				case ParsedChartCommandType::GoGoStart:
				{
					appendCommandLine(out, Key::Chart_GOGOSTART, "");
				} break;
				case ParsedChartCommandType::GoGoEnd:
				{
					appendCommandLine(out, Key::Chart_GOGOEND, "");
				} break;
				case ParsedChartCommandType::BranchStart:
				{
					// TODO:
				} break;
				case ParsedChartCommandType::BranchNormal:
				{
					// TODO:
				} break;
				case ParsedChartCommandType::BranchExpert:
				{
					// TODO:
				} break;
				case ParsedChartCommandType::BranchMaster:
				{
					// TODO:
				} break;
				case ParsedChartCommandType::BranchEnd:
				{
					// TODO:
				} break;
				case ParsedChartCommandType::BranchLevelHold:
				{
					// TODO:
				} break;
				case ParsedChartCommandType::ResetAccuracyValues:
				{
					// TODO:
				} break;
				case ParsedChartCommandType::SetLyricLine:
				{
					// TODO: Handle escape characters, most importantly "\n"
					appendCommandLine(out, Key::Chart_LYRIC, command.Param.SetLyricLine.Value);
				} break;
				case ParsedChartCommandType::BMScroll:
				{
					// TODO:
				} break;
				case ParsedChartCommandType::HBScroll:
				{
					// TODO:
				} break;
				case ParsedChartCommandType::SENoteChange:
				{
					// TODO:
				} break;
				case ParsedChartCommandType::SetNextSong:
				{
					// TODO:
				} break;
				case ParsedChartCommandType::ChangeDirection:
				{
					// TODO:
				} break;
				case ParsedChartCommandType::SetSudden:
				{
					// TODO:
				} break;
				case ParsedChartCommandType::SetScrollTransition:
				{
					// TODO:
				} break;
				default: { assert(!"Unhandled ParsedChartCommandType switch case"); } break;
				}
			}
			appendCommandLine(out, Key::Chart_END, "");
		}
	}

	void ConvertConvertedMeasuresToParsedCommands(const std::vector<ConvertedMeasure>& inMeasures, const std::vector<ConvertedGoGoRange>& inGoGo, std::vector<ParsedChartCommand>& outCommands)
	{
		struct TempCommand { Beat TimeWithinMeasure; ParsedChartCommand ParsedCommand; };
		std::vector<TempCommand> tempBuffer;
		tempBuffer.reserve(64);

		outCommands.reserve(inMeasures.size() * 4);

		TimeSignature lastSignature = DefaultTimeSignature;
		for (const ConvertedMeasure& inMeasure : inMeasures)
		{
			if (inMeasure.TimeSignature != lastSignature)
			{
				ParsedChartCommand& tempCommand = tempBuffer.emplace_back(TempCommand { Beat::Zero() }).ParsedCommand;
				tempCommand.Type = ParsedChartCommandType::ChangeTimeSignature;
				tempCommand.Param.ChangeTimeSignature.Value = inMeasure.TimeSignature;
				lastSignature = inMeasure.TimeSignature;
			}

			if (!inGoGo.empty())
			{
				// HACK: Could yet again be sped up a lot using a binary search
				const Beat inMeasureStartTime = inMeasure.StartTime;
				const Beat inMeasureEndTime = inMeasure.StartTime + inMeasure.TimeSignature.GetDurationPerBar();
				for (const auto& gogo : inGoGo)
				{
					if (gogo.StartTime >= inMeasureStartTime && gogo.StartTime < inMeasureEndTime)
						tempBuffer.emplace_back(TempCommand { (gogo.StartTime - inMeasureStartTime) }).ParsedCommand.Type = ParsedChartCommandType::GoGoStart;
					if (gogo.EndTime >= inMeasureStartTime && gogo.EndTime < inMeasureEndTime)
						tempBuffer.emplace_back(TempCommand { (gogo.EndTime - inMeasureStartTime) }).ParsedCommand.Type = ParsedChartCommandType::GoGoEnd;
				}
			}

			for (const ConvertedBarLineChange& barLineChange : inMeasure.BarLineChanges)
			{
				ParsedChartCommand& tempCommand = tempBuffer.emplace_back(TempCommand { barLineChange.TimeWithinMeasure }).ParsedCommand;
				tempCommand.Type = ParsedChartCommandType::ChangeBarLine;
				tempCommand.Param.ChangeBarLine.Visible = barLineChange.Visibile;
			}

			for (const ConvertedTempoChange& tempoChange : inMeasure.TempoChanges)
			{
				ParsedChartCommand& tempCommand = tempBuffer.emplace_back(TempCommand { tempoChange.TimeWithinMeasure }).ParsedCommand;
				tempCommand.Type = ParsedChartCommandType::ChangeTempo;
				tempCommand.Param.ChangeTempo.Value = tempoChange.Tempo;
			}

			for (const ConvertedScrollChange& scrollChange : inMeasure.ScrollChanges)
			{
				ParsedChartCommand& tempCommand = tempBuffer.emplace_back(TempCommand { scrollChange.TimeWithinMeasure }).ParsedCommand;
				tempCommand.Type = ParsedChartCommandType::ChangeScrollSpeed;
				tempCommand.Param.ChangeScrollSpeed.Value = scrollChange.ScrollSpeed;
			}

			for (const ConvertedLyricChange& lyricChange : inMeasure.LyricChanges)
			{
				ParsedChartCommand& tempCommand = tempBuffer.emplace_back(TempCommand { lyricChange.TimeWithinMeasure }).ParsedCommand;
				tempCommand.Type = ParsedChartCommandType::SetLyricLine;
				tempCommand.Param.SetLyricLine.Value = lyricChange.Lyric;
			}

			for (const ConvertedDelayChange& delayChange : inMeasure.DelayChanges)
			{
				ParsedChartCommand& tempCommand = tempBuffer.emplace_back(TempCommand { delayChange.TimeWithinMeasure }).ParsedCommand;
				tempCommand.Type = ParsedChartCommandType::ChangeDelay;
				tempCommand.Param.ChangeDelay.Value = delayChange.Delay;
			}

			for (const ConvertedNote& note : inMeasure.Notes)
			{
				ParsedChartCommand& tempCommand = tempBuffer.emplace_back(TempCommand { note.TimeWithinMeasure }).ParsedCommand;
				tempCommand.Type = ParsedChartCommandType::MeasureNotes;
				tempCommand.Param.MeasureNotes.Notes.push_back(note.Type);
			}

			if (!tempBuffer.empty())
			{
				// TODO: Find smallest bar division to fit in all the commands then add into temp
				static constexpr i32 supportedBarDivisions[] = { 1, 4, 8, 12, 16, 24, 32, 48, 64, 96, 192 };
				i32 smallestBarDivision = 192;
				for (const i32 itDivision : supportedBarDivisions)
				{
					const Beat itBeatDivision = Beat::FromBars(1) / itDivision;
					if (std::all_of(tempBuffer.begin(), tempBuffer.end(), [&](const TempCommand& c) { return (c.TimeWithinMeasure.Ticks % itBeatDivision.Ticks) == 0; }))
						smallestBarDivision = Min(smallestBarDivision, itDivision);
				}

				const Beat beatPerNoteInThisMeasure = Beat::FromBars(1) / smallestBarDivision;
				const Beat measureBarDuration = lastSignature.GetDurationPerBar();
				const i32 noteCommandsInThisMeasure = (beatPerNoteInThisMeasure == Beat::Zero()) ? 0 : (measureBarDuration.Ticks / beatPerNoteInThisMeasure.Ticks);

				// NOTE: Insert empty notes based on smallestBarDivision
				if (noteCommandsInThisMeasure > 0)
				{
					for (i32 noteIndex = 0; noteIndex < noteCommandsInThisMeasure; noteIndex++)
					{
						const Beat noteBeat = Beat::FromTicks(noteIndex * beatPerNoteInThisMeasure.Ticks);

						b8 noteAlreadyExists = false;
						for (const TempCommand& tempCommand : tempBuffer)
						{
							if (tempCommand.ParsedCommand.Type == ParsedChartCommandType::MeasureNotes && tempCommand.TimeWithinMeasure == noteBeat)
								noteAlreadyExists = true;
						}

						if (!noteAlreadyExists)
						{
							ParsedChartCommand& tempCommand = tempBuffer.emplace_back(TempCommand { noteBeat }).ParsedCommand;
							tempCommand.Type = ParsedChartCommandType::MeasureNotes;
							tempCommand.Param.MeasureNotes.Notes.push_back(NoteType::None);
						}
					}

					i32 actualNotesInThisMeasure = 0;
					for (const TempCommand& tempCommand : tempBuffer)
						if (tempCommand.ParsedCommand.Type == ParsedChartCommandType::MeasureNotes)
							actualNotesInThisMeasure++;
					if (actualNotesInThisMeasure != noteCommandsInThisMeasure)
					{
						// BUG: Loss of precision due to integer division...
						// assert(false);
					}
				}

				// TODO: Check if this works correctly (TimeWithinMeasure biggest priorirty, but note commands should come last)
				std::stable_sort(tempBuffer.begin(), tempBuffer.end(), [](const TempCommand& a, const TempCommand& b) { return (a.TimeWithinMeasure < b.TimeWithinMeasure); });

				// NOTE: Merge adjacent single-note MeasureNotes commands
				{
					for (size_t i = 0; i < tempBuffer.size(); i++)
					{
						TempCommand* lastCommand = (i >= 1) ? &tempBuffer[i - 1] : nullptr;
						TempCommand& thisCommand = tempBuffer[i];

						if (lastCommand != nullptr && (thisCommand.ParsedCommand.Type == ParsedChartCommandType::MeasureNotes) && (lastCommand->ParsedCommand.Type == ParsedChartCommandType::MeasureNotes))
						{
							for (NoteType note : thisCommand.ParsedCommand.Param.MeasureNotes.Notes)
								lastCommand->ParsedCommand.Param.MeasureNotes.Notes.push_back(note);

							tempBuffer.erase(tempBuffer.begin() + i); i--;
							continue;
						}
					}
				}

				for (size_t i = 0; i < tempBuffer.size(); i++)
					outCommands.push_back(std::move(tempBuffer[i].ParsedCommand));
				tempBuffer.clear();
			}

			outCommands.push_back(ParsedChartCommand { ParsedChartCommandType::MeasureEnd });
		}
	}

	ConvertedCourse ConvertParsedToConvertedCourse(const ParsedTJA& inContent, const ParsedCourse& inCourse)
	{
		ConvertedCourse out = {};
		out.MainMetadata = inContent.Metadata;
		out.CourseMetadata = inCourse.Metadata;

		{
			const size_t measureCount = std::count_if(inCourse.ChartCommands.begin(), inCourse.ChartCommands.end(), [](auto& c) { return c.Type == ParsedChartCommandType::MeasureEnd; });
			out.Measures.reserve(measureCount + 1);
		}

		// NOTE: Add measures with time signatures and notes (including "empty" ones as needed for time calculations)
		{
			ConvertedMeasure* currentMeasure = &out.Measures.emplace_back();
			currentMeasure->TimeSignature = DefaultTimeSignature;

			for (const ParsedChartCommand& command : inCourse.ChartCommands)
			{
				if (command.Type == ParsedChartCommandType::MeasureNotes)
				{
					for (const NoteType note : command.Param.MeasureNotes.Notes)
						currentMeasure->Notes.push_back(ConvertedNote { Beat::Zero(), note });
				}
				else if (command.Type == ParsedChartCommandType::MeasureEnd)
				{
					currentMeasure = &out.Measures.emplace_back();
					currentMeasure->TimeSignature = out.Measures[out.Measures.size() - 2].TimeSignature;
				}
				else if (command.Type == ParsedChartCommandType::ChangeTimeSignature)
				{
					currentMeasure->TimeSignature = command.Param.ChangeTimeSignature.Value;
				}
			}

			Beat currentMeasureTime = Beat::Zero();
			for (ConvertedMeasure& measure : out.Measures)
			{
				measure.StartTime = currentMeasureTime;
				currentMeasureTime += measure.TimeSignature.GetDurationPerBar();

				Beat currentTimeWithinMeasure = Beat::Zero();
				for (ConvertedNote& note : measure.Notes)
				{
					note.TimeWithinMeasure = currentTimeWithinMeasure;
					currentTimeWithinMeasure += (measure.TimeSignature.GetDurationPerBar() / static_cast<i32>(measure.Notes.size()));
				}
			}
		}

		// NOTE: Insert additional commands
		{
			ConvertedMeasure* currentMeasure = &out.Measures[0];
			Beat currentTimeWithinMeasure = Beat::Zero();
			i32 currentNotesInMeasure = 0;

			for (const ParsedChartCommand& command : inCourse.ChartCommands)
			{
				if (command.Type == ParsedChartCommandType::MeasureNotes)
				{
					currentNotesInMeasure += static_cast<i32>(command.Param.MeasureNotes.Notes.size());

					if (!currentMeasure->Notes.empty() && currentNotesInMeasure > 0)
						currentTimeWithinMeasure = currentMeasure->Notes[currentNotesInMeasure - 1].TimeWithinMeasure +
						// TODO: WHAT TO DO HERE?
						(currentMeasure->Notes.size() > 1 ? currentMeasure->Notes[1].TimeWithinMeasure : Beat {});
				}
				if (command.Type == ParsedChartCommandType::MeasureEnd)
				{
					currentMeasure++;
					currentTimeWithinMeasure = Beat::Zero();
					currentNotesInMeasure = 0;
				}
				else if (command.Type == ParsedChartCommandType::ChangeTempo)
				{
					currentMeasure->TempoChanges.push_back(ConvertedTempoChange { currentTimeWithinMeasure, command.Param.ChangeTempo.Value });
				}
				else if (command.Type == ParsedChartCommandType::ChangeDelay)
				{
					currentMeasure->DelayChanges.push_back(ConvertedDelayChange { currentTimeWithinMeasure, command.Param.ChangeDelay.Value });
				}
				else if (command.Type == ParsedChartCommandType::ChangeScrollSpeed)
				{
					currentMeasure->ScrollChanges.push_back(ConvertedScrollChange { currentTimeWithinMeasure, command.Param.ChangeScrollSpeed.Value });
				}
				else if (command.Type == ParsedChartCommandType::GoGoStart)
				{
					const Beat startTime = currentMeasure->StartTime + currentTimeWithinMeasure;
					out.GoGoRanges.push_back(ConvertedGoGoRange { startTime, startTime });
				}
				else if (command.Type == ParsedChartCommandType::GoGoEnd)
				{
					if (!out.GoGoRanges.empty())
						out.GoGoRanges.back().EndTime = currentMeasure->StartTime + currentTimeWithinMeasure;
				}
				else if (command.Type == ParsedChartCommandType::ChangeBarLine)
				{
					currentMeasure->BarLineChanges.push_back(ConvertedBarLineChange { currentTimeWithinMeasure, command.Param.ChangeBarLine.Visible });
				}
				else if (command.Type == ParsedChartCommandType::SetLyricLine)
				{
					currentMeasure->LyricChanges.push_back(ConvertedLyricChange { currentTimeWithinMeasure, command.Param.SetLyricLine.Value });
				}
			}

			ConvertedMeasure& firstMeasure = out.Measures[0];
			if (firstMeasure.TempoChanges.empty())
				firstMeasure.TempoChanges.push_back(ConvertedTempoChange { Beat::Zero(), inContent.Metadata.BPM });
		}

		// NOTE: Final cleanup
		{
			for (ConvertedMeasure& measure : out.Measures)
			{
				if (!measure.Notes.empty())
					erase_remove_if(measure.Notes, [](const ConvertedNote& note) { return (note.Type == NoteType::None); });
			}
		}

		return out;
	}
}
