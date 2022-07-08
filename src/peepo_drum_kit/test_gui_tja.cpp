#include "test_gui_tja.h"

namespace PeepoDrumKit
{
	static constexpr const char* TJATokenTypeNames[] = { "???", "...", "// comment", "Key:Value", "# Chart Command", "Chart Data" };
	static_assert(ArrayCount(TJATokenTypeNames) == EnumCount<TJA::TokenType>);

	static constexpr const char* TJADifficultyTypeNames[] = { "Easy", "Normal", "Hard", "Oni", "Oni (Ura)", "Tower", "Dan", };
	static_assert(ArrayCount(TJADifficultyTypeNames) == EnumCount<TJA::DifficultyType>);

	static constexpr const char* TJAScoreModeNames[] = { "AC1 - AC7", "AC8 - AC14", "AC0", };
	static_assert(ArrayCount(TJAScoreModeNames) == EnumCount<TJA::ScoreMode>);

	static constexpr const char* TJASongSelectSideNames[] = { "Normal", "Ex", "Both", };
	static_assert(ArrayCount(TJASongSelectSideNames) == EnumCount<TJA::SongSelectSide>);

	static constexpr const char* TJAGameTypeNames[] = { "Taiko", "Jubeat", };
	static_assert(ArrayCount(TJAGameTypeNames) == EnumCount<TJA::GameType>);

	static constexpr const char* TJAStyleModeNames[] = { "Single", "Double", };
	static_assert(ArrayCount(TJAStyleModeNames) == EnumCount<TJA::StyleMode>);

	static constexpr const char* TJAParsedChartCommandTypeNames[] =
	{
		"UNKNOWN",
		"Notes",
		"Measure End",
		"Change Time Signature",
		"Change Tempo",
		"Change Delay",
		"Change Scroll Speed",
		"Change Bar Line",
		"GoGo Start",
		"GoGo End",
		"Branch Start",
		"Branch Normal",
		"Branch Expert",
		"Branch Master",
		"Branch End",
		"Branch Level Hold",
		"Reset Accuracy Values",
		"Set Lyric Line",
		"BM Scroll",
		"HB Scroll",
		"SE Note Change",
		"Set Next Song",
		"Change Direction",
		"Set Sudden",
		"Set Scroll Transition",
	};
	static_assert(ArrayCount(TJAParsedChartCommandTypeNames) == EnumCount<TJA::ParsedChartCommandType>);

	static constexpr const char* TJANoteTypeNames[] =
	{
		u8"_",
		u8"o",
		u8"x",
		u8"O",
		u8"X",
		u8"d->",
		u8"D->",
		u8"b->",
		u8"<-",
		u8"B->",
		u8"OO",
		u8"XX",
		u8"?",
	};
	static_assert(ArrayCount(TJANoteTypeNames) == EnumCount<TJA::NoteType>);

	static constexpr ::TextEditor::Palette NiceImImGuiColorTextEditColorPalette =
	{ {
		0xFFA4B3B7,	// Default
		0xFFA1C94E,	// Keyword	
		0xFFA7CEA2,	// Number
		0xFF859DD6,	// String
		0xFF859DD6, // Char literal
		0xFFB4B4B4, // Punctuation
		0xFFC563BD,	// Preprocessor
		0xFFCF9C56, // Identifier
		0xFFA1C94E, // Known identifier
		0xFFC563BD, // Preproc identifier
		0xFF4AA657, // Comment (single line)
		0xFF4AA657, // Comment (multi line)
		0xFF1F1F1F, // Background
		0xFFDCDCDC, // Cursor
		0x40C1C1C1, // Selection
		0x80002080, // ErrorMarker
		0x40F08000, // Breakpoint
		0xFF858585, // Line number
		0x40303030, // Current line fill
		0x40303030, // Current line fill (inactive)
		0x40303030, // Current line edge
	} };

	::TextEditor CreateImGuiColorTextEditWithNiceTheme()
	{
		// https://github.com/BalazsJako/ColorTextEditorDemo/blob/master/main.cpp
		::TextEditor out;
		out.SetShowWhitespaces(false);
		out.SetPalette(NiceImImGuiColorTextEditColorPalette);
		return out;
	}

