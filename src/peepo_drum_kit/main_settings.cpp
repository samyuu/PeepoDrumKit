#include "main_settings.h"
#include "core_string.h"
#include "core_io.h"
#include <algorithm>

namespace PeepoDrumKit
{
	void RecentFilesList::Add(std::string newFilePath, b8 addToEnd)
	{
		Path::NormalizeInPlace(newFilePath);
		erase_remove_if(SortedPaths, [&](const auto& path) { return ASCII::MatchesInsensitive(path, newFilePath); });
		SortedPaths.reserve(MaxCount);
		if (addToEnd)
			SortedPaths.push_back(std::move(newFilePath));
		else
			SortedPaths.emplace(SortedPaths.begin(), std::move(newFilePath));
		if (SortedPaths.size() > MaxCount)
			SortedPaths.resize(MaxCount);
	}

	void RecentFilesList::Remove(std::string filePathToRemove)
	{
		Path::NormalizeInPlace(filePathToRemove);
		erase_remove_if(SortedPaths, [&](const auto& path) { return ASCII::MatchesInsensitive(path, filePathToRemove); });
		if (SortedPaths.size() > MaxCount)
			SortedPaths.resize(MaxCount);
	}

	void GuiStyleColorPeepoDrumKit(ImGuiStyle* destination)
	{
		ImVec4* colors = (destination != nullptr) ? destination->Colors : Gui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		colors[ImGuiCol_Border] = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.21f, 0.21f, 0.21f, 0.54f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.27f, 0.27f, 0.27f, 0.64f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 0.54f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.12f, 0.12f, 0.12f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.37f, 0.56f, 0.18f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.52f, 0.18f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.33f, 0.49f, 0.17f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.45f, 0.45f, 0.45f, 0.40f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.59f, 0.59f, 0.59f, 0.40f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.36f, 0.36f, 0.36f, 0.40f);
		colors[ImGuiCol_Header] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.23f, 0.23f, 0.23f, 1.00f);
		colors[ImGuiCol_Separator] = ImVec4(0.49f, 0.49f, 0.49f, 0.50f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.32f, 0.49f, 0.21f, 0.78f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.39f, 0.67f, 0.18f, 0.78f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.44f, 0.44f, 0.44f, 0.20f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.39f, 0.60f, 0.24f, 0.20f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.43f, 0.65f, 0.27f, 0.20f);
		colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.22f, 0.25f, 0.18f, 1.00f);
		colors[ImGuiCol_TabActive] = ImVec4(0.30f, 0.41f, 0.16f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
		colors[ImGuiCol_DockingPreview] = ImVec4(0.52f, 0.68f, 0.46f, 0.70f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.34f, 0.45f, 0.20f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.35f, 0.49f, 0.18f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.34f, 0.45f, 0.20f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.31f, 0.78f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.23f, 0.78f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.76f, 0.76f, 0.76f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(0.50f, 0.75f, 0.21f, 1.00f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.50f, 0.75f, 0.21f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.50f);
	}

	static cstr BoolToString(b8 in)
	{
		return in ? "true" : "false";
	}

	static b8 BoolFromString(std::string_view in, b8& out)
	{
		if (in == "true") { out = true; return true; }
		if (in == "false") { out = false; return true; }
		if (i32 v = 0; ASCII::TryParseI32(in, v)) { out = (v != 0); return true; }
		return false;
	}

	static i32 RectToTLSizeString(char* buffer, size_t bufferSize, const Rect& in)
	{
		return sprintf_s(buffer, bufferSize, "%g, %g, %g, %g", in.TL.x, in.TL.y, in.GetWidth(), in.GetHeight());
	}

	static b8 RectFromTLSizeString(std::string_view in, Rect& out)
	{
		b8 outSuccess = true; vec2 outTL {}, outSize {};
		ASCII::ForEachInCommaSeparatedList(in, [&, index = 0](std::string_view value) mutable
		{
			value = ASCII::Trim(value);
			if (index == 0) { outSuccess &= ASCII::TryParseF32(value, outTL.x); }
			if (index == 1) { outSuccess &= ASCII::TryParseF32(value, outTL.y); }
			if (index == 2) { outSuccess &= ASCII::TryParseF32(value, outSize.x); }
			if (index == 3) { outSuccess &= ASCII::TryParseF32(value, outSize.y); }
			index++;
		});
		out = Rect::FromTLSize(outTL, outSize); return outSuccess;
	}

	namespace Ini
	{
		constexpr IniMemberParseResult MemberParseError(cstr errorMessage) { return IniMemberParseResult { true, errorMessage }; }

		static IniMemberParseResult FromString(std::string_view stringToParse, i32& out)
		{
			return ASCII::TryParseI32(stringToParse, out) ? IniMemberParseResult {} : MemberParseError("Invalid int");
		}

		static void ToString(const i32& in, std::string& stringToAppendTo)
		{
			char b[32]; stringToAppendTo += std::string_view(b, sprintf_s(b, "%d", in));
		}

		static IniMemberParseResult FromString(std::string_view stringToParse, f32& out)
		{
			return ASCII::TryParseF32(stringToParse, out) ? IniMemberParseResult {} : MemberParseError("Invalid float");
		}

		static void ToString(const f32& in, std::string& stringToAppendTo)
		{
			char b[64]; stringToAppendTo += std::string_view(b, sprintf_s(b, "%g", in));
		}

		static IniMemberParseResult FromString(std::string_view stringToParse, b8& out)
		{
			return BoolFromString(stringToParse, out) ? IniMemberParseResult {} : MemberParseError("Invalid bool");
		}

		static void ToString(const b8& in, std::string& stringToAppendTo)
		{
			stringToAppendTo += BoolToString(in);
		}

		static void ToString(const std::string& in, std::string& stringToAppendTo)
		{
			stringToAppendTo += in;
		}

		static IniMemberParseResult FromString(std::string_view stringToParse, std::string& out)
		{
			out = stringToParse; return {};
		}

		static IniMemberParseResult FromString(std::string_view stringToParse, Beat& out)
		{
			return ASCII::TryParseI32(stringToParse, out.Ticks) ? IniMemberParseResult {} : MemberParseError("Invalid int");
		}

		static void ToString(const Beat& in, std::string& stringToAppendTo)
		{
			char b[32]; stringToAppendTo += std::string_view(b, sprintf_s(b, "%d", in.Ticks));
		}

		static IniMemberParseResult FromString(std::string_view stringToParse, MultiInputBinding& out)
		{
			return InputBindingFromStorageString(stringToParse, out) ? IniMemberParseResult {} : MemberParseError("Invalid input binding");
		}

		static void ToString(const MultiInputBinding& in, std::string& stringToAppendTo)
		{
			InputBindingToStorageString(in, stringToAppendTo);
		}

		// NOTE: Rely entirely on overload resolution for these
		template<typename T> static IniMemberParseResult VoidPtrToTypeWrapper(std::string_view stringToParse, IniMemberVoidPtr out)
		{
			assert(out.Address != nullptr && out.ByteSize == sizeof(T));
			return FromString(stringToParse, *static_cast<T*>(out.Address));
		}
		template<typename T> static void VoidPtrToTypeWrapper(IniMemberVoidPtr in, std::string& stringToAppendTo)
		{
			assert(in.Address != nullptr && in.ByteSize == sizeof(T));
			ToString(*static_cast<const T*>(in.Address), stringToAppendTo);
		}

		struct IniParser
		{
			struct SectionIt { std::string_view Line, Section; };
			struct KeyValueIt { std::string_view Line, Key, Value; };

			SettingsParseResult Result = {};
			i32 LineIndex = 0;
			std::string_view CurrentSection = "";

			inline void Error(cstr errorMessage) { Result.HasError = true; Result.ErrorLineIndex = LineIndex; Result.ErrorMessage = errorMessage; }
			inline void Error_InvalidInt() { Error("Invalid int"); }
			inline void Error_InvalidFloat() { Error("Invalid float"); }
			inline void Error_InvalidRect() { Error("Invalid rect"); }
			inline void Error_InvalidBool() { Error("Invalid bool"); }

			template <typename SectionFunc, typename KeyValueFunc>
			inline void ForEachIniKeyValueLine(std::string_view fileContent, SectionFunc sectionFunc, KeyValueFunc keyValueFunc)
			{
				ASCII::ForEachLineInMultiLineString(fileContent, false, [&](std::string_view line)
				{
					defer { LineIndex++; };
					if (Result.HasError)
						return;

					line = ASCII::Trim(line);
					if (ASCII::IsAllWhitespace(line) || line[0] == ';' || line[0] == '#')
						return;

					const size_t commentIndex = line.find_first_of(';');
					if (commentIndex != std::string_view::npos)
						line = ASCII::TrimRight(line.substr(0, commentIndex));

					if (line[0] == '[')
					{
						const size_t clsoingIndex = line.find_first_of(']');
						if (clsoingIndex == std::string_view::npos)
							return Error("Missing closing ']' for section");

						if (clsoingIndex != line.size() - 1)
							return Error("Unexpected trailing data");

						const std::string_view section = ASCII::Trim(line.substr(sizeof('['), line.size() - (sizeof('[') + sizeof(']'))));

						CurrentSection = section;
						sectionFunc(SectionIt { line, section });
					}
					else
					{
						const size_t separatorIndex = line.find_first_of('=');
						if (separatorIndex == std::string_view::npos)
							return Error("Missing '=' separator");

						const std::string_view key = ASCII::Trim(line.substr(0, separatorIndex));
						const std::string_view value = ASCII::Trim(line.substr(separatorIndex + 1));

						if (key.empty())
							return Error("Empty key");

						keyValueFunc(KeyValueIt { line, key, value });
					}
				});
			}
		};

		struct IniWriter
		{
			std::string& Out;
			char Buffer[4096];

			inline void Line() { Out += '\n'; }
			inline void Line(std::string_view line) { Out += line; Out += '\n'; }
			inline void LineSection(std::string_view section) { Out += '['; Out += section; Out += "]\n"; }
			inline void LineKeyValue_Str(std::string_view key, std::string_view value) { Out += key; Out += " = "; Out += value; Out += '\n'; }
			inline void LineKeyValue_I32(std::string_view key, i32 value) { LineKeyValue_Str(key, std::string_view(Buffer, sprintf_s(Buffer, "%d", value))); }
			inline void LineKeyValue_F32(std::string_view key, f32 value) { LineKeyValue_Str(key, std::string_view(Buffer, sprintf_s(Buffer, "%g", value))); }
			inline void LineKeyValue_F64(std::string_view key, f64 value) { LineKeyValue_Str(key, std::string_view(Buffer, sprintf_s(Buffer, "%g", value))); }

			inline void Comment() { Out += "; "; }
			inline void LineComment(std::string_view comment) { Comment(); Out += comment; Out += '\n'; }
		};
	}

	constexpr size_t SizeOfPersistentAppData = sizeof(PersistentAppData);
	static_assert(PEEPO_RELEASE || SizeOfPersistentAppData == 88, "TODO: Add missing ini file handling for newly added PersistentAppData fields");

	SettingsParseResult ParseSettingsIni(std::string_view fileContent, PersistentAppData& out)
	{
		Ini::IniParser parser = {};

		parser.ForEachIniKeyValueLine(fileContent,
			[&](const Ini::IniParser::SectionIt& it)
		{
			return;
		}, [&](const Ini::IniParser::KeyValueIt& it)
		{
			const std::string_view in = it.Value;
			if (parser.CurrentSection == "last_session")
			{
				if (it.Key == "")
				{
					if (in == "file_version")
					{
						// TODO: ...
					}
				}
				else if (it.Key == "gui_scale")
				{
					if (f32 v = out.LastSession.GuiScale; ASCII::TryParseF32(in, v)) out.LastSession.GuiScale = FromPercent(v); else return parser.Error_InvalidFloat();
				}
				else if (it.Key == "window_swap_interval")
				{
					if (!ASCII::TryParseI32(in, out.LastSession.WindowSwapInterval)) return parser.Error_InvalidInt();
				}
				else if (it.Key == "window_region")
				{
					if (!RectFromTLSizeString(in, out.LastSession.WindowRegion)) return parser.Error_InvalidRect();
				}
				else if (it.Key == "window_region_restore")
				{
					if (!RectFromTLSizeString(in, out.LastSession.WindowRegionRestore)) return parser.Error_InvalidRect();
				}
				else if (it.Key == "window_is_fullscreen")
				{
					if (!BoolFromString(in, out.LastSession.WindowIsFullscreen)) return parser.Error_InvalidBool();
				}
				else if (it.Key == "window_is_maximized")
				{
					if (!BoolFromString(in, out.LastSession.WindowIsMaximized)) return parser.Error_InvalidBool();
				}
			}
			else if (parser.CurrentSection == "recent_files")
			{
				const std::string_view indexSuffix = ASCII::TrimPrefix(it.Key, "file_");
				if (i32 v = 0; ASCII::TryParseI32(indexSuffix, v))
					out.RecentFiles.Add(std::string { in }, true);
				else
					return parser.Error("Invalid file index");
			}
		});

		return parser.Result;
	}

	void SettingsToIni(const PersistentAppData& in, std::string& out)
	{
		Ini::IniWriter writer { out };
		char b[512], keyBuffer[64];

		writer.LineKeyValue_Str("file_version", std::string_view(b, sprintf_s(b, "%d.%d.%d", 1, 0, 0)));
		writer.Line();

		writer.LineSection("last_session");
		writer.LineKeyValue_F32("gui_scale", ToPercent(in.LastSession.GuiScale));
		writer.LineKeyValue_I32("window_swap_interval", in.LastSession.WindowSwapInterval);
		writer.LineKeyValue_Str("window_region", std::string_view(b, RectToTLSizeString(b, sizeof(b), in.LastSession.WindowRegion)));
		writer.LineKeyValue_Str("window_region_restore", std::string_view(b, RectToTLSizeString(b, sizeof(b), in.LastSession.WindowRegionRestore)));
		writer.LineKeyValue_Str("window_is_fullscreen", BoolToString(in.LastSession.WindowIsFullscreen));
		writer.LineKeyValue_Str("window_is_maximized", BoolToString(in.LastSession.WindowIsMaximized));
		writer.Line();

		writer.LineSection("recent_files");
		if (!in.RecentFiles.SortedPaths.empty())
			for (size_t i = 0; i < in.RecentFiles.SortedPaths.size(); i++)
				writer.LineKeyValue_Str(std::string_view(keyBuffer, sprintf_s(keyBuffer, "file_%02zu", i)), in.RecentFiles.SortedPaths[i]);
		else
			writer.LineComment("...");
	}

	SettingsParseResult ParseSettingsIni(std::string_view fileContent, UserSettingsData& out)
	{
		Ini::IniParser parser = {};

		parser.ForEachIniKeyValueLine(fileContent,
			[&](const Ini::IniParser::SectionIt& it)
		{
			return;
		}, [&](const Ini::IniParser::KeyValueIt& it)
		{
			if (parser.CurrentSection == "" && it.Value == "file_version")
			{
				// TODO: ...
			}

			for (size_t i = 0; i < AppSettingsReflectionMap.MemberCount; i++)
			{
				const auto& member = AppSettingsReflectionMap.MemberSlots[i];
				if (parser.CurrentSection == member.SerializedSection && it.Key == member.SerializedName)
				{
					const IniMemberParseResult memberParseResult = member.FromStringFunc(it.Value, { reinterpret_cast<u8*>(&out) + member.ByteOffsetValue, member.ByteSizeValue });
					if (!memberParseResult.HasError)
						*(reinterpret_cast<b8*>(reinterpret_cast<u8*>(&out) + member.ByteOffsetHasValueB8)) = true;
					else
						return parser.Error(memberParseResult.ErrorMessage);
					break;
				}
			}
		});

		out.General.DrumrollAutoHitBarDivision.Value = Clamp(out.General.DrumrollAutoHitBarDivision.Value, 1, Beat::TicksPerBeat);

		return parser.Result;
	}

	void SettingsToIni(const UserSettingsData& in, std::string& out)
	{
		Ini::IniWriter writer { out };
		char b[512];
		std::string strBuffer; strBuffer.reserve(256);

		writer.LineKeyValue_Str("file_version", std::string_view(b, sprintf_s(b, "%d.%d.%d", 1, 0, 0)));
		cstr lastSection = nullptr;

		for (size_t i = 0; i < AppSettingsReflectionMap.MemberCount; i++)
		{
			const auto& member = AppSettingsReflectionMap.MemberSlots[i];
			if (member.SerializedSection != lastSection) { writer.Line(); writer.LineSection(member.SerializedSection); lastSection = member.SerializedSection; }

			if (!*(reinterpret_cast<const b8*>(reinterpret_cast<const u8*>(&in) + member.ByteOffsetHasValueB8)))
			{
				writer.Comment();
				member.ToStringFunc({ const_cast<u8*>(reinterpret_cast<const u8*>(&in)) + member.ByteOffsetDefault, member.ByteSizeValue }, strBuffer);
			}
			else
			{
				member.ToStringFunc({ const_cast<u8*>(reinterpret_cast<const u8*>(&in)) + member.ByteOffsetValue, member.ByteSizeValue }, strBuffer);
			}

			writer.LineKeyValue_Str(member.SerializedName, strBuffer);
			strBuffer.clear();
		}
	}

	constexpr size_t SizeOfUserSettingsData = sizeof(UserSettingsData);
	static_assert(PEEPO_RELEASE || SizeOfUserSettingsData == 14968, "TODO: Add missing reflection entries for newly added UserSettingsData fields");

	SettingsReflectionMap StaticallyInitializeAppSettingsReflectionMap()
	{
		// TODO: Static assert size to not forget about adding new members
		SettingsReflectionMap outMap {};
		cstr currentSection = "";

#define SECTION(serializedSection) do { currentSection = serializedSection; } while (false)
#define X(member, serializedName) do																			\
		{																										\
			SettingsReflectionMember& out = outMap.MemberSlots[outMap.MemberCount++];							\
			out.ByteSizeValue = static_cast<u16>(sizeof(UserSettingsData::member.Value));						\
			out.ByteOffsetDefault = static_cast<u16>(offsetof(UserSettingsData, member.Default));				\
			out.ByteOffsetValue = static_cast<u16>(offsetof(UserSettingsData, member.Value));					\
			out.ByteOffsetHasValueB8 = static_cast<u16>(offsetof(UserSettingsData, member.HasValue));			\
			out.SourceCodeName = #member;																		\
			out.SerializedSection = currentSection;																\
			out.SerializedName = serializedName;																\
			out.FromStringFunc = Ini::VoidPtrToTypeWrapper<decltype(UserSettingsData::member)::UnderlyingType>;	\
			out.ToStringFunc = Ini::VoidPtrToTypeWrapper<decltype(UserSettingsData::member)::UnderlyingType>;	\
		} while (false)
		{
			SECTION("general");
			X(General.DefaultCreatorName, "default_creator_name");
			X(General.DrumrollAutoHitBarDivision, "drumroll_auto_hit_bar_division");
			X(General.TimelineScrubAutoScrollPixelThreshold, "timeline_scrub_auto_scroll_pixel_threshold");
			X(General.TimelineScrubAutoScrollSpeedMin, "timeline_scrub_auto_scroll_speed_min");
			X(General.TimelineScrubAutoScrollSpeedMax, "timeline_scrub_auto_scroll_speed_max");

			SECTION("audio");
			X(Audio.OpenDeviceOnStartup, "open_device_on_startup");
			X(Audio.CloseDeviceOnIdleFocusLoss, "close_device_on_idle_focus_loss");
			X(Audio.RequestExclusiveDeviceAccess, "request_exclusive_device_access");

			SECTION("animation");
			X(Animation.TimelineSmoothScrollSpeed, "timeline_smooth_scroll_speed");
			X(Animation.TimelineWaveformFadeSpeed, "timeline_waveform_fade_speed");
			X(Animation.TimelineRangeSelectionExpansionSpeed, "timeline_range_selection_expansion_speed");
			X(Animation.TimelineWorldSpaceCursorXSpeed, "timeline_world_space_cursor_x_speed");
			X(Animation.TimelineGridSnapLineSpeed, "timeline_grid_snap_line_speed");
			X(Animation.TimelineGoGoRangeExpansionSpeed, "timeline_gogo_range_expansion_speed");

			SECTION("input");
			X(Input.Dialog_YesOrOk, "dialog_yes_or_ok");
			X(Input.Dialog_No, "dialog_no");
			X(Input.Dialog_Cancel, "dialog_cancel");
			X(Input.Dialog_SelectNextTab, "dialog_select_next_tab");
			X(Input.Dialog_SelectPreviousTab, "dialog_select_previous_tab");
			X(Input.Editor_ToggleFullscreen, "editor_toggle_fullscreen");
			X(Input.Editor_ToggleVSync, "editor_toggle_vsync");
			X(Input.Editor_IncreaseGuiScale, "editor_increase_gui_scale");
			X(Input.Editor_DecreaseGuiScale, "editor_decrease_gui_scale");
			X(Input.Editor_ResetGuiScale, "editor_reset_gui_scale");
			X(Input.Editor_Undo, "editor_undo");
			X(Input.Editor_Redo, "editor_redo");
			X(Input.Editor_OpenSettings, "editor_open_settings");
			X(Input.Editor_ChartNew, "editor_chart_new");
			X(Input.Editor_ChartOpen, "editor_chart_open");
			X(Input.Editor_ChartOpenDirectory, "editor_chart_open_directory");
			X(Input.Editor_ChartSave, "editor_chart_save");
			X(Input.Editor_ChartSaveAs, "editor_chart_save_as");
			X(Input.Timeline_PlaceNoteDon, "timeline_place_note_don");
			X(Input.Timeline_PlaceNoteKa, "timeline_place_note_ka");
			X(Input.Timeline_PlaceNoteBalloon, "timeline_place_note_balloon");
			X(Input.Timeline_PlaceNoteDrumroll, "timeline_place_note_drumroll");
			X(Input.Timeline_FlipNoteType, "timeline_flip_note_type");
			X(Input.Timeline_ToggleNoteSize, "timeline_toggle_note_size");
			X(Input.Timeline_Cut, "timeline_cut");
			X(Input.Timeline_Copy, "timeline_copy");
			X(Input.Timeline_Paste, "timeline_paste");
			X(Input.Timeline_DeleteSelection, "timeline_delete_selection");
			X(Input.Timeline_StartEndRangeSelection, "timeline_start_end_range_selection");
			X(Input.Timeline_StepCursorLeft, "timeline_step_cursor_left");
			X(Input.Timeline_StepCursorRight, "timeline_step_cursor_right");
			X(Input.Timeline_JumpToTimelineStart, "timeline_jump_to_timeline_start");
			X(Input.Timeline_JumpToTimelineEnd, "timeline_jump_to_timeline_end");
			X(Input.Timeline_IncreaseGridDivision, "timeline_increase_grid_division");
			X(Input.Timeline_DecreaseGridDivision, "timeline_decrease_grid_division");
			X(Input.Timeline_SetGridDivision_1_4, "timeline_set_grid_division_1_4");
			X(Input.Timeline_SetGridDivision_1_8, "timeline_set_grid_division_1_8");
			X(Input.Timeline_SetGridDivision_1_12, "timeline_set_grid_division_1_12");
			X(Input.Timeline_SetGridDivision_1_16, "timeline_set_grid_division_1_16");
			X(Input.Timeline_SetGridDivision_1_24, "timeline_set_grid_division_1_24");
			X(Input.Timeline_SetGridDivision_1_32, "timeline_set_grid_division_1_32");
			X(Input.Timeline_SetGridDivision_1_48, "timeline_set_grid_division_1_48");
			X(Input.Timeline_SetGridDivision_1_64, "timeline_set_grid_division_1_64");
			X(Input.Timeline_SetGridDivision_1_96, "timeline_set_grid_division_1_96");
			X(Input.Timeline_SetGridDivision_1_192, "timeline_set_grid_division_1_192");
			X(Input.Timeline_IncreasePlaybackSpeed, "timeline_increase_playback_speed");
			X(Input.Timeline_DecreasePlaybackSpeed, "timeline_decrease_playback_speed");
			X(Input.Timeline_SetPlaybackSpeed_100, "timeline_set_playback_speed_100");
			X(Input.Timeline_SetPlaybackSpeed_75, "timeline_set_playback_speed_75");
			X(Input.Timeline_SetPlaybackSpeed_50, "timeline_set_playback_speed_50");
			X(Input.Timeline_SetPlaybackSpeed_25, "timeline_set_playback_speed_25");
			X(Input.Timeline_TogglePlayback, "timeline_toggle_playback");
			X(Input.Timeline_ToggleMetronome, "timeline_toggle_metronome");
			X(Input.TempoCalculator_Tap, "tempo_calculator_tap");
			X(Input.TempoCalculator_Reset, "tempo_calculator_reset");
		}
#undef X
#undef SECTION
		assert(outMap.MemberCount < ArrayCount(outMap.MemberSlots));
		return outMap;
	}
}
