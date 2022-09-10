#include "chart_editor_widgets.h"
#include "chart_editor_settings.h"
#include "chart_editor_undo.h"
#include "chart_editor_common.h"

// TODO: Populate char[U8Max] lookup table using provided flags and index into instead of using a switch (?)
enum class EscapeSequenceFlags : u32 { NewLines };

static void ResolveEscapeSequences(const std::string_view in, std::string& out, EscapeSequenceFlags flags)
{
	assert(flags == EscapeSequenceFlags::NewLines);
	for (size_t i = 0; i < in.size(); i++)
	{
		if (in[i] == '\\' && i + 1 < in.size())
		{
			switch (in[i + 1])
			{
			case '\\': { out += '\\'; i++; } break;
			case 'n': { out += '\n'; i++; } break;
			}
		}
		else { out += in[i]; }
	}
}

static void ConvertToEscapeSequences(const std::string_view in, std::string& out, EscapeSequenceFlags flags)
{
	assert(flags == EscapeSequenceFlags::NewLines);
	out.reserve(out.size() + in.size());
	for (const char c : in)
	{
		switch (c)
		{
		case '\\': { out += "\\\\"; } break;
		case '\n': { out += "\\n"; } break;
		default: { out += c; } break;
		}
	}
}

namespace PeepoDrumKit
{
	static b8 GuiDragLabelScalar(std::string_view label, ImGuiDataType dataType, void* inOutValue, f32 speed = 1.0f, const void* min = nullptr, const void* max = nullptr, ImGuiSliderFlags flags = ImGuiSliderFlags_None)
	{
		b8 valueChanged = false;
		Gui::BeginGroup();
		Gui::PushID(Gui::StringViewStart(label), Gui::StringViewEnd(label));
		{
			const ImVec2 topLeftCursorPos = Gui::GetCursorScreenPos();

			Gui::PushStyleColor(ImGuiCol_Text, 0); Gui::PushStyleColor(ImGuiCol_FrameBg, 0); Gui::PushStyleColor(ImGuiCol_FrameBgHovered, 0); Gui::PushStyleColor(ImGuiCol_FrameBgActive, 0);
			valueChanged = Gui::DragScalar("##DragScalar", dataType, inOutValue, speed, min, max, nullptr, flags | ImGuiSliderFlags_NoInput);
			Gui::SetDragScalarMouseCursor();
			Gui::PopStyleColor(4);

			// NOTE: Calling GetColorU32(ImGuiCol_Text) here instead of GetStyleColorVec4() causes the disabled-item alpha factor to get applied twice
			Gui::SetCursorScreenPos(topLeftCursorPos);
			Gui::PushStyleColor(ImGuiCol_Text, Gui::IsItemActive() ? DragTextActiveColor : Gui::IsItemHovered() ? DragTextHoveredColor : Gui::ColorConvertFloat4ToU32(Gui::GetStyleColorVec4(ImGuiCol_Text)));
			Gui::AlignTextToFramePadding(); Gui::TextUnformatted(Gui::StringViewStart(label), Gui::FindRenderedTextEnd(Gui::StringViewStart(label), Gui::StringViewEnd(label)));
			Gui::PopStyleColor(1);
		}
		Gui::PopID();
		Gui::EndGroup();
		return valueChanged;
	}

	static b8 GuiDragLabelFloat(std::string_view label, f32* inOutValue, f32 speed = 1.0f, f32 min = 0.0f, f32 max = 0.0f, ImGuiSliderFlags flags = ImGuiSliderFlags_None)
	{
		return GuiDragLabelScalar(label, ImGuiDataType_Float, inOutValue, speed, &min, &max, flags);
	}

	static b8 GuiInputFraction(cstr label, ivec2* inOutValue, std::optional<ivec2> valueRange, i32 step = 0, i32 stepFast = 0)
	{
		static constexpr i32 components = 2;
		static constexpr std::string_view divisionText = " / ";
		const f32 divisionLabelWidth = Gui::CalcTextSize(Gui::StringViewStart(divisionText), Gui::StringViewEnd(divisionText)).x;
		const f32 perComponentInputFloatWidth = Floor(((Gui::GetContentRegionAvail().x - divisionLabelWidth) / static_cast<f32>(components)));

		b8 anyValueChanged = false;
		for (i32 component = 0; component < components; component++)
		{
			Gui::PushID(&(*inOutValue)[component]);

			const b8 isLastComponent = ((component + 1) == components);
			Gui::SetNextItemWidth(isLastComponent ? (Gui::GetContentRegionAvail().x - 1.0f) : perComponentInputFloatWidth);
			if (Gui::SpinInt("##Component", &(*inOutValue)[component], step, stepFast, ImGuiInputTextFlags_None))
			{
				if (valueRange.has_value()) (*inOutValue)[component] = Clamp((*inOutValue)[component], valueRange->x, valueRange->y);
				anyValueChanged = true;
			}

			if (!isLastComponent)
			{
				Gui::SameLine(0.0f, 0.0f);
				Gui::TextUnformatted(divisionText);
				Gui::SameLine(0.0f, 0.0f);
			}

			Gui::PopID();
		}
		return anyValueChanged;
	}

	static b8 GuiDifficultyLevelStarSliderWidget(cstr label, DifficultyLevel* inOutLevel, b8& inOutFitOnScreenLastFrame, b8& inOutHoveredLastFrame)
	{
		b8 valueWasChanged = false;

		// NOTE: Make text transparent instead of using an empty slider format string 
		//		 so that the slider can still convert the input to a string on the same frame it is turned into an InputText (due to the frame delayed starsFitOnScreen)
		if (inOutFitOnScreenLastFrame) Gui::PushStyleColor(ImGuiCol_Text, 0x00000000);
		Gui::PushStyleColor(ImGuiCol_SliderGrab, Gui::GetStyleColorVec4(inOutHoveredLastFrame ? ImGuiCol_ButtonHovered : ImGuiCol_Button));
		Gui::PushStyleColor(ImGuiCol_SliderGrabActive, Gui::GetStyleColorVec4(ImGuiCol_ButtonActive));
		Gui::PushStyleColor(ImGuiCol_FrameBgHovered, Gui::GetStyleColorVec4(ImGuiCol_FrameBg));
		Gui::PushStyleColor(ImGuiCol_FrameBgActive, Gui::GetStyleColorVec4(ImGuiCol_FrameBg));
		if (i32 v = static_cast<i32>(*inOutLevel); Gui::SliderInt(label, &v,
			static_cast<i32>(DifficultyLevel::Min), static_cast<i32>(DifficultyLevel::Max), "x%d", ImGuiSliderFlags_AlwaysClamp))
		{
			*inOutLevel = static_cast<DifficultyLevel>(v);
			valueWasChanged = true;
		}
		Gui::PopStyleColor(4 + (inOutFitOnScreenLastFrame ? 1 : 0));

		const Rect sliderRect = Gui::GetItemRect();
		const f32 availableWidth = sliderRect.GetWidth();
		const vec2 starSize = vec2(availableWidth / static_cast<f32>(DifficultyLevel::Max), Gui::GetFrameHeight());

		const b8 starsFitOnScreen = (starSize.x >= Gui::GetFrameHeight()) && !Gui::IsItemBeingEditedAsText();

		// NOTE: Use the last frame result here too to match the slider text as it has already been drawn
		if (inOutFitOnScreenLastFrame)
		{
			// NOTE: Manually tuned for a 16px font size
			struct StarParam { f32 OuterRadius, InnerRadius, Thickness; };
			static constexpr StarParam fontSizedStarParamOutline = { 8.0f, 4.0f, 1.0f };
			static constexpr StarParam fontSizedStarParamFilled = { 10.0f, 4.0f, 0.0f };
			const f32 starScale = Gui::GetFontSize() / 16.0f;

			// TODO: Consider drawing star background manually instead of using the slider grab hand (?)
			for (i32 i = 0; i < static_cast<i32>(DifficultyLevel::Max); i++)
			{
				const Rect starRect = Rect::FromTLSize(sliderRect.TL + vec2(i * starSize.x, 0.0f), starSize);
				const auto star = (i >= static_cast<i32>(*inOutLevel)) ? fontSizedStarParamOutline : fontSizedStarParamFilled;
				Gui::DrawStar(Gui::GetWindowDrawList(), starRect.GetCenter(), starScale * star.OuterRadius, starScale * star.InnerRadius, Gui::GetColorU32(ImGuiCol_Text), star.Thickness);
			}
		}

		inOutHoveredLastFrame = Gui::IsItemHovered();
		inOutFitOnScreenLastFrame = starsFitOnScreen;
		return valueWasChanged;
	}

	union MultiEditDataUnion
	{
		f32 F32; i32 I32; i16 I16;
		f32 F32_V[4]; i32 I32_V[4]; i16 I16_V[4];
	};
	struct MultiEditWidgetResult
	{
		u32 HasValueExact;
		u32 HasValueIncrement;
		MultiEditDataUnion ValueExact;
		MultiEditDataUnion ValueIncrement;
	};
	struct MultiEditWidgetParam
	{
		ImGuiDataType DataType = ImGuiDataType_Float;
		i32 Components = 1;
		b8 EnableStepButtons = false;
		b8 UseSpinButtonsInstead = false;
		b8 EnableDragLabel = true;
		b8 EnableClamp = false;
		u32 HasMixedValues = false;
		MultiEditDataUnion Value = {};
		MultiEditDataUnion MixedValuesMin = {};
		MultiEditDataUnion MixedValuesMax = {};
		MultiEditDataUnion ValueClampMin = {};
		MultiEditDataUnion ValueClampMax = {};
		MultiEditDataUnion ButtonStep = {};
		MultiEditDataUnion ButtonStepFast = {};
		f32 DragLabelSpeed = 1.0f;
		cstr FormatString = nullptr;
	};

