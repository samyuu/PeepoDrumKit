#include "chart_editor.h"
#include "core_build_info.h"
#include "chart_undo_commands.h"
#include "audio/audio_file_formats.h"

namespace PeepoDrumKit
{
	// TODO: Populate char[U8Max] lookup table using provided flags and index into instead of using a switch (?)
	enum class EscapeSequenceFlags : u32 { NewLines };

	static void ResolveEscapeSequences(const std::string_view in, std::string& out, EscapeSequenceFlags flags)
	{
		assert(flags == EscapeSequenceFlags::NewLines);
		for (size_t i = 0; i < in.size(); i++)
		{
			if (in[i] == '\\' && i + 1 < in.size())
			{
				switch (in[i + 1])
				{
				case '\\': { out += '\\'; i++; } break;
				case 'n': { out += '\n'; i++; } break;
				}
			}
			else { out += in[i]; }
		}
	}

	static void ConvertToEscapeSequences(const std::string_view in, std::string& out, EscapeSequenceFlags flags)
	{
		assert(flags == EscapeSequenceFlags::NewLines);
		out.reserve(out.size() + in.size());
		for (const char c : in)
		{
			switch (c)
			{
			case '\\': { out += "\\\\"; } break;
			case '\n': { out += "\\n"; } break;
			default: { out += c; } break;
			}
		}
	}
}

namespace PeepoDrumKit
{
	struct MultiWidgetIt { i32 Index; };
	struct MultiWidgetResult { bool ValueChanged; i32 ChangedIndex; };

	template <typename Func>
	static MultiWidgetResult GuiMultiWidget(i32 itemCount, Func onWidgetFunc)
	{
		i32 changedIndex = -1;
		Gui::BeginGroup();
		Gui::PushMultiItemsWidths(itemCount, Gui::CalcItemWidth());
		for (i32 i = 0; i < itemCount; i++)
		{
			if (i > 0)
				Gui::SameLine(0, Gui::GetStyle().ItemInnerSpacing.x);
			if (onWidgetFunc(MultiWidgetIt { i }))
				changedIndex = i;
			Gui::PopItemWidth();
		}
		Gui::EndGroup();
		return MultiWidgetResult { (changedIndex != -1), changedIndex };
	}

	static bool GuiDragFloatLabel(std::string_view label, f32* inOutValue, f32 speed, f32 min, f32 max, const char* format, ImGuiSliderFlags flags)
	{
		bool valueChanged = false;
		Gui::BeginGroup();
		Gui::PushID(Gui::StringViewStart(label), Gui::StringViewEnd(label));
		{
			const auto topLeftCursorPos = Gui::GetCursorScreenPos();

			// TODO: Implement properly without hacky invisible DragFloat (?)
			Gui::PushStyleColor(ImGuiCol_Text, 0); Gui::PushStyleColor(ImGuiCol_FrameBg, 0); Gui::PushStyleColor(ImGuiCol_FrameBgHovered, 0); Gui::PushStyleColor(ImGuiCol_FrameBgActive, 0);
			valueChanged = Gui::DragFloat("##InvisibleDragFloat", inOutValue, speed, min, max, format, flags | ImGuiSliderFlags_NoInput);
			Gui::SetDragScalarMouseCursor();
			Gui::PopStyleColor(4);

			Gui::SetCursorScreenPos(topLeftCursorPos);
			Gui::PushStyleColor(ImGuiCol_Text, Gui::IsItemActive() ? DragTextActiveColor : Gui::IsItemHovered() ? DragTextHoveredColor : Gui::GetColorU32(ImGuiCol_Text));
			Gui::AlignTextToFramePadding(); Gui::TextUnformatted(Gui::StringViewStart(label), Gui::FindRenderedTextEnd(Gui::StringViewStart(label), Gui::StringViewEnd(label)));
			Gui::PopStyleColor(1);
		}
		Gui::PopID();
		Gui::EndGroup();
		return valueChanged;
	}

	static bool GuiInputFraction(const char* label, ivec2* inOutValue, std::optional<ivec2> valueRange)
	{
		static constexpr i32 componentCount = 2;

		constexpr std::string_view divisionText = " / ";
		const f32 divisionLabelWidth = Gui::CalcTextSize(Gui::StringViewStart(divisionText), Gui::StringViewEnd(divisionText)).x;
		const f32 perComponentInputFloatWidth = Floor(((Gui::GetContentRegionAvail().x - divisionLabelWidth) / static_cast<f32>(componentCount)));

		bool anyValueChanged = false;
		for (i32 component = 0; component < componentCount; component++)
		{
			Gui::PushID(&(*inOutValue)[component]);

			const bool isLastComponent = ((component + 1) == componentCount);
			Gui::SetNextItemWidth(isLastComponent ? (Gui::GetContentRegionAvail().x - 1.0f) : perComponentInputFloatWidth);

			if (Gui::InputScalar("##Component", ImGuiDataType_S32, &(*inOutValue)[component], nullptr, nullptr, "%d", ImGuiInputTextFlags_None))
			{
				if (valueRange.has_value()) (*inOutValue)[component] = Clamp((*inOutValue)[component], valueRange->x, valueRange->y);
				anyValueChanged = true;
			}

			if (!isLastComponent)
			{
				Gui::SameLine(0.0f, 0.0f);
				Gui::TextUnformatted(divisionText);
				Gui::SameLine(0.0f, 0.0f);
			}

			Gui::PopID();
		}
		return anyValueChanged;
	}

