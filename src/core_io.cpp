#include "core_io.h"
#include "core_string.h"
#include <vector>
#include <algorithm>
#include <shlwapi.h>
#include <shobjidl.h>
#include <Windows.h>
#include <wrl.h>
using Microsoft::WRL::ComPtr;

namespace Path
{
	static constexpr std::string_view Win32RootDirectoryPrefix = "\\";
	static constexpr std::string_view Win32CurrentDirectoryPrefix = ".\\";
	static constexpr std::string_view Win32ParentDirectoryPrefix = "..\\";

	std::string_view GetExtension(std::string_view filePath)
	{
		const size_t lastDirectoryIndex = filePath.find_last_of(DirectorySeparators);
		const std::string_view directoryTrimmed = (lastDirectoryIndex == std::string_view::npos) ? filePath : filePath.substr(lastDirectoryIndex);

		const size_t lastExtensionIndex = directoryTrimmed.find_last_of(ExtensionSeparator);
		return (lastExtensionIndex == std::string_view::npos) ? "" : directoryTrimmed.substr(lastExtensionIndex);
	}

	std::string_view TrimExtension(std::string_view filePath)
	{
		const std::string_view filePathExtension = GetExtension(filePath);
		return filePath.substr(0, filePath.size() - filePathExtension.size());
	}

	b8 HasExtension(std::string_view filePath, std::string_view extension)
	{
		const std::string_view filePathExtension = GetExtension(filePath);
		return ASCII::MatchesInsensitive(filePathExtension, extension);
	}

	b8 HasAnyExtension(std::string_view filePath, std::string_view packedExtensions)
	{
		const std::string_view filePathExtension = GetExtension(filePath);
		if (filePathExtension.empty() || packedExtensions.empty())
			return false;

		b8 anyExtensionMatches = false;
		ASCII::ForEachInCharSeparatedList(packedExtensions, ';', [&](std::string_view extension)
		{
			if (extension.empty()) { assert(!"Accidental invalid packedExtensions format (?)"); return; }
			if (ASCII::MatchesInsensitive(filePathExtension, extension))
				anyExtensionMatches = true;
		});
		return anyExtensionMatches;
	}

	std::string_view GetFileName(std::string_view filePath, b8 includeExtension)
	{
		const size_t lastDirectoryIndex = filePath.find_last_of(DirectorySeparators);
		const std::string_view fileName = (lastDirectoryIndex == std::string_view::npos) ? filePath : filePath.substr(lastDirectoryIndex + 1);
		return (includeExtension) ? fileName : TrimExtension(fileName);
	}

	std::string_view GetDirectoryName(std::string_view filePath)
	{
		const std::string_view fileName = GetFileName(filePath);
		return fileName.empty() ? filePath : filePath.substr(0, filePath.size() - fileName.size() - 1);
	}

	b8 IsRelative(std::string_view filePath)
	{
		return ::PathIsRelativeW(UTF8::WideArg(filePath).c_str());
	}

	b8 IsDirectory(std::string_view filePath)
	{
		return ::PathIsDirectoryW(UTF8::WideArg(filePath).c_str());
	}

	std::string TryMakeAbsolute(std::string_view relativePath, std::string_view baseFileOrDirectory)
	{
		if (relativePath.empty() || baseFileOrDirectory.empty() || !IsRelative(relativePath))
			return std::string(relativePath);

		// TODO: Also resolve "../" etc. I guess..?
		std::string baseDirectory { IsDirectory(baseFileOrDirectory) ? baseFileOrDirectory : GetDirectoryName(baseFileOrDirectory) };
		return baseDirectory.append("/").append(relativePath);
	}

	std::string TryMakeRelative(std::string_view absolutePath, std::string_view baseFileOrDirectory)
	{
		auto basePathU16 = UTF8::WideArg(CopyAndNormalizeWin32(baseFileOrDirectory));
		auto absolutePathU16 = UTF8::WideArg(CopyAndNormalizeWin32(absolutePath));

		wchar_t outRelative[MAX_PATH] = L"";
		const BOOL success = ::PathRelativePathToW(outRelative,
			basePathU16.c_str(), ::PathIsDirectoryW(basePathU16.c_str()) ? FILE_ATTRIBUTE_DIRECTORY : 0,
			absolutePathU16.c_str(), ::PathIsDirectoryW(absolutePathU16.c_str()) ? FILE_ATTRIBUTE_DIRECTORY : 0);

		return success ? std::string { ASCII::TrimPrefix(UTF8::Narrow(FixedBufferWStringView(outRelative)), Win32CurrentDirectoryPrefix) } : "";
	}

