#pragma once
#include "core_types.h"
#include "imgui/3rdparty/imgui.h"
#include "imgui/3rdparty/imgui_internal.h"
#include <string>

constexpr void AnimateExponentialF32(f32* inOutCurrent, f32 target, f32 animationSpeed, f32 deltaTime)
{
	// NOTE: If no time elapsed then no animation should take place
	if (deltaTime <= 0.0f) { return; }
	// NOTE: If the animation speed is "disabled" however it should always snap to its target immediately
	if (animationSpeed <= 0.0f) { *inOutCurrent = target; return; }

	const b8 targetIsLess = (target <= *inOutCurrent);
	*inOutCurrent += (target - *inOutCurrent) * animationSpeed * deltaTime;

	// NOTE Prevent overshooting the target resulting in unstable 'jittering' for high speed values
	*inOutCurrent = targetIsLess ? ClampBot(*inOutCurrent, target) : ClampTop(*inOutCurrent, target);
}

constexpr void AnimateExponentialVec2(vec2* inOutCurrent, vec2 target, f32 animationSpeed, f32 deltaTime)
{
	AnimateExponentialF32(&inOutCurrent->x, target.x, animationSpeed, deltaTime);
	AnimateExponentialF32(&inOutCurrent->y, target.y, animationSpeed, deltaTime);
}

namespace ImGui
{
	constexpr const char* StringViewStart(std::string_view stringView) { return stringView.data(); }
	constexpr const char* StringViewEnd(std::string_view stringView) { return stringView.data() + stringView.size(); }

	// NOTE: Gotta be careful not to accidentally construct a string_view from a nullptr!
	inline std::string_view GetClipboardTextView() { cstr v = ImGui::GetClipboardText(); return (v != nullptr) ? std::string_view(v) : ""; }

	inline ImGuiID GetID(std::string_view stringViewID) { return ImGui::GetID(StringViewStart(stringViewID), StringViewEnd(stringViewID)); }
	inline void TextUnformatted(std::string_view stringView) { ImGui::TextUnformatted(StringViewStart(stringView), StringViewEnd(stringView)); }

	inline f32 DeltaTime() { return GImGui->IO.DeltaTime; }

	inline void AnimateExponential(f32* inOutCurrent, f32 target, f32 animationSpeed) { AnimateExponentialF32(inOutCurrent, target, animationSpeed, DeltaTime()); }
	inline void AnimateExponential(vec2* inOutCurrent, vec2 target, f32 animationSpeed) { AnimateExponentialVec2(inOutCurrent, target, animationSpeed, DeltaTime()); }

	void UpdateSmoothScrollWindow(ImGuiWindow* window = nullptr, f32 animationSpeed = 20.0f);