	static bool GuiDifficultyLevelStarSliderWidget(const char* label, DifficultyLevel* inOutLevel, bool& inOutFitOnScreenLastFrame, bool& inOutHoveredLastFrame)
	{
		bool valueWasChanged = false;

		// NOTE: Make text transparent instead of using an empty slider format string 
		//		 so that the slider can still convert the input to a string on the same frame it is turned into an InputText (due to the frame delayed starsFitOnScreen)
		if (inOutFitOnScreenLastFrame) Gui::PushStyleColor(ImGuiCol_Text, 0x00000000);
		Gui::PushStyleColor(ImGuiCol_SliderGrab, Gui::GetStyleColorVec4(inOutHoveredLastFrame ? ImGuiCol_ButtonHovered : ImGuiCol_Button));
		Gui::PushStyleColor(ImGuiCol_SliderGrabActive, Gui::GetStyleColorVec4(ImGuiCol_ButtonActive));
		Gui::PushStyleColor(ImGuiCol_FrameBgHovered, Gui::GetStyleColorVec4(ImGuiCol_FrameBg));
		Gui::PushStyleColor(ImGuiCol_FrameBgActive, Gui::GetStyleColorVec4(ImGuiCol_FrameBg));
		if (i32 v = static_cast<i32>(*inOutLevel); Gui::SliderInt(label, &v,
			static_cast<i32>(DifficultyLevel::Min), static_cast<i32>(DifficultyLevel::Max), "x%d", ImGuiSliderFlags_AlwaysClamp))
		{
			*inOutLevel = static_cast<DifficultyLevel>(v);
			valueWasChanged = true;
		}
		Gui::PopStyleColor(4 + (inOutFitOnScreenLastFrame ? 1 : 0));

		const Rect sliderRect = Gui::GetItemRect();
		const f32 availableWidth = sliderRect.GetWidth();
		const vec2 starSize = vec2(availableWidth / static_cast<f32>(DifficultyLevel::Max), Gui::GetFrameHeight());

		// NOTE: As noted in https://github.com/ocornut/imgui/issues/3556
		const bool sliderBeingEditedAsText = Gui::IsItemActive() && Gui::TempInputIsActive(Gui::GetActiveID());
		const bool starsFitOnScreen = (starSize.x >= Gui::GetFrameHeight()) && !sliderBeingEditedAsText;

		// NOTE: Use the last frame result here too to match the slider text as it has already been drawn
		if (inOutFitOnScreenLastFrame)
		{
			// NOTE: Manually tuned for a 16px font size
			struct StarParam { f32 OuterRadius, InnerRadius, Thickness; };
			static constexpr StarParam fontSizedStarParamOutline = { 8.0f, 4.0f, 1.0f };
			static constexpr StarParam fontSizedStarParamFilled = { 10.0f, 4.0f, 0.0f };
			const f32 starScale = Gui::GetFontSize() / 16.0f;

			// TODO: Consider drawing star background manually instead of using the slider grab hand (?)
			for (i32 i = 0; i < static_cast<i32>(DifficultyLevel::Max); i++)
			{
				const Rect starRect = Rect::FromTLSize(sliderRect.TL + vec2(i * starSize.x, 0.0f), starSize);
				const auto star = (i >= static_cast<i32>(*inOutLevel)) ? fontSizedStarParamOutline : fontSizedStarParamFilled;
				Gui::DrawStar(Gui::GetWindowDrawList(), starRect.GetCenter(), starScale * star.OuterRadius, starScale * star.InnerRadius, Gui::GetColorU32(ImGuiCol_Text), star.Thickness);
			}
		}

		inOutHoveredLastFrame = Gui::IsItemHovered();
		inOutFitOnScreenLastFrame = starsFitOnScreen;
		return valueWasChanged;
	}

	static void GuiUndoHistoryTable(Undo::UndoHistory& undo, vec2 size)
	{
		const auto& undoStack = undo.UndoStack;
		const auto& redoStack = undo.RedoStack;
		i32 undoClickedIndex = -1, redoClickedIndex = -1;

		const auto& style = Gui::GetStyle();
		// NOTE: Desperate attempt for the table row selectables to not having any spacing between them
		Gui::PushStyleVar(ImGuiStyleVar_CellPadding, { style.CellPadding.x, 4.0f });
		Gui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { style.ItemSpacing.x, 8.0f });
		Gui::PushStyleColor(ImGuiCol_Header, Gui::GetColorU32(ImGuiCol_Header, 0.5f));
		Gui::PushStyleColor(ImGuiCol_HeaderHovered, Gui::GetColorU32(ImGuiCol_HeaderHovered, 0.5f));
		defer { Gui::PopStyleColor(2); Gui::PopStyleVar(2); };

		if (Gui::BeginTable("UndoHistoryTable", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, size))
		{
			Gui::PushFont(FontMedium_EN);
			Gui::TableSetupScrollFreeze(0, 1);
			Gui::TableSetupColumn("Description", ImGuiTableColumnFlags_None);
			Gui::TableSetupColumn("Time", ImGuiTableColumnFlags_None);
			Gui::TableHeadersRow();
			Gui::PopFont();

			static constexpr auto undoCommandRow = [](Undo::CommandInfo commandInfo, CPUTime creationTime, const void* id, bool isSelected)
			{
				Gui::TableNextRow();
				Gui::TableSetColumnIndex(0);
				char labelBuffer[256]; sprintf_s(labelBuffer, "%.*s###%p", FmtStrViewArgs(commandInfo.Description), id);
				const bool clicked = Gui::Selectable(labelBuffer, isSelected, ImGuiSelectableFlags_SpanAllColumns);

				// TODO: Display as formatted local time instead of time relative to program startup (?)
				Gui::TableSetColumnIndex(1);
				Gui::TextDisabled("%s", CPUTime::DeltaTime(CPUTime {}, creationTime).ToString().Data);
				return clicked;
			};

			if (undoCommandRow(Undo::CommandInfo { "Initial State" }, CPUTime {}, nullptr, undoStack.empty()))
				undo.Undo(undoStack.size());

			if (!undoStack.empty())
			{
				ImGuiListClipper clipper; clipper.Begin(static_cast<i32>(undoStack.size()));
				while (clipper.Step())
				{
					for (i32 i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
					{
						if (undoCommandRow(undoStack[i]->GetInfo(), undoStack[i]->CreationTime, undoStack[i].get(), ((i + 1) == undoStack.size())))
							undoClickedIndex = i;
					}
				}
			}

			if (!redoStack.empty())
			{
				Gui::PushStyleColor(ImGuiCol_Text, Gui::GetStyleColorVec4(ImGuiCol_TextDisabled));
				ImGuiListClipper clipper; clipper.Begin(static_cast<i32>(redoStack.size()));
				while (clipper.Step())
				{
					for (i32 i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
					{
						const i32 redoIndex = ((static_cast<i32>(redoStack.size()) - 1) - i);
						if (undoCommandRow(redoStack[redoIndex]->GetInfo(), redoStack[redoIndex]->CreationTime, redoStack[redoIndex].get(), false))
							redoClickedIndex = redoIndex;
					}
				}
				Gui::PopStyleColor();
			}

			// NOTE: Auto scroll if not already at bottom
			if (Gui::GetScrollY() >= Gui::GetScrollMaxY())
				Gui::SetScrollHereY(1.0f);

			Gui::EndTable();
		}

		if (InBounds(undoClickedIndex, undoStack))
			undo.Undo(undoStack.size() - undoClickedIndex - 1);
		else if (InBounds(redoClickedIndex, redoStack))
			undo.Redo(redoStack.size() - redoClickedIndex);
	}
}

namespace PeepoDrumKit
{
	// NOTE: Soft clamp for sliders but hard limit to allow *typing in* values higher, even if it can cause clipping
	static constexpr f32 MinVolume = 0.0f;
	static constexpr f32 MaxVolumeSoftLimit = 1.0f;
	static constexpr f32 MaxVolumeHardLimit = 4.0f;

	static constexpr std::string_view UntitledChartFileName = "Untitled Chart.tja";

	// DEBUG: Save and automatically load a separate copy so to never overwrite the original .tja (due to conversion data loss)
	static constexpr std::string_view DEBUG_EXPORTED_PEEPODRUMKIT_FILE_SUFFIX = " (PeepoDrumKit)";

