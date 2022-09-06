#include "imgui_common.h"
#include "core_io.h"

namespace ImGui
{
	struct InputTextStdStringCallbackUserData
	{
		std::string* Str;
		ImGuiInputTextCallback ChainCallback;
		void* ChainCallbackUserData;
	};

	static int InputTextStdStringCallback(ImGuiInputTextCallbackData* data)
	{
		InputTextStdStringCallbackUserData* user_data = (InputTextStdStringCallbackUserData*)data->UserData;
		if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
		{
			std::string* str = user_data->Str;
			IM_ASSERT(data->Buf == str->c_str());
			str->resize(data->BufTextLen);
			data->Buf = (char*)str->c_str();
		}
		else if (user_data->ChainCallback)
		{
			data->UserData = user_data->ChainCallbackUserData;
			return user_data->ChainCallback(data);
		}
		return 0;
	}

	void UpdateSmoothScrollWindow(ImGuiWindow* window, f32 animationSpeed)
	{
		struct VoidPtrStorage { f32 ScrollYTarget, ScrollYLastFrame; };
		static_assert(sizeof(void*) == sizeof(VoidPtrStorage));

		window = (window == nullptr) ? ImGui::GetCurrentWindowRead() : window;
		auto* storage = reinterpret_cast<VoidPtrStorage*>(window->StateStorage.GetVoidPtrRef(ImGui::GetID("SmoothScrollStorage")));
		if (storage == nullptr) { assert(false); return; }

		f32& scrollYTarget = storage->ScrollYTarget;
		f32& scrollYCurrentLastFrame = storage->ScrollYLastFrame;
		f32& scrollYCurrent = window->Scroll.y;
		AnimateExponential(&scrollYCurrent, scrollYTarget, animationSpeed);

		if (ImGui::GetCurrentContext()->WheelingWindow == window)
		{
			if (!ImGui::GetIO().KeyCtrl)
			{
				// HACK: Take from "imgui.cpp -> ImGui::UpdateMouseWheel() -> Vertical Mouse Wheel scrolling"
				const f32 max_step = window->InnerRect.GetHeight() * 0.67f;
				const f32 scroll_step = ImFloor(ImMin(5 * window->CalcFontSize(), max_step));
				const f32 wheel_y = ImGui::GetIO().MouseWheel;

				// HACK: Adjust for the scroll amount that was already added by the native imgui scroll handling
				scrollYTarget -= wheel_y * scroll_step;
				scrollYTarget -= wheel_y * scroll_step;
				scrollYTarget = Clamp(scrollYTarget, 0.0f, window->ScrollMax.y);
			}
		}
		else
		{
			// HACK: Try to detect and adjust for external scroll changes (such as grabbing the scrollbar)
			if (!ApproxmiatelySame(scrollYCurrentLastFrame, scrollYCurrent))
				scrollYTarget = scrollYCurrent;
		}
		scrollYCurrentLastFrame = scrollYCurrent;
	}

	ImVec2 CalcTextSize(std::string_view text, b8 hide_text_after_double_hash, f32 wrap_width)
	{
		return ImGui::CalcTextSize(StringViewStart(text), StringViewEnd(text), hide_text_after_double_hash, wrap_width);
	}

	void AddTextAligned(ImDrawList* drawList, Rect rectToAlignWithin, vec2 normalizedAlignment, std::string_view text)
	{
		ImGui::RenderTextClippedEx(drawList, rectToAlignWithin.TL, rectToAlignWithin.BR, StringViewStart(text), StringViewEnd(text), nullptr, normalizedAlignment, nullptr);
	}

	void AddTextAligned(ImDrawList* drawList, Rect rectToAlignWithin, vec2 normalizedAlignment, std::string_view text, Rect clippingRect)
	{
		const ImRect imClippingRect = { clippingRect.TL, clippingRect.BR };
		ImGui::RenderTextClippedEx(drawList, rectToAlignWithin.TL, rectToAlignWithin.BR, StringViewStart(text), StringViewEnd(text), nullptr, normalizedAlignment, &imClippingRect);
	}

