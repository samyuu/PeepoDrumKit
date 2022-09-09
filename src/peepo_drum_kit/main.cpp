#include "core_types.h"
#include "core_string.h"
#include "chart_editor.h"
#include "chart_editor_settings.h"

namespace PeepoDrumKit
{
	struct ImGuiApplication
	{
		ChartEditor ChartEditor = {};

		ImGuiApplication()
		{
			auto& io = Gui::GetIO();
			io.ConfigWindowsMoveFromTitleBarOnly = true;
			io.KeyRepeatDelay = 0.450f;
			io.KeyRepeatRate = 0.050f;

			auto& style = Gui::GetStyle();
			style.CellPadding.y = 4.0f;
			style.FramePadding.y = 4.0f;
			// NOTE: The one downside of disabling this is that is makes tab bars and docked windows look the same
			style.WindowMenuButtonPosition = ImGuiDir_None;

			GuiStyleColorPeepoDrumKit(&style);
		}

		void OnUpdate()
		{
			static constexpr ImGuiWindowFlags fullscreenWindowFlags =
				ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
				ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
				ImGuiWindowFlags_NoBackground;

			const ImGuiViewport* mainViewport = Gui::GetMainViewport();
			Gui::SetNextWindowPos(mainViewport->WorkPos);
			Gui::SetNextWindowSize(mainViewport->WorkSize);
			Gui::SetNextWindowViewport(mainViewport->ID);

			Gui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			Gui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			Gui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
			Gui::Begin("FullscreenDockSpaceWindow", nullptr, fullscreenWindowFlags);
			Gui::PopStyleVar(3);
			{
				const ImGuiID dockSpaceID = Gui::GetID("FullscreenDockSpace");
				if (Gui::DockBuilderGetNode(dockSpaceID) == nullptr)
					ChartEditor.RestoreDefaultDockSpaceLayout(dockSpaceID);

				Gui::DockSpace(dockSpaceID, { 0.0f, 0.0f }, ImGuiDockNodeFlags_PassthruCentralNode);
				ChartEditor.DrawFullscreenMenuBar();
			}
			Gui::End();

			ChartEditor.DrawGui();
		}

		ApplicationHost::CloseResponse OnWindowCloseRequest()
		{
			return ChartEditor.OnWindowCloseRequest();
		}
	};

	enum class LoadSettingsResponse { FileNotFound, AllGood, ErrorAbort, ErrorRetry, ErrorIgnore };

	template <typename T>
	static LoadSettingsResponse ReadParseSettingsIniFile(cstr iniFilePath, T& out)
	{
		auto fileContent = File::ReadAllBytes(iniFilePath);
		if (fileContent.Content == nullptr || fileContent.Size <= 0)
			return LoadSettingsResponse::AllGood;

		const SettingsParseResult parseResult = ParseSettingsIni(std::string_view(reinterpret_cast<const char*>(fileContent.Content.get()), fileContent.Size), out);
		if (!parseResult.HasError)
			return LoadSettingsResponse::AllGood;

		out.ResetDefault();

		char messageBuffer[2048];
		const int messageLength = sprintf_s(messageBuffer,
			"Failed to parse settings file:\n"
			"\"%s\"\n"
			"\n"
			"Syntax Error '%s' on Line %d\n"
			"\n"
			"[ Abort ] to exit application\n"
			"[ Retry ]  to read file again\n"
			"[ Ignore ] to *LOSE ALL SETTINGS* and restore the defaults\n"
			, iniFilePath, !parseResult.ErrorMessage.empty() ? parseResult.ErrorMessage.c_str() : "Unknown", parseResult.ErrorLineIndex + 1);

		const Shell::MessageBoxResult result = Shell::ShowMessageBox(
			std::string_view(messageBuffer, messageLength), "Peepo Drum Kit - Syntax Error",
			Shell::MessageBoxButtons::AbortRetryIgnore, Shell::MessageBoxIcon::Warning, nullptr);

		if (result == Shell::MessageBoxResult::Abort) return LoadSettingsResponse::ErrorAbort;
		if (result == Shell::MessageBoxResult::Retry) return LoadSettingsResponse::ErrorRetry;
		if (result == Shell::MessageBoxResult::Ignore) return LoadSettingsResponse::ErrorIgnore;
		assert(!"Unreachable"); return LoadSettingsResponse::ErrorAbort;
	}