	std::string CopyAndNormalize(std::string_view filePath)
	{
		std::string normalizedCopy { filePath };
		std::replace(normalizedCopy.begin(), normalizedCopy.end(), DirectorySeparatorWin32, DirectorySeparator);
		return normalizedCopy;
	}

	std::string& NormalizeInPlace(std::string& inOutFilePath)
	{
		std::replace(inOutFilePath.begin(), inOutFilePath.end(), DirectorySeparatorWin32, DirectorySeparator);
		return inOutFilePath;
	}

	std::string CopyAndNormalizeWin32(std::string_view filePath)
	{
		std::string normalizedCopy { filePath };
		std::replace(normalizedCopy.begin(), normalizedCopy.end(), DirectorySeparator, DirectorySeparatorWin32);
		return normalizedCopy;
	}

	std::string& NormalizeInPlaceWin32(std::string& inOutFilePath)
	{
		std::replace(inOutFilePath.begin(), inOutFilePath.end(), DirectorySeparator, DirectorySeparatorWin32);
		return inOutFilePath;
	}
}

namespace File
{
	UniqueFileContent ReadAllBytes(std::string_view filePath)
	{
		if (filePath.empty())
			return UniqueFileContent {};

		const HANDLE fileHandle = ::CreateFileW(UTF8::WideArg(filePath).c_str(), GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (fileHandle == INVALID_HANDLE_VALUE)
			return UniqueFileContent {};

		defer { ::CloseHandle(fileHandle); };

		LARGE_INTEGER largeIntegerFileSize = {};
		if (::GetFileSizeEx(fileHandle, &largeIntegerFileSize) == 0)
			return UniqueFileContent {};

		const size_t fileSize = static_cast<size_t>(largeIntegerFileSize.QuadPart);
		auto fileContent = std::unique_ptr<u8[]>(new u8[fileSize + 1]);

		if (fileContent == nullptr)
			return UniqueFileContent {};

		// HACK: Assume the entire file fits inside a single DWORD for now
		DWORD bytesRead = 0;
		if (::ReadFile(fileHandle, fileContent.get(), static_cast<DWORD>(fileSize), &bytesRead, nullptr) == FALSE)
			return UniqueFileContent {};

		assert(bytesRead == fileSize);
		fileContent[fileSize] = '\0';

		return UniqueFileContent { std::move(fileContent), fileSize };
	}

	b8 WriteAllBytes(std::string_view filePath, const void* fileContent, size_t fileSize)
	{
		if (filePath.empty() || fileContent == nullptr)
			return false;

		const HANDLE fileHandle = ::CreateFileW(UTF8::WideArg(filePath).c_str(), GENERIC_WRITE, (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (fileHandle == INVALID_HANDLE_VALUE)
			return false;

		defer { ::CloseHandle(fileHandle); };

		// HACK: Assume the entire file fits inside a single DWORD for now
		DWORD bytesWritten = 0;
		if (::WriteFile(fileHandle, fileContent, static_cast<DWORD>(fileSize), &bytesWritten, nullptr) == FALSE)
			return false;

		return true;
	}

	b8 WriteAllBytes(std::string_view filePath, const UniqueFileContent& uniqueFileContent)
	{
		return WriteAllBytes(filePath, uniqueFileContent.Content.get(), uniqueFileContent.Size);
	}

	b8 WriteAllBytes(std::string_view filePath, const std::string_view textFileContent)
	{
		return WriteAllBytes(filePath, textFileContent.data(), textFileContent.size());
	}

	b8 Exists(std::string_view filePath)
	{
		const DWORD attributes = ::GetFileAttributesW(UTF8::WideArg(filePath).c_str());
		return (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY));
	}

	b8 Copy(std::string_view source, std::string_view destination, b8 overwriteExisting)
	{
		return ::CopyFileW(UTF8::WideArg(source).c_str(), UTF8::WideArg(destination).c_str(), !overwriteExisting);
	}
}

namespace CommandLine
{
	CommandLineArrayView GetCommandLineUTF8()
	{
		static b8 initialized = false;
		static std::vector<std::string> argvString;
		static std::vector<std::string_view> argvStringViews;

		if (initialized)
			return CommandLineArrayView { argvStringViews.size(), argvStringViews.data() };

		int argc = 0;
		auto argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
		{
			argvString.reserve(argc);
			argvStringViews.reserve(argc);

			for (int i = 0; i < argc; i++)
				argvStringViews.emplace_back(argvString.emplace_back(UTF8::Narrow(argv[i])).c_str());
		}
		::LocalFree(argv);

		initialized = true;
		return CommandLineArrayView { static_cast<size_t>(argc), argvStringViews.data() };
	}
}

namespace Directory
{
	b8 Create(std::string_view directoryPath)
	{
		if (directoryPath.empty())
			return false;

		return ::CreateDirectoryW(UTF8::WideArg(directoryPath).c_str(), 0);
	}

	b8 Exists(std::string_view directoryPath)
	{
		if (directoryPath.empty())
			return false;

		const DWORD attributes = ::GetFileAttributesW(UTF8::WideArg(directoryPath).c_str());
		return (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY));
	}


	std::string GetExecutablePath()
	{
		// TODO: First ask for size then resize dynamic buffer accordingly
		wchar_t buffer[MAX_PATH] = L"";
		::GetModuleFileNameW(NULL, buffer, MAX_PATH);
		return UTF8::Narrow(FixedBufferWStringView(buffer));
	}

	std::string GetExecutableDirectory()
	{
		return std::string { Path::GetDirectoryName(GetExecutablePath()) };
	}

	std::string GetWorkingDirectory()
	{
		// TODO: First ask for size then resize dynamic buffer accordingly
		wchar_t buffer[MAX_PATH] = L"";
		::GetCurrentDirectoryW(MAX_PATH, buffer);
		return UTF8::Narrow(FixedBufferWStringView(buffer));
	}

	void SetWorkingDirectory(std::string_view directoryPath)
	{
		::SetCurrentDirectoryW(UTF8::WideArg(directoryPath).c_str());
	}
}

namespace Shell
{
	void OpenInExplorer(std::string_view filePath)
	{
		if (filePath.empty())
			return;

		if (Path::IsRelative(filePath))
		{
			std::string absolutePath = Directory::GetWorkingDirectory(); absolutePath += "/"; absolutePath += filePath;
			::ShellExecuteW(NULL, L"open", UTF8::WideArg(Path::NormalizeInPlaceWin32(absolutePath)).c_str(), NULL, NULL, SW_SHOWDEFAULT);
		}
		else
		{
			::ShellExecuteW(NULL, L"open", UTF8::WideArg(Path::CopyAndNormalizeWin32(filePath)).c_str(), NULL, NULL, SW_SHOWDEFAULT);
		}
	}