	void AddTextCentered(ImDrawList* drawList, Rect rectToCenterWithin, std::string_view text)
	{
		ImGui::RenderTextClippedEx(drawList, rectToCenterWithin.TL, rectToCenterWithin.BR, StringViewStart(text), StringViewEnd(text), nullptr, { 0.5f, 0.5f }, nullptr);
	}

	void AddTextCentered(ImDrawList* drawList, Rect rectToCenterWithin, std::string_view text, Rect clippingRect)
	{
		const ImRect imClippingRect = { clippingRect.TL, clippingRect.BR };
		ImGui::RenderTextClippedEx(drawList, rectToCenterWithin.TL, rectToCenterWithin.BR, StringViewStart(text), StringViewEnd(text), nullptr, { 0.5f, 0.5f }, &imClippingRect);
	}

	void AddTextWithDropShadow(ImDrawList* drawList, vec2 textPosition, u32 textColor, std::string_view text, u32 shadowColor, vec2 shadowOffset)
	{
		drawList->AddText(textPosition + shadowOffset, shadowColor, StringViewStart(text), StringViewEnd(text));
		drawList->AddText(textPosition, textColor, StringViewStart(text), StringViewEnd(text));
	}

	void AddTextWithDropShadow(ImDrawList* drawList, const ImFont* font, f32 fontSize, vec2 textPosition, u32 textColor, std::string_view text, f32 wrap_width, const ImVec4* cpu_fine_clip_rect, u32 shadowColor, vec2 shadowOffset)
	{
		drawList->AddText(font, fontSize, textPosition + shadowOffset, shadowColor, StringViewStart(text), StringViewEnd(text), wrap_width, cpu_fine_clip_rect);
		drawList->AddText(font, fontSize, textPosition, textColor, StringViewStart(text), StringViewEnd(text), wrap_width, cpu_fine_clip_rect);
	}

