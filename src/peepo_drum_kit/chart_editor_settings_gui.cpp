#include "chart_editor_settings_gui.h"

namespace PeepoDrumKit
{
	static b8 GuiAssignableInputBindingButton(InputBinding& inOutBinding, vec2 buttonSize, ImGuiButtonFlags buttonFlags, InputBinding*& inOutAwaitInputBinding, CPUStopwatch& inOutAwaitInputStopwatch)
	{
		static constexpr u32 activeTextColor = 0xFF3DEBD2;
		static constexpr Time timeoutThreshold = Time::FromSeconds(4.0);
		static constexpr Time mouseClickThreshold = Time::FromMilliseconds(200.0);
		static constexpr auto animateBlink = [&](Time elapsed)
		{
			constexpr f32 frequency = static_cast<f32>(timeoutThreshold.TotalSeconds() * 1.2);
			constexpr f32 low = 0.25f, high = 1.45f, min = 0.25f, max = 1.00f;
			return Clamp(ConvertRange(-1.0f, +1.0f, low, high, ::sinf(static_cast<f32>(elapsed.TotalSeconds() * frequency) + 1.0f)), min, max);
		};

		auto requestAssignment = [&](InputBinding* inOut)
		{
			inOutAwaitInputBinding = inOut;
			inOutAwaitInputStopwatch.Restart();
		};

		auto finishAsignment = [&](b8 hasNewBinding, InputBinding newBinding = {})
		{
			inOutAwaitInputBinding = nullptr;
			inOutAwaitInputStopwatch.Stop();
			if (hasNewBinding)
				inOutBinding = newBinding;
		};

		const InputBinding inBindingCopy = inOutBinding;
		InputFormatBuffer buffer = ToShortcutString(inOutBinding);

		if (buffer.Data[0] == '\0')
			strcpy_s(buffer.Data, "(None)");
		if (inOutAwaitInputBinding == &inOutBinding)
			strcpy_s(buffer.Data, "[Press any Key]");
		strcat_s(buffer.Data, "###BindingButton");

		const b8 isBeingAsigned = (inOutAwaitInputBinding == &inOutBinding);
		Gui::PushStyleColor(ImGuiCol_Text, isBeingAsigned ? Gui::ColorU32WithAlpha(activeTextColor, animateBlink(inOutAwaitInputStopwatch.GetElapsed())) : Gui::GetColorU32(ImGuiCol_Text));
		Gui::PushStyleColor(ImGuiCol_Button, Gui::GetStyleColorVec4(isBeingAsigned ? ImGuiCol_ButtonHovered : ImGuiCol_Button));
		if (Gui::ButtonEx(buffer.Data, buttonSize, buttonFlags))
		{
			if (inOutAwaitInputBinding != &inOutBinding)
				requestAssignment(&inOutBinding);
		}
		const b8 buttonHovered = Gui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
		Gui::PopStyleColor(2);

		if (inOutAwaitInputBinding == &inOutBinding)
		{
			const b8 timedOut = (inOutAwaitInputStopwatch.GetElapsed() > timeoutThreshold);
			const b8 mouseClickCancelRequest = (inOutAwaitInputStopwatch.GetElapsed() > mouseClickThreshold && !buttonHovered && (Gui::IsMouseClicked(ImGuiMouseButton_Left, false) || Gui::IsMouseClicked(ImGuiMouseButton_Right, false)));

			if (timedOut || mouseClickCancelRequest)
				finishAsignment(false);

			Gui::SetActiveID(Gui::GetID(&inOutBinding), Gui::GetCurrentWindow());

			for (ImGuiKey keyCode = ImGuiKey_NamedKey_BEGIN; keyCode < ImGuiKey_NamedKey_END; keyCode++)
			{
				if (Gui::IsKeyReleased(keyCode))
					finishAsignment(true, KeyBinding(keyCode, Gui::GetIO().KeyMods));
			}
		}

		return (inOutBinding != inBindingCopy);
	}