	MessageBoxResult ShowMessageBox(std::string_view message, std::string_view title, MessageBoxButtons buttons, MessageBoxIcon icon, void* parentWindowHandle)
	{
		UINT flags = 0;
		switch (buttons)
		{
		case MessageBoxButtons::AbortRetryIgnore: flags |= MB_ABORTRETRYIGNORE; break;
		case MessageBoxButtons::CancelTryContinue: flags |= MB_CANCELTRYCONTINUE; break;
		case MessageBoxButtons::OK: flags |= MB_OK; break;
		case MessageBoxButtons::OKCancel: flags |= MB_OKCANCEL; break;
		case MessageBoxButtons::RetryCancel: flags |= MB_RETRYCANCEL; break;
		case MessageBoxButtons::YesNo: flags |= MB_YESNO; break;
		case MessageBoxButtons::YesNoCancel: flags |= MB_YESNOCANCEL; break;
		default: break;
		}
		switch (icon)
		{
		case MessageBoxIcon::Asterisk: flags |= MB_ICONASTERISK; break;
		case MessageBoxIcon::Error: flags |= MB_ICONERROR; break;
		case MessageBoxIcon::Exclamation: flags |= MB_ICONEXCLAMATION; break;
		case MessageBoxIcon::Hand: flags |= MB_ICONHAND; break;
		case MessageBoxIcon::Information: flags |= MB_ICONINFORMATION; break;
		case MessageBoxIcon::None: break;
		case MessageBoxIcon::Question: flags |= MB_ICONQUESTION; break;
		case MessageBoxIcon::Stop: flags |= MB_ICONSTOP; break;
		case MessageBoxIcon::Warning: flags |= MB_ICONWARNING; break;
		default: break;
		}

		const WORD languageID = 0;
		const int result = ::MessageBoxExW(reinterpret_cast<HWND>(parentWindowHandle), UTF8::WideArg(message).c_str(), title.empty() ? nullptr : UTF8::WideArg(title).c_str(), flags, languageID);
		switch (result)
		{
		case IDABORT: return MessageBoxResult::Abort;
		case IDCANCEL: return MessageBoxResult::Cancel;
		case IDCONTINUE: return MessageBoxResult::Continue;
		case IDIGNORE: return MessageBoxResult::Ignore;
		case IDNO: return MessageBoxResult::No;
		case IDOK: return MessageBoxResult::OK;
		case IDRETRY: return MessageBoxResult::Retry;
		case IDTRYAGAIN: return MessageBoxResult::TryAgain;
		case IDYES: return MessageBoxResult::Yes;
		default: return MessageBoxResult::None;
		}
	}
}

namespace Shell
{
	class VeryPogDialogEventHandler : public IFileDialogEvents, public IFileDialogControlEvents
	{
	public:
		VeryPogDialogEventHandler() : refCount(1) {}
		~VeryPogDialogEventHandler() = default;