	int EntryPoint()
	{
		// TODO: Parse arguments and write into global argv settings struct
		// auto[argc, argv] = CommandLine::GetCommandLineUTF8();

		while (true)
		{
			const auto response = ReadParseSettingsIniFile<PersistentAppData>(PersistentAppIniFileName, PersistentApp);
			if (response == LoadSettingsResponse::FileNotFound) { break; }
			if (response == LoadSettingsResponse::AllGood) { break; }
			if (response == LoadSettingsResponse::ErrorAbort) { return -1; }
			if (response == LoadSettingsResponse::ErrorRetry) { continue; }
			if (response == LoadSettingsResponse::ErrorIgnore) { break; }
			assert(!"Unreachable"); break;
		}

		while (true)
		{
			// TODO: Also set IsDirty in case of (older) version mismatch (?)
			const auto response = ReadParseSettingsIniFile<UserSettingsData>(SettingsIniFileName, Settings_Mutable);
			if (response == LoadSettingsResponse::FileNotFound) { Settings_Mutable.IsDirty = true; break; }
			if (response == LoadSettingsResponse::AllGood) { break; }
			if (response == LoadSettingsResponse::ErrorAbort) { return -1; }
			if (response == LoadSettingsResponse::ErrorRetry) { continue; }
			if (response == LoadSettingsResponse::ErrorIgnore) { break; }
			assert(!"Unreachable"); break;
		}

		static std::unique_ptr<ImGuiApplication> app;
		ApplicationHost::StartupParam startupParam = {};
		ApplicationHost::UserCallbacks callbacks = {};

		GuiScaleFactorTarget = ClampRoundGuiScaleFactor(PersistentApp.LastSession.GuiScale);
		ApplicationHost::GlobalState.SwapInterval = PersistentApp.LastSession.OSWindow_SwapInterval;
		startupParam.WindowTitle = PeepoDrumKitApplicationTitle;
		// TODO: ...
		// startupParam.WindowPosition = ...;
		// startupParam.WindowSize = ...;

		callbacks.OnStartup = []
		{
			Audio::Engine.ApplicationStartup();
			app = std::make_unique<ImGuiApplication>();
		};
		callbacks.OnUpdate = []
		{
			app->OnUpdate();
		};
		callbacks.OnShutdown = []
		{
			app = nullptr;
			Audio::Engine.ApplicationShutdown();

			PersistentApp.LastSession.GuiScale = GuiScaleFactorTarget;
			PersistentApp.LastSession.OSWindow_SwapInterval = ApplicationHost::GlobalState.SwapInterval;
			PersistentApp.LastSession.OSWindow_Region = Rect::FromTLSize(vec2(ApplicationHost::GlobalState.WindowPosition), vec2(ApplicationHost::GlobalState.WindowSize));
			// TODO: PersistentApp.LastSession.OSWindow_RegionRestore = ...;
			PersistentApp.LastSession.OSWindow_IsFullscreen = ApplicationHost::GlobalState.IsBorderlessFullscreen;
			// TODO: PersistentApp.LastSession.OSWindow_IsMaximized = ...;

			std::string iniFileContent; iniFileContent.reserve(4096);
			SettingsToIni(PersistentApp, iniFileContent); File::WriteAllBytes(PersistentAppIniFileName, iniFileContent);

			if (Settings.IsDirty)
			{
				iniFileContent.clear();
				SettingsToIni(Settings, iniFileContent); File::WriteAllBytes(SettingsIniFileName, iniFileContent);
				Settings_Mutable.IsDirty = false;
			}
		};
		callbacks.OnWindowCloseRequest = []
		{
			return app->OnWindowCloseRequest();
		};

		return ApplicationHost::EnterProgramLoop(startupParam, callbacks);
	}
}

#if 1
#include <Windows.h>
#include <fcntl.h>
#include <io.h>
static void Win32SetupConsoleMagic()
{
	::SetConsoleOutputCP(CP_UTF8);
	::_setmode(::_fileno(stdout), _O_BINARY);
	// TODO: Maybe overwrite the current locale too (?)
}
#else
static void Win32SetupConsoleMagic() { return; }
#endif

#if PEEPO_DEBUG
int main(int, const char**) { Win32SetupConsoleMagic(); return PeepoDrumKit::EntryPoint(); }
#elif PEEPO_RELEASE

#if 1
#include <Windows.h>
int WinMain(HINSTANCE, HINSTANCE, LPSTR, int) { Win32SetupConsoleMagic(); return PeepoDrumKit::EntryPoint(); }
#else
int WinMain(void*, void*, const wchar_t*, int) { Win32SetupConsoleMagic(); return PeepoDrumKit::EntryPoint(); }
#endif

#else
#error "Neither PEEPO_DEBUG nor PEEPO_RELEASE are defined :monkaS:"
#endif
