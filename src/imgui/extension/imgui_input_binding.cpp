#include "imgui_input_binding.h"

struct ImGuiKeyInfo
{
	ImGuiKey KeyEnum;
	const char* DisplayName;
	const char* DisplayNameAlt;
};

static constexpr ImGuiKeyInfo NamedImGuiKeyInfoTable[] =
{
	{ ImGuiKey_Tab,				"Tab", },
	{ ImGuiKey_LeftArrow,		"Left", },
	{ ImGuiKey_RightArrow,		"Right", },
	{ ImGuiKey_UpArrow,			"Up", },
	{ ImGuiKey_DownArrow,		"Down", },
	{ ImGuiKey_PageUp,			"Page Up", },
	{ ImGuiKey_PageDown,		"Page Down", },
	{ ImGuiKey_Home,			"Home", },
	{ ImGuiKey_End,				"End", },
	{ ImGuiKey_Insert,			"Insert", },
	{ ImGuiKey_Delete,			"Delete", },
	{ ImGuiKey_Backspace,		"Backspace", },
	{ ImGuiKey_Space,			"Space", },
	{ ImGuiKey_Enter,			"Enter", },
	{ ImGuiKey_Escape,			"Escape", },
	{ ImGuiKey_LeftCtrl,		"Left Ctrl", },
	{ ImGuiKey_LeftShift,		"Left Shift", },
	{ ImGuiKey_LeftAlt, 		"Left Alt", },
	{ ImGuiKey_LeftSuper,		"Left Super", },
	{ ImGuiKey_RightCtrl, 		"Right Ctrl", },
	{ ImGuiKey_RightShift, 		"Right Shift", },
	{ ImGuiKey_RightAlt, 		"Right Alt", },
	{ ImGuiKey_RightSuper,		"Right Super", },
	{ ImGuiKey_Menu,			"Menu", },
	{ ImGuiKey_0, 				"0", },
	{ ImGuiKey_1, 				"1", },
	{ ImGuiKey_2, 				"2", },
	{ ImGuiKey_3, 				"3", },
	{ ImGuiKey_4, 				"4", },
	{ ImGuiKey_5, 				"5", },
	{ ImGuiKey_6, 				"6", },
	{ ImGuiKey_7, 				"7", },
	{ ImGuiKey_8, 				"8", },
	{ ImGuiKey_9,				"9", },
	{ ImGuiKey_A, 				"A", },
	{ ImGuiKey_B, 				"B", },
	{ ImGuiKey_C, 				"C", },
	{ ImGuiKey_D, 				"D", },
	{ ImGuiKey_E, 				"E", },
	{ ImGuiKey_F, 				"F", },
	{ ImGuiKey_G, 				"G", },
	{ ImGuiKey_H, 				"H", },
	{ ImGuiKey_I, 				"I", },
	{ ImGuiKey_J,				"J", },
	{ ImGuiKey_K, 				"K", },
	{ ImGuiKey_L, 				"L", },
	{ ImGuiKey_M, 				"M", },
	{ ImGuiKey_N, 				"N", },
	{ ImGuiKey_O, 				"O", },
	{ ImGuiKey_P, 				"P", },
	{ ImGuiKey_Q, 				"Q", },
	{ ImGuiKey_R, 				"R", },
	{ ImGuiKey_S, 				"S", },
	{ ImGuiKey_T,				"T", },
	{ ImGuiKey_U, 				"U", },
	{ ImGuiKey_V, 				"V", },
	{ ImGuiKey_W, 				"W", },
	{ ImGuiKey_X, 				"X", },
	{ ImGuiKey_Y, 				"Y", },
	{ ImGuiKey_Z,				"Z", },
	{ ImGuiKey_F1, 				"F1", },
	{ ImGuiKey_F2, 				"F2", },
	{ ImGuiKey_F3, 				"F3", },
	{ ImGuiKey_F4, 				"F4", },
	{ ImGuiKey_F5, 				"F5", },
	{ ImGuiKey_F6,				"F6", },
	{ ImGuiKey_F7, 				"F7", },
	{ ImGuiKey_F8, 				"F8", },
	{ ImGuiKey_F9, 				"F9", },
	{ ImGuiKey_F10, 			"F10", },
	{ ImGuiKey_F11, 			"F11", },
	{ ImGuiKey_F12,				"F12", },
	{ ImGuiKey_Apostrophe,		"'", "Apostrophe", },        // '
	{ ImGuiKey_Comma,           ",", "Comma", },             // ,
	{ ImGuiKey_Minus,           "-", "Minus", },             // -
	{ ImGuiKey_Period,          ".", "Period", },            // .
	{ ImGuiKey_Slash,           "/", "Slash", },             // /
	{ ImGuiKey_Semicolon,       ";", "Semicolon", },         // ;
	{ ImGuiKey_Equal,           "=", "Equal", },             // =
	{ ImGuiKey_LeftBracket,     "[", "Left Bracket", },      // [
	{ ImGuiKey_Backslash,       "\\", "Backslash", },        // \ (this text inhibit multiline comment caused by backslash)
	{ ImGuiKey_RightBracket,    "]", "Right Bracket", },     // ]
	{ ImGuiKey_GraveAccent,     "`", "Grave Accent", },      // `
	{ ImGuiKey_CapsLock,		"Caps Lock", },
	{ ImGuiKey_ScrollLock,		"Scroll Lock", },
	{ ImGuiKey_NumLock,			"Num Lock", },
	{ ImGuiKey_PrintScreen,		"Print Screen", },
	{ ImGuiKey_Pause,			"Pause", },
	{ ImGuiKey_Keypad0, 		"Keypad 0", },
	{ ImGuiKey_Keypad1, 		"Keypad 1", },
	{ ImGuiKey_Keypad2, 		"Keypad 2", },
	{ ImGuiKey_Keypad3, 		"Keypad 3", },
	{ ImGuiKey_Keypad4,			"Keypad 4", },
	{ ImGuiKey_Keypad5, 		"Keypad 5", },
	{ ImGuiKey_Keypad6, 		"Keypad 6", },
	{ ImGuiKey_Keypad7, 		"Keypad 7", },
	{ ImGuiKey_Keypad8, 		"Keypad 8", },
	{ ImGuiKey_Keypad9,			"Keypad 9", },
	{ ImGuiKey_KeypadDecimal,	"Keypad Decimal", },
	{ ImGuiKey_KeypadDivide,	"Keypad Divide", },
	{ ImGuiKey_KeypadMultiply,	"Keypad Multiply", },
	{ ImGuiKey_KeypadSubtract,	"Keypad Subtract", },
	{ ImGuiKey_KeypadAdd,		"Keypad Add", },
	{ ImGuiKey_KeypadEnter,		"Keypad Enter", },
	{ ImGuiKey_KeypadEqual,		"Keypad Equal", },

	{ ImGuiKey_GamepadStart,		"Gamepad Start", },        // Menu (Xbox)          + (Switch)   Start/Options (PS) // --
	{ ImGuiKey_GamepadBack,			"Gamepad Back", },         // View (Xbox)          - (Switch)   Share (PS)         // --
	{ ImGuiKey_GamepadFaceLeft,		"Gamepad Face Left", },    // X (Xbox)             Y (Switch)   Square (PS)        // -> ImGuiNavInput_Menu
	{ ImGuiKey_GamepadFaceRight,	"Gamepad Face Right", },   // B (Xbox)             A (Switch)   Circle (PS)        // -> ImGuiNavInput_Cancel
	{ ImGuiKey_GamepadFaceUp,		"Gamepad Face Up", },      // Y (Xbox)             X (Switch)   Triangle (PS)      // -> ImGuiNavInput_Input
	{ ImGuiKey_GamepadFaceDown,		"Gamepad Face Down", },    // A (Xbox)             B (Switch)   Cross (PS)         // -> ImGuiNavInput_Activate
	{ ImGuiKey_GamepadDpadLeft,		"Gamepad Dpad Left", },    // D-pad Left                                           // -> ImGuiNavInput_DpadLeft
	{ ImGuiKey_GamepadDpadRight,	"Gamepad Dpad Right", },   // D-pad Right                                          // -> ImGuiNavInput_DpadRight
	{ ImGuiKey_GamepadDpadUp,		"Gamepad Dpad Up", },      // D-pad Up                                             // -> ImGuiNavInput_DpadUp
	{ ImGuiKey_GamepadDpadDown,		"Gamepad Dpad Down", },    // D-pad Down                                           // -> ImGuiNavInput_DpadDown
	{ ImGuiKey_GamepadL1,			"Gamepad L1", },           // L Bumper (Xbox)      L (Switch)   L1 (PS)             // -> ImGuiNavInput_FocusPrev + ImGuiNavInput_TweakSlow
	{ ImGuiKey_GamepadR1,			"Gamepad R1", },           // R Bumper (Xbox)      R (Switch)   R1 (PS)             // -> ImGuiNavInput_FocusNext + ImGuiNavInput_TweakFast
	{ ImGuiKey_GamepadL2,			"Gamepad L2", },           // L Trigger (Xbox)     ZL (Switch)  L2 (PS) [Analog]
	{ ImGuiKey_GamepadR2,			"Gamepad R2", },           // R Trigger (Xbox)     ZR (Switch)  R2 (PS) [Analog]
	{ ImGuiKey_GamepadL3,			"Gamepad L3", },           // L Thumbstick (Xbox)  L3 (Switch)  L3 (PS)
	{ ImGuiKey_GamepadR3,			"Gamepad R3", },           // R Thumbstick (Xbox)  R3 (Switch)  R3 (PS)
	{ ImGuiKey_GamepadLStickLeft,	"Gamepad LStick Left", },  // [Analog]                                             // -> ImGuiNavInput_LStickLeft
	{ ImGuiKey_GamepadLStickRight,	"Gamepad LStick Right", }, // [Analog]                                             // -> ImGuiNavInput_LStickRight
	{ ImGuiKey_GamepadLStickUp,		"Gamepad LStick Up", },    // [Analog]                                             // -> ImGuiNavInput_LStickUp
	{ ImGuiKey_GamepadLStickDown,	"Gamepad LStick Down", },  // [Analog]                                             // -> ImGuiNavInput_LStickDown
	{ ImGuiKey_GamepadRStickLeft,	"Gamepad RStick Left", },  // [Analog]
	{ ImGuiKey_GamepadRStickRight,	"Gamepad RStick Right", }, // [Analog]
	{ ImGuiKey_GamepadRStickUp,		"Gamepad RStick Up", },    // [Analog]
	{ ImGuiKey_GamepadRStickDown,	"Gamepad RStick Down", },  // [Analog]

	{ ImGuiKey_ModCtrl,  "Ctrl", "Control", },
	{ ImGuiKey_ModShift, "Shift", },
	{ ImGuiKey_ModAlt, 	 "Alt", },
	{ ImGuiKey_ModSuper, "Super", },
};