	b8 ChartSettingsWindow::DrawGui(ChartContext& context, UserSettingsData& settings)
	{
		b8 changesWereMade = false;

		const ImVec2 originalFramePadding = Gui::GetStyle().FramePadding;
		Gui::PushStyleVar(ImGuiStyleVar_FramePadding, GuiScale(vec2(10.0f, 5.0f)));
		Gui::PushStyleColor(ImGuiCol_TabHovered, Gui::GetStyleColorVec4(ImGuiCol_HeaderActive));
		Gui::PushStyleColor(ImGuiCol_TabActive, Gui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
		if (Gui::BeginTabBar("SettingsTabs", ImGuiTabBarFlags_None))
		{
			if (Gui::BeginTabItem("General Settings"))
			{
				Gui::PushStyleVar(ImGuiStyleVar_FramePadding, originalFramePadding);
				changesWereMade |= DrawTabMain(context, settings);
				Gui::PopStyleVar();
				Gui::EndTabItem();
			}

			if (Gui::BeginTabItem("Input Bindings"))
			{
				Gui::PushStyleVar(ImGuiStyleVar_FramePadding, originalFramePadding);
				changesWereMade |= DrawTabInput(context, settings.Input);
				Gui::PopStyleVar();
				Gui::EndTabItem();
			}

			if (Gui::BeginTabItem("Audio Settings"))
			{
				Gui::PushStyleVar(ImGuiStyleVar_FramePadding, originalFramePadding);
				changesWereMade |= DrawTabAudio(context, settings.Audio);
				Gui::PopStyleVar();
				Gui::EndTabItem();
			}

			Gui::EndTabBar();
		}
		Gui::PopStyleColor(2);
		Gui::PopStyleVar();

		return changesWereMade;
	}

	b8 ChartSettingsWindow::DrawTabMain(ChartContext& context, UserSettingsData& settings)
	{
		b8 changesWereMade = false;

		Gui::AlignTextToFramePadding();
		Gui::TextDisabled("// TODO: ...");

		return changesWereMade;
	}

	b8 ChartSettingsWindow::DrawTabInput(ChartContext& context, UserSettingsData::InputData& settings)
	{
		struct InputBindingDesc { WithDefault<MultiInputBinding>* Binding; cstr Name; };
		const InputBindingDesc bindingsTable[] =
		{
			{ &settings.Editor_ToggleFullscreen, "Editor: Toggle Fullscreen", },
			{ &settings.Editor_ToggleVSync, "Editor: Toggle VSync", },
			{ &settings.Editor_IncreaseGuiScale, "Editor: Zoom In", },
			{ &settings.Editor_DecreaseGuiScale, "Editor: Zoom Out", },
			{ &settings.Editor_ResetGuiScale, "Editor: Reset Zoom", },
			{ &settings.Editor_Undo, "Editor: Undo", },
			{ &settings.Editor_Redo, "Editor: Redo", },
			{ &settings.Editor_OpenSettings, "Editor: Open Settings", },
			{ &settings.Editor_ChartNew, "Editor: Chart New", },
			{ &settings.Editor_ChartOpen, "Editor: Chart Open", },
			{ &settings.Editor_ChartOpenDirectory, "Editor: Chart Open Directory", },
			{ &settings.Editor_ChartSave, "Editor: Chart Save", },
			{ &settings.Editor_ChartSaveAs, "Editor: Chart Save As", },
			{ &settings.Timeline_PlaceNoteDon, "Timeline: Place Note Don", },
			{ &settings.Timeline_PlaceNoteKa, "Timeline: Place Note Ka", },
			{ &settings.Timeline_PlaceNoteBalloon, "Timeline: Place Note Balloon", },
			{ &settings.Timeline_PlaceNoteDrumroll, "Timeline: Place Note Drumroll", },
			{ &settings.Timeline_Cut, "Timeline: Cut", },
			{ &settings.Timeline_Copy, "Timeline: Copy", },
			{ &settings.Timeline_Paste, "Timeline: Paste", },
			{ &settings.Timeline_DeleteSelection, "Timeline: Delete Selection", },
			{ &settings.Timeline_StartEndRangeSelection, "Timeline: Start/End Range Selection", },
			{ &settings.Timeline_StepCursorLeft, "Timeline: Step Cursor Left", },
			{ &settings.Timeline_StepCursorRight, "Timeline: Step Cursor Right", },
			{ &settings.Timeline_IncreaseGridDivision, "Timeline: Increase Grid Division", },
			{ &settings.Timeline_DecreaseGridDivision, "Timeline: Decrease Grid Division", },
			{ &settings.Timeline_IncreasePlaybackSpeed, "Timeline: Increase PlaybackSpeed", },
			{ &settings.Timeline_DecreasePlaybackSpeed, "Timeline: Decrease PlaybackSpeed", },
			{ &settings.Timeline_TogglePlayback, "Timeline: Toggle Playback", },
			{ &settings.Timeline_ToggleMetronome, "Timeline: Toggle Metronome", },
			{ &settings.TempoCalculator_Tap, "Tempo Calculator: Tap", },
			{ &settings.TempoCalculator_Reset, "Tempo Calculator: Reset", },
			{ &settings.Dialog_YesOrOk, "Dialog: Yes/Ok", },
			{ &settings.Dialog_No, "Dialog: No", },
			{ &settings.Dialog_Cancel, "Dialog: Cancel", },
			{ &settings.Dialog_SelectNextTab, "Dialog: Select Next Tab", },
			{ &settings.Dialog_SelectPreviousTab, "Dialog: Select Previous Tab", },
		};

		b8 changesWereMade = false;
		const auto& style = Gui::GetStyle();

		// NOTE: Desperate attempt for the table row selectables to not having any spacing between them
		Gui::PushStyleVar(ImGuiStyleVar_CellPadding, { style.CellPadding.x, GuiScale(4.0f) });
		Gui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { style.ItemSpacing.x, GuiScale(8.0f) });
		Gui::PushStyleColor(ImGuiCol_Header, Gui::GetColorU32(ImGuiCol_Header, 0.5f));
		Gui::PushStyleColor(ImGuiCol_HeaderHovered, Gui::GetColorU32(ImGuiCol_HeaderHovered, 0.5f));
		defer { Gui::PopStyleColor(2); Gui::PopStyleVar(2); };

		// TODO: CTRL+F keybinding for all search fields
		// TODO: Also match shortcut strings themselves (and maybe draw in different color on match?)
		Gui::SetNextItemWidth(-1.0f);
		if (Gui::InputTextWithHint("##BindingsFilter", "Type to search...", bindingsFilter.InputBuf, ArrayCount(bindingsFilter.InputBuf)))
			bindingsFilter.Build();

		// TODO: Allow sorting table (?)
		b8 openMultiBindingPopupAtEndOfFrame = false;
		if (Gui::BeginTable("BindingsTable", 3, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, Gui::GetContentRegionAvail()))
		{
			Gui::UpdateSmoothScrollWindow();

			Gui::PushFont(FontMedium_EN);
			Gui::TableSetupScrollFreeze(0, 1);
			Gui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, Gui::CalcTextSize("(Default)").x + Gui::GetFrameHeight());
			Gui::TableSetupColumn("Action", ImGuiTableColumnFlags_None);
			Gui::TableSetupColumn("Binding", ImGuiTableColumnFlags_None);
			Gui::TableHeadersRow();
			Gui::PopFont();

			Gui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, ConvertRangeClampInput(0.0f, 1.0f, 1.0f, style.DisabledAlpha, multiBindingPopupFadeCurrent));
			Gui::BeginDisabled(multiBindingPopupFadeTarget >= 0.001f);

