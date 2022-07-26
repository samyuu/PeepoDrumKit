#include "chart_editor_widgets.h"
#include "chart_undo_commands.h"
#include "chart_draw_common.h"
#include "main_settings.h"

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

		const bool starsFitOnScreen = (starSize.x >= Gui::GetFrameHeight()) && !Gui::IsItemBeingEditedAsText();

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
}

namespace PeepoDrumKit
{
	// NOTE: Soft clamp for sliders but hard limit to allow *typing in* values higher, even if it can cause clipping
	static constexpr f32 MinVolume = 0.0f;
	static constexpr f32 MaxVolumeSoftLimit = 1.0f;
	static constexpr f32 MaxVolumeHardLimit = 4.0f;

	// TODO: Maybe this should be a user setting (?)
	static constexpr f32 MinBPM = 30.0f;
	static constexpr f32 MaxBPM = 960.0f;

	const char* LoadingTextAnimation::UpdateFrameAndGetText(bool isLoadingThisFrame, f32 deltaTimeSec)
	{
		static constexpr const char* TextFrames[] = { "Loading o....", "Loading .o...", "Loading ..o..", "Loading ...o.", "Loading ....o", "Loading ...o.", "Loading ..o..", "Loading .o..." };
		static constexpr f32 FrameIntervalSec = (1.0f / 12.0f);

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

	void ChartHelpWindow::DrawGui(ChartContext& context)
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
					row([] { Gui::Text("Play / pause"); }, [] { Gui::Text(ToShortcutString(*Settings.Input.Timeline_TogglePlayback).Data); });
					row([] { Gui::Text("Add / remove note"); }, []
					{
						assert(Settings.Input.Timeline_PlaceNoteKa->Count == 2 && Settings.Input.Timeline_PlaceNoteDon->Count == 2);
						Gui::Text("%s / %s / %s / %s",
							ToShortcutString(Settings.Input.Timeline_PlaceNoteKa->Slots[0]).Data, ToShortcutString(Settings.Input.Timeline_PlaceNoteDon->Slots[0]).Data,
							ToShortcutString(Settings.Input.Timeline_PlaceNoteDon->Slots[1]).Data, ToShortcutString(Settings.Input.Timeline_PlaceNoteKa->Slots[1]).Data);
					});
					row([] { Gui::Text("Add long note"); }, []
					{
						assert(Settings.Input.Timeline_PlaceNoteBalloon->Count == 2 && Settings.Input.Timeline_PlaceNoteDrumroll->Count == 2);
						Gui::Text("%s / %s / %s / %s",
							ToShortcutString(Settings.Input.Timeline_PlaceNoteBalloon->Slots[0]).Data, ToShortcutString(Settings.Input.Timeline_PlaceNoteDrumroll->Slots[0]).Data,
							ToShortcutString(Settings.Input.Timeline_PlaceNoteDrumroll->Slots[1]).Data, ToShortcutString(Settings.Input.Timeline_PlaceNoteBalloon->Slots[1]).Data);
					});
					row([] { Gui::Text("Add big note"); }, [] { Gui::Text("Alt + { Note }"); });
					row([] { Gui::Text("Fill range-selection with notes"); }, [] { Gui::Text("Shift + { Note }"); });
					row([] { Gui::Text("Start / end range-selection"); }, [] { Gui::Text(ToShortcutString(*Settings.Input.Timeline_StartEndRangeSelection).Data); });
					row([] { Gui::Text("Grid division"); }, [] { Gui::Text("Mouse [X1/X2] / [%s/%s]", ToShortcutString(*Settings.Input.Timeline_IncreaseGridDivision).Data, ToShortcutString(*Settings.Input.Timeline_DecreaseGridDivision).Data); });
					row([] { Gui::Text("Step cursor"); }, [] { Gui::Text("%s / %s", ToShortcutString(*Settings.Input.Timeline_StepCursorLeft).Data, ToShortcutString(*Settings.Input.Timeline_StepCursorRight).Data); });
					rowSeparator();
					row([] { Gui::Text("Move selected notes"); }, [] { Gui::Text("Mouse Left (Hover)"); });
					row([] { Gui::Text("Cut selected notes"); }, [] { Gui::Text(ToShortcutString(*Settings.Input.Timeline_Cut).Data); });
					row([] { Gui::Text("Copy selected notes"); }, [] { Gui::Text(ToShortcutString(*Settings.Input.Timeline_Copy).Data); });
					row([] { Gui::Text("Paste selected notes"); }, [] { Gui::Text(ToShortcutString(*Settings.Input.Timeline_Paste).Data); });
					row([] { Gui::Text("Delete selected notes"); }, [] { Gui::Text(ToShortcutString(*Settings.Input.Timeline_DeleteSelection).Data); });
					rowSeparator();
					row([] { Gui::Text("Playback speed"); }, [] { Gui::Text("%s / %s", ToShortcutString(*Settings.Input.Timeline_DecreasePlaybackSpeed).Data, ToShortcutString(*Settings.Input.Timeline_IncreasePlaybackSpeed).Data); });
					row([] { Gui::Text("Toggle metronome"); }, [] { Gui::Text(ToShortcutString(*Settings.Input.Timeline_ToggleMetronome).Data); });

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

	void ChartUndoHistoryWindow::DrawGui(ChartContext& context)
	{
		const auto& undoStack = context.Undo.UndoStack;
		const auto& redoStack = context.Undo.RedoStack;
		i32 undoClickedIndex = -1, redoClickedIndex = -1;

		const auto& style = Gui::GetStyle();
		// NOTE: Desperate attempt for the table row selectables to not having any spacing between them
		Gui::PushStyleVar(ImGuiStyleVar_CellPadding, { style.CellPadding.x, GuiScale(4.0f) });
		Gui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { style.ItemSpacing.x, GuiScale(8.0f) });
		Gui::PushStyleColor(ImGuiCol_Header, Gui::GetColorU32(ImGuiCol_Header, 0.5f));
		Gui::PushStyleColor(ImGuiCol_HeaderHovered, Gui::GetColorU32(ImGuiCol_HeaderHovered, 0.5f));
		defer { Gui::PopStyleColor(2); Gui::PopStyleVar(2); };

		if (Gui::BeginTable("UndoHistoryTable", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, Gui::GetContentRegionAvail()))
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
				context.Undo.Undo(undoStack.size());

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
			context.Undo.Undo(undoStack.size() - undoClickedIndex - 1);
		else if (InBounds(redoClickedIndex, redoStack))
			context.Undo.Redo(redoStack.size() - redoClickedIndex);
	}

