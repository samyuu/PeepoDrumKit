// NOTE: This is a meta-program that does not get included in the actual build of PeepoDrumKit
//		 used for the generation of the embedded icons header file source code

#include "core_types.h"
#include "core_io.h"
#include "core_string.h"
#include "imgui/3rdparty/imgui.h"
#include "imgui/3rdparty/imgui_internal.h"
#include <stb/stb_image.h>
#include <filesystem>

namespace CodeGen
{
	// NOTE: https://en.wikipedia.org/wiki/Private_Use_Areas
	static constexpr ImWchar UnicodePrivateUseArea = 0xE000;
	static constexpr ImWchar UnicodePrivateUseAreaEnd = 0xF8FF;

	static std::string GenerateIconSourceCode(std::string_view inputDirectoryWithImagesInside /* = "src_res_icons/" */)
	{
		struct LoadedImage { i32 Width, Height; u32* RGBA; ImWchar Codepoint; char UTF8[5]; std::string FilePath, Name; i32 OffsetIntoPixelData; };
		std::vector<LoadedImage> loadedImages;

		assert(std::filesystem::exists(inputDirectoryWithImagesInside));
		u32 lastOffsetIntoPixelData = 0;
		for (const auto& path : std::filesystem::directory_iterator(inputDirectoryWithImagesInside))
		{
			const std::string filePath = path.path().u8string();
			const std::string_view name = Path::GetFileName(filePath, false);
			if (!Path::HasAnyExtension(filePath, ".png"))
				continue;

			LoadedImage& loadedImage = loadedImages.emplace_back();
			loadedImage.RGBA = reinterpret_cast<u32*>(::stbi_load(filePath.c_str(), &loadedImage.Width, &loadedImage.Height, nullptr, 4));
			loadedImage.Codepoint = UnicodePrivateUseArea + static_cast<i32>(loadedImages.size()) - 1;
			ImTextCharToUtf8(loadedImage.UTF8, loadedImage.Codepoint);
			loadedImage.FilePath = filePath;
			loadedImage.Name = name;
			loadedImage.OffsetIntoPixelData = lastOffsetIntoPixelData;
			assert(loadedImage.RGBA != nullptr);
			lastOffsetIntoPixelData += (loadedImage.Width * loadedImage.Height);
		}

		struct SourceCodeBuffer
		{
			i32 Indentation = 0; std::string Data;
			inline SourceCodeBuffer() { Data.reserve(0x4000); }
			inline void Line() { Data += '\n'; }
			inline void Line(std::string_view line) { Data.append(Indentation, '\t').append(line).append("\n"); }
			inline void LineComment() { Data.append(Indentation, '\t').append("//").append("\n"); }
			inline void LineComment(std::string_view line) { Data.append(Indentation, '\t').append("// ").append(line).append("\n"); }
			inline void BeginLine() { Data.append(Indentation, '\t'); }
			inline void SameLine(std::string_view sameLine) { Data += sameLine; }
			inline void EndLine() { Data += '\n'; }
			inline void PushIndent() { Indentation++; }
			inline void PopIndent() { Indentation--; }
		};

		i32 totalPixelCount = 0;
		for (const LoadedImage& it : loadedImages)
			totalPixelCount += (it.Width * it.Height);

		char b[0x2000];
		SourceCodeBuffer out;
		out.LineComment("NOTE: THIS FILE WAS GENERATED AUTOMATICALLY. DO NOT EDIT!");
		out.Line();
		out.LineComment("NOTE: Paste these into a public header to use inside strings");
		out.Line("/*");
		for (const LoadedImage& it : loadedImages)
		{
			assert(strlen(it.UTF8) == 3);
			out.Line(std::string_view(b, sprintf_s(b,
				"#define UTF8_%s \"\\x%02X\\x%02X\\x%02X\"",
				it.Name.c_str(),
				static_cast<u8>(it.UTF8[0]),
				static_cast<u8>(it.UTF8[1]),
				static_cast<u8>(it.UTF8[2]))));
		}
		out.Line("*/");
		out.Line();
		out.Line("struct EmbeddedIcon { cstr Name; ImWchar Codepoint; ivec2 Size; const u32 OffsetIntoPixelData; };");
		out.Line("static constexpr EmbeddedIcon EmbeddedIcons[] =");
		out.Line("{"); out.PushIndent();
		for (const LoadedImage& it : loadedImages)
		{
			out.Line(std::string_view(b, sprintf_s(b,
				"{ u8\"%s\", static_cast<ImWchar>(0x%04X), { %d, %d }, 0x%04X },",
				it.Name.c_str(),
				static_cast<u32>(it.Codepoint),
				it.Width, it.Height,
				it.OffsetIntoPixelData)));
		}
		out.PopIndent(); out.Line("};");
		out.Line();
		out.Line(std::string_view(b, sprintf_s(b, "static constexpr u32 EmbeddedIconsPixelData[0x%04X] = ", totalPixelCount)));
		out.Line("{"); out.PushIndent();
		{
			for (const LoadedImage& it : loadedImages)
			{
				out.LineComment("--------------------------------------------");
				out.LineComment(std::string_view(b, sprintf_s(b, "NOTE: %s", it.Name.c_str())));
				out.LineComment("--------------------------------------------");
				out.BeginLine();
				for (i32 i = 0; i < (it.Width * it.Height); i++)
				{
					if (i > 0 && (i % 32) == 0) { out.EndLine(); out.BeginLine(); }
					out.SameLine(std::string_view(b, sprintf_s(b, "0x%08X, ", it.RGBA[i])));
				}
				out.EndLine();
				out.LineComment("--------------------------------------------");

				if (&it != &loadedImages.back())
					out.Line();
			}
		}
		out.PopIndent(); out.Line("};");

		return std::move(out.Data);
	}

	static void WriteSourceCodeToFile(std::string_view sourceCode, std::string_view filePath /* = "src_res_icons/x_embedded_icons_include.h" */)
	{
		File::WriteAllBytes(filePath, sourceCode);
	}
}
