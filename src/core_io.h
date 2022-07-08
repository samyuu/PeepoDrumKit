#pragma once
#include "core_types.h"
#include <string_view>
#include <vector>
#include <memory>

namespace Path
{
	constexpr char ExtensionSeparator = '.';
	constexpr char DirectorySeparator = '/', DirectorySeparatorWin32 = '\\';
	constexpr const char* DirectorySeparators = "/\\";

	constexpr char InvalidPathCharacters[] = { '\"', '<', '>', '|', '\0', };
	constexpr char InvalidFileNameCharacters[] = { '\"', '<', '>', '|', ':', '*', '?', '\\', '/', '\0', };

	constexpr bool IsValidPathChar(char charToCheck) { for (char invalid : InvalidFileNameCharacters) { if (charToCheck == invalid) return false; } return true; }

	std::string_view GetExtension(std::string_view filePath);
	std::string_view TrimExtension(std::string_view filePath);

	// NOTE: Case insensitive
	bool HasExtension(std::string_view filePath, std::string_view extension);
	// NOTE: Case insensitive, packed extensions to be separated by a semicolon (Example: ".wav;.flac;.ogg;.mp3")
	bool HasAnyExtension(std::string_view filePath, std::string_view packedExtensions);

	std::string_view GetFileName(std::string_view filePath, bool includeExtension = true);
	std::string_view GetDirectoryName(std::string_view filePath);

	bool IsRelative(std::string_view filePath);
	bool IsDirectory(std::string_view filePath);

	std::string TryMakeAbsolute(std::string_view relativePath, std::string_view baseFileOrDirectory);
	std::string TryMakeRelative(std::string_view absolutePath, std::string_view baseFileOrDirectory);

	// NOTE: Replace '\\' -> '/' etc.
	std::string CopyAndNormalize(std::string_view filePath);
	std::string& NormalizeInPlace(std::string& inOutFilePath);
	// NOTE: Replace '/' -> '\\' etc.
	std::string CopyAndNormalizeWin32(std::string_view filePath);
	std::string& NormalizeInPlaceWin32(std::string& inOutFilePath);
}

namespace File
{
	struct UniqueFileContent
	{
		std::unique_ptr<u8[]> Content;
		size_t Size;
	};

	UniqueFileContent ReadAllBytes(std::string_view filePath);
	bool WriteAllBytes(std::string_view filePath, const void* fileContent, size_t fileSize);
	bool WriteAllBytes(std::string_view filePath, const UniqueFileContent& uniqueFileContent);
	bool WriteAllBytes(std::string_view filePath, const std::string_view textFileContent);

	bool Exists(std::string_view filePath);
	bool Copy(std::string_view source, std::string_view destination, bool overwriteExisting = false);
}

namespace Directory
{
	// TODO: CreateNested, Exists, WIN32FINDFILE wrapper, etc.
	bool Create(std::string_view directoryPath);
	bool Exists(std::string_view directoryPath);

	std::string GetWorkingDirectory();
	void SetWorkingDirectory(std::string_view directoryPath);
}

namespace CommandLine
{
	struct CommandLineArrayView
	{
		size_t Count;
		std::string_view* Arguments;
	};

	// NOTE: Arguments[0] = program path, the returned string_views are also null-terminated
	CommandLineArrayView GetCommandLineUTF8();
}

namespace Shell
{
	// constexpr std::string_view FileLinkExtension = ".lnk";
	// bool IsFileLink(std::string filePath);
	// std::string ResolveFileLink(std::string_view lnkFilePath);

	void OpenInExplorer(std::string_view filePath);
	// void OpenExplorerProperties(std::string_view filePath);
	// void OpenWithDefaultProgram(std::string_view filePath);

	enum class MessageBoxButtons : u8 { AbortRetryIgnore, CancelTryContinue, OK, OKCancel, RetryCancel, YesNo, YesNoCancel };
	enum class MessageBoxIcon : u8 { Asterisk, Error, Exclamation, Hand, Information, None, Question, Stop, Warning };
	enum class MessageBoxResult : u8 { Abort, Cancel, Continue, Ignore, No, None, OK, Retry, TryAgain, Yes };
	MessageBoxResult ShowMessageBox(std::string_view message, std::string_view title, MessageBoxButtons buttons, MessageBoxIcon icon, void* parentWindowHandle);

	enum class FileDialogItemType { VisualGroupStart, VisualGroupEnd, Checkbox };

	struct FileDialogItem
	{
		FileDialogItemType Type;
		std::string_view Label;
		struct ItemData
		{
			bool* CheckboxChecked;
		} InOut;
	};

	constexpr std::string_view AllFilesFilterName = "All Files";
	constexpr std::string_view AllFilesFilterSpec = "*.*";

	struct FileFilter
	{
		// NOTE: In the format "File Type Name" without spec prefix
		std::string_view Name;
		// NOTE: In the format "*.ext" for a single format and "*.ext;*.ext;*.ext" for a list
		std::string_view Spec;
	};

	enum class FileDialogResult { OK, Cancel, Error };

	struct FileDialog
	{
		std::string_view InTitle;
		std::string_view InFileName;
		std::string_view InDefaultExtension;
		std::vector<FileFilter> InFilters;
		u32 InOutFilterIndex = 0;
		void* InParentWindowHandle = nullptr;
		std::vector<FileDialogItem> InOutCustomizeItems;
		std::string OutFilePath;

		FileDialogResult OpenRead();
		FileDialogResult OpenSave();
		FileDialogResult OpenSelectFolder();
	};
}