	void TempoCalculatorWindow::DrawGui(ChartContext& context)
	{
		Gui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		Gui::PushStyleColor(ImGuiCol_Button, Gui::GetStyleColorVec4(ImGuiCol_Header));
		Gui::PushStyleColor(ImGuiCol_ButtonHovered, Gui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
		Gui::PushStyleColor(ImGuiCol_ButtonActive, Gui::GetStyleColorVec4(ImGuiCol_HeaderActive));
		{
			const bool windowFocused = Gui::IsWindowFocused() && (Gui::GetActiveID() == 0);
			const bool tapPressed = windowFocused && Gui::IsAnyPressed(*Settings.Input.TempoCalculator_Tap, false);
			const bool resetPressed = windowFocused && Gui::IsAnyPressed(*Settings.Input.TempoCalculator_Reset, false);
			const bool resetDown = windowFocused && Gui::IsAnyDown(*Settings.Input.TempoCalculator_Reset);

			Gui::PushFont(FontLarge_EN);
			{
				const Time lastBeatDuration = Time::FromSeconds(60.0 / Round(Calculator.LastTempo.BPM));
				const f32 tapBeatLerpT = ImSaturate((Calculator.TapCount == 0) ? 1.0f : static_cast<f32>(Calculator.LastTap.GetElapsed() / lastBeatDuration));
				const ImVec4 animatedButtonColor = ImLerp(Gui::GetStyleColorVec4(ImGuiCol_ButtonActive), Gui::GetStyleColorVec4(ImGuiCol_Button), tapBeatLerpT);

				const bool hasTimedOut = Calculator.HasTimedOut();
				Gui::PushStyleColor(ImGuiCol_Text, Gui::GetColorU32(ImGuiCol_Text, (Calculator.TapCount > 0 && hasTimedOut) ? 0.8f : 1.0f));
				Gui::PushStyleColor(ImGuiCol_Button, animatedButtonColor);
				Gui::PushStyleColor(ImGuiCol_ButtonHovered, (Calculator.TapCount > 0 && !hasTimedOut) ? animatedButtonColor : Gui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
				Gui::PushStyleColor(ImGuiCol_ButtonActive, (Calculator.TapCount > 0 && !hasTimedOut) ? animatedButtonColor : Gui::GetStyleColorVec4(ImGuiCol_ButtonActive));

				char buttonName[32]; sprintf_s(buttonName, (Calculator.TapCount == 0) ? "Tap" : (Calculator.TapCount == 1) ? "First Beat" : "%.2f BPM", Calculator.LastTempo.BPM);
				if (tapPressed | Gui::ButtonEx(buttonName, vec2(-1.0f, Gui::GetFrameHeightWithSpacing() * 3.0f), ImGuiButtonFlags_PressedOnClick))
				{
					context.SfxVoicePool.PlaySound(SoundEffectType::MetronomeBeat);
					Calculator.Tap();
				}

				Gui::PopStyleColor(4);
			}
			Gui::PopFont();

			Gui::PushStyleColor(ImGuiCol_Text, Gui::GetStyleColorVec4((Calculator.TapCount > 0) ? ImGuiCol_Text : ImGuiCol_TextDisabled));
			Gui::PushStyleColor(ImGuiCol_Button, Gui::GetStyleColorVec4(resetDown ? ImGuiCol_ButtonActive : ImGuiCol_Button));
			if (resetPressed | Gui::Button("Reset", vec2(-1.0f, Gui::GetFrameHeightWithSpacing() * 1.0f)))
			{
				if (Calculator.TapCount > 0)
					context.SfxVoicePool.PlaySound(SoundEffectType::MetronomeBar);
				Calculator.Reset();
			}
			Gui::PopStyleColor(2);
		}
		Gui::PopStyleColor(3);
		Gui::PopStyleVar(1);

		Gui::PushFont(FontMedium_EN);
		if (Gui::BeginTable("Table", 2, ImGuiTableFlags_BordersInner | ImGuiTableFlags_NoSavedSettings, Gui::GetContentRegionAvail()))
		{
			const char* formatStrBPM_g = (Calculator.TapCount > 1) ? "%g BPM" : "--.-- BPM";
			const char* formatStrBPM_2f = (Calculator.TapCount > 1) ? "%.2f BPM" : "--.-- BPM";
			static constexpr auto row = [&](auto funcLeft, auto funcRight)
			{
				Gui::TableNextRow();
				Gui::TableSetColumnIndex(0); Gui::AlignTextToFramePadding(); funcLeft();
				Gui::TableSetColumnIndex(1); Gui::AlignTextToFramePadding(); Gui::SetNextItemWidth(-1.0f); funcRight();
			};
			row([&] { Gui::TextUnformatted("Nearest Whole"); }, [&]
			{
				f32 v = Round(Calculator.LastTempo.BPM);
				Gui::InputFloat("##NearestWhole", &v, 0.0f, 0.0f, formatStrBPM_g, ImGuiInputTextFlags_ReadOnly);
			});
			row([&] { Gui::TextUnformatted("Nearest"); }, [&]
			{
				f32 v = Calculator.LastTempo.BPM;
				Gui::InputFloat("##Nearest", &v, 0.0f, 0.0f, formatStrBPM_2f, ImGuiInputTextFlags_ReadOnly);
			});
			row([&] { Gui::TextUnformatted("Min and Max"); }, [&]
			{
				f32 v[2] = { Calculator.LastTempoMin.BPM, Calculator.LastTempoMax.BPM };
				Gui::InputFloat2("##MinMax", v, formatStrBPM_2f, ImGuiInputTextFlags_ReadOnly);
			});
			row([&] { Gui::TextUnformatted("Timing Taps"); }, [&]
			{
				Gui::Text((Calculator.TapCount == 1) ? "First Beat" : "%d Taps", Calculator.TapCount);
			});
			// NOTE: ReadOnly InputFloats specifically to allow easy copy and paste
			Gui::EndTable();
		}
		Gui::PopFont();
	}

	void ChartPropertiesWindow::DrawGui(ChartContext& context, const ChartPropertiesWindowIn& in, ChartPropertiesWindowOut& out)
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
					const bool songIsLoading = in.IsSongAsyncLoading;
					const char* loadingText = SongLoadingTextAnimation.UpdateFrameAndGetText(songIsLoading, Gui::DeltaTime());
					SongFileNameInputBuffer = songIsLoading ? "" : chart.SongFileName;

					Gui::BeginDisabled(songIsLoading);
					const auto result = Gui::PathInputTextWithHintAndBrowserDialogButton("##SongFileName",
						songIsLoading ? loadingText : "song.ogg", &SongFileNameInputBuffer, songIsLoading ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_EnterReturnsTrue);
					Gui::EndDisabled();

					if (result.InputTextEdited)
					{
						out.LoadNewSong = true;
						out.NewSongFilePath = SongFileNameInputBuffer;
					}
					else if (result.BrowseButtonClicked)
					{
						out.BrowseOpenSong = true;
					}
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
					if (GuiDifficultyLevelStarSliderWidget("##DifficultyLevel", &course.Level, DifficultySliderStarsFitOnScreenLastFrame, DifficultySliderStarsWasHoveredLastFrame))
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

	void ChartTempoWindow::DrawGui(ChartContext& context, ChartTimeline& timeline)
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

				const TempoChange* tempoChangeAtCursor = course.TempoMap.TempoTryFindLastAtBeat(cursorBeat);
				const Tempo tempoAtCursor = (tempoChangeAtCursor != nullptr) ? tempoChangeAtCursor->Tempo : FallbackTempo;

				auto insertOrUpdateCursorTempoChange = [&](Tempo newTempo)
				{
					if (tempoChangeAtCursor == nullptr || tempoChangeAtCursor->Beat != cursorBeat)
						context.Undo.Execute<Commands::AddTempoChange>(&course.TempoMap, TempoChange(cursorBeat, newTempo));
					else
						context.Undo.Execute<Commands::UpdateTempoChange>(&course.TempoMap, TempoChange(cursorBeat, newTempo));
				};

				Gui::Property::Property([&]
				{
					Gui::SetNextItemWidth(-1.0f);
					if (f32 v = tempoAtCursor.BPM; GuiDragFloatLabel("Tempo##DragFloatLabel", &v, 1.0f, MinBPM, MaxBPM, "%g", ImGuiSliderFlags_AlwaysClamp))
						insertOrUpdateCursorTempoChange(Tempo(v));
				});
				Gui::Property::Value([&]
				{
					Gui::SetNextItemWidth(-1.0f);
					if (f32 v = tempoAtCursor.BPM; Gui::InputFloat("##TempoAtCursor", &v, 1.0f, 10.0f, "%g BPM", ImGuiInputTextFlags_None))
						insertOrUpdateCursorTempoChange(Tempo(Clamp(v, MinBPM, MaxBPM)));

					if (Gui::Button("Remove##TempoAtCursor", vec2(-1.0f, 0.0f)))
					{
						if (tempoChangeAtCursor != nullptr && tempoChangeAtCursor->Beat == cursorBeat)
							context.Undo.Execute<Commands::RemoveTempoChange>(&course.TempoMap, cursorBeat);
					}
				});
				Gui::Property::PropertyTextValueFunc("Time Signature", [&]
				{
					const TimeSignatureChange* signatureChangeAtCursor = course.TempoMap.SignatureTryFindLastAtBeat(cursorBeat);
					const TimeSignature signatureAtCursor = (signatureChangeAtCursor != nullptr) ? signatureChangeAtCursor->Signature : FallbackTimeSignature;

					Gui::SetNextItemWidth(-1.0f);
					if (ivec2 v = { signatureAtCursor.Numerator, signatureAtCursor.Denominator };
						GuiInputFraction("##SignatureAtCursor", &v, ivec2(1, Beat::TicksPerBeat * 4)))
					{
						// TODO: Also floor cursor beat to ~~last~~ next whole bar
						if (signatureChangeAtCursor == nullptr || signatureChangeAtCursor->Beat != cursorBeat)
							context.Undo.Execute<Commands::AddTimeSignatureChange>(&course.TempoMap, TimeSignatureChange(cursorBeat, TimeSignature(v[0], v[1])));
						else
							context.Undo.Execute<Commands::UpdateTimeSignatureChange>(&course.TempoMap, TimeSignatureChange(cursorBeat, TimeSignature(v[0], v[1])));
					}

					if (Gui::Button("Remove##SignatureAtCursor", vec2(-1.0f, 0.0f)))
					{
						if (signatureChangeAtCursor != nullptr && signatureChangeAtCursor->Beat == cursorBeat)
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
							if (f32 v = tempoAtCursor.BPM * ((scrollChangeChangeAtCursor == nullptr) ? 1.0f : scrollChangeChangeAtCursor->ScrollSpeed);
								Gui::DragFloat("##ScrollTempoAtCursor", &v, 1.0f, MinBPM, MaxBPM, "%g BPM", ImGuiSliderFlags_NoRoundToFormat))
								newScrollSpeed = (tempoAtCursor.BPM == 0.0f) ? 0.0f : (v / tempoAtCursor.BPM);
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
					const GoGoRange* gogoRangeAtCursor = course.GoGoRanges.TryFindOverlappingBeat(cursorBeat);

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

	void ChartLyricsWindow::DrawGui(ChartContext& context, ChartTimeline& timeline)
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
		LyricInputBuffer.clear();
		if (lyricChangeAtCursor != nullptr)
			ResolveEscapeSequences(lyricChangeAtCursor->Lyric, LyricInputBuffer, EscapeSequenceFlags::NewLines);

		const f32 textInputHeight = ClampBot(Gui::GetContentRegionAvail().y - Gui::GetFrameHeightWithSpacing(), Gui::GetFrameHeightWithSpacing());
		Gui::SetNextItemWidth(-1.0f);
		if (Gui::InputTextMultilineWithHint("##LyricAtCursor", "n/a", &LyricInputBuffer, { -1.0f, textInputHeight }, ImGuiInputTextFlags_None))
		{
			std::string newLyricLine;
			ConvertToEscapeSequences(LyricInputBuffer, newLyricLine, EscapeSequenceFlags::NewLines);

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
}
