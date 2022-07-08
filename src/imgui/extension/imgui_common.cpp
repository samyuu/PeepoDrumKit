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

	ImVec2 CalcTextSize(std::string_view text, bool hide_text_after_double_hash, float wrap_width)
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

	void AddTextWithDropShadow(ImDrawList* drawList, const ImFont* font, float fontSize, vec2 textPosition, u32 textColor, std::string_view text, float wrap_width, const ImVec4* cpu_fine_clip_rect, u32 shadowColor, vec2 shadowOffset)
	{
		drawList->AddText(font, fontSize, textPosition + shadowOffset, shadowColor, StringViewStart(text), StringViewEnd(text), wrap_width, cpu_fine_clip_rect);
		drawList->AddText(font, fontSize, textPosition, textColor, StringViewStart(text), StringViewEnd(text), wrap_width, cpu_fine_clip_rect);
	}

	bool InputText(const char* label, std::string* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
	{
		IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
		flags |= ImGuiInputTextFlags_CallbackResize;

		InputTextStdStringCallbackUserData cb_user_data;
		cb_user_data.Str = str;
		cb_user_data.ChainCallback = callback;
		cb_user_data.ChainCallbackUserData = user_data;
		return ImGui::InputText(label, str->data(), str->capacity() + 1, flags, InputTextStdStringCallback, &cb_user_data);
	}

	bool InputTextMultiline(const char* label, std::string* str, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
	{
		IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
		flags |= ImGuiInputTextFlags_CallbackResize;

		InputTextStdStringCallbackUserData cb_user_data;
		cb_user_data.Str = str;
		cb_user_data.ChainCallback = callback;
		cb_user_data.ChainCallbackUserData = user_data;
		return ImGui::InputTextMultiline(label, str->data(), str->capacity() + 1, size, flags, InputTextStdStringCallback, &cb_user_data);
	}

	bool InputTextMultilineWithHint(const char* label, const char* hint, std::string* str, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
	{
		IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
		flags |= ImGuiInputTextFlags_CallbackResize;

		InputTextStdStringCallbackUserData cb_user_data;
		cb_user_data.Str = str;
		cb_user_data.ChainCallback = callback;
		cb_user_data.ChainCallbackUserData = user_data;
		return ImGui::InputTextEx(label, hint, str->data(), static_cast<int>(str->capacity() + 1), size, flags | ImGuiInputTextFlags_Multiline, InputTextStdStringCallback, &cb_user_data);
	}

	bool InputTextWithHint(const char* label, const char* hint, std::string* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
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

	bool PathInputTextWithHint(const char* label, const char* hint, std::string* str, ImGuiInputTextFlags flags)
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

	PathInputTextWithBrowserButtonResult PathInputTextWithHintAndBrowserDialogButton(const char* label, const char* hint, std::string* str, ImGuiInputTextFlags flags)
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
