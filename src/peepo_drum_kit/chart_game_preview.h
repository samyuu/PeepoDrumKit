#pragma once
#include "core_types.h"
#include "chart_editor_context.h"
#include "chart_draw_common.h"

namespace PeepoDrumKit
{
	struct ChartGamePreview
	{
		GameCamera Camera = {};

		struct DeferredNoteDrawData { f32 RefLaneHeadX, RefLaneTailX; const Note* OriginalNote; Time NoteTime; };
		std::vector<DeferredNoteDrawData> ReverseNoteDrawBuffer;

		void DrawGui(ChartContext& context, Time animatedCursorTime);
	};
}
