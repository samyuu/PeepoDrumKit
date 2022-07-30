#pragma once
#include "core_types.h"
#include <string>
#include <string_view>

// NOTE: Runs the danger of double evaluating the string expression but I'm starting to get really tired of manually typing out the size cast
#define StrViewFmtString "%.*s"
#define FmtStrViewArgs(stringView) static_cast<i32>(stringView.size()), stringView.data()

constexpr cstr BoolToCStr(b8 value) { return value ? "true" : "false"; }

// NOTE: To correctly handle potentially missing null terminator in a fixed-sized char[] buffer
template <size_t BufferSize>
inline std::string_view FixedBufferStringView(const char(&buffer)[BufferSize]) { return std::string_view(buffer, ::strnlen(buffer, BufferSize)); }
inline std::string_view FixedBufferStringView(const char* buffer, size_t bufferSize) { return std::string_view(buffer, ::strnlen(buffer, bufferSize)); }

template <size_t BufferSize>
inline std::wstring_view FixedBufferWStringView(const wchar_t(&buffer)[BufferSize]) { return std::wstring_view(buffer, ::wcsnlen(buffer, BufferSize)); }
inline std::wstring_view FixedBufferWStringView(const wchar_t* buffer, size_t bufferSize) { return std::wstring_view(buffer, ::wcsnlen(buffer, bufferSize)); }

template <size_t BufferSize>
inline void CopyStringViewIntoFixedBuffer(char(&buffer)[BufferSize], std::string_view stringToCopy)
{
	const size_t length = Min(stringToCopy.size() + 1, BufferSize) - 1;
	::memcpy(buffer, stringToCopy.data(), length);
	buffer[length] = '\0';
}
inline void CopyStringViewIntoFixedBuffer(char* buffer, size_t bufferSize, std::string_view stringToCopy)
{
	const size_t length = Min(stringToCopy.size() + 1, bufferSize) - 1;
	::memcpy(buffer, stringToCopy.data(), length);
	buffer[length] = '\0';
}

// NOTE: Following the "UTF-8 Everywhere" guidelines
namespace UTF8
{
	// NOTE: https://docs.microsoft.com/en-us/globalization/encoding/byte-order-mark
	constexpr const char BOM_UTF8[3] = { '\xEF', '\xBB', '\xBF' };
	constexpr const char BOM_UTF16_LE[2] = { '\xFF', '\xFE' };
	constexpr const char BOM_UTF16_BE[2] = { '\xFE', '\xFF' };
	constexpr const char BOM_UTF32_LE[4] = { '\xFF', '\xFE', '\x00', '\x00' };
	constexpr const char BOM_UTF32_BE[4] = { '\x00', '\x00', '\xFE', '\xFF' };

	constexpr b8 HasBOM(std::string_view input) { return (input.size() >= sizeof(BOM_UTF8)) && (input.substr(0, sizeof(BOM_UTF8)) == std::string_view(BOM_UTF8, sizeof(BOM_UTF8))); }
	constexpr std::string_view TrimBOM(std::string_view inputWithBOM) { return inputWithBOM.substr(sizeof(BOM_UTF8)); }

	// NOTE: Convert UTF-16 to UTF-8
	std::string Narrow(std::wstring_view utf16Input);

	// NOTE: Convert UTF-8 to UTF-16
	std::wstring Widen(std::string_view utf8Input);

	// NOTE: To avoid needless heap allocations for temporary wchar_t C-API function arguments
	//		 Example: DummyU16FuncW(UTF8::WideArg(stringU8).c_str(), ...)
	class WideArg : NonCopyable
	{
	public:
		WideArg(std::string_view utf8Input);
		const wchar_t* c_str() const;

	private:
		std::unique_ptr<wchar_t[]> heapBuffer;
		wchar_t stackBuffer[260];
		int convertedLength;
	};

	std::string FromShiftJIS(std::string_view shiftJISInput);
}

namespace ShiftJIS
{
	// NOTE: Convert UTF-16 to Shift-JIS
	std::string Narrow(std::wstring_view utf16Input);

	// NOTE: Convert SHIFT-JIS to UTF-16
	std::wstring Widen(std::string_view shiftJISInput);

	std::string FromUTF8(std::string_view utf8Input);
}

namespace ASCII
{
	constexpr char WhitespaceCharacters[] = " \t\r\n";
	constexpr char CaseDifference = ('A' - 'a');
	constexpr char LowerCaseMin = 'a', LowerCaseMax = 'z';
	constexpr char UpperCaseMin = 'A', UpperCaseMax = 'Z';

	constexpr b8 IsWhitespace(char c) { return (c == ' ' || c == '\t' || c == '\r' || c == '\n'); }
	constexpr b8 IsAllWhitespace(std::string_view v) { for (const char c : v) { if (!IsWhitespace(c)) return false; } return true; }

	constexpr b8 IsLowerCase(char c) { return (c >= LowerCaseMin && c <= LowerCaseMax); }
	constexpr b8 IsUpperCase(char c) { return (c >= UpperCaseMin && c <= UpperCaseMax); }
	constexpr char ToLowerCase(char c) { return IsUpperCase(c) ? (c - CaseDifference) : c; }
	constexpr char ToUpperCase(char c) { return IsLowerCase(c) ? (c + CaseDifference) : c; }

