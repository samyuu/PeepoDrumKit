#include "core_string.h"
#include <charconv>
#include <Windows.h>

static std::string Win32NarrowStdStringWithCodePage(std::wstring_view input, UINT win32CodePage)
{
	std::string output;
	const int outputLength = ::WideCharToMultiByte(win32CodePage, 0, input.data(), static_cast<int>(input.size() + 1), nullptr, 0, nullptr, nullptr) - 1;

	if (outputLength > 0)
	{
		output.resize(outputLength);
		::WideCharToMultiByte(win32CodePage, 0, input.data(), static_cast<int>(input.size()), output.data(), outputLength, nullptr, nullptr);
	}

	return output;
}

static std::wstring Win32WidenStdStringWithCodePage(std::string_view input, UINT win32CodePage)
{
	std::wstring utf16Output;
	const int utf16Length = ::MultiByteToWideChar(win32CodePage, 0, input.data(), static_cast<int>(input.size() + 1), nullptr, 0) - 1;

	if (utf16Length > 0)
	{
		utf16Output.resize(utf16Length);
		::MultiByteToWideChar(win32CodePage, 0, input.data(), static_cast<int>(input.size()), utf16Output.data(), utf16Length);
	}

	return utf16Output;
}

namespace UTF8
{
	std::string Narrow(std::wstring_view utf16Input)
	{
		return Win32NarrowStdStringWithCodePage(utf16Input, CP_UTF8);
	}

	std::wstring Widen(std::string_view utf8Input)
	{
		return Win32WidenStdStringWithCodePage(utf8Input, CP_UTF8);
	}

	std::string FromShiftJIS(std::string_view shiftJISInput)
	{
		// HACK: Quite inefficient of course but should work just fine for converting small amounts of text here and there
		return UTF8::Narrow(ShiftJIS::Widen(shiftJISInput));
	}

	WideArg::WideArg(std::string_view utf8Input)
	{
		// NOTE: Length **without** null terminator
		convertedLength = ::MultiByteToWideChar(CP_UTF8, 0, utf8Input.data(), static_cast<int>(utf8Input.size() + 1), nullptr, 0) - 1;
		if (convertedLength <= 0)
		{
			stackBuffer[0] = L'\0';
			return;
		}

		if (convertedLength < ArrayCount(stackBuffer))
		{
			::MultiByteToWideChar(CP_UTF8, 0, utf8Input.data(), static_cast<int>(utf8Input.size()), stackBuffer, convertedLength);
			stackBuffer[convertedLength] = L'\0';
		}
		else
		{
			// heapBuffer = std::make_unique<wchar_t[]>(convertedLength + 1);
			heapBuffer = std::unique_ptr<wchar_t[]>(new wchar_t[convertedLength + 1]);
			::MultiByteToWideChar(CP_UTF8, 0, utf8Input.data(), static_cast<int>(utf8Input.size()), heapBuffer.get(), convertedLength);
			heapBuffer[convertedLength] = L'\0';
		}
	}

	const wchar_t* WideArg::c_str() const
	{
		return (convertedLength < ArrayCount(stackBuffer)) ? stackBuffer : heapBuffer.get();
	}
}

namespace ShiftJIS
{
	// NOTE: According to https://docs.microsoft.com/en-us/windows/win32/intl/code-page-identifiers
	//		 932 | shift_jis | ANSI/OEM Japanese; Japanese (Shift-JIS)
	static constexpr UINT CP_SHIFT_JIS = 932;

	std::string Narrow(std::wstring_view utf16Input)
	{
		return Win32NarrowStdStringWithCodePage(utf16Input, CP_SHIFT_JIS);
	}

	std::wstring Widen(std::string_view utf8Input)
	{
		return Win32WidenStdStringWithCodePage(utf8Input, CP_SHIFT_JIS);
	}

	std::string FromUTF8(std::string_view utf8Input)
	{
		// HACK: Same problem as UTF8::FromShiftJIS() ...
		return ShiftJIS::Narrow(UTF8::WideArg(utf8Input).c_str());
	}
}

namespace ASCII
{
	template <typename T>
	static inline bool TryParsePrimitiveTypeT(std::string_view string, T& out)
	{
		const std::from_chars_result result = std::from_chars(string.data(), string.data() + string.size(), out);
		const bool hasNoError = (result.ec == std::errc {});
		const bool parsedFully = (result.ptr == string.data() + string.size());

		return hasNoError && parsedFully;
	}

	bool TryParseU32(std::string_view string, u32& out) { return TryParsePrimitiveTypeT(string, out); }
	bool TryParseI32(std::string_view string, i32& out) { return TryParsePrimitiveTypeT(string, out); }
	bool TryParseU64(std::string_view string, u64& out) { return TryParsePrimitiveTypeT(string, out); }
	bool TryParseI64(std::string_view string, i64& out) { return TryParsePrimitiveTypeT(string, out); }
	bool TryParseF32(std::string_view string, f32& out) { return TryParsePrimitiveTypeT(string, out); }
	bool TryParseF64(std::string_view string, f64& out) { return TryParsePrimitiveTypeT(string, out); }
}
