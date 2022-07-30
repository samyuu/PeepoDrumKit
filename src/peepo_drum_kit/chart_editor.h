#pragma once
#include "core_types.h"
#include "core_string.h"
#include "chart.h"
#include "chart_editor_context.h"
#include "chart_editor_widgets.h"
#include "chart_editor_settings_gui.h"
#include "chart_timeline.h"
#include "chart_game_preview.h"
#include "imgui/imgui_include.h"
#include "audio/audio_engine.h"

#include "test_gui_audio.h"
#include "test_gui_tja.h"

namespace PeepoDrumKit
{
	struct AsyncImportChartResult
	{
		std::string ChartFilePath;
		ChartProject Chart;

		struct TJATempData
		{
			std::string FileContentUTF8;
			std::vector<std::string_view> Lines;
			std::vector<TJA::Token> Tokens;
			TJA::ParsedTJA Parsed;
			TJA::ErrorList ParseErrors;
		} TJA;
	};

	struct AsyncLoadSongResult
	{
		std::string SongFilePath;
		Audio::PCMSampleBuffer SampleBuffer;
		Audio::WaveformMipChain WaveformL, WaveformR;
	};

	struct ChartEditor
	{
	public:
		ChartEditor();
		~ChartEditor();

	public:
		void DrawFullscreenMenuBar();
		void DrawGui();
		ApplicationHost::CloseResponse OnWindowCloseRequest();

	public:
		void UpdateApplicationWindowTitle(const ChartContext& context);

		void CreateNewChart(ChartContext& context);
		void SaveChart(ChartContext& context, std::string_view filePath = "");
		b8 OpenChartSaveAsDialog(ChartContext& context);
		b8 TrySaveChartOrOpenSaveAsDialog(ChartContext& context);

		void StartAsyncImportingChartFile(std::string_view absoluteChartFilePath);
		void StartAsyncLoadingSongAudioFile(std::string_view absoluteAudioFilePath);
		void SetAndStartLoadingChartSongFileName(std::string_view relativeOrAbsoluteAudioFilePath, Undo::UndoHistory& undo);

		b8 OpenLoadChartFileDialog(ChartContext& context);
		b8 OpenLoadAudioFileDialog(Undo::UndoHistory& undo);

		void CheckOpenSaveConfirmationPopupThenCall(std::function<void()> onSuccess);

	private:
		void UpdateAsyncLoading();

	private:
		// TODO: BRING BACK PepePls! (even makes sense in context because of the regular donchan dancing to the beat)
		ChartContext context = {};
		ChartTimeline timeline = {};
		ChartGamePreview gamePreview = {};

		std::future<AsyncImportChartResult> importChartFuture {};
		std::future<AsyncLoadSongResult> loadSongFuture {};
		CPUStopwatch loadSongStopwatch = {};

		b8 tryToCloseApplicationOnNextFrame = false;
		b8 showHelpWindow = false;

		b8 showSettingsWindow = true;
		b8 focusSettingsWindowNextFrame = false;

		ChartHelpWindow helpWindow = {};
		ChartUndoHistoryWindow undoHistoryWindow = {};
		TempoCalculatorWindow tempoCalculatorWindow = {};
		ChartPropertiesWindow propertiesWindow = {};
		ChartTempoWindow tempoWindow = {};
		ChartLyricsWindow lyricsWindow = {};
		ChartSettingsWindow settingsWindow = {};

		struct ZoomPopupData
		{
			b8 IsOpen;
			Time TimeSinceOpen;
			Time TimeSinceLastChange;
			inline void Open() { IsOpen = true; TimeSinceOpen = TimeSinceLastChange = {}; }
			inline void OnChange() { TimeSinceLastChange = {}; }
		} zoomPopup = {};

		struct SaveConfirmationPopupData
		{
			b8 OpenOnNextFrame;
			std::function<void()> OnSuccessFunction;
		} saveConfirmationPopup = {};

		struct TestData
		{
			b8 ShowAudioTestWindow;
			b8 ShowTJATestWindows;
			b8 ShowTJAExportDebugView;
			b8 ShowImGuiDemoWindow;
			b8 ShowImGuiStyleEditor;
			AudioTestWindow AudioTestWindow;
			TJATestWindows TJATestGui;
		} test = {};

		struct PerformanceData
		{
			b8 ShowOverlay;
			f32 FrameTimesMS[256];
			size_t FrameTimeIndex;
			size_t FrameTimeCount;
		} performance = {};
	};
}
