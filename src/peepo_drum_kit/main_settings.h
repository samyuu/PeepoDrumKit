#pragma once
#include "core_types.h"
#include "core_beat.h"
#include "imgui/imgui_include.h"

namespace PeepoDrumKit
{
	struct RecentFilesList
	{
		// NOTE: Default 9 to match the keyboard number row key count
		size_t MaxCount = 9;
		// NOTE: Newest items are stored first and in normalized form
		std::vector<std::string> SortedPaths;

		// NOTE: Either add new item or move existing item to top
		void Add(std::string newFilePath, b8 addToEnd = false);
		void Remove(std::string filePathToRemove);
	};

	constexpr std::string_view PeepoDrumKitApplicationTitle = PEEPO_DEBUG ? "Peepo Drum Kit (Debug)" : PEEPO_RELEASE ? "Peepo Drum Kit (Release)" : "Peepo Drum Kit (Unknown)";

	void GuiStyleColorPeepoDrumKit(ImGuiStyle* destination = nullptr);

	enum class BoolOrDefault : u8 { Default = 0, True = 1, False = 2 };
	struct Optional_B8
	{
		BoolOrDefault Value;
		constexpr void Reset() { Value = BoolOrDefault::Default; }
		constexpr void SetValue(b8 value) { Value = value ? BoolOrDefault::True : BoolOrDefault::False; }
		constexpr b8 IsTrue() const { return (Value == BoolOrDefault::True); }
		constexpr b8 IsFalse() const { return (Value == BoolOrDefault::False); }
		constexpr b8 HasValue() const { return (Value == BoolOrDefault::Default); }
		constexpr b8 ValueOrDefault(b8 defaultValue) const { return (Value == BoolOrDefault::Default) ? defaultValue : (Value == BoolOrDefault::True); }
	};

	static_assert(sizeof(BoolOrDefault) == sizeof(b8));
	static_assert(sizeof(Optional_B8) == sizeof(b8));

	template <typename T>
	struct WithDefault
	{
		using UnderlyingType = T;

		// NOTE: Default is initialized once and then shouldn't really change afterwards
		T Default;
		// NOTE: Value is either a *valid* copy of Default or user defined (to avoid always having to check HasValue first)
		T Value;
		b8 HasValue;

		template<typename... Args>
		constexpr WithDefault(Args&&... args) : Default(std::forward<Args>(args)...), Value(Default), HasValue(false) {}
		inline void ResetToDefault() { Value = Default; HasValue = false; }
		inline void SetHasValueIfNotDefault() { HasValue = (Value != Default); }

		constexpr T& operator*() { return Value; }
		constexpr T* operator->() { return &Value; }
		constexpr const T& operator*() const { return Value; }
		constexpr const T* operator->() const { return &Value; }
	};

	struct PersistentAppData
	{
		inline void ResetDefault() { *this = PersistentAppData {}; }

		struct LastSessionData
		{
			f32 GuiScale = 1.0f;
			i32 OSWindow_SwapInterval = 1;
			Rect OSWindow_Region = {};
			Rect OSWindow_RegionRestore = {};
			b8 OSWindow_IsFullscreen = false;
			b8 OSWindow_IsMaximized = false;

			b8 ShowWindow_Help = true;
			b8 ShowWindow_Settings = true;
			b8 ShowWindow_AudioTest = false;
			b8 ShowWindow_TJAImportTest = false;
			b8 ShowWindow_TJAExportTest = false;
			b8 ShowWindow_ImGuiDemo = false;
			b8 ShowWindow_ImGuiStyleEditor = false;
		} LastSession = {};

		RecentFilesList RecentFiles;
	};

	struct UserSettingsData
	{
		inline void ResetDefault() { *this = UserSettingsData {}; }

		// NOTE: Defer save ini settings file on application exit
		b8 IsDirty = false;

		struct GeneralData
		{
			WithDefault<std::string> DefaultCreatorName = {};
			WithDefault<i32> DrumrollAutoHitBarDivision = 16;
			// TODO: Adjust default values (?)
			WithDefault<f32> TimelineScrubAutoScrollPixelThreshold = 160.0f;
			WithDefault<f32> TimelineScrubAutoScrollSpeedMin = 150.0f;
			WithDefault<f32> TimelineScrubAutoScrollSpeedMax = 3500.0f;
			// TODO: Should probably also store timeline scroll / zoom increment "speed" here
		} General;

		struct AudioData
		{
			WithDefault<b8> OpenDeviceOnStartup = true;
			WithDefault<b8> CloseDeviceOnIdleFocusLoss = false;
			WithDefault<b8> RequestExclusiveDeviceAccess = false;
		} Audio;

		struct AnimationData
		{
			WithDefault<f32> TimelineSmoothScrollSpeed = 20.0f;
			WithDefault<f32> TimelineWaveformFadeSpeed = 20.0f;
			WithDefault<f32> TimelineRangeSelectionExpansionSpeed = 20.0f;
			WithDefault<f32> TimelineWorldSpaceCursorXSpeed = 35.0f;
			WithDefault<f32> TimelineGridSnapLineSpeed = 10.0f;
			WithDefault<f32> TimelineGoGoRangeExpansionSpeed = 20.0f;
		} Animation;