	static void SetImGuiColorTextEditErrorMarkersFromErrorList(TextEditor& outTextEditor, const TJA::ErrorList& inErrorList)
	{
		TextEditor::ErrorMarkers errorMarkers;
		for (const auto& error : inErrorList.Errors)
			errorMarkers[error.LineIndex + 1] = error.Description;
		outTextEditor.SetErrorMarkers(errorMarkers);
	}

	void TJATestWindows::DrawGui(bool* isOpen)
	{
		WasTJAEditedThisFrame = false;

		if (LoadTJAFuture.valid() && LoadTJAFuture._Is_ready())
		{
			LoadedTJAFile = LoadTJAFuture.get();
			TJATextEditor.SetText(LoadedTJAFile.FileContentUTF8);
			SetImGuiColorTextEditErrorMarkersFromErrorList(TJATextEditor, LoadedTJAFile.ParseErrors);
			WasTJAEditedThisFrame = true;
		}

#if 0
		for (const std::string& droppedFilePath : ApplicationHost::GlobalState.FilePathsDroppedThisFrame)
		{
			if (ASCII::EndsWithInsensitive(droppedFilePath, ".tja"))
			{
				LoadTJAFuture = std::async(std::launch::async, [pathCopy = droppedFilePath] { ParsedAndConvertedTJAFile tja; tja.LoadFromFile(pathCopy); return tja; });
				break;
			}
		}
#endif

		DrawGuiFileContentWindow(isOpen);
		DrawGuiTokensWindow(isOpen);
#if 0
		DrawGuiConvertedWindow(isOpen);
#endif
		DrawGuiParsedWindow(isOpen);
	}

	void TJATestWindows::DrawGuiFileContentWindow(bool* isOpen)
	{
		if (Gui::Begin("TJA Test - File Content", isOpen, ImGuiWindowFlags_None))
		{
			if (Gui::BeginTabBar("FileContentTabBar", ImGuiTabBarFlags_None))
			{
				if (Gui::BeginTabItem("Live Edit (Code)"))
				{
					// HACK: Make it almost impossible to tripple click select whole line because it's just annoying
					const f32 originalMouseDoubleClickTime = Gui::GetIO().MouseDoubleClickTime;
					Gui::GetIO().MouseDoubleClickTime = 0.1f;
					defer { Gui::GetIO().MouseDoubleClickTime = originalMouseDoubleClickTime; };

					Gui::BeginChild("Inner", Gui::GetContentRegionAvail(), true);
					TJATextEditor.Render("TJATextEditor", Gui::GetContentRegionAvail(), false);
					if (TJATextEditor.IsTextChanged())
					{
						LoadedTJAFile.FileContentUTF8 = TJATextEditor.GetText();
						LoadedTJAFile.DebugReloadFromModifiedFileContentUTF8();
						SetImGuiColorTextEditErrorMarkersFromErrorList(TJATextEditor, LoadedTJAFile.ParseErrors);
						WasTJAEditedThisFrame = true;
					}
					Gui::EndChild();
					Gui::EndTabItem();
				}

				if (Gui::BeginTabItem("Live Edit"))
				{
					Gui::BeginChild("Inner", Gui::GetContentRegionAvail(), true);
					{
						Gui::PushStyleColor(ImGuiCol_FrameBg, Gui::GetStyleColorVec4(ImGuiCol_WindowBg));

						if (Gui::InputTextMultiline("##FileContent", &LoadedTJAFile.FileContentUTF8, Gui::GetContentRegionAvail(), ImGuiInputTextFlags_None))
						{
							LoadedTJAFile.DebugReloadFromModifiedFileContentUTF8();
							TJATextEditor.SetText(LoadedTJAFile.FileContentUTF8);
							SetImGuiColorTextEditErrorMarkersFromErrorList(TJATextEditor, LoadedTJAFile.ParseErrors);
							WasTJAEditedThisFrame = true;
						}

						Gui::PopStyleColor();
					}
					Gui::EndChild();
					Gui::EndTabItem();
				}

				if (Gui::BeginTabItem("Line View"))
				{
					Gui::BeginChild("Inner", Gui::GetContentRegionAvail(), true);
					{
						ASCII::ForEachLineInMultiLineString(LoadedTJAFile.FileContentUTF8, true, [this, lineIndex = 0](const std::string_view line) mutable
						{
							Gui::TextDisabled("[Line: %03d, 0x%04X:]:", ++lineIndex, static_cast<i32>(line.data() - LoadedTJAFile.FileContentUTF8.data()));
							Gui::SameLine();
							Gui::TextUnformatted(line);
						});
					}
					Gui::EndChild();
					Gui::EndTabItem();
				}

				Gui::EndTabBar();
			}
		}
		Gui::End();
	}

