#include "chart_sound_effects.h"
#include "core_io.h"
#include "audio/audio_file_formats.h"

namespace PeepoDrumKit
{
	static constexpr const char* SoundEffectTypeFilePaths[] =
	{
		u8"assets/3rdparty/taiko_don_16bit_44100.wav",
		u8"assets/3rdparty/taiko_ka_16bit_44100.wav",
		u8"assets/3rdparty/metronome_bar_16bit_44100.wav",
		u8"assets/3rdparty/metronome_beat_16bit_44100.wav",
	};

	static_assert(ArrayCount(SoundEffectTypeFilePaths) == EnumCount<SoundEffectType>);

	void SoundEffectsVoicePool::StartAsyncLoadingAndAddVoices()
	{
		assert(!LoadSoundEffectFuture.valid());
		LoadSoundEffectFuture = std::async(std::launch::async, []() -> AsyncLoadSoundEffectsResult
		{
			AsyncLoadSoundEffectsResult result {};
			for (size_t i = 0; i < EnumCount<SoundEffectType>; i++)
			{
				const std::string_view inFilePath = SoundEffectTypeFilePaths[i];
				auto& resultBuffer = result.SampleBuffers[i];

				auto[fileContent, fileSize] = File::ReadAllBytes(inFilePath);
				if (fileContent == nullptr || fileSize == 0)
				{
					printf("Failed to read file '%.*s'\n", FmtStrViewArgs(inFilePath));
					continue;
				}

				if (Audio::DecodeEntireFile(inFilePath, fileContent.get(), fileSize, resultBuffer) != Audio::DecodeFileResult::FeelsGoodMan)
				{
					printf("Failed to decode audio file '%.*s'\n", FmtStrViewArgs(inFilePath));
					continue;
				}

				// HACK: ...
				if (resultBuffer.SampleRate != Audio::Engine.OutputSampleRate)
					Audio::LinearlyResampleBuffer<i16>(resultBuffer.InterleavedSamples, resultBuffer.FrameCount, resultBuffer.SampleRate, resultBuffer.ChannelCount, Audio::Engine.OutputSampleRate);
			}
			return result;
		});

		char nameBuffer[64];
		for (size_t i = 0; i < VoicePoolSize; i++)
		{
			VoicePool[i] = Audio::Engine.AddVoice(Audio::SourceHandle::Invalid, std::string_view(nameBuffer, sprintf_s(nameBuffer, "SoundEffectVoicePool[%02zu]", i)), false);
			VoicePool[i].SetPauseOnEnd(true);
		}
	}

	void SoundEffectsVoicePool::UpdateAsyncLoading()
	{
		if (LoadSoundEffectFuture.valid() && LoadSoundEffectFuture._Is_ready())
		{
			AsyncLoadSoundEffectsResult loadResult = LoadSoundEffectFuture.get();
			for (size_t i = 0; i < EnumCount<SoundEffectType>; i++)
			{
				if (loadResult.SampleBuffers[i].InterleavedSamples != nullptr)
					LoadedSources[i] = Audio::Engine.LoadSourceFromBufferMove(Path::GetFileName(SoundEffectTypeFilePaths[i]), std::move(loadResult.SampleBuffers[i]));
			}
		}
	}

	void SoundEffectsVoicePool::UnloadAllSourcesAndVoices()
	{
		for (auto& voice : VoicePool)
			Audio::Engine.RemoveVoice(voice);

		if (LoadSoundEffectFuture.valid())
		{
			LoadSoundEffectFuture.wait();
			UpdateAsyncLoading();
		}

		for (auto& handle : LoadedSources)
		{
			if (handle != Audio::SourceHandle::Invalid)
				Audio::Engine.UnloadSource(handle);
		}
	}

	void SoundEffectsVoicePool::PlaySound(SoundEffectType type, Time startTime, std::optional<Time> externalClock)
	{
		Audio::Engine.EnsureStreamRunning();

		// TODO: Maybe handle metronome in a different way entirely (separate voice pool?)
		const bool isMetronome = (type == SoundEffectType::MetronomeBar || type == SoundEffectType::MetronomeBeat);

		// TODO: Handle external clock differently so that there isn't any problem with preview sounds / the metronome clashing with manual inputs
		f32 voiceVolume = 1.0f;

		const Time timeSinceLastVoice = LastPlayedVoiceStopwatch.GetElapsed();
		if (!isMetronome && LastPlayedVoiceStopwatch.IsRunning)
		{
			static constexpr Time silenceThreshold = Time::FromFrames(2.0, 60.0);
			static constexpr Time fadeOutThreshold = Time::FromFrames(3.0, 60.0);

			if (timeSinceLastVoice < silenceThreshold)
			{
				voiceVolume = 0.0f;
			}
			else if (timeSinceLastVoice <= fadeOutThreshold)
			{
				voiceVolume = static_cast<f32>(ConvertRangeClampInput<f64>(0.0, fadeOutThreshold.Seconds, 1.0, 0.0, timeSinceLastVoice.Seconds));
				if (voiceVolume != 0.0f) voiceVolume /= voiceVolume;
			}
		}

		voiceVolume = isMetronome ? (voiceVolume * BaseVolumeMetronome) : (voiceVolume * BaseVolumeSfx);
		voiceVolume *= BaseVolumeMaster;

		if (voiceVolume > 0.0f)
		{
			if (!isMetronome)
				LastPlayedVoiceStopwatch.Restart();

			Audio::Voice voice = VoicePool[VoicePoolRingIndex];
			voice.SetSource(TryGetSourceForType(type));
			voice.SetPosition(startTime);
			voice.SetVolume(voiceVolume);
			voice.SetIsPlaying(true);

			VoicePoolRingIndex++;
			if (VoicePoolRingIndex >= VoicePoolSize)
				VoicePoolRingIndex = 0;
		}
	}

	void SoundEffectsVoicePool::PauseAllFutureVoices()
	{
		for (auto& voice : VoicePool)
		{
			if (voice.GetIsPlaying() && voice.GetPosition() < Time::Zero())
				voice.SetIsPlaying(false);
		}
	}

	Audio::SourceHandle SoundEffectsVoicePool::TryGetSourceForType(SoundEffectType type) const
	{
		assert(type < SoundEffectType::Count);

#if 0 // TODO: Hmm... maybe make user configurable
		if (type == SoundEffectType::MetronomeBar) return LoadedSources[EnumToIndex(SoundEffectType::TaikoDon)];
		if (type == SoundEffectType::MetronomeBeat) return LoadedSources[EnumToIndex(SoundEffectType::TaikoKa)];
#endif

		return LoadedSources[EnumToIndex(type)];
	}
}
