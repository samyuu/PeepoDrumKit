#pragma once
#include "core_types.h"
#include "core_io.h"
#include "tja_file_format.h"
#include "imgui/imgui_include.h"
#include "imgui/3rdparty_extension/ImGuiColorTextEdit/TextEditor.h"

#include <stdio.h>
#include <vector>
#include <future>

namespace PeepoDrumKit
{
	namespace TJADrawUtil
	{
		static constexpr u32 RedColor = 0xFF443AF9;
		static constexpr u32 BlueColor = 0xFFFEC955;
		static constexpr u32 OrangeColor = 0xFF47BEFB;
		static constexpr u32 YellowColor = 0xFF48EFFD;
		static constexpr u32 WhiteColor = 0xFFFFFFFF;
		static constexpr u32 BlackColor = 0xFF000000;
		static constexpr u32 GoGoBackground = 0x448787FF;
		static constexpr f32 SmallRadiusFactor = 0.28125f;
		static constexpr f32 BigRadiusFactor = 0.375f;

		static constexpr u32 NoteTypeToColor[] =
		{
			BlackColor,		// Empty
			RedColor,		// Don
			BlueColor,		// Ka
			RedColor,		// DonBig
			BlueColor,		// KaBig
			YellowColor,	// Start_Drumroll
			YellowColor,	// Start_DrumrollBig
			OrangeColor,	// Start_Balloon
			YellowColor,	// End_BalloonOrDrumroll
			OrangeColor,	// Start_SpecialBaloon
			RedColor,		// DonBigBoth
			BlueColor,		// KaBigBoth
			BlackColor,		// Hidden
		};
		static_assert(ArrayCount(NoteTypeToColor) == EnumCount<TJA::NoteType>);

		inline void DrawNote(ImDrawList* drawList, vec2 center, f32 rowHeight, TJA::NoteType noteType)
		{
			const bool isBigNote = TJA::IsBigNote(noteType);
			const f32 radius = rowHeight * (isBigNote ? BigRadiusFactor : SmallRadiusFactor);
			drawList->AddCircleFilled(center, radius, BlackColor);
			drawList->AddCircleFilled(center, radius - 2.0f, WhiteColor);
			drawList->AddCircleFilled(center, radius - (isBigNote ? 5.0f : 4.0f), NoteTypeToColor[EnumToIndex(noteType)]);
		}
	}

	::TextEditor CreateImGuiColorTextEditWithNiceTheme();

	struct ParsedAndConvertedTJAFile
	{
		std::string FilePath;
		File::UniqueFileContent FileContentBytes;
		std::string FileContentUTF8;

		std::vector<std::string_view> Lines;
		std::vector<TJA::Token> Tokens;
		TJA::ErrorList ParseErrors;
		TJA::ParsedTJA Parsed;

		std::vector<TJA::ConvertedCourse> ConvertedCourses;

		inline bool LoadFromFile(std::string_view filePath)
		{
			FilePath = filePath;
			FileContentBytes = File::ReadAllBytes(filePath);

			const std::string_view fileContentView = std::string_view(reinterpret_cast<const char*>(FileContentBytes.Content.get()), FileContentBytes.Size);
			if (UTF8::HasBOM(fileContentView))
				FileContentUTF8 = UTF8::TrimBOM(fileContentView);
			else
				FileContentUTF8 = UTF8::FromShiftJIS(fileContentView);

			DebugReloadFromModifiedFileContentUTF8();
			return (FileContentBytes.Content != nullptr);
		}

		inline bool DebugReloadFromModifiedFileContentUTF8()
		{
			Lines = TJA::SplitLines(FileContentUTF8);
			Tokens = TJA::TokenizeLines(Lines);
			ParseErrors.Clear();
			Parsed = ParseTokens(Tokens, ParseErrors);

			ConvertedCourses.clear();
			ConvertedCourses.reserve(Parsed.Courses.size());
			for (const auto& course : Parsed.Courses)
				ConvertedCourses.push_back(ConvertParsedToConvertedCourse(Parsed, course));

			return true;
		}
	};

	struct TJATestWindows
	{
		//static constexpr std::string_view TestTJAFilePath = u8"test/Songs/Namco Original/åéâeSASURAI/åéâeSASURAI.tja";
		//static constexpr std::string_view TestTJAFilePath = u8"test/Songs/Namco Original/ÉRÉiÉÇÉmÅô/ÉRÉiÉÇÉmÅô.tja";
		static constexpr std::string_view TestTJAFilePath = u8"";

		std::future<ParsedAndConvertedTJAFile> LoadTJAFuture = std::async(std::launch::async, []() { ParsedAndConvertedTJAFile tja; tja.LoadFromFile(TestTJAFilePath); return tja; });
		ParsedAndConvertedTJAFile LoadedTJAFile;

		::TextEditor TJATextEditor = CreateImGuiColorTextEditWithNiceTheme();
		bool WasTJAEditedThisFrame = false;

	public:
		void DrawGui(bool* isOpen);

	private:
		void DrawGuiFileContentWindow(bool* isOpen);
		void DrawGuiTokensWindow(bool* isOpen);
		void DrawGuiParsedWindow(bool* isOpen);
		void DrawGuiConvertedWindow(bool* isOpen);
	};
}
