#include "chart_editor.h"
#include "core_build_info.h"
#include "chart_undo_commands.h"
#include "audio/audio_file_formats.h"

namespace PeepoDrumKit
{
	static constexpr std::string_view UntitledChartFileName = "Untitled Chart.tja";
	static constexpr f32 ScaleFactorIncrementStep = FromPercent(10.0f); // 20.0f;

	static bool CanOpenChartDirectoryInFileExplorer(const ChartContext& context)
	{
		return !context.ChartFilePath.empty();
	}

	static void OpenChartDirectoryInFileExplorer(const ChartContext& context)
	{
		const std::string_view chartDirectory = Path::GetDirectoryName(context.ChartFilePath);
		if (!chartDirectory.empty() && Directory::Exists(chartDirectory))
			Shell::OpenInExplorer(chartDirectory);
	}

	ChartEditor::ChartEditor()
	{
		context.SongVoice = Audio::Engine.AddVoice(Audio::SourceHandle::Invalid, "ChartEditor SongVoice", false, 1.0f, true);
		context.SfxVoicePool.StartAsyncLoadingAndAddVoices();
		context.ChartSelectedCourse = context.Chart.Courses.emplace_back(std::make_unique<ChartCourse>()).get();
		showHelpWindow = true;

		Audio::Engine.SetMasterVolume(0.75f);
		Audio::Engine.EnsureStreamRunning();
	}

	ChartEditor::~ChartEditor()
	{
		context.SfxVoicePool.UnloadAllSourcesAndVoices();
	}