	static void DrawChartEditorHelpWindowContent(ChartEditor& chartEditor)
	{
		static struct
		{
			u32 GreenDark = 0xFFACF7DC; // 0xFF60BE9C;
			u32 GreenBright = 0xFF95CCB8; // 0xFF60BE9C;
			u32 RedDark = 0xFF72A0ED; // 0xFF71A4FA; // 0xFF7474EF; // 0xFF6363DE;
			u32 RedBright = 0xFF93B2E7; // 0xFFBAC2E4; // 0xFF6363DE;
			u32 WhiteDark = 0xFF95DCDC; // 0xFFEAEAEA;
			u32 WhiteBright = 0xFFBBD3D3; // 0xFFEAEAEA;
			bool Show = false;
		} colors;
#if PEEPO_DEBUG
		if (Gui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && Gui::IsKeyPressed(ImGuiKey_GraveAccent))
			colors.Show ^= true;
		if (colors.Show)
		{
			Gui::ColorEdit4_U32("Green Dark", &colors.GreenDark);
			Gui::ColorEdit4_U32("Green Bright", &colors.GreenBright);
			Gui::ColorEdit4_U32("Red Dark", &colors.RedDark);
			Gui::ColorEdit4_U32("Red Bright", &colors.RedBright);
			Gui::ColorEdit4_U32("White Dark", &colors.WhiteDark);
			Gui::ColorEdit4_U32("White Bright", &colors.WhiteBright);
		}
#endif

		Gui::PushStyleColor(ImGuiCol_Border, Gui::GetColorU32(ImGuiCol_Separator));
		Gui::BeginChild("BackgroundChild", Gui::GetContentRegionAvail(), true);
		Gui::PopStyleColor(1);
		defer { Gui::EndChild(); };

		// TODO: Maybe use .md markup language to write instructions (?) (how feasable would it be to integrate peepo emotes inbetween text..?)
		Gui::PushFont(FontLarge_EN);
		{
			{
				Gui::PushStyleColor(ImGuiCol_Text, colors.GreenDark);
				Gui::PushFont(FontLarge_EN);
				Gui::TextUnformatted("Welcome to Peepo Drum Kit (Alpha)");
				Gui::PopFont();
				Gui::PopStyleColor();

				Gui::PushStyleColor(ImGuiCol_Text, colors.GreenBright);
				Gui::PushFont(FontMedium_EN);
				Gui::TextWrapped("Things are still very much WIP and subject to change with many features still missing :FeelsOkayMan:");
				Gui::Separator();
				Gui::TextUnformatted("");
				Gui::PopFont();
				Gui::PopStyleColor();
			}

			{
				Gui::PushStyleColor(ImGuiCol_Text, colors.RedDark);
				Gui::PushFont(FontMedium_EN);
				Gui::TextUnformatted("Basic Controls:");
				Gui::PopFont();
				Gui::PopStyleColor();

				Gui::PushStyleColor(ImGuiCol_Text, colors.RedBright);
				Gui::PushFont(FontMain_JP);
				if (Gui::BeginTable("ControlsTable", 2, ImGuiTableFlags_BordersInner | ImGuiTableFlags_NoSavedSettings))
				{
					static constexpr auto row = [&](auto funcLeft, auto funcRight)
					{
						Gui::TableNextRow();
						Gui::TableSetColumnIndex(0); /*Gui::AlignTextToFramePadding();*/ funcLeft();
						Gui::TableSetColumnIndex(1); /*Gui::AlignTextToFramePadding();*/ funcRight();
					};
					static constexpr auto rowSeparator = []() { row([] { Gui::Text(""); }, [] {}); };

					row([] { Gui::Text("Move cursor"); }, [] { Gui::Text("Mouse Left"); });
					row([] { Gui::Text("Select notes"); }, [] { Gui::Text("Mouse Right"); });
					row([] { Gui::Text("Scroll"); }, [] { Gui::Text("Mouse Wheel"); });
					row([] { Gui::Text("Scroll panning"); }, [] { Gui::Text("Mouse Middle"); });
					row([] { Gui::Text("Zoom"); }, [] { Gui::Text("Alt + Mouse Wheel"); });
					row([] { Gui::Text("Fast (Action)"); }, [] { Gui::Text("Shift + { Action }"); });
					row([] { Gui::Text("Slow (Action)"); }, [] { Gui::Text("Alt + { Action }"); });
					rowSeparator();
					row([] { Gui::Text("Play / pause"); }, [] { Gui::Text(ToShortcutString(GlobalSettings.Input.Timeline_TogglePlayback).Data); });
					row([] { Gui::Text("Add / remove note"); }, []
					{
						assert(GlobalSettings.Input.Timeline_PlaceNoteKa.Count == 2 && GlobalSettings.Input.Timeline_PlaceNoteDon.Count == 2);
						Gui::Text("%s / %s / %s / %s",
							ToShortcutString(GlobalSettings.Input.Timeline_PlaceNoteKa.Slots[0]).Data, ToShortcutString(GlobalSettings.Input.Timeline_PlaceNoteDon.Slots[0]).Data,
							ToShortcutString(GlobalSettings.Input.Timeline_PlaceNoteDon.Slots[1]).Data, ToShortcutString(GlobalSettings.Input.Timeline_PlaceNoteKa.Slots[1]).Data);
					});
					row([] { Gui::Text("Add long note"); }, []
					{
						assert(GlobalSettings.Input.Timeline_PlaceNoteBalloon.Count == 2 && GlobalSettings.Input.Timeline_PlaceNoteDrumroll.Count == 2);
						Gui::Text("%s / %s / %s / %s",
							ToShortcutString(GlobalSettings.Input.Timeline_PlaceNoteBalloon.Slots[0]).Data, ToShortcutString(GlobalSettings.Input.Timeline_PlaceNoteDrumroll.Slots[0]).Data,
							ToShortcutString(GlobalSettings.Input.Timeline_PlaceNoteDrumroll.Slots[1]).Data, ToShortcutString(GlobalSettings.Input.Timeline_PlaceNoteBalloon.Slots[1]).Data);
					});
					row([] { Gui::Text("Add big note"); }, [] { Gui::Text("Alt + { Note }"); });
					row([] { Gui::Text("Fill range-selection with notes"); }, [] { Gui::Text("Shift + { Note }"); });
					row([] { Gui::Text("Start / end range-selection"); }, [] { Gui::Text(ToShortcutString(GlobalSettings.Input.Timeline_StartEndRangeSelection).Data); });
					row([] { Gui::Text("Grid division"); }, [] { Gui::Text("Mouse [X1/X2] / [%s/%s]", ToShortcutString(GlobalSettings.Input.Timeline_IncreaseGridDivision).Data, ToShortcutString(GlobalSettings.Input.Timeline_DecreaseGridDivision).Data); });
					row([] { Gui::Text("Step cursor"); }, [] { Gui::Text("%s / %s", ToShortcutString(GlobalSettings.Input.Timeline_StepCursorLeft).Data, ToShortcutString(GlobalSettings.Input.Timeline_StepCursorRight).Data); });
					rowSeparator();
					row([] { Gui::Text("Move selected notes"); }, [] { Gui::Text("Mouse Left (Hover)"); });
					row([] { Gui::Text("Cut selected notes"); }, [] { Gui::Text(ToShortcutString(GlobalSettings.Input.Timeline_Cut).Data); });
					row([] { Gui::Text("Copy selected notes"); }, [] { Gui::Text(ToShortcutString(GlobalSettings.Input.Timeline_Copy).Data); });
					row([] { Gui::Text("Paste selected notes"); }, [] { Gui::Text(ToShortcutString(GlobalSettings.Input.Timeline_Paste).Data); });
					row([] { Gui::Text("Delete selected notes"); }, [] { Gui::Text(ToShortcutString(GlobalSettings.Input.Timeline_DeleteSelection).Data); });
					rowSeparator();
					row([] { Gui::Text("Playback speed"); }, [] { Gui::Text("%s / %s", ToShortcutString(GlobalSettings.Input.Timeline_DecreasePlaybackSpeed).Data, ToShortcutString(GlobalSettings.Input.Timeline_IncreasePlaybackSpeed).Data); });
					row([] { Gui::Text("Toggle metronome"); }, [] { Gui::Text(ToShortcutString(GlobalSettings.Input.Timeline_ToggleMetronome).Data); });

					Gui::EndTable();
				}
				Gui::PopFont();
				Gui::PopStyleColor();
			}

			{
				Gui::PushFont(FontMedium_EN);
				Gui::PushStyleColor(ImGuiCol_Text, colors.WhiteDark);
				Gui::TextUnformatted("");
				Gui::TextUnformatted("About reading and writing TJAs:");
				Gui::PopStyleColor();
				Gui::Separator();

				Gui::PushStyleColor(ImGuiCol_Text, colors.WhiteBright);
				Gui::TextWrapped(
					"Despite what it might look like, Peepo Drum Kit is not really a \"TJA Editor\" in the traditional sense.\n"
					"It's a Taiko chart editor that transparently converts *to* and *from* the TJA format\n"
					"however its internal data representation of a chart is drastically different and optimized for easy data transformations.\n"
					"\n"
					"As such, potential data-loss is the convertion process *to some extend* is to be expected.\n"
					"This may either take the form of general formatting differences (removing comments, white space, etc.),\n"
					"issues with (yet) unimplemented features (such as branches and other commands)\n"
					"or general \"rounding errors\" due to notes and other commands being quantized onto a fixed 1/192nd grid (as is a common rhythm game convention).\n"
					"\n"
					"To prevent the user from accidentally overwriting existing TJAs, edited TJAs will therefore always be saved with a \"%.*s\" suffix.\n"
					"(... for now at least)\n"
					"\n"
					"With that said, none of this should be a problem when creating new charts from inside the program itself\n"
					"as the goal is for the user to not have to interact with the \".tja\" in text form *at all* :FeelsOkayMan:"
					, FmtStrViewArgs(DEBUG_EXPORTED_PEEPODRUMKIT_FILE_SUFFIX));
				Gui::PopStyleColor();
				Gui::PopFont();
			}
		}
		Gui::PopFont();
	}

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