			for (const auto& it : bindingsTable)
			{
				if (!bindingsFilter.PassFilter(it.Name))
					continue;

				Gui::PushID(it.Binding);
				Gui::TableNextRow();

				Gui::TableSetColumnIndex(0);
				(it.Binding->HasValue) ? Gui::TextUnformatted("(User)") : Gui::TextDisabled("(Default)");

				Gui::TableSetColumnIndex(1);

				const b8 clicked = Gui::Selectable(it.Name, false, ImGuiSelectableFlags_SpanAllColumns);
				if (clicked) { selectedMultiBinding = it.Binding; memcpy(&selectedMultiBindingOnOpenCopy, it.Binding, sizeof(selectedMultiBindingOnOpenCopy)); openMultiBindingPopupAtEndOfFrame = true; }

				if (Gui::BeginPopupContextItem("MultiBindingContextMenu"))
				{
					Gui::TextUnformatted(it.Name);
					Gui::Separator();
					if (Gui::MenuItem("Reset to Default", "", nullptr))
					{
						it.Binding->ResetToDefault();
						changesWereMade = true;
					}

					Gui::EndPopup();
				}

				Gui::TableSetColumnIndex(2);
				for (size_t i = 0; i < it.Binding->Value.Count; i++)
				{
					if (i > 0) { Gui::SameLine(0.0f, 0.0f); Gui::Text(" , "); Gui::SameLine(0.0f, 0.0f); }
					const auto& binding = it.Binding->Value.Slots[i];

					Gui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, GuiScale(1.0f));
					Gui::PushStyleColor(ImGuiCol_Border, Gui::GetColorU32(ImGuiCol_ButtonActive));
					{
						Gui::SmallButton(ToShortcutString(binding).Data);
					}
					Gui::PopStyleColor();
					Gui::PopStyleVar();
				}
				Gui::PopID();
			}
			Gui::EndDisabled();
			Gui::PopStyleVar();

			Gui::EndTable();
		}

		Gui::AnimateExponential(&multiBindingPopupFadeCurrent, multiBindingPopupFadeTarget, 20.0f);
		if (openMultiBindingPopupAtEndOfFrame)
		{
			multiBindingPopupFadeCurrent = multiBindingPopupFadeTarget;
			multiBindingPopupFadeTarget = 1.0f;
		}

		if (multiBindingPopupFadeCurrent >= 0.001f)
		{
			cstr selectedBindingName = "Unnamed";
			for (const auto& it : bindingsTable)
				if (it.Binding == selectedMultiBinding)
					selectedBindingName = it.Name;
			assert(selectedMultiBinding != nullptr);

			char popupTitleBuffer[128];
			sprintf_s(popupTitleBuffer, "%s###EditBindingPopup", selectedBindingName);

			const ImGuiWindowFlags allowInputWindowFlag = (multiBindingPopupFadeCurrent < 0.9f) ? ImGuiWindowFlags_NoInputs : ImGuiWindowFlags_None;

			Gui::PushStyleVar(ImGuiStyleVar_Alpha, multiBindingPopupFadeCurrent);
			Gui::PushStyleVar(ImGuiStyleVar_WindowPadding, GuiScale(vec2(12.0f, 8.0f)));
			Gui::PushStyleVar(ImGuiStyleVar_FramePadding, GuiScale(vec2(8.0f, 6.0f)));
			Gui::PushStyleColor(ImGuiCol_Border, Gui::GetStyleColorVec4(ImGuiCol_TableBorderStrong));
			Gui::SetNextWindowPos(Gui::GetMousePos(), ImGuiCond_Appearing, vec2(0.0f, 0.0f));
			Gui::PushFont(FontMedium_EN);
			Gui::Begin(popupTitleBuffer, nullptr, allowInputWindowFlag | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking);
			{
				if ((Gui::IsMouseClicked(ImGuiMouseButton_Left, false) && !Gui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) ||
					(Gui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && Gui::GetActiveID() == 0 && tempAssignedBinding == nullptr && Gui::IsAnyPressed(*Settings.Input.Dialog_Cancel, false)))
				{
					multiBindingPopupFadeTarget = 0.0f;
					assignedBindingStopwatch.Stop();
					tempAssignedBinding = nullptr;
				}

				if (Gui::BeginTable("SelectedBindingTable", 4, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders))
				{
					Gui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, Gui::CalcTextSize("xxx").x * 9.0f + Gui::GetFrameHeight());
					Gui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, Gui::CalcTextSize("xxx").x * 1.0f + Gui::GetFrameHeight());
					Gui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, Gui::CalcTextSize("xxx").x * 1.0f + Gui::GetFrameHeight());
					Gui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, Gui::CalcTextSize("xxx").x * 2.0f + Gui::GetFrameHeight());

					i32 indexToMoveUp = -1, indexToMoveDown = -1, indexToRemove = -1;
					for (i32 i = 0; i < selectedMultiBinding->Value.Count; i++)
					{
						auto& it = selectedMultiBinding->Value.Slots[i];
						Gui::PushID(&it);
						Gui::TableNextRow();
						{
							Gui::TableSetColumnIndex(0);
							if (GuiAssignableInputBindingButton(it, { Gui::GetContentRegionAvail().x, 0.0f }, ImGuiButtonFlags_None, tempAssignedBinding, assignedBindingStopwatch))
							{
								selectedMultiBinding->HasValue = true;
								selectedMultiBinding->HasValue = (selectedMultiBinding->Value != selectedMultiBinding->Default);
							}
						}
						Gui::PushStyleColor(ImGuiCol_Button, Gui::GetStyleColorVec4(ImGuiCol_Header));
						Gui::PushStyleColor(ImGuiCol_ButtonHovered, Gui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
						Gui::PushStyleColor(ImGuiCol_ButtonActive, Gui::GetStyleColorVec4(ImGuiCol_HeaderActive));
						{
							Gui::TableSetColumnIndex(1);
							if (Gui::Button("Up", { Gui::GetContentRegionAvail().x, 0.0f })) { indexToMoveUp = i; }

							Gui::TableSetColumnIndex(2);
							if (Gui::Button("Down", { Gui::GetContentRegionAvail().x, 0.0f })) { indexToMoveDown = i; }

							Gui::TableSetColumnIndex(3);
							if (Gui::Button("Remove", { Gui::GetContentRegionAvail().x, 0.0f })) { indexToRemove = i; }
						}
						Gui::PopStyleColor(3);

						Gui::PopID();
					}

					Gui::TableNextRow();
					{
						Gui::TableSetColumnIndex(0);
						Gui::BeginDisabled(selectedMultiBinding->Value.Count >= MultiInputBinding::MaxCount);
						Gui::PushStyleColor(ImGuiCol_Button, Gui::GetStyleColorVec4(ImGuiCol_Header));
						Gui::PushStyleColor(ImGuiCol_ButtonHovered, Gui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
						Gui::PushStyleColor(ImGuiCol_ButtonActive, Gui::GetStyleColorVec4(ImGuiCol_HeaderActive));
						if (InputBinding newBinding {}; GuiAssignableInputBindingButton(newBinding, { Gui::GetContentRegionAvail().x, 0.0f }, ImGuiButtonFlags_None, tempAssignedBinding, assignedBindingStopwatch))
						{
							selectedMultiBinding->Value.Slots[selectedMultiBinding->Value.Count++] = newBinding;
							selectedMultiBinding->HasValue = true;
							selectedMultiBinding->HasValue = (selectedMultiBinding->Value != selectedMultiBinding->Default);
							changesWereMade = true;
						}
						Gui::PopStyleColor(3);
						Gui::EndDisabled();
					}

					if (indexToMoveUp >= 0 || indexToMoveDown >= 0 || indexToRemove >= 0)
					{
						auto& inOutCount = selectedMultiBinding->Value.Count;
						auto& inOutSlots = selectedMultiBinding->Value.Slots;

						if (indexToMoveUp >= 1 && indexToMoveUp <= inOutCount)
						{
							std::swap(inOutSlots[indexToMoveUp], inOutSlots[indexToMoveUp - 1]);
							selectedMultiBinding->HasValue = true;
							selectedMultiBinding->HasValue = (selectedMultiBinding->Value != selectedMultiBinding->Default);
							changesWereMade = true;
						}
						else if (indexToMoveDown >= 0 && (indexToMoveDown + 1) < inOutCount)
						{
							std::swap(inOutSlots[indexToMoveDown], inOutSlots[indexToMoveDown + 1]);
							selectedMultiBinding->HasValue = true;
							selectedMultiBinding->HasValue = (selectedMultiBinding->Value != selectedMultiBinding->Default);
							changesWereMade = true;
						}
						else if (indexToRemove >= 0)
						{
							selectedMultiBinding->Value.RemoveAt(indexToRemove);
							selectedMultiBinding->HasValue = true;
							selectedMultiBinding->HasValue = (selectedMultiBinding->Value != selectedMultiBinding->Default);
							changesWereMade = true;
						}
					}

					Gui::EndTable();
				}

				Gui::InvisibleButton("##Spacing", vec2(Gui::GetFrameHeight() * 0.25f));

				Gui::PushMultiItemsWidths(2, GuiScale(260.0f));
				{
					const b8 isSameAsOnOpen =
						(selectedMultiBinding->HasValue == selectedMultiBindingOnOpenCopy.HasValue) &&
						(selectedMultiBinding->Value == selectedMultiBindingOnOpenCopy.Value);

					Gui::BeginDisabled(isSameAsOnOpen);
					if (Gui::Button("Revert Changes", { Gui::CalcItemWidth(), 0.0f }))
					{
						selectedMultiBinding->HasValue = selectedMultiBindingOnOpenCopy.HasValue;
						selectedMultiBinding->Value = selectedMultiBindingOnOpenCopy.Value;
						changesWereMade = true;
					}
					Gui::EndDisabled();
					Gui::PopItemWidth();
					Gui::SameLine();

					Gui::BeginDisabled(!selectedMultiBinding->HasValue);
					if (Gui::Button("Reset to Default", { Gui::CalcItemWidth(), 0.0f }))
					{
						selectedMultiBinding->ResetToDefault();
						changesWereMade = true;
					}
					Gui::EndDisabled();
					Gui::PopItemWidth();
				}
			}
			Gui::End();
			Gui::PopFont();
			Gui::PopStyleColor(1);
			Gui::PopStyleVar(3);
		}

		return changesWereMade;
	}

	b8 ChartSettingsWindow::DrawTabAudio(ChartContext& context, UserSettingsData::AudioData& settings)
	{
		b8 changesWereMade = false;

		Gui::AlignTextToFramePadding();
		Gui::TextDisabled("// TODO: ...");

		return changesWereMade;
	}
}