	void TJATestWindows::DrawGuiTokensWindow(bool* isOpen)
	{
		if (Gui::Begin("TJA Test - Tokens", isOpen, ImGuiWindowFlags_None))
		{
			if (Gui::BeginTable("TokenTable", 3, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, Gui::GetContentRegionAvail()))
			{
				Gui::TableSetupScrollFreeze(0, 1);
				Gui::TableSetupColumn("Type", ImGuiTableColumnFlags_None);
				Gui::TableSetupColumn("Key", ImGuiTableColumnFlags_None);
				Gui::TableSetupColumn("Value", ImGuiTableColumnFlags_None);
				Gui::TableHeadersRow();

				for (const TJA::Token& token : LoadedTJAFile.Tokens)
				{
					Gui::TableNextRow();

					Gui::TableNextColumn();
					Gui::TextUnformatted(TJATokenTypeNames[EnumToIndex(token.Type)]);

					Gui::TableNextColumn();
					{
						if (token.Key >= TJA::Key::KeyColonValue_First && token.Key <= TJA::Key::KeyColonValue_Last)
							Gui::PushStyleColor(ImGuiCol_Text, NiceImImGuiColorTextEditColorPalette[EnumToIndex(::TextEditor::PaletteIndex::Identifier)]);
						else if (token.Key >= TJA::Key::HashCommand_First && token.Key <= TJA::Key::HashCommand_Last)
							Gui::PushStyleColor(ImGuiCol_Text, NiceImImGuiColorTextEditColorPalette[EnumToIndex(::TextEditor::PaletteIndex::Preprocessor)]);
						else
							Gui::PushStyleColor(ImGuiCol_Text, NiceImImGuiColorTextEditColorPalette[EnumToIndex(::TextEditor::PaletteIndex::Default)]);

						Gui::TextUnformatted(token.KeyString);

						Gui::PopStyleColor();
					}

					Gui::TableNextColumn();
					{
						if (token.Type == TJA::TokenType::Comment)
							Gui::PushStyleColor(ImGuiCol_Text, NiceImImGuiColorTextEditColorPalette[EnumToIndex(::TextEditor::PaletteIndex::Comment)]);
						else if (token.Type == TJA::TokenType::ChartData)
							Gui::PushStyleColor(ImGuiCol_Text, NiceImImGuiColorTextEditColorPalette[EnumToIndex(::TextEditor::PaletteIndex::Number)]);
						else
							Gui::PushStyleColor(ImGuiCol_Text, NiceImImGuiColorTextEditColorPalette[EnumToIndex(::TextEditor::PaletteIndex::Default)]);

						Gui::TextUnformatted(token.ValueString);

						Gui::PopStyleColor();
					}
				}
				Gui::EndTable();
			}
		}
		Gui::End();
	}

