#include "imgui_input_binding.h"
#include "core_string.h"

struct ImGuiKeyInfo
{
	ImGuiKey KeyEnum;
	cstr EnumName;
	cstr DisplayName;
	cstr DisplayNameAlt;
};

static constexpr ImGuiKeyInfo NamedImGuiKeyInfoTable[] =
{
	{ ImGuiKey_Tab,				"Tab", "Tab", },
	{ ImGuiKey_LeftArrow,		"LeftArrow", "Left", },
	{ ImGuiKey_RightArrow,		"RightArrow", "Right", },
	{ ImGuiKey_UpArrow,			"UpArrow", "Up", },
	{ ImGuiKey_DownArrow,		"DownArrow", "Down", },
	{ ImGuiKey_PageUp,			"PageUp", "Page Up", },
	{ ImGuiKey_PageDown,		"PageDown", "Page Down", },
	{ ImGuiKey_Home,			"Home", "Home", },
	{ ImGuiKey_End,				"End", "End", },
	{ ImGuiKey_Insert,			"Insert", "Insert", },
	{ ImGuiKey_Delete,			"Delete", "Delete", },
	{ ImGuiKey_Backspace,		"Backspace", "Backspace", },
	{ ImGuiKey_Space,			"Space", "Space", },
	{ ImGuiKey_Enter,			"Enter", "Enter", },
	{ ImGuiKey_Escape,			"Escape", "Escape", },
	{ ImGuiKey_LeftCtrl,		"LeftCtrl", "Left Ctrl", },
	{ ImGuiKey_LeftShift,		"LeftShift", "Left Shift", },
	{ ImGuiKey_LeftAlt, 		"LeftAlt", "Left Alt", },
	{ ImGuiKey_LeftSuper,		"LeftSuper", "Left Super", },
	{ ImGuiKey_RightCtrl, 		"RightCtrl", "Right Ctrl", },
	{ ImGuiKey_RightShift, 		"RightShift", "Right Shift", },
	{ ImGuiKey_RightAlt, 		"RightAlt", "Right Alt", },
	{ ImGuiKey_RightSuper,		"RightSuper", "Right Super", },
	{ ImGuiKey_Menu,			"Menu", "Menu", },
	{ ImGuiKey_0, 				"0", "0", },
	{ ImGuiKey_1, 				"1", "1", },
	{ ImGuiKey_2, 				"2", "2", },
	{ ImGuiKey_3, 				"3", "3", },
	{ ImGuiKey_4, 				"4", "4", },
	{ ImGuiKey_5, 				"5", "5", },
	{ ImGuiKey_6, 				"6", "6", },
	{ ImGuiKey_7, 				"7", "7", },
	{ ImGuiKey_8, 				"8", "8", },
	{ ImGuiKey_9,				"9", "9", },
	{ ImGuiKey_A, 				"A", "A", },
	{ ImGuiKey_B, 				"B", "B", },
	{ ImGuiKey_C, 				"C", "C", },
	{ ImGuiKey_D, 				"D", "D", },
	{ ImGuiKey_E, 				"E", "E", },
	{ ImGuiKey_F, 				"F", "F", },
	{ ImGuiKey_G, 				"G", "G", },
	{ ImGuiKey_H, 				"H", "H", },
	{ ImGuiKey_I, 				"I", "I", },
	{ ImGuiKey_J,				"J", "J", },
	{ ImGuiKey_K, 				"K", "K", },
	{ ImGuiKey_L, 				"L", "L", },
	{ ImGuiKey_M, 				"M", "M", },
	{ ImGuiKey_N, 				"N", "N", },
	{ ImGuiKey_O, 				"O", "O", },
	{ ImGuiKey_P, 				"P", "P", },
	{ ImGuiKey_Q, 				"Q", "Q", },
	{ ImGuiKey_R, 				"R", "R", },
	{ ImGuiKey_S, 				"S", "S", },
	{ ImGuiKey_T,				"T", "T", },
	{ ImGuiKey_U, 				"U", "U", },
	{ ImGuiKey_V, 				"V", "V", },
	{ ImGuiKey_W, 				"W", "W", },
	{ ImGuiKey_X, 				"X", "X", },
	{ ImGuiKey_Y, 				"Y", "Y", },
	{ ImGuiKey_Z,				"Z", "Z", },
	{ ImGuiKey_F1, 				"F1", "F1", },
	{ ImGuiKey_F2, 				"F2", "F2", },
	{ ImGuiKey_F3, 				"F3", "F3", },
	{ ImGuiKey_F4, 				"F4", "F4", },
	{ ImGuiKey_F5, 				"F5", "F5", },
	{ ImGuiKey_F6,				"F6", "F6", },
	{ ImGuiKey_F7, 				"F7", "F7", },
	{ ImGuiKey_F8, 				"F8", "F8", },
	{ ImGuiKey_F9, 				"F9", "F9", },
	{ ImGuiKey_F10, 			"F10", "F10", },
	{ ImGuiKey_F11, 			"F11", "F11", },
	{ ImGuiKey_F12,				"F12", "F12", },
	{ ImGuiKey_Apostrophe,		"Apostrophe", "'", "Apostrophe", },         // '
	{ ImGuiKey_Comma,           "Comma", ",", "Comma", },                   // ,
	{ ImGuiKey_Minus,           "Minus", "-", "Minus", },                   // -
	{ ImGuiKey_Period,          "Period", ".", "Period", },                 // .
	{ ImGuiKey_Slash,           "Slash", "/", "Slash", },                   // /
	{ ImGuiKey_Semicolon,       "Semicolon", ";", "Semicolon", },           // ;
	{ ImGuiKey_Equal,           "Equal", "=", "Equal", },                   // =
	{ ImGuiKey_LeftBracket,     "LeftBracket", "[", "Left Bracket", },      // [
	{ ImGuiKey_Backslash,       "Backslash", "\\", "Backslash", },          // \ (this text inhibit multiline comment caused by backslash)
	{ ImGuiKey_RightBracket,    "RightBracket", "]", "Right Bracket", },    // ]
	{ ImGuiKey_GraveAccent,     "GraveAccent", "`", "Grave Accent", },     // `
	{ ImGuiKey_CapsLock,		"CapsLock", "Caps Lock", },
	{ ImGuiKey_ScrollLock,		"ScrollLock", "Scroll Lock", },
	{ ImGuiKey_NumLock,			"NumLock", "Num Lock", },
	{ ImGuiKey_PrintScreen,		"PrintScreen", "Print Screen", },
	{ ImGuiKey_Pause,			"Pause", "Pause", },
	{ ImGuiKey_Keypad0, 		"Keypad0", "Keypad 0", },
	{ ImGuiKey_Keypad1, 		"Keypad1", "Keypad 1", },
	{ ImGuiKey_Keypad2, 		"Keypad2", "Keypad 2", },
	{ ImGuiKey_Keypad3, 		"Keypad3", "Keypad 3", },
	{ ImGuiKey_Keypad4,			"Keypad4", "Keypad 4", },
	{ ImGuiKey_Keypad5, 		"Keypad5", "Keypad 5", },
	{ ImGuiKey_Keypad6, 		"Keypad6", "Keypad 6", },
	{ ImGuiKey_Keypad7, 		"Keypad7", "Keypad 7", },
	{ ImGuiKey_Keypad8, 		"Keypad8", "Keypad 8", },
	{ ImGuiKey_Keypad9,			"Keypad9", "Keypad 9", },
	{ ImGuiKey_KeypadDecimal,	"KeypadDecimal", "Keypad Decimal", },
	{ ImGuiKey_KeypadDivide,	"KeypadDivide", "Keypad Divide", },
	{ ImGuiKey_KeypadMultiply,	"KeypadMultiply", "Keypad Multiply", },
	{ ImGuiKey_KeypadSubtract,	"KeypadSubtract", "Keypad Subtract", },
	{ ImGuiKey_KeypadAdd,		"KeypadAdd", "Keypad Add", },
	{ ImGuiKey_KeypadEnter,		"KeypadEnter", "Keypad Enter", },
	{ ImGuiKey_KeypadEqual,		"KeypadEqual", "Keypad Equal", },

	{ ImGuiKey_GamepadStart,		"GamepadStart", "Gamepad Start", },              // Menu (Xbox)          + (Switch)   Start/Options (PS) // --
	{ ImGuiKey_GamepadBack,			"GamepadBack", "Gamepad Back", },                // View (Xbox)          - (Switch)   Share (PS)         // --
	{ ImGuiKey_GamepadFaceLeft,		"GamepadFaceLeft", "Gamepad Face Left", },       // X (Xbox)             Y (Switch)   Square (PS)        // -> ImGuiNavInput_Menu
	{ ImGuiKey_GamepadFaceRight,	"GamepadFaceRight", "Gamepad Face Right", },     // B (Xbox)             A (Switch)   Circle (PS)        // -> ImGuiNavInput_Cancel
	{ ImGuiKey_GamepadFaceUp,		"GamepadFaceUp", "Gamepad Face Up", },           // Y (Xbox)             X (Switch)   Triangle (PS)      // -> ImGuiNavInput_Input
	{ ImGuiKey_GamepadFaceDown,		"GamepadFaceDown", "Gamepad Face Down", },       // A (Xbox)             B (Switch)   Cross (PS)         // -> ImGuiNavInput_Activate
	{ ImGuiKey_GamepadDpadLeft,		"GamepadDpadLeft", "Gamepad Dpad Left", },       // D-pad Left                                           // -> ImGuiNavInput_DpadLeft
	{ ImGuiKey_GamepadDpadRight,	"GamepadDpadRight", "Gamepad Dpad Right", },     // D-pad Right                                          // -> ImGuiNavInput_DpadRight
	{ ImGuiKey_GamepadDpadUp,		"GamepadDpadUp", "Gamepad Dpad Up", },           // D-pad Up                                             // -> ImGuiNavInput_DpadUp
	{ ImGuiKey_GamepadDpadDown,		"GamepadDpadDown", "Gamepad Dpad Down", },       // D-pad Down                                           // -> ImGuiNavInput_DpadDown
	{ ImGuiKey_GamepadL1,			"GamepadL1", "Gamepad L1", },                    // L Bumper (Xbox)      L (Switch)   L1 (PS)            // -> ImGuiNavInput_FocusPrev + ImGuiNavInput_TweakSlow
	{ ImGuiKey_GamepadR1,			"GamepadR1", "Gamepad R1", },                    // R Bumper (Xbox)      R (Switch)   R1 (PS)            // -> ImGuiNavInput_FocusNext + ImGuiNavInput_TweakFast
	{ ImGuiKey_GamepadL2,			"GamepadL2", "Gamepad L2", },                    // L Trigger (Xbox)     ZL (Switch)  L2 (PS) [Analog]
	{ ImGuiKey_GamepadR2,			"GamepadR2", "Gamepad R2", },                    // R Trigger (Xbox)     ZR (Switch)  R2 (PS) [Analog]
	{ ImGuiKey_GamepadL3,			"GamepadL3", "Gamepad L3", },                    // L Thumbstick (Xbox)  L3 (Switch)  L3 (PS)
	{ ImGuiKey_GamepadR3,			"GamepadR3", "Gamepad R3", },                    // R Thumbstick (Xbox)  R3 (Switch)  R3 (PS)
	{ ImGuiKey_GamepadLStickLeft,	"GamepadLStickLeft", "Gamepad LStick Left", },   // [Analog]                                             // -> ImGuiNavInput_LStickLeft
	{ ImGuiKey_GamepadLStickRight,	"GamepadLStickRight", "Gamepad LStick Right", }, // [Analog]                                             // -> ImGuiNavInput_LStickRight
	{ ImGuiKey_GamepadLStickUp,		"GamepadLStickUp", "Gamepad LStick Up", },       // [Analog]                                             // -> ImGuiNavInput_LStickUp
	{ ImGuiKey_GamepadLStickDown,	"GamepadLStickDown", "Gamepad LStick Down", },   // [Analog]                                             // -> ImGuiNavInput_LStickDown
	{ ImGuiKey_GamepadRStickLeft,	"GamepadRStickLeft", "Gamepad RStick Left", },   // [Analog]
	{ ImGuiKey_GamepadRStickRight,	"GamepadRStickRight", "Gamepad RStick Right", }, // [Analog]
	{ ImGuiKey_GamepadRStickUp,		"GamepadRStickUp", "Gamepad RStick Up", },       // [Analog]
	{ ImGuiKey_GamepadRStickDown,	"GamepadRStickDown", "Gamepad RStick Down", },   // [Analog]

	{ ImGuiKey_ModCtrl,  "ModCtrl", "Ctrl", "Control", },
	{ ImGuiKey_ModShift, "ModShift", "Shift", },
	{ ImGuiKey_ModAlt, 	 "ModAlt", "Alt", },
	{ ImGuiKey_ModSuper, "ModSuper", "Super", },
};

