#pragma once
#include "core_types.h"
#include "core_beat.h"
#include "imgui/imgui_include.h"

namespace PeepoDrumKit
{
	constexpr std::string_view PeepoDrumKitApplicationTitle = PEEPO_DEBUG ? "Peepo Drum Kit (Debug)" : PEEPO_RELEASE ? "Peepo Drum Kit (Release)" : "Peepo Drum Kit (Unknown)";

	void GuiStyleColorPeepoDrumKit(ImGuiStyle* destination = nullptr);

	template <typename T>
	struct WithDefault
	{
		// NOTE: Default is initialized once and then shouldn't really change afterwards
		T Default;
		// NOTE: Value is either a *valid* copy of Default or user defined (to avoid always having to check HasValue first)
		T Value;
		bool HasValue;

		template<typename... Args>
		constexpr WithDefault(Args&&... args) : Default(std::forward<Args>(args)...), Value(Default), HasValue(false) {}

		constexpr T& operator*() { return Value; }
		constexpr T* operator->() { return &Value; }
		constexpr const T& operator*() const { return Value; }
		constexpr const T* operator->() const { return &Value; }
	};

	struct PersistentAppData
	{
		// TODO:
		// i32 SwapInterval;
		// std::vector<std::string> RecentFilePaths;
	};

	struct UserSettingsData
	{
		WithDefault<Beat> DrumrollHitBeatInterval = Beat::FromBars(1) / 16;

		// TODO: Adjust default values (?)
		WithDefault<f32> TimelineScrubAutoScrollPixelThreshold = 160.0f;
		WithDefault<f32> TimelineScrubAutoScrollSpeedMin = 150.0f;
		WithDefault<f32> TimelineScrubAutoScrollSpeedMax = 3500.0f;

		WithDefault<f32> TimelineSmoothScrollAnimationSpeed = 20.0f;
		WithDefault<f32> TimelineWaveformFadeAnimationSpeed = 20.0f;
		WithDefault<f32> TimelineRangeSelectionExpansionAnimationSpeed = 20.0f;
		WithDefault<f32> TimelineWorldSpaceCursorXAnimationSpeed = 35.0f;
		WithDefault<f32> TimelineGridSnapLineAnimationSpeed = 10.0f;
		WithDefault<f32> TimelineGoGoRangeExpansionAnimationSpeed = 20.0f;
		// TODO: Should probably also store scroll / zoom speed here

		struct InputData
		{
			WithDefault<MultiInputBinding> Dialog_YesOrOk = { KeyBinding(ImGuiKey_Enter), KeyBinding(ImGuiKey_KeypadEnter) };
			WithDefault<MultiInputBinding> Dialog_No = { KeyBinding(ImGuiKey_Backspace) };
			WithDefault<MultiInputBinding> Dialog_Cancel = { KeyBinding(ImGuiKey_Escape) };
			WithDefault<MultiInputBinding> Dialog_SelectNextTab = { KeyBinding(ImGuiKey_Tab, ImGuiModFlags_Ctrl) };
			WithDefault<MultiInputBinding> Dialog_SelectPreviousTab = { KeyBinding(ImGuiKey_Tab, ImGuiModFlags_Ctrl | ImGuiModFlags_Shift) };

			WithDefault<MultiInputBinding> Editor_ToggleFullscreen = { KeyBinding(ImGuiKey_F11), KeyBinding(ImGuiKey_Enter, ImGuiModFlags_Alt), KeyBinding(ImGuiKey_KeypadEnter, ImGuiModFlags_Alt) };
			WithDefault<MultiInputBinding> Editor_ToggleVSync = { KeyBinding(ImGuiKey_F10) };
			WithDefault<MultiInputBinding> Editor_Undo = { KeyBinding(ImGuiKey_Z, ImGuiModFlags_Ctrl) };
			WithDefault<MultiInputBinding> Editor_Redo = { KeyBinding(ImGuiKey_Y, ImGuiModFlags_Ctrl) };
			WithDefault<MultiInputBinding> Editor_ChartNew = { KeyBinding(ImGuiKey_N, ImGuiModFlags_Ctrl) };
			WithDefault<MultiInputBinding> Editor_ChartOpen = { KeyBinding(ImGuiKey_O, ImGuiModFlags_Ctrl) };
			WithDefault<MultiInputBinding> Editor_ChartOpenDirectory = {};
			WithDefault<MultiInputBinding> Editor_ChartSave = { KeyBinding(ImGuiKey_S, ImGuiModFlags_Ctrl) };
			WithDefault<MultiInputBinding> Editor_ChartSaveAs = { KeyBinding(ImGuiKey_S, ImGuiModFlags_Ctrl | ImGuiModFlags_Shift) };