	public:
#pragma region IUnknown
		IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
		{
			static const QITAB searchTable[] =
			{
				QITABENT(VeryPogDialogEventHandler, IFileDialogEvents),
				QITABENT(VeryPogDialogEventHandler, IFileDialogControlEvents),
				{ nullptr, 0 },
			};
			return QISearch(this, searchTable, riid, ppv);
		}

		IFACEMETHODIMP_(ULONG) AddRef()
		{
			return InterlockedIncrement(&refCount);
		}

		IFACEMETHODIMP_(ULONG) Release()
		{
			long cRef = InterlockedDecrement(&refCount);
			if (!cRef)
				delete this;
			return cRef;
		}
#pragma endregion IUnknown

#pragma region IFileDialogEvents
		IFACEMETHODIMP OnFileOk(IFileDialog*) { return S_OK; }
		IFACEMETHODIMP OnFolderChange(IFileDialog*) { return S_OK; }
		IFACEMETHODIMP OnFolderChanging(IFileDialog*, IShellItem*) { return S_OK; }
		IFACEMETHODIMP OnHelp(IFileDialog*) { return S_OK; }
		IFACEMETHODIMP OnSelectionChange(IFileDialog*) { return S_OK; }
		IFACEMETHODIMP OnShareViolation(IFileDialog*, IShellItem*, FDE_SHAREVIOLATION_RESPONSE*) { return S_OK; }
		IFACEMETHODIMP OnTypeChange(IFileDialog* pfd) { return S_OK; }
		IFACEMETHODIMP OnOverwrite(IFileDialog*, IShellItem*, FDE_OVERWRITE_RESPONSE*) { return S_OK; }

		IFACEMETHODIMP OnItemSelected(IFileDialogCustomize* pfdc, DWORD dwIDCtl, DWORD dwIDItem)
		{
			HRESULT result = S_OK;
			IFileDialog* fileDialog = nullptr;

			if (result = pfdc->QueryInterface(&fileDialog); !SUCCEEDED(result))
				return result;

#if 0 // NOTE: Add future item check logic here if needed
			switch (dwIDItem)
			{
			default:
				break;
			}
#endif

			fileDialog->Release();
			return result;
		}

		IFACEMETHODIMP OnButtonClicked(IFileDialogCustomize*, DWORD) { return S_OK; }
		IFACEMETHODIMP OnCheckButtonToggled(IFileDialogCustomize*, DWORD, BOOL) { return S_OK; }
		IFACEMETHODIMP OnControlActivating(IFileDialogCustomize*, DWORD) { return S_OK; }
#pragma endregion IFileDialogEvents

