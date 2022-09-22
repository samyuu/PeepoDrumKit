#include "chart_editor.h"
#include "core_build_info.h"
#include "chart_editor_undo.h"
#include "audio/audio_file_formats.h"
#include "chart_editor_i18n.h"

namespace PeepoDrumKit
{
	static constexpr std::string_view UntitledChartFileName = "Untitled Chart.tja";

	static constexpr f32 PresetGuiScaleFactors[] = { 0.5, (2.0f / 3.0f), 0.75f, 0.8f, 0.9f, 1.0f, 1.1f, 1.25f, 1.5f, 1.75f, 2.0f, 2.5f, 3.0f, };
	static constexpr f32 PresetGuiScaleFactorMin = PresetGuiScaleFactors[0];
	static constexpr f32 PresetGuiScaleFactorMax = PresetGuiScaleFactors[ArrayCount(PresetGuiScaleFactors) - 1];
	static constexpr f32 NextPresetGuiScaleFactor(f32 current, i32 direction)
	{
		i32 closest = 0;
		for (const f32& it : PresetGuiScaleFactors) if (it <= current || ApproxmiatelySame(it, current)) closest = ArrayItToIndexI32(&it, &PresetGuiScaleFactors[0]);
		return ClampRoundGuiScaleFactor(PresetGuiScaleFactors[Clamp(closest + direction, 0, ArrayCountI32(PresetGuiScaleFactors) - 1)]);
	}

	static b8 CanZoomInGuiScale() { return (GuiScaleFactorTarget < PresetGuiScaleFactorMax); }
	static b8 CanZoomOutGuiScale() { return (GuiScaleFactorTarget > PresetGuiScaleFactorMin); }
	static b8 CanZoomResetGuiScale() { return (GuiScaleFactorTarget != 1.0f); }
	static void ZoomInGuiScale() { GuiScaleFactorToSetNextFrame = NextPresetGuiScaleFactor(GuiScaleFactorTarget, +1); }
	static void ZoomOutGuiScale() { GuiScaleFactorToSetNextFrame = NextPresetGuiScaleFactor(GuiScaleFactorTarget, -1); }
	static void ZoomResetGuiScale() { GuiScaleFactorToSetNextFrame = 1.0f; }

	static b8 CanOpenChartDirectoryInFileExplorer(const ChartContext& context)
	{
		return !context.ChartFilePath.empty();
	}

	static void OpenChartDirectoryInFileExplorer(const ChartContext& context)
	{
		const std::string_view chartDirectory = Path::GetDirectoryName(context.ChartFilePath);
		if (!chartDirectory.empty() && Directory::Exists(chartDirectory))
			Shell::OpenInExplorer(chartDirectory);
	}

	static void SetChartDefaultSettingsAndCourses(ChartProject& outChart)
	{
		outChart.ChartCreator = *Settings.General.DefaultCreatorName;

		assert(!outChart.Courses.empty() && "Expected to have initialized the base course first");
		for (auto& course : outChart.Courses)
		{
			course->TempoMap.Tempo.Sorted = { TempoChange(Beat::Zero(), Tempo(160.0f)) };
			course->TempoMap.Signature.Sorted = { TimeSignatureChange(Beat::Zero(), TimeSignature(4, 4)) };
			course->TempoMap.RebuildAccelerationStructure();
		}
	}

	static b8 GlobalLastSetRequestExclusiveDeviceAccessAudioSetting = {};

	ChartEditor::ChartEditor()
	{
		context.SongVoice = Audio::Engine.AddVoice(Audio::SourceHandle::Invalid, "ChartEditor SongVoice", false, 1.0f, true);
		context.SfxVoicePool.StartAsyncLoadingAndAddVoices();

		context.ChartSelectedCourse = context.Chart.Courses.emplace_back(std::make_unique<ChartCourse>()).get();
		SetChartDefaultSettingsAndCourses(context.Chart);

		GlobalLastSetRequestExclusiveDeviceAccessAudioSetting = *Settings.Audio.RequestExclusiveDeviceAccess;
		Audio::Engine.SetBackend(*Settings.Audio.RequestExclusiveDeviceAccess ? Audio::Backend::WASAPI_Exclusive : Audio::Backend::WASAPI_Shared);
		Audio::Engine.SetMasterVolume(0.75f);
		if (*Settings.Audio.OpenDeviceOnStartup)
			Audio::Engine.OpenStartStream();
	}

	ChartEditor::~ChartEditor()
	{
		context.SfxVoicePool.UnloadAllSourcesAndVoices();
	}