	static MultiEditWidgetResult GuiPropertyMultiSelectionEditWidget(std::string_view label, const MultiEditWidgetParam& in)
	{
		MultiEditWidgetResult out = {};
		Gui::PushID(Gui::StringViewStart(label), Gui::StringViewEnd(label));
		Gui::Property::Property([&]
		{
			if (in.EnableDragLabel)
			{
				MultiEditDataUnion v = in.Value;
				Gui::SetNextItemWidth(-1.0f);
				if (GuiDragLabelScalar(label, in.DataType, &v, in.DragLabelSpeed, in.EnableClamp ? &in.ValueClampMin : nullptr, in.EnableClamp ? &in.ValueClampMax : nullptr))
				{
					for (i32 c = 0; c < in.Components; c++)
					{
						out.HasValueIncrement |= (1 << c);
						switch (in.DataType)
						{
						case ImGuiDataType_Float: { out.ValueIncrement.F32_V[c] = (v.F32_V[c] - in.Value.F32_V[c]); } break;
						case ImGuiDataType_S32: { out.ValueIncrement.I32_V[c] = (v.I32_V[c] - in.Value.I32_V[c]); } break;
						case ImGuiDataType_S16: { out.ValueIncrement.I16_V[c] = (v.I16_V[c] - in.Value.I16_V[c]); } break;
						default: assert(false); break;
						}
					}
				}
			}
			else
			{
				Gui::AlignTextToFramePadding();
				Gui::TextUnformatted(label);
			}
		});
		Gui::Property::Value([&]()
		{
			const auto& style = Gui::GetStyle();
			cstr formatString = (in.FormatString != nullptr) ? in.FormatString : Gui::DataTypeGetInfo(in.DataType)->PrintFmt;

			b8& textInputActiveLastFrame = *Gui::GetStateStorage()->GetBoolRef(Gui::GetID("TextInputActiveLastFrame"), false);
			const b8 showMixedValues = (in.HasMixedValues && !textInputActiveLastFrame);

			Gui::SetNextItemWidth(-1.0f);
			MultiEditDataUnion v = in.Value;
			Gui::InputScalarWithButtonsExData exData {}; exData.TextColor = Gui::GetColorU32(ImGuiCol_Text, showMixedValues ? 0.0f : 1.0f); exData.SpinButtons = in.UseSpinButtonsInstead;
			Gui::InputScalarWithButtonsResult result = Gui::InputScalarN_WithExtraStuff("##Input", in.DataType, &v, in.Components, in.EnableStepButtons ? &in.ButtonStep : nullptr, in.EnableStepButtons ? &in.ButtonStepFast : nullptr, formatString, ImGuiInputTextFlags_None, &exData);
			if (result.ValueChanged)
			{
				for (i32 c = 0; c < in.Components; c++)
				{
					if (result.ButtonClicked & (1 << c))
						out.HasValueIncrement |= ((result.ValueChanged & (1 << c)) ? 1 : 0) << c;
					else
						out.HasValueExact |= ((result.ValueChanged & (1 << c)) ? 1 : 0) << c;

					if (in.EnableClamp)
					{
						switch (in.DataType)
						{
						case ImGuiDataType_Float: { v.F32_V[c] = Clamp(v.F32_V[c], in.ValueClampMin.F32_V[c], in.ValueClampMax.F32_V[c]); } break;
						case ImGuiDataType_S32: { v.I32_V[c] = Clamp(v.I32_V[c], in.ValueClampMin.I32_V[c], in.ValueClampMax.I32_V[c]); } break;
						case ImGuiDataType_S16: { v.I16_V[c] = Clamp(v.I16_V[c], in.ValueClampMin.I16_V[c], in.ValueClampMax.I16_V[c]); } break;
						default: assert(false); break;
						}
					}

					switch (in.DataType)
					{
					case ImGuiDataType_Float: { out.ValueExact.F32_V[c] = v.F32_V[c]; out.ValueIncrement.F32_V[c] = (v.F32_V[c] - in.Value.F32_V[c]); } break;
					case ImGuiDataType_S32: { out.ValueExact.I32_V[c] = v.I32_V[c]; out.ValueIncrement.I32_V[c] = (v.I32_V[c] - in.Value.I32_V[c]); } break;
					case ImGuiDataType_S16: { out.ValueExact.I16_V[c] = v.I16_V[c]; out.ValueIncrement.I16_V[c] = (v.I16_V[c] - in.Value.I16_V[c]); } break;
					default: assert(false); break;
					}
				}
			}
			textInputActiveLastFrame = result.IsTextItemActive;

			if (showMixedValues)
			{
				for (i32 c = 0; c < in.Components; c++)
				{
					if (in.HasMixedValues & (1 << c))
					{
						char min[64], max[64];
						switch (in.DataType)
						{
						case ImGuiDataType_Float: { sprintf_s(min, formatString, in.MixedValuesMin.F32_V[c]); sprintf_s(max, formatString, in.MixedValuesMax.F32_V[c]); } break;
						case ImGuiDataType_S32: { sprintf_s(min, formatString, in.MixedValuesMin.I32_V[c]); sprintf_s(max, formatString, in.MixedValuesMax.I32_V[c]); } break;
						case ImGuiDataType_S16: { sprintf_s(min, formatString, in.MixedValuesMin.I16_V[c]); sprintf_s(max, formatString, in.MixedValuesMax.I16_V[c]); } break;
						default: assert(false); break;
						}
						char multiSelectionPreview[128]; sprintf_s(multiSelectionPreview, "(%s ... %s)", min, max);

						Gui::PushStyleColor(ImGuiCol_Text, Gui::GetColorU32(ImGuiCol_Text, 0.6f));
						Gui::RenderTextClipped(result.TextItemRect[c].TL + vec2(style.FramePadding.x, 0.0f), result.TextItemRect[c].BR, multiSelectionPreview, nullptr, nullptr, { 0.0f, 0.5f });
						Gui::PopStyleColor();
					}
					else
					{
						char preview[64];
						switch (in.DataType)
						{
						case ImGuiDataType_Float: { sprintf_s(preview, formatString, in.MixedValuesMin.F32_V[c]); } break;
						case ImGuiDataType_S32: { sprintf_s(preview, formatString, in.MixedValuesMin.I32_V[c]); } break;
						case ImGuiDataType_S16: { sprintf_s(preview, formatString, in.MixedValuesMin.I16_V[c]); } break;
						default: assert(false); break;
						}
						Gui::RenderTextClipped(result.TextItemRect[c].TL + vec2(style.FramePadding.x, 0.0f), result.TextItemRect[c].BR, preview, nullptr, nullptr, { 0.0f, 0.5f });
					}
				}
			}
		});
		Gui::PopID();
		return out;
	}

	static b8 GuiPropertyRangeInterpolationEditWidget(std::string_view label, f32 inOutStartEnd[2], f32 step, f32 stepFast, f32 minValue, f32 maxValue, cstr format, const cstr previewStrings[2])
	{
		b8 wasValueChanged = false;
		Gui::PushID(Gui::StringViewStart(label), Gui::StringViewEnd(label));
		Gui::Property::PropertyTextValueFunc(label, [&]
		{
			static constexpr i32 components = 2; // NOTE: Unicode "Rightwards Arrow" U+2192
			static constexpr std::string_view divisionText = "  \xE2\x86\x92  "; // "  ->  "; // " < > ";
			const f32 divisionLabelWidth = Gui::CalcTextSize(Gui::StringViewStart(divisionText), Gui::StringViewEnd(divisionText)).x;
			const f32 perComponentInputFloatWidth = Floor(((Gui::GetContentRegionAvail().x - divisionLabelWidth) / static_cast<f32>(components)));

			for (i32 component = 0; component < components; component++)
			{
				Gui::PushID(&inOutStartEnd[component]);
				b8& textInputActiveLastFrame = *Gui::GetStateStorage()->GetBoolRef(Gui::GetID("TextInputActiveLastFrame"), false);
				const b8 showPreviewStrings = ((previewStrings[component] != nullptr) && !textInputActiveLastFrame);
				const b8 isLastComponent = ((component + 1) == components);

				Gui::SetNextItemWidth(isLastComponent ? (Gui::GetContentRegionAvail().x - 1.0f) : perComponentInputFloatWidth);
				Gui::InputScalarWithButtonsExData exData {}; exData.TextColor = Gui::GetColorU32(ImGuiCol_Text, showPreviewStrings ? 0.0f : 1.0f); exData.SpinButtons = true;
				Gui::InputScalarWithButtonsResult result = Gui::InputScalar_WithExtraStuff("##Component", ImGuiDataType_Float, &inOutStartEnd[component], &step, &stepFast, format, ImGuiInputTextFlags_None, &exData);
				if (result.ValueChanged)
				{
					inOutStartEnd[component] = Clamp(inOutStartEnd[component], minValue, maxValue);
					wasValueChanged = true;
				}
				textInputActiveLastFrame = result.IsTextItemActive;

				if (showPreviewStrings)
				{
					Gui::PushStyleColor(ImGuiCol_Text, Gui::GetColorU32(ImGuiCol_Text, 0.6f));
					Gui::RenderTextClipped(result.TextItemRect[0].TL + vec2(Gui::GetStyle().FramePadding.x, 0.0f), result.TextItemRect[0].BR, previewStrings[component], nullptr, nullptr, { 0.0f, 0.5f });
					Gui::PopStyleColor();
				}

				if (!isLastComponent)
				{
					Gui::SameLine(0.0f, 0.0f);
					Gui::TextUnformatted(divisionText);
					Gui::SameLine(0.0f, 0.0f);
				}
				Gui::PopID();
			}
		});
		Gui::PopID();
		return wasValueChanged;
	}
}

namespace PeepoDrumKit
{
	// NOTE: Soft clamp for sliders but hard limit to allow *typing in* values higher, even if it can cause clipping
	static constexpr f32 MinVolume = 0.0f;
	static constexpr f32 MaxVolumeSoftLimit = 1.0f;
	static constexpr f32 MaxVolumeHardLimit = 4.0f;

	// TODO: Maybe this should be a user setting (?)
	static constexpr f32 MinBPM = 30.0f;
	static constexpr f32 MaxBPM = 960.0f;
	static constexpr i32 MinTimeSignatureValue = 1;
	static constexpr i32 MaxTimeSignatureValue = Beat::TicksPerBeat * 4;
	static constexpr f32 MinScrollSpeed = -100.0f;
	static constexpr f32 MaxScrollSpeed = +100.0f;
	static constexpr Time MinNoteTimeOffset = Time::FromMS(-25.0);
	static constexpr Time MaxNoteTimeOffset = Time::FromMS(+25.0);
	static constexpr i16 MinBalloonCount = 0;
	static constexpr i16 MaxBalloonCount = 999;

	cstr LoadingTextAnimation::UpdateFrameAndGetText(b8 isLoadingThisFrame, f32 deltaTimeSec)
	{
		static constexpr cstr TextFrames[] = { "Loading o....", "Loading .o...", "Loading ..o..", "Loading ...o.", "Loading ....o", "Loading ...o.", "Loading ..o..", "Loading .o..." };
		static constexpr f32 FrameIntervalSec = (1.0f / 12.0f);

		if (!WasLoadingLastFrame && isLoadingThisFrame)
			RingIndex = 0;

		WasLoadingLastFrame = isLoadingThisFrame;
		if (isLoadingThisFrame)
			AccumulatedTimeSec += deltaTimeSec;

		cstr loadingText = TextFrames[RingIndex];
		if (AccumulatedTimeSec >= FrameIntervalSec)
		{
			AccumulatedTimeSec -= FrameIntervalSec;
			if (++RingIndex >= ArrayCountI32(TextFrames))
				RingIndex = 0;
		}
		return loadingText;
	}

