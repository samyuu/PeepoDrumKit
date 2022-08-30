#include "chart_editor_settings_gui.h"

namespace PeepoDrumKit
{
	struct CategoryTextPair { std::string_view Category, Text; };
	static CategoryTextPair GuiSplitCategoryText(std::string_view textWithPrefix)
	{
		if (size_t categorySeparatorIndex = textWithPrefix.find_first_of(':'); categorySeparatorIndex != std::string_view::npos)
		{
			if ((categorySeparatorIndex + 1) < textWithPrefix.size() && textWithPrefix[categorySeparatorIndex + 1] == ' ')
				categorySeparatorIndex++;
			return { textWithPrefix.substr(0, categorySeparatorIndex + 1), textWithPrefix.substr(categorySeparatorIndex + 1) };
		}
		else
			return { "", textWithPrefix };
	}

	static void GuiTextWithCategoryHighlight(std::string_view text)
	{
		auto split = GuiSplitCategoryText(text);
		if (!split.Category.empty())
		{
			Gui::PushStyleColor(ImGuiCol_Text, Gui::GetColorU32(ImGuiCol_Text, 0.8f));
			Gui::TextUnformatted(split.Category);
			Gui::PopStyleColor();
			Gui::SameLine(0.0f, 0.0f);
		}
		if (!split.Text.empty())
			Gui::TextUnformatted(split.Text);
	}

	static void GuiRightAlignedTextDisabled(std::string_view text)
	{
		Gui::SetCursorScreenPos(vec2(Gui::GetCursorScreenPos()) + vec2(Gui::GetContentRegionAvail().x - Gui::CalcTextSize(text).x, 0.0f));
		Gui::PushStyleColor(ImGuiCol_Text, Gui::GetStyleColorVec4(ImGuiCol_TextDisabled));
		Gui::TextUnformatted(text.data());
		Gui::PopStyleColor();
	}

	struct BoolLabels { cstr True = "True", False = "False"; inline cstr operator[](b8 v) const { return v ? True : False; } };
	static b8 GuiBoolCombo(cstr label, b8* inOutValue, b8 defaultValue, BoolLabels boolLabels = {})
	{
		b8 changesWereMade = false;
		if (Gui::BeginCombo(label, boolLabels[*inOutValue], ImGuiComboFlags_None))
		{
			for (i32 i = 0; i <= 1; i++)
			{
				const b8 it = (i == 0);
				const b8 isSelected = (*inOutValue == it);
				if (Gui::Selectable(boolLabels[it], isSelected)) { *inOutValue = it; changesWereMade = true; }
				if (isSelected) { Gui::SetItemDefaultFocus(); }
				if (it == defaultValue) { Gui::SameLine(); GuiRightAlignedTextDisabled("(Default)"); }
			}
			Gui::EndCombo();
		}
		return changesWereMade;
	}

	static b8 GuiBoolCombo(cstr label, WithDefault<b8>* inOutValue, BoolLabels boolLabels = {})
	{
		return GuiBoolCombo(label, &inOutValue->Value, inOutValue->Default, boolLabels);
	}

