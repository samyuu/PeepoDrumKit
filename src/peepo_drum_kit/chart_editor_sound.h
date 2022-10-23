#pragma once
#include "core_types.h"
#include "audio/audio_engine.h"
#include <optional>

namespace PeepoDrumKit
{
	enum class SoundEffectType : u8
	{
		TaikoDon,
		TaikoKa,
		MetronomeBar,
		MetronomeBeat,
		Count
	};

	static constexpr cstr SoundEffectTypeFilePaths[] =
	{
		u8"assets/audio/taiko_don_16bit_44100.wav",
		u8"assets/audio/taiko_ka_16bit_44100.wav",
		u8"assets/audio/metronome_bar_16bit_44100.wav",
		u8"assets/audio/metronome_beat_16bit_44100.wav",
	};

	static_assert(ArrayCount(SoundEffectTypeFilePaths) == EnumCount<SoundEffectType>);

	struct AsyncLoadSoundEffectsResult
	{
		Audio::PCMSampleBuffer SampleBuffers[EnumCount<SoundEffectType>];
	};

	struct SoundEffectsVoicePool
	{
		SoundEffectsVoicePool() { for (auto& handle : LoadedSources) handle = Audio::SourceHandle::Invalid; }
		void StartAsyncLoadingAndAddVoices();
		void UpdateAsyncLoading();
		void UnloadAllSourcesAndVoices();

		void PlaySound(SoundEffectType type, Time startTime = Time::Zero(), std::optional<Time> externalClock = {});
		void PauseAllFutureVoices();
		Audio::SourceHandle TryGetSourceForType(SoundEffectType type) const;

		f32 BaseVolumeMaster = 1.0f;
		f32 BaseVolumeSfx = 1.0f;
		f32 BaseVolumeMetronome = 1.0f;
		i32 VoicePoolRingIndex = 0;
		static constexpr size_t VoicePoolSize = 32;
		Audio::Voice VoicePool[VoicePoolSize] = {};
		CPUStopwatch LastPlayedVoiceStopwatch = {};
		Time LastPlayedVoiceExternalClockTime = {};

		Audio::SourceHandle LoadedSources[EnumCount<SoundEffectType>] = {};
		std::future<AsyncLoadSoundEffectsResult> LoadSoundEffectFuture = {};
	};
}
