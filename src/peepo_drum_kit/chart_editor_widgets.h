#pragma once
#include "core_types.h"
#include "core_string.h"
#include "chart.h"
#include "chart_timeline.h"
#include "chart_editor_context.h"
#include "imgui/imgui_include.h"

namespace PeepoDrumKit
{
	struct LoadingTextAnimation
	{
		bool WasLoadingLastFrame = false;
		u8 RingIndex = 0;
		f32 AccumulatedTimeSec = 0.0f;

		const char* UpdateFrameAndGetText(bool isLoadingThisFrame, f32 deltaTimeSec);
	};

	struct ChartHelpWindow
	{
		void DrawGui(ChartContext& context);
	};

	struct ChartUndoHistoryWindow
	{
		void DrawGui(ChartContext& context);
	};

	struct ChartPropertiesWindowIn { bool IsSongAsyncLoading; };
	struct ChartPropertiesWindowOut { bool BrowseOpenSong; bool LoadNewSong; std::string NewSongFilePath; };
	struct ChartPropertiesWindow
	{
		std::string SongFileNameInputBuffer;
		LoadingTextAnimation SongLoadingTextAnimation {};
		bool DifficultySliderStarsFitOnScreenLastFrame = false;
		bool DifficultySliderStarsWasHoveredLastFrame = false;

		void DrawGui(ChartContext& context, const ChartPropertiesWindowIn& in, ChartPropertiesWindowOut& out);
	};

	struct ChartTempoWindow
	{
		void DrawGui(ChartContext& context, ChartTimeline& timeline);
	};

	struct ChartLyricsWindow
	{
		std::string LyricInputBuffer;

		void DrawGui(ChartContext& context, ChartTimeline& timeline);
	};
}