	b8 InputText(cstr label, std::string* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
	{
		IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
		flags |= ImGuiInputTextFlags_CallbackResize;

		InputTextStdStringCallbackUserData cb_user_data;
		cb_user_data.Str = str;
		cb_user_data.ChainCallback = callback;
		cb_user_data.ChainCallbackUserData = user_data;
		return ImGui::InputText(label, str->data(), str->capacity() + 1, flags, InputTextStdStringCallback, &cb_user_data);
	}

	b8 InputTextMultiline(cstr label, std::string* str, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
	{
		IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
		flags |= ImGuiInputTextFlags_CallbackResize;

		InputTextStdStringCallbackUserData cb_user_data;
		cb_user_data.Str = str;
		cb_user_data.ChainCallback = callback;
		cb_user_data.ChainCallbackUserData = user_data;
		return ImGui::InputTextMultiline(label, str->data(), str->capacity() + 1, size, flags, InputTextStdStringCallback, &cb_user_data);
	}

	b8 InputTextMultilineWithHint(cstr label, cstr hint, std::string* str, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
	{
		IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
		flags |= ImGuiInputTextFlags_CallbackResize;

		InputTextStdStringCallbackUserData cb_user_data;
		cb_user_data.Str = str;
		cb_user_data.ChainCallback = callback;
		cb_user_data.ChainCallbackUserData = user_data;
		return ImGui::InputTextEx(label, hint, str->data(), static_cast<int>(str->capacity() + 1), size, flags | ImGuiInputTextFlags_Multiline, InputTextStdStringCallback, &cb_user_data);
	}

	b8 InputTextWithHint(cstr label, cstr hint, std::string* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
	{
		IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
		flags |= ImGuiInputTextFlags_CallbackResize;

		InputTextStdStringCallbackUserData cb_user_data;
		cb_user_data.Str = str;
		cb_user_data.ChainCallback = callback;
		cb_user_data.ChainCallbackUserData = user_data;
		return ImGui::InputTextWithHint(label, hint, str->data(), str->capacity() + 1, flags, InputTextStdStringCallback, &cb_user_data);
	}

	static int ValidPathCharTextCallbackFilter(ImGuiInputTextCallbackData* data)
	{
		if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter)
		{
			if (data->EventChar <= U8Max && !Path::IsValidPathChar(static_cast<char>(data->EventChar)))
				return 1;
		}
		return 0;
	}

	b8 PathInputTextWithHint(cstr label, cstr hint, std::string* str, ImGuiInputTextFlags flags)
	{
		IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
		flags |= ImGuiInputTextFlags_CallbackResize;
		flags |= ImGuiInputTextFlags_CallbackCharFilter;

		InputTextStdStringCallbackUserData cb_user_data;
		cb_user_data.Str = str;
		cb_user_data.ChainCallback = ValidPathCharTextCallbackFilter;
		cb_user_data.ChainCallbackUserData = nullptr;
		return InputTextWithHint(label, hint, str->data(), str->capacity() + 1, flags, InputTextStdStringCallback, &cb_user_data);
	}

	b8 ButtonPlusMinusScalar(char plusOrMinus, ImGuiDataType dataType, void* inOutValue, void* step, void* stepFast, vec2 buttonSize)
	{
		assert(plusOrMinus == '+' || plusOrMinus == '-' && step != nullptr);
		char label[2] = { plusOrMinus, '\0' };
		const f32 frameHeight = GetFrameHeight();

		PushID(inOutValue);
		const b8 buttonClicked = ButtonEx(label, vec2((buttonSize.x <= 0.0f) ? frameHeight : buttonSize.x, (buttonSize.y <= 0.0f) ? frameHeight : buttonSize.y), ImGuiButtonFlags_Repeat | ImGuiButtonFlags_DontClosePopups);
		PopID();

		if (buttonClicked)
			DataTypeApplyOp(dataType, plusOrMinus, inOutValue, inOutValue, (GetIO().KeyCtrl && stepFast != nullptr) ? stepFast : step);
		return buttonClicked;
	}

	b8 ButtonPlusMinusFloat(char plusOrMinus, f32* inOutValue, f32 step, f32 stepFast, vec2 buttonSize)
	{
		return ButtonPlusMinusScalar(plusOrMinus, ImGuiDataType_Float, inOutValue, &step, &stepFast, buttonSize);
	}

	b8 ButtonPlusMinusInt(char plusOrMinus, i32* inOutValue, i32 step, i32 stepFast, vec2 buttonSize)
	{
		return ButtonPlusMinusScalar(plusOrMinus, ImGuiDataType_S32, inOutValue, &step, &stepFast, buttonSize);
	}

	// NOTE: Basically ImGui::InputScalar() but with some extra parameters and more detailed return values
	InputScalarWithButtonsResult InputScalar_WithExtraStuff(cstr label, ImGuiDataType data_type, void* p_data, const void* p_step, const void* p_step_fast, cstr format, ImGuiInputTextFlags flags, const InputScalarWithButtonsExData* ex_data)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return InputScalarWithButtonsResult {};

		ImGuiContext& g = *GImGui;
		ImGuiStyle& style = g.Style;

		if (format == NULL)
			format = DataTypeGetInfo(data_type)->PrintFmt;

		char buf[64];
		DataTypeFormatString(buf, IM_ARRAYSIZE(buf), data_type, p_data, format);

		flags |= ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoMarkEdited; // We call MarkItemEdited() ourselves by comparing the actual data rather than the string.

		InputScalarWithButtonsResult result = {};
		if (p_step != NULL)
		{
			const bool spin_buttons = (ex_data != nullptr) ? ex_data->SpinButtons : false;
			const float frame_height = GetFrameHeight();
			const float button_size = spin_buttons ? ImFloor(frame_height * 0.5f) : frame_height;
			const float button_spacing_x = spin_buttons ? 0.0f : style.ItemInnerSpacing.x;
			const float button_spacing_y = 0.0f;

			const ImVec4 backup_text_color = style.Colors[ImGuiCol_Text];
			style.Colors[ImGuiCol_Text] = (ex_data != nullptr) ? ColorConvertU32ToFloat4(ex_data->TextColor) : backup_text_color;

			BeginGroup(); // The only purpose of the group here is to allow the caller to query item data e.g. IsItemActive()
			PushID(label);
			SetNextItemWidth(ImMax(1.0f, CalcItemWidth() - (spin_buttons ? button_size : (button_size + style.ItemInnerSpacing.x) * 2)));
			if (InputText("", buf, IM_ARRAYSIZE(buf), flags)) // PushId(label) + "" gives us the expected ID from outside point of view
				result.ValueChanged = DataTypeApplyFromText(buf, data_type, p_data, format) || WasInputTextStateEditedThisFrame();

			result.IsTextItemActive = IsItemActive();
			result.TextItemRect[0] = GetItemRect();
			style.Colors[ImGuiCol_Text] = backup_text_color;

			const ImVec2 backup_frame_padding = style.FramePadding;

			style.FramePadding.x = style.FramePadding.y;
			ImGuiButtonFlags button_flags = ImGuiButtonFlags_Repeat | ImGuiButtonFlags_DontClosePopups;
			if (flags & ImGuiInputTextFlags_ReadOnly)
				BeginDisabled();
			SameLine(0, button_spacing_x);
			if (spin_buttons)
			{
				BeginGroup();
				const float backup_font_size = g.DrawListSharedData.FontSize;
				const float backup_item_spacing_y = g.Style.ItemSpacing.y;
				g.DrawListSharedData.FontSize = button_size;
				g.Style.ItemSpacing.y = button_spacing_y;
				if (ArrowButtonEx("+", ImGuiDir_Up, ImVec2(button_size, button_size), button_flags))
				{
					DataTypeApplyOp(data_type, '+', p_data, p_data, g.IO.KeyCtrl && p_step_fast ? p_step_fast : p_step);
					result.ValueChanged = true;
					result.ButtonClicked = true;
				}
				if (ArrowButtonEx("-", ImGuiDir_Down, ImVec2(button_size, button_size), button_flags))
				{
					DataTypeApplyOp(data_type, '-', p_data, p_data, g.IO.KeyCtrl && p_step_fast ? p_step_fast : p_step);
					result.ValueChanged = true;
					result.ButtonClicked = true;
				}
				g.Style.ItemSpacing.y = backup_item_spacing_y;
				g.DrawListSharedData.FontSize = backup_font_size;
				EndGroup();
			}
			else
			{
				if (ButtonEx("-", ImVec2(button_size, button_size), button_flags))
				{
					DataTypeApplyOp(data_type, '-', p_data, p_data, g.IO.KeyCtrl && p_step_fast ? p_step_fast : p_step);
					result.ValueChanged = true;
					result.ButtonClicked = true;
				}
				SameLine(0, style.ItemInnerSpacing.x);
				if (ButtonEx("+", ImVec2(button_size, button_size), button_flags))
				{
					DataTypeApplyOp(data_type, '+', p_data, p_data, g.IO.KeyCtrl && p_step_fast ? p_step_fast : p_step);
					result.ValueChanged = true;
					result.ButtonClicked = true;
				}
			}
			if (flags & ImGuiInputTextFlags_ReadOnly)
				EndDisabled();

			const char* label_end = FindRenderedTextEnd(label);
			if (label != label_end)
			{
				SameLine(0, style.ItemInnerSpacing.x);
				TextEx(label, label_end);
			}
			style.FramePadding = backup_frame_padding;

			PopID();
			EndGroup();
		}
		else
		{
			const auto backup_text_color = style.Colors[ImGuiCol_Text];
			style.Colors[ImGuiCol_Text] = (ex_data != nullptr) ? ColorConvertU32ToFloat4(ex_data->TextColor) : backup_text_color;

			if (InputText(label, buf, IM_ARRAYSIZE(buf), flags))
				result.ValueChanged = DataTypeApplyFromText(buf, data_type, p_data, format) || WasInputTextStateEditedThisFrame();
			result.IsTextItemActive = IsItemActive();
			result.TextItemRect[0] = GetItemRect();

			style.Colors[ImGuiCol_Text] = backup_text_color;
		}
		if (result.ValueChanged)
			MarkItemEdited(g.LastItemData.ID);

		return result;
	}

	InputScalarWithButtonsResult InputScalarN_WithExtraStuff(cstr label, ImGuiDataType data_type, void* p_data, i32 components, const void* p_step, const void* p_step_fast, cstr format, ImGuiInputTextFlags flags, const InputScalarWithButtonsExData* ex_data)
	{
		assert(components > 0);
		if (components == 1)
			return InputScalar_WithExtraStuff(label, data_type, p_data, p_step, p_step_fast, format, flags, ex_data);

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return InputScalarWithButtonsResult {};

		// NOTE: Duplicated from imgui_widgets.cpp beacuse it's not exposed publicly
		static constexpr size_t GDataTypeInfo_Sizes[] = { sizeof(i8), sizeof(u8), sizeof(i16), sizeof(u16), sizeof(i32), sizeof(u32), sizeof(f32), sizeof(i64), sizeof(u64), sizeof(f64) };
		IM_STATIC_ASSERT(IM_ARRAYSIZE(GDataTypeInfo_Sizes) == ImGuiDataType_COUNT);

		InputScalarWithButtonsResult result = {};

		ImGuiContext& g = *GImGui;
		BeginGroup();
		PushID(label);
		PushMultiItemsWidths(components, CalcItemWidth());
		size_t type_size = GDataTypeInfo_Sizes[data_type];
		for (int i = 0; i < components; i++)
		{
			PushID(i);
			if (i > 0)
				SameLine(0, g.Style.ItemInnerSpacing.x);

			const auto scalarResult = InputScalar_WithExtraStuff("", data_type, p_data, p_step, p_step_fast, format, flags, ex_data);
			result.ValueChanged |= (static_cast<u32>(scalarResult.ValueChanged) << i);
			result.ButtonClicked |= (static_cast<u32>(scalarResult.ButtonClicked) << i);
			result.IsTextItemActive |= (static_cast<u32>(scalarResult.IsTextItemActive) << i);
			assert(i < ArrayCountI32(result.TextItemRect));
			result.TextItemRect[i] = scalarResult.TextItemRect[0];

			PopID();
			PopItemWidth();
			p_data = (void*)((char*)p_data + type_size);
		}
		PopID();

		const char* label_end = FindRenderedTextEnd(label);
		if (label != label_end)
		{
			SameLine(0.0f, g.Style.ItemInnerSpacing.x);
			TextEx(label, label_end);
		}

		EndGroup();
		return result;
	}

	b8 SpinScalar(cstr label, ImGuiDataType data_type, void* p_data, const void* p_step, const void* p_step_fast, cstr format, ImGuiInputTextFlags flags)
	{
		InputScalarWithButtonsExData ex_data {};
		ex_data.TextColor = GetColorU32(ImGuiCol_Text);
		ex_data.SpinButtons = true;
		InputScalarWithButtonsResult result = InputScalar_WithExtraStuff(label, data_type, p_data, p_step, p_step_fast, format, flags, &ex_data);
		return result.ValueChanged;
	}

	b8 SpinInt(cstr label, i32* v, i32 step, i32 step_fast, ImGuiInputTextFlags flags)
	{
		cstr format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%08X" : "%d";
		return SpinScalar(label, ImGuiDataType_S32, (void*)v, (void*)(step > 0 ? &step : nullptr), (void*)(step_fast > 0 ? &step_fast : nullptr), format, flags);
	}

	b8 SpinFloat(cstr label, f32* v, f32 step, f32 step_fast, cstr format, ImGuiInputTextFlags flags)
	{
		flags |= ImGuiInputTextFlags_CharsScientific;
		return SpinScalar(label, ImGuiDataType_Float, (void*)v, (void*)(step > 0.0f ? &step : nullptr), (void*)(step_fast > 0.0f ? &step_fast : nullptr), format, flags);
	}

	b8 SpinDouble(cstr label, f64* v, f64 step, f64 step_fast, cstr format, ImGuiInputTextFlags flags)
	{
		flags |= ImGuiInputTextFlags_CharsScientific;
		return SpinScalar(label, ImGuiDataType_Double, (void*)v, (void*)(step > 0.0 ? &step : nullptr), (void*)(step_fast > 0.0 ? &step_fast : nullptr), format, flags);
	}

	PathInputTextWithBrowserButtonResult PathInputTextWithHintAndBrowserDialogButton(cstr label, cstr hint, std::string* str, ImGuiInputTextFlags flags)
	{
		PathInputTextWithBrowserButtonResult result {};

		const auto& style = ImGui::GetStyle();
		const f32 buttonSize = ImGui::GetFrameHeight();

		ImGui::SetNextItemWidth(Max(1.0f, (ImGui::GetContentRegionAvail().x - 1.0f) - (buttonSize + style.ItemInnerSpacing.x)));
		result.InputTextEdited = PathInputTextWithHint(label, hint, str, flags);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { style.FramePadding.y, style.FramePadding.y });
		ImGui::SameLine(0, style.ItemInnerSpacing.x);
		result.BrowseButtonClicked = ImGui::Button("...", { buttonSize, buttonSize });
		ImGui::PopStyleVar();

		return result;
	}

	void DrawStar(ImDrawList* drawList, vec2 center, f32 outerRadius, f32 innerRadius, u32 color, f32 thicknessOrZeroToFill)
	{
		vec2 outer[5], inner[5];
		for (i32 i = 0; i < ArrayCountI32(outer); i++)
		{
			static constexpr f32 step = (360.0f / 5.0f), stepHalf = (step * 0.5f);
			outer[i] = center + (Rotate(vec2(0.0f, -1.0f), Angle::FromDegrees(step * i)) * outerRadius);
			inner[i] = center + (Rotate(vec2(0.0f, -1.0f), Angle::FromDegrees(stepHalf + (step * i))) * innerRadius);
		}

		if (thicknessOrZeroToFill <= 0.0f)
		{
			// NOTE: Inner pentagon
			drawList->PathLineTo(inner[0]);
			drawList->PathLineTo(inner[1]);
			drawList->PathLineTo(inner[2]);
			drawList->PathLineTo(inner[3]);
			drawList->PathLineTo(inner[4]);
			drawList->PathFillConvex(color);
			// NOTE: Spiky triangle bits
			drawList->AddTriangleFilled(inner[4], outer[0], inner[1], color);
			drawList->AddTriangleFilled(inner[0], outer[1], inner[2], color);
			drawList->AddTriangleFilled(inner[1], outer[2], inner[3], color);
			drawList->AddTriangleFilled(inner[2], outer[3], inner[4], color);
			drawList->AddTriangleFilled(inner[3], outer[4], inner[0], color);
		}
		else
		{
			drawList->PathLineTo(outer[0]); drawList->PathLineTo(inner[0]);
			drawList->PathLineTo(outer[1]); drawList->PathLineTo(inner[1]);
			drawList->PathLineTo(outer[2]); drawList->PathLineTo(inner[2]);
			drawList->PathLineTo(outer[3]); drawList->PathLineTo(inner[3]);
			drawList->PathLineTo(outer[4]); drawList->PathLineTo(inner[4]);
			drawList->PathStroke(color, ImDrawFlags_Closed, thicknessOrZeroToFill);
		}
	}
}