	void ChartEditor::DrawFullscreenMenuBar()
	{
		if (Gui::BeginMenuBar())
		{
			if (Gui::BeginMenu("File"))
			{
				if (Gui::MenuItem("New Chart", ToShortcutString(*Settings.Input.Editor_ChartNew).Data)) { CheckOpenSaveConfirmationPopupThenCall([&] { CreateNewChart(context); }); }
				if (Gui::MenuItem("Open...", ToShortcutString(*Settings.Input.Editor_ChartOpen).Data)) { CheckOpenSaveConfirmationPopupThenCall([&] { OpenLoadChartFileDialog(context); }); }
				if (Gui::MenuItem("Open Recent", "(TODO)", nullptr, false)) { /* // TODO: ...*/ }
				if (Gui::MenuItem("Open Chart Directory...", ToShortcutString(*Settings.Input.Editor_ChartOpenDirectory).Data, nullptr, CanOpenChartDirectoryInFileExplorer(context))) { OpenChartDirectoryInFileExplorer(context); }
				Gui::Separator();
				if (Gui::MenuItem("Save", ToShortcutString(*Settings.Input.Editor_ChartSave).Data)) { TrySaveChartOrOpenSaveAsDialog(context); }
				if (Gui::MenuItem("Save As...", ToShortcutString(*Settings.Input.Editor_ChartSaveAs).Data)) { OpenChartSaveAsDialog(context); }
				Gui::Separator();
				if (Gui::MenuItem("Exit", ToShortcutString(InputBinding(ImGuiKey_F4, ImGuiModFlags_Alt)).Data))
					tryToCloseApplicationOnNextFrame = true;
				Gui::EndMenu();
			}

			if (Gui::BeginMenu("Edit"))
			{
				if (Gui::MenuItem("Undo", ToShortcutString(*Settings.Input.Editor_Undo).Data, nullptr, context.Undo.CanUndo())) { context.Undo.Undo(); }
				if (Gui::MenuItem("Redo", ToShortcutString(*Settings.Input.Editor_Redo).Data, nullptr, context.Undo.CanRedo())) { context.Undo.Redo(); }
				Gui::Separator();
				if (Gui::MenuItem("Settings...", "(TODO)", nullptr, false)) { /* // TODO: ...*/ }
				Gui::EndMenu();
			}

			if (Gui::BeginMenu("Window"))
			{
				if (bool v = (ApplicationHost::GlobalState.SwapInterval != 0); Gui::MenuItem("Toggle VSync", ToShortcutString(*Settings.Input.Editor_ToggleVSync).Data, &v))
					ApplicationHost::GlobalState.SwapInterval = v ? 1 : 0;

				if (Gui::MenuItem("Toggle Fullscreen", ToShortcutString(*Settings.Input.Editor_ToggleFullscreen).Data, ApplicationHost::GlobalState.IsBorderlessFullscreen))
					ApplicationHost::GlobalState.SetBorderlessFullscreenNextFrame = !ApplicationHost::GlobalState.IsBorderlessFullscreen;

				if (Gui::BeginMenu("Window Size"))
				{
					const bool allowResize = !ApplicationHost::GlobalState.IsBorderlessFullscreen;
					const ivec2 currentResolution = ApplicationHost::GlobalState.WindowSize;

					struct NamedResolution { ivec2 Resolution; const char* Name; };
					static constexpr NamedResolution presetResolutions[] =
					{
						{ { 1280,  720 }, "HD" },
						{ { 1366,  768 }, "FWXGA" },
						{ { 1600,  900 }, "HD+" },
						{ { 1920, 1080 }, "FHD" },
						{ { 2560, 1440 }, "QHD" },
					};

					char labelBuffer[32];
					for (auto[resolution, name] : presetResolutions)
					{
						sprintf_s(labelBuffer, "Resize to %dx%d", resolution.x, resolution.y);
						if (Gui::MenuItem(labelBuffer, name, (resolution == currentResolution), allowResize))
							ApplicationHost::GlobalState.SetWindowSizeNextFrame = resolution;
					}

					Gui::Separator();
					sprintf_s(labelBuffer, "Current Size: %dx%d", currentResolution.x, currentResolution.y);
					Gui::MenuItem(labelBuffer, nullptr, nullptr, false);

					Gui::EndMenu();
				}

				if (Gui::BeginMenu("DPI Scale"))
				{
					const f32 guiScaleFactorToSetNextFrame = GuiScaleFactorToSetNextFrame;
					if (Gui::MenuItem("Zoom In", ToShortcutString(*Settings.Input.Editor_IncreaseGuiScale).Data, nullptr, (GuiScaleFactor < GuiScaleFactorMax)))
						GuiScaleFactorToSetNextFrame = ClampRoundGuiScaleFactor(GuiScaleFactor + ScaleFactorIncrementStep);
					if (Gui::MenuItem("Zoom Out", ToShortcutString(*Settings.Input.Editor_DecreaseGuiScale).Data, nullptr, (GuiScaleFactor > GuiScaleFactorMin)))
						GuiScaleFactorToSetNextFrame = ClampRoundGuiScaleFactor(GuiScaleFactor - ScaleFactorIncrementStep);
					if (Gui::MenuItem("Reset Zoom", ToShortcutString(*Settings.Input.Editor_ResetGuiScale).Data, nullptr, (GuiScaleFactor != 1.0f)))
						GuiScaleFactorToSetNextFrame = 1.0f;

					if (guiScaleFactorToSetNextFrame != GuiScaleFactorToSetNextFrame) { if (!zoomPopup.IsOpen) zoomPopup.Open(); zoomPopup.OnChange(); }

					char labelBuffer[32]; sprintf_s(labelBuffer, "Current Scale: %g%%", ToPercent(GuiScaleFactor));
					Gui::MenuItem(labelBuffer, nullptr, nullptr, false);

					Gui::EndMenu();
				}

				Gui::EndMenu();
			}

			if (Gui::BeginMenu("Test Menu"))
			{
				Gui::MenuItem("Show Audio Test", "(Debug)", &test.ShowAudioTestWindow);
				Gui::MenuItem("Show TJA Import Test", "(Debug)", &test.ShowTJATestWindows);
				Gui::MenuItem("Show TJA Export View", "(Debug)", &test.ShowTJAExportDebugView);
#if !defined(IMGUI_DISABLE_DEMO_WINDOWS)
				Gui::Separator();
				Gui::MenuItem("Show ImGui Demo", " ", &test.ShowImGuiDemoWindow);
				Gui::MenuItem("Show ImGui Style Editor", " ", &test.ShowImGuiStyleEditor);
				Gui::Separator();
				if (Gui::MenuItem("Reset Style Colors", " "))
					GuiStyleColorPeepoDrumKit();
#endif
				Gui::EndMenu();
			}

			if (Gui::BeginMenu("Help"))
			{
				Gui::MenuItem("Copyright (c) 2022", "Samyuu", false, false);
				Gui::MenuItem("Build Time:", BuildInfo::CompilationTime(), false, false);
				Gui::MenuItem("Build Date:", BuildInfo::CompilationDate(), false, false);
				Gui::MenuItem("Build Configuration:", BuildInfo::BuildConfiguration(), false, false);
				Gui::Separator();

				// TODO: Maybe rename to something else
				Gui::MenuItem("Show Help Window", nullptr, &showHelpWindow);
				Gui::EndMenu();
			}

			static constexpr Audio::Backend availableBackends[] = { Audio::Backend::WASAPI_Shared, Audio::Backend::WASAPI_Exclusive, };
			static constexpr auto backendToString = [](Audio::Backend backend) -> const char*
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
					Audio::FramesToTime(Audio::Engine.GetBufferFrameSize(), Audio::Engine.OutputSampleRate).TotalMilliseconds(),
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
				if (Gui::BeginMenu("Courses"))
				{
					// TODO:
					Gui::MenuItem("Add New", "(TODO)", nullptr, false);
					Gui::MenuItem("Edit...", "(TODO)", nullptr, false);
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
						bool isAnyCourseTabSelected = false;

						for (std::unique_ptr<ChartCourse>& course : context.Chart.Courses)
						{
							char buffer[64]; sprintf_s(buffer, "%s x%d (Single)###Course_%p", DifficultyTypeNames[static_cast<i32>(course->Type)], static_cast<i32>(course->Level), course.get());
							const bool setSelectedThisFrame = (course.get() == context.ChartSelectedCourse && course.get() != lastFrameSelectedCoursePtrID);

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
					const bool deviceIsOpen = Audio::Engine.GetIsStreamOpenRunning();
					if (Gui::MenuItem("Open Audio Device", nullptr, false, !deviceIsOpen))
						Audio::Engine.OpenStartStream();
					if (Gui::MenuItem("Close Audio Device", nullptr, false, deviceIsOpen))
						Audio::Engine.StopCloseStream();
					Gui::Separator();

					const Audio::Backend currentBackend = Audio::Engine.GetBackend();
					for (const Audio::Backend backendType : availableBackends)
					{
						char labelBuffer[32];
						sprintf_s(labelBuffer, "Use %s", backendToString(backendType));
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
							"Average: %.3f ms\n"
							"Min: %.3f ms\n"
							"Max: %.3f ms",
							averageFrameTime, minFrameTime, maxFrameTime));

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
		UpdateAsyncLoading();

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
			const bool noActiveID = (Gui::GetActiveID() == 0);
			const bool noOpenPopup = (Gui::GetCurrentContext()->OpenPopupStack.Size <= 0);

			if (noActiveID)
			{
				const f32 guiScaleFactorToSetNextFrame = GuiScaleFactorToSetNextFrame;
				if (Gui::IsAnyPressed(*Settings.Input.Editor_IncreaseGuiScale, true))
					GuiScaleFactorToSetNextFrame = ClampRoundGuiScaleFactor(GuiScaleFactor + ScaleFactorIncrementStep);
				if (Gui::IsAnyPressed(*Settings.Input.Editor_DecreaseGuiScale, true))
					GuiScaleFactorToSetNextFrame = ClampRoundGuiScaleFactor(GuiScaleFactor - ScaleFactorIncrementStep);
				if (Gui::IsAnyPressed(*Settings.Input.Editor_ResetGuiScale, false))
					GuiScaleFactorToSetNextFrame = 1.0f;

				if (guiScaleFactorToSetNextFrame != GuiScaleFactorToSetNextFrame) { if (!zoomPopup.IsOpen) zoomPopup.Open(); zoomPopup.OnChange(); }

				if (Gui::IsAnyPressed(*Settings.Input.Editor_ToggleFullscreen, false))
					ApplicationHost::GlobalState.SetBorderlessFullscreenNextFrame = !ApplicationHost::GlobalState.IsBorderlessFullscreen;

				if (Gui::IsAnyPressed(*Settings.Input.Editor_ToggleVSync, false))
					ApplicationHost::GlobalState.SwapInterval = !ApplicationHost::GlobalState.SwapInterval;
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

		if (showHelpWindow)
		{
			if (Gui::Begin("Usage Guide", &showHelpWindow, ImGuiWindowFlags_None))
				helpWindow.DrawGui(context);
			Gui::End();
		}

		if (Gui::Begin("Undo History", nullptr, ImGuiWindowFlags_None))
		{
			undoHistoryWindow.DrawGui(context);
		}
		Gui::End();

		if (Gui::Begin("Chart Lyrics", nullptr, ImGuiWindowFlags_None))
		{
			lyricsWindow.DrawGui(context, timeline);
		}
		Gui::End();

		if (Gui::Begin("Chart Tempo", nullptr, ImGuiWindowFlags_None))
		{
			tempoWindow.DrawGui(context, timeline);
		}
		Gui::End();

		if (Gui::Begin("Chart Properties", nullptr, ImGuiWindowFlags_None))
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

		// DEBUG: Manually submit debug window before the timeline window is drawn for better tab ordering
		if (Gui::Begin("Chart Timeline - Debug")) { /* ... */ } Gui::End();

		if (Gui::Begin("Game Preview", nullptr, ImGuiWindowFlags_None))
		{
			gamePreview.DrawGui(context, timeline.Camera.WorldSpaceXToTime(timeline.WorldSpaceCursorXAnimationCurrent));
		}
		Gui::End();

		// NOTE: Always update the timeline even if the window isn't visible so that child-windows can be docked properly and hit sounds can always be heard
		Gui::Begin("Chart Timeline", nullptr, ImGuiWindowFlags_None);
		timeline.DrawGui(context);
		Gui::End();

		// NOTE: Test stuff
		{
			if (test.ShowImGuiDemoWindow)
				Gui::ShowDemoWindow(&test.ShowImGuiDemoWindow);

			if (test.ShowImGuiStyleEditor)
			{
				if (Gui::Begin("Style Editor", &test.ShowImGuiStyleEditor))
					Gui::ShowStyleEditor();
				Gui::End();
			}

			if (test.ShowTJATestWindows)
			{
				test.TJATestGui.DrawGui(&test.ShowTJATestWindows);
				if (test.TJATestGui.WasTJAEditedThisFrame)
				{
					CheckOpenSaveConfirmationPopupThenCall([&]
					{
						// HACK: Incredibly inefficient of course but just here for quick testing
						ChartProject convertedChart {};
						CreateChartProjectFromTJA(test.TJATestGui.LoadedTJAFile.Parsed, convertedChart);
						context.Chart = std::move(convertedChart);
						context.ChartFilePath = test.TJATestGui.LoadedTJAFile.FilePath;
						context.ChartSelectedCourse = context.Chart.Courses.empty() ? context.Chart.Courses.emplace_back(std::make_unique<ChartCourse>()).get() : context.Chart.Courses.front().get();
						context.Undo.ClearAll();
					});
				}
			}

			if (test.ShowAudioTestWindow)
				test.AudioTestWindow.DrawGui(&test.ShowAudioTestWindow);

			// DEBUG: LIVE PREVIEW PagMan
			if (test.ShowTJAExportDebugView)
			{
				if (Gui::Begin("TJA Export Debug View", &test.ShowTJAExportDebugView))
				{
					static struct { bool Update = true; i32 Changes = -1, Undos = 0, Redos = 0;  std::string Text; ::TextEditor Editor = CreateImGuiColorTextEditWithNiceTheme(); } exportDebugViewData;
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
			static constexpr Time closeDuration = Time::FromSeconds(2.0);
			const f32 fadeIn = static_cast<f32>(ConvertRangeClampInput(0.0, fadeInDuration.Seconds, 0.0, 1.0, zoomPopup.TimeSinceOpen.Seconds));
			const f32 fadeOut = static_cast<f32>(ConvertRangeClampInput(0.0, fadeOutDuration.Seconds, 0.0, 1.0, (closeDuration - zoomPopup.TimeSinceLastChange).Seconds));

			bool isWindowHovered = false;
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
				Gui::Text("%g%% ", ToPercent(GuiScaleFactor));

				Gui::BeginDisabled(GuiScaleFactor <= GuiScaleFactorMin);
				Gui::SameLine(0.0f, 0.0f);
				if (Gui::Button("-", vec2(Gui::GetFrameHeight(), 0.0f))) { GuiScaleFactorToSetNextFrame = ClampRoundGuiScaleFactor(GuiScaleFactor - ScaleFactorIncrementStep); zoomPopup.OnChange(); }
				Gui::EndDisabled();

				Gui::BeginDisabled(GuiScaleFactor >= GuiScaleFactorMax);
				Gui::SameLine(0.0f, 0.0f);
				if (Gui::Button("+", vec2(Gui::GetFrameHeight(), 0.0f))) { GuiScaleFactorToSetNextFrame = ClampRoundGuiScaleFactor(GuiScaleFactor + ScaleFactorIncrementStep); zoomPopup.OnChange(); }
				Gui::EndDisabled();

				Gui::SameLine(0.0f, 0.0f);
				if (Gui::Button(" Reset ")) { GuiScaleFactorToSetNextFrame = 1.0f; zoomPopup.OnChange(); }
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
				zoomPopup.TimeSinceOpen += Time::FromSeconds(ClampTop(Gui::GetIO().DeltaTime, 1.0f / 30.0f));
				zoomPopup.TimeSinceLastChange = isWindowHovered ? (closeDuration * 0.5) : zoomPopup.TimeSinceLastChange + Time::FromSeconds(Gui::GetIO().DeltaTime);
			}
		}

		// NOTE: Save confirmation popup
		{
			static constexpr const char* saveConfirmationPopupID = "Peepo Drum Kit - Unsaved Changes";
			if (saveConfirmationPopup.OpenOnNextFrame) { Gui::OpenPopup(saveConfirmationPopupID); saveConfirmationPopup.OpenOnNextFrame = false; }

			const ImGuiViewport* mainViewport = Gui::GetMainViewport();
			Gui::SetNextWindowPos(Rect::FromTLSize(mainViewport->Pos, mainViewport->Size).GetCenter(), ImGuiCond_Appearing, vec2(0.5f));

			Gui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { GuiScale(4.0f), Gui::GetStyle().ItemSpacing.y });
			Gui::PushStyleVar(ImGuiStyleVar_WindowPadding, { GuiScale(6.0f), GuiScale(6.0f) });
			bool isPopupOpen = true;
			if (Gui::BeginPopupModal(saveConfirmationPopupID, &isPopupOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
			{
				const vec2 buttonSize = GuiScale(vec2(120.0f, 0.0f));
				Gui::PushFont(FontMedium_EN);
				{
					// NOTE: Manual child size calculation required for proper dynamic scaling
					Gui::BeginChild("TextChild", vec2((buttonSize.x * 3.0f) + Gui::GetStyle().ItemSpacing.x, Gui::GetFontSize() * 3.0f), true, ImGuiWindowFlags_NoBackground);
					Gui::AlignTextToFramePadding();
					Gui::TextUnformatted("Save changes to the current file?");
					Gui::EndChild();
				}
				Gui::PopFont();

				const bool clickedYes = Gui::Button("Save Changes", buttonSize) | (Gui::IsWindowFocused() && Gui::IsAnyPressed(*Settings.Input.Dialog_YesOrOk, false));
				Gui::SameLine();
				const bool clickedNo = Gui::Button("Discard Changes", buttonSize) | (Gui::IsWindowFocused() && Gui::IsAnyPressed(*Settings.Input.Dialog_No, false));
				Gui::SameLine();
				const bool clickedCancel = Gui::Button("Cancel", buttonSize) | (Gui::IsWindowFocused() && Gui::IsAnyPressed(*Settings.Input.Dialog_Cancel, false));

				if (clickedYes || clickedNo || clickedCancel)
				{
					const bool saveDialogCanceled = clickedYes ? !TrySaveChartOrOpenSaveAsDialog(context) : false;
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
		UpdateAsyncLoading();

		context.Chart = {};
		context.ChartFilePath.clear();
		context.ChartSelectedCourse = context.Chart.Courses.empty() ? context.Chart.Courses.emplace_back(std::make_unique<ChartCourse>()).get() : context.Chart.Courses.front().get();
		context.ChartSelectedBranch = BranchType::Normal;

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

			// TODO: Proper saving to file + async (and create .bak backup if not already exists before overwriting existing .tja)

#if 1 // DEBUG:
			assert(Path::HasExtension(filePath, TJA::Extension));
			std::string finalOutputPath { Path::GetDirectoryName(filePath) };
			finalOutputPath.append("/").append(Path::GetFileName(filePath, false)).append(DEBUG_EXPORTED_PEEPODRUMKIT_FILE_SUFFIX).append(TJA::Extension);
			File::WriteAllBytes(finalOutputPath, tjaText);
#endif

			context.ChartFilePath = filePath;
			context.Undo.ClearChangesWereMade();
		}
	}

	bool ChartEditor::OpenChartSaveAsDialog(ChartContext& context)
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

	bool ChartEditor::TrySaveChartOrOpenSaveAsDialog(ChartContext& context)
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

	bool ChartEditor::OpenLoadChartFileDialog(ChartContext& context)
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

	bool ChartEditor::OpenLoadAudioFileDialog(Undo::UndoHistory& undo)
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

	void ChartEditor::UpdateAsyncLoading()
	{
		context.SfxVoicePool.UpdateAsyncLoading();

		if (importChartFuture.valid() && importChartFuture._Is_ready())
		{
			const Time previousChartSongOffset = context.Chart.SongOffset;

			AsyncImportChartResult loadResult = importChartFuture.get();
			context.Chart = std::move(loadResult.Chart);
			context.ChartFilePath = std::move(loadResult.ChartFilePath);
			context.ChartSelectedCourse = context.Chart.Courses.empty() ? context.Chart.Courses.emplace_back(std::make_unique<ChartCourse>()).get() : context.Chart.Courses.front().get();
			StartAsyncLoadingSongAudioFile(Path::TryMakeAbsolute(context.Chart.SongFileName, context.ChartFilePath));

#if 1 // DEBUG:
			const std::string originalFilePath = context.ChartFilePath;
			const std::string_view fileNameWithoutExtension = Path::GetFileName(originalFilePath, false);
			if (ASCII::EndsWithInsensitive(fileNameWithoutExtension, DEBUG_EXPORTED_PEEPODRUMKIT_FILE_SUFFIX))
			{
				context.ChartFilePath = Path::GetDirectoryName(originalFilePath);
				context.ChartFilePath.append("/");
				context.ChartFilePath.append(ASCII::TrimSuffixInsensitive(fileNameWithoutExtension, DEBUG_EXPORTED_PEEPODRUMKIT_FILE_SUFFIX));
				context.ChartFilePath.append(Path::GetExtension(originalFilePath));
			}
#endif

			// NOTE: Prevent the cursor from changing screen position. Not needed if paused because a stable beat time is used instead
			if (context.GetIsPlayback())
				context.SetCursorTime(context.GetCursorTime() + (previousChartSongOffset - context.Chart.SongOffset));

			context.Undo.ClearAll();
		}

		// NOTE: Just in case there is something wrong with the animation, that could otherwise prevent the song from finishing to load
		static constexpr Time maxWaveformFadeOutDelaySafetyLimit = Time::FromSeconds(0.5);
		const bool waveformHasFadedOut = (context.SongWaveformFadeAnimationCurrent <= 0.01f || loadSongStopwatch.GetElapsed() >= maxWaveformFadeOutDelaySafetyLimit);

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