	inline Rect GetItemRect() { return Rect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()); }
	ImVec2 CalcTextSize(std::string_view text, b8 hide_text_after_double_hash = false, f32 wrap_width = -1.0f);

	inline ImU32 ColorU32WithAlpha(ImU32 colorU32, f32 alphaFactor) { ImVec4 v4 = ImGui::ColorConvertU32ToFloat4(colorU32); v4.w *= alphaFactor; return ImGui::ColorConvertFloat4ToU32(v4); }
	inline ImU32 ColorU32WithNewAlpha(ImU32 colorU32, f32 newAlpha) { ImVec4 v4 = ImGui::ColorConvertU32ToFloat4(colorU32); v4.w = newAlpha; return ImGui::ColorConvertFloat4ToU32(v4); }
	inline b8 ColorEdit3_U32(cstr label, u32* color, ImGuiColorEditFlags flags = 0) { ImVec4 v3 = ImGui::ColorConvertU32ToFloat4(*color); if (ImGui::ColorEdit3(label, &v3.x, flags)) { *color = ImGui::ColorConvertFloat4ToU32(v3); return true; } return false; }
	inline b8 ColorEdit4_U32(cstr label, u32* color, ImGuiColorEditFlags flags = 0) { ImVec4 v4 = ImGui::ColorConvertU32ToFloat4(*color); if (ImGui::ColorEdit4(label, &v4.x, flags)) { *color = ImGui::ColorConvertFloat4ToU32(v4); return true; } return false; }

	void AddTextAligned(ImDrawList* drawList, Rect rectToAlignWithin, vec2 normalizedAlignment, std::string_view text);
	void AddTextAligned(ImDrawList* drawList, Rect rectToAlignWithin, vec2 normalizedAlignment, std::string_view text, Rect clippingRect);
	void AddTextCentered(ImDrawList* drawList, Rect rectToCenterWithin, std::string_view text);
	void AddTextCentered(ImDrawList* drawList, Rect rectToCenterWithin, std::string_view text, Rect clippingRect);

	void AddTextWithDropShadow(ImDrawList* drawList, vec2 textPosition, u32 textColor, std::string_view text, u32 shadowColor = 0xFF000000, vec2 shadowOffset = vec2(1.0f));
	void AddTextWithDropShadow(ImDrawList* drawList, const ImFont* font, f32 fontSize, vec2 textPosition, u32 textColor, std::string_view text, f32 wrap_width = 0.0f, const ImVec4* cpu_fine_clip_rect = nullptr, u32 shadowColor = 0xFF000000, vec2 shadowOffset = vec2(1.0f));

	b8 InputText(cstr label, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
	b8 InputTextMultiline(cstr label, std::string* str, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
	b8 InputTextMultilineWithHint(cstr label, cstr hinit, std::string* str, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
	b8 InputTextWithHint(cstr label, cstr hint, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
	b8 PathInputTextWithHint(cstr label, cstr hint, std::string* str, ImGuiInputTextFlags flags = 0);

	b8 ButtonPlusMinusScalar(char plusOrMinus, ImGuiDataType dataType, void* inOutValue, void* step, void* stepFast, vec2 buttonSize = {});
	b8 ButtonPlusMinusFloat(char plusOrMinus, f32* inOutValue, f32 step, f32 stepFast, vec2 buttonSize = {});
	b8 ButtonPlusMinusInt(char plusOrMinus, i32* inOutValue, i32 step, i32 stepFast, vec2 buttonSize = {});

	struct InputScalarWithButtonsResult { u32 ValueChanged, ButtonClicked, IsTextItemActive; Rect TextItemRect[4]; };
	struct InputScalarWithButtonsExData { u32 TextColor; b8 SpinButtons; };
	InputScalarWithButtonsResult InputScalar_WithExtraStuff(cstr label, ImGuiDataType data_type, void* p_data, const void* p_step = nullptr, const void* p_step_fast = nullptr, cstr format = nullptr, ImGuiInputTextFlags flags = 0, const InputScalarWithButtonsExData* ex_data = nullptr);
	InputScalarWithButtonsResult InputScalarN_WithExtraStuff(cstr label, ImGuiDataType data_type, void* p_data, i32 components, const void* p_step = nullptr, const void* p_step_fast = nullptr, cstr format = nullptr, ImGuiInputTextFlags flags = 0, const InputScalarWithButtonsExData* ex_data = nullptr);

	b8 SpinScalar(cstr label, ImGuiDataType data_type, void* p_data, const void* p_step, const void* p_step_fast, cstr format, ImGuiInputTextFlags flags);
	b8 SpinInt(cstr label, i32* v, i32 step = 1, i32 step_fast = 100, ImGuiInputTextFlags flags = 0);
	b8 SpinFloat(cstr label, f32* v, f32 step = 0.0f, f32 step_fast = 0.0f, cstr format = "%.3f", ImGuiInputTextFlags flags = 0);
	b8 SpinDouble(cstr label, f64* v, f64 step = 0.0, f64 step_fast = 0.0, cstr format = "%.6f", ImGuiInputTextFlags flags = 0);

	struct PathInputTextWithBrowserButtonResult { b8 InputTextEdited; b8 BrowseButtonClicked; };
	PathInputTextWithBrowserButtonResult PathInputTextWithHintAndBrowserDialogButton(cstr label, cstr hint, std::string* str, ImGuiInputTextFlags flags = 0);

	// NOTE: As noted in https://github.com/ocornut/imgui/issues/3556
	inline b8 IsItemBeingEditedAsText() { return ImGui::IsItemActive() && ImGui::TempInputIsActive(ImGui::GetActiveID()); }

	// HACK: Usually InputScalar() and co only return true if the actual numeric value was changed (not just any letter typed)
	//		 this behavior however directly conflicts with multi object editing ("MixedValues") as the value being compared for this check only belongs to a *single* object.
	//		 This function therefore allows detecting changes even if the entered value is "the same"
	inline b8 WasInputTextStateEditedThisFrame() { const auto* state = ImGui::GetInputTextState(ImGui::GetActiveID()); return (state != nullptr && state->Edited); }

	inline void SetDragScalarMouseCursor() { if (!IsItemBeingEditedAsText() && (ImGui::IsItemActive() || ImGui::IsItemHovered())) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW); }

	template <typename EnumType>
	b8 ComboEnum(cstr label, EnumType* inOutEnum, cstr const(&enumToCStrLookupTable)[EnumCount<EnumType>], ImGuiComboFlags flags = ImGuiComboFlags_None)
	{
		static_assert(std::is_enum_v<EnumType> && sizeof(EnumType) <= sizeof(i32));
		static constexpr i32 minIndex = static_cast<i32>(0), maxIndex = static_cast<i32>(EnumCount<EnumType>);
		auto indexToCStrOrNull = [&](i32 index) { return (index < maxIndex) ? enumToCStrLookupTable[index] : nullptr; };

		const i32 inIndex = static_cast<i32>(*inOutEnum);
		b8 outValueWasChanged = false;

		if (cstr preview = indexToCStrOrNull(inIndex); ImGui::BeginCombo(label, (preview == nullptr) ? "" : preview, flags))
		{
			for (i32 i = minIndex; i < maxIndex; i++)
			{
				if (cstr itemLabel = indexToCStrOrNull(i); itemLabel != nullptr)
				{
					const b8 isSelected = (i == inIndex);
					if (ImGui::Selectable(itemLabel, isSelected)) { *inOutEnum = static_cast<EnumType>(i); outValueWasChanged = true; }
					if (isSelected) { ImGui::SetItemDefaultFocus(); }
				}
			}
			ImGui::EndCombo();
		}

		return outValueWasChanged;
	}

	struct MultiWidgetIt { i32 Index; };
	struct MultiWidgetResult { b8 ValueChanged; i32 ChangedIndex; };

	template <typename Func>
	inline MultiWidgetResult SameLineMultiWidget(i32 itemCount, Func onWidgetFunc)
	{
		i32 changedIndex = -1;
		ImGui::BeginGroup();
		ImGui::PushMultiItemsWidths(itemCount, ImGui::CalcItemWidth());
		for (i32 i = 0; i < itemCount; i++)
		{
			if (i > 0)
				ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
			if (onWidgetFunc(MultiWidgetIt { i }))
				changedIndex = i;
			ImGui::PopItemWidth();
		}
		ImGui::EndGroup();
		return MultiWidgetResult { (changedIndex != -1), changedIndex };
	}

	void DrawStar(ImDrawList* drawList, vec2 center, f32 outerRadius, f32 innerRadius, u32 color, f32 thicknessOrZeroToFill = 1.0f);

	namespace Property
	{
		inline b8 BeginTable(ImGuiTableFlags flags = ImGuiTableFlags_None, vec2 outerSize = {}, f32 innerWidth = {}) { return ImGui::BeginTable("PropertyTable", 2, ImGuiTableFlags_NoSavedSettings | flags); }
		inline void EndTable() { ImGui::EndTable(); }

		template <typename Func>
		inline void Table(ImGuiTableFlags flags, Func func, vec2 outerSize = {}, f32 innerWidth = {}) { if (Property::BeginTable(flags, outerSize, innerWidth)) { func(); Property::EndTable(); } }

		inline void BeginPropertyColumn() { ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); }
		inline void EndPropertyColumn() { return; }

		inline void BeginValueColumn() { ImGui::TableSetColumnIndex(1); }
		inline void EndValueColumn() { return; }

		template <typename Func>
		inline void Property(Func propertyFunc) { Property::BeginPropertyColumn(); propertyFunc(); Property::EndPropertyColumn(); }
		inline void PropertyText(std::string_view propertyText) { Property::Property([&] { ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted(propertyText); }); }

		template <typename Func>
		inline void Value(Func valueFunc) { Property::BeginValueColumn(); valueFunc(); Property::EndValueColumn(); }

		template <typename Func>
		inline void PropertyTextValueFunc(std::string_view propertyText, Func valueFunc) { Property::PropertyText(propertyText); Property::Value(valueFunc); }

		template <typename Func>
		inline void PropertyTreeNodeValueFunc(cstr nodeLabel, ImGuiTreeNodeFlags nodeFlags, Func valueFunc)
		{
			Property::BeginPropertyColumn();
			ImGui::AlignTextToFramePadding();
			const b8 treeNodeOpen = ImGui::TreeNodeEx(nodeLabel, ImGuiTreeNodeFlags_SpanAvailWidth | nodeFlags);
			Property::EndPropertyColumn();

			if (treeNodeOpen)
			{
				Property::BeginValueColumn();
				valueFunc();
				Property::EndValueColumn();

				if (!(nodeFlags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
					ImGui::TreePop();
			}
		}
	}
}