static constexpr b8 CompileTimeValidateNamedImGuiKeyInfoTable(const ImGuiKeyInfo* namedImGuiKeyInfoTable)
{
	for (i32 i = 0; i < ImGuiKey_NamedKey_COUNT; i++)
	{
		if (namedImGuiKeyInfoTable[i].KeyEnum != (ImGuiKey_NamedKey_BEGIN + i))
			return false;
		if (namedImGuiKeyInfoTable[i].DisplayName == nullptr)
			return false;
	}
	return true;
}

static_assert(ArrayCount(NamedImGuiKeyInfoTable) == ImGuiKey_NamedKey_COUNT);
static_assert(CompileTimeValidateNamedImGuiKeyInfoTable(NamedImGuiKeyInfoTable));

static constexpr ImGuiKeyInfo GetImGuikeyInfo(ImGuiKey key)
{
	if (key == ImGuiKey_None)
		return ImGuiKeyInfo { ImGuiKey_None, "None" };
	else if (key < ImGuiKey_NamedKey_BEGIN || key >= ImGuiKey_NamedKey_END)
		return ImGuiKeyInfo { key, "Unknown" };
	else
		return NamedImGuiKeyInfoTable[key - ImGuiKey_NamedKey_BEGIN];
}

static constexpr cstr ImGuiMouseButtonToCString(ImGuiMouseButton button)
{
	switch (button)
	{
	case ImGuiMouseButton_Left: return "Left";
	case ImGuiMouseButton_Right: return "Right";
	case ImGuiMouseButton_Middle: return "Middle";
	case ImGuiMouseButton_X1: return "X1";
	case ImGuiMouseButton_X2: return "X2";
	}
	return "";
}