	const char* LoadingTextAnimationData::UpdateFrameAndGetText(bool isLoadingThisFrame, f32 deltaTimeSec)
	{
		if (!WasLoadingLastFrame && isLoadingThisFrame)
			RingIndex = 0;

		WasLoadingLastFrame = isLoadingThisFrame;
		if (isLoadingThisFrame)
			AccumulatedTimeSec += deltaTimeSec;

		const char* loadingText = TextFrames[RingIndex];
		if (AccumulatedTimeSec >= FrameIntervalSec)
		{
			AccumulatedTimeSec -= FrameIntervalSec;
			if (++RingIndex >= ArrayCountI32(TextFrames))
				RingIndex = 0;
		}
		return loadingText;
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
				if (Gui::MenuItem("New Chart", ToShortcutString(GlobalSettings.Input.Editor_ChartNew).Data)) { CheckOpenSaveConfirmationPopupThenCall([&] { CreateNewChart(context); }); }
				if (Gui::MenuItem("Open...", ToShortcutString(GlobalSettings.Input.Editor_ChartOpen).Data)) { CheckOpenSaveConfirmationPopupThenCall([&] { OpenLoadChartFileDialog(context); }); }
				if (Gui::MenuItem("Open Recent", "(TODO)", nullptr, false)) { /* // TODO: ...*/ }
				if (Gui::MenuItem("Open Chart Directory...", ToShortcutString(GlobalSettings.Input.Editor_ChartOpenDirectory).Data, nullptr, CanOpenChartDirectoryInFileExplorer(context))) { OpenChartDirectoryInFileExplorer(context); }
				Gui::Separator();
				if (Gui::MenuItem("Save", ToShortcutString(GlobalSettings.Input.Editor_ChartSave).Data)) { TrySaveChartOrOpenSaveAsDialog(context); }
				if (Gui::MenuItem("Save As...", ToShortcutString(GlobalSettings.Input.Editor_ChartSaveAs).Data)) { OpenChartSaveAsDialog(context); }
				Gui::Separator();
				if (Gui::MenuItem("Exit", ToShortcutString(InputBinding(ImGuiKey_F4, ImGuiKeyModFlags_Alt)).Data))
					tryToCloseApplicationOnNextFrame = true;
				Gui::EndMenu();
			}

			if (Gui::BeginMenu("Edit"))
			{
				if (Gui::MenuItem("Undo", ToShortcutString(GlobalSettings.Input.Editor_Undo).Data, nullptr, context.Undo.CanUndo())) { context.Undo.Undo(); }
				if (Gui::MenuItem("Redo", ToShortcutString(GlobalSettings.Input.Editor_Redo).Data, nullptr, context.Undo.CanRedo())) { context.Undo.Redo(); }
				Gui::Separator();
				if (Gui::MenuItem("Settings...", "(TODO)", nullptr, false)) { /* // TODO: ...*/ }
				Gui::EndMenu();
			}

