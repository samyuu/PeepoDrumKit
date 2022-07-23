#pragma once
#include "core_types.h"
#include "imgui/3rdparty/imgui.h"
#include "imgui/3rdparty/imgui_internal.h"
#include <string>

enum class InputBindingType : u8
{
	None,
	Keyboard,
	Mouse,
	Count
};

struct InputBinding
{
	InputBindingType Type = InputBindingType::None;
	struct
	{
		ImGuiKey Key = ImGuiKey_None;
		ImGuiModFlags Modifiers = ImGuiModFlags_None;
	} Keyboard = {};
	struct
	{
		ImGuiMouseButton Button = ImGuiMouseButton_Left;
	} Mouse = {};

	constexpr InputBinding() = default;
	explicit constexpr InputBinding(ImGuiKey key, ImGuiModFlags modifiers) : Type(InputBindingType::Keyboard), Keyboard({ key, modifiers }) {}
	explicit constexpr InputBinding(ImGuiMouseButton mouseButton) : Type(InputBindingType::Mouse), Mouse({ mouseButton }) {}

	constexpr bool operator!=(const InputBinding& other) const { return !(*this == other); }
	constexpr bool operator==(const InputBinding& other) const
	{
		return (Type != other.Type) ? false :
			(Type == InputBindingType::None) ? true :
			(Type == InputBindingType::Keyboard) ? (Keyboard.Key == other.Keyboard.Key) && (Keyboard.Modifiers == other.Keyboard.Modifiers) :
			(Type == InputBindingType::Mouse) ? (Mouse.Button == other.Mouse.Button) : false;
	}
};

// NOTE: Separate "constructor-functions" beacuse a default ImGuiModFlags can't be used inside the constructor itself due to non "class enum" type ambiguity between ImGuiKey and ImGuiMouseButton
constexpr InputBinding KeyBinding(ImGuiKey key, ImGuiModFlags modifiers = ImGuiModFlags_None) { return InputBinding(key, modifiers); }
constexpr InputBinding MouseBinding(ImGuiMouseButton mouseButton) { return InputBinding(mouseButton); }

struct MultiInputBinding
{
	static constexpr size_t MaxCount = 8;

	u8 Count = 0;
	InputBinding Slots[MaxCount] = {};

	constexpr MultiInputBinding() = default;
	constexpr MultiInputBinding(InputBinding primary) : Count(1), Slots { primary } {}
	constexpr MultiInputBinding(InputBinding primary, InputBinding secondary) : Count(2), Slots { primary, secondary } {}
	constexpr MultiInputBinding(InputBinding a, InputBinding b, InputBinding c) : Count(3), Slots { a, b, c } {}
	constexpr MultiInputBinding(InputBinding a, InputBinding b, InputBinding c, InputBinding d) : Count(4), Slots { a, b, c, d } {}

	constexpr auto begin() const { return Slots; }
	constexpr auto end() const { return Slots + Count; }
};

enum class InputModifierBehavior
{
	// NOTE: Bindings are only triggered if the *exact* key modifiers are held down; no more, no less.
	//		 Intended for most standard commands, to avoid conflicts between multiple same key bindings only differing in their modifiers
	Strict,
	// NOTE: Bindings are triggered if all of the modifiers are held down even if unspecified modifiers are held down too.
	//		 Intended for mostly single key bindings that make use of modifiers to change their behavior (i.e. holding shift/alt to increase/decrease step distance)
	Relaxed,
};

struct InputFormatBuffer { char Data[128]; };
// NOTE: Specifically to be displayed inside menu items
InputFormatBuffer ToShortcutString(ImGuiKey key);
InputFormatBuffer ToShortcutString(ImGuiKey key, ImGuiModFlags modifiers);
InputFormatBuffer ToShortcutString(const InputBinding& binding);
InputFormatBuffer ToShortcutString(const MultiInputBinding& binding);

void InputBindingToStorageString(const MultiInputBinding& in, std::string& stringToAppendTo);
bool InputBindingFromStorageString(std::string_view stringToParse, MultiInputBinding& out);

// NOTE: Must be called once at the start / end of the frame by the backend!
void ImGui_UpdateInternalInputExtraDataAtStartOfFrame();
void ImGui_UpdateInternalInputExtraDataAtEndOfFrame();

namespace ImGui
{
	bool AreAllModifiersDown(ImGuiModFlags modifiers);
	bool AreOnlyModifiersDown(ImGuiModFlags modifiers);

	bool IsDown(const InputBinding& binding, InputModifierBehavior behavior = InputModifierBehavior::Strict);
	bool IsPressed(const InputBinding& binding, bool repeat = true, InputModifierBehavior behavior = InputModifierBehavior::Strict);

	bool IsAnyDown(const MultiInputBinding& binding, InputModifierBehavior behavior = InputModifierBehavior::Strict);
	bool IsAnyPressed(const MultiInputBinding& binding, bool repeat = true, InputModifierBehavior behavior = InputModifierBehavior::Strict);
}