			WithDefault<MultiInputBinding> Timeline_PlaceNoteDon = { KeyBinding(ImGuiKey_F), KeyBinding(ImGuiKey_J) };
			WithDefault<MultiInputBinding> Timeline_PlaceNoteKa = { KeyBinding(ImGuiKey_D), KeyBinding(ImGuiKey_K) };
			WithDefault<MultiInputBinding> Timeline_PlaceNoteBalloon = { KeyBinding(ImGuiKey_E), KeyBinding(ImGuiKey_I) };
			WithDefault<MultiInputBinding> Timeline_PlaceNoteDrumroll = { KeyBinding(ImGuiKey_R), KeyBinding(ImGuiKey_U) };
			WithDefault<MultiInputBinding> Timeline_Cut = { KeyBinding(ImGuiKey_X, ImGuiModFlags_Ctrl) };
			WithDefault<MultiInputBinding> Timeline_Copy = { KeyBinding(ImGuiKey_C, ImGuiModFlags_Ctrl) };
			WithDefault<MultiInputBinding> Timeline_Paste = { KeyBinding(ImGuiKey_V, ImGuiModFlags_Ctrl) };
			WithDefault<MultiInputBinding> Timeline_DeleteSelection = { KeyBinding(ImGuiKey_Delete) };
			WithDefault<MultiInputBinding> Timeline_StartEndRangeSelection = { KeyBinding(ImGuiKey_Tab) };
			WithDefault<MultiInputBinding> Timeline_StepCursorLeft = { KeyBinding(ImGuiKey_LeftArrow) };
			WithDefault<MultiInputBinding> Timeline_StepCursorRight = { KeyBinding(ImGuiKey_RightArrow) };
			WithDefault<MultiInputBinding> Timeline_IncreaseGridDivision = { KeyBinding(ImGuiKey_UpArrow) };
			WithDefault<MultiInputBinding> Timeline_DecreaseGridDivision = { KeyBinding(ImGuiKey_DownArrow) };
			WithDefault<MultiInputBinding> Timeline_IncreasePlaybackSpeed = { KeyBinding(ImGuiKey_C) };
			WithDefault<MultiInputBinding> Timeline_DecreasePlaybackSpeed = { KeyBinding(ImGuiKey_Z) };
			WithDefault<MultiInputBinding> Timeline_TogglePlayback = { KeyBinding(ImGuiKey_Space) };
			WithDefault<MultiInputBinding> Timeline_ToggleMetronome = { KeyBinding(ImGuiKey_F1) };
		} Input;

		struct AnimationData
		{
			// TODO: Move animation speeds here (?)
		} Animation;

#if 0  // TODO: (Should probably partially be inside TJA namespace)
		struct TJAWriteData
		{
			enum class TJATextEncoding { UTF8WithBom, ShiftJIS };
			enum class StringOrInt { String, Int };
			TJATextEncoding Encoding;
			StringOrInt DifficultyTypeFormat;
		} TJAWrite;
#endif
	};

	// TODO: Implement loading and saving...

	// NOTE: Load and save on every application startup and exit
	constexpr std::string_view PersistentAppIniFileName = "settings_app.ini";
	inline PersistentAppData PersistentApp = {};

	// NOTE: Load on application startup but only saved after making changes
	constexpr std::string_view SettingsIniFileName = "settings_user.ini";
	inline UserSettingsData Settings = {};
}