	void ChartHelpWindow::DrawGui(ChartContext& context)
	{
		static struct
		{
			u32 GreenDark = 0xFFACF7DC; // 0xFF60BE9C;
			u32 GreenBright = 0xFF95CCB8; // 0xFF60BE9C;
			u32 RedDark = 0xFF9BBAEF; // 0xFF72A0ED; // 0xFF71A4FA; // 0xFF7474EF; // 0xFF6363DE;
			u32 RedBright = 0xFF93B2E7; // 0xFFBAC2E4; // 0xFF6363DE;
			u32 WhiteDark = 0xFF95DCDC; // 0xFFEAEAEA;
			u32 WhiteBright = 0xFFBBD3D3; // 0xFFEAEAEA;
			b8 Show = false;
		} colors;
#if PEEPO_DEBUG
		if (Gui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && Gui::IsKeyPressed(ImGuiKey_GraveAccent))
			colors.Show ^= true;
		if (colors.Show)
		{
			Gui::ColorEdit4_U32("Green Dark", &colors.GreenDark);
			Gui::ColorEdit4_U32("Green Bright", &colors.GreenBright);
			Gui::ColorEdit4_U32("Red Dark", &colors.RedDark);
			Gui::ColorEdit4_U32("Red Bright", &colors.RedBright);
			Gui::ColorEdit4_U32("White Dark", &colors.WhiteDark);
			Gui::ColorEdit4_U32("White Bright", &colors.WhiteBright);
		}
#endif

		Gui::PushStyleColor(ImGuiCol_Border, Gui::GetColorU32(ImGuiCol_Separator));
		Gui::BeginChild("BackgroundChild", Gui::GetContentRegionAvail(), true);
		Gui::PopStyleColor(1);
		defer { Gui::EndChild(); };

		Gui::UpdateSmoothScrollWindow();

		// TODO: Maybe use .md markup language to write instructions (?) (how feasable would it be to integrate peepo emotes inbetween text..?)
		Gui::PushFont(FontLarge_EN);
		{
			{
				Gui::PushStyleColor(ImGuiCol_Text, colors.GreenDark);
				Gui::PushFont(FontLarge_EN);
				Gui::TextUnformatted("Welcome to Peepo Drum Kit (Beta)");
				Gui::PopFont();
				Gui::PopStyleColor();

				Gui::PushStyleColor(ImGuiCol_Text, colors.GreenBright);
				Gui::PushFont(FontMedium_EN);
				Gui::TextWrapped("Things are still very much WIP and subject to change with some features still missing " UTF8_FeelsOkayMan);
				Gui::Separator();
				Gui::TextUnformatted("");
				Gui::PopFont();
				Gui::PopStyleColor();
			}

			{
				Gui::PushStyleColor(ImGuiCol_Text, colors.RedDark);
				Gui::PushFont(FontMedium_EN);
				Gui::TextUnformatted("Basic Controls:");
				Gui::PopFont();
				Gui::PopStyleColor();

				Gui::PushStyleColor(ImGuiCol_Text, colors.RedBright);
				Gui::PushFont(FontMain_JP);
				if (Gui::BeginTable("ControlsTable", 2, ImGuiTableFlags_BordersInner | ImGuiTableFlags_NoSavedSettings))
				{
					static constexpr auto row = [&](auto funcLeft, auto funcRight)
					{
						Gui::TableNextRow();
						Gui::TableSetColumnIndex(0); funcLeft();
						Gui::TableSetColumnIndex(1); funcRight();
					};
					static constexpr auto rowSeparator = []() { row([] { Gui::Text(""); }, [] {}); };

					row([] { Gui::Text("Move cursor"); }, [] { Gui::Text("Mouse Left"); });
					row([] { Gui::Text("Select items"); }, [] { Gui::Text("Mouse Right"); });
					row([] { Gui::Text("Scroll"); }, [] { Gui::Text("Mouse Wheel"); });
					row([] { Gui::Text("Scroll panning"); }, [] { Gui::Text("Mouse Middle"); });
					row([] { Gui::Text("Zoom"); }, [] { Gui::Text("Alt + Mouse Wheel"); });
					row([] { Gui::Text("Fast (Action)"); }, [] { Gui::Text("Shift + { Action }"); });
					row([] { Gui::Text("Slow (Action)"); }, [] { Gui::Text("Alt + { Action }"); });
					rowSeparator();
					row([] { Gui::Text("Play / pause"); }, [] { Gui::Text(ToShortcutString(*Settings.Input.Timeline_TogglePlayback).Data); });
					row([] { Gui::Text("Add / remove note"); }, []
					{
						Gui::Text("%s / %s / %s / %s",
							ToShortcutString(Settings.Input.Timeline_PlaceNoteKa->Slots[0]).Data, ToShortcutString(Settings.Input.Timeline_PlaceNoteDon->Slots[0]).Data,
							ToShortcutString(Settings.Input.Timeline_PlaceNoteDon->Slots[1]).Data, ToShortcutString(Settings.Input.Timeline_PlaceNoteKa->Slots[1]).Data);
					});
					row([] { Gui::Text("Add long note"); }, []
					{
						Gui::Text("%s / %s / %s / %s",
							ToShortcutString(Settings.Input.Timeline_PlaceNoteBalloon->Slots[0]).Data, ToShortcutString(Settings.Input.Timeline_PlaceNoteDrumroll->Slots[0]).Data,
							ToShortcutString(Settings.Input.Timeline_PlaceNoteDrumroll->Slots[1]).Data, ToShortcutString(Settings.Input.Timeline_PlaceNoteBalloon->Slots[1]).Data);
					});
					row([] { Gui::Text("Add big note"); }, [] { Gui::Text("Alt + { Note }"); });
					row([] { Gui::Text("Fill range-selection with notes"); }, [] { Gui::Text("Shift + { Note }"); });
					row([] { Gui::Text("Start / end range-selection"); }, [] { Gui::Text(ToShortcutString(*Settings.Input.Timeline_StartEndRangeSelection).Data); });
					row([] { Gui::Text("Grid division"); }, [] { Gui::Text("Mouse [X1/X2] / [%s/%s]", ToShortcutString(*Settings.Input.Timeline_IncreaseGridDivision).Data, ToShortcutString(*Settings.Input.Timeline_DecreaseGridDivision).Data); });
					row([] { Gui::Text("Step cursor"); }, [] { Gui::Text("%s / %s", ToShortcutString(*Settings.Input.Timeline_StepCursorLeft).Data, ToShortcutString(*Settings.Input.Timeline_StepCursorRight).Data); });
					rowSeparator();
					row([] { Gui::Text("Move selected items"); }, [] { Gui::Text("Mouse Left (Hover)"); });
					row([] { Gui::Text("Cut selected items"); }, [] { Gui::Text(ToShortcutString(*Settings.Input.Timeline_Cut).Data); });
					row([] { Gui::Text("Copy selected items"); }, [] { Gui::Text(ToShortcutString(*Settings.Input.Timeline_Copy).Data); });
					row([] { Gui::Text("Paste selected items"); }, [] { Gui::Text(ToShortcutString(*Settings.Input.Timeline_Paste).Data); });
					row([] { Gui::Text("Delete selected items"); }, [] { Gui::Text(ToShortcutString(*Settings.Input.Timeline_DeleteSelection).Data); });
					rowSeparator();
					row([] { Gui::Text("Playback speed"); }, [] { Gui::Text("%s / %s", ToShortcutString(*Settings.Input.Timeline_DecreasePlaybackSpeed).Data, ToShortcutString(*Settings.Input.Timeline_IncreasePlaybackSpeed).Data); });
					row([] { Gui::Text("Toggle metronome"); }, [] { Gui::Text(ToShortcutString(*Settings.Input.Timeline_ToggleMetronome).Data); });

					Gui::EndTable();
				}
				Gui::PopFont();
				Gui::PopStyleColor();
			}

			{
				Gui::PushFont(FontMedium_EN);
				Gui::PushStyleColor(ImGuiCol_Text, colors.WhiteDark);
				Gui::TextUnformatted("");
				Gui::TextUnformatted("About reading and writing TJAs:");
				Gui::PopStyleColor();
				Gui::Separator();

				Gui::PushStyleColor(ImGuiCol_Text, colors.WhiteBright);
				Gui::TextWrapped(
					"Peepo Drum Kit is not really a TJA editor in the same sense that a text editor is one.\n"
					"It's a Taiko chart editor that transparently converts to and from the TJA format,"
					" however its internal data representation of a chart differs significantly.\n"
					"\n"
					"As such, potential data-loss is the convertion process (to some extend) is to be expected.\n"
					"This may either take the form of general formatting differences (removing comments, white space, etc.),\n"
					"issues with (yet) unimplemented features (such as branches and other commands)\n"
					"or general \"rounding errors\" due to notes and other commands being quantized onto a fixed 1/192nd grid (as is a common rhythm game convention).\n"
					"\n"
					"To prevent the user from accidentally overwriting existing TJAs, that have not originally been created via this program (as marked by a \"PeepoDrumKit\" comment),\n"
					"edited TJAs will therefore have a \".bak\" backup file created in-place before being overwritten.\n"
					"\n"
					"With that said, none of this should be a problem when creating new charts from inside the program itself\n"
					"as the goal is for the user to not have to interact with the \".tja\" in text form at all " UTF8_PeepoCoffeeBlonket);
				Gui::PopStyleColor();
				Gui::PopFont();
			}
		}
		Gui::PopFont();
	}

	void ChartUndoHistoryWindow::DrawGui(ChartContext& context)
	{
		const auto& undoStack = context.Undo.UndoStack;
		const auto& redoStack = context.Undo.RedoStack;
		i32 undoClickedIndex = -1, redoClickedIndex = -1;

		const auto& style = Gui::GetStyle();
		// NOTE: Desperate attempt for the table row selectables to not having any spacing between them
		Gui::PushStyleVar(ImGuiStyleVar_CellPadding, { style.CellPadding.x, GuiScale(4.0f) });
		Gui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { style.ItemSpacing.x, GuiScale(8.0f) });
		Gui::PushStyleColor(ImGuiCol_Header, Gui::GetColorU32(ImGuiCol_Header, 0.5f));
		Gui::PushStyleColor(ImGuiCol_HeaderHovered, Gui::GetColorU32(ImGuiCol_HeaderHovered, 0.5f));
		defer { Gui::PopStyleColor(2); Gui::PopStyleVar(2); };