	static b8 GuiAssignableInputBindingButton(InputBinding& inOutBinding, vec2 buttonSize, ImGuiButtonFlags buttonFlags, InputBinding*& inOutAwaitInputBinding, CPUStopwatch& inOutAwaitInputStopwatch)
	{
		static constexpr u32 activeTextColor = 0xFF3DEBD2;
		static constexpr Time timeoutThreshold = Time::FromSec(4.0);
		static constexpr Time mouseClickThreshold = Time::FromMS(200.0);
		static constexpr auto animateBlink = [&](Time elapsed)
		{
			constexpr f32 frequency = (timeoutThreshold.ToSec_F32() * 1.2f);
			constexpr f32 low = 0.25f, high = 1.45f, min = 0.25f, max = 1.00f;
			return Clamp(ConvertRange(-1.0f, +1.0f, low, high, ::sinf((elapsed.ToSec_F32() * frequency) + 1.0f)), min, max);
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

	namespace SettingsGui
	{
		enum class DataType : u32 { Invalid, B8, I32, F32, StdString, };
		template <typename T> constexpr DataType TemplateToDataType() = delete;
		template <> constexpr DataType TemplateToDataType<b8>() { return DataType::B8; }
		template <> constexpr DataType TemplateToDataType<i32>() { return DataType::I32; }
		template <> constexpr DataType TemplateToDataType<f32>() { return DataType::F32; }
		template <> constexpr DataType TemplateToDataType<std::string>() { return DataType::StdString; }

		enum class WidgetType : u32 { Default, B8_ExclusiveAudioComboBox, I32_BarDivisionComboBox, F32_TimelineScrollSensitivity, F32_ExponentialSpeed, };

		struct SettingsEntry
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

			template <typename T>
			SettingsEntry(WithDefault<T>& v, std::string_view header, std::string_view description, WidgetType widgetType = WidgetType::Default)
				: DataType(TemplateToDataType<T>()), Widget(widgetType), ValuePtr(&v), HasValuePtr(&v.HasValue), Header(header), Description(description)
			{
				ResetToDefaultFunc = [](void* valuePtr) { static_cast<WithDefault<T>*>(valuePtr)->ResetToDefault(); };
				SetHasValueIfNotDefaultFunc = [](void* valuePtr) { static_cast<WithDefault<T>*>(valuePtr)->SetHasValueIfNotDefault(); };
			}
		};

		static b8 DrawEntryWidgetGroupGui(SettingsEntry& in)
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
				GuiTextWithCategoryHighlight(in.Header);
				Gui::PopFont();

				if (!in.Description.empty())
				{
					assert(in.Description.back() == '.');
					Gui::PushStyleColor(ImGuiCol_Text, Gui::GetColorU32(ImGuiCol_Text, 0.8f));
					Gui::PushTextWrapPos(ClampBot(Gui::GetCursorPosX() + Gui::GetContentRegionAvail().x, Gui::GetFrameHeight() * 12.0f));
					Gui::TextWrapped(in.Description.data());
					Gui::PopTextWrapPos();
					Gui::PopStyleColor();
				}

				auto* inOutB8 = in.ValueAs<b8>();
				auto* inOutI32 = in.ValueAs<i32>();
				auto* inOutF32 = in.ValueAs<f32>();
				auto* inOutStr = in.ValueAs<std::string>();

				if (inOutB8 != nullptr)
				{
					if (in.Widget == WidgetType::B8_ExclusiveAudioComboBox)
					{
						changesWereMade |= GuiBoolCombo("##", inOutB8, { "True (Exclusive Mode)", "False" });
					}
					else
					{
						changesWereMade |= GuiBoolCombo("##", inOutB8);
					}
				}
				else if (inOutI32 != nullptr)
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
					if (in.Widget == WidgetType::F32_TimelineScrollSensitivity)
					{
						Gui::SetNextItemWidth(Max(1.0f, Gui::CalcItemWidth() - ((Gui::GetFrameHeight() + style.ItemInnerSpacing.x) * 3)));
						changesWereMade |= Gui::SliderFloat("##", &inOutF32->Value, 50.0f, 350.0f, "%.0f", ImGuiSliderFlags_None);
						Gui::SameLine(0, style.ItemInnerSpacing.x);
						changesWereMade |= Gui::ButtonPlusMinusFloat('-', &inOutF32->Value, 1.0f, 10.0f);
						Gui::SameLine(0, style.ItemInnerSpacing.x);
						changesWereMade |= Gui::ButtonPlusMinusFloat('+', &inOutF32->Value, 1.0f, 10.0f);
						Gui::SameLine(0.0f, style.ItemInnerSpacing.x);
						if (Gui::BeginCombo("##Combo", "", ImGuiComboFlags_NoPreview | ImGuiComboFlags_PopupAlignLeft))
						{
							if (Gui::Selectable("Reset to Default", false, !inOutF32->HasValue ? ImGuiSelectableFlags_Disabled : 0)) { in.ResetToDefault(); changesWereMade = true; }
							Gui::EndCombo();
						}
						if (changesWereMade) { inOutF32->Value = Clamp(inOutF32->Value, 50.0f, 5000.0f); }
					}
					else if (in.Widget == WidgetType::F32_ExponentialSpeed)
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
						if (changesWereMade) { inOutF32->Value = Clamp(inOutF32->Value, 0.0f, 1000.0f); }
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
				else
				{
					assert(false);
				}

				if (changesWereMade)
					in.SetHasValueIfNotDefault();
			}
			addGroupPadding(GuiScale(2.0f));
			Gui::EndGroup();

			return changesWereMade;
		}