		struct InputData
		{
			WithDefault<MultiInputBinding> Dialog_YesOrOk = { KeyBinding(ImGuiKey_Enter), KeyBinding(ImGuiKey_KeypadEnter) };
			WithDefault<MultiInputBinding> Dialog_No = { KeyBinding(ImGuiKey_Backspace) };
			WithDefault<MultiInputBinding> Dialog_Cancel = { KeyBinding(ImGuiKey_Escape) };
			WithDefault<MultiInputBinding> Dialog_SelectNextTab = { KeyBinding(ImGuiKey_Tab, ImGuiModFlags_Ctrl) };
			WithDefault<MultiInputBinding> Dialog_SelectPreviousTab = { KeyBinding(ImGuiKey_Tab, ImGuiModFlags_Ctrl | ImGuiModFlags_Shift) };

			WithDefault<MultiInputBinding> Editor_ToggleFullscreen = { KeyBinding(ImGuiKey_F11), KeyBinding(ImGuiKey_Enter, ImGuiModFlags_Alt), KeyBinding(ImGuiKey_KeypadEnter, ImGuiModFlags_Alt) };
			WithDefault<MultiInputBinding> Editor_ToggleVSync = {};
			WithDefault<MultiInputBinding> Editor_IncreaseGuiScale = { KeyBinding(ImGuiKey_Equal, ImGuiModFlags_Ctrl), KeyBinding(ImGuiKey_KeypadAdd, ImGuiModFlags_Ctrl) };
			WithDefault<MultiInputBinding> Editor_DecreaseGuiScale = { KeyBinding(ImGuiKey_Minus, ImGuiModFlags_Ctrl), KeyBinding(ImGuiKey_KeypadSubtract, ImGuiModFlags_Ctrl) };
			WithDefault<MultiInputBinding> Editor_ResetGuiScale = { KeyBinding(ImGuiKey_0, ImGuiModFlags_Ctrl) };
			WithDefault<MultiInputBinding> Editor_Undo = { KeyBinding(ImGuiKey_Z, ImGuiModFlags_Ctrl) };
			WithDefault<MultiInputBinding> Editor_Redo = { KeyBinding(ImGuiKey_Y, ImGuiModFlags_Ctrl) };
			WithDefault<MultiInputBinding> Editor_OpenHelp = { KeyBinding(ImGuiKey_F1) };
			WithDefault<MultiInputBinding> Editor_OpenSettings = { KeyBinding(ImGuiKey_Comma, ImGuiModFlags_Ctrl) };
			WithDefault<MultiInputBinding> Editor_ChartNew = { KeyBinding(ImGuiKey_N, ImGuiModFlags_Ctrl) };
			WithDefault<MultiInputBinding> Editor_ChartOpen = { KeyBinding(ImGuiKey_O, ImGuiModFlags_Ctrl) };
			WithDefault<MultiInputBinding> Editor_ChartOpenDirectory = { KeyBinding(ImGuiKey_O, ImGuiModFlags_Ctrl | ImGuiModFlags_Shift) };
			WithDefault<MultiInputBinding> Editor_ChartSave = { KeyBinding(ImGuiKey_S, ImGuiModFlags_Ctrl) };
			WithDefault<MultiInputBinding> Editor_ChartSaveAs = { KeyBinding(ImGuiKey_S, ImGuiModFlags_Ctrl | ImGuiModFlags_Shift) };