	void TJATestWindows::DrawGuiParsedWindow(bool* isOpen)
	{
		if (Gui::Begin("TJA Test - Parsed", isOpen, ImGuiWindowFlags_None))
		{
			Gui::BeginChild("MainMetadataChild", { 0.0f, Gui::GetContentRegionAvail().y * 0.35f }, false);
			{
				if (Gui::BeginTable("MainMetadataTable", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, Gui::GetContentRegionAvail()))
				{
					Gui::TableSetupScrollFreeze(0, 1);
					Gui::TableSetupColumn("Main Metadata", ImGuiTableColumnFlags_None);
					Gui::TableSetupColumn("Value", ImGuiTableColumnFlags_None);
					Gui::TableHeadersRow();
					{
						auto row = [](std::string_view propertyKey, std::string_view propertyValue)
						{
							Gui::TableNextRow();
							Gui::TableNextColumn();
							Gui::TextUnformatted(propertyKey);
							Gui::TableNextColumn();
							if (!propertyValue.empty())
								Gui::TextUnformatted(propertyValue);
							else
								Gui::TextDisabled("n/a");
						};

						const TJA::ParsedMainMetadata& metadata = LoadedTJAFile.Parsed.Metadata;

						char b[256];
						row("Title", metadata.TITLE);
						if (!metadata.TITLE_JA.empty()) row("Title (JA)", metadata.TITLE_JA);
						if (!metadata.TITLE_EN.empty()) row("Title (EN)", metadata.TITLE_EN);
						if (!metadata.TITLE_CN.empty()) row("Title (CN)", metadata.TITLE_CN);
						if (!metadata.TITLE_TW.empty()) row("Title (TW)", metadata.TITLE_TW);
						if (!metadata.TITLE_KO.empty()) row("Title (KO)", metadata.TITLE_KO);
						row("Subtitle", metadata.SUBTITLE);
						if (!metadata.SUBTITLE_JA.empty()) row("Subtitle (JA)", metadata.SUBTITLE_JA);
						if (!metadata.SUBTITLE_EN.empty()) row("Subtitle (EN)", metadata.SUBTITLE_EN);
						if (!metadata.SUBTITLE_CN.empty()) row("Subtitle (CN)", metadata.SUBTITLE_CN);
						if (!metadata.SUBTITLE_TW.empty()) row("Subtitle (TW)", metadata.SUBTITLE_TW);
						if (!metadata.SUBTITLE_KO.empty()) row("Subtitle (KO)", metadata.SUBTITLE_KO);
						row("Song File Name", metadata.WAVE);
						if (!metadata.BGIMAGE.empty()) row("Background Image File Name", metadata.BGIMAGE);
						if (!metadata.BGMOVIE.empty()) row("Background Movie File Name", metadata.BGMOVIE);
						if (!metadata.LYRICS.empty()) row("Lyrics File Name", metadata.LYRICS);
						row("Chart Creator", metadata.MAKER);
						row("Initial Tempo", std::string_view(b, sprintf_s(b, "%g BPM", metadata.BPM.BPM)));
						row("Initial Scroll Speed", std::string_view(b, sprintf_s(b, "%gx", metadata.HEADSCROLL)));
						row("Song Offset", std::string_view(b, sprintf_s(b, "%g sec", metadata.OFFSET.Seconds)));
						row("Movie Offset", std::string_view(b, sprintf_s(b, "%g sec", metadata.MOVIEOFFSET.Seconds)));
						row("Demo Start Time", std::string_view(b, sprintf_s(b, "%g sec", metadata.DEMOSTART.Seconds)));
						row("Song Volume", std::string_view(b, sprintf_s(b, "%g %%", ToPercent(metadata.SONGVOL))));
						row("Sound Effect Volume", std::string_view(b, sprintf_s(b, "%g %%", ToPercent(metadata.SEVOL))));
						row("Score Mode", TJAScoreModeNames[EnumToIndex(metadata.SCOREMODE)]);
						row("Side", TJASongSelectSideNames[EnumToIndex(metadata.SIDE)]);
						row("Life", (metadata.LIFE == 0) ? "" : std::string_view(b, sprintf_s(b, "%d", metadata.LIFE)));
						row("Genre", metadata.GENRE);
						row("Game", TJAGameTypeNames[EnumToIndex(metadata.GAME)]);
						if (!metadata.TAIKOWEBSKIN.empty()) row("Taiko Web Skin", metadata.TAIKOWEBSKIN);
					}
					Gui::EndTable();
				}
			}
			Gui::EndChild();

			//Gui::BeginChild("CourseChild", Gui::GetContentRegionAvail(), true);
			if (Gui::BeginTabBar("CourseTabBar", ImGuiTabBarFlags_None))
			{
				for (size_t courseIndex = 0; courseIndex < LoadedTJAFile.Parsed.Courses.size(); courseIndex++)
				{
					const TJA::ParsedCourse& course = LoadedTJAFile.Parsed.Courses[courseIndex];

					char tabNameBuffer[64];
					sprintf_s(tabNameBuffer, "%s x%d (%s)###Course[%zu]",
						TJADifficultyTypeNames[EnumToIndex(course.Metadata.COURSE)],
						course.Metadata.LEVEL,
						TJAStyleModeNames[EnumToIndex(course.Metadata.STYLE)],
						courseIndex);

					Gui::PushID(static_cast<ImGuiID>(courseIndex));
					if (Gui::BeginTabItem(tabNameBuffer))
					{
						Gui::BeginChild("LeftChild", { Gui::GetContentRegionAvail().x * 0.5f, 0.0f }, false);
						if (Gui::BeginTable("CourseMetadataTable", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, Gui::GetContentRegionAvail()))
						{
							Gui::TableSetupScrollFreeze(0, 1);
							Gui::TableSetupColumn("Course Metadata", ImGuiTableColumnFlags_None);
							Gui::TableSetupColumn("Value", ImGuiTableColumnFlags_None);
							Gui::TableHeadersRow();
							{
								auto row = [](std::string_view propertyKey, std::string_view propertyValue)
								{
									Gui::TableNextRow();
									Gui::TableNextColumn();
									Gui::TextUnformatted(propertyKey);
									Gui::TableNextColumn();
									if (!propertyValue.empty())
										Gui::TextUnformatted(propertyValue);
									else
										Gui::TextDisabled("n/a");
								};

								const TJA::ParsedCourseMetadata& metadata = course.Metadata;

								char b[256];
								row("Difficulty Type", TJADifficultyTypeNames[EnumToIndex(metadata.COURSE)]);
								row("Difficulty Level", std::string_view(b, sprintf_s(b, "%d", metadata.LEVEL)));
								row("Score Init", std::string_view(b, sprintf_s(b, "%d", metadata.SCOREINIT)));
								row("Score Diff", std::string_view(b, sprintf_s(b, "%d", metadata.SCOREDIFF)));
								row("Style", TJAStyleModeNames[EnumToIndex(metadata.STYLE)]);

								auto rowBalloonPops = [](std::string_view propertyKey, const std::vector<i32>& i32s)
								{
									Gui::TableNextRow();
									Gui::TableNextColumn();
									Gui::TextUnformatted(propertyKey);
									Gui::TableNextColumn();
									if (i32s.empty())
										Gui::TextDisabled("n/a");
									else
										for (const i32& v : i32s) { Gui::Text("%d", v); if (&v != &i32s.back()) { Gui::SameLine(0.0f, 0.0f); Gui::Text(", ", v); Gui::SameLine(0.0f, 0.0f); } }
								};

								rowBalloonPops("Balloon Pop Counts", metadata.BALLOON);
								rowBalloonPops("Balloon Pop Counts (Normal)", metadata.BALLOON_Normal);
								rowBalloonPops("Balloon Pop Counts (Expert)", metadata.BALLOON_Expert);
								rowBalloonPops("Balloon Pop Counts (Master)", metadata.BALLOON_Master);
							}
							Gui::EndTable();
						}
						Gui::EndChild();
						Gui::SameLine();
						Gui::BeginChild("RightChild", Gui::GetContentRegionAvail(), false);
						if (Gui::BeginTable("ChartCommandsTable", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, Gui::GetContentRegionAvail()))
						{
							Gui::TableSetupScrollFreeze(0, 1);
							Gui::TableSetupColumn("Chart Command", ImGuiTableColumnFlags_None);
							Gui::TableSetupColumn("Param", ImGuiTableColumnFlags_None);
							Gui::TableHeadersRow();

							for (const TJA::ParsedChartCommand& command : course.ChartCommands)
							{
								char paramBuffer[512] = {};
								const TJA::ParsedChartCommand::ParamData& param = command.Param;

								switch (command.Type)
								{
								case TJA::ParsedChartCommandType::Unknown: {} break;
								case TJA::ParsedChartCommandType::MeasureNotes:
								{
									static std::string strBuffer; strBuffer.clear();
									for (const TJA::NoteType& note : param.MeasureNotes.Notes)
									{
										strBuffer += TJANoteTypeNames[EnumToIndex(note)];
										if (&note != &param.MeasureNotes.Notes.back())
											strBuffer += " ";
									}
									if (!strBuffer.empty())
										memcpy(paramBuffer, strBuffer.data(), strBuffer.size() + sizeof('\0'));
								} break;
								case TJA::ParsedChartCommandType::MeasureEnd: {} break;
								case TJA::ParsedChartCommandType::ChangeTimeSignature: { sprintf_s(paramBuffer, "%d / %d", param.ChangeTimeSignature.Value.Numerator, param.ChangeTimeSignature.Value.Denominator); } break;
								case TJA::ParsedChartCommandType::ChangeTempo: { sprintf_s(paramBuffer, "%g BPM", param.ChangeTempo.Value.BPM); } break;
								case TJA::ParsedChartCommandType::ChangeDelay: { sprintf_s(paramBuffer, "%g sec", param.ChangeDelay.Value.Seconds); } break;
								case TJA::ParsedChartCommandType::ChangeScrollSpeed: { sprintf_s(paramBuffer, "%gx", param.ChangeScrollSpeed.Value); } break;
								case TJA::ParsedChartCommandType::ChangeBarLine: { sprintf_s(paramBuffer, "%s", param.ChangeBarLine.Visible ? "On" : "Off"); } break;
								case TJA::ParsedChartCommandType::GoGoStart: {} break;
								case TJA::ParsedChartCommandType::GoGoEnd: {} break;
								case TJA::ParsedChartCommandType::BranchStart: {} break;
								case TJA::ParsedChartCommandType::BranchNormal: {} break;
								case TJA::ParsedChartCommandType::BranchExpert: {} break;
								case TJA::ParsedChartCommandType::BranchMaster: {} break;
								case TJA::ParsedChartCommandType::BranchEnd: {} break;
								case TJA::ParsedChartCommandType::BranchLevelHold: {} break;
								case TJA::ParsedChartCommandType::ResetAccuracyValues: {} break;
								case TJA::ParsedChartCommandType::SetLyricLine: {} break;
								case TJA::ParsedChartCommandType::BMScroll: {} break;
								case TJA::ParsedChartCommandType::HBScroll: {} break;
								case TJA::ParsedChartCommandType::SENoteChange: {} break;
								case TJA::ParsedChartCommandType::SetNextSong: {} break;
								case TJA::ParsedChartCommandType::ChangeDirection: {} break;
								case TJA::ParsedChartCommandType::SetSudden: {} break;
								case TJA::ParsedChartCommandType::SetScrollTransition: {} break;
								default: {} break;
								}

								Gui::TableNextRow();

								Gui::TableNextColumn();
								Gui::TextUnformatted(TJAParsedChartCommandTypeNames[EnumToIndex(command.Type)]);

								Gui::TableNextColumn();
								Gui::TextUnformatted(paramBuffer);
							}

							Gui::EndTable();
						}
						Gui::EndChild();
						Gui::EndTabItem();
					}
					Gui::PopID();
				}
				Gui::EndTabBar();
			}
			//Gui::EndChild();
		}
		Gui::End();
	}