	constexpr b8 MatchesInsensitive(std::string_view a, std::string_view b)
	{
		if (a.size() != b.size())
			return false;

		for (auto i = 0; i < a.size(); i++)
		{
			if (ASCII::ToLowerCase(a[i]) != ASCII::ToLowerCase(b[i]))
				return false;
		}
		return true;
	}

	constexpr b8 StartsWith(std::string_view v, char prefix) { return (!v.empty() && v.front() == prefix); }
	constexpr b8 StartsWith(std::string_view v, std::string_view prefix) { return (v.size() >= prefix.size() && (v.substr(0, prefix.size()) == prefix)); }
	constexpr b8 StartsWithInsensitive(std::string_view v, std::string_view prefix) { return (v.size() >= prefix.size() && MatchesInsensitive(v.substr(0, prefix.size()), prefix)); }
	constexpr b8 EndsWith(std::string_view v, char suffix) { return (!v.empty() && v.back() == suffix); }
	constexpr b8 EndsWith(std::string_view v, std::string_view suffix) { return (v.size() >= suffix.size() && (v.substr(v.size() - suffix.size()) == suffix)); }
	constexpr b8 EndsWithInsensitive(std::string_view v, std::string_view suffix) { return (v.size() >= suffix.size() && MatchesInsensitive(v.substr(v.size() - suffix.size()), suffix)); }

	constexpr std::string_view TrimLeft(std::string_view v) { size_t nonWS = v.find_first_not_of(WhitespaceCharacters); return (nonWS == std::string_view::npos) ? v : v.substr(nonWS); }
	constexpr std::string_view TrimRight(std::string_view v) { size_t nonWS = v.find_last_not_of(WhitespaceCharacters); return (nonWS == std::string_view::npos) ? v : v.substr(0, nonWS + 1); }
	constexpr std::string_view Trim(std::string_view v) { return TrimRight(TrimLeft(v)); }
	constexpr std::string_view TrimPrefix(std::string_view v, std::string_view prefix) { return StartsWith(v, prefix) ? v.substr(prefix.size(), v.size() - prefix.size()) : v; }
	constexpr std::string_view TrimPrefixInsensitive(std::string_view v, std::string_view prefix) { return StartsWithInsensitive(v, prefix) ? v.substr(prefix.size(), v.size() - prefix.size()) : v; }
	constexpr std::string_view TrimSuffix(std::string_view v, std::string_view suffix) { return EndsWith(v, suffix) ? v.substr(0, v.size() - suffix.size()) : v; }
	constexpr std::string_view TrimSuffixInsensitive(std::string_view v, std::string_view suffix) { return EndsWithInsensitive(v, suffix) ? v.substr(0, v.size() - suffix.size()) : v; }

	template <typename Func>
	void ForEachLineInMultiLineString(std::string_view multiLineString, b8 includeEmptyTrailingLine, Func perLineFunc)
	{
		const char* const begin = multiLineString.data();
		const char* const end = multiLineString.data() + multiLineString.size();
		const char* readHead = begin;
		const char* lineBegin = begin;

		if (includeEmptyTrailingLine && begin == end)
			perLineFunc(std::string_view(begin, 0));

		while (readHead < end)
		{
			if (&readHead[1] == end)
			{
				const char* lineEnd = end;
				perLineFunc(std::string_view(lineBegin, (lineEnd - lineBegin)));

				if (includeEmptyTrailingLine && &lineEnd[-1] >= begin && lineEnd[-1] == '\n')
					perLineFunc(std::string_view(lineEnd, 0));
			}
			else if (readHead[0] == '\n')
			{
				const char* lineEnd = (&readHead[-1] >= begin && readHead[-1] == '\r') ? (readHead - 1) : readHead;
				perLineFunc(std::string_view(lineBegin, (lineEnd - lineBegin)));
				lineBegin = (readHead + 1);
			}
			readHead++;
		}
	}

	template <typename Func>
	void ForEachInCharSeparatedList(std::string_view separatedList, char separator, Func perValueFunc)
	{
		for (size_t absoluteIndex = 0; absoluteIndex < separatedList.size(); absoluteIndex++)
		{
			const std::string_view separatedValue = separatedList.substr(absoluteIndex, separatedList.find_first_of(separator, absoluteIndex) - absoluteIndex);
			perValueFunc(separatedValue);
			absoluteIndex += separatedValue.size();
		}
	}

	template <typename Func>
	void ForEachInCommaSeparatedList(std::string_view commaSeparatedList, Func perValueFunc) { return ForEachInCharSeparatedList(commaSeparatedList, ',', perValueFunc); }

	template <typename Func>
	void ForEachInSpaceSeparatedList(std::string_view spaceSeparatedList, Func perValueFunc) { return ForEachInCharSeparatedList(spaceSeparatedList, ' ', perValueFunc); }

	b8 TryParseU32(std::string_view string, u32& out);
	b8 TryParseI32(std::string_view string, i32& out);
	b8 TryParseU64(std::string_view string, u64& out);
	b8 TryParseI64(std::string_view string, i64& out);
	b8 TryParseF32(std::string_view string, f32& out);
	b8 TryParseF64(std::string_view string, f64& out);
}