static constexpr bool CompileTimeValidateNamedImGuiKeyInfoTable(const ImGuiKeyInfo* namedImGuiKeyInfoTable)
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

static constexpr const char* ImGuiMouseButtonToCString(ImGuiMouseButton button)
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

	auto append = [&](const char* stringToAppend)
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
		return ToShortcutString(binding.Keyboard.Key, binding.Keyboard.Modifiers);
	else if (binding.Type == InputBindingType::Mouse)
	{
		InputFormatBuffer out {};
		::strcpy_s(out.Data, ImGuiMouseButtonToCString(binding.Mouse.Button));
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
	static bool Internal_IsKeyDownAfterAllModifiers(ImGuiKey key)
	{
		const f32 keyDuration = ImGui::IsNamedKey(key) ? ImGui::GetKeyData(key)->DownDuration : 0.0f;
		return ThisFrameInputExData.TimeSinceLastModifiersChange >= keyDuration;
	}

	static bool Internal_AreModifiersDownFirst(ImGuiKey key, ImGuiModFlags modifiers)
	{
		const f32 keyDuration = ImGui::IsNamedKey(key) ? ImGui::GetKeyData(key)->DownDuration : 0.0f;
		bool allLonger = true;
		ForEachImGuiKeyInKeyModFlags(modifiers, [&](ImGuiKey modifierKey) { allLonger &= (ImGui::GetKeyData(modifierKey)->DownDuration >= keyDuration); });
		return allLonger;
	}

	bool AreAllModifiersDown(ImGuiModFlags modifiers)
	{
		return ((GImGui->IO.KeyMods & modifiers) == modifiers);
	}

	bool AreOnlyModifiersDown(ImGuiModFlags modifiers)
	{
		return (GImGui->IO.KeyMods == modifiers);
	}

	// BUG: Regular bindings with modifier keys as primary keys aren't triggered correctly
	bool IsDown(const InputBinding& binding, InputModifierBehavior behavior)
	{
		if (binding.Type == InputBindingType::Keyboard)
		{
			if (behavior == InputModifierBehavior::Strict)
				return ImGui::IsKeyDown(binding.Keyboard.Key) && AreOnlyModifiersDown(binding.Keyboard.Modifiers) && Internal_IsKeyDownAfterAllModifiers(binding.Keyboard.Key);
			else
				return ImGui::IsKeyDown(binding.Keyboard.Key) && AreAllModifiersDown(binding.Keyboard.Modifiers) && Internal_AreModifiersDownFirst(binding.Keyboard.Key, binding.Keyboard.Modifiers);
		}
		else if (binding.Type == InputBindingType::Mouse)
		{
			return ImGui::IsMouseDown(binding.Mouse.Button);
		}
		else
		{
			assert(!"Invalid binding type");
			return false;
		}
	}

	bool IsPressed(const InputBinding& binding, bool repeat, InputModifierBehavior behavior)
	{
		if (binding.Type == InputBindingType::Keyboard)
		{
			// NOTE: Still have to explictily check the modifier hold durations here in case of repeat
			if (behavior == InputModifierBehavior::Strict)
				return ImGui::IsKeyPressed(binding.Keyboard.Key, repeat) && AreOnlyModifiersDown(binding.Keyboard.Modifiers) && Internal_IsKeyDownAfterAllModifiers(binding.Keyboard.Key);
			else
				return ImGui::IsKeyPressed(binding.Keyboard.Key, repeat) && AreAllModifiersDown(binding.Keyboard.Modifiers) && Internal_AreModifiersDownFirst(binding.Keyboard.Key, binding.Keyboard.Modifiers);
		}
		else if (binding.Type == InputBindingType::Mouse)
		{
			return ImGui::IsMouseClicked(binding.Mouse.Button, repeat);
		}
		else
		{
			assert(!"Invalid binding type");
			return false;
		}
	}

	bool IsAnyDown(const MultiInputBinding& binding, InputModifierBehavior behavior)
	{
		for (size_t i = 0; i < binding.Count; i++)
			if (IsDown(binding.Slots[i], behavior))
				return true;
		return false;
	}

	bool IsAnyPressed(const MultiInputBinding& binding, bool repeat, InputModifierBehavior behavior)
	{
		for (size_t i = 0; i < binding.Count; i++)
			if (IsPressed(binding.Slots[i], repeat, behavior))
				return true;
		return false;
	}
}