template <typename Func>
static constexpr void ForEachImGuiKeyInKeyModFlags(ImGuiModFlags modifiers, Func func)
{
	if (modifiers & ImGuiModFlags_Ctrl) func(ImGuiKey_ModCtrl);
	if (modifiers & ImGuiModFlags_Shift) func(ImGuiKey_ModShift);
	if (modifiers & ImGuiModFlags_Alt) func(ImGuiKey_ModAlt);
}

InputFormatBuffer ToShortcutString(ImGuiKey key)
{
	InputFormatBuffer out {};
	if (const ImGuiKeyInfo info = GetImGuikeyInfo(key); info.DisplayName != nullptr)
		::strcpy_s(out.Data, info.DisplayName);
	return out;
}

InputFormatBuffer ToShortcutString(ImGuiKey key, ImGuiModFlags modifiers)
{
	InputFormatBuffer out {};
	char* bufferWriteHead = out.Data;
	char* const bufferEnd = out.Data + ArrayCount(out.Data);

	auto append = [&](cstr stringToAppend)
	{
		if (stringToAppend != nullptr && stringToAppend[0] != '\0')
		{
			const size_t strLength = strlen(stringToAppend);
			::memcpy(bufferWriteHead, stringToAppend, strLength);
			bufferWriteHead[strLength] = '\0';
			bufferWriteHead += strLength;
		}
	};

	if (modifiers != ImGuiModFlags_None)
		ForEachImGuiKeyInKeyModFlags(modifiers, [&](ImGuiKey modifierKey) { append(GetImGuikeyInfo(modifierKey).DisplayName); append(" + "); });
	append(GetImGuikeyInfo(key).DisplayName);

	assert(bufferWriteHead <= bufferEnd);
	return out;
}

