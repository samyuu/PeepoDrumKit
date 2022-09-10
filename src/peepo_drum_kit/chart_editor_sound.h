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

	struct AsyncLoadSoundEffectsResult
	{
		Audio::PCMSampleBuffer SampleBuffers[EnumCount<SoundEffectType>];
	};

	struct SoundEffectsVoicePool
	{
		static constexpr size_t VoicePoolSize = 32;

		f32 BaseVolumeMaster = 1.0f;
		f32 BaseVolumeSfx = 1.0f;
		f32 BaseVolumeMetronome = 1.0f;
		i32 VoicePoolRingIndex = 0;
		Audio::Voice VoicePool[VoicePoolSize] = {};
		CPUStopwatch LastPlayedVoiceStopwatch = {};
		Time LastPlayedVoiceExternalClockTime = {};

		Audio::SourceHandle LoadedSources[EnumCount<SoundEffectType>] = {};
		std::future<AsyncLoadSoundEffectsResult> LoadSoundEffectFuture = {};

	public:
		SoundEffectsVoicePool() { for (auto& handle : LoadedSources) handle = Audio::SourceHandle::Invalid; }
	
	public:
		void StartAsyncLoadingAndAddVoices();
		void UpdateAsyncLoading();
		void UnloadAllSourcesAndVoices();

		void PlaySound(SoundEffectType type, Time startTime = Time::Zero(), std::optional<Time> externalClock = {});
		void PauseAllFutureVoices();

		Audio::SourceHandle TryGetSourceForType(SoundEffectType type) const;
	};
}
