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

	const bool targetIsLess = (target <= *inOutCurrent);
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
	inline std::string_view GetClipboardTextView() { const char* v = ImGui::GetClipboardText(); return (v != nullptr) ? std::string_view(v) : ""; }

	inline ImGuiID GetID(std::string_view stringViewID) { return ImGui::GetID(StringViewStart(stringViewID), StringViewEnd(stringViewID)); }
	inline void TextUnformatted(std::string_view stringView) { ImGui::TextUnformatted(StringViewStart(stringView), StringViewEnd(stringView)); }

	inline f32 DeltaTime() { return GImGui->IO.DeltaTime; }

	inline void AnimateExponential(f32* inOutCurrent, f32 target, f32 animationSpeed) { AnimateExponentialF32(inOutCurrent, target, animationSpeed, DeltaTime()); }
	inline void AnimateExponential(vec2* inOutCurrent, vec2 target, f32 animationSpeed) { AnimateExponentialVec2(inOutCurrent, target, animationSpeed, DeltaTime()); }

	inline Rect GetItemRect() { return Rect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()); }

	ImVec2 CalcTextSize(std::string_view text, bool hide_text_after_double_hash = false, float wrap_width = -1.0f);

	inline ImU32 ColorU32WithAlpha(ImU32 colorU32, f32 alphaFactor) { ImVec4 v4 = ImGui::ColorConvertU32ToFloat4(colorU32); v4.w *= alphaFactor; return ImGui::ColorConvertFloat4ToU32(v4); }
	inline bool ColorEdit3_U32(const char* label, u32* color, ImGuiColorEditFlags flags = 0) { ImVec4 v3 = ImGui::ColorConvertU32ToFloat4(*color); if (ImGui::ColorEdit3(label, &v3.x, flags)) { *color = ImGui::ColorConvertFloat4ToU32(v3); return true; } return false; }
	inline bool ColorEdit4_U32(const char* label, u32* color, ImGuiColorEditFlags flags = 0) { ImVec4 v4 = ImGui::ColorConvertU32ToFloat4(*color); if (ImGui::ColorEdit4(label, &v4.x, flags)) { *color = ImGui::ColorConvertFloat4ToU32(v4); return true; } return false; }

	void AddTextAligned(ImDrawList* drawList, Rect rectToAlignWithin, vec2 normalizedAlignment, std::string_view text);
	void AddTextAligned(ImDrawList* drawList, Rect rectToAlignWithin, vec2 normalizedAlignment, std::string_view text, Rect clippingRect);
	void AddTextCentered(ImDrawList* drawList, Rect rectToCenterWithin, std::string_view text);
	void AddTextCentered(ImDrawList* drawList, Rect rectToCenterWithin, std::string_view text, Rect clippingRect);

	void AddTextWithDropShadow(ImDrawList* drawList, vec2 textPosition, u32 textColor, std::string_view text, u32 shadowColor = 0xFF000000, vec2 shadowOffset = vec2(1.0f));
	void AddTextWithDropShadow(ImDrawList* drawList, const ImFont* font, float fontSize, vec2 textPosition, u32 textColor, std::string_view text, float wrap_width = 0.0f, const ImVec4* cpu_fine_clip_rect = nullptr, u32 shadowColor = 0xFF000000, vec2 shadowOffset = vec2(1.0f));

	bool InputText(const char* label, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
	bool InputTextMultiline(const char* label, std::string* str, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
	bool InputTextMultilineWithHint(const char* label, const char* hinit, std::string* str, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
	bool InputTextWithHint(const char* label, const char* hint, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
	bool PathInputTextWithHint(const char* label, const char* hint, std::string* str, ImGuiInputTextFlags flags = 0);

	struct PathInputTextWithBrowserButtonResult { bool InputTextEdited; bool BrowseButtonClicked; };
	PathInputTextWithBrowserButtonResult PathInputTextWithHintAndBrowserDialogButton(const char* label, const char* hint, std::string* str, ImGuiInputTextFlags flags = 0);

	// NOTE: As noted in https://github.com/ocornut/imgui/issues/3556
	inline bool IsItemBeingEditedAsText() { return ImGui::IsItemActive() && ImGui::TempInputIsActive(ImGui::GetActiveID()); }
	inline void SetDragScalarMouseCursor() { if (!IsItemBeingEditedAsText() && (ImGui::IsItemActive() || ImGui::IsItemHovered())) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW); }

	template <typename EnumType>
	bool ComboEnum(const char* label, EnumType* inOutEnum, const char* const(&enumToCStrLookupTable)[EnumCount<EnumType>], ImGuiComboFlags flags = ImGuiComboFlags_None)
	{
		static_assert(std::is_enum_v<EnumType> && sizeof(EnumType) <= sizeof(i32));
		static constexpr i32 minIndex = static_cast<i32>(0), maxIndex = static_cast<i32>(EnumCount<EnumType>);
		auto indexToCStrOrNull = [&](i32 index) { return (index < maxIndex) ? enumToCStrLookupTable[index] : nullptr; };

		const i32 inIndex = static_cast<i32>(*inOutEnum);
		bool outValueWasChanged = false;

		if (const char* preview = indexToCStrOrNull(inIndex); ImGui::BeginCombo(label, (preview == nullptr) ? "" : preview, flags))
		{
			for (i32 i = minIndex; i < maxIndex; i++)
			{
				if (const char* itemLabel = indexToCStrOrNull(i); itemLabel != nullptr)
				{
					const bool isSelected = (i == inIndex);
					if (ImGui::Selectable(itemLabel, isSelected)) { *inOutEnum = static_cast<EnumType>(i); outValueWasChanged = true; }
					if (isSelected) { ImGui::SetItemDefaultFocus(); }
				}
			}
			ImGui::EndCombo();
		}

		return outValueWasChanged;
	}

	void DrawStar(ImDrawList* drawList, vec2 center, f32 outerRadius, f32 innerRadius, u32 color, f32 thicknessOrZeroToFill = 1.0f);

	namespace Property
	{
		inline bool BeginTable(ImGuiTableFlags flags = ImGuiTableFlags_None, vec2 outerSize = {}, f32 innerWidth = {}) { return ImGui::BeginTable("PropertyTable", 2, ImGuiTableFlags_NoSavedSettings | flags); }
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
		inline void PropertyTreeNodeValueFunc(const char* nodeLabel, ImGuiTreeNodeFlags nodeFlags, Func valueFunc)
		{
			Property::BeginPropertyColumn();
			ImGui::AlignTextToFramePadding();
			const bool treeNodeOpen = ImGui::TreeNodeEx(nodeLabel, ImGuiTreeNodeFlags_SpanAvailWidth | nodeFlags);
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