InputFormatBuffer ToShortcutString(const InputBinding& binding)
{
	if (binding.Type == InputBindingType::Keyboard)
		return ToShortcutString(binding.KeyOrButton, binding.KeyModifiers);
	else if (binding.Type == InputBindingType::Mouse)
	{
		InputFormatBuffer out {};
		::strcpy_s(out.Data, ImGuiMouseButtonToCString(binding.KeyOrButton));
		return out;
	}
	else
	{
		return InputFormatBuffer {};
	}
}

InputFormatBuffer ToShortcutString(const MultiInputBinding& binding)
{
	return (binding.Count > 0) ? ToShortcutString(binding.Slots[0]) : InputFormatBuffer {};
}

void InputBindingToStorageString(const MultiInputBinding& in, std::string& stringToAppendTo)
{
	for (size_t i = 0; i < in.Count; i++)
	{
		if (i > 0)
			stringToAppendTo += ", ";

		const InputBinding& binding = in.Slots[i];
		if (binding.Type == InputBindingType::None)
		{
			// ...
		}
		else if (binding.Type == InputBindingType::Keyboard)
		{
			if (binding.KeyModifiers & ImGuiModFlags_Ctrl) { stringToAppendTo += "Ctrl+"; }
			if (binding.KeyModifiers & ImGuiModFlags_Shift) { stringToAppendTo += "Shift+"; }
			if (binding.KeyModifiers & ImGuiModFlags_Alt) { stringToAppendTo += "Alt+"; }

			const ImGuiKeyInfo keyInfo = GetImGuikeyInfo(binding.KeyOrButton);
			stringToAppendTo += (keyInfo.EnumName != nullptr) ? keyInfo.EnumName : "None";
		}
		else if (binding.Type == InputBindingType::Mouse)
		{
			// TODO: Handle mouse bindings
			assert(!"Unimplemented");
		}
		else
		{
			assert(!"Invalid binding type");
		}
	}
}

