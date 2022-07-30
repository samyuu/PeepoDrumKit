#pragma once
#include "core_types.h"
#include "audio_common.h"
#include "core_string.h"
#include <functional>
#include <future>
#include <array>

// TODO: Automatically add VariableRate playback for samplerate mismatched voices (?)

// NOTE: Terminology:
//		 "Sample" -> Raw PCM value for a single point in time
//		 "Frame"  -> Pair of samples for each channel
//		 "Source" -> Buffer of decoded samples (or potentially a file stream)
//		 "Voice"  -> Instance of a source, rendered to the output stream
namespace Audio
{
	// NOTE: Opaque types for referncing data stored in the AudioEngine, internally interpreted as in index.
	//		 A genertion ID could easily be tagged on for extra safety but so far it hasn't felt necessary
	using HandleBaseType = u16;
	enum class VoiceHandle : HandleBaseType { Invalid = 0xFFFF };
	enum class SourceHandle : HandleBaseType { Invalid = 0xFFFF };

	// NOTE: Lightweight non-owning wrapper around a VoiceHandle providing a convenient OOP interface
	struct Voice
	{
	public:
		VoiceHandle Handle = VoiceHandle::Invalid;

		constexpr Voice() = default;
		constexpr Voice(VoiceHandle handle) : Handle(handle) {}
		constexpr operator VoiceHandle() const { return Handle; }

	public:
		b8 IsValid() const;

		f32 GetVolume() const;
		void SetVolume(f32 value);

		f32 GetPlaybackSpeed() const;
		void SetPlaybackSpeed(f32 value);

		Time GetPosition() const;
		Time GetPositionSmooth() const;
		void SetPosition(Time value);

		SourceHandle GetSource() const;
		void SetSource(SourceHandle value);

		Time GetSourceDuration() const;

		b8 GetIsPlaying() const;
		void SetIsPlaying(b8 value);

		b8 GetIsLooping() const;
		void SetIsLooping(b8 value);

		b8 GetPlayPastEnd() const;
		void SetPlayPastEnd(b8 value);

		b8 GetRemoveOnEnd() const;
		void SetRemoveOnEnd(b8 value);

		b8 GetPauseOnEnd() const;
		void SetPauseOnEnd(b8 value);

		std::string_view GetName() const;

		void ResetVolumeMap();
		void SetVolumeMap(Time startTime, Time endTime, f32 startVolume, f32 endVolume);

	private:
		b8 GetInternalFlag(u16 flag) const;
		void SetInternalFlag(u16 flag, b8 value);
	};

	enum class Backend : u8
	{
		WASAPI_Shared,
		WASAPI_Exclusive,
		Count,
		// TEMP: Switching to shared during early developement where there isn't actually any charting to do yet
		// Default = WASAPI_Exclusive,
		Default = WASAPI_Shared,
	};

	constexpr cstr BackendNames[EnumCount<Backend>] =
	{
		"WASAPI (Shared)",
		"WASAPI (Exclusive)",
	};

	class AudioEngine : NonCopyable
	{
	public:
		static constexpr f32 MinVolume = 0.0f, MaxVolume = 1.0f;
		static constexpr size_t MaxSimultaneousVoices = 128;
		static constexpr size_t MaxLoadedSources = 256;

		static constexpr u32 OutputChannelCount = 2;
		static constexpr u32 OutputSampleRate = 44100;

		static constexpr u32 DefaultBufferFrameCount = 64;
		static constexpr u32 MinBufferFrameCount = 8;
		static constexpr u32 MaxBufferFrameCount = OutputSampleRate;

		static constexpr size_t CallbackDurationRingBufferSize = 64;
		static constexpr size_t LastPlayedSamplesRingBufferFrameCount = MaxBufferFrameCount;

	public:
		AudioEngine();
		~AudioEngine();

	public:
		void ApplicationStartup();
		void ApplicationShutdown();

		void OpenStartStream();
		void StopCloseStream();

		// NOTE: Little helper to easily delay opening and starting of the stream until it's necessary
		void EnsureStreamRunning();

	public:
		std::future<SourceHandle> LoadSourceFromFileAsync(std::string_view filePath);
		SourceHandle LoadSourceFromFileSync(std::string_view filePath);
		SourceHandle LoadSourceFromFileContentSync(std::string_view fileName, const void* fileContent, size_t fileSize);
		SourceHandle LoadSourceFromBufferMove(std::string_view sourceName, PCMSampleBuffer bufferToMove);
		void UnloadSource(SourceHandle source);

		const PCMSampleBuffer* GetSourceSampleBufferView(SourceHandle source);

		f32 GetSourceBaseVolume(SourceHandle source);
		void SetSourceBaseVolume(SourceHandle source, f32 value);

		std::string_view GetSourceName(SourceHandle source);
		void SetSourceName(SourceHandle source, std::string_view newName);

		// NOTE: Add a voice and keep a handle to it
		VoiceHandle AddVoice(SourceHandle source, std::string_view name, b8 playing, f32 volume = MaxVolume, b8 playPastEnd = false);
		void RemoveVoice(VoiceHandle voice);

		// NOTE: Add a voice, play it once then discard
		void PlayOneShotSound(SourceHandle source, std::string_view name, f32 volume = MaxVolume);

	public:
		Backend GetBackend() const;
		void SetBackend(Backend value);

		b8 GetIsStreamOpenRunning() const;
		b8 GetAllVoicesAreIdle() const;

		f32 GetMasterVolume() const;
		void SetMasterVolume(f32 value);

		u32 GetBufferFrameSize() const;
		void SetBufferFrameSize(u32 bufferFrameSize);

		Time GetCallbackFrequency() const;
		ChannelMixer& GetChannelMixer();

	public:
		i64 DebugGetTotalRenderedFrames() const;

		struct DebugVoicesArray { Voice Slots[MaxSimultaneousVoices]; size_t Count; };
		struct DebugSourcesArray { SourceHandle Slots[MaxLoadedSources]; size_t Count; };
		DebugVoicesArray DebugGetAllActiveVoices();
		DebugSourcesArray DebugGetAllLoadedSources();
		i32 DebugGetSourceVoiceInstanceCount(SourceHandle source);

		std::array<Time, CallbackDurationRingBufferSize> DebugGetRenderPerformanceHistory();
		std::array<std::array<i16, LastPlayedSamplesRingBufferFrameCount>, OutputChannelCount> DebugGetLastPlayedSamples();

	private:
		friend Voice;

		struct Impl;
		std::unique_ptr<Impl> impl = nullptr;
	};

	// NOTE: Single global instance
	inline AudioEngine Engine = {};
}