	private:
		long refCount;

	public:
		static HRESULT CreateInstance(REFIID riid, void** ppv)
		{
			*ppv = nullptr;
			if (VeryPogDialogEventHandler* pDialogEventHandler = new VeryPogDialogEventHandler(); pDialogEventHandler != nullptr)
			{
				HRESULT result = pDialogEventHandler->QueryInterface(riid, ppv);
				pDialogEventHandler->Release();
				return S_OK;
			}

			return E_OUTOFMEMORY;
		}
	};

	static constexpr uint32_t RandomFileDialogItemBaseID = 0x666;

	static void CreateNativeIFileDialogCustomizeItems(const std::vector<FileDialogItem>& inItems, IFileDialogCustomize* outDialogCustomize)
	{
		HRESULT result = S_OK;
		for (size_t i = 0; i < inItems.size(); i++)
		{
			const auto& inItem = inItems[i];
			const DWORD itemID = static_cast<DWORD>(RandomFileDialogItemBaseID + i);

			if (inItem.Type == FileDialogItemType::VisualGroupStart)
				result = outDialogCustomize->StartVisualGroup(itemID, UTF8::WideArg(inItem.Label).c_str());
			else if (inItem.Type == FileDialogItemType::VisualGroupEnd)
				result = outDialogCustomize->EndVisualGroup();
			else if (inItem.Type == FileDialogItemType::Checkbox)
			{
				result = outDialogCustomize->AddCheckButton(itemID, UTF8::WideArg(inItem.Label).c_str(), (inItem.InOut.CheckboxChecked != nullptr) ? *inItem.InOut.CheckboxChecked : false);

				if (inItem.InOut.CheckboxChecked == nullptr)
					result = outDialogCustomize->SetControlState(itemID, CDCS_VISIBLE);
			}
		}
	}

	static void ReadNativeIFileDialogCustomizeItemState(std::vector<FileDialogItem>& outItems, IFileDialogCustomize* inDialogCustomize)
	{
		HRESULT result = S_OK;
		for (size_t i = 0; i < outItems.size(); i++)
		{
			auto& outItem = outItems[i];
			const DWORD itemID = static_cast<DWORD>(RandomFileDialogItemBaseID + i);

			if (outItem.Type == FileDialogItemType::Checkbox)
			{
				BOOL checked;
				result = inDialogCustomize->GetCheckButtonState(itemID, &checked);
				if (outItem.InOut.CheckboxChecked != nullptr)
					*outItem.InOut.CheckboxChecked = checked;
			}
		}
	}

	enum class DialogType : u8 { Save, Open };
	enum class DialogPickType : u8 { File, Folder };