b8 InputBindingFromStorageString(std::string_view stringToParse, MultiInputBinding& out)
{
	out = MultiInputBinding {};
	if (stringToParse.empty())
		return true;

	// NOTE: This assumes the KeyInfo EnumName itself won't contain a comma itself (which it shouldn't due to being a c++ type name)
	b8 outSuccess = true;
	ASCII::ForEachInCommaSeparatedList(stringToParse, [&](std::string_view in)
	{
		in = ASCII::Trim(in);
		if (ASCII::IsAllWhitespace(in) || out.Count + 1 >= MultiInputBinding::MaxCount)
			return;

		ImGuiModFlags outMod = ImGuiModFlags_None;
		auto parseMod = [&](ImGuiModFlags mod, std::string_view name)
		{
			if (ASCII::StartsWithInsensitive(in, name))
			{
				const size_t plusIndex = in.find_first_of('+');
				if (plusIndex != std::string_view::npos) { outMod |= mod; in = ASCII::TrimLeft(in.substr(plusIndex + sizeof('+'))); }
			}
		};
		parseMod(ImGuiModFlags_Ctrl, "Ctrl");
		parseMod(ImGuiModFlags_Shift, "Shift");
		parseMod(ImGuiModFlags_Alt, "Alt");

		ImGuiKey outKey = ImGuiKey_None;
		for (const ImGuiKeyInfo& info : NamedImGuiKeyInfoTable)
			if (ASCII::MatchesInsensitive(in, info.EnumName)) { outKey = info.KeyEnum; break; }

		if (outKey != ImGuiKey_None || ASCII::MatchesInsensitive(in, "None"))
			out.Slots[out.Count++] = KeyBinding(outKey, outMod);
		else
			outSuccess = false;

		// TODO: Handle mouse bindings
	});
	return outSuccess;
}

struct InternalInputExtraFrameData
{
	ImGuiModFlags ModifiersDown;
	f32 TimeSinceLastModifiersChange;
};

static InternalInputExtraFrameData ThisFrameInputExData, LastFrameInputExData;

void ImGui_UpdateInternalInputExtraDataAtStartOfFrame()
{
	LastFrameInputExData = ThisFrameInputExData;
	ThisFrameInputExData.ModifiersDown = GImGui->IO.KeyMods;

	if (ThisFrameInputExData.ModifiersDown == LastFrameInputExData.ModifiersDown)
		ThisFrameInputExData.TimeSinceLastModifiersChange += GImGui->IO.DeltaTime;
	else
		ThisFrameInputExData.TimeSinceLastModifiersChange = 0.0;
}

