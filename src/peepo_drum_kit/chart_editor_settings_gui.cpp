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

	namespace SettingsGui
	{
		enum class DataType : u32
		{
			Invalid,
			I32,
			F32,
			StdString,
		};

		enum class WidgetType : u32
		{
			Default,
			I32_BarDivisionComboBox,
			F32_ExponentialSpeed,
		};

		template <typename T> constexpr DataType TemplateToDataType() = delete;
		template <> constexpr DataType TemplateToDataType<i32>() { return DataType::I32; }
		template <> constexpr DataType TemplateToDataType<f32>() { return DataType::F32; }
		template <> constexpr DataType TemplateToDataType<std::string>() { return DataType::StdString; }

		struct Entry
		{
			DataType DataType;
			WidgetType Widget;
			void* ValuePtr;
			b8* HasValuePtr;
			std::string_view Header;
			std::string_view Description;
			void(*ResetToDefaultFunc)(void* valuePtr);
			void(*SetHasValueIfNotDefaultFunc)(void* valuePtr);

			inline void ResetToDefault() { ResetToDefaultFunc(ValuePtr); }
			inline void SetHasValueIfNotDefault() { SetHasValueIfNotDefaultFunc(ValuePtr); }

			template <typename T>
			inline WithDefault<T>* ValueAs() { return (DataType == TemplateToDataType<T>()) ? static_cast<WithDefault<T>*>(ValuePtr) : nullptr; }
		};

		template <typename T>
		static Entry MakeEntry(WithDefault<T>& v, std::string_view header, std::string_view description, WidgetType widgetType = WidgetType::Default)
		{
			return Entry
			{
				TemplateToDataType<T>(), widgetType, &v, &v.HasValue, header, description,
				[](void* valuePtr) { static_cast<WithDefault<T>*>(valuePtr)->ResetToDefault(); },
				[](void* valuePtr) { static_cast<WithDefault<T>*>(valuePtr)->SetHasValueIfNotDefault(); },
			};
		}

		static void GuiRightAlignedTextDisabled(std::string_view text)
		{
			Gui::SetCursorScreenPos(vec2(Gui::GetCursorScreenPos()) + vec2(Gui::GetContentRegionAvail().x - Gui::CalcTextSize(text).x, 0.0f));
			Gui::PushStyleColor(ImGuiCol_Text, Gui::GetStyleColorVec4(ImGuiCol_TextDisabled));
			Gui::TextUnformatted(text.data());
			Gui::PopStyleColor();
		};

		static b8 DrawGui(Entry& in)
		{
			auto addGroupPadding = [](f32 padding) { Gui::InvisibleButton("", vec2(1.0f, 1.0f + padding)); };
			const auto& style = Gui::GetStyle();

			assert(in.DataType != DataType::Invalid && !in.Header.empty() && in.ValuePtr != nullptr);
			b8 changesWereMade = false;

			Gui::BeginGroup();
			addGroupPadding(GuiScale(2.0f));
			{
				Gui::AlignTextToFramePadding();
				Gui::PushFont(FontMedium_EN);
				Gui::TextUnformatted(in.Header);
				Gui::PopFont();

				if (!in.Description.empty())
				{
					assert(in.Description.back() == '.');
					Gui::PushStyleColor(ImGuiCol_Text, Gui::GetColorU32(ImGuiCol_Text, 0.8f));
					Gui::TextWrapped(in.Description.data());
					Gui::PopStyleColor();
				}

				auto* inOutI32 = in.ValueAs<i32>();
				auto* inOutF32 = in.ValueAs<f32>();
				auto* inOutStr = in.ValueAs<std::string>();

				if (inOutI32 != nullptr)
				{
					if (in.Widget == WidgetType::I32_BarDivisionComboBox)
					{
						char buffer[64]; sprintf_s(buffer, "Bar Division 1/%d", inOutI32->Value);
						if (Gui::BeginCombo("##", buffer, ImGuiComboFlags_None))
						{
							static constexpr i32 barDivisions[] = { 4, 8, 12, 16, 24, 32, 48, 64 };
							for (const i32 it : barDivisions)
							{
								sprintf_s(buffer, "Bar Division 1/%d", it);
								const b8 isSelected = (it == inOutI32->Value);
								if (Gui::Selectable(buffer, isSelected)) { inOutI32->Value = it; changesWereMade = true; }
								if (isSelected) { Gui::SetItemDefaultFocus(); }
								if (it == inOutI32->Default) { Gui::SameLine(); GuiRightAlignedTextDisabled("(Default)"); }
							}
							Gui::EndCombo();
						}
					}
					else
					{
						changesWereMade |= Gui::InputInt("##", &inOutI32->Value, 1, 10);
					}
				}
				else if (inOutF32 != nullptr)
				{
					if (in.Widget == WidgetType::F32_ExponentialSpeed)
					{
						const b8 isDisabled = (inOutF32->Value <= 0.0f);
						Gui::PushStyleColor(ImGuiCol_Text, Gui::GetStyleColorVec4(isDisabled ? ImGuiCol_TextDisabled : ImGuiCol_Text));
						Gui::SetNextItemWidth(Max(1.0f, Gui::CalcItemWidth() - ((Gui::GetFrameHeight() + style.ItemInnerSpacing.x) * 3)));
						changesWereMade |= Gui::SliderFloat("##", &inOutF32->Value, 0.0f, 50.0f, isDisabled ? "(Disabled)" : "%.0f", ImGuiSliderFlags_None);
						Gui::PopStyleColor();
						Gui::SameLine(0, style.ItemInnerSpacing.x);
						changesWereMade |= Gui::ButtonPlusMinusFloat('-', &inOutF32->Value, 1.0f, 10.0f);
						Gui::SameLine(0, style.ItemInnerSpacing.x);
						changesWereMade |= Gui::ButtonPlusMinusFloat('+', &inOutF32->Value, 1.0f, 10.0f);
						Gui::SameLine(0.0f, style.ItemInnerSpacing.x);
						if (Gui::BeginCombo("##Combo", "", ImGuiComboFlags_NoPreview | ImGuiComboFlags_PopupAlignLeft))
						{
							if (Gui::Selectable("Reset to Default", false, !inOutF32->HasValue ? ImGuiSelectableFlags_Disabled : 0)) { in.ResetToDefault(); changesWereMade = true; }
							if (Gui::Selectable("Disable Animation", false, (inOutF32->Value == 0.0f) ? ImGuiSelectableFlags_Disabled : 0)) { inOutF32->Value = 0.0f; changesWereMade = true; }
							Gui::EndCombo();
						}
						if (changesWereMade) { inOutF32->Value = ClampBot(inOutF32->Value, 0.0f); }
					}
					else
					{
						changesWereMade |= Gui::InputFloat("##", &inOutF32->Value, 1.0f, 10.0f);
					}
				}
				else if (inOutStr != nullptr)
				{
					changesWereMade |= Gui::InputTextWithHint("##", "n/a", &inOutStr->Value);
				}

				if (changesWereMade)
					in.SetHasValueIfNotDefault();
			}
			addGroupPadding(GuiScale(2.0f));
			Gui::EndGroup();

			return changesWereMade;
		}
	}

	constexpr size_t SizeOfUserSettingsData = sizeof(UserSettingsData);
	static_assert(PEEPO_RELEASE || SizeOfUserSettingsData == 13896, "TODO: Add missing settings entries for newly added UserSettingsData fields");

	b8 ChartSettingsWindow::DrawTabMain(ChartContext& context, UserSettingsData& settings)
	{
		SettingsGui::Entry settingsEntries[] =
		{
			SettingsGui::MakeEntry(settings.General.DefaultCreatorName, "Default Creator Name", "The name that is automatically filled in when creating a new chart."),

			SettingsGui::MakeEntry(
				settings.General.DrumrollAutoHitBarDivision,
				"Drumroll Preview Interval",
				"The interval at which drumrolls have their hit sounds previewed at.",
				SettingsGui::WidgetType::I32_BarDivisionComboBox),

			// TODO: Remove..?
			//SettingsGui::MakeEntry(settings.General.TimelineScrubAutoScrollPixelThreshold, "Timeline Scrub Auto Scroll Pixel Threshold", ""),
			//SettingsGui::MakeEntry(settings.General.TimelineScrubAutoScrollSpeedMin, "Timeline Scrub Auto Scroll Speed Min", ""),
			//SettingsGui::MakeEntry(settings.General.TimelineScrubAutoScrollSpeedMax, "Timeline Scrub Auto Scroll Speed Max", ""),

			SettingsGui::MakeEntry(settings.Animation.TimelineSmoothScrollSpeed,
				"Timeline Smooth Scroll Animation Speed",
				"The animation speed when scrolling the timeline.",
				SettingsGui::WidgetType::F32_ExponentialSpeed),

			SettingsGui::MakeEntry(settings.Animation.TimelineWorldSpaceCursorXSpeed,
				"Timeline Cursor Animation Speed",
				"The animation speed for the timeline cursor when left clicking on a new position.",
				SettingsGui::WidgetType::F32_ExponentialSpeed),

			SettingsGui::MakeEntry(settings.Animation.TimelineRangeSelectionExpansionSpeed,
				"Timeline Range Selection Animation Speed",
				"The expansion animation speed for the timeline range selection region.",
				SettingsGui::WidgetType::F32_ExponentialSpeed),
		};

		b8 changesWereMade = false;
		const auto& style = Gui::GetStyle();

		Gui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { style.ItemSpacing.x, GuiScale(8.0f) });
		Gui::SetNextItemWidth(-1.0f);
		if (Gui::InputTextWithHint("##MainFilter", "Type to search...", mainSettingsFilter.InputBuf, ArrayCount(mainSettingsFilter.InputBuf)))
			mainSettingsFilter.Build();

		Gui::PushStyleVar(ImGuiStyleVar_CellPadding, GuiScale(vec2(8.0f, 4.0f)));
		const bool beginTable = Gui::BeginTable("InnerTable", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_ScrollY, Gui::GetContentRegionAvail());
		Gui::PopStyleVar(2);
		if (beginTable)
		{
			Gui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, Gui::CalcTextSize("(Default)").x + Gui::GetFrameHeight());

			Gui::UpdateSmoothScrollWindow();
			Gui::PushStyleVar(ImGuiStyleVar_ItemSpacing, GuiScale(vec2(8.0f, 8.0f)));

			b8 isAnyItemGroupHoveredOrActive = false;
			for (auto& entry : settingsEntries)
			{
				if (!mainSettingsFilter.PassFilter(Gui::StringViewStart(entry.Header), Gui::StringViewEnd(entry.Header)) &&
					!mainSettingsFilter.PassFilter(Gui::StringViewStart(entry.Description), Gui::StringViewEnd(entry.Description)))
					continue;

				Gui::TableNextRow();
				Gui::PushID(entry.ValuePtr);

				Gui::TableSetColumnIndex(0);
				Gui::AlignTextToFramePadding();
				(*entry.HasValuePtr) ? Gui::TextUnformatted("(User)") : Gui::TextDisabled("(Default)");

				Gui::TableSetColumnIndex(1);

				if (lastActiveGroup.ValuePtr == entry.ValuePtr)
				{
					const vec2 padding = { GuiScale(12.0f), 0.0f };
					Gui::GetWindowDrawList()->AddRectFilled(
						lastActiveGroup.GroupRect.TL + vec2(-padding.x, 0.0f),
						lastActiveGroup.GroupRect.BR + vec2(+padding.x, +padding.y),
						Gui::GetColorU32(ImGuiCol_ButtonHovered, 0.15f));
				}

				if (SettingsGui::DrawGui(entry))
					changesWereMade = true;

				const Rect itemGroupRect = Gui::GetItemRect();
				const b8 isItemGroupHovered = Gui::IsItemHovered();
				b8 isPopupOpen = false;

				if (isPopupOpen = Gui::BeginPopupContextItem("SettingsEntryContextMenu"))
				{
					Gui::TextUnformatted(entry.Header);
					Gui::Separator();
					if (Gui::MenuItem("Reset to Default", "", nullptr))
					{
						entry.ResetToDefault();
						changesWereMade = true;
					}

					Gui::EndPopup();
				}

				if (isItemGroupHovered || isPopupOpen)
				{
					isAnyItemGroupHoveredOrActive = true;
					lastActiveGroup = { entry.ValuePtr, itemGroupRect };
				}

				Gui::PopID();

				if (ArrayItToIndex(&entry, &settingsEntries[0]) + 1 < ArrayCount(settingsEntries))
					Gui::Separator();
			}

			if (!isAnyItemGroupHoveredOrActive)
				lastActiveGroup = {};

			Gui::PopStyleVar(1);
			Gui::EndTable();
		}

		return changesWereMade;
	}

	constexpr size_t SizeOfUserSettingsInputData = sizeof(UserSettingsData::InputData);
	static_assert(PEEPO_RELEASE || SizeOfUserSettingsInputData == 13668, "TODO: Add missing table entries for newly added UserSettingsData::InputData bindings");

	b8 ChartSettingsWindow::DrawTabInput(ChartContext& context, UserSettingsData::InputData& settings)
	{
		struct InputBindingDesc { WithDefault<MultiInputBinding>* Binding; std::string_view Name; };
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
			{},
			{ &settings.Timeline_PlaceNoteDon, "Timeline: Place Note Don", },
			{ &settings.Timeline_PlaceNoteKa, "Timeline: Place Note Ka", },
			{ &settings.Timeline_PlaceNoteBalloon, "Timeline: Place Note Balloon", },
			{ &settings.Timeline_PlaceNoteDrumroll, "Timeline: Place Note Drumroll", },
			{ &settings.Timeline_FlipNoteType, "Timeline: Flip Note Type", },
			{ &settings.Timeline_ToggleNoteSize, "Timeline: Toggle Note Size", },
			{ &settings.Timeline_Cut, "Timeline: Cut", },
			{ &settings.Timeline_Copy, "Timeline: Copy", },
			{ &settings.Timeline_Paste, "Timeline: Paste", },
			{ &settings.Timeline_DeleteSelection, "Timeline: Delete Selection", },
			{ &settings.Timeline_StartEndRangeSelection, "Timeline: Start/End Range Selection", },
			{ &settings.Timeline_StepCursorLeft, "Timeline: Step Cursor Left", },
			{ &settings.Timeline_StepCursorRight, "Timeline: Step Cursor Right", },
			{ &settings.Timeline_JumpToTimelineStart, "Timeline: Jump to Timeline Start", },
			{ &settings.Timeline_JumpToTimelineEnd, "Timeline: Jump to Timeline End", },
			{ &settings.Timeline_IncreaseGridDivision, "Timeline: Increase Grid Division", },
			{ &settings.Timeline_DecreaseGridDivision, "Timeline: Decrease Grid Division", },
			{ &settings.Timeline_SetGridDivision_1_4, "Timeline: Set Grid Division 1 / 4", },
			{ &settings.Timeline_SetGridDivision_1_8, "Timeline: Set Grid Division 1 / 8", },
			{ &settings.Timeline_SetGridDivision_1_12, "Timeline: Set Grid Division 1 / 12", },
			{ &settings.Timeline_SetGridDivision_1_16, "Timeline: Set Grid Division 1 / 16", },
			{ &settings.Timeline_SetGridDivision_1_24, "Timeline: Set Grid Division 1 / 24", },
			{ &settings.Timeline_SetGridDivision_1_32, "Timeline: Set Grid Division 1 / 32", },
			{ &settings.Timeline_SetGridDivision_1_48, "Timeline: Set Grid Division 1 / 48", },
			{ &settings.Timeline_SetGridDivision_1_64, "Timeline: Set Grid Division 1 / 64", },
			{ &settings.Timeline_SetGridDivision_1_96, "Timeline: Set Grid Division 1 / 96", },
			{ &settings.Timeline_SetGridDivision_1_192, "Timeline: Set Grid Division 1 / 192", },
			{ &settings.Timeline_IncreasePlaybackSpeed, "Timeline: Increase PlaybackSpeed", },
			{ &settings.Timeline_DecreasePlaybackSpeed, "Timeline: Decrease PlaybackSpeed", },
			{ &settings.Timeline_TogglePlayback, "Timeline: Toggle Playback", },
			{ &settings.Timeline_ToggleMetronome, "Timeline: Toggle Metronome", },
			{},
			{ &settings.TempoCalculator_Tap, "Tempo Calculator: Tap", },
			{ &settings.TempoCalculator_Reset, "Tempo Calculator: Reset", },
			{},
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
		if (Gui::InputTextWithHint("##BindingsFilter", "Type to search...", inputSettingsFilter.InputBuf, ArrayCount(inputSettingsFilter.InputBuf)))
			inputSettingsFilter.Build();

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
				if (!inputSettingsFilter.PassFilter(Gui::StringViewStart(it.Name), Gui::StringViewEnd(it.Name)))
					continue;

				if (it.Binding == nullptr)
				{
					if (!inputSettingsFilter.IsActive()) { Gui::TableNextRow(); Gui::TableSetColumnIndex(0); Gui::Selectable(" ##", false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_Disabled); }
					continue;
				}

				Gui::PushID(it.Binding);
				Gui::TableNextRow();

				Gui::TableSetColumnIndex(0);
				(it.Binding->HasValue) ? Gui::TextUnformatted("(User)") : Gui::TextDisabled("(Default)");

				Gui::TableSetColumnIndex(1);

				const b8 clicked = Gui::Selectable(it.Name.data(), false, ImGuiSelectableFlags_SpanAllColumns);
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
					selectedBindingName = it.Name.data();
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
								selectedMultiBinding->SetHasValueIfNotDefault();
								changesWereMade = true;
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
							selectedMultiBinding->SetHasValueIfNotDefault();
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
							selectedMultiBinding->SetHasValueIfNotDefault();
							changesWereMade = true;
						}
						else if (indexToMoveDown >= 0 && (indexToMoveDown + 1) < inOutCount)
						{
							std::swap(inOutSlots[indexToMoveDown], inOutSlots[indexToMoveDown + 1]);
							selectedMultiBinding->SetHasValueIfNotDefault();
							changesWereMade = true;
						}
						else if (indexToRemove >= 0)
						{
							selectedMultiBinding->Value.RemoveAt(indexToRemove);
							selectedMultiBinding->SetHasValueIfNotDefault();
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