			if (Gui::BeginMenu("Window"))
			{
				if (Gui::MenuItem("Toggle Fullscreen", ToShortcutString(GlobalSettings.Input.Editor_ToggleFullscreen).Data))
					ApplicationHost::GlobalState.SetBorderlessFullscreenNextFrame = !ApplicationHost::GlobalState.IsBorderlessFullscreen;

				if (Gui::BeginMenu("Swap Interval"))
				{
					if (Gui::MenuItem("Swap Interval 0 - Unlimited", nullptr, (ApplicationHost::GlobalState.SwapInterval == 0))) ApplicationHost::GlobalState.SwapInterval = 0;
					if (Gui::MenuItem("Swap Interval 1 - VSync", nullptr, (ApplicationHost::GlobalState.SwapInterval == 1))) ApplicationHost::GlobalState.SwapInterval = 1;
					Gui::EndMenu();
				}

				if (Gui::BeginMenu("Resize"))
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
					GuiStyleColorNiceLimeGreen();
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
					Gui::SetNextWindowPos(vec2(mainViewport->Pos.x + mainViewport->Size.x, mainViewport->Pos.y + 24.0f), ImGuiCond_Always, vec2(1.0f, 0.0f));
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
							"", scaleMin, scaleMax, vec2(static_cast<f32>(ArrayCount(performance.FrameTimesMS)), plotLinesHeight));
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
			// BUG: Should also unload song instantly when dropping a new audio file..?
			if (Path::HasAnyExtension(droppedFilePath, TJA::Extension)) { CheckOpenSaveConfirmationPopupThenCall([this, pathCopy = droppedFilePath] { StartAsyncImportingChartFile(pathCopy); }); break; }
			if (Path::HasAnyExtension(droppedFilePath, Audio::SupportedFileFormatExtensionsPacked)) { SetAndStartLoadingChartSongFileName(droppedFilePath, context.Undo); break; }
			// HACK: To allow quickly dropping folders from songs directory without manually selecting tja within (should probably iterate directory for first tja instead)
			if (Path::IsDirectory(droppedFilePath))
			{
				std::string tjaPath = std::string(droppedFilePath).append("/").append(Path::GetFileName(droppedFilePath)).append(TJA::Extension);
				if (File::Exists(tjaPath)) { CheckOpenSaveConfirmationPopupThenCall([this, pathCopy = std::move(tjaPath)] { StartAsyncImportingChartFile(pathCopy); }); break; }
			}
		}

		// NOTE: Global input bindings
		{
			// HACK: Works for now I guess...
			if (/*parentApplication.GetHost().IsWindowFocused() &&*/ Gui::GetActiveID() == 0)
			{
				// HACK: Not quite sure about this one yet but seems reasonable..?
				if (Gui::GetCurrentContext()->OpenPopupStack.Size <= 0)
				{
					if (Gui::IsAnyPressed(GlobalSettings.Input.Editor_ToggleFullscreen, false))
						ApplicationHost::GlobalState.SetBorderlessFullscreenNextFrame = !ApplicationHost::GlobalState.IsBorderlessFullscreen;

					if (Gui::IsAnyPressed(GlobalSettings.Input.Editor_Undo, true))
						context.Undo.Undo();

					if (Gui::IsAnyPressed(GlobalSettings.Input.Editor_Redo, true))
						context.Undo.Redo();

					if (Gui::IsAnyPressed(GlobalSettings.Input.Editor_ChartNew, false))
						CheckOpenSaveConfirmationPopupThenCall([&] { CreateNewChart(context); });

					if (Gui::IsAnyPressed(GlobalSettings.Input.Editor_ChartOpen, false))
						CheckOpenSaveConfirmationPopupThenCall([&] { OpenLoadChartFileDialog(context); });

					if (Gui::IsAnyPressed(GlobalSettings.Input.Editor_ChartOpenDirectory, false) && CanOpenChartDirectoryInFileExplorer(context))
						OpenChartDirectoryInFileExplorer(context);

					if (Gui::IsAnyPressed(GlobalSettings.Input.Editor_ChartSave, false))
						TrySaveChartOrOpenSaveAsDialog(context);

					if (Gui::IsAnyPressed(GlobalSettings.Input.Editor_ChartSaveAs, false))
						OpenChartSaveAsDialog(context);
				}
			}
		}

		if (showHelpWindow)
		{
			if (Gui::Begin("Usage Guide", &showHelpWindow, ImGuiWindowFlags_None))
				DrawChartEditorHelpWindowContent(*this);
			Gui::End();
		}

		if (Gui::Begin("Undo History", nullptr, ImGuiWindowFlags_None))
			GuiUndoHistoryTable(context.Undo, Gui::GetContentRegionAvail());
		Gui::End();

		if (Gui::Begin("Chart Lyrics", nullptr, ImGuiWindowFlags_None))
		{
			assert(context.ChartSelectedCourse != nullptr);
			auto& chart = context.Chart;
			auto& course = *context.ChartSelectedCourse;

			// TODO: Separate lyrics tab with options to write/import lyrics text in some standard format (?)
			const Beat cursorBeat = FloorBeatToGrid(context.GetCursorBeat(), GetGridBeatSnap(timeline.CurrentGridBarDivision));
			const LyricChange* lyricChangeAtCursor = context.ChartSelectedCourse->Lyrics.TryFindLastAtBeat(cursorBeat);
			Gui::BeginDisabled(cursorBeat.Ticks < 0);

			static constexpr auto guiDoubleButton = [](const char* labelA, const char* labelB) -> i32
			{
				return GuiMultiWidget(2, [&](const MultiWidgetIt& i) { return Gui::Button((i.Index == 0) ? labelA : labelB, { Gui::CalcItemWidth(), 0.0f }); }).ChangedIndex;
			};

			// TODO: Handle inside the tja format code instead (?)
			lyricInputBuffer.clear();
			if (lyricChangeAtCursor != nullptr)
				ResolveEscapeSequences(lyricChangeAtCursor->Lyric, lyricInputBuffer, EscapeSequenceFlags::NewLines);

			const f32 textInputHeight = ClampBot(Gui::GetContentRegionAvail().y - Gui::GetFrameHeightWithSpacing(), Gui::GetFrameHeightWithSpacing());
			Gui::SetNextItemWidth(-1.0f);
			if (Gui::InputTextMultilineWithHint("##LyricAtCursor", "n/a", &lyricInputBuffer, { -1.0f, textInputHeight }, ImGuiInputTextFlags_None))
			{
				std::string newLyricLine;
				ConvertToEscapeSequences(lyricInputBuffer, newLyricLine, EscapeSequenceFlags::NewLines);

				if (lyricChangeAtCursor == nullptr || lyricChangeAtCursor->BeatTime != cursorBeat)
					context.Undo.Execute<Commands::AddLyricChange>(&course.Lyrics, LyricChange { cursorBeat, std::move(newLyricLine) });
				else
					context.Undo.Execute<Commands::UpdateLyricChange>(&course.Lyrics, LyricChange { cursorBeat, std::move(newLyricLine) });
			}

			Gui::SetNextItemWidth(-1.0f);
			if (i32 clicked = guiDoubleButton("Clear##LyricAtCursor", "Remove##LyricAtCursor"); clicked > -1)
			{
				if (clicked == 0)
				{
					if (lyricChangeAtCursor == nullptr || lyricChangeAtCursor->BeatTime != cursorBeat)
						context.Undo.Execute<Commands::AddLyricChange>(&course.Lyrics, LyricChange { cursorBeat, "" });
					else
						context.Undo.Execute<Commands::UpdateLyricChange>(&course.Lyrics, LyricChange { cursorBeat, "" });
				}
				else if (clicked == 1)
				{
					if (lyricChangeAtCursor != nullptr && lyricChangeAtCursor->BeatTime == cursorBeat)
						context.Undo.Execute<Commands::RemoveLyricChange>(&course.Lyrics, cursorBeat);
				}
			}

			Gui::EndDisabled();
		}
		Gui::End();