		static b8 DrawEntriesListTableGui(SettingsEntry* entries, size_t entriesCount, ImGuiTextFilter* filter, ChartSettingsWindowTempActiveWidgetGroup& lastActiveGroup)
		{
			const auto& style = Gui::GetStyle();
			b8 changesWereMade = false;

			Gui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { style.ItemSpacing.x, GuiScale(8.0f) });

			if (filter != nullptr)
			{
				Gui::SetNextItemWidth(-1.0f);
				if (Gui::InputTextWithHint("##Filter", "Type to search...", filter->InputBuf, ArrayCount(filter->InputBuf)))
					filter->Build();
			}

			Gui::PushStyleVar(ImGuiStyleVar_CellPadding, GuiScale(vec2(8.0f, 4.0f)));
			const b8 beginTable = Gui::BeginTable("InnerTable", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_ScrollY, Gui::GetContentRegionAvail());
			Gui::PopStyleVar(2);
			if (beginTable)
			{
				Gui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, Gui::CalcTextSize("(Default)").x + Gui::GetFrameHeight());

				Gui::UpdateSmoothScrollWindow();
				Gui::PushStyleVar(ImGuiStyleVar_ItemSpacing, GuiScale(vec2(8.0f, 8.0f)));

				b8 isAnyItemGroupHoveredOrActive = false;
				for (size_t entryIndex = 0; entryIndex < entriesCount; entryIndex++)
				{
					SettingsEntry& entry = entries[entryIndex];
					if (filter != nullptr)
					{
						if (!filter->PassFilter(Gui::StringViewStart(entry.Header), Gui::StringViewEnd(entry.Header)) &&
							!filter->PassFilter(Gui::StringViewStart(entry.Description), Gui::StringViewEnd(entry.Description)))
							continue;
					}

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

					if (DrawEntryWidgetGroupGui(entry))
						changesWereMade = true;

					const Rect itemGroupRect = Gui::GetItemRect();
					const b8 isItemGroupHovered = Gui::IsItemHovered();
					b8 isPopupOpen = false;

					if (isPopupOpen = Gui::BeginPopupContextItem("SettingsEntryContextMenu"))
					{
						GuiTextWithCategoryHighlight(entry.Header);
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

					if (ArrayItToIndex(&entry, entries) + 1 < entriesCount)
						Gui::Separator();
				}

				if (!isAnyItemGroupHoveredOrActive)
					lastActiveGroup = {};

				Gui::PopStyleVar(1);
				Gui::EndTable();
			}