	void TJATestWindows::DrawGuiConvertedWindow(bool* isOpen)
	{
		if (Gui::Begin("TJA Test - Converted", isOpen, ImGuiWindowFlags_None))
		{
			if (Gui::BeginTabBar("CourseTabBar", ImGuiTabBarFlags_None))
			{
				for (size_t courseIndex = 0; courseIndex < LoadedTJAFile.ConvertedCourses.size(); courseIndex++)
				{
					const TJA::ConvertedCourse& course = LoadedTJAFile.ConvertedCourses[courseIndex];

					char tabNameBuffer[64];
					sprintf_s(tabNameBuffer, "%s x%d (%s)###ConvertedCourse[%zu]",
						TJADifficultyTypeNames[EnumToIndex(course.CourseMetadata.COURSE)],
						course.CourseMetadata.LEVEL,
						TJAStyleModeNames[EnumToIndex(course.CourseMetadata.STYLE)],
						courseIndex);

					static constexpr f32 rowHeight = 36.0f;

					Gui::PushID(&course);
					if (Gui::BeginTabItem(tabNameBuffer))
					{
						Gui::BeginChild("Inner", Gui::GetContentRegionAvail(), true);
						{
							auto* windowDrawList = Gui::GetWindowDrawList();
							const Rect windowRectWithOverdraw = Rect::FromTLSize(vec2(Gui::GetWindowPos()) - vec2(0.0f, rowHeight), vec2(Gui::GetWindowSize()) + vec2(0.0f, rowHeight * 2));

							const f32 xPadding = Min(Gui::GetContentRegionAvail().x / 2.0f, 8.0f);
							Rect rowRect = Rect::FromTLSize(vec2(Gui::GetCursorScreenPos()) + vec2(xPadding, 0.0f), vec2(Max(1.0f, Gui::GetContentRegionAvail().x - (xPadding * 2.0f)), rowHeight));

							Gui::InvisibleButton("Dummy", vec2(rowRect.GetWidth(), static_cast<f32>(course.Measures.size()) * rowHeight));

							i32 maxBeatsInMeasure = 1;
							for (const TJA::ConvertedMeasure& measure : course.Measures)
								maxBeatsInMeasure = Max(maxBeatsInMeasure, measure.TimeSignature.GetBeatsPerBar());

							const f32 screenSpaceXPerBeat = Floor(rowRect.GetWidth() / static_cast<f32>(maxBeatsInMeasure));
							auto beatToScreenSpaceX = [&](Beat beat) -> f32 { return static_cast<f32>(beat.BeatsFraction() * screenSpaceXPerBeat); };

							// TODO: Also draw temo changes etc.
							for (const TJA::ConvertedMeasure& measure : course.Measures)
							{
								const i32 beatsInMeasure = measure.TimeSignature.GetBeatsPerBar();
								rowRect.BR.x = rowRect.TL.x + (screenSpaceXPerBeat * static_cast<f32>(beatsInMeasure));

								if (windowRectWithOverdraw.Contains(rowRect))
								{
									const Beat measureDuration = measure.TimeSignature.GetDurationPerBar();
									for (const auto& gogoRange : course.GoGoRanges)
									{
										// BUG: DOES NOT CORRECTLY HANDLE SUB-MEASURE GOGO RANGES 
										//		(half functional without this if branch but still applies to the wrong measure..?)
										// if (measure.StartTime >= gogoRange.StartTime && (measure.StartTime + measureDuration) <= gogoRange.EndTime)
										{
											const Beat startTimeWithinMeasure = Clamp(gogoRange.StartTime - measure.StartTime, Beat::Zero(), measureDuration);
											const Beat endTimeWithinMeasure = Clamp(gogoRange.EndTime - measure.StartTime, Beat::Zero(), measureDuration);
											if (endTimeWithinMeasure == startTimeWithinMeasure)
												continue;

											const f32 startX = beatToScreenSpaceX(startTimeWithinMeasure);
											const f32 endX = beatToScreenSpaceX(endTimeWithinMeasure);
											windowDrawList->AddRectFilled(rowRect.GetTL() + vec2(startX, 1.0f), rowRect.GetBL() + vec2(endX, -1.0f), TJADrawUtil::GoGoBackground);
										}
									}

									for (i32 beatLine = 0; beatLine <= beatsInMeasure; beatLine++)
									{
										const f32 x = beatToScreenSpaceX(Beat::FromBeats(beatLine));
										windowDrawList->AddLine(rowRect.GetTL() + vec2(x, 0.0f), rowRect.GetBL() + vec2(x, 0.0f), Gui::GetColorU32(ImGuiCol_Separator, 0.5f));
									}
									windowDrawList->AddRect(rowRect.TL, rowRect.BR, Gui::GetColorU32(ImGuiCol_Separator));

									for (const TJA::ConvertedNote& note : measure.Notes)
										TJADrawUtil::DrawNote(windowDrawList, rowRect.TL + vec2(beatToScreenSpaceX(note.TimeWithinMeasure), rowRect.GetHeight() / 2.0f), rowHeight, note.Type);
								}

								rowRect.TL.y += rowHeight;
								rowRect.BR.y += rowHeight;
							}
						}
						Gui::EndChild();
						Gui::EndTabItem();
					}
					Gui::PopID();
				}
				Gui::EndTabBar();
			}
		}
		Gui::End();
	}
}