		if (Gui::Begin("Chart Tempo", nullptr, ImGuiWindowFlags_None))
		{
			assert(context.ChartSelectedCourse != nullptr);
			auto& chart = context.Chart;
			auto& course = *context.ChartSelectedCourse;

			if (Gui::CollapsingHeader("Sync", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (Gui::Property::BeginTable(ImGuiTableFlags_BordersInner))
				{
					enum class TimeSpace : u8 { Chart, Song };
					static constexpr auto guiDragTimeAndSetCursorTimeButtonWidgets = [](ChartContext& context, const TimelineCamera& camera, const char* label, Time* inOutValue, TimeSpace timeSpace) -> bool
					{
						const Time timeSpaceOffset = (timeSpace == TimeSpace::Song) ? context.Chart.SongOffset : Time::Zero();
						Gui::PushID(label); const auto result = GuiMultiWidget(2, [&](const MultiWidgetIt& i)
						{
							static_assert(sizeof(Time::Seconds) == sizeof(double));
							const Time min = timeSpaceOffset, max = Time::FromSeconds(F64Max);
							Time v = (*inOutValue + timeSpaceOffset);
							if (i.Index == 0 && Gui::DragScalar("##DragTime", ImGuiDataType_Double, &v.Seconds, TimelineDragScalarSpeedAtZoomSec(camera), &min.Seconds, &max.Seconds,
								(*inOutValue <= Time::Zero()) ? "n/a" : v.ToString().Data, ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoRoundToFormat | ImGuiSliderFlags_NoInput))
							{
								*inOutValue = (v - timeSpaceOffset);
								return true;
							}
							else if (i.Index == 1 && Gui::Button("Set Cursor##CursorTime", { Gui::CalcItemWidth(), 0.0f }))
							{
								*inOutValue = ClampBot(context.GetCursorTime() - timeSpaceOffset, Time::Zero());
								context.Undo.DisallowMergeForLastCommand();
								return true;
							}
							return false;
						});
						Gui::PopID(); return result.ValueChanged;
					};

					Gui::Property::PropertyTextValueFunc("Chart Duration", [&]
					{
						Gui::SetNextItemWidth(-1.0f);
						if (Time v = chart.ChartDuration; guiDragTimeAndSetCursorTimeButtonWidgets(context, timeline.Camera, "##ChartDuration", &v, TimeSpace::Chart))
							context.Undo.Execute<Commands::ChangeChartDuration>(&chart, v);
					});
					Gui::Property::PropertyTextValueFunc("Song Demo Start", [&]
					{
						Gui::SetNextItemWidth(-1.0f);
						if (Time v = chart.SongDemoStartTime; guiDragTimeAndSetCursorTimeButtonWidgets(context, timeline.Camera, "##SongDemoStartTime", &v, TimeSpace::Song))
							context.Undo.Execute<Commands::ChangeSongDemoStartTime>(&chart, v);
					});
					Gui::Property::PropertyTextValueFunc("Song Offset", [&]
					{
						Gui::SetNextItemWidth(-1.0f);
						if (f32 v = static_cast<f32>(chart.SongOffset.TotalMilliseconds()); Gui::DragFloat("##ChartSongOffset", &v, TimelineDragScalarSpeedAtZoomMS(timeline.Camera), 0.0f, 0.0f, "%.2f ms", ImGuiSliderFlags_NoRoundToFormat))
							context.Undo.Execute<Commands::ChangeSongOffset>(&chart, Time::FromMilliseconds(v));

						// TODO: Disable merge if made inactive this frame (?)
						// if (Gui::IsItemDeactivatedAfterEdit()) {}
					});
					Gui::Property::EndTable();
				}
			}

			if (Gui::CollapsingHeader("Tempo", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (Gui::Property::BeginTable(ImGuiTableFlags_BordersInner))
				{
					// TODO: Might wanna disable during playbackt to not spam a bunch of tempo changes etc. (but don't dim out widgets as it might become annoying)
					const Beat cursorBeat = FloorBeatToGrid(context.GetCursorBeat(), GetGridBeatSnap(timeline.CurrentGridBarDivision));
					Gui::BeginDisabled(cursorBeat.Ticks < 0);

					const TempoChange tempoChangeAtCursor = course.TempoMap.TempoFindLastAtBeat(cursorBeat);
					auto insertOrUpdateCursorTempoChange = [&](Tempo newTempo)
					{
						if (tempoChangeAtCursor.Beat != cursorBeat)
							context.Undo.Execute<Commands::AddTempoChange>(&course.TempoMap, TempoChange(cursorBeat, newTempo));
						else
							context.Undo.Execute<Commands::UpdateTempoChange>(&course.TempoMap, TempoChange(cursorBeat, newTempo));
					};

					Gui::Property::Property([&]
					{
						Gui::SetNextItemWidth(-1.0f);
						if (f32 v = tempoChangeAtCursor.Tempo.BPM; GuiDragFloatLabel("Tempo##DragFloatLabel", &v, 1.0f, Tempo::MinBPM, Tempo::MaxBPM, "%g", ImGuiSliderFlags_AlwaysClamp))
							insertOrUpdateCursorTempoChange(Tempo(v));
					});
					Gui::Property::Value([&]
					{
						Gui::SetNextItemWidth(-1.0f);
						if (f32 v = tempoChangeAtCursor.Tempo.BPM; Gui::InputFloat("##TempoAtCursor", &v, 1.0f, 10.0f, "%g BPM", ImGuiInputTextFlags_None))
							insertOrUpdateCursorTempoChange(Tempo(Clamp(v, 1.0f, Tempo::MaxBPM)));

						if (Gui::Button("Remove##TempoAtCursor", vec2(-1.0f, 0.0f)))
						{
							if (tempoChangeAtCursor.Beat == cursorBeat)
								context.Undo.Execute<Commands::RemoveTempoChange>(&course.TempoMap, cursorBeat);
						}
					});
					Gui::Property::PropertyTextValueFunc("Time Signature", [&]
					{
						const TimeSignatureChange signatureChangeAtCursor = course.TempoMap.SignatureFindLastAtBeat(cursorBeat);

						Gui::SetNextItemWidth(-1.0f);
						if (ivec2 v = { signatureChangeAtCursor.Signature.Numerator, signatureChangeAtCursor.Signature.Denominator };
							GuiInputFraction("##SignatureAtCursor", &v, ivec2(1, Beat::TicksPerBeat * 4)))
						{
							// TODO: Also floor cursor beat to ~~last~~ next whole bar
							if (signatureChangeAtCursor.Beat != cursorBeat)
								context.Undo.Execute<Commands::AddTimeSignatureChange>(&course.TempoMap, TimeSignatureChange(cursorBeat, TimeSignature(v[0], v[1])));
							else
								context.Undo.Execute<Commands::UpdateTimeSignatureChange>(&course.TempoMap, TimeSignatureChange(cursorBeat, TimeSignature(v[0], v[1])));
						}

						if (Gui::Button("Remove##SignatureAtCursor", vec2(-1.0f, 0.0f)))
						{
							if (signatureChangeAtCursor.Beat == cursorBeat)
								context.Undo.Execute<Commands::RemoveTimeSignatureChange>(&course.TempoMap, cursorBeat);
						}
					});
					Gui::Property::PropertyTextValueFunc("Scroll Speed", [&]
					{
						static constexpr f32 minMaxScrollSpeed = 100.0f;
						const ScrollChange* scrollChangeChangeAtCursor = course.ScrollChanges.TryFindLastAtBeat(cursorBeat);

						std::optional<f32> newScrollSpeed = {};
						Gui::SetNextItemWidth(-1.0f); GuiMultiWidget(2, [&](const MultiWidgetIt& i)
						{
							if (i.Index == 0)
							{
								if (f32 v = (scrollChangeChangeAtCursor == nullptr) ? 1.0f : scrollChangeChangeAtCursor->ScrollSpeed;
									Gui::DragFloat("##ScrollSpeedAtCursor", &v, 0.005f, -minMaxScrollSpeed, +minMaxScrollSpeed, "%gx", ImGuiSliderFlags_NoRoundToFormat))
									newScrollSpeed = v;
							}
							else
							{
								if (f32 v = tempoChangeAtCursor.Tempo.BPM * ((scrollChangeChangeAtCursor == nullptr) ? 1.0f : scrollChangeChangeAtCursor->ScrollSpeed);
									Gui::DragFloat("##ScrollTempoAtCursor", &v, 1.0f, Tempo::MinBPM, Tempo::MaxBPM, "%g BPM", ImGuiSliderFlags_NoRoundToFormat))
									newScrollSpeed = (v / tempoChangeAtCursor.Tempo.BPM);
							}
							return false;
						});

						if (newScrollSpeed.has_value())
						{
							if (scrollChangeChangeAtCursor == nullptr || scrollChangeChangeAtCursor->BeatTime != cursorBeat)
								context.Undo.Execute<Commands::AddScrollChange>(&course.ScrollChanges, ScrollChange { cursorBeat, newScrollSpeed.value() });
							else
								context.Undo.Execute<Commands::UpdateScrollChange>(&course.ScrollChanges, ScrollChange { cursorBeat, newScrollSpeed.value() });
						}

						if (Gui::Button("Remove##ScrollSpeedAtCursor", vec2(-1.0f, 0.0f)))
						{
							if (scrollChangeChangeAtCursor != nullptr && scrollChangeChangeAtCursor->BeatTime == cursorBeat)
								context.Undo.Execute<Commands::RemoveScrollChange>(&course.ScrollChanges, cursorBeat);
						}
					});
					Gui::Property::PropertyTextValueFunc("Bar Line Visibility", [&]
					{
						const BarLineChange* barLineChangeAtCursor = course.BarLineChanges.TryFindLastAtBeat(cursorBeat);

						static constexpr auto guiOnOffButton = [](const char* label, const char* onLabel, const char* offLabel, bool* inOutIsOn) -> bool
						{
							bool valueChanged = false;
							Gui::PushID(label); GuiMultiWidget(2, [&](const MultiWidgetIt& i)
							{
								const f32 alphaFactor = ((i.Index == 0) == *inOutIsOn) ? 1.0f : 0.5f;
								Gui::PushStyleColor(ImGuiCol_Button, Gui::GetColorU32(ImGuiCol_Button, alphaFactor));
								Gui::PushStyleColor(ImGuiCol_ButtonHovered, Gui::GetColorU32(ImGuiCol_ButtonHovered, alphaFactor));
								Gui::PushStyleColor(ImGuiCol_ButtonActive, Gui::GetColorU32(ImGuiCol_ButtonActive, alphaFactor));
								if (Gui::Button((i.Index == 0) ? onLabel : offLabel, { Gui::CalcItemWidth(), 0.0f })) { *inOutIsOn = (i.Index == 0); valueChanged = true; }
								Gui::PopStyleColor(3);
								return false;
							});
							Gui::PopID(); return valueChanged;
						};

						Gui::SetNextItemWidth(-1.0f);
						if (bool v = (barLineChangeAtCursor == nullptr) ? true : barLineChangeAtCursor->IsVisible;
							guiOnOffButton("##OnOffBarLineAtCursor", "Visible", "Hidden", &v))
						{
							if (barLineChangeAtCursor == nullptr || barLineChangeAtCursor->BeatTime != cursorBeat)
								context.Undo.Execute<Commands::AddBarLineChange>(&course.BarLineChanges, BarLineChange { cursorBeat, v });
							else
								context.Undo.Execute<Commands::UpdateBarLineChange>(&course.BarLineChanges, BarLineChange { cursorBeat, v });
						}

						if (Gui::Button("Remove##BarLineAtCursor", vec2(-1.0f, 0.0f)))
						{
							if (barLineChangeAtCursor != nullptr && barLineChangeAtCursor->BeatTime == cursorBeat)
								context.Undo.Execute<Commands::RemoveBarLineChange>(&course.BarLineChanges, cursorBeat);
						}
					});
					Gui::Property::PropertyTextValueFunc("Go-Go Time", [&]
					{
						const GoGoRange* gogoRangeAtCursor = course.GoGoRanges.TryFindLastAtBeat(cursorBeat);
						if (gogoRangeAtCursor != nullptr && cursorBeat > gogoRangeAtCursor->GetEnd())
							gogoRangeAtCursor = nullptr;

						const bool hasRangeSelection = (timeline.RangeSelection.IsActive && timeline.RangeSelection.HasEnd);
						Gui::BeginDisabled(!hasRangeSelection);
						if (Gui::Button("Set from Range Selection##GoGoAtCursor", vec2(-1.0f, 0.0f)))
						{
							const Beat rangeSelectionMin = timeline.RoundBeatToCurrentGrid(timeline.RangeSelection.GetMin());
							const Beat rangeSelectionMax = timeline.RoundBeatToCurrentGrid(timeline.RangeSelection.GetMax());
							auto gogoIntersectsSelection = [&](const GoGoRange& gogo) { return (gogo.GetStart() < rangeSelectionMax) && (gogo.GetEnd() > rangeSelectionMin); };

							// TODO: Try to shorten/move intersecting gogo ranges instead of removing them outright
							SortedGoGoRangesList newGoGoRanges = course.GoGoRanges;
							erase_remove_if(newGoGoRanges.Sorted, gogoIntersectsSelection);
							newGoGoRanges.InsertOrUpdate(GoGoRange { rangeSelectionMin, (rangeSelectionMax - rangeSelectionMin) });

							context.Undo.Execute<Commands::AddGoGoRange>(&course.GoGoRanges, std::move(newGoGoRanges));
						}
						Gui::EndDisabled();

						if (Gui::Button("Remove##GoGoAtCursor", vec2(-1.0f, 0.0f)))
						{
							if (gogoRangeAtCursor != nullptr)
								context.Undo.Execute<Commands::RemoveGoGoRange>(&course.GoGoRanges, gogoRangeAtCursor->BeatTime);
						}
					});

					Gui::EndDisabled();
					Gui::Property::EndTable();
				}
			}
		}
		Gui::End();

		if (Gui::Begin("Chart Properties", nullptr, ImGuiWindowFlags_None))
		{
			assert(context.ChartSelectedCourse != nullptr);
			auto& chart = context.Chart;
			auto& course = *context.ChartSelectedCourse;

			if (Gui::CollapsingHeader("Chart", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (Gui::Property::BeginTable(ImGuiTableFlags_BordersInner))
				{
					Gui::Property::PropertyTextValueFunc("Chart Title", [&]
					{
						Gui::SetNextItemWidth(-1.0f);
						if (Gui::InputTextWithHint("##ChartTitle", "n/a", &chart.ChartTitle.Base()))
							context.Undo.NotifyChangesWereMade();
					});
					Gui::Property::PropertyTextValueFunc("Chart Subtitle", [&]
					{
						Gui::SetNextItemWidth(-1.0f);
						if (Gui::InputTextWithHint("##ChartSubtitle", "n/a", &chart.ChartSubtitle.Base()))
							context.Undo.NotifyChangesWereMade();
					});
					Gui::Property::PropertyTextValueFunc("Chart Creator", [&]
					{
						Gui::SetNextItemWidth(-1.0f);
						if (Gui::InputTextWithHint("##ChartCreator", "n/a", &chart.ChartCreator))
							context.Undo.NotifyChangesWereMade();
					});
					Gui::Property::PropertyTextValueFunc("Song File Name", [&]
					{
						const bool songIsLoading = loadSongFuture.valid();
						const char* loadingText = songLoadingTextAnimation.UpdateFrameAndGetText(songIsLoading, Gui::DeltaTime());
						songFileNameInputBuffer = songIsLoading ? "" : chart.SongFileName;

						Gui::BeginDisabled(songIsLoading);
						const auto result = Gui::PathInputTextWithHintAndBrowserDialogButton("##SongFileName",
							songIsLoading ? loadingText : "song.ogg", &songFileNameInputBuffer, songIsLoading ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_EnterReturnsTrue);
						Gui::EndDisabled();

						if (result.InputTextEdited)
							SetAndStartLoadingChartSongFileName(songFileNameInputBuffer, context.Undo);
						else if (result.BrowseButtonClicked)
							OpenLoadAudioFileDialog(context.Undo);
					});
					Gui::Property::PropertyTextValueFunc("Song Volume", [&]
					{
						Gui::SetNextItemWidth(-1.0f);
						if (f32 v = ToPercent(chart.SongVolume); Gui::SliderFloat("##SongVolume", &v, ToPercent(MinVolume), ToPercent(MaxVolumeSoftLimit), "%.0f%%"))
						{
							chart.SongVolume = Clamp(FromPercent(v), MinVolume, MaxVolumeHardLimit);
							context.Undo.NotifyChangesWereMade();
						}
					});
					Gui::Property::PropertyTextValueFunc("Sound Effect Volume", [&]
					{
						Gui::SetNextItemWidth(-1.0f);
						if (f32 v = ToPercent(chart.SoundEffectVolume); Gui::SliderFloat("##SoundEffectVolume", &v, ToPercent(MinVolume), ToPercent(MaxVolumeSoftLimit), "%.0f%%"))
						{
							chart.SoundEffectVolume = Clamp(FromPercent(v), MinVolume, MaxVolumeHardLimit);
							context.Undo.NotifyChangesWereMade();
						}
					});
					Gui::Property::EndTable();
				}
			}

			if (Gui::CollapsingHeader("Selected Course", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (Gui::Property::BeginTable(ImGuiTableFlags_BordersInner))
				{
					Gui::Property::PropertyTextValueFunc("Difficulty Type", [&]
					{
						Gui::SetNextItemWidth(-1.0f);
						if (Gui::ComboEnum("##DifficultyType", &course.Type, DifficultyTypeNames))
							context.Undo.NotifyChangesWereMade();
					});
					Gui::Property::PropertyTextValueFunc("Difficulty Level", [&]
					{
						Gui::SetNextItemWidth(-1.0f);
						if (GuiDifficultyLevelStarSliderWidget("##DifficultyLevel", &course.Level, difficultySliderStarsFitOnScreenLastFrame, difficultySliderStarsWasHoveredLastFrame))
							context.Undo.NotifyChangesWereMade();
					});
#if 0 // TODO:
					Gui::Property::PropertyTextValueFunc("Score Init (TODO)", [&] { Gui::Text("%d", course.ScoreInit); });
					Gui::Property::PropertyTextValueFunc("Score Diff (TODO)", [&] { Gui::Text("%d", course.ScoreDiff); });
#endif
					Gui::Property::EndTable();
				}
			}
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

		// NOTE: Save confirmation popup
		{
			static constexpr const char* saveConfirmationPopupID = "Peepo Drum Kit - Unsaved Changes";
			if (saveConfirmationPopup.OpenOnNextFrame) { Gui::OpenPopup(saveConfirmationPopupID); saveConfirmationPopup.OpenOnNextFrame = false; }

			const ImGuiViewport* mainViewport = Gui::GetMainViewport();
			Gui::SetNextWindowPos(Rect::FromTLSize(mainViewport->Pos, mainViewport->Size).GetCenter(), ImGuiCond_Appearing, vec2(0.5f));

			Gui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4.0f, Gui::GetStyle().ItemSpacing.y });
			Gui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 6.0f, 6.0f });
			bool isPopupOpen = true;
			if (Gui::BeginPopupModal(saveConfirmationPopupID, &isPopupOpen, ImGuiWindowFlags_AlwaysAutoResize))
			{
				Gui::PushFont(FontMedium_EN);
				Gui::BeginChild("SaveConfirmationPopupText", vec2(0.0f, Gui::GetFontSize() * 3.0f), true, ImGuiWindowFlags_NoBackground);
				Gui::AlignTextToFramePadding();
				Gui::Text("Save changes to the current file?");
				Gui::EndChild();
				Gui::PopFont();

				// TODO: Improve button appearance (?)
				//Gui::PushStyleColor(ImGuiCol_Button, Gui::GetStyleColorVec4(ImGuiCol_FrameBg));
				static constexpr vec2 buttonSize = vec2(120.0f, 0.0f);
				const bool clickedYes = Gui::Button("Save Changes", buttonSize) | (Gui::IsWindowFocused() && Gui::IsAnyPressed(GlobalSettings.Input.Dialog_YesOrOk, false));
				Gui::SameLine();
				const bool clickedNo = Gui::Button("Discard Changes", buttonSize) | (Gui::IsWindowFocused() && Gui::IsAnyPressed(GlobalSettings.Input.Dialog_No, false));
				Gui::SameLine();
				const bool clickedCancel = Gui::Button("Cancel", buttonSize) | (Gui::IsWindowFocused() && Gui::IsAnyPressed(GlobalSettings.Input.Dialog_Cancel, false));
				//Gui::PopStyleColor(1);

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
