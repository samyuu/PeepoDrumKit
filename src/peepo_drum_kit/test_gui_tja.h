#pragma once
#include "core_types.h"
#include "core_io.h"
#include "file_format_tja.h"
#include "imgui/imgui_include.h"
#include "imgui/3rdparty_extension/ImGuiColorTextEdit/TextEditor.h"

#include <stdio.h>
#include <vector>
#include <future>

namespace PeepoDrumKit
{
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

		inline b8 LoadFromFile(std::string_view filePath)
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

		inline b8 DebugReloadFromModifiedFileContentUTF8()
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

	struct TJATestWindow
	{
		std::future<ParsedAndConvertedTJAFile> LoadTJAFuture = {};
		ParsedAndConvertedTJAFile LoadedTJAFile;

		::TextEditor TJATextEditor = CreateImGuiColorTextEditWithNiceTheme();
		
		b8 IsFirstFrame = true;
		b8 WasTJAEditedThisFrame = false;
		i32 TabIndexToSelectThisFrame = -1;

	public:
		void DrawGui();

	private:
		void DrawGuiFileContentTabContent();
		void DrawGuiTokensTabContent();
		void DrawGuiParsedTabContent();
	};
}
