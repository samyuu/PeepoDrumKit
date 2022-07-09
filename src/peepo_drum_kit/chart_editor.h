#pragma once
#include "core_types.h"
#include "core_string.h"
#include "chart.h"
#include "chart_editor_context.h"
#include "chart_editor_widgets.h"
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
		bool OpenChartSaveAsDialog(ChartContext& context);
		bool TrySaveChartOrOpenSaveAsDialog(ChartContext& context);

		void StartAsyncImportingChartFile(std::string_view absoluteChartFilePath);
		void StartAsyncLoadingSongAudioFile(std::string_view absoluteAudioFilePath);
		void SetAndStartLoadingChartSongFileName(std::string_view relativeOrAbsoluteAudioFilePath, Undo::UndoHistory& undo);

		bool OpenLoadChartFileDialog(ChartContext& context);
		bool OpenLoadAudioFileDialog(Undo::UndoHistory& undo);
		
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

		bool tryToCloseApplicationOnNextFrame = false;
		bool showHelpWindow = false;

		ChartHelpWindow helpWindow = {};
		ChartUndoHistoryWindow undoHistoryWindow = {};
		ChartPropertiesWindow propertiesWindow = {};
		ChartTempoWindow tempoWindow = {};
		ChartLyricsWindow lyricsWindow = {};

		struct SaveConfirmationPopupData
		{
			bool OpenOnNextFrame;
			std::function<void()> OnSuccessFunction;
		} saveConfirmationPopup = {};

		struct TestData
		{
			bool ShowAudioTestWindow;
			bool ShowTJATestWindows;
			bool ShowTJAExportDebugView;
			bool ShowImGuiDemoWindow;
			bool ShowImGuiStyleEditor;
			AudioTestWindow AudioTestWindow;
			TJATestWindows TJATestGui;
		} test = {};

		struct PerformanceData
		{
			bool ShowOverlay;
			f32 FrameTimesMS[256];
			size_t FrameTimeIndex;
			size_t FrameTimeCount;
		} performance = {};
	};
}