			WithDefault<MultiInputBinding> Timeline_PlaceNoteDon = { KeyBinding(ImGuiKey_F), KeyBinding(ImGuiKey_J) };
			WithDefault<MultiInputBinding> Timeline_PlaceNoteKa = { KeyBinding(ImGuiKey_D), KeyBinding(ImGuiKey_K) };
			WithDefault<MultiInputBinding> Timeline_PlaceNoteBalloon = { KeyBinding(ImGuiKey_E), KeyBinding(ImGuiKey_I) };
			WithDefault<MultiInputBinding> Timeline_PlaceNoteDrumroll = { KeyBinding(ImGuiKey_R), KeyBinding(ImGuiKey_U) };
			WithDefault<MultiInputBinding> Timeline_FlipNoteType = { KeyBinding(ImGuiKey_H) };
			WithDefault<MultiInputBinding> Timeline_ToggleNoteSize = { KeyBinding(ImGuiKey_B) };
			WithDefault<MultiInputBinding> Timeline_Cut = { KeyBinding(ImGuiKey_X, ImGuiModFlags_Ctrl) };
			WithDefault<MultiInputBinding> Timeline_Copy = { KeyBinding(ImGuiKey_C, ImGuiModFlags_Ctrl) };
			WithDefault<MultiInputBinding> Timeline_Paste = { KeyBinding(ImGuiKey_V, ImGuiModFlags_Ctrl) };
			WithDefault<MultiInputBinding> Timeline_DeleteSelection = { KeyBinding(ImGuiKey_Delete) };
			WithDefault<MultiInputBinding> Timeline_StartEndRangeSelection = { KeyBinding(ImGuiKey_Tab) };
			WithDefault<MultiInputBinding> Timeline_StepCursorLeft = { KeyBinding(ImGuiKey_LeftArrow) };
			WithDefault<MultiInputBinding> Timeline_StepCursorRight = { KeyBinding(ImGuiKey_RightArrow) };
			WithDefault<MultiInputBinding> Timeline_JumpToTimelineStart = { KeyBinding(ImGuiKey_Home) };
			WithDefault<MultiInputBinding> Timeline_JumpToTimelineEnd = { KeyBinding(ImGuiKey_End) };
			WithDefault<MultiInputBinding> Timeline_IncreaseGridDivision = { KeyBinding(ImGuiKey_UpArrow) };
			WithDefault<MultiInputBinding> Timeline_DecreaseGridDivision = { KeyBinding(ImGuiKey_DownArrow) };
			WithDefault<MultiInputBinding> Timeline_SetGridDivision_1_4 = {};
			WithDefault<MultiInputBinding> Timeline_SetGridDivision_1_8 = {};
			WithDefault<MultiInputBinding> Timeline_SetGridDivision_1_12 = {};
			WithDefault<MultiInputBinding> Timeline_SetGridDivision_1_16 = {};
			WithDefault<MultiInputBinding> Timeline_SetGridDivision_1_24 = {};
			WithDefault<MultiInputBinding> Timeline_SetGridDivision_1_32 = {};
			WithDefault<MultiInputBinding> Timeline_SetGridDivision_1_48 = {};
			WithDefault<MultiInputBinding> Timeline_SetGridDivision_1_64 = {};
			WithDefault<MultiInputBinding> Timeline_SetGridDivision_1_96 = {};
			WithDefault<MultiInputBinding> Timeline_SetGridDivision_1_192 = {};
			WithDefault<MultiInputBinding> Timeline_IncreasePlaybackSpeed = { KeyBinding(ImGuiKey_C) };
			WithDefault<MultiInputBinding> Timeline_DecreasePlaybackSpeed = { KeyBinding(ImGuiKey_Z) };
			WithDefault<MultiInputBinding> Timeline_SetPlaybackSpeed_100 = {};
			WithDefault<MultiInputBinding> Timeline_SetPlaybackSpeed_75 = {};
			WithDefault<MultiInputBinding> Timeline_SetPlaybackSpeed_50 = {};
			WithDefault<MultiInputBinding> Timeline_SetPlaybackSpeed_25 = {};
			WithDefault<MultiInputBinding> Timeline_TogglePlayback = { KeyBinding(ImGuiKey_Space) };
			WithDefault<MultiInputBinding> Timeline_ToggleMetronome = { KeyBinding(ImGuiKey_M) };

			WithDefault<MultiInputBinding> TempoCalculator_Tap = { KeyBinding(ImGuiKey_Space) };
			WithDefault<MultiInputBinding> TempoCalculator_Reset = { KeyBinding(ImGuiKey_Escape) };
		} Input;

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
	constexpr cstr PersistentAppIniFileName = "settings_app.ini";
	inline PersistentAppData PersistentApp = {};

	// NOTE: Load on application startup but only saved after making changes
	constexpr cstr SettingsIniFileName = "settings_user.ini";
	// NOTE: Separate const global reference to (hopefully) not accidentally forget setting the IsDirty flag after making changes
	inline UserSettingsData Settings_Mutable = {};
	inline const UserSettingsData& Settings = Settings_Mutable;

	struct SettingsParseResult { b8 HasError; i32 ErrorLineIndex; std::string ErrorMessage; };

	SettingsParseResult ParseSettingsIni(std::string_view fileContent, PersistentAppData& out);
	void SettingsToIni(const PersistentAppData& in, std::string& out);

	SettingsParseResult ParseSettingsIni(std::string_view fileContent, UserSettingsData& out);
	void SettingsToIni(const UserSettingsData& in, std::string& out);

	struct IniMemberVoidPtr { void* Address; size_t ByteSize; };
	struct IniMemberParseResult { b8 HasError; cstr ErrorMessage; };
	using IniVoidPtrTypeFromStringFunc = IniMemberParseResult(*)(std::string_view stringToParse, IniMemberVoidPtr out);
	using IniVoidPtrTypeToStringFunc = void(*)(IniMemberVoidPtr in, std::string& stringToAppendTo);

	struct SettingsReflectionMember
	{
		u16 ByteSizeValue;
		u16 ByteOffsetDefault;
		u16 ByteOffsetValue;
		u16 ByteOffsetHasValueB8;
		cstr SourceCodeName;
		cstr SerializedSection;
		cstr SerializedName;
		IniVoidPtrTypeFromStringFunc FromStringFunc;
		IniVoidPtrTypeToStringFunc ToStringFunc;
	};
	struct SettingsReflectionMap { SettingsReflectionMember MemberSlots[72]; size_t MemberCount; };

	SettingsReflectionMap StaticallyInitializeAppSettingsReflectionMap();
	inline const SettingsReflectionMap AppSettingsReflectionMap = StaticallyInitializeAppSettingsReflectionMap();
}
