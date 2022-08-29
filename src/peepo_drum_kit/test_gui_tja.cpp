#include "test_gui_tja.h"

namespace PeepoDrumKit
{
	static constexpr cstr TJATokenTypeNames[] = { "???", "...", "// comment", "Key:Value", "# Chart Command", "Chart Data" };
	static_assert(ArrayCount(TJATokenTypeNames) == EnumCount<TJA::TokenType>);

	static constexpr cstr TJADifficultyTypeNames[] = { "Easy", "Normal", "Hard", "Oni", "Oni (Ura)", "Tower", "Dan", };
	static_assert(ArrayCount(TJADifficultyTypeNames) == EnumCount<TJA::DifficultyType>);

	static constexpr cstr TJAScoreModeNames[] = { "AC1 - AC7", "AC8 - AC14", "AC0", };
	static_assert(ArrayCount(TJAScoreModeNames) == EnumCount<TJA::ScoreMode>);

	static constexpr cstr TJASongSelectSideNames[] = { "Normal", "Ex", "Both", };
	static_assert(ArrayCount(TJASongSelectSideNames) == EnumCount<TJA::SongSelectSide>);

	static constexpr cstr TJAGameTypeNames[] = { "Taiko", "Jubeat", };
	static_assert(ArrayCount(TJAGameTypeNames) == EnumCount<TJA::GameType>);

	static constexpr cstr TJAStyleModeNames[] = { "Single", "Double", };
	static_assert(ArrayCount(TJAStyleModeNames) == EnumCount<TJA::StyleMode>);

	static constexpr cstr TJAParsedChartCommandTypeNames[] =
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

	static constexpr cstr TJANoteTypeNames[] =
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

	void TJATestWindow::DrawGui(b8* isOpen)
	{
		WasTJAEditedThisFrame = false;

		if (IsFirstFrame)
		{
			LoadedTJAFile.Parsed.Courses.emplace_back();
			IsFirstFrame = false;
		}

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

		if (Gui::Begin("TJA Import Test", isOpen, ImGuiWindowFlags_None))
		{
			const ImVec2 originalFramePadding = Gui::GetStyle().FramePadding;
			Gui::PushStyleVar(ImGuiStyleVar_FramePadding, GuiScale(vec2(10.0f, 5.0f)));
			Gui::PushStyleColor(ImGuiCol_TabHovered, Gui::GetStyleColorVec4(ImGuiCol_HeaderActive));
			Gui::PushStyleColor(ImGuiCol_TabActive, Gui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
			if (Gui::BeginTabBar("TJATestTabs", ImGuiTabBarFlags_None))
			{
				auto beginEndTabItem = [&](cstr label, auto func)
				{
					if (Gui::BeginTabItem(label)) { Gui::PushStyleVar(ImGuiStyleVar_FramePadding, originalFramePadding); func(); Gui::PopStyleVar(); Gui::EndTabItem(); }
				};
				beginEndTabItem("Parsed", [this] { DrawGuiParsedTabContent(); });
				beginEndTabItem("Tokens", [this] { DrawGuiTokensTabContent(); });
				beginEndTabItem("File Content", [this] { DrawGuiFileContentTabContent(); });
				Gui::EndTabBar();
			}
			Gui::PopStyleColor(2);
			Gui::PopStyleVar();
		}
		Gui::End();
	}

	void TJATestWindow::DrawGuiFileContentTabContent()
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
	}

	void TJATestWindow::DrawGuiTokensTabContent()
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

	void TJATestWindow::DrawGuiParsedTabContent()
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
	}
}