	static FileDialogResult CreateAndShowFileDialog(FileDialog& dialog, DialogType dialogType, DialogPickType pickType)
	{
		for (const auto& filter : dialog.InFilters)
		{
			assert(!filter.Name.empty() && filter.Name.back() != ')' && "Don't include a file spec suffix");
			assert(!filter.Spec.empty() && filter.Spec[0] == '*' && filter.Spec[1] == '.' && filter.Spec.back() != ';' && "Don't forget the \"*.\" prefix");
		}

		HRESULT hr = S_OK;
		hr = ::CoInitialize(nullptr);
		defer { ::CoUninitialize(); };

		ComPtr<IFileDialog> fileDialog = nullptr;
		hr = ::CoCreateInstance((dialogType == DialogType::Save) ? CLSID_FileSaveDialog : CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, __uuidof(fileDialog), &fileDialog);
		if (!SUCCEEDED(hr))
			return FileDialogResult::Error;

		DWORD eventCookie = 0;
		ComPtr<IFileDialogEvents> dialogEvents = nullptr;
		ComPtr<IFileDialogCustomize> dialogCustomize = nullptr;

		if (hr = VeryPogDialogEventHandler::CreateInstance(__uuidof(dialogEvents), &dialogEvents); SUCCEEDED(hr))
		{
			if (hr = fileDialog->Advise(dialogEvents.Get(), &eventCookie); SUCCEEDED(hr))
			{
				if (hr = fileDialog->QueryInterface(__uuidof(dialogCustomize), &dialogCustomize); SUCCEEDED(hr))
					CreateNativeIFileDialogCustomizeItems(dialog.InOutCustomizeItems, dialogCustomize.Get());

				DWORD existingOptionFlags = 0;
				hr = fileDialog->GetOptions(&existingOptionFlags);
				hr = fileDialog->SetOptions(existingOptionFlags | FOS_NOCHANGEDIR | FOS_FORCEFILESYSTEM | (pickType == DialogPickType::Folder ? FOS_PICKFOLDERS : 0));

				if (!dialog.InTitle.empty())
					hr = fileDialog->SetTitle(UTF8::WideArg(dialog.InTitle).c_str());

				if (!dialog.InFileName.empty())
					hr = fileDialog->SetFileName(UTF8::WideArg(dialog.InFileName).c_str());

				if (!dialog.InDefaultExtension.empty())
					hr = fileDialog->SetDefaultExtension(UTF8::WideArg(ASCII::TrimPrefix(dialog.InDefaultExtension, ".")).c_str());

				if (pickType == DialogPickType::File && !dialog.InFilters.empty())
				{
					// NOTE: Must reserve so the c_str()s won't get invalidated in case of small-string-optimization
					std::vector<std::wstring> owningWideStrings;
					owningWideStrings.reserve(dialog.InFilters.size() * 2);

					std::vector<COMDLG_FILTERSPEC> nonOwningConvertedFilters;
					nonOwningConvertedFilters.reserve(dialog.InFilters.size());

					std::string nameWithSpecSuffix;
					for (const auto& inFilter : dialog.InFilters)
					{
						nameWithSpecSuffix.clear();
						nameWithSpecSuffix += inFilter.Name;
						nameWithSpecSuffix += " (";
						nameWithSpecSuffix += inFilter.Spec;
						nameWithSpecSuffix += ')';

						nonOwningConvertedFilters.push_back(COMDLG_FILTERSPEC
							{
								owningWideStrings.emplace_back(std::move(UTF8::Widen(nameWithSpecSuffix))).c_str(),
								owningWideStrings.emplace_back(std::move(UTF8::Widen(inFilter.Spec))).c_str(),
							});
					}

					hr = fileDialog->SetFileTypes(static_cast<UINT>(nonOwningConvertedFilters.size()), nonOwningConvertedFilters.data());
					hr = fileDialog->SetFileTypeIndex(dialog.InOutFilterIndex);
				}
			}
		}

		if (SUCCEEDED(hr))
		{
			if (hr = fileDialog->Show(reinterpret_cast<HWND>(dialog.InParentWindowHandle)); SUCCEEDED(hr))
			{
				ComPtr<IShellItem> itemResult = nullptr;
				if (hr = fileDialog->GetResult(&itemResult); SUCCEEDED(hr))
				{
					wchar_t* filePath = nullptr;
					if (hr = itemResult->GetDisplayName(SIGDN_FILESYSPATH, &filePath); SUCCEEDED(hr))
					{
						hr = fileDialog->GetFileTypeIndex(&dialog.InOutFilterIndex);
						dialog.OutFilePath = UTF8::Narrow(filePath);

						if (dialogCustomize != nullptr)
							ReadNativeIFileDialogCustomizeItemState(dialog.InOutCustomizeItems, dialogCustomize.Get());

						::CoTaskMemFree(filePath);
					}
				}
			}
		}

		if (fileDialog != nullptr)
			fileDialog->Unadvise(eventCookie);

		if (FAILED(hr))
			return FileDialogResult::Error;

		return !dialog.OutFilePath.empty() ? FileDialogResult::OK : FileDialogResult::Cancel;
	}

	FileDialogResult FileDialog::OpenRead() { return CreateAndShowFileDialog(*this, DialogType::Open, DialogPickType::File); }
	FileDialogResult FileDialog::OpenSave() { return CreateAndShowFileDialog(*this, DialogType::Save, DialogPickType::File); }
	FileDialogResult FileDialog::OpenSelectFolder() { return CreateAndShowFileDialog(*this, DialogType::Open, DialogPickType::Folder); }
}