void ImGui_UpdateInternalInputExtraDataAtEndOfFrame()
{
	// NOTE: Nothing to do here... yet!
	return;
}

namespace ImGui
{
	static b8 Internal_IsKeyDownAfterAllModifiers(ImGuiKey key)
	{
		const f32 keyDuration = ImGui::IsNamedKey(key) ? ImGui::GetKeyData(key)->DownDuration : 0.0f;
		return ThisFrameInputExData.TimeSinceLastModifiersChange >= keyDuration;
	}

	static b8 Internal_AreModifiersDownFirst(ImGuiKey key, ImGuiModFlags modifiers)
	{
		const f32 keyDuration = ImGui::IsNamedKey(key) ? ImGui::GetKeyData(key)->DownDuration : 0.0f;
		b8 allLonger = true;
		ForEachImGuiKeyInKeyModFlags(modifiers, [&](ImGuiKey modifierKey) { allLonger &= (ImGui::GetKeyData(modifierKey)->DownDuration >= keyDuration); });
		return allLonger;
	}

	b8 AreAllModifiersDown(ImGuiModFlags modifiers)
	{
		return ((GImGui->IO.KeyMods & modifiers) == modifiers);
	}

	b8 AreOnlyModifiersDown(ImGuiModFlags modifiers)
	{
		return (GImGui->IO.KeyMods == modifiers);
	}

	// BUG: Regular bindings with modifier keys as primary keys aren't triggered correctly
	b8 IsDown(const InputBinding& binding, InputModifierBehavior behavior)
	{
		if (binding.Type == InputBindingType::None)
		{
			return false;
		}
		else if (binding.Type == InputBindingType::Keyboard)
		{
			if (behavior == InputModifierBehavior::Strict)
				return ImGui::IsKeyDown(binding.KeyOrButton) && AreOnlyModifiersDown(binding.KeyModifiers) && Internal_IsKeyDownAfterAllModifiers(binding.KeyOrButton);
			else
				return ImGui::IsKeyDown(binding.KeyOrButton) && AreAllModifiersDown(binding.KeyModifiers) && Internal_AreModifiersDownFirst(binding.KeyOrButton, binding.KeyModifiers);
		}
		else if (binding.Type == InputBindingType::Mouse)
		{
			return ImGui::IsMouseDown(binding.KeyOrButton);
		}
		else
		{
			assert(!"Invalid binding type");
			return false;
		}
	}

	b8 IsPressed(const InputBinding& binding, b8 repeat, InputModifierBehavior behavior)
	{
		if (binding.Type == InputBindingType::None)
		{
			return false;
		}
		else if (binding.Type == InputBindingType::Keyboard)
		{
			// NOTE: Still have to explictily check the modifier hold durations here in case of repeat
			if (behavior == InputModifierBehavior::Strict)
				return ImGui::IsKeyPressed(binding.KeyOrButton, repeat) && AreOnlyModifiersDown(binding.KeyModifiers) && Internal_IsKeyDownAfterAllModifiers(binding.KeyOrButton);
			else
				return ImGui::IsKeyPressed(binding.KeyOrButton, repeat) && AreAllModifiersDown(binding.KeyModifiers) && Internal_AreModifiersDownFirst(binding.KeyOrButton, binding.KeyModifiers);
		}
		else if (binding.Type == InputBindingType::Mouse)
		{
			return ImGui::IsMouseClicked(binding.KeyOrButton, repeat);
		}
		else
		{
			assert(!"Invalid binding type");
			return false;
		}
	}

	b8 IsAnyDown(const MultiInputBinding& binding, InputModifierBehavior behavior)
	{
		for (size_t i = 0; i < binding.Count; i++)
			if (IsDown(binding.Slots[i], behavior))
				return true;
		return false;
	}

	b8 IsAnyPressed(const MultiInputBinding& binding, b8 repeat, InputModifierBehavior behavior)
	{
		for (size_t i = 0; i < binding.Count; i++)
			if (IsPressed(binding.Slots[i], repeat, behavior))
				return true;
		return false;
	}
}