		if (Gui::BeginTable("UndoHistoryTable", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, Gui::GetContentRegionAvail()))
		{
			Gui::PushFont(FontMedium_EN);
			Gui::TableSetupScrollFreeze(0, 1);
			Gui::TableSetupColumn("Description", ImGuiTableColumnFlags_None);
			Gui::TableSetupColumn("Time", ImGuiTableColumnFlags_None);
			Gui::TableHeadersRow();
			Gui::PopFont();

			static constexpr auto undoCommandRow = [](Undo::CommandInfo commandInfo, CPUTime creationTime, const void* id, b8 isSelected)
			{
				Gui::TableNextRow();
				Gui::TableSetColumnIndex(0);
				char labelBuffer[256]; sprintf_s(labelBuffer, "%.*s###%p", FmtStrViewArgs(commandInfo.Description), id);
				const b8 clicked = Gui::Selectable(labelBuffer, isSelected, ImGuiSelectableFlags_SpanAllColumns);

				// TODO: Display as formatted local time instead of time relative to program startup (?)
				Gui::TableSetColumnIndex(1);
				Gui::TextDisabled("%s", CPUTime::DeltaTime(CPUTime {}, creationTime).ToString().Data);
				return clicked;
			};

			if (undoCommandRow(Undo::CommandInfo { "Initial State" }, CPUTime {}, nullptr, undoStack.empty()))
				context.Undo.Undo(undoStack.size());

			if (!undoStack.empty())
			{
				ImGuiListClipper clipper; clipper.Begin(static_cast<i32>(undoStack.size()));
				while (clipper.Step())
				{
					for (i32 i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
					{
						if (undoCommandRow(undoStack[i]->GetInfo(), undoStack[i]->CreationTime, undoStack[i].get(), ((i + 1) == undoStack.size())))
							undoClickedIndex = i;
					}
				}
			}

			if (!redoStack.empty())
			{
				Gui::PushStyleColor(ImGuiCol_Text, Gui::GetStyleColorVec4(ImGuiCol_TextDisabled));
				ImGuiListClipper clipper; clipper.Begin(static_cast<i32>(redoStack.size()));
				while (clipper.Step())
				{
					for (i32 i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
					{
						const i32 redoIndex = ((static_cast<i32>(redoStack.size()) - 1) - i);
						if (undoCommandRow(redoStack[redoIndex]->GetInfo(), redoStack[redoIndex]->CreationTime, redoStack[redoIndex].get(), false))
							redoClickedIndex = redoIndex;
					}
				}
				Gui::PopStyleColor();
			}

			// NOTE: Auto scroll if not already at bottom
			if (Gui::GetScrollY() >= Gui::GetScrollMaxY())
				Gui::SetScrollHereY(1.0f);

			Gui::EndTable();
		}

		if (InBounds(undoClickedIndex, undoStack))
			context.Undo.Undo(undoStack.size() - undoClickedIndex - 1);
		else if (InBounds(redoClickedIndex, redoStack))
			context.Undo.Redo(redoStack.size() - redoClickedIndex);
	}

	void TempoCalculatorWindow::DrawGui(ChartContext& context)
	{
		Gui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		Gui::PushStyleColor(ImGuiCol_Button, Gui::GetStyleColorVec4(ImGuiCol_Header));
		Gui::PushStyleColor(ImGuiCol_ButtonHovered, Gui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
		Gui::PushStyleColor(ImGuiCol_ButtonActive, Gui::GetStyleColorVec4(ImGuiCol_HeaderActive));
		{
			const b8 windowFocused = Gui::IsWindowFocused() && (Gui::GetActiveID() == 0);
			const b8 tapPressed = windowFocused && Gui::IsAnyPressed(*Settings.Input.TempoCalculator_Tap, false);
			const b8 resetPressed = windowFocused && Gui::IsAnyPressed(*Settings.Input.TempoCalculator_Reset, false);
			const b8 resetDown = windowFocused && Gui::IsAnyDown(*Settings.Input.TempoCalculator_Reset);

			Gui::PushFont(FontLarge_EN);
			{
				const Time lastBeatDuration = Time::FromSec(60.0 / Round(Calculator.LastTempo.BPM));
				const f32 tapBeatLerpT = ImSaturate((Calculator.TapCount == 0) ? 1.0f : static_cast<f32>(Calculator.LastTap.GetElapsed() / lastBeatDuration));
				const ImVec4 animatedButtonColor = ImLerp(Gui::GetStyleColorVec4(ImGuiCol_ButtonActive), Gui::GetStyleColorVec4(ImGuiCol_Button), tapBeatLerpT);

				const b8 hasTimedOut = Calculator.HasTimedOut();
				Gui::PushStyleColor(ImGuiCol_Text, Gui::GetColorU32(ImGuiCol_Text, (Calculator.TapCount > 0 && hasTimedOut) ? 0.8f : 1.0f));
				Gui::PushStyleColor(ImGuiCol_Button, animatedButtonColor);
				Gui::PushStyleColor(ImGuiCol_ButtonHovered, (Calculator.TapCount > 0 && !hasTimedOut) ? animatedButtonColor : Gui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
				Gui::PushStyleColor(ImGuiCol_ButtonActive, (Calculator.TapCount > 0 && !hasTimedOut) ? animatedButtonColor : Gui::GetStyleColorVec4(ImGuiCol_ButtonActive));

				char buttonName[32]; sprintf_s(buttonName, (Calculator.TapCount == 0) ? "Tap" : (Calculator.TapCount == 1) ? "First Beat" : "%.2f BPM", Calculator.LastTempo.BPM);
				if (tapPressed | Gui::ButtonEx(buttonName, vec2(-1.0f, Gui::GetFrameHeightWithSpacing() * 3.0f), ImGuiButtonFlags_PressedOnClick))
				{
					context.SfxVoicePool.PlaySound(SoundEffectType::MetronomeBeat);
					Calculator.Tap();
				}

				Gui::PopStyleColor(4);
			}
			Gui::PopFont();

			Gui::PushStyleColor(ImGuiCol_Text, Gui::GetStyleColorVec4((Calculator.TapCount > 0) ? ImGuiCol_Text : ImGuiCol_TextDisabled));
			Gui::PushStyleColor(ImGuiCol_Button, Gui::GetStyleColorVec4(resetDown ? ImGuiCol_ButtonActive : ImGuiCol_Button));
			if (resetPressed | Gui::Button("Reset", vec2(-1.0f, Gui::GetFrameHeightWithSpacing() * 1.0f)))
			{
				if (Calculator.TapCount > 0)
					context.SfxVoicePool.PlaySound(SoundEffectType::MetronomeBar);
				Calculator.Reset();
			}
			Gui::PopStyleColor(2);
		}
		Gui::PopStyleColor(3);
		Gui::PopStyleVar(1);

		Gui::PushFont(FontMedium_EN);
		if (Gui::BeginTable("Table", 2, ImGuiTableFlags_BordersInner | ImGuiTableFlags_NoSavedSettings, Gui::GetContentRegionAvail()))
		{
			cstr formatStrBPM_g = (Calculator.TapCount > 1) ? "%g BPM" : "--.-- BPM";
			cstr formatStrBPM_2f = (Calculator.TapCount > 1) ? "%.2f BPM" : "--.-- BPM";
			static constexpr auto row = [&](auto funcLeft, auto funcRight)
			{
				Gui::TableNextRow();
				Gui::TableSetColumnIndex(0); Gui::AlignTextToFramePadding(); funcLeft();
				Gui::TableSetColumnIndex(1); Gui::AlignTextToFramePadding(); Gui::SetNextItemWidth(-1.0f); funcRight();
			};
			row([&] { Gui::TextUnformatted("Nearest Whole"); }, [&]
			{
				f32 v = Round(Calculator.LastTempo.BPM);
				Gui::InputFloat("##NearestWhole", &v, 0.0f, 0.0f, formatStrBPM_g, ImGuiInputTextFlags_ReadOnly);
			});
			row([&] { Gui::TextUnformatted("Nearest"); }, [&]
			{
				f32 v = Calculator.LastTempo.BPM;
				Gui::InputFloat("##Nearest", &v, 0.0f, 0.0f, formatStrBPM_2f, ImGuiInputTextFlags_ReadOnly);
			});
			row([&] { Gui::TextUnformatted("Min and Max"); }, [&]
			{
				f32 v[2] = { Calculator.LastTempoMin.BPM, Calculator.LastTempoMax.BPM };
				Gui::InputFloat2("##MinMax", v, formatStrBPM_2f, ImGuiInputTextFlags_ReadOnly);
			});
			row([&] { Gui::TextUnformatted("Timing Taps"); }, [&]
			{
				Gui::Text((Calculator.TapCount == 1) ? "First Beat" : "%d Taps", Calculator.TapCount);
			});
			// NOTE: ReadOnly InputFloats specifically to allow easy copy and paste
			Gui::EndTable();
		}
		Gui::PopFont();
	}

	void ChartInspectorWindow::DrawGui(ChartContext& context)
	{
		Gui::UpdateSmoothScrollWindow();

		assert(context.ChartSelectedCourse != nullptr);
		auto& chart = context.Chart;
		auto& course = *context.ChartSelectedCourse;

		if (Gui::CollapsingHeader("Selected Items", ImGuiTreeNodeFlags_DefaultOpen))
		{
			defer { SelectedItems.clear(); };
			BeatSortedForwardIterator<TempoChange> scrollTempoChangeIt {};
			ForEachSelectedChartItem(course, [&](const ForEachChartItemData& it)
			{
				TempChartItem& out = SelectedItems.emplace_back();
				out.List = it.List;
				out.Index = it.Index;
				out.AvailableMembers = GetAvailableMemberFlags(it.List);

				for (GenericMember member = {}; member < GenericMember::Count; IncrementEnum(member))
				{
					if (out.AvailableMembers & EnumToFlag(member))
					{
						const b8 success = TryGetGeneric(course, it.List, it.Index, member, out.MemberValues[member]);
						assert(success);
					}
				}

				if (it.List == GenericList::ScrollChanges)
					out.BaseScrollTempo = TempoOrDefault(scrollTempoChangeIt.Next(course.TempoMap.Tempo.Sorted, out.MemberValues.BeatStart()));
			});

			GenericMemberFlags commonAvailableMemberFlags = SelectedItems.empty() ? GenericMemberFlags_None : GenericMemberFlags_All;
			GenericList commonListType = SelectedItems.empty() ? GenericList::Count : SelectedItems[0].List;
			size_t perListSelectionCounts[EnumCount<GenericList>] = {};
			{

				for (const TempChartItem& item : SelectedItems)
				{
					commonAvailableMemberFlags &= item.AvailableMembers;
					perListSelectionCounts[EnumToIndex(item.List)]++;
					if (item.List != commonListType)
						commonListType = GenericList::Count;
				}
			}

			GenericMemberFlags commonEqualMemberFlags = commonAvailableMemberFlags;
			AllGenericMembersUnionArray sharedValues = {};
			AllGenericMembersUnionArray mixedValuesMin = {};
			AllGenericMembersUnionArray mixedValuesMax = {};
			if (!SelectedItems.empty())
			{
				for (GenericMember member = {}; member < GenericMember::Count; IncrementEnum(member))
				{
					sharedValues[member] = SelectedItems[0].MemberValues[member];
					mixedValuesMin[member] = sharedValues[member];
					mixedValuesMax[member] = sharedValues[member];
				}

				for (const TempChartItem& item : SelectedItems)
				{
					for (GenericMember member = {}; member < GenericMember::Count; IncrementEnum(member))
					{
						if ((commonAvailableMemberFlags & EnumToFlag(member)) && item.MemberValues[member] != sharedValues[member])
							commonEqualMemberFlags &= ~EnumToFlag(member);

						const auto& v = item.MemberValues[member];
						auto& min = mixedValuesMin[member];
						auto& max = mixedValuesMax[member];
						switch (member)
						{
						case GenericMember::B8_IsSelected: { /* ... */ } break;
						case GenericMember::B8_BarLineVisible: { /* ... */ } break;
						case GenericMember::I16_BalloonPopCount: { min.I16 = Min(min.I16, v.I16); max.I16 = Max(max.I16, v.I16); } break;
						case GenericMember::F32_ScrollSpeed: { min.F32 = Min(min.F32, v.F32); max.F32 = Max(max.F32, v.F32); } break;
						case GenericMember::Beat_Start: { min.Beat = Min(min.Beat, v.Beat); max.Beat = Max(max.Beat, v.Beat); } break;
						case GenericMember::Beat_Duration: { min.Beat = Min(min.Beat, v.Beat); max.Beat = Max(max.Beat, v.Beat); } break;
						case GenericMember::Time_Offset: { min.Time = Min(min.Time, v.Time); max.Time = Max(max.Time, v.Time); } break;
						case GenericMember::NoteType_V:
						{
							min.NoteType = static_cast<NoteType>(Min(EnumToIndex(min.NoteType), EnumToIndex(v.NoteType)));
							max.NoteType = static_cast<NoteType>(Max(EnumToIndex(max.NoteType), EnumToIndex(v.NoteType)));
						} break;
						case GenericMember::Tempo_V: { min.Tempo.BPM = Min(min.Tempo.BPM, v.Tempo.BPM); max.Tempo.BPM = Max(max.Tempo.BPM, v.Tempo.BPM); } break;
						case GenericMember::TimeSignature_V:
						{
							min.TimeSignature.Numerator = Min(min.TimeSignature.Numerator, v.TimeSignature.Numerator);
							max.TimeSignature.Numerator = Max(max.TimeSignature.Numerator, v.TimeSignature.Numerator);
							min.TimeSignature.Denominator = Min(min.TimeSignature.Denominator, v.TimeSignature.Denominator);
							max.TimeSignature.Denominator = Max(max.TimeSignature.Denominator, v.TimeSignature.Denominator);
						} break;
						case GenericMember::CStr_Lyric: { /* ... */ } break;
						default: assert(false); break;
						}
					}
				}
			}

			if (SelectedItems.empty())
			{
				Gui::PushFont(FontLarge_EN);
				Gui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				Gui::PushStyleColor(ImGuiCol_Text, Gui::GetColorU32(ImGuiCol_TextDisabled));
				Gui::PushStyleColor(ImGuiCol_Button, 0x00000000);
				Gui::Button("( Nothing Selected )", { Gui::GetContentRegionAvail().x, Gui::GetFrameHeight() * 2.0f });
				Gui::PopStyleColor(2);
				Gui::PopItemFlag();
				Gui::PopFont();
			}
			else
			{
				if (Gui::Property::BeginTable(ImGuiTableFlags_BordersInner))
				{
					static constexpr cstr listTypeNames[] = { "Tempos", "Time Signatures", "Notes", "Notes", "Notes", "Scroll Speeds", "Bar Lines", "Go-Go Ranges", "Lyrics", };
					static_assert(ArrayCount(listTypeNames) == EnumCount<GenericList>);

					Gui::Property::Property([&]
					{
						std::string_view selectedListName = (commonListType < GenericList::Count) ? listTypeNames[EnumToIndex(commonListType)] : "Items";
						if (SelectedItems.size() == 1 && !selectedListName.empty())
							selectedListName = selectedListName.substr(0, selectedListName.size() - sizeof('s'));

						Gui::AlignTextToFramePadding();
						Gui::Text("Selected %.*s: %zu", FmtStrViewArgs(selectedListName), SelectedItems.size());
					});
					Gui::Property::Value([&]()
					{
						const TimeSpace displaySpace = *Settings.General.DisplayTimeInSongSpace ? TimeSpace::Song : TimeSpace::Chart;
						auto chartToDisplaySpace = [&](Time v) -> Time { return ConvertTimeSpace(v, TimeSpace::Chart, displaySpace, context.Chart); };

						// TODO: Also account for + Beat_Duration (?)
						Gui::AlignTextToFramePadding();
						if (SelectedItems.size() == 1)
							Gui::TextDisabled("%s",
								chartToDisplaySpace(context.BeatToTime(mixedValuesMin.BeatStart())).ToString().Data);
						else
							Gui::TextDisabled("(%s ... %s)",
								chartToDisplaySpace(context.BeatToTime(mixedValuesMin.BeatStart())).ToString().Data,
								chartToDisplaySpace(context.BeatToTime(mixedValuesMax.BeatStart())).ToString().Data);
					});

					if (commonListType >= GenericList::Count)
					{
						Gui::Property::PropertyTextValueFunc("Refine Selection", [&]
						{
							for (GenericList list = {}; list < GenericList::Count; IncrementEnum(list))
							{
								if (perListSelectionCounts[EnumToIndex(list)] == 0)
									continue;

								char buttonName[64];
								sprintf_s(buttonName, "[ %s ]  x%zu", listTypeNames[EnumToIndex(list)], perListSelectionCounts[EnumToIndex(list)]);
								if (list == GenericList::Notes_Master) { strcat_s(buttonName, " (Master)"); }
								if (list == GenericList::Notes_Expert) { strcat_s(buttonName, " (Expert)"); }

								Gui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, { 0.0f, 0.0f });
								Gui::PushStyleColor(ImGuiCol_Button, Gui::GetColorU32(ImGuiCol_FrameBg));
								Gui::PushStyleColor(ImGuiCol_ButtonHovered, Gui::GetColorU32(ImGuiCol_FrameBgHovered));
								Gui::PushStyleColor(ImGuiCol_ButtonActive, Gui::GetColorU32(ImGuiCol_FrameBgActive));
								if (Gui::Button(buttonName, vec2(-1.0f, 0.0f)))
								{
									const GenericMemberUnion unselectedValue = {};
									for (const auto& selectedItem : SelectedItems)
									{
										if (selectedItem.List != list)
											TrySetGeneric(course, selectedItem.List, selectedItem.Index, GenericMember::B8_IsSelected, unselectedValue);
									}
								}
								Gui::PopStyleColor(3);
								Gui::PopStyleVar(1);
							}
						});
					}

					b8 disableChangePropertiesCommandMerge = false;
					GenericMemberFlags outModifiedMembers = GenericMemberFlags_None;
					for (const GenericMember member : { GenericMember::NoteType_V, GenericMember::I16_BalloonPopCount, GenericMember::Time_Offset,
						GenericMember::Tempo_V, GenericMember::TimeSignature_V, GenericMember::F32_ScrollSpeed, GenericMember::B8_BarLineVisible })
					{
						if (!(commonAvailableMemberFlags & EnumToFlag(member)))
							continue;

						b8 valueWasChanged = false;
						switch (member)
						{
						case GenericMember::B8_BarLineVisible:
						{
							Gui::Property::PropertyTextValueFunc("Bar Line Visible", [&]
							{
								enum class VisibilityType { Visible, Hidden, Count }; static constexpr cstr visibilityTypeNames[] = { "Visible", "Hidden", };
								auto v = !(commonEqualMemberFlags & EnumToFlag(member)) ? VisibilityType::Count : sharedValues.BarLineVisible() ? VisibilityType::Visible : VisibilityType::Hidden;

								Gui::PushItemWidth(-1.0f);
								if (Gui::ComboEnum("##BarLineVisible", &v, visibilityTypeNames, ImGuiComboFlags_None))
								{
									for (auto& selectedItem : SelectedItems)
									{
										auto& isVisible = selectedItem.MemberValues.BarLineVisible();
										isVisible = (v == VisibilityType::Visible) ? true : (v == VisibilityType::Hidden) ? false : isVisible;
									}
									valueWasChanged = true;
									disableChangePropertiesCommandMerge = true;
								}
							});
						} break;
						case GenericMember::I16_BalloonPopCount:
						{
							b8 isAnyBalloonNoteSelected = false;
							for (const auto& selectedItem : SelectedItems)
								isAnyBalloonNoteSelected |= IsBalloonNote(selectedItem.MemberValues.NoteType());

							MultiEditWidgetParam widgetIn = {};
							widgetIn.DataType = ImGuiDataType_S16;
							widgetIn.Value.I16 = sharedValues.BalloonPopCount();
							widgetIn.HasMixedValues = !(commonEqualMemberFlags & EnumToFlag(member));
							widgetIn.MixedValuesMin.I16 = mixedValuesMin.BalloonPopCount();
							widgetIn.MixedValuesMax.I16 = mixedValuesMax.BalloonPopCount();
							widgetIn.EnableStepButtons = true;
							widgetIn.ButtonStep.I16 = 1;
							widgetIn.ButtonStepFast.I16 = 4;
							widgetIn.EnableDragLabel = false;
							widgetIn.FormatString = "%d";
							widgetIn.EnableDragLabel = true;
							widgetIn.DragLabelSpeed = 0.05f;
							Gui::BeginDisabled(!isAnyBalloonNoteSelected);
							const MultiEditWidgetResult widgetOut = GuiPropertyMultiSelectionEditWidget("Balloon Pop Count", widgetIn);
							Gui::EndDisabled();

							if (widgetOut.HasValueExact)
							{
								for (auto& selectedItem : SelectedItems)
								{
									if (IsBalloonNote(selectedItem.MemberValues.NoteType()))
										selectedItem.MemberValues.BalloonPopCount() = Clamp(widgetOut.ValueExact.I16, MinBalloonCount, MaxBalloonCount);
								}
								valueWasChanged = true;
							}
							else if (widgetOut.HasValueIncrement)
							{
								for (auto& selectedItem : SelectedItems)
								{
									if (IsBalloonNote(selectedItem.MemberValues.NoteType()))
										selectedItem.MemberValues.BalloonPopCount() = Clamp(static_cast<i16>(selectedItem.MemberValues.BalloonPopCount() + widgetOut.ValueIncrement.I16), MinBalloonCount, MaxBalloonCount);
								}
								valueWasChanged = true;
							}
						} break;
						case GenericMember::F32_ScrollSpeed:
						{
							b8 areAllScrollSpeedsTheSame = (commonEqualMemberFlags & EnumToFlag(member));
							b8 areAllScrollTemposTheSame = true;
							Tempo commonScrollTempo = {}, minScrollTempo {}, maxScrollTempo {};
							for (const auto& selectedItem : SelectedItems)
							{
								const Tempo scrollTempo = ScrollSpeedToTempo(selectedItem.MemberValues.ScrollSpeed(), selectedItem.BaseScrollTempo);
								if (&selectedItem == &SelectedItems[0])
								{
									commonScrollTempo = minScrollTempo = maxScrollTempo = scrollTempo;
								}
								else
								{
									minScrollTempo.BPM = Min(minScrollTempo.BPM, scrollTempo.BPM);
									maxScrollTempo.BPM = Max(maxScrollTempo.BPM, scrollTempo.BPM);
									areAllScrollTemposTheSame &= ApproxmiatelySame(scrollTempo.BPM, commonScrollTempo.BPM, 0.001f);
								}
							}

							for (size_t i = 0; i < 2; i++)
							{
								MultiEditWidgetParam widgetIn = {};
								widgetIn.EnableStepButtons = true;
								if (i == 0)
								{
									widgetIn.Value.F32 = sharedValues.ScrollSpeed();
									widgetIn.HasMixedValues = !(commonEqualMemberFlags & EnumToFlag(member));
									widgetIn.MixedValuesMin.F32 = mixedValuesMin.ScrollSpeed();
									widgetIn.MixedValuesMax.F32 = mixedValuesMax.ScrollSpeed();
									widgetIn.ButtonStep.F32 = 0.1f;
									widgetIn.ButtonStepFast.F32 = 0.5f;
									widgetIn.DragLabelSpeed = 0.005f;
									widgetIn.FormatString = "%gx";
									widgetIn.EnableClamp = true;
									widgetIn.ValueClampMin.F32 = MinScrollSpeed;
									widgetIn.ValueClampMax.F32 = MaxScrollSpeed;
								}
								else
								{
									widgetIn.Value.F32 = commonScrollTempo.BPM;
									widgetIn.HasMixedValues = !areAllScrollTemposTheSame;
									widgetIn.MixedValuesMin.F32 = minScrollTempo.BPM;
									widgetIn.MixedValuesMax.F32 = maxScrollTempo.BPM;
									widgetIn.ButtonStep.F32 = 1.0f;
									widgetIn.ButtonStepFast.F32 = 10.0f;
									widgetIn.DragLabelSpeed = 1.0f;
									widgetIn.FormatString = "%g BPM";
									widgetIn.EnableClamp = true;
									widgetIn.ValueClampMin.F32 = MinBPM;
									widgetIn.ValueClampMax.F32 = MaxBPM;
								}
								const MultiEditWidgetResult widgetOut = GuiPropertyMultiSelectionEditWidget((i == 0) ? "Scroll Speed" : "Scroll Speed Tempo", widgetIn);
								if (widgetOut.HasValueExact)
								{
									for (auto& selectedItem : SelectedItems)
									{
										if (i == 0)
											selectedItem.MemberValues.ScrollSpeed() = widgetOut.ValueExact.F32;
										else
											selectedItem.MemberValues.ScrollSpeed() = ScrollTempoToSpeed(Tempo(widgetOut.ValueExact.F32), selectedItem.BaseScrollTempo);
									}
									valueWasChanged = true;
								}
								else if (widgetOut.HasValueIncrement)
								{
									for (auto& selectedItem : SelectedItems)
									{
										if (i == 0)
										{
											selectedItem.MemberValues.ScrollSpeed() = Clamp(selectedItem.MemberValues.ScrollSpeed() + widgetOut.ValueIncrement.F32, MinScrollSpeed, MaxScrollSpeed);
										}
										else if (selectedItem.BaseScrollTempo.BPM != 0.0f)
										{
											const Tempo oldScrollTempo = ScrollSpeedToTempo(selectedItem.MemberValues.ScrollSpeed(), selectedItem.BaseScrollTempo);
											const Tempo newScrollTempo = Tempo(oldScrollTempo.BPM + widgetOut.ValueIncrement.F32);
											selectedItem.MemberValues.ScrollSpeed() = ScrollTempoToSpeed(Tempo(Clamp(newScrollTempo.BPM, MinBPM, MaxBPM)), selectedItem.BaseScrollTempo);
										}
									}
									valueWasChanged = true;
								}
							}

							for (size_t i = 0; i < 2; i++)
							{
								// TODO: Maybe option to switch between Beat/Time interpolation modes (?)
								static constexpr auto getT = [](const TempChartItem& item) -> f64 { return static_cast<f64>(item.MemberValues.BeatStart().Ticks); };
								static constexpr auto getInterpolatedScrollSpeed = [](const TempChartItem& startItem, const TempChartItem& endItem, const TempChartItem& thisItem, f32 startValue, f32 endValue) -> f32
								{
									return static_cast<f32>(ConvertRange<f64>(getT(startItem), getT(endItem), startValue, endValue, getT(thisItem)));
								};

								TempChartItem* startItem = !SelectedItems.empty() ? &SelectedItems.front() : nullptr;
								TempChartItem* endItem = !SelectedItems.empty() ? &SelectedItems.back() : nullptr;
								f32 inOutStartEnd[2] =
								{
									 (i == 0) ? startItem->MemberValues.ScrollSpeed() : ScrollSpeedToTempo(startItem->MemberValues.ScrollSpeed(), startItem->BaseScrollTempo).BPM,
									 (i == 0) ? endItem->MemberValues.ScrollSpeed() : ScrollSpeedToTempo(endItem->MemberValues.ScrollSpeed(), endItem->BaseScrollTempo).BPM,
								};

								const b8 isSelectionTooSmall = (SelectedItems.size() < 2);
								b8 isSelectionAlreadyInterpolated = true;
								if (!isSelectionTooSmall)
								{
									for (const auto& thisItem : SelectedItems)
									{
										const f32 thisValue = getInterpolatedScrollSpeed(*startItem, *endItem, thisItem, inOutStartEnd[0], inOutStartEnd[1]);
										if (!ApproxmiatelySame(thisItem.MemberValues.ScrollSpeed(), (i == 0) ? thisValue : ScrollTempoToSpeed(Tempo(thisValue), thisItem.BaseScrollTempo)))
											isSelectionAlreadyInterpolated = false;
									}
								}

								// NOTE: Invalid selection		-> "..." dummied out
								//		 All the same			-> "=" marker
								//		 Mixed values			-> "()" values
								//		 Already interpolated	-> regular values
								char previewBuffersStartEnd[2][64]; cstr previewStrings[2] = { nullptr, nullptr };
								if (isSelectionTooSmall)
								{
									previewStrings[0] = "...";
									previewStrings[1] = "...";
								}
								else if ((i == 0) ? areAllScrollSpeedsTheSame : areAllScrollTemposTheSame)
								{
									previewStrings[1] = "=";
								}
								else if (!isSelectionAlreadyInterpolated)
								{
									previewStrings[0] = previewBuffersStartEnd[0]; sprintf_s(previewBuffersStartEnd[0], (i == 0) ? "(%gx)" : "(%g BPM)", inOutStartEnd[0]);
									previewStrings[1] = previewBuffersStartEnd[1]; sprintf_s(previewBuffersStartEnd[1], (i == 0) ? "(%gx)" : "(%g BPM)", inOutStartEnd[1]);
								}

								Gui::BeginDisabled(isSelectionTooSmall);
								if ((i == 0) ?
									GuiPropertyRangeInterpolationEditWidget("Interpolate: Scroll Speed", inOutStartEnd, 0.1f, 0.5f, MinScrollSpeed, MaxScrollSpeed, "%gx", previewStrings) :
									GuiPropertyRangeInterpolationEditWidget("Interpolate: Scroll Speed Tempo", inOutStartEnd, 1.0f, 10.0f, MinBPM, MaxBPM, "%g BPM", previewStrings))
								{
									for (auto& thisItem : SelectedItems)
									{
										const f32 thisValue = getInterpolatedScrollSpeed(*startItem, *endItem, thisItem, inOutStartEnd[0], inOutStartEnd[1]);
										thisItem.MemberValues.ScrollSpeed() = (i == 0) ? thisValue : ScrollTempoToSpeed(Tempo(thisValue), thisItem.BaseScrollTempo);
									}
									valueWasChanged = true;
								}
								Gui::EndDisabled();
							}
						} break;
						case GenericMember::Time_Offset:
						{
							MultiEditWidgetParam widgetIn = {};
							widgetIn.EnableStepButtons = true;
							widgetIn.Value.F32 = sharedValues.TimeOffset().ToMS_F32();
							widgetIn.HasMixedValues = !(commonEqualMemberFlags & EnumToFlag(member));
							widgetIn.MixedValuesMin.F32 = mixedValuesMin.TimeOffset().ToMS_F32();
							widgetIn.MixedValuesMax.F32 = mixedValuesMax.TimeOffset().ToMS_F32();
							widgetIn.ButtonStep.F32 = 1.0f;
							widgetIn.ButtonStepFast.F32 = 5.0f;
							widgetIn.EnableDragLabel = true;
							widgetIn.DragLabelSpeed = 1.0f;
							widgetIn.FormatString = "%g ms";
							const MultiEditWidgetResult widgetOut = GuiPropertyMultiSelectionEditWidget("Time Offset", widgetIn);
							if (widgetOut.HasValueExact || widgetOut.HasValueIncrement)
							{
								if (widgetOut.HasValueExact)
								{
									const Time valueExact = Clamp(Time::FromMS(widgetOut.ValueExact.F32), MinNoteTimeOffset, MaxNoteTimeOffset);
									for (auto& selectedItem : SelectedItems)
										selectedItem.MemberValues.TimeOffset() = valueExact;
									valueWasChanged = true;
								}
								else if (widgetOut.HasValueIncrement)
								{
									const Time valueIncrement = Time::FromMS(widgetOut.ValueIncrement.F32);
									for (auto& selectedItem : SelectedItems)
										selectedItem.MemberValues.TimeOffset() = Clamp(selectedItem.MemberValues.TimeOffset() + valueIncrement, MinNoteTimeOffset, MaxNoteTimeOffset);
									valueWasChanged = true;
								}

								for (auto& selectedItem : SelectedItems)
								{
									if (ApproxmiatelySame(selectedItem.MemberValues.TimeOffset().Seconds, 0.0))
										selectedItem.MemberValues.TimeOffset() = Time::Zero();
								}
							}
						} break;
						case GenericMember::NoteType_V:
						{
							b8 isAnyRegularNoteSelected = false, isAnyDrumrollNoteSelected = false, isAnyBalloonNoteSelected = false;
							b8 areAllSelectedNotesSmall = true, areAllSelectedNotesBig = true;
							b8 perNoteTypeHasAtLeastOneSelected[EnumCount<NoteType>] = {};
							for (const auto& selectedItem : SelectedItems)
							{
								const auto noteType = selectedItem.MemberValues.NoteType();
								isAnyRegularNoteSelected |= IsRegularNote(noteType);
								isAnyDrumrollNoteSelected |= IsDrumrollNote(noteType);
								isAnyBalloonNoteSelected |= IsBalloonNote(noteType);
								areAllSelectedNotesSmall &= IsSmallNote(noteType);
								areAllSelectedNotesBig &= IsBigNote(noteType);
								perNoteTypeHasAtLeastOneSelected[EnumToIndex(noteType)] = true;
							}

							Gui::Property::PropertyTextValueFunc("Note Type", [&]
							{
								static constexpr cstr noteTypeNames[] = { "Don", "DON", "Ka", "KA", "Drumroll", "DRUMROLL", "Balloon", "BALLOON", };
								static_assert(ArrayCount(noteTypeNames) == EnumCount<NoteType>);

								const b8 isSingleNoteType = (commonEqualMemberFlags & EnumToFlag(member));
								char previewValue[256] {}; if (!isSingleNoteType) { previewValue[0] = '('; }
								for (i32 i = 0; i < EnumCountI32<NoteType>; i++) { if (perNoteTypeHasAtLeastOneSelected[i]) { strcat_s(previewValue, noteTypeNames[i]); strcat_s(previewValue, ", "); } }
								for (i32 i = ArrayCountI32(previewValue) - 1; i >= 0; i--) { if (previewValue[i] == ',') { if (!isSingleNoteType) { previewValue[i++] = ')'; } previewValue[i] = '\0'; break; } }

								Gui::SetNextItemWidth(-1.0f);
								Gui::PushStyleColor(ImGuiCol_Text, Gui::GetColorU32(ImGuiCol_Text, (commonEqualMemberFlags & EnumToFlag(member)) ? 1.0f : 0.6f));
								const b8 beginCombo = Gui::BeginCombo("##NoteType", previewValue, ImGuiComboFlags_None);
								Gui::PopStyleColor();
								if (beginCombo)
								{
									for (NoteType i = {}; i < NoteType::Count; IncrementEnum(i))
									{
										const b8 isSelected = perNoteTypeHasAtLeastOneSelected[EnumToIndex(i)];

										b8 isEnabled = false;
										isEnabled |= IsRegularNote(i) && isAnyRegularNoteSelected;
										isEnabled |= IsDrumrollNote(i) && isAnyDrumrollNoteSelected;
										isEnabled |= IsBalloonNote(i) && isAnyBalloonNoteSelected;

										if (Gui::Selectable(noteTypeNames[EnumToIndex(i)], isSelected, !isEnabled ? ImGuiSelectableFlags_Disabled : ImGuiSelectableFlags_None))
										{
											for (auto& selectedItem : SelectedItems)
											{
												// NOTE: Don't allow changing long notes and balloons for now at that would also require updating BeatDuration / BalloonPopCount
												auto& inOutNoteType = selectedItem.MemberValues.NoteType();
												if (IsLongNote(i) == IsLongNote(inOutNoteType) && IsBalloonNote(i) == IsBalloonNote(inOutNoteType))
													inOutNoteType = i;
											}
											valueWasChanged = true;
											disableChangePropertiesCommandMerge = true;
										}

										if (isSelected)
											Gui::SetItemDefaultFocus();
									}
									Gui::EndCombo();
								}
							});

							Gui::Property::PropertyTextValueFunc("Note Type Size", [&]
							{
								enum class NoteSizeType { Small, Big, Count }; static constexpr cstr noteSizeTypeNames[] = { "Small", "Big", };
								auto v = (!areAllSelectedNotesSmall && !areAllSelectedNotesBig) ? NoteSizeType::Count : IsBigNote(sharedValues.NoteType()) ? NoteSizeType::Big : NoteSizeType::Small;

								Gui::PushItemWidth(-1.0f);
								if (Gui::ComboEnum("##NoteTypeIsBig", &v, noteSizeTypeNames, ImGuiComboFlags_None))
								{
									for (auto& selectedItem : SelectedItems)
									{
										auto& inOutNoteType = selectedItem.MemberValues.NoteType();
										inOutNoteType = (v == NoteSizeType::Big) ? ToBigNote(inOutNoteType) : (v == NoteSizeType::Small) ? ToSmallNote(inOutNoteType) : inOutNoteType;
									}
									valueWasChanged = true;
									disableChangePropertiesCommandMerge = true;
								}
							});
						} break;
						case GenericMember::Tempo_V:
						{
							// TODO: X0.5/x2.0 buttons (?)
							MultiEditWidgetParam widgetIn = {};
							widgetIn.Value.F32 = sharedValues.Tempo().BPM;
							widgetIn.HasMixedValues = !(commonEqualMemberFlags & EnumToFlag(member));
							widgetIn.MixedValuesMin.F32 = mixedValuesMin.Tempo().BPM;
							widgetIn.MixedValuesMax.F32 = mixedValuesMax.Tempo().BPM;
							widgetIn.EnableStepButtons = true;
							widgetIn.ButtonStep.F32 = 1.0f;
							widgetIn.ButtonStepFast.F32 = 10.0f;
							widgetIn.DragLabelSpeed = 1.0f;
							widgetIn.FormatString = "%g BPM";
							widgetIn.EnableClamp = true;
							widgetIn.ValueClampMin.F32 = MinBPM;
							widgetIn.ValueClampMax.F32 = MaxBPM;
							const MultiEditWidgetResult widgetOut = GuiPropertyMultiSelectionEditWidget("Tempo", widgetIn);

							if (widgetOut.HasValueExact)
							{
								for (auto& selectedItem : SelectedItems)
									selectedItem.MemberValues.Tempo() = Tempo(widgetOut.ValueExact.F32);
								valueWasChanged = true;
							}
							else if (widgetOut.HasValueIncrement)
							{
								for (auto& selectedItem : SelectedItems)
									selectedItem.MemberValues.Tempo().BPM = Clamp(selectedItem.MemberValues.Tempo().BPM + widgetOut.ValueIncrement.F32, MinBPM, MaxBPM);
								valueWasChanged = true;
							}
						} break;
						case GenericMember::TimeSignature_V:
						{
							static constexpr i32 components = 2;

							MultiEditWidgetParam widgetIn = {};
							widgetIn.DataType = ImGuiDataType_S32;
							widgetIn.Components = components;
							widgetIn.EnableClamp = true;
							widgetIn.EnableStepButtons = true;
							widgetIn.UseSpinButtonsInstead = true;
							for (i32 c = 0; c < components; c++)
							{
								widgetIn.Value.I32_V[c] = sharedValues.TimeSignature()[c];
								widgetIn.HasMixedValues |= (static_cast<u32>(mixedValuesMin.TimeSignature()[c] != mixedValuesMax.TimeSignature()[c]) << c);
								widgetIn.MixedValuesMin.I32_V[c] = mixedValuesMin.TimeSignature()[c];
								widgetIn.MixedValuesMax.I32_V[c] = mixedValuesMax.TimeSignature()[c];
								widgetIn.ValueClampMin.I32_V[c] = MinTimeSignatureValue;
								widgetIn.ValueClampMax.I32_V[c] = MaxTimeSignatureValue;
								widgetIn.ButtonStep.I32_V[c] = 1;
								widgetIn.ButtonStepFast.I32_V[c] = 4;
							}
							widgetIn.EnableDragLabel = false;
							widgetIn.FormatString = "%d";
							const MultiEditWidgetResult widgetOut = GuiPropertyMultiSelectionEditWidget("Time Signature", widgetIn);

							if (widgetOut.HasValueExact || widgetOut.HasValueIncrement)
							{
								for (i32 c = 0; c < components; c++)
								{
									if (widgetOut.HasValueExact & (1 << c))
									{
										for (auto& selectedItem : SelectedItems)
											selectedItem.MemberValues.TimeSignature()[c] = widgetOut.ValueExact.I32_V[c];
										valueWasChanged = true;
									}
									else if (widgetOut.HasValueIncrement & (1 << c))
									{
										for (auto& selectedItem : SelectedItems)
											selectedItem.MemberValues.TimeSignature()[c] = Clamp(selectedItem.MemberValues.TimeSignature()[c] + widgetOut.ValueIncrement.I32_V[c], MinTimeSignatureValue, MaxTimeSignatureValue);
										valueWasChanged = true;
									}
								}
							}
						} break;
						default: { assert(false); } break;
						}

						if (valueWasChanged)
							outModifiedMembers |= EnumToFlag(member);
					}

					if (outModifiedMembers != GenericMemberFlags_None)
					{
						std::vector<Commands::ChangeMultipleGenericProperties::Data> propertiesToChange;
						propertiesToChange.reserve(SelectedItems.size());

						for (const TempChartItem& selectedItem : SelectedItems)
						{
							for (GenericMember member = {}; member < GenericMember::Count; IncrementEnum(member))
							{
								if (outModifiedMembers & EnumToFlag(member))
								{
									auto& out = propertiesToChange.emplace_back();
									out.Index = selectedItem.Index;
									out.List = selectedItem.List;
									out.Member = member;
									out.NewValue = selectedItem.MemberValues[member];
								}
							}
						}

						if (disableChangePropertiesCommandMerge)
							context.Undo.DisallowMergeForLastCommand();
						context.Undo.Execute<Commands::ChangeMultipleGenericProperties>(&course, std::move(propertiesToChange));
					}

					Gui::Property::EndTable();
				}
			}
		}
	}

	void ChartPropertiesWindow::DrawGui(ChartContext& context, const ChartPropertiesWindowIn& in, ChartPropertiesWindowOut& out)
	{
		Gui::UpdateSmoothScrollWindow();

		assert(context.ChartSelectedCourse != nullptr);
		auto& chart = context.Chart;
		auto& course = *context.ChartSelectedCourse;

		if (Gui::CollapsingHeader("Chart", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (Gui::Property::BeginTable(ImGuiTableFlags_BordersInner))
			{
				Gui::Property::PropertyTextValueFunc("Chart Title", [&]
				{
					Gui::SetNextItemWidth(-1.0f);
					if (Gui::InputTextWithHint("##ChartTitle", "n/a", &chart.ChartTitle.Base()))
						context.Undo.NotifyChangesWereMade();
				});
				Gui::Property::PropertyTextValueFunc("Chart Subtitle", [&]
				{
					Gui::SetNextItemWidth(-1.0f);
					if (Gui::InputTextWithHint("##ChartSubtitle", "n/a", &chart.ChartSubtitle.Base()))
						context.Undo.NotifyChangesWereMade();
				});
				Gui::Property::PropertyTextValueFunc("Chart Creator", [&]
				{
					Gui::SetNextItemWidth(-1.0f);
					if (Gui::InputTextWithHint("##ChartCreator", "n/a", &chart.ChartCreator))
						context.Undo.NotifyChangesWereMade();
				});
				Gui::Property::PropertyTextValueFunc("Song File Name", [&]
				{
					const b8 songIsLoading = in.IsSongAsyncLoading;
					cstr loadingText = SongLoadingTextAnimation.UpdateFrameAndGetText(songIsLoading, Gui::DeltaTime());
					SongFileNameInputBuffer = songIsLoading ? "" : chart.SongFileName;

					Gui::BeginDisabled(songIsLoading);
					const auto result = Gui::PathInputTextWithHintAndBrowserDialogButton("##SongFileName",
						songIsLoading ? loadingText : "song.ogg", &SongFileNameInputBuffer, songIsLoading ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_EnterReturnsTrue);
					Gui::EndDisabled();

					if (result.InputTextEdited)
					{
						out.LoadNewSong = true;
						out.NewSongFilePath = SongFileNameInputBuffer;
					}
					else if (result.BrowseButtonClicked)
					{
						out.BrowseOpenSong = true;
					}
				});
				Gui::Property::PropertyTextValueFunc("Song Volume", [&]
				{
					Gui::SetNextItemWidth(-1.0f);
					if (f32 v = ToPercent(chart.SongVolume); Gui::SliderFloat("##SongVolume", &v, ToPercent(MinVolume), ToPercent(MaxVolumeSoftLimit), "%.0f%%"))
					{
						chart.SongVolume = Clamp(FromPercent(v), MinVolume, MaxVolumeHardLimit);
						context.Undo.NotifyChangesWereMade();
					}
				});
				Gui::Property::PropertyTextValueFunc("Sound Effect Volume", [&]
				{
					Gui::SetNextItemWidth(-1.0f);
					if (f32 v = ToPercent(chart.SoundEffectVolume); Gui::SliderFloat("##SoundEffectVolume", &v, ToPercent(MinVolume), ToPercent(MaxVolumeSoftLimit), "%.0f%%"))
					{
						chart.SoundEffectVolume = Clamp(FromPercent(v), MinVolume, MaxVolumeHardLimit);
						context.Undo.NotifyChangesWereMade();
					}
				});
				Gui::Property::EndTable();
			}
		}

		if (Gui::CollapsingHeader("Selected Course", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (Gui::Property::BeginTable(ImGuiTableFlags_BordersInner))
			{
				Gui::Property::PropertyTextValueFunc("Difficulty Type", [&]
				{
					Gui::SetNextItemWidth(-1.0f);
					if (Gui::ComboEnum("##DifficultyType", &course.Type, DifficultyTypeNames))
						context.Undo.NotifyChangesWereMade();
				});
				Gui::Property::PropertyTextValueFunc("Difficulty Level", [&]
				{
					Gui::SetNextItemWidth(-1.0f);
					if (GuiDifficultyLevelStarSliderWidget("##DifficultyLevel", &course.Level, DifficultySliderStarsFitOnScreenLastFrame, DifficultySliderStarsWasHoveredLastFrame))
						context.Undo.NotifyChangesWereMade();
				});
#if 0 // TODO:
				Gui::Property::PropertyTextValueFunc("Score Init (TODO)", [&] { Gui::Text("%d", course.ScoreInit); });
				Gui::Property::PropertyTextValueFunc("Score Diff (TODO)", [&] { Gui::Text("%d", course.ScoreDiff); });
#endif
				Gui::Property::EndTable();
			}
		}
	}

	void ChartTempoWindow::DrawGui(ChartContext& context, ChartTimeline& timeline)
	{
		Gui::UpdateSmoothScrollWindow();

		assert(context.ChartSelectedCourse != nullptr);
		auto& chart = context.Chart;
		auto& course = *context.ChartSelectedCourse;

		if (Gui::CollapsingHeader("Sync", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (Gui::Property::BeginTable(ImGuiTableFlags_BordersInner))
			{
				static constexpr auto guiPropertyDragTimeAndSetCursorTimeButtonWidgets = [](ChartContext& context, const TimelineCamera& camera, cstr label, Time* inOutValue, TimeSpace storageSpace) -> b8
				{
					const TimeSpace displaySpace = *Settings.General.DisplayTimeInSongSpace ? TimeSpace::Song : TimeSpace::Chart;
					const Time min = ConvertTimeSpace(Time::Zero(), storageSpace, displaySpace, context.Chart), max = Time::FromSec(F64Max);

					b8 valueChanged = false;
					Gui::Property::Property([&]
					{
						Gui::SetNextItemWidth(-1.0f);
						f32 v = ConvertTimeSpace(*inOutValue, storageSpace, displaySpace, context.Chart).ToSec_F32();
						if (GuiDragLabelFloat(label, &v, TimelineDragScalarSpeedAtZoomSec(camera), min.ToSec_F32(), max.ToSec_F32()))
						{
							*inOutValue = ConvertTimeSpace(Time::FromSec(v), displaySpace, storageSpace, context.Chart);
							valueChanged = true;
						}
					});
					Gui::Property::Value([&]
					{
						Gui::SetNextItemWidth(-1.0f);
						Gui::PushID(label);
						f32 v = ConvertTimeSpace(*inOutValue, storageSpace, displaySpace, context.Chart).ToSec_F32();
						Gui::SameLineMultiWidget(2, [&](const Gui::MultiWidgetIt& i)
						{
							if (i.Index == 0 && Gui::DragFloat("##DragTime", &v, TimelineDragScalarSpeedAtZoomSec(camera), min.ToSec_F32(), max.ToSec_F32(),
								(*inOutValue <= Time::Zero()) ? "n/a" : Time::FromSec(v).ToString().Data, ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoRoundToFormat | ImGuiSliderFlags_NoInput))
							{
								*inOutValue = ConvertTimeSpace(Time::FromSec(v), displaySpace, storageSpace, context.Chart);
								valueChanged = true;
							}
							else if (i.Index == 1 && Gui::Button("Set Cursor##CursorTime", { Gui::CalcItemWidth(), 0.0f }))
							{
								*inOutValue = ClampBot(ConvertTimeSpace(context.GetCursorTime(), TimeSpace::Chart, storageSpace, context.Chart), Time::Zero());
								context.Undo.DisallowMergeForLastCommand();
								valueChanged = true;
							}
							return false;
						});
						Gui::PopID();
					});
					return valueChanged;
				};

				if (Time v = chart.ChartDuration; guiPropertyDragTimeAndSetCursorTimeButtonWidgets(context, timeline.Camera, "Chart Duration", &v, TimeSpace::Chart))
					context.Undo.Execute<Commands::ChangeChartDuration>(&chart, v);

				if (Time v = chart.SongDemoStartTime; guiPropertyDragTimeAndSetCursorTimeButtonWidgets(context, timeline.Camera, "Song Demo Start", &v, TimeSpace::Song))
					context.Undo.Execute<Commands::ChangeSongDemoStartTime>(&chart, v);

				Gui::Property::Property([&]
				{
					Gui::SetNextItemWidth(-1.0f);
					if (f32 v = chart.SongOffset.ToMS_F32(); GuiDragLabelFloat("Song Offset##DragFloatLabel", &v, TimelineDragScalarSpeedAtZoomMS(timeline.Camera)))
						context.Undo.Execute<Commands::ChangeSongOffset>(&chart, Time::FromMS(v));
				});
				Gui::Property::Value([&]
				{
					Gui::SetNextItemWidth(-1.0f);
					if (f32 v = chart.SongOffset.ToMS_F32(); Gui::SpinFloat("##SongOffset", &v, 1.0f, 10.0f, "%.2f ms", ImGuiInputTextFlags_None))
						context.Undo.Execute<Commands::ChangeSongOffset>(&chart, Time::FromMS(v));
				});
				// TODO: Disable merge if made inactive this frame (?)
				// if (Gui::IsItemDeactivatedAfterEdit()) {}

				Gui::Property::EndTable();
			}
		}

		if (Gui::CollapsingHeader("Tempo", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (Gui::Property::BeginTable(ImGuiTableFlags_BordersInner))
			{
				// NOTE: This might seem like a strange design decision at first (hence it at least being optional)
				//		 but the widgets here can only be used for inserting / editing (single) changes directly underneath the cursor
				//		 which in case of an active selection (in the properties window) can become confusing
				//		 since there then are multiple edit widgets shown to the user with similar labels yet that differ in functionality.
				//		 To make it clear that selected items can only be edited via the properties window, these regular widgets here will therefore be disabled.
				b8 disableWidgetsBeacuseOfSelection = false;
				if (*Settings.General.DisableTempoWindowWidgetsIfHasSelection)
				{
					b8 isAnyItemOtherThanNotesSelected = false;
					ForEachSelectedChartItem(course, [&](const ForEachChartItemData& it) { if (!IsNotesList(it.List)) { isAnyItemOtherThanNotesSelected = true; } });
					disableWidgetsBeacuseOfSelection = isAnyItemOtherThanNotesSelected;
				}

				const Beat cursorBeat = FloorBeatToGrid(context.GetCursorBeat(), GetGridBeatSnap(timeline.CurrentGridBarDivision));
				// NOTE: Specifically to prevent ugly "flashing" between add/remove labels during playback
				const b8 disallowRemoveButton = (cursorBeat.Ticks < 0) || context.GetIsPlayback();
				Gui::BeginDisabled(disableWidgetsBeacuseOfSelection || cursorBeat.Ticks < 0);

				const TempoChange* tempoChangeAtCursor = course.TempoMap.Tempo.TryFindLastAtBeat(cursorBeat);
				const Tempo tempoAtCursor = (tempoChangeAtCursor != nullptr) ? tempoChangeAtCursor->Tempo : FallbackTempo;
				auto insertOrUpdateCursorTempoChange = [&](Tempo newTempo)
				{
					if (tempoChangeAtCursor == nullptr || tempoChangeAtCursor->Beat != cursorBeat)
						context.Undo.Execute<Commands::AddTempoChange>(&course.TempoMap, TempoChange(cursorBeat, newTempo));
					else
						context.Undo.Execute<Commands::UpdateTempoChange>(&course.TempoMap, TempoChange(cursorBeat, newTempo));
				};

				Gui::Property::Property([&]
				{
					Gui::SetNextItemWidth(-1.0f);
					if (f32 v = tempoAtCursor.BPM; GuiDragLabelFloat("Tempo##DragFloatLabel", &v, 1.0f, MinBPM, MaxBPM, ImGuiSliderFlags_AlwaysClamp))
						insertOrUpdateCursorTempoChange(Tempo(v));
				});
				Gui::Property::Value([&]
				{
					Gui::SetNextItemWidth(-1.0f);
					if (f32 v = tempoAtCursor.BPM; Gui::SpinFloat("##TempoAtCursor", &v, 1.0f, 10.0f, "%g BPM", ImGuiInputTextFlags_None))
						insertOrUpdateCursorTempoChange(Tempo(Clamp(v, MinBPM, MaxBPM)));

					if (!disallowRemoveButton && tempoChangeAtCursor != nullptr && tempoChangeAtCursor->Beat == cursorBeat)
					{
						if (Gui::Button("Remove##TempoAtCursor", vec2(-1.0f, 0.0f)))
							context.Undo.Execute<Commands::RemoveTempoChange>(&course.TempoMap, cursorBeat);
					}
					else
					{
						if (Gui::Button("Add##TempoAtCursor", vec2(-1.0f, 0.0f)))
							insertOrUpdateCursorTempoChange(tempoAtCursor);
					}
				});

				Gui::Property::PropertyTextValueFunc("Time Signature", [&]
				{
					const TimeSignatureChange* signatureChangeAtCursor = course.TempoMap.Signature.TryFindLastAtBeat(cursorBeat);
					const TimeSignature signatureAtCursor = (signatureChangeAtCursor != nullptr) ? signatureChangeAtCursor->Signature : FallbackTimeSignature;
					auto insertOrUpdateCursorSignatureChange = [&](TimeSignature newSignature)
					{
						// TODO: Also floor cursor beat to next whole bar (?)
						if (signatureChangeAtCursor == nullptr || signatureChangeAtCursor->Beat != cursorBeat)
							context.Undo.Execute<Commands::AddTimeSignatureChange>(&course.TempoMap, TimeSignatureChange(cursorBeat, newSignature));
						else
							context.Undo.Execute<Commands::UpdateTimeSignatureChange>(&course.TempoMap, TimeSignatureChange(cursorBeat, newSignature));
					};

					Gui::SetNextItemWidth(-1.0f);
					if (ivec2 v = { signatureAtCursor.Numerator, signatureAtCursor.Denominator };
						GuiInputFraction("##SignatureAtCursor", &v, ivec2(MinTimeSignatureValue, MaxTimeSignatureValue), 1, 4))
						insertOrUpdateCursorSignatureChange(TimeSignature(v[0], v[1]));

					if (!disallowRemoveButton && signatureChangeAtCursor != nullptr && signatureChangeAtCursor->Beat == cursorBeat)
					{
						if (Gui::Button("Remove##SignatureAtCursor", vec2(-1.0f, 0.0f)))
							context.Undo.Execute<Commands::RemoveTimeSignatureChange>(&course.TempoMap, cursorBeat);
					}
					else
					{
						if (Gui::Button("Add##SignatureAtCursor", vec2(-1.0f, 0.0f)))
							insertOrUpdateCursorSignatureChange(signatureAtCursor);
					}
				});

				const ScrollChange* scrollChangeChangeAtCursor = course.ScrollChanges.TryFindLastAtBeat(cursorBeat);
				auto insertOrUpdateCursorScrollSpeedChange = [&](f32 newScrollSpeed)
				{
					if (scrollChangeChangeAtCursor == nullptr || scrollChangeChangeAtCursor->BeatTime != cursorBeat)
						context.Undo.Execute<Commands::AddScrollChange>(&course.ScrollChanges, ScrollChange { cursorBeat, newScrollSpeed });
					else
						context.Undo.Execute<Commands::UpdateScrollChange>(&course.ScrollChanges, ScrollChange { cursorBeat, newScrollSpeed });
				};

				Gui::Property::Property([&]
				{
					Gui::SetNextItemWidth(-1.0f);
					if (f32 v = (scrollChangeChangeAtCursor == nullptr) ? 1.0f : scrollChangeChangeAtCursor->ScrollSpeed;
						GuiDragLabelFloat("Scroll Speed##DragFloatLabel", &v, 0.005f, MinScrollSpeed, MaxScrollSpeed, ImGuiSliderFlags_AlwaysClamp))
						insertOrUpdateCursorScrollSpeedChange(v);
				});
				Gui::Property::Value([&]
				{
					Gui::SetNextItemWidth(-1.0f); Gui::SameLineMultiWidget(2, [&](const Gui::MultiWidgetIt& i)
					{
						if (i.Index == 0)
						{
							if (f32 v = (scrollChangeChangeAtCursor == nullptr) ? 1.0f : scrollChangeChangeAtCursor->ScrollSpeed;
								Gui::SpinFloat("##ScrollSpeedAtCursor", &v, 0.1f, 0.5f, "%gx"))
								insertOrUpdateCursorScrollSpeedChange(Clamp(v, MinScrollSpeed, MaxScrollSpeed));
						}
						else
						{
							if (f32 v = ScrollSpeedToTempo((scrollChangeChangeAtCursor == nullptr) ? 1.0f : scrollChangeChangeAtCursor->ScrollSpeed, tempoAtCursor).BPM;
								Gui::SpinFloat("##ScrollTempoAtCursor", &v, 1.0f, 10.0f, "%g BPM"))
								insertOrUpdateCursorScrollSpeedChange(ScrollTempoToSpeed(Tempo(Clamp(v, MinBPM, MaxBPM)), tempoAtCursor));
						}
						return false;
					});

					if (!disallowRemoveButton && scrollChangeChangeAtCursor != nullptr && scrollChangeChangeAtCursor->BeatTime == cursorBeat)
					{
						if (Gui::Button("Remove##ScrollSpeedAtCursor", vec2(-1.0f, 0.0f)))
							context.Undo.Execute<Commands::RemoveScrollChange>(&course.ScrollChanges, cursorBeat);
					}
					else
					{
						if (Gui::Button("Add##ScrollSpeedAtCursor", vec2(-1.0f, 0.0f)))
							insertOrUpdateCursorScrollSpeedChange((scrollChangeChangeAtCursor != nullptr) ? scrollChangeChangeAtCursor->ScrollSpeed : 1.0f);
					}
				});

				Gui::Property::PropertyTextValueFunc("Bar Line Visibility", [&]
				{
					const BarLineChange* barLineChangeAtCursor = course.BarLineChanges.TryFindLastAtBeat(cursorBeat);
					auto insertOrUpdateCursorBarLineChange = [&](b8 newIsVisible)
					{
						if (barLineChangeAtCursor == nullptr || barLineChangeAtCursor->BeatTime != cursorBeat)
							context.Undo.Execute<Commands::AddBarLineChange>(&course.BarLineChanges, BarLineChange { cursorBeat, newIsVisible });
						else
							context.Undo.Execute<Commands::UpdateBarLineChange>(&course.BarLineChanges, BarLineChange { cursorBeat, newIsVisible });
					};

					static constexpr auto guiOnOffButton = [](cstr label, cstr onLabel, cstr offLabel, b8* inOutIsOn) -> b8
					{
						b8 valueChanged = false;
						Gui::PushID(label); Gui::SameLineMultiWidget(2, [&](const Gui::MultiWidgetIt& i)
						{
							const f32 alphaFactor = ((i.Index == 0) == *inOutIsOn) ? 1.0f : 0.5f;
							Gui::PushStyleColor(ImGuiCol_Button, Gui::GetColorU32(ImGuiCol_Button, alphaFactor));
							Gui::PushStyleColor(ImGuiCol_ButtonHovered, Gui::GetColorU32(ImGuiCol_ButtonHovered, alphaFactor));
							Gui::PushStyleColor(ImGuiCol_ButtonActive, Gui::GetColorU32(ImGuiCol_ButtonActive, alphaFactor));
							if (Gui::Button((i.Index == 0) ? onLabel : offLabel, { Gui::CalcItemWidth(), 0.0f })) { *inOutIsOn = (i.Index == 0); valueChanged = true; }
							Gui::PopStyleColor(3);
							return false;
						});
						Gui::PopID(); return valueChanged;
					};

					Gui::SetNextItemWidth(-1.0f);
					if (b8 v = (barLineChangeAtCursor == nullptr) ? true : barLineChangeAtCursor->IsVisible; guiOnOffButton("##OnOffBarLineAtCursor", "Visible", "Hidden", &v))
						insertOrUpdateCursorBarLineChange(v);

					if (!disallowRemoveButton && barLineChangeAtCursor != nullptr && barLineChangeAtCursor->BeatTime == cursorBeat)
					{
						if (Gui::Button("Remove##BarLineAtCursor", vec2(-1.0f, 0.0f)))
							context.Undo.Execute<Commands::RemoveBarLineChange>(&course.BarLineChanges, cursorBeat);
					}
					else
					{
						if (Gui::Button("Add##BarLineAtCursor", vec2(-1.0f, 0.0f)))
							insertOrUpdateCursorBarLineChange((barLineChangeAtCursor != nullptr) ? barLineChangeAtCursor->IsVisible : true);
					}
				});
				Gui::Property::PropertyTextValueFunc("Go-Go Time", [&]
				{
					const GoGoRange* gogoRangeAtCursor = course.GoGoRanges.TryFindOverlappingBeat(cursorBeat, cursorBeat);

					const b8 hasRangeSelection = timeline.RangeSelection.IsActiveAndHasEnd();
					Gui::BeginDisabled(!hasRangeSelection);
					if (Gui::Button("Set from Range Selection##GoGoAtCursor", vec2(-1.0f, 0.0f)))
					{
						const Beat rangeSelectionMin = timeline.RoundBeatToCurrentGrid(timeline.RangeSelection.GetMin());
						const Beat rangeSelectionMax = timeline.RoundBeatToCurrentGrid(timeline.RangeSelection.GetMax());
						auto gogoIntersectsSelection = [&](const GoGoRange& gogo) { return (gogo.GetStart() < rangeSelectionMax) && (gogo.GetEnd() > rangeSelectionMin); };

						// TODO: Try to shorten/move intersecting gogo ranges instead of removing them outright
						SortedGoGoRangesList newGoGoRanges = course.GoGoRanges;
						erase_remove_if(newGoGoRanges.Sorted, gogoIntersectsSelection);
						newGoGoRanges.InsertOrUpdate(GoGoRange { rangeSelectionMin, (rangeSelectionMax - rangeSelectionMin) });

						context.Undo.Execute<Commands::AddGoGoRange>(&course.GoGoRanges, std::move(newGoGoRanges));
					}
					Gui::EndDisabled();

					Gui::BeginDisabled(gogoRangeAtCursor == nullptr);
					if (Gui::Button("Remove##GoGoAtCursor", vec2(-1.0f, 0.0f)))
					{
						if (gogoRangeAtCursor != nullptr)
							context.Undo.Execute<Commands::RemoveGoGoRange>(&course.GoGoRanges, gogoRangeAtCursor->BeatTime);
					}
					Gui::EndDisabled();
				});

				Gui::EndDisabled();

				Gui::Property::EndTable();
			}

			size_t nonScrollChangeSelectedItemCount = 0;
			ForEachSelectedChartItem(course, [&](const ForEachChartItemData& it) { nonScrollChangeSelectedItemCount += (it.List != GenericList::ScrollChanges); });

			Gui::BeginDisabled(nonScrollChangeSelectedItemCount <= 0);
			if (Gui::Button("Insert Scroll Changes (Selection)", vec2(-1.0f, 0.0f)))
			{
				std::vector<ScrollChange*> scrollChangesThatAlreadyExist; scrollChangesThatAlreadyExist.reserve(nonScrollChangeSelectedItemCount);
				std::vector<ScrollChange> scrollChangesToAdd; scrollChangesToAdd.reserve(nonScrollChangeSelectedItemCount);
				ForEachSelectedChartItem(course, [&](const ForEachChartItemData& it)
				{
					if (it.List != GenericList::ScrollChanges)
					{
						const Beat itBeat = it.GetBeat(course);
						if (ScrollChange* lastScrollChange = course.ScrollChanges.TryFindLastAtBeat(itBeat); lastScrollChange != nullptr && lastScrollChange->BeatTime == itBeat)
							scrollChangesThatAlreadyExist.push_back(lastScrollChange);
						else
							scrollChangesToAdd.push_back(ScrollChange { itBeat, (lastScrollChange != nullptr) ? lastScrollChange->ScrollSpeed : 1.0f });
					}
				});

				if (!scrollChangesThatAlreadyExist.empty() || !scrollChangesToAdd.empty())
				{
					if (*Settings.General.InsertSelectionScrollChangesUnselectOld)
						ForEachSelectedChartItem(course, [&](const ForEachChartItemData& it) { it.SetIsSelected(course, false); });

					if (*Settings.General.InsertSelectionScrollChangesSelectNew)
					{
						for (auto* it : scrollChangesThatAlreadyExist) { it->IsSelected = true; }
						for (auto& it : scrollChangesToAdd) { it.IsSelected = true; }
					}

					if (!scrollChangesToAdd.empty())
						context.Undo.Execute<Commands::AddMultipleScrollChanges>(&course.ScrollChanges, std::move(scrollChangesToAdd));
				}
			}
			Gui::EndDisabled();
		}
	}

	void ChartLyricsWindow::DrawGui(ChartContext& context, ChartTimeline& timeline)
	{
		assert(context.ChartSelectedCourse != nullptr);
		auto& chart = context.Chart;
		auto& course = *context.ChartSelectedCourse;

		// TODO: Separate lyrics tab with options to write/import lyrics text in some standard format (?)
		const Beat cursorBeat = FloorBeatToGrid(context.GetCursorBeat(), GetGridBeatSnap(timeline.CurrentGridBarDivision));
		const LyricChange* lyricChangeAtCursor = context.ChartSelectedCourse->Lyrics.TryFindLastAtBeat(cursorBeat);
		Gui::BeginDisabled(cursorBeat.Ticks < 0);

		static constexpr auto guiDoubleButton = [](cstr labelA, cstr labelB) -> i32
		{
			return Gui::SameLineMultiWidget(2, [&](const Gui::MultiWidgetIt& i) { return Gui::Button((i.Index == 0) ? labelA : labelB, { Gui::CalcItemWidth(), 0.0f }); }).ChangedIndex;
		};

		// TODO: Handle inside the tja format code instead (?)
		LyricInputBuffer.clear();
		if (lyricChangeAtCursor != nullptr)
			ResolveEscapeSequences(lyricChangeAtCursor->Lyric, LyricInputBuffer, EscapeSequenceFlags::NewLines);

		const f32 textInputHeight = ClampBot(Gui::GetContentRegionAvail().y - Gui::GetFrameHeightWithSpacing(), Gui::GetFrameHeightWithSpacing());
		Gui::SetNextItemWidth(-1.0f);
		if (Gui::InputTextMultilineWithHint("##LyricAtCursor", "n/a", &LyricInputBuffer, { -1.0f, textInputHeight }, ImGuiInputTextFlags_None))
		{
			std::string newLyricLine;
			ConvertToEscapeSequences(LyricInputBuffer, newLyricLine, EscapeSequenceFlags::NewLines);

			if (lyricChangeAtCursor == nullptr || lyricChangeAtCursor->BeatTime != cursorBeat)
				context.Undo.Execute<Commands::AddLyricChange>(&course.Lyrics, LyricChange { cursorBeat, std::move(newLyricLine) });
			else
				context.Undo.Execute<Commands::UpdateLyricChange>(&course.Lyrics, LyricChange { cursorBeat, std::move(newLyricLine) });
		}

		Gui::SetNextItemWidth(-1.0f);
		if (i32 clicked = guiDoubleButton("Clear##LyricAtCursor", "Remove##LyricAtCursor"); clicked > -1)
		{
			if (clicked == 0)
			{
				if (lyricChangeAtCursor == nullptr || lyricChangeAtCursor->BeatTime != cursorBeat)
					context.Undo.Execute<Commands::AddLyricChange>(&course.Lyrics, LyricChange { cursorBeat, "" });
				else
					context.Undo.Execute<Commands::UpdateLyricChange>(&course.Lyrics, LyricChange { cursorBeat, "" });
			}
			else if (clicked == 1)
			{
				if (lyricChangeAtCursor != nullptr && lyricChangeAtCursor->BeatTime == cursorBeat)
					context.Undo.Execute<Commands::RemoveLyricChange>(&course.Lyrics, cursorBeat);
			}
		}

		Gui::EndDisabled();
	}
}