			return changesWereMade;
		}

		struct InputSettingsEntry
		{
			WithDefault<MultiInputBinding>* Binding;
			std::string_view Name;
		};

		static b8 DrawInputEntriesListTableGui(InputSettingsEntry* entries, size_t entriesCount, ImGuiTextFilter* filter, ChartSettingsWindowTempInputState& state)
		{
			const auto& style = Gui::GetStyle();
			b8 changesWereMade = false;

			Gui::PushStyleVar(ImGuiStyleVar_CellPadding, { style.CellPadding.x, GuiScale(4.0f) });
			Gui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { style.ItemSpacing.x, GuiScale(8.0f) });
			Gui::PushStyleColor(ImGuiCol_Header, Gui::GetColorU32(ImGuiCol_Header, 0.5f));
			Gui::PushStyleColor(ImGuiCol_HeaderHovered, Gui::GetColorU32(ImGuiCol_HeaderHovered, 0.5f));
			defer { Gui::PopStyleColor(2); Gui::PopStyleVar(2); };

			// TODO: CTRL+F keybinding for all search fields and also match shortcut strings themselves (and draw in different color on match?)
			if (filter != nullptr)
			{
				Gui::SetNextItemWidth(-1.0f);
				if (Gui::InputTextWithHint("##Filter", "Type to search...", filter->InputBuf, ArrayCount(filter->InputBuf)))
					filter->Build();
			}

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

				Gui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, ConvertRangeClampInput(0.0f, 1.0f, 1.0f, style.DisabledAlpha, state.MultiBindingPopupFadeCurrent));
				Gui::BeginDisabled(state.MultiBindingPopupFadeTarget >= 0.001f);

				for (size_t entryIndex = 0; entryIndex < entriesCount; entryIndex++)
				{
					InputSettingsEntry& entry = entries[entryIndex];
					if (filter != nullptr)
					{
						if (!filter->PassFilter(Gui::StringViewStart(entry.Name), Gui::StringViewEnd(entry.Name)))
							continue;

						if (entry.Binding == nullptr)
						{
							if (!filter->IsActive()) { Gui::TableNextRow(); Gui::TableSetColumnIndex(0); Gui::Selectable("##", false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_Disabled); }
							continue;
						}
					}

					Gui::PushID(entry.Binding);
					Gui::TableNextRow();

					Gui::TableSetColumnIndex(0);
					(entry.Binding->HasValue) ? Gui::TextUnformatted("(User)") : Gui::TextDisabled("(Default)");

					Gui::TableSetColumnIndex(1);

					const b8 clicked = Gui::Selectable("##", false, ImGuiSelectableFlags_SpanAllColumns);
					if (Gui::BeginPopupContextItem("InputSettingsEntryContextMenu"))
					{
						GuiTextWithCategoryHighlight(entry.Name);
						Gui::Separator();
						if (Gui::MenuItem("Clear All", "", nullptr)) { entry.Binding->Value.ClearAll(); entry.Binding->SetHasValueIfNotDefault(); changesWereMade = true; }
						if (Gui::MenuItem("Reset to Default", "", nullptr)) { entry.Binding->ResetToDefault(); changesWereMade = true; }
						Gui::EndPopup();
					}

					Gui::SameLine(0.0f, 0.0f);
					GuiTextWithCategoryHighlight(entry.Name);
					if (clicked) { state.SelectedMultiBinding = entry.Binding; memcpy(&state.SelectedMultiBindingOnOpenCopy, entry.Binding, sizeof(state.SelectedMultiBindingOnOpenCopy)); openMultiBindingPopupAtEndOfFrame = true; }

					Gui::TableSetColumnIndex(2);
					if (entry.Binding->Value.Count > 0)
					{
						for (size_t i = 0; i < entry.Binding->Value.Count; i++)
						{
							if (i > 0) { Gui::SameLine(0.0f, 0.0f); Gui::Text(" , "); Gui::SameLine(0.0f, 0.0f); }
							const auto& binding = entry.Binding->Value.Slots[i];

							Gui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, GuiScale(1.0f));
							Gui::PushStyleColor(ImGuiCol_Border, Gui::GetColorU32(ImGuiCol_ButtonActive));
							{
								Gui::SmallButton(ToShortcutString(binding).Data);
							}
							Gui::PopStyleColor();
							Gui::PopStyleVar();
						}
					}
					else
					{
						// NOTE: Use a chouonpu here instead of a regular minus for a more readable and thicker glyph at smaller font sizes
						Gui::TextDisabled("\xE3\x83\xBC"); // u8"[" // "-" // "(None)"
					}

					Gui::PopID();
				}
				Gui::EndDisabled();
				Gui::PopStyleVar();

				Gui::EndTable();
			}

			Gui::AnimateExponential(&state.MultiBindingPopupFadeCurrent, state.MultiBindingPopupFadeTarget, 20.0f);
			if (openMultiBindingPopupAtEndOfFrame)
			{
				state.MultiBindingPopupFadeCurrent = state.MultiBindingPopupFadeTarget;
				state.MultiBindingPopupFadeTarget = 1.0f;
			}

			if (state.MultiBindingPopupFadeCurrent >= 0.001f)
			{
				cstr selectedBindingName = "Unnamed";
				for (size_t i = 0; i < entriesCount; i++)
				{
					if (entries[i].Binding == state.SelectedMultiBinding)
						selectedBindingName = entries[i].Name.data();
				}
				assert(state.SelectedMultiBinding != nullptr);

				const ImGuiWindowFlags allowInputWindowFlag = (state.MultiBindingPopupFadeCurrent < 0.9f) ? ImGuiWindowFlags_NoInputs : ImGuiWindowFlags_None;

				Gui::PushStyleVar(ImGuiStyleVar_Alpha, state.MultiBindingPopupFadeCurrent);
				Gui::PushStyleVar(ImGuiStyleVar_WindowPadding, GuiScale(vec2(12.0f, 8.0f)));
				Gui::PushStyleVar(ImGuiStyleVar_FramePadding, GuiScale(vec2(8.0f, 6.0f)));
				Gui::PushStyleColor(ImGuiCol_Border, Gui::GetStyleColorVec4(ImGuiCol_TableBorderStrong));
				Gui::SetNextWindowPos(Gui::GetMousePos(), ImGuiCond_Appearing, vec2(0.0f, 0.0f));
				Gui::PushFont(FontMedium_EN);
				Gui::Begin(selectedBindingName, nullptr, allowInputWindowFlag | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking);
				{
					if ((Gui::IsMouseClicked(ImGuiMouseButton_Left, false) && !Gui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) ||
						(Gui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && Gui::GetActiveID() == 0 && state.TempAssignedBinding == nullptr && Gui::IsAnyPressed(*Settings.Input.Dialog_Cancel, false)))
					{
						state.MultiBindingPopupFadeTarget = 0.0f;
						state.AssignedBindingStopwatch.Stop();
						state.TempAssignedBinding = nullptr;
					}

					GuiTextWithCategoryHighlight(selectedBindingName);

					if (Gui::BeginTable("SelectedBindingTable", 4, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders))
					{
						Gui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, Gui::CalcTextSize("xxx").x * 9.0f + Gui::GetFrameHeight());
						Gui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, Gui::CalcTextSize("xxx").x * 1.0f + Gui::GetFrameHeight());
						Gui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, Gui::CalcTextSize("xxx").x * 1.0f + Gui::GetFrameHeight());
						Gui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, Gui::CalcTextSize("xxx").x * 2.0f + Gui::GetFrameHeight());

						i32 indexToMoveUp = -1, indexToMoveDown = -1, indexToRemove = -1;
						for (i32 i = 0; i < state.SelectedMultiBinding->Value.Count; i++)
						{
							auto& it = state.SelectedMultiBinding->Value.Slots[i];
							Gui::PushID(&it);
							Gui::TableNextRow();
							{
								Gui::TableSetColumnIndex(0);
								if (GuiAssignableInputBindingButton(it, { Gui::GetContentRegionAvail().x, 0.0f }, ImGuiButtonFlags_None, state.TempAssignedBinding, state.AssignedBindingStopwatch))
								{
									state.SelectedMultiBinding->SetHasValueIfNotDefault();
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
							Gui::BeginDisabled(state.SelectedMultiBinding->Value.Count >= MultiInputBinding::MaxCount);
							Gui::PushStyleColor(ImGuiCol_Button, Gui::GetStyleColorVec4(ImGuiCol_Header));
							Gui::PushStyleColor(ImGuiCol_ButtonHovered, Gui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
							Gui::PushStyleColor(ImGuiCol_ButtonActive, Gui::GetStyleColorVec4(ImGuiCol_HeaderActive));
							if (InputBinding newBinding {}; GuiAssignableInputBindingButton(newBinding, { Gui::GetContentRegionAvail().x, 0.0f }, ImGuiButtonFlags_None, state.TempAssignedBinding, state.AssignedBindingStopwatch))
							{
								state.SelectedMultiBinding->Value.Slots[state.SelectedMultiBinding->Value.Count++] = newBinding;
								state.SelectedMultiBinding->SetHasValueIfNotDefault();
								changesWereMade = true;
							}
							Gui::PopStyleColor(3);
							Gui::EndDisabled();
						}

						if (indexToMoveUp >= 0 || indexToMoveDown >= 0 || indexToRemove >= 0)
						{
							auto& inOutCount = state.SelectedMultiBinding->Value.Count;
							auto& inOutSlots = state.SelectedMultiBinding->Value.Slots;

							if (indexToMoveUp >= 1 && indexToMoveUp <= inOutCount)
							{
								std::swap(inOutSlots[indexToMoveUp], inOutSlots[indexToMoveUp - 1]);
								state.SelectedMultiBinding->SetHasValueIfNotDefault();
								changesWereMade = true;
							}
							else if (indexToMoveDown >= 0 && (indexToMoveDown + 1) < inOutCount)
							{
								std::swap(inOutSlots[indexToMoveDown], inOutSlots[indexToMoveDown + 1]);
								state.SelectedMultiBinding->SetHasValueIfNotDefault();
								changesWereMade = true;
							}
							else if (indexToRemove >= 0)
							{
								state.SelectedMultiBinding->Value.RemoveAt(indexToRemove);
								state.SelectedMultiBinding->SetHasValueIfNotDefault();
								changesWereMade = true;
							}
						}

						Gui::EndTable();
					}

					Gui::PushMultiItemsWidths(2, GuiScale(260.0f));
					{
						const b8 isSameAsOnOpen =
							(state.SelectedMultiBinding->HasValue == state.SelectedMultiBindingOnOpenCopy.HasValue) &&
							(state.SelectedMultiBinding->Value == state.SelectedMultiBindingOnOpenCopy.Value);

						Gui::BeginDisabled(isSameAsOnOpen);
						if (Gui::Button("Revert Changes", { Gui::CalcItemWidth(), 0.0f }))
						{
							state.SelectedMultiBinding->HasValue = state.SelectedMultiBindingOnOpenCopy.HasValue;
							state.SelectedMultiBinding->Value = state.SelectedMultiBindingOnOpenCopy.Value;
							changesWereMade = true;
						}
						Gui::EndDisabled();
						Gui::PopItemWidth();
						Gui::SameLine();

						Gui::BeginDisabled(!state.SelectedMultiBinding->HasValue);
						if (Gui::Button("Reset to Default", { Gui::CalcItemWidth(), 0.0f }))
						{
							state.SelectedMultiBinding->ResetToDefault();
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
				{
					constexpr size_t SizeOfUserSettingsData = sizeof(UserSettingsData);
					static_assert(PEEPO_RELEASE || SizeOfUserSettingsData == 15496, "TODO: Add missing settings entries for newly added UserSettingsData fields");

					SettingsGui::SettingsEntry settingsEntriesMain[] =
					{
						SettingsGui::SettingsEntry(
							settings.General.DefaultCreatorName,
							"General: Default Creator Name",
							"The name that is automatically filled in when creating a new chart."),

						SettingsGui::SettingsEntry(
							settings.General.DrumrollAutoHitBarDivision,
							"General: Drumroll Preview Interval",
							"The interval at which drumrolls have their hit sounds previewed at.",
							SettingsGui::WidgetType::I32_BarDivisionComboBox),

						SettingsGui::SettingsEntry(
							settings.General.TimelineScrollInvertMouseWheel,
							"Timeline: Invert Scroll Wheel Direction",
							"Invert the mouse wheel scroll direcion so that scrolling downwards results in moving forward through the timeline."),

						SettingsGui::SettingsEntry(
							settings.General.TimelineScrollDistancePerMouseWheelTick,
							"Timeline: Scroll Wheel Sensitivity",
							"The timeline distance moved per mouse wheel scroll tick.",
							SettingsGui::WidgetType::F32_TimelineScrollSensitivity),

						SettingsGui::SettingsEntry(
							settings.General.TimelineScrollDistancePerMouseWheelTickFast,
							"Timeline: Scroll Wheel Sensitivity (Shift)",
							"The timeline distance moved per mouse wheel scroll tick while holding down shift.",
							SettingsGui::WidgetType::F32_TimelineScrollSensitivity),

						// TODO: Remove..?
						// SettingsGui::SettingsEntry(settings.General.TimelineScrubAutoScrollPixelThreshold, "Timeline Scrub Auto Scroll Pixel Threshold", ""),
						// SettingsGui::SettingsEntry(settings.General.TimelineScrubAutoScrollSpeedMin, "Timeline Scrub Auto Scroll Speed Min", ""),
						// SettingsGui::SettingsEntry(settings.General.TimelineScrubAutoScrollSpeedMax, "Timeline Scrub Auto Scroll Speed Max", ""),

						SettingsGui::SettingsEntry(settings.Animation.TimelineSmoothScrollSpeed,
							"Animation: Timeline Smooth Scroll Speed",
							"The animation speed when scrolling the timeline.",
							SettingsGui::WidgetType::F32_ExponentialSpeed),

						SettingsGui::SettingsEntry(settings.Animation.TimelineWorldSpaceCursorXSpeed,
							"Animation: Timeline Smooth Cursor Speed",
							"The animation speed for the timeline cursor when moving to a new position.",
							SettingsGui::WidgetType::F32_ExponentialSpeed),

						SettingsGui::SettingsEntry(settings.Animation.TimelineRangeSelectionExpansionSpeed,
							"Animation: Timeline Range Selection Speed",
							"The animation speed for the timeline range selection expansion.",
							SettingsGui::WidgetType::F32_ExponentialSpeed),
					};

					changesWereMade |= SettingsGui::DrawEntriesListTableGui(settingsEntriesMain, ArrayCount(settingsEntriesMain), &settingsFilterMain, lastActiveGroup);
				}
				Gui::PopStyleVar();
				Gui::EndTabItem();
			}

			if (Gui::BeginTabItem("Input Bindings"))
			{
				Gui::PushStyleVar(ImGuiStyleVar_FramePadding, originalFramePadding);
				{
					constexpr size_t SizeOfUserSettingsInputData = sizeof(UserSettingsData::InputData);
					static_assert(PEEPO_RELEASE || SizeOfUserSettingsInputData == 15008, "TODO: Add missing settings entries for newly added UserSettingsData::InputData bindings");

					SettingsGui::InputSettingsEntry settingsEntriesInput[] =
					{
						{ &settings.Input.Editor_ToggleFullscreen, "Editor: Toggle Fullscreen", },
						{ &settings.Input.Editor_ToggleVSync, "Editor: Toggle VSync", },
						{ &settings.Input.Editor_IncreaseGuiScale, "Editor: Zoom In", },
						{ &settings.Input.Editor_DecreaseGuiScale, "Editor: Zoom Out", },
						{ &settings.Input.Editor_ResetGuiScale, "Editor: Reset Zoom", },
						{ &settings.Input.Editor_Undo, "Editor: Undo", },
						{ &settings.Input.Editor_Redo, "Editor: Redo", },
						{ &settings.Input.Editor_OpenHelp, "Editor: Open Help", },
						{ &settings.Input.Editor_OpenSettings, "Editor: Open Settings", },
						{ &settings.Input.Editor_ChartNew, "Editor: Chart New", },
						{ &settings.Input.Editor_ChartOpen, "Editor: Chart Open", },
						{ &settings.Input.Editor_ChartOpenDirectory, "Editor: Chart Open Directory", },
						{ &settings.Input.Editor_ChartSave, "Editor: Chart Save", },
						{ &settings.Input.Editor_ChartSaveAs, "Editor: Chart Save As", },
						{},
						{ &settings.Input.Timeline_PlaceNoteDon, "Timeline: Place Note Don", },
						{ &settings.Input.Timeline_PlaceNoteKa, "Timeline: Place Note Ka", },
						{ &settings.Input.Timeline_PlaceNoteBalloon, "Timeline: Place Note Balloon", },
						{ &settings.Input.Timeline_PlaceNoteDrumroll, "Timeline: Place Note Drumroll", },
						{ &settings.Input.Timeline_FlipNoteType, "Timeline: Flip Note Type", },
						{ &settings.Input.Timeline_ToggleNoteSize, "Timeline: Toggle Note Size", },
						{ &settings.Input.Timeline_Cut, "Timeline: Cut", },
						{ &settings.Input.Timeline_Copy, "Timeline: Copy", },
						{ &settings.Input.Timeline_Paste, "Timeline: Paste", },
						{ &settings.Input.Timeline_DeleteSelection, "Timeline: Delete Selection", },
						{ &settings.Input.Timeline_StartEndRangeSelection, "Timeline: Start/End Range Selection", },
						{ &settings.Input.Timeline_StepCursorLeft, "Timeline: Step Cursor Left", },
						{ &settings.Input.Timeline_StepCursorRight, "Timeline: Step Cursor Right", },
						{ &settings.Input.Timeline_JumpToTimelineStart, "Timeline: Jump to Timeline Start", },
						{ &settings.Input.Timeline_JumpToTimelineEnd, "Timeline: Jump to Timeline End", },
						{ &settings.Input.Timeline_IncreaseGridDivision, "Timeline: Increase Grid Division", },
						{ &settings.Input.Timeline_DecreaseGridDivision, "Timeline: Decrease Grid Division", },
						{ &settings.Input.Timeline_SetGridDivision_1_4, "Timeline: Set Grid Division 1 / 4", },
						{ &settings.Input.Timeline_SetGridDivision_1_8, "Timeline: Set Grid Division 1 / 8", },
						{ &settings.Input.Timeline_SetGridDivision_1_12, "Timeline: Set Grid Division 1 / 12", },
						{ &settings.Input.Timeline_SetGridDivision_1_16, "Timeline: Set Grid Division 1 / 16", },
						{ &settings.Input.Timeline_SetGridDivision_1_24, "Timeline: Set Grid Division 1 / 24", },
						{ &settings.Input.Timeline_SetGridDivision_1_32, "Timeline: Set Grid Division 1 / 32", },
						{ &settings.Input.Timeline_SetGridDivision_1_48, "Timeline: Set Grid Division 1 / 48", },
						{ &settings.Input.Timeline_SetGridDivision_1_64, "Timeline: Set Grid Division 1 / 64", },
						{ &settings.Input.Timeline_SetGridDivision_1_96, "Timeline: Set Grid Division 1 / 96", },
						{ &settings.Input.Timeline_SetGridDivision_1_192, "Timeline: Set Grid Division 1 / 192", },
						{ &settings.Input.Timeline_IncreasePlaybackSpeed, "Timeline: Increase Playback Speed", },
						{ &settings.Input.Timeline_DecreasePlaybackSpeed, "Timeline: Decrease Playback Speed", },
						{ &settings.Input.Timeline_SetPlaybackSpeed_100, "Timeline: Set Playback Speed 100%", },
						{ &settings.Input.Timeline_SetPlaybackSpeed_75, "Timeline: Set Playback Speed 75%", },
						{ &settings.Input.Timeline_SetPlaybackSpeed_50, "Timeline: Set Playback Speed 50%", },
						{ &settings.Input.Timeline_SetPlaybackSpeed_25, "Timeline: Set Playback Speed 25%", },
						{ &settings.Input.Timeline_TogglePlayback, "Timeline: Toggle Playback", },
						{ &settings.Input.Timeline_ToggleMetronome, "Timeline: Toggle Metronome", },
						{},
						{ &settings.Input.TempoCalculator_Tap, "Tempo Calculator: Tap", },
						{ &settings.Input.TempoCalculator_Reset, "Tempo Calculator: Reset", },
						{},
						{ &settings.Input.Dialog_YesOrOk, "Dialog: Yes/Ok", },
						{ &settings.Input.Dialog_No, "Dialog: No", },
						{ &settings.Input.Dialog_Cancel, "Dialog: Cancel", },
						{ &settings.Input.Dialog_SelectNextTab, "Dialog: Select Next Tab", },
						{ &settings.Input.Dialog_SelectPreviousTab, "Dialog: Select Previous Tab", },
					};

					changesWereMade |= SettingsGui::DrawInputEntriesListTableGui(settingsEntriesInput, ArrayCount(settingsEntriesInput), &settingsFilterInput, inputState);
				}
				Gui::PopStyleVar();
				Gui::EndTabItem();
			}

			if (Gui::BeginTabItem("Audio Settings"))
			{
				Gui::PushStyleVar(ImGuiStyleVar_FramePadding, originalFramePadding);
				{
					SettingsGui::SettingsEntry settingsEntriesAudio[] =
					{
						SettingsGui::SettingsEntry(
							settings.Audio.OpenDeviceOnStartup,
							"Open Device on Startup",
							"Create an audio session as soon as the program starts."),

						SettingsGui::SettingsEntry(
							settings.Audio.CloseDeviceOnIdleFocusLoss,
							"Close Device on Idle Focus Loss",
							"Automatically close the audio session when loosing window focus and while not playing any sounds."),

						SettingsGui::SettingsEntry(
							settings.Audio.RequestExclusiveDeviceAccess,
							"Low-Latency Exclusive Mode",
							"Reduce audio latency by requesting exlusive device access.\n"
							"This will prevent all *other* applications from playing back or recording audio.",
							SettingsGui::WidgetType::B8_ExclusiveAudioComboBox),
					};

					changesWereMade |= SettingsGui::DrawEntriesListTableGui(settingsEntriesAudio, ArrayCount(settingsEntriesAudio), nullptr, lastActiveGroup);
				}
				Gui::PopStyleVar();
				Gui::EndTabItem();
			}

			Gui::EndTabBar();
		}
		Gui::PopStyleColor(2);
		Gui::PopStyleVar();

		return changesWereMade;
	}
}