	void ChartEditor::DrawFullscreenMenuBar()
	{
		if (Gui::BeginMenuBar())
		{
			GuiLanguage nextLanguageToSelect = SelectedGuiLanguage;
			defer { SelectedGuiLanguage = nextLanguageToSelect; };

			if (Gui::BeginMenu(UI_Str("File")))
			{
				if (Gui::MenuItem(UI_Str("New Chart"), ToShortcutString(*Settings.Input.Editor_ChartNew).Data)) { CheckOpenSaveConfirmationPopupThenCall([&] { CreateNewChart(context); }); }
				if (Gui::MenuItem(UI_Str("Open..."), ToShortcutString(*Settings.Input.Editor_ChartOpen).Data)) { CheckOpenSaveConfirmationPopupThenCall([&] { OpenLoadChartFileDialog(context); }); }

				if (Gui::BeginMenu(UI_Str("Open Recent"), !PersistentApp.RecentFiles.SortedPaths.empty()))
				{
					for (size_t i = 0; i < PersistentApp.RecentFiles.SortedPaths.size(); i++)
					{
						const auto& path = PersistentApp.RecentFiles.SortedPaths[i];
						const char shortcutDigit[2] = { (i <= ('9' - '1')) ? static_cast<char>('1' + i) : '\0', '\0' };

						if (Gui::MenuItem(path.c_str(), shortcutDigit))
						{
							if (File::Exists(path))
							{
								CheckOpenSaveConfirmationPopupThenCall([this, pathCopy = std::string(path)] { StartAsyncImportingChartFile(pathCopy); });
							}
							else
							{
								// TODO: Popup asking to remove from list
							}
						}
					}
					Gui::Separator();
					if (Gui::MenuItem(UI_Str("Clear Items")))
						PersistentApp.RecentFiles.SortedPaths.clear();
					Gui::EndMenu();
				}

				if (Gui::MenuItem(UI_Str("Open Chart Directory..."), ToShortcutString(*Settings.Input.Editor_ChartOpenDirectory).Data, nullptr, CanOpenChartDirectoryInFileExplorer(context))) { OpenChartDirectoryInFileExplorer(context); }
				Gui::Separator();
				if (Gui::MenuItem(UI_Str("Save"), ToShortcutString(*Settings.Input.Editor_ChartSave).Data)) { TrySaveChartOrOpenSaveAsDialog(context); }
				if (Gui::MenuItem(UI_Str("Save As..."), ToShortcutString(*Settings.Input.Editor_ChartSaveAs).Data)) { OpenChartSaveAsDialog(context); }
				Gui::Separator();
				if (Gui::MenuItem(UI_Str("Exit"), ToShortcutString(InputBinding(ImGuiKey_F4, ImGuiModFlags_Alt)).Data))
					tryToCloseApplicationOnNextFrame = true;
				Gui::EndMenu();
			}

			size_t selectedItemCount = 0, selectedNoteCount = 0;
			ForEachSelectedChartItem(*context.ChartSelectedCourse, [&](const ForEachChartItemData& it) { selectedItemCount++; selectedNoteCount += IsNotesList(it.List); });
			const b8 isAnyItemSelected = (selectedItemCount > 0);
			const b8 isAnyNoteSelected = (selectedNoteCount > 0);

			if (Gui::BeginMenu(UI_Str("Edit")))
			{
				if (Gui::MenuItem(UI_Str("Undo"), ToShortcutString(*Settings.Input.Editor_Undo).Data, nullptr, context.Undo.CanUndo())) { context.Undo.Undo(); }
				if (Gui::MenuItem(UI_Str("Redo"), ToShortcutString(*Settings.Input.Editor_Redo).Data, nullptr, context.Undo.CanRedo())) { context.Undo.Redo(); }
				Gui::Separator();
				if (Gui::MenuItem(UI_Str("Cut"), ToShortcutString(*Settings.Input.Timeline_Cut).Data, nullptr, isAnyItemSelected)) { timeline.ExecuteClipboardAction(context, ClipboardAction::Cut); }
				if (Gui::MenuItem(UI_Str("Copy"), ToShortcutString(*Settings.Input.Timeline_Copy).Data, nullptr, isAnyItemSelected)) { timeline.ExecuteClipboardAction(context, ClipboardAction::Copy); }
				if (Gui::MenuItem(UI_Str("Paste"), ToShortcutString(*Settings.Input.Timeline_Paste).Data, nullptr, true)) { timeline.ExecuteClipboardAction(context, ClipboardAction::Paste); }
				if (Gui::MenuItem(UI_Str("Delete"), ToShortcutString(*Settings.Input.Timeline_DeleteSelection).Data, nullptr, isAnyItemSelected)) { timeline.ExecuteClipboardAction(context, ClipboardAction::Delete); }
				Gui::Separator();
				if (Gui::MenuItem(UI_Str("Settings"), ToShortcutString(*Settings.Input.Editor_OpenSettings).Data)) { PersistentApp.LastSession.ShowWindow_Settings = focusSettingsWindowNextFrame = true; }
				Gui::EndMenu();
			}

			if (Gui::BeginMenu(UI_Str("Selection")))
			{
				const b8 setRangeSelectionStartNext = (!timeline.RangeSelection.IsActive || timeline.RangeSelection.HasEnd);
				if (Gui::MenuItem(setRangeSelectionStartNext ? UI_Str("Start Range Selection") : UI_Str("End Range Selection"), ToShortcutString(*Settings.Input.Timeline_StartEndRangeSelection).Data))
					timeline.StartEndRangeSelectionAtCursor(context);
				Gui::Separator();

				SelectionActionParam param {};
				if (Gui::MenuItem(UI_Str("Select All"), ToShortcutString(*Settings.Input.Timeline_SelectAll).Data))
					timeline.ExecuteSelectionAction(context, SelectionAction::SelectAll, param);
				if (Gui::MenuItem(UI_Str("Clear Selection"), ToShortcutString(*Settings.Input.Timeline_ClearSelection).Data, false, isAnyItemSelected))
					timeline.ExecuteSelectionAction(context, SelectionAction::UnselectAll, param);
				if (Gui::MenuItem(UI_Str("Invert Selection"), ToShortcutString(*Settings.Input.Timeline_InvertSelection).Data))
					timeline.ExecuteSelectionAction(context, SelectionAction::InvertAll, param);
				if (Gui::MenuItem(UI_Str("From Range Selection"), ToShortcutString(*Settings.Input.Timeline_SelectAllWithinRangeSelection).Data, nullptr, timeline.RangeSelection.IsActiveAndHasEnd()))
					timeline.ExecuteSelectionAction(context, SelectionAction::SelectAllWithinRangeSelection, param);
				Gui::Separator();

				if (Gui::BeginMenu(UI_Str("Refine Selection")))
				{
					if (Gui::MenuItem(UI_Str("Shift selection Left"), ToShortcutString(*Settings.Input.Timeline_ShiftSelectionLeft).Data, nullptr, isAnyItemSelected))
						timeline.ExecuteSelectionAction(context, SelectionAction::PerRowShiftSelected, param.SetShiftDelta(-1));
					if (Gui::MenuItem(UI_Str("Shift selection Right"), ToShortcutString(*Settings.Input.Timeline_ShiftSelectionRight).Data, nullptr, isAnyItemSelected))
						timeline.ExecuteSelectionAction(context, SelectionAction::PerRowShiftSelected, param.SetShiftDelta(+1));
					Gui::Separator();

					if (Gui::MenuItem(UI_Str("Select Item Pattern xo"), ToShortcutString(*Settings.Input.Timeline_SelectItemPattern_xo).Data, nullptr, isAnyItemSelected))
						timeline.ExecuteSelectionAction(context, SelectionAction::PerRowSelectPattern, param.SetPattern("xo"));
					if (Gui::MenuItem(UI_Str("Select Item Pattern xoo"), ToShortcutString(*Settings.Input.Timeline_SelectItemPattern_xoo).Data, nullptr, isAnyItemSelected))
						timeline.ExecuteSelectionAction(context, SelectionAction::PerRowSelectPattern, param.SetPattern("xoo"));
					if (Gui::MenuItem(UI_Str("Select Item Pattern xooo"), ToShortcutString(*Settings.Input.Timeline_SelectItemPattern_xooo).Data, nullptr, isAnyItemSelected))
						timeline.ExecuteSelectionAction(context, SelectionAction::PerRowSelectPattern, param.SetPattern("xooo"));
					if (Gui::MenuItem(UI_Str("Select Item Pattern xxoo"), ToShortcutString(*Settings.Input.Timeline_SelectItemPattern_xxoo).Data, nullptr, isAnyItemSelected))
						timeline.ExecuteSelectionAction(context, SelectionAction::PerRowSelectPattern, param.SetPattern("xxoo"));
					Gui::Separator();

					WithDefault<CustomSelectionPatternList>& customPatterns = Settings_Mutable.General.CustomSelectionPatterns;
					WithDefault<MultiInputBinding>* customBindings[] =
					{
						&Settings_Mutable.Input.Timeline_SelectItemPattern_CustomA, &Settings_Mutable.Input.Timeline_SelectItemPattern_CustomB, &Settings_Mutable.Input.Timeline_SelectItemPattern_CustomC,
						&Settings_Mutable.Input.Timeline_SelectItemPattern_CustomD, &Settings_Mutable.Input.Timeline_SelectItemPattern_CustomE, &Settings_Mutable.Input.Timeline_SelectItemPattern_CustomF,
					};

					const b8 disableAddNew = (customPatterns->V.size() >= 6);
					if (Gui::Selectable(UI_Str("Add New Pattern..."), false, ImGuiSelectableFlags_DontClosePopups | (disableAddNew ? ImGuiSelectableFlags_Disabled : 0)))
					{
						static constexpr std::string_view defaultPattern = "xoooooo";
						const std::string_view newPattern = defaultPattern.substr(0, ClampTop<size_t>(customPatterns->V.size() + 2, defaultPattern.size()));

						CopyStringViewIntoFixedBuffer(customPatterns->V.emplace_back().Data, newPattern);
						customPatterns.SetHasValueIfNotDefault();
						Settings_Mutable.IsDirty = true;
					}

					for (size_t i = 0; i < customPatterns->V.size(); i++)
					{
						char label[64]; sprintf_s(label, "%s %c", UI_Str("Select Custom Pattern"), static_cast<char>('A' + i));
						if (Gui::MenuItem(label, (i < ArrayCount(customBindings)) ? ToShortcutString(**customBindings[i]).Data : "", nullptr, isAnyItemSelected && customPatterns->V[i].Data[0] != '\0'))
							timeline.ExecuteSelectionAction(context, SelectionAction::PerRowSelectPattern, param.SetPattern(customPatterns->V[i].Data));
					}

					size_t indexToRemove = customPatterns->V.size();
					for (size_t i = 0; i < customPatterns->V.size(); i++)
					{
						Gui::PushID(&customPatterns->V[i]);
						Gui::PushStyleVar(ImGuiStyleVar_FramePadding, vec2(Gui::GetStyle().FramePadding.x, 0.0f));
						{
							char label[] = { static_cast<char>('A' + i), '\0' };
							Gui::TextUnformatted(label);
							Gui::SameLine();

							static constexpr auto textFilterSelectionPattern = [](ImGuiInputTextCallbackData* data) -> int { return (data->EventChar == 'x' || data->EventChar == 'o') ? 0 : 1; };
							Gui::SetNextItemWidth(Gui::GetContentRegionAvail().x);
							if (Gui::InputTextWithHint("##", UI_Str("Delete?"), customPatterns->V[i].Data, sizeof(CustomSelectionPattern::Data), ImGuiInputTextFlags_CallbackCharFilter, textFilterSelectionPattern))
							{
								customPatterns.SetHasValueIfNotDefault();
								Settings_Mutable.IsDirty = true;
							}

							b8* inputActiveLastFrame = Gui::GetCurrentWindow()->StateStorage.GetBoolRef(Gui::GetID("CustomPatternInputActive"), false);
							if (!Gui::IsItemActive() && *inputActiveLastFrame && customPatterns->V[i].Data[0] == '\0')
								indexToRemove = i;
							*inputActiveLastFrame = Gui::IsItemActive();
						}
						Gui::PopStyleVar();
						Gui::PopID();
					}

					if (indexToRemove < customPatterns->V.size())
					{
						customPatterns->V.erase(customPatterns->V.begin() + indexToRemove);
						customPatterns.SetHasValueIfNotDefault();
						Settings_Mutable.IsDirty = true;
					}

					Gui::EndMenu();
				}
				Gui::EndMenu();
			}

			if (Gui::BeginMenu(UI_Str("Transform")))
			{
				TransformActionParam param {};
				if (Gui::MenuItem(UI_Str("Flip Note Types"), ToShortcutString(*Settings.Input.Timeline_FlipNoteType).Data, nullptr, isAnyNoteSelected))
					timeline.ExecuteTransformAction(context, TransformAction::FlipNoteType, param);
				if (Gui::MenuItem(UI_Str("Toggle Note Sizes"), ToShortcutString(*Settings.Input.Timeline_ToggleNoteSize).Data, nullptr, isAnyNoteSelected))
					timeline.ExecuteTransformAction(context, TransformAction::ToggleNoteSize, param);

				if (Gui::BeginMenu(UI_Str("Expand Items")))
				{
					if (Gui::MenuItem(UI_Str("2:1 (8th to 4th)"), ToShortcutString(*Settings.Input.Timeline_ExpandItemTime_2To1).Data, nullptr, isAnyItemSelected))
						timeline.ExecuteTransformAction(context, TransformAction::ScaleItemTime, param.SetTimeRatio(2, 1));
					if (Gui::MenuItem(UI_Str("3:2 (12th to 8th)"), ToShortcutString(*Settings.Input.Timeline_ExpandItemTime_3To2).Data, nullptr, isAnyItemSelected))
						timeline.ExecuteTransformAction(context, TransformAction::ScaleItemTime, param.SetTimeRatio(3, 2));
					if (Gui::MenuItem(UI_Str("4:3 (16th to 12th)"), ToShortcutString(*Settings.Input.Timeline_ExpandItemTime_4To3).Data, nullptr, isAnyItemSelected))
						timeline.ExecuteTransformAction(context, TransformAction::ScaleItemTime, param.SetTimeRatio(4, 3));
					Gui::EndMenu();
				}

				if (Gui::BeginMenu(UI_Str("Compress Items")))
				{
					if (Gui::MenuItem(UI_Str("1:2 (4th to 8th)"), ToShortcutString(*Settings.Input.Timeline_CompressItemTime_1To2).Data, nullptr, isAnyItemSelected))
						timeline.ExecuteTransformAction(context, TransformAction::ScaleItemTime, param.SetTimeRatio(1, 2));
					if (Gui::MenuItem(UI_Str("2:3 (8th to 12th)"), ToShortcutString(*Settings.Input.Timeline_CompressItemTime_2To3).Data, nullptr, isAnyItemSelected))
						timeline.ExecuteTransformAction(context, TransformAction::ScaleItemTime, param.SetTimeRatio(2, 3));
					if (Gui::MenuItem(UI_Str("3:4 (12th to 16th)"), ToShortcutString(*Settings.Input.Timeline_CompressItemTime_3To4).Data, nullptr, isAnyItemSelected))
						timeline.ExecuteTransformAction(context, TransformAction::ScaleItemTime, param.SetTimeRatio(3, 4));
					Gui::EndMenu();
				}

				Gui::EndMenu();
			}

			if (Gui::BeginMenu(UI_Str("Window")))
			{
				if (b8 v = (ApplicationHost::GlobalState.SwapInterval != 0); Gui::MenuItem(UI_Str("Toggle VSync"), ToShortcutString(*Settings.Input.Editor_ToggleVSync).Data, &v))
					ApplicationHost::GlobalState.SwapInterval = v ? 1 : 0;

				if (Gui::MenuItem(UI_Str("Toggle Fullscreen"), ToShortcutString(*Settings.Input.Editor_ToggleFullscreen).Data, ApplicationHost::GlobalState.IsBorderlessFullscreen))
					ApplicationHost::GlobalState.SetBorderlessFullscreenNextFrame = !ApplicationHost::GlobalState.IsBorderlessFullscreen;

				if (Gui::BeginMenu(UI_Str("Window Size")))
				{
					const b8 allowResize = !ApplicationHost::GlobalState.IsBorderlessFullscreen;
					const ivec2 currentResolution = ApplicationHost::GlobalState.WindowSize;

					struct NamedResolution { ivec2 Resolution; cstr Name; };
					static constexpr NamedResolution presetResolutions[] =
					{
						{ { 1280,  720 }, "HD" },
						{ { 1366,  768 }, "FWXGA" },
						{ { 1600,  900 }, "HD+" },
						{ { 1920, 1080 }, "FHD" },
						{ { 2560, 1440 }, "QHD" },
					};

					char labelBuffer[128];
					for (auto[resolution, name] : presetResolutions)
					{
						sprintf_s(labelBuffer, "%s %dx%d", UI_Str("Resize to"), resolution.x, resolution.y);
						if (Gui::MenuItem(labelBuffer, name, (resolution == currentResolution), allowResize))
							ApplicationHost::GlobalState.SetWindowSizeNextFrame = resolution;
					}

					Gui::Separator();
					sprintf_s(labelBuffer, "%s: %dx%d", UI_Str("Current Size"), currentResolution.x, currentResolution.y);
					Gui::MenuItem(labelBuffer, nullptr, nullptr, false);

					Gui::EndMenu();
				}

				if (Gui::BeginMenu(UI_Str("DPI Scale")))
				{
					const f32 guiScaleFactorToSetNextFrame = GuiScaleFactorToSetNextFrame;
					if (Gui::MenuItem(UI_Str("Zoom In"), ToShortcutString(*Settings.Input.Editor_IncreaseGuiScale).Data, nullptr, CanZoomInGuiScale())) ZoomInGuiScale();
					if (Gui::MenuItem(UI_Str("Zoom Out"), ToShortcutString(*Settings.Input.Editor_DecreaseGuiScale).Data, nullptr, CanZoomOutGuiScale())) ZoomOutGuiScale();
					if (Gui::MenuItem(UI_Str("Reset Zoom"), ToShortcutString(*Settings.Input.Editor_ResetGuiScale).Data, nullptr, CanZoomResetGuiScale())) ZoomResetGuiScale();

					if (guiScaleFactorToSetNextFrame != GuiScaleFactorToSetNextFrame) { if (!zoomPopup.IsOpen) zoomPopup.Open(); zoomPopup.OnChange(); }

					Gui::Separator();
					char labelBuffer[128]; sprintf_s(labelBuffer, "%s: %g%%", UI_Str("Current Scale"), ToPercent(GuiScaleFactorTarget));
					Gui::MenuItem(labelBuffer, nullptr, nullptr, false);

					Gui::EndMenu();
				}

				Gui::EndMenu();
			}

			if (Gui::BeginMenu(UI_Str("Language")))
			{
				const struct { GuiLanguage Language; cstr Code, Name, CurrentName; } languages[] =
				{
					{ GuiLanguage::EN, "en", "English", UI_Str("English"), },
					{ GuiLanguage::JA, "ja", "Japanese", UI_Str("Japanese"), },
				};
				static_assert(ArrayCount(languages) == EnumCount<GuiLanguage>);

				for (const auto& it : languages)
				{
					const cstr localName = i18n::HashToString(i18n::Hash(it.Name), it.Language);

					char labelBuffer[128];
					sprintf_s(labelBuffer, UI_Str("%s (%s)"), it.CurrentName, (strcmp(it.Name, it.CurrentName) == 0) ? localName : it.Name);

					if (Gui::MenuItem(labelBuffer, it.Code, (SelectedGuiLanguage == it.Language)))
						nextLanguageToSelect = it.Language;
				}
				Gui::EndMenu();
			}

			if ((PEEPO_DEBUG || PersistentApp.LastSession.ShowWindow_TestMenu) && Gui::BeginMenu(UI_Str("Test Menu")))
			{
				Gui::MenuItem(UI_Str("Show Audio Test"), "(Debug)", &PersistentApp.LastSession.ShowWindow_AudioTest);
				Gui::MenuItem(UI_Str("Show TJA Import Test"), "(Debug)", &PersistentApp.LastSession.ShowWindow_TJAImportTest);
				Gui::MenuItem(UI_Str("Show TJA Export View"), "(Debug)", &PersistentApp.LastSession.ShowWindow_TJAExportTest);
#if !defined(IMGUI_DISABLE_DEMO_WINDOWS)
				Gui::Separator();
				Gui::MenuItem(UI_Str("Show ImGui Demo"), " ", &PersistentApp.LastSession.ShowWindow_ImGuiDemo);
				Gui::MenuItem(UI_Str("Show ImGui Style Editor"), " ", &PersistentApp.LastSession.ShowWindow_ImGuiStyleEditor);
				Gui::Separator();
				if (Gui::MenuItem(UI_Str("Reset Style Colors"), " "))
					GuiStyleColorPeepoDrumKit();
#endif

#if PEEPO_DEBUG
				if (Gui::BeginMenu("Embedded Icons Test"))
				{
					for (size_t i = 0; i < GetEmbeddedIconsList().Count; i++)
					{
						if (auto icon = GetEmbeddedIconsList().V[i]; Gui::MenuItem(icon.Name, icon.UTF8))
							Gui::SetClipboardText(icon.UTF8);
					}
					Gui::EndMenu();
				}
#endif

				Gui::EndMenu();
			}

			if (Gui::BeginMenu(UI_Str("Help")))
			{
				Gui::MenuItem(UI_Str("Copyright (c) 2022"), "Samyuu", false, false);
				Gui::MenuItem(UI_Str("Build Time:"), BuildInfo::CompilationTime(), false, false);
				Gui::MenuItem(UI_Str("Build Date:"), BuildInfo::CompilationDate(), false, false);
				Gui::MenuItem(UI_Str("Build Configuration:"), UI_Str(BuildInfo::BuildConfiguration()), false, false);
				Gui::Separator();
				if (Gui::MenuItem(UI_Str("Usage Guide"), ToShortcutString(*Settings.Input.Editor_OpenHelp).Data)) { PersistentApp.LastSession.ShowWindow_Help = focusHelpWindowNextFrame = true; }
				Gui::EndMenu();
			}

			static constexpr Audio::Backend availableBackends[] = { Audio::Backend::WASAPI_Shared, Audio::Backend::WASAPI_Exclusive, };
			static constexpr auto backendToString = [](Audio::Backend backend) -> cstr
			{
				return (backend == Audio::Backend::WASAPI_Shared) ? "WASAPI Shared" : (backend == Audio::Backend::WASAPI_Exclusive) ? "WASAPI Exclusive" : "Invalid";
			};

			char performanceTextBuffer[64];
			sprintf_s(performanceTextBuffer, "[ %.3f ms (%.1f FPS) ]", (1000.0f / Gui::GetIO().Framerate), Gui::GetIO().Framerate);

			char audioTextBuffer[128];
			if (Audio::Engine.GetIsStreamOpenRunning())
			{
				sprintf_s(audioTextBuffer, "[ %gkHz %zubit %dch ~%.0fms %s ]",
					static_cast<f64>(Audio::Engine.OutputSampleRate) / 1000.0,
					sizeof(i16) * BitsPerByte,
					Audio::Engine.OutputChannelCount,
					Audio::FramesToTime(Audio::Engine.GetBufferFrameSize(), Audio::Engine.OutputSampleRate).ToMS(),
					backendToString(Audio::Engine.GetBackend()));
			}
			else
			{
				strcpy_s(audioTextBuffer, "[ Audio Device Closed ]");
			}

			const f32 perItemItemSpacing = (Gui::GetStyle().ItemSpacing.x * 2.0f);
			const f32 performanceMenuWidth = Gui::CalcTextSize(performanceTextBuffer).x + perItemItemSpacing;
			const f32 audioMenuWidth = Gui::CalcTextSize(audioTextBuffer).x + perItemItemSpacing;

			{
				Gui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
				if (Gui::BeginMenu(UI_Str("Courses")))
				{
					// TODO:
					Gui::MenuItem(UI_Str("Add New"), "(TODO)", nullptr, false);
					Gui::MenuItem(UI_Str("Edit..."), "(TODO)", nullptr, false);
					Gui::EndMenu();
				}

				if (Gui::BeginChild("MenuBarTabsChild", vec2(-(audioMenuWidth + performanceMenuWidth + Gui::GetStyle().ItemSpacing.x), 0.0f)))
				{
					// NOTE: To essentially make these tab items look similar to regular menu items (the inverted Active <-> Hovered colors are not a mistake)
					Gui::PushStyleColor(ImGuiCol_TabHovered, Gui::GetStyleColorVec4(ImGuiCol_HeaderActive));
					Gui::PushStyleColor(ImGuiCol_TabActive, Gui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
					if (Gui::BeginTabBar("MenuBarTabs", ImGuiTabBarFlags_FittingPolicyScroll))
					{
						// HACK: How to properly manage the imgui selected tab internal state..?
						static const ChartCourse* lastFrameSelectedCoursePtrID = nullptr;
						b8 isAnyCourseTabSelected = false;

						for (std::unique_ptr<ChartCourse>& course : context.Chart.Courses)
						{
							char buffer[64]; sprintf_s(buffer, "%s x%d (%s)###Course_%p", UI_StrRuntime(DifficultyTypeNames[EnumToIndex(course->Type)]), static_cast<i32>(course->Level), UI_Str("Single"), course.get());
							const b8 setSelectedThisFrame = (course.get() == context.ChartSelectedCourse && course.get() != lastFrameSelectedCoursePtrID);

							Gui::PushID(course.get());
							if (Gui::BeginTabItem(buffer, nullptr, setSelectedThisFrame ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
							{
								// TODO: Selecting a course should also be an undo command so that there isn't ever any confusion (?)
								context.ChartSelectedCourse = course.get();
								isAnyCourseTabSelected = true;
								Gui::EndTabItem();
							}
							Gui::PopID();
						}

						lastFrameSelectedCoursePtrID = context.ChartSelectedCourse;
						if (!isAnyCourseTabSelected)
						{
							assert(!context.Chart.Courses.empty() && "Courses must never be empty so that the selected course always points to a valid object");
							context.ChartSelectedCourse = context.Chart.Courses.front().get();
						}

						Gui::EndTabBar();
					}
					Gui::PopStyleColor(2);
				}
				Gui::EndChild();
			}

			// NOTE Right-aligned peformance / audio display
			{
				// TODO: Redirect to an audio settings window instead similar to how it works in REAPER for example (?)
				Gui::SetCursorPos(vec2(Gui::GetWindowWidth() - performanceMenuWidth - audioMenuWidth, Gui::GetCursorPos().y));
				if (Gui::BeginMenu(audioTextBuffer))
				{
					const b8 deviceIsOpen = Audio::Engine.GetIsStreamOpenRunning();
					if (Gui::MenuItem(UI_Str("Open Audio Device"), nullptr, false, !deviceIsOpen))
						Audio::Engine.OpenStartStream();
					if (Gui::MenuItem(UI_Str("Close Audio Device"), nullptr, false, deviceIsOpen))
						Audio::Engine.StopCloseStream();
					Gui::Separator();

					const Audio::Backend currentBackend = Audio::Engine.GetBackend();
					for (const Audio::Backend backendType : availableBackends)
					{
						char labelBuffer[128];
						sprintf_s(labelBuffer, UI_Str("Use %s"), backendToString(backendType));
						if (Gui::MenuItem(labelBuffer, nullptr, (backendType == currentBackend), (backendType != currentBackend)))
							Audio::Engine.SetBackend(backendType);
					}
					Gui::EndMenu();
				}

				if (Gui::MenuItem(performanceTextBuffer))
					performance.ShowOverlay ^= true;

				if (performance.ShowOverlay)
				{
					const ImGuiViewport* mainViewport = Gui::GetMainViewport();
					Gui::SetNextWindowPos(vec2(mainViewport->Pos.x + mainViewport->Size.x, mainViewport->Pos.y + GuiScale(24.0f)), ImGuiCond_Always, vec2(1.0f, 0.0f));
					Gui::SetNextWindowViewport(mainViewport->ID);
					Gui::PushStyleColor(ImGuiCol_WindowBg, Gui::GetStyleColorVec4(ImGuiCol_PopupBg));
					Gui::PushStyleColor(ImGuiCol_FrameBg, Gui::GetColorU32(ImGuiCol_FrameBg, 0.0f));
					Gui::PushStyleColor(ImGuiCol_PlotLines, Gui::GetStyleColorVec4(ImGuiCol_TextDisabled));

					static constexpr ImGuiWindowFlags overlayWindowFlags =
						ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
						ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
						ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

					if (Gui::Begin("PerformanceOverlay", nullptr, overlayWindowFlags))
					{
						performance.FrameTimesMS[performance.FrameTimeIndex++] = (Gui::GetIO().DeltaTime * 1000.0f);
						if (performance.FrameTimeIndex >= ArrayCount(performance.FrameTimesMS)) performance.FrameTimeIndex = 0;
						performance.FrameTimeCount = Max(performance.FrameTimeIndex, performance.FrameTimeCount);

						f32 averageFrameTime = 0.0f, minFrameTime = F32Max, maxFrameTime = F32Min;
						for (size_t i = 0; i < performance.FrameTimeCount; i++) averageFrameTime += performance.FrameTimesMS[i]; averageFrameTime /= static_cast<f32>(performance.FrameTimeCount);
						for (size_t i = 0; i < performance.FrameTimeCount; i++) minFrameTime = Min(minFrameTime, performance.FrameTimesMS[i]);
						for (size_t i = 0; i < performance.FrameTimeCount; i++) maxFrameTime = Max(maxFrameTime, performance.FrameTimesMS[i]);

						static constexpr f32 plotLinesHeight = 120.0f;
						const f32 scaleMin = ClampBot((1000.0f / 30.0f), minFrameTime - 0.001f);
						const f32 scaleMax = ClampTop((1000.0f / 1000.0f), maxFrameTime + 0.001f);
						Gui::PlotLines("##PerformanceHistoryPlot", performance.FrameTimesMS, ArrayCountI32(performance.FrameTimesMS), 0,
							"", scaleMin, scaleMax, GuiScale(vec2(static_cast<f32>(ArrayCount(performance.FrameTimesMS)), plotLinesHeight)));
						const Rect plotLinesRect = Gui::GetItemRect();

						char overlayTextBuffer[64];
						const auto overlayText = std::string_view(overlayTextBuffer, sprintf_s(overlayTextBuffer,
							"%s%.3f ms\n"
							"%s%.3f ms\n"
							"%s%.3f ms",
							UI_Str("Average: "), averageFrameTime,
							UI_Str("Min: "), minFrameTime,
							UI_Str("Max: "), maxFrameTime));

						const vec2 overlayTextSize = Gui::CalcTextSize(overlayText);
						const Rect overlayTextRect = Rect::FromTLSize(plotLinesRect.GetCenter() - (overlayTextSize * 0.5f) - vec2(0.0f, plotLinesRect.GetHeight() / 4.0f), overlayTextSize);
						Gui::GetWindowDrawList()->AddRectFilled(overlayTextRect.TL - vec2(2.0f), overlayTextRect.BR + vec2(2.0f), Gui::GetColorU32(ImGuiCol_WindowBg, 0.5f));
						Gui::AddTextWithDropShadow(Gui::GetWindowDrawList(), overlayTextRect.TL, Gui::GetColorU32(ImGuiCol_Text), overlayText, 0xFF111111);
					}
					Gui::End();

					Gui::PopStyleColor(3);
				}
			}

			Gui::EndMenuBar();
		}
	}

	void ChartEditor::DrawGui()
	{
		InternalUpdateAsyncLoading();

		if (tryToCloseApplicationOnNextFrame)
		{
			tryToCloseApplicationOnNextFrame = false;
			CheckOpenSaveConfirmationPopupThenCall([&]
			{
				if (loadSongFuture.valid()) loadSongFuture.get();
				if (importChartFuture.valid()) importChartFuture.get();
				context.Undo.ClearAll();
				ApplicationHost::GlobalState.RequestExitNextFrame = EXIT_SUCCESS;
			});
		}

		UpdateApplicationWindowTitle(context);

		// NOTE: Check for external changes made via the settings window
		if (GlobalLastSetRequestExclusiveDeviceAccessAudioSetting != *Settings.Audio.RequestExclusiveDeviceAccess)
		{
			Audio::Engine.SetBackend(*Settings.Audio.RequestExclusiveDeviceAccess ? Audio::Backend::WASAPI_Exclusive : Audio::Backend::WASAPI_Shared);
			GlobalLastSetRequestExclusiveDeviceAccessAudioSetting = *Settings.Audio.RequestExclusiveDeviceAccess;
		}
		EnableGuiScaleAnimation = *Settings.Animation.EnableGuiScaleAnimation;

		// NOTE: Window focus audio engine response
		{
			if (ApplicationHost::GlobalState.HasAllFocusBeenLostThisFrame)
			{
				wasAudioEngineRunningIdleOnFocusLost = (Audio::Engine.GetIsStreamOpenRunning() && Audio::Engine.GetAllVoicesAreIdle());
				if (wasAudioEngineRunningIdleOnFocusLost && *Settings.Audio.CloseDeviceOnIdleFocusLoss)
					Audio::Engine.StopCloseStream();
			}
			if (ApplicationHost::GlobalState.HasAnyFocusBeenGainedThisFrame)
			{
				if (wasAudioEngineRunningIdleOnFocusLost && *Settings.Audio.CloseDeviceOnIdleFocusLoss)
					Audio::Engine.OpenStartStream();
			}
		}

		// NOTE: Apply volume
		{
			context.SongVoice.SetVolume(context.Chart.SongVolume);
			context.SfxVoicePool.BaseVolumeSfx = context.Chart.SoundEffectVolume;
		}

		// NOTE: Drag and drop handling
		for (const std::string& droppedFilePath : ApplicationHost::GlobalState.FilePathsDroppedThisFrame)
		{
			if (Path::HasAnyExtension(droppedFilePath, TJA::Extension)) { CheckOpenSaveConfirmationPopupThenCall([this, pathCopy = droppedFilePath] { StartAsyncImportingChartFile(pathCopy); }); break; }
			if (Path::HasAnyExtension(droppedFilePath, Audio::SupportedFileFormatExtensionsPacked)) { SetAndStartLoadingChartSongFileName(droppedFilePath, context.Undo); break; }
		}

		// NOTE: Global input bindings
		{
			const b8 noActiveID = (Gui::GetActiveID() == 0);
			const b8 noOpenPopup = (Gui::GetCurrentContext()->OpenPopupStack.Size <= 0);

			if (noActiveID)
			{
				const f32 guiScaleFactorToSetNextFrame = GuiScaleFactorToSetNextFrame;
				if (Gui::IsAnyPressed(*Settings.Input.Editor_IncreaseGuiScale, true)) ZoomInGuiScale();
				if (Gui::IsAnyPressed(*Settings.Input.Editor_DecreaseGuiScale, true)) ZoomOutGuiScale();
				if (Gui::IsAnyPressed(*Settings.Input.Editor_ResetGuiScale, false)) ZoomResetGuiScale();

				if (guiScaleFactorToSetNextFrame != GuiScaleFactorToSetNextFrame) { if (!zoomPopup.IsOpen) zoomPopup.Open(); zoomPopup.OnChange(); }

				if (Gui::IsAnyPressed(*Settings.Input.Editor_ToggleFullscreen, false))
					ApplicationHost::GlobalState.SetBorderlessFullscreenNextFrame = !ApplicationHost::GlobalState.IsBorderlessFullscreen;

				if (Gui::IsAnyPressed(*Settings.Input.Editor_ToggleVSync, false))
					ApplicationHost::GlobalState.SwapInterval = !ApplicationHost::GlobalState.SwapInterval;

				if (Gui::IsAnyPressed(*Settings.Input.Editor_OpenHelp, true)) PersistentApp.LastSession.ShowWindow_Help = focusHelpWindowNextFrame = true;
				if (Gui::IsAnyPressed(*Settings.Input.Editor_OpenSettings, true)) PersistentApp.LastSession.ShowWindow_Settings = focusSettingsWindowNextFrame = true;
			}

			if (noActiveID && noOpenPopup)
			{
				if (Gui::IsAnyPressed(*Settings.Input.Editor_Undo, true))
					context.Undo.Undo();
				if (Gui::IsAnyPressed(*Settings.Input.Editor_Redo, true))
					context.Undo.Redo();

				if (Gui::IsAnyPressed(*Settings.Input.Editor_ChartNew, false))
					CheckOpenSaveConfirmationPopupThenCall([&] { CreateNewChart(context); });

				if (Gui::IsAnyPressed(*Settings.Input.Editor_ChartOpen, false))
					CheckOpenSaveConfirmationPopupThenCall([&] { OpenLoadChartFileDialog(context); });

				if (Gui::IsAnyPressed(*Settings.Input.Editor_ChartOpenDirectory, false) && CanOpenChartDirectoryInFileExplorer(context))
					OpenChartDirectoryInFileExplorer(context);

				if (Gui::IsAnyPressed(*Settings.Input.Editor_ChartSave, false))
					TrySaveChartOrOpenSaveAsDialog(context);

				if (Gui::IsAnyPressed(*Settings.Input.Editor_ChartSaveAs, false))
					OpenChartSaveAsDialog(context);
			}
		}

		if (PersistentApp.LastSession.ShowWindow_Help)
		{
			if (Gui::Begin(UI_WindowName("Usage Guide"), &PersistentApp.LastSession.ShowWindow_Help, ImGuiWindowFlags_None))
			{
				helpWindow.DrawGui(context);
			}
			if (focusHelpWindowNextFrame) { focusHelpWindowNextFrame = false; Gui::SetWindowFocus(); }
			Gui::End();
		}

		if (PersistentApp.LastSession.ShowWindow_Settings)
		{
			if (Gui::Begin(UI_WindowName("Settings"), &PersistentApp.LastSession.ShowWindow_Settings, ImGuiWindowFlags_None))
			{
				if (settingsWindow.DrawGui(context, Settings_Mutable))
					Settings_Mutable.IsDirty = true;
			}
			if (focusSettingsWindowNextFrame) { focusSettingsWindowNextFrame = false; Gui::SetWindowFocus(); }
			Gui::End();
		}

		if (Gui::Begin(UI_WindowName("Chart Inspector"), nullptr, ImGuiWindowFlags_None))
		{
			chartInspectorWindow.DrawGui(context);
		}
		Gui::End();

		if (Gui::Begin(UI_WindowName("Undo History"), nullptr, ImGuiWindowFlags_None))
		{
			undoHistoryWindow.DrawGui(context);
		}
		Gui::End();

		if (Gui::Begin(UI_WindowName("Tempo Calculator"), nullptr, ImGuiWindowFlags_None))
		{
			tempoCalculatorWindow.DrawGui(context);
		}
		Gui::End();

		if (Gui::Begin(UI_WindowName("Chart Lyrics"), nullptr, ImGuiWindowFlags_None))
		{
			lyricsWindow.DrawGui(context, timeline);
		}
		Gui::End();

		if (Gui::Begin(UI_WindowName("Chart Tempo"), nullptr, ImGuiWindowFlags_None))
		{
			tempoWindow.DrawGui(context, timeline);
		}
		Gui::End();

		if (Gui::Begin(UI_WindowName("Chart Properties"), nullptr, ImGuiWindowFlags_None))
		{
			ChartPropertiesWindowIn in = {};
			in.IsSongAsyncLoading = loadSongFuture.valid();
			ChartPropertiesWindowOut out = {};
			propertiesWindow.DrawGui(context, in, out);

			if (out.LoadNewSong)
				SetAndStartLoadingChartSongFileName(out.NewSongFilePath, context.Undo);
			else if (out.BrowseOpenSong)
				OpenLoadAudioFileDialog(context.Undo);
		}
		Gui::End();

#if PEEPO_DEBUG // DEBUG: Manually submit debug window before the timeline window is drawn for better tab ordering
		if (Gui::Begin(UI_WindowName("Chart Timeline - Debug"))) { /* ... */ } Gui::End();
#endif

		if (Gui::Begin(UI_WindowName("Game Preview"), nullptr, ImGuiWindowFlags_None))
		{
			gamePreview.DrawGui(context, timeline.Camera.WorldSpaceXToTime(timeline.WorldSpaceCursorXAnimationCurrent));
		}
		Gui::End();

		// NOTE: Always update the timeline even if the window isn't visible so that child-windows can be docked properly and hit sounds can always be heard
		Gui::Begin(UI_WindowName("Chart Timeline"), nullptr, ImGuiWindowFlags_None);
		timeline.DrawGui(context);
		Gui::End();

		// NOTE: Test stuff
		{
			if (PersistentApp.LastSession.ShowWindow_ImGuiDemo)
			{
				if (auto* window = Gui::FindWindowByName("Dear ImGui Demo"))
					Gui::UpdateSmoothScrollWindow(window);

				Gui::ShowDemoWindow(&PersistentApp.LastSession.ShowWindow_ImGuiDemo);
			}

			if (PersistentApp.LastSession.ShowWindow_ImGuiStyleEditor)
			{
				if (Gui::Begin("ImGui Style Editor", &PersistentApp.LastSession.ShowWindow_ImGuiStyleEditor))
				{
					Gui::UpdateSmoothScrollWindow();
					Gui::ShowStyleEditor();
				}
				Gui::End();
			}

			if (PersistentApp.LastSession.ShowWindow_TJAImportTest)
			{
				if (Gui::Begin(UI_WindowName("TJA Import Test"), &PersistentApp.LastSession.ShowWindow_TJAImportTest, ImGuiWindowFlags_None))
				{
					tjaTestWindow.DrawGui();
					if (tjaTestWindow.WasTJAEditedThisFrame)
					{
						CheckOpenSaveConfirmationPopupThenCall([&]
						{
							// HACK: Incredibly inefficient of course but just here for quick testing
							ChartProject convertedChart {};
							CreateChartProjectFromTJA(tjaTestWindow.LoadedTJAFile.Parsed, convertedChart);
							createBackupOfOriginalTJABeforeOverwriteSave = false;
							context.Chart = std::move(convertedChart);
							context.ChartFilePath = tjaTestWindow.LoadedTJAFile.FilePath;
							context.ChartSelectedCourse = context.Chart.Courses.empty() ? context.Chart.Courses.emplace_back(std::make_unique<ChartCourse>()).get() : context.Chart.Courses.front().get();
							context.Undo.ClearAll();
						});
					}
				}
				Gui::End();
			}

			if (PersistentApp.LastSession.ShowWindow_AudioTest)
			{
				if (Gui::Begin(UI_WindowName("Audio Test"), &PersistentApp.LastSession.ShowWindow_AudioTest, ImGuiWindowFlags_None))
				{
					audioTestWindow.DrawGui();
					Gui::End();
				}
			}

			// DEBUG: LIVE PREVIEW PagMan
			if (PersistentApp.LastSession.ShowWindow_TJAExportTest)
			{
				if (Gui::Begin(UI_WindowName("TJA Export Debug View"), &PersistentApp.LastSession.ShowWindow_TJAExportTest))
				{
					static struct { b8 Update = true; i32 Changes = -1, Undos = 0, Redos = 0;  std::string Text; ::TextEditor Editor = CreateImGuiColorTextEditWithNiceTheme(); } exportDebugViewData;
					if (context.Undo.NumberOfChangesMade != exportDebugViewData.Changes) { exportDebugViewData.Update = true; exportDebugViewData.Changes = context.Undo.NumberOfChangesMade; }
					if (context.Undo.UndoStack.size() != exportDebugViewData.Undos) { exportDebugViewData.Update = true; exportDebugViewData.Undos = static_cast<i32>(context.Undo.UndoStack.size()); }
					if (context.Undo.RedoStack.size() != exportDebugViewData.Redos) { exportDebugViewData.Update = true; exportDebugViewData.Redos = static_cast<i32>(context.Undo.RedoStack.size()); }
					if (exportDebugViewData.Update)
					{
						exportDebugViewData.Update = false;
						TJA::ParsedTJA tja;
						ConvertChartProjectToTJA(context.Chart, tja);
						exportDebugViewData.Text.clear();
						TJA::ConvertParsedToText(tja, exportDebugViewData.Text, TJA::Encoding::Unknown);
						exportDebugViewData.Editor.SetText(exportDebugViewData.Text);
					}
					const f32 buttonHeight = Gui::GetFrameHeight();
					exportDebugViewData.Editor.SetReadOnly(true);
					exportDebugViewData.Editor.Render("TJAExportTextEditor", vec2(Gui::GetContentRegionAvail().x, ClampBot(buttonHeight * 2.0f, Gui::GetContentRegionAvail().y - buttonHeight)), true);
					exportDebugViewData.Update |= Gui::Button("Force Update", vec2(Gui::GetContentRegionAvail().x, buttonHeight));
				}
				Gui::End();
			}
		}

		// NOTE: Zoom change popup
		if (zoomPopup.IsOpen)
		{
			static constexpr Time fadeInDuration = Time::FromFrames(10.0), fadeOutDuration = Time::FromFrames(14.0);
			static constexpr Time closeDuration = Time::FromSec(2.0);
			const f32 fadeIn = ConvertRangeClampInput(0.0f, fadeInDuration.ToSec_F32(), 0.0f, 1.0f, zoomPopup.TimeSinceOpen.ToSec_F32());
			const f32 fadeOut = ConvertRangeClampInput(0.0f, fadeOutDuration.ToSec_F32(), 0.0f, 1.0f, (closeDuration - zoomPopup.TimeSinceLastChange).ToSec_F32());

			b8 isWindowHovered = false;
			Gui::PushStyleVar(ImGuiStyleVar_Alpha, (fadeIn < 1.0f) ? (fadeIn * fadeIn) : (fadeOut * fadeOut));
			Gui::PushStyleVar(ImGuiStyleVar_WindowRounding, GuiScale(6.0f));
			Gui::PushStyleColor(ImGuiCol_WindowBg, Gui::ColorU32WithNewAlpha(Gui::GetColorU32(ImGuiCol_FrameBg), 1.0f));
			Gui::PushStyleColor(ImGuiCol_Button, 0x00000000);
			Gui::PushFont(FontMedium_EN);

			constexpr ImGuiWindowFlags popupFlags =
				ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing |
				ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings;
			const ImGuiViewport* mainViewport = Gui::GetMainViewport();
			Gui::SetNextWindowPos(vec2(mainViewport->Pos.x + mainViewport->Size.x, mainViewport->Pos.y + GuiScale(24.0f)) + vec2(Gui::GetStyle().FramePadding) * vec2(-1.0f, +1.0f), ImGuiCond_Always, vec2(1.0f, 0.0f));
			Gui::Begin("ZoomPopup", nullptr, popupFlags);
			{
				isWindowHovered = Gui::IsWindowHovered();
				Gui::AlignTextToFramePadding();
				Gui::Text("%g%% ", ToPercent(GuiScaleFactorTarget));

				Gui::BeginDisabled(!CanZoomOutGuiScale());
				Gui::SameLine(0.0f, 0.0f);
				if (Gui::Button("-", vec2(Gui::GetFrameHeight(), 0.0f))) { ZoomOutGuiScale(); zoomPopup.OnChange(); }
				Gui::EndDisabled();

				Gui::BeginDisabled(!CanZoomInGuiScale());
				Gui::SameLine(0.0f, 0.0f);
				if (Gui::Button("+", vec2(Gui::GetFrameHeight(), 0.0f))) { ZoomInGuiScale(); zoomPopup.OnChange(); }
				Gui::EndDisabled();

				Gui::SameLine(0.0f, 0.0f);
				if (Gui::Button(UI_Str(" Reset "))) { ZoomResetGuiScale(); zoomPopup.OnChange(); }
			}
			Gui::End();

			Gui::PopFont();
			Gui::PopStyleColor(2);
			Gui::PopStyleVar(2);

			if (zoomPopup.TimeSinceLastChange >= closeDuration)
			{
				zoomPopup.IsOpen = false;
				zoomPopup.TimeSinceOpen = zoomPopup.TimeSinceLastChange = {};
			}
			else
			{
				// NOTE: Clamp so that the fade-in animation won't ever be fully skipped even with font rebuild lag
				zoomPopup.TimeSinceOpen += Time::FromSec(ClampTop(Gui::GetIO().DeltaTime, 1.0f / 30.0f));
				zoomPopup.TimeSinceLastChange = isWindowHovered ? (closeDuration * 0.5) : zoomPopup.TimeSinceLastChange + Time::FromSec(Gui::GetIO().DeltaTime);
			}
		}

		// NOTE: Save confirmation popup
		{
			static constexpr cstr saveConfirmationPopupID = "Peepo Drum Kit - Unsaved Changes";
			if (saveConfirmationPopup.OpenOnNextFrame) { Gui::OpenPopup(UI_WindowName(saveConfirmationPopupID)); saveConfirmationPopup.OpenOnNextFrame = false; }

			const ImGuiViewport* mainViewport = Gui::GetMainViewport();
			Gui::SetNextWindowPos(Rect::FromTLSize(mainViewport->Pos, mainViewport->Size).GetCenter(), ImGuiCond_Appearing, vec2(0.5f));

			Gui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { GuiScale(4.0f), Gui::GetStyle().ItemSpacing.y });
			Gui::PushStyleVar(ImGuiStyleVar_WindowPadding, { GuiScale(6.0f), GuiScale(6.0f) });
			b8 isPopupOpen = true;
			if (Gui::BeginPopupModal(UI_WindowName(saveConfirmationPopupID), &isPopupOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
			{
				const vec2 buttonSize = GuiScale(vec2(120.0f, 0.0f));
				Gui::PushFont(FontMedium_EN);
				{
					// NOTE: Manual child size calculation required for proper dynamic scaling
					Gui::BeginChild("TextChild", vec2((buttonSize.x * 3.0f) + Gui::GetStyle().ItemSpacing.x, Gui::GetFontSize() * 3.0f), true, ImGuiWindowFlags_NoBackground);
					Gui::AlignTextToFramePadding();
					Gui::TextUnformatted(UI_Str("Save changes to the current file?"));
					Gui::EndChild();
				}
				Gui::PopFont();

				const b8 clickedYes = Gui::Button(UI_Str("Save Changes"), buttonSize) | (Gui::IsWindowFocused() && Gui::IsAnyPressed(*Settings.Input.Dialog_YesOrOk, false));
				Gui::SameLine();
				const b8 clickedNo = Gui::Button(UI_Str("Discard Changes"), buttonSize) | (Gui::IsWindowFocused() && Gui::IsAnyPressed(*Settings.Input.Dialog_No, false));
				Gui::SameLine();
				const b8 clickedCancel = Gui::Button(UI_Str("Cancel"), buttonSize) | (Gui::IsWindowFocused() && Gui::IsAnyPressed(*Settings.Input.Dialog_Cancel, false));

				if (clickedYes || clickedNo || clickedCancel)
				{
					const b8 saveDialogCanceled = clickedYes ? !TrySaveChartOrOpenSaveAsDialog(context) : false;
					UpdateApplicationWindowTitle(context);
					if (saveConfirmationPopup.OnSuccessFunction)
					{
						if (!clickedCancel && !saveDialogCanceled)
							saveConfirmationPopup.OnSuccessFunction();
						saveConfirmationPopup.OnSuccessFunction = {};
					}
					Gui::CloseCurrentPopup();
				}
				Gui::EndPopup();
			}
			Gui::PopStyleVar(2);
		}

		context.Undo.FlushAndExecuteEndOfFrameCommands();
	}

	void ChartEditor::RestoreDefaultDockSpaceLayout(ImGuiID dockSpaceID)
	{
		const ImGuiViewport* mainViewport = Gui::GetMainViewport();

		// HACK: This kinda sucks. Referencing windows by hardcoded ID strings and also not allow for incremental window additions without rebuilding the entire node tree...
		//		 This function can also only be called right before the fullscreen "Gui::DockSpace()" (meaining outside the chart editor update functions)
		Gui::DockBuilderRemoveNode(dockSpaceID);
		Gui::DockBuilderAddNode(dockSpaceID, ImGuiDockNodeFlags_DockSpace);
		Gui::DockBuilderSetNodeSize(dockSpaceID, mainViewport->WorkSize);
		{
			struct DockIDs { ImGuiID Top, Bot, TopLeft, TopCenter, TopRight, TopRightBot; } dock = {};
			Gui::DockBuilderSplitNode(dockSpaceID, ImGuiDir_Down, 0.55f, &dock.Bot, &dock.Top);
			Gui::DockBuilderSplitNode(dock.Top, ImGuiDir_Left, 0.3f, &dock.TopLeft, &dock.TopRight);
			Gui::DockBuilderSplitNode(dock.TopRight, ImGuiDir_Right, 0.4f, &dock.TopRight, &dock.TopCenter);
			Gui::DockBuilderSplitNode(dock.TopRight, ImGuiDir_Down, 0.4f, &dock.TopRightBot, &dock.TopRight);

			// HACK: Shitty dock tab order. Windows which are "Gui::Begin()"-added last (ones with higher focus priority) are always put on the right
			//		 and it doesn't look like there's an easy way to change this...
			Gui::DockBuilderDockWindow(UI_WindowName("Chart Timeline - Debug"), dock.Bot);
			Gui::DockBuilderDockWindow(UI_WindowName("Chart Timeline"), dock.Bot);

			Gui::DockBuilderDockWindow(UI_WindowName("Tempo Calculator"), dock.TopLeft);
			Gui::DockBuilderDockWindow(UI_WindowName("Chart Lyrics"), dock.TopLeft);
			Gui::DockBuilderDockWindow(UI_WindowName("Chart Tempo"), dock.TopLeft);
			Gui::DockBuilderDockWindow(UI_WindowName("TJA Export Debug View"), dock.TopLeft);

			Gui::DockBuilderDockWindow(UI_WindowName("Settings"), dock.TopCenter);
			Gui::DockBuilderDockWindow(UI_WindowName("Usage Guide"), dock.TopCenter);
			Gui::DockBuilderDockWindow(UI_WindowName("Game Preview"), dock.TopCenter);
			Gui::DockBuilderDockWindow(UI_WindowName("Audio Test"), dock.TopCenter);
			Gui::DockBuilderDockWindow(UI_WindowName("TJA Import Test"), dock.TopCenter);
			Gui::DockBuilderDockWindow("Dear ImGui Demo", dock.TopCenter);
			Gui::DockBuilderDockWindow("ImGui Style Editor", dock.TopCenter);

			Gui::DockBuilderDockWindow(UI_WindowName("Undo History"), dock.TopRight);
			Gui::DockBuilderDockWindow(UI_WindowName("Chart Properties"), dock.TopRight);

			Gui::DockBuilderDockWindow(UI_WindowName("Chart Inspector"), dock.TopRightBot);
		}
		Gui::DockBuilderFinish(dockSpaceID);
	}

	ApplicationHost::CloseResponse ChartEditor::OnWindowCloseRequest()
	{
		if (context.Undo.HasPendingChanges)
		{
			tryToCloseApplicationOnNextFrame = true;
			return ApplicationHost::CloseResponse::SupressExit;
		}
		else
		{
			return ApplicationHost::CloseResponse::Exit;
		}
	}

	void ChartEditor::UpdateApplicationWindowTitle(const ChartContext& context)
	{
		ApplicationHost::GlobalState.SetWindowTitleNextFrame = PeepoDrumKitApplicationTitle;
		ApplicationHost::GlobalState.SetWindowTitleNextFrame += " - ";
		if (!context.ChartFilePath.empty())
			ApplicationHost::GlobalState.SetWindowTitleNextFrame += Path::GetFileName(context.ChartFilePath);
		else
			ApplicationHost::GlobalState.SetWindowTitleNextFrame += UntitledChartFileName;

		if (context.Undo.HasPendingChanges)
			ApplicationHost::GlobalState.SetWindowTitleNextFrame += "*";
	}

	void ChartEditor::CreateNewChart(ChartContext& context)
	{
		if (loadSongFuture.valid()) loadSongFuture.get();
		if (!context.SongSourceFilePath.empty()) StartAsyncLoadingSongAudioFile("");
		if (importChartFuture.valid()) importChartFuture.get();
		InternalUpdateAsyncLoading();

		createBackupOfOriginalTJABeforeOverwriteSave = false;
		context.Chart = {};
		context.ChartFilePath.clear();
		context.ChartSelectedCourse = context.Chart.Courses.empty() ? context.Chart.Courses.emplace_back(std::make_unique<ChartCourse>()).get() : context.Chart.Courses.front().get();
		context.ChartSelectedBranch = BranchType::Normal;
		SetChartDefaultSettingsAndCourses(context.Chart);

		context.SetIsPlayback(false);
		context.SetCursorBeat(Beat::Zero());
		context.Undo.ClearAll();

		timeline.Camera.PositionTarget.x = TimelineCameraBaseScrollX;
		timeline.Camera.ZoomTarget = vec2(1.0f);
	}

	void ChartEditor::SaveChart(ChartContext& context, std::string_view filePath)
	{
		if (filePath.empty())
			filePath = context.ChartFilePath;

		assert(!filePath.empty());
		if (!filePath.empty())
		{
			TJA::ParsedTJA tja;
			ConvertChartProjectToTJA(context.Chart, tja);
			std::string tjaText;
			TJA::ConvertParsedToText(tja, tjaText, TJA::Encoding::UTF8);

			// TODO: Proper async file saving by copying in-memory
			if (createBackupOfOriginalTJABeforeOverwriteSave)
			{
				static constexpr b8 overwriteExisting = false;
				const std::string originalFileBackupPath { std::string(filePath).append(".bak") };

				File::Copy(filePath, originalFileBackupPath, overwriteExisting);
				createBackupOfOriginalTJABeforeOverwriteSave = false;
			}

			File::WriteAllBytes(filePath, tjaText);

			context.ChartFilePath = filePath;
			context.Undo.ClearChangesWereMade();

			PersistentApp.RecentFiles.Add(std::string { filePath });
		}
	}

	b8 ChartEditor::OpenChartSaveAsDialog(ChartContext& context)
	{
		static constexpr auto getChartFileNameWithoutExtensionOrDefault = [](const ChartContext& context) -> std::string_view
		{
			if (!context.ChartFilePath.empty()) return Path::GetFileName(context.ChartFilePath, false);
			if (!context.Chart.ChartTitle.Base().empty()) return context.Chart.ChartTitle.Base();
			return Path::TrimExtension(UntitledChartFileName);
		};

		Shell::FileDialog fileDialog {};
		fileDialog.InTitle = "Save Chart As";
		fileDialog.InFileName = getChartFileNameWithoutExtensionOrDefault(context);
		fileDialog.InDefaultExtension = TJA::Extension;
		fileDialog.InFilters = { { TJA::FilterName, TJA::FilterSpec }, { Shell::AllFilesFilterName, Shell::AllFilesFilterSpec }, };
		fileDialog.InParentWindowHandle = ApplicationHost::GlobalState.NativeWindowHandle;

		if (fileDialog.OpenSave() != Shell::FileDialogResult::OK)
			return false;

		SaveChart(context, fileDialog.OutFilePath);
		return true;
	}

	b8 ChartEditor::TrySaveChartOrOpenSaveAsDialog(ChartContext& context)
	{
		if (context.ChartFilePath.empty())
			return OpenChartSaveAsDialog(context);
		else
			SaveChart(context);
		return true;
	}

	void ChartEditor::StartAsyncImportingChartFile(std::string_view absoluteChartFilePath)
	{
		if (importChartFuture.valid())
			importChartFuture.get();

		PersistentApp.RecentFiles.Add(std::string { absoluteChartFilePath });
		importChartFuture = std::async(std::launch::async, [tempPathCopy = std::string(absoluteChartFilePath)]() mutable->AsyncImportChartResult
		{
			AsyncImportChartResult result {};
			result.ChartFilePath = std::move(tempPathCopy);

			auto[fileContent, fileSize] = File::ReadAllBytes(result.ChartFilePath);
			if (fileContent == nullptr || fileSize == 0)
			{
				printf("Failed to read file '%.*s'\n", FmtStrViewArgs(result.ChartFilePath));
				return result;
			}

			assert(Path::HasExtension(result.ChartFilePath, TJA::Extension));

			const std::string_view fileContentView = std::string_view(reinterpret_cast<const char*>(fileContent.get()), fileSize);
			if (UTF8::HasBOM(fileContentView))
				result.TJA.FileContentUTF8 = UTF8::TrimBOM(fileContentView);
			else
				result.TJA.FileContentUTF8 = UTF8::FromShiftJIS(fileContentView);

			result.TJA.Lines = TJA::SplitLines(result.TJA.FileContentUTF8);
			result.TJA.Tokens = TJA::TokenizeLines(result.TJA.Lines);
			result.TJA.Parsed = ParseTokens(result.TJA.Tokens, result.TJA.ParseErrors);

			if (!CreateChartProjectFromTJA(result.TJA.Parsed, result.Chart))
			{
				printf("Failed to create chart from TJA file '%.*s'\n", FmtStrViewArgs(result.ChartFilePath));
				return result;
			}

			return result;
		});
	}

	void ChartEditor::StartAsyncLoadingSongAudioFile(std::string_view absoluteAudioFilePath)
	{
		if (loadSongFuture.valid())
			loadSongFuture.get();

		context.SongWaveformFadeAnimationTarget = 0.0f;
		loadSongStopwatch.Restart();
		loadSongFuture = std::async(std::launch::async, [tempPathCopy = std::string(absoluteAudioFilePath)]()->AsyncLoadSongResult
		{
			AsyncLoadSongResult result {};
			result.SongFilePath = std::move(tempPathCopy);

			// TODO: Maybe handle this in a different way... but for now loading an empty file path works as an "unload"
			if (result.SongFilePath.empty())
				return result;

			auto[fileContent, fileSize] = File::ReadAllBytes(result.SongFilePath);
			if (fileContent == nullptr || fileSize == 0)
			{
				printf("Failed to read file '%.*s'\n", FmtStrViewArgs(result.SongFilePath));
				return result;
			}

			if (Audio::DecodeEntireFile(result.SongFilePath, fileContent.get(), fileSize, result.SampleBuffer) != Audio::DecodeFileResult::FeelsGoodMan)
			{
				printf("Failed to decode audio file '%.*s'\n", FmtStrViewArgs(result.SongFilePath));
				return result;
			}

			// HACK: ...
			if (result.SampleBuffer.SampleRate != Audio::Engine.OutputSampleRate)
				Audio::LinearlyResampleBuffer<i16>(result.SampleBuffer.InterleavedSamples, result.SampleBuffer.FrameCount, result.SampleBuffer.SampleRate, result.SampleBuffer.ChannelCount, Audio::Engine.OutputSampleRate);

			if (result.SampleBuffer.ChannelCount > 0) result.WaveformL.GenerateEntireMipChainFromSampleBuffer(result.SampleBuffer, 0);
#if !PEEPO_DEBUG // NOTE: Always ignore the second channel in debug builds for performance reasons!
			if (result.SampleBuffer.ChannelCount > 1) result.WaveformR.GenerateEntireMipChainFromSampleBuffer(result.SampleBuffer, 1);
#endif

			return result;
		});
	}

	void ChartEditor::SetAndStartLoadingChartSongFileName(std::string_view relativeOrAbsoluteAudioFilePath, Undo::UndoHistory& undo)
	{
		if (!relativeOrAbsoluteAudioFilePath.empty() && !Path::IsRelative(relativeOrAbsoluteAudioFilePath))
		{
			context.Chart.SongFileName = Path::TryMakeRelative(relativeOrAbsoluteAudioFilePath, context.ChartFilePath);
			if (context.Chart.SongFileName.empty())
				context.Chart.SongFileName = relativeOrAbsoluteAudioFilePath;
		}
		else
		{
			context.Chart.SongFileName = relativeOrAbsoluteAudioFilePath;
		}

		Path::NormalizeInPlace(context.Chart.SongFileName);
		undo.NotifyChangesWereMade();

		// NOTE: Construct a new absolute path even if the input path was already absolute
		//		 so that there won't ever be any discrepancy in case the relative path code is behaving unexpectedly
		StartAsyncLoadingSongAudioFile(Path::TryMakeAbsolute(context.Chart.SongFileName, context.ChartFilePath));
	}

	b8 ChartEditor::OpenLoadChartFileDialog(ChartContext& context)
	{
		Shell::FileDialog fileDialog {};
		fileDialog.InTitle = "Open Chart File";
		fileDialog.InFilters = { { TJA::FilterName, TJA::FilterSpec }, { Shell::AllFilesFilterName, Shell::AllFilesFilterSpec }, };
		fileDialog.InParentWindowHandle = ApplicationHost::GlobalState.NativeWindowHandle;

		if (fileDialog.OpenRead() != Shell::FileDialogResult::OK)
			return false;

		StartAsyncImportingChartFile(fileDialog.OutFilePath);
		return true;
	}

	b8 ChartEditor::OpenLoadAudioFileDialog(Undo::UndoHistory& undo)
	{
		Shell::FileDialog fileDialog {};
		fileDialog.InTitle = "Open Audio File";
		fileDialog.InFilters = { { "Audio Files", "*.flac;*.ogg;*.mp3;*.wav" }, { Shell::AllFilesFilterName, Shell::AllFilesFilterSpec }, };
		fileDialog.InParentWindowHandle = ApplicationHost::GlobalState.NativeWindowHandle;

		if (fileDialog.OpenRead() != Shell::FileDialogResult::OK)
			return false;

		SetAndStartLoadingChartSongFileName(fileDialog.OutFilePath, undo);
		return true;
	}

	void ChartEditor::CheckOpenSaveConfirmationPopupThenCall(std::function<void()> onSuccess)
	{
		if (context.Undo.HasPendingChanges)
		{
			saveConfirmationPopup.OpenOnNextFrame = true;
			saveConfirmationPopup.OnSuccessFunction = std::move(onSuccess);
		}
		else
		{
			onSuccess();
		}
	}

	void ChartEditor::InternalUpdateAsyncLoading()
	{
		context.SfxVoicePool.UpdateAsyncLoading();

		if (importChartFuture.valid() && importChartFuture._Is_ready())
		{
			const Time previousChartSongOffset = context.Chart.SongOffset;

			AsyncImportChartResult loadResult = importChartFuture.get();

			// TODO: Maybe also do date version check (?)
			createBackupOfOriginalTJABeforeOverwriteSave = !loadResult.TJA.Parsed.HasPeepoDrumKitComment;

			context.Chart = std::move(loadResult.Chart);
			context.ChartFilePath = std::move(loadResult.ChartFilePath);
			context.ChartSelectedCourse = context.Chart.Courses.empty() ? context.Chart.Courses.emplace_back(std::make_unique<ChartCourse>()).get() : context.Chart.Courses.front().get();
			StartAsyncLoadingSongAudioFile(Path::TryMakeAbsolute(context.Chart.SongFileName, context.ChartFilePath));

			// NOTE: Prevent the cursor from changing screen position. Not needed if paused because a stable beat time is used instead
			if (context.GetIsPlayback())
				context.SetCursorTime(context.GetCursorTime() + (previousChartSongOffset - context.Chart.SongOffset));

			context.Undo.ClearAll();
		}

		// NOTE: Just in case there is something wrong with the animation, that could otherwise prevent the song from finishing to load
		static constexpr Time maxWaveformFadeOutDelaySafetyLimit = Time::FromSec(0.5);
		const b8 waveformHasFadedOut = (context.SongWaveformFadeAnimationCurrent <= 0.01f || loadSongStopwatch.GetElapsed() >= maxWaveformFadeOutDelaySafetyLimit);

		if (loadSongFuture.valid() && loadSongFuture._Is_ready() && waveformHasFadedOut)
		{
			loadSongStopwatch.Stop();
			AsyncLoadSongResult loadResult = loadSongFuture.get();
			context.SongSourceFilePath = std::move(loadResult.SongFilePath);
			context.SongWaveformL = std::move(loadResult.WaveformL);
			context.SongWaveformR = std::move(loadResult.WaveformR);
			context.SongWaveformFadeAnimationTarget = context.SongWaveformL.IsEmpty() ? 0.0f : 1.0f;

			// TODO: Maybe handle this differently...
			if (context.Chart.ChartTitle.Base().empty() && !context.SongSourceFilePath.empty())
				context.Chart.ChartTitle.Base() = Path::GetFileName(context.SongSourceFilePath, false);

			if (context.Chart.ChartDuration.Seconds <= 0.0 && loadResult.SampleBuffer.SampleRate > 0)
				context.Chart.ChartDuration = Audio::FramesToTime(loadResult.SampleBuffer.FrameCount, loadResult.SampleBuffer.SampleRate);

			if (context.SongSource != Audio::SourceHandle::Invalid)
				Audio::Engine.UnloadSource(context.SongSource);

			context.SongSource = Audio::Engine.LoadSourceFromBufferMove(Path::GetFileName(context.SongSourceFilePath), std::move(loadResult.SampleBuffer));
			context.SongVoice.SetSource(context.SongSource);

			Audio::Engine.EnsureStreamRunning();
		}
	}
}
