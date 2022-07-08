#include "core_types.h"
#include "core_string.h"
#include "chart_editor.h"
#include "main_settings.h"
#include <stdio.h>

namespace PeepoDrumKit
{
	struct ImGuiApplication
	{
		ChartEditor ChartEditor = {};

		ImGuiApplication()
		{
			auto& io = Gui::GetIO();
			io.ConfigWindowsMoveFromTitleBarOnly = true;

			// TODO: Maybe do some further adjustments here...
			io.KeyRepeatDelay = 0.450f;
			io.KeyRepeatRate = 0.050f;

			auto& style = Gui::GetStyle();
			style.CellPadding.y = 4.0f;
			style.FramePadding.y = 4.0f;
			// NOTE: The one downside of disabling this is that is makes tab bars and docked windows look the same
			style.WindowMenuButtonPosition = ImGuiDir_None;

			// TODO: Improve button color and remove green from sliders and maybe some other widgets too
			GuiStyleColorNiceLimeGreen(&style);
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
				Gui::DockSpace(Gui::GetID("FullscreenDockSpace"), { 0.0f, 0.0f }, ImGuiDockNodeFlags_PassthruCentralNode);
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

	int EntryPoint()
	{
		// TODO: Parse arguments and write into global argv settings struct
		// auto[argc, argv] = CommandLine::GetCommandLineUTF8();

		static std::unique_ptr<ImGuiApplication> app;

		ApplicationHost::StartupParam startupParam = {};
		startupParam.WindowTitle = PeepoDrumKitApplicationTitle;

		ApplicationHost::UserCallbacks callbacks = {};
		callbacks.OnStartup = [] { Audio::Engine.ApplicationStartup(); app = std::make_unique<ImGuiApplication>(); };
		callbacks.OnUpdate = [] { app->OnUpdate(); };
		callbacks.OnShutdown = [] { app = nullptr; Audio::Engine.ApplicationShutdown(); };
		callbacks.OnWindowCloseRequest = [] { return app->OnWindowCloseRequest(); };

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
