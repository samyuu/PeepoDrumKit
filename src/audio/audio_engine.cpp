#include "audio_engine.h"
#include "audio_file_formats.h"
#include "audio_backend.h"
#include "core_io.h"
#include <mutex>

namespace Audio
{
	constexpr HandleBaseType VoiceHandleToIndex(VoiceHandle handle) { return static_cast<HandleBaseType>(handle); }
	constexpr VoiceHandle IndexToVoiceHandle(HandleBaseType index) { return static_cast<VoiceHandle>(index); }

	constexpr HandleBaseType SourceHandleToIndex(SourceHandle handle) { return static_cast<HandleBaseType>(handle); }
	constexpr SourceHandle IndexToSourceHandle(HandleBaseType index) { return static_cast<SourceHandle>(index); }

	static std::unique_ptr<IAudioBackend> CreateBackendInterface(Backend backend)
	{
		switch (backend)
		{
		case Backend::WASAPI_Shared:
		case Backend::WASAPI_Exclusive:
			return std::make_unique<WASAPIBackend>();
		}

		assert(false);
		return nullptr;
	}

	using VoiceFlags = u16;
	enum VoiceFlagsEnum : VoiceFlags
	{
		VoiceFlags_Dead = 0,
		VoiceFlags_Alive = 1 << 0,
		VoiceFlags_Playing = 1 << 1,
		VoiceFlags_Looping = 1 << 2,
		VoiceFlags_PlayPastEnd = 1 << 3,
		VoiceFlags_RemoveOnEnd = 1 << 4,
		VoiceFlags_PauseOnEnd = 1 << 5,
		VoiceFlags_VariablePlaybackSpeed = 1 << 6,
	};

	// NOTE: Indexed into by VoiceHandle, slot valid if Flags != VoiceFlags_Dead
	struct VoiceData
	{
		// NOTE: Automatically resets to SourceHandle::Invalid when the source is unloaded
		std::atomic<VoiceFlags> Flags;
		std::atomic<SourceHandle> Source;
		std::atomic<f32> Volume;
		std::atomic<i64> FramePosition;

		std::atomic<f32> PlaybackSpeed;
		std::atomic<f64> TimePositionSec;

		struct SmoothTimeData
		{
			std::atomic<bool> RequestUpdate;
			//std::atomic<f64> BaseSystemTimeSec;
			std::atomic<i64> BaseCPUTimeTicks;
			std::atomic<f64> BaseVoiceTimeSec;
		} SmoothTime;

		// TODO: Loop between
		// std::atomic<i64> LoopStartFrame, LoopEndFrame;

		struct AtomicVoiceVolumeMap
		{
			std::atomic<i64> StartFrame, EndFrame;
			std::atomic<f32> StartVolume, EndVolume;
		} VolumeMap;

		char Name[64];
	};

	// NOTE: Indexed into by SourceHandle, slot valid if SlotUsed
	struct SourceData
	{
		std::atomic<bool> SlotUsed;
		PCMSampleBuffer Buffer;
		std::atomic<f32> BaseVolume = 0.0f;
		char Name[256];
	};

	struct AudioEngine::Impl
	{
	public:
		b8 IsStreamOpenRunning = false;
		std::atomic<f32> MasterVolume = AudioEngine::MaxVolume;

	public:
		ChannelMixer ChannelMixer = {};

		Backend CurrentBackendType = {};
		std::unique_ptr<IAudioBackend> CurrentBackend = nullptr;

	public:
		std::mutex VoiceRenderMutex;

		// NOTE: Slot maps indexed into via handles
		std::array<VoiceData, MaxSimultaneousVoices> VoicePool;
		std::array<SourceData, MaxLoadedSources> LoadedSources;

	public:
		std::array<i16, (MaxBufferFrameCount * OutputChannelCount)> TempOutputBuffer = {};
		u32 CurrentBufferFrameSize = DefaultBufferFrameCount;

		// TODO: Rename "CallbackDuration" to "RenderDuration" (?)
		// NOTE: For measuring performance
		size_t CallbackDurationRingIndex = 0;
		std::array<Time, AudioEngine::CallbackDurationRingBufferSize> CallbackDurationsRingBuffer = {};

		// NOTE: For visualizing the current audio output
		size_t LastPlayedSamplesRingIndex = 0;
		std::array<std::array<i16, AudioEngine::LastPlayedSamplesRingBufferFrameCount>, OutputChannelCount> LastPlayedSamplesRingBuffer = {};

		CPUStopwatch StreamTimeStopwatch = {};
		Time CallbackFrequency = {};
		Time CallbackStreamTime = {}, LastCallbackStreamTime = {};

		std::atomic<i64> TotalRenderedFrames = {};

	public:
		VoiceData* TryGetVoiceData(VoiceHandle handle)
		{
			const HandleBaseType handleIndex = VoiceHandleToIndex(handle);
			if (!InBounds(handleIndex, VoicePool))
				return nullptr;

			VoiceData* voiceData = &VoicePool[handleIndex];
			return (voiceData->Flags & VoiceFlags_Alive) ? voiceData : nullptr;
		}

		enum class GetSourceDataParam : u8 { None, ValidateBuffer };
		SourceData* TryGetSourceData(SourceHandle handle, GetSourceDataParam param)
		{
			const HandleBaseType handleIndex = SourceHandleToIndex(handle);
			if (!InBounds(handleIndex, LoadedSources))
				return nullptr;

			SourceData* sourceData = &LoadedSources[handleIndex];
			if (!sourceData->SlotUsed)
				return nullptr;

			if (param == GetSourceDataParam::ValidateBuffer)
				return (sourceData->Buffer.InterleavedSamples != nullptr) && (sourceData->Buffer.SampleRate > 0) ? sourceData : nullptr;
			else
				return sourceData;
		}

		f32 GetSourceBaseVolume(SourceHandle source)
		{
			SourceData* sourceData = TryGetSourceData(source, GetSourceDataParam::None);
			return (sourceData != nullptr) ? sourceData->BaseVolume.load() : 1.0f;
		}

		void SetSourceBaseVolume(SourceHandle source, f32 value)
		{
			SourceData* sourceData = TryGetSourceData(source, GetSourceDataParam::None);
			if (sourceData != nullptr)
				sourceData->BaseVolume.store(value);
		}

		std::string_view GetSourceName(SourceHandle source)
		{
			SourceData* sourceData = TryGetSourceData(source, GetSourceDataParam::None);
			return (sourceData != nullptr) ? FixedBufferStringView(sourceData->Name) : "";
		}

		void SetSourceName(SourceHandle source, std::string_view newName)
		{
			SourceData* sourceData = TryGetSourceData(source, GetSourceDataParam::None);
			if (sourceData != nullptr)
				CopyStringViewIntoFixedBuffer(sourceData->Name, newName);
		}

		void CallbackClearOutPreviousCallBuffer(i16* outputBuffer, const size_t sampleCount)
		{
			std::fill(outputBuffer, outputBuffer + sampleCount, 0);
		}

		f32 SampleVolumeMapAt(const i64 startFrame, const i64 endFrame, const f32 startVolume, const f32 endVolume, const i64 frame)
		{
			if (frame <= startFrame)
				return startVolume;

			if (frame >= endFrame)
				return endVolume;

			const f64 delta = static_cast<f64>(frame - startFrame) / static_cast<f64>(endFrame - startFrame);
			const f32 lerpVolume = (startVolume + (endVolume - startVolume) * static_cast<f32>(delta));

			return lerpVolume;
		}

		void CallbackApplyVoiceVolumeAndMixTempBufferIntoOutput(i16* outputBuffer, const i64 frameCount, const VoiceData& voiceData, const u32 sampleRate)
		{
			const f32 voiceVolume = voiceData.Volume * GetSourceBaseVolume(voiceData.Source);
			const f32 startVolume = voiceData.VolumeMap.StartVolume;
			const f32 endVolume = voiceData.VolumeMap.EndVolume;

			if (startVolume == endVolume)
			{
				for (i64 i = 0; i < (frameCount * OutputChannelCount); i++)
					outputBuffer[i] = MixSamplesI16_Clamped(outputBuffer[i], ScaleSampleI16Linear_AsI32(TempOutputBuffer[i], voiceVolume));
			}
			else
			{
				const i64 volumeMapStartFrame = voiceData.VolumeMap.StartFrame;
				const i64 volumeMapEndFrame = voiceData.VolumeMap.EndFrame;

				if (voiceData.Flags & VoiceFlags_VariablePlaybackSpeed)
				{
					const Time frameDuration = FramesToTime(1, sampleRate) * voiceData.PlaybackSpeed;
					const Time bufferDuration = Time::FromSec(frameDuration.ToSec() * frameCount);
					const Time voiceStartTime = Time::FromSec(voiceData.TimePositionSec) - bufferDuration;

					for (i64 f = 0; f < frameCount; f++)
					{
						const Time frameTime = Time::FromSec(voiceStartTime.ToSec() + (f * frameDuration.ToSec()));
						const f32 frameVolume = SampleVolumeMapAt(volumeMapStartFrame, volumeMapEndFrame, startVolume, endVolume, TimeToFrames(frameTime, sampleRate)) * voiceVolume;

						for (u32 c = 0; c < OutputChannelCount; c++)
						{
							const i64 sampleIndex = (f * OutputChannelCount) + c;
							outputBuffer[sampleIndex] = MixSamplesI16_Clamped(outputBuffer[sampleIndex], ScaleSampleI16Linear_AsI32(TempOutputBuffer[sampleIndex], frameVolume));
						}
					}
				}
				else
				{
					const i64 voiceStartFrame = (voiceData.FramePosition - frameCount);

					for (i64 f = 0; f < frameCount; f++)
					{
						const f32 frameVolume = SampleVolumeMapAt(volumeMapStartFrame, volumeMapEndFrame, startVolume, endVolume, voiceStartFrame + f) * voiceVolume;
						for (u32 c = 0; c < OutputChannelCount; c++)
						{
							const i64 sampleIndex = (f * OutputChannelCount) + c;
							outputBuffer[sampleIndex] = MixSamplesI16_Clamped(outputBuffer[sampleIndex], ScaleSampleI16Linear_AsI32(TempOutputBuffer[sampleIndex], frameVolume));
						}
					}
				}
			}
		}

		void CallbackProcessVoices(i16* outputBuffer, const u32 bufferFrameCount, const u32 bufferSampleCount)
		{
			const auto lock = std::scoped_lock(VoiceRenderMutex);

			for (size_t voiceIndex = 0; voiceIndex < VoicePool.size(); voiceIndex++)
			{
				VoiceData& voiceData = VoicePool[voiceIndex];
				if (!(voiceData.Flags & VoiceFlags_Alive))
					continue;

				// TODO: Handle sample rate mismatch (by always setting variable playback speed?)
				SourceData* sourceData = TryGetSourceData(voiceData.Source, GetSourceDataParam::ValidateBuffer);

				const b8 variablePlaybackSpeed = (voiceData.Flags & VoiceFlags_VariablePlaybackSpeed);
				const b8 playPastEnd = (voiceData.Flags & VoiceFlags_PlayPastEnd);
				b8 hasReachedEnd = (sourceData == nullptr) ? false :
					(variablePlaybackSpeed ? (voiceData.TimePositionSec >= FramesToTime(sourceData->Buffer.FrameCount, sourceData->Buffer.SampleRate).ToSec()) :
					(voiceData.FramePosition >= sourceData->Buffer.FrameCount));

				if (sourceData == nullptr && (voiceData.Flags & VoiceFlags_RemoveOnEnd))
					hasReachedEnd = true;

				if (voiceData.SmoothTime.RequestUpdate.exchange(false))
				{
					voiceData.SmoothTime.BaseCPUTimeTicks = CPUTime::GetNow().Ticks;
					voiceData.SmoothTime.BaseVoiceTimeSec =
						variablePlaybackSpeed ? voiceData.TimePositionSec.load() :
						FramesToTime(voiceData.FramePosition, (sourceData != nullptr) ? sourceData->Buffer.SampleRate : OutputSampleRate).ToSec();
				}

				if (voiceData.Flags & VoiceFlags_Playing)
				{
					if (variablePlaybackSpeed)
						CallbackProcessVariableSpeedVoiceSamples(outputBuffer, bufferFrameCount, playPastEnd, hasReachedEnd, voiceData, sourceData);
					else
						CallbackProcessNormalSpeedVoiceSamples(outputBuffer, bufferFrameCount, playPastEnd, hasReachedEnd, voiceData, sourceData);
				}

				if (hasReachedEnd)
				{
					if (!playPastEnd && (voiceData.Flags & VoiceFlags_RemoveOnEnd))
					{
						voiceData.Flags = VoiceFlags_Dead;
						continue;
					}
					else if (voiceData.Flags & VoiceFlags_PauseOnEnd)
					{
						voiceData.Flags &= ~VoiceFlags_Playing;
						continue;
					}
				}
			}

			TotalRenderedFrames += bufferFrameCount;
		}

		void CallbackProcessNormalSpeedVoiceSamples(i16* outputBuffer, const u32 bufferFrameCount, const b8 playPastEnd, const b8 hasReachedEnd, VoiceData& voiceData, SourceData* sourceData)
		{
			if (sourceData == nullptr)
			{
				voiceData.FramePosition += bufferFrameCount;
				return;
			}

			i64 framesRead = 0;
			if (sourceData->Buffer.ChannelCount != 0 && sourceData->Buffer.ChannelCount != OutputChannelCount)
				framesRead = ChannelMixer.MixChannels(sourceData->Buffer, TempOutputBuffer.data(), voiceData.FramePosition, bufferFrameCount);
			else
				framesRead = sourceData->Buffer.ReadAtOrFillSilence(voiceData.FramePosition, bufferFrameCount, TempOutputBuffer.data());

			voiceData.FramePosition += framesRead;
			if (hasReachedEnd && !playPastEnd)
				voiceData.FramePosition = (voiceData.Flags & VoiceFlags_Looping) ? 0 : sourceData->Buffer.FrameCount;

			CallbackApplyVoiceVolumeAndMixTempBufferIntoOutput(outputBuffer, framesRead, voiceData, sourceData->Buffer.SampleRate);
		}

		void CallbackProcessVariableSpeedVoiceSamples(i16* outputBuffer, const u32 bufferFrameCount, const b8 playPastEnd, const b8 hasReachedEnd, VoiceData& voiceData, SourceData* sourceData)
		{
			const u32 sampleRate = (sourceData != nullptr) ? sourceData->Buffer.SampleRate : OutputSampleRate;
			const f64 bufferDurationSec = (FramesToTime(bufferFrameCount, sampleRate).ToSec() * voiceData.PlaybackSpeed);

			const i16* rawSamples = (sourceData != nullptr) ? sourceData->Buffer.InterleavedSamples.get() : nullptr;

			if (sourceData == nullptr || rawSamples == nullptr)
			{
				voiceData.TimePositionSec = voiceData.TimePositionSec + bufferDurationSec;
				return;
			}

			const f64 sampleDurationSec = (1.0 / static_cast<i64>(sampleRate)) * voiceData.PlaybackSpeed;
			const i64 framesRead = static_cast<i64>(Round(bufferDurationSec / sampleDurationSec));

			const size_t providerSampleCount = sourceData->Buffer.SampleCount();
			const u32 providerChannelCount = sourceData->Buffer.ChannelCount;

			const f64 sampleRateF64 = static_cast<f64>(sampleRate);
			const f64 voiceStartTimeSec = voiceData.TimePositionSec;

			if (providerChannelCount != OutputChannelCount)
			{
				i16* mixBuffer = ChannelMixer.GetMixSampleBufferWithMinSize(framesRead * providerChannelCount);

				for (i64 f = 0; f < framesRead; f++)
				{
					const f64 frameTimeSec = voiceStartTimeSec + (f * sampleDurationSec);
					for (u32 c = 0; c < providerChannelCount; c++)
						mixBuffer[(f * providerChannelCount) + c] = LinearSampleAtTimeOrZero<i16>(frameTimeSec, c, rawSamples, providerSampleCount, sampleRateF64, providerChannelCount);
				}

				ChannelMixer.MixChannels(providerChannelCount, mixBuffer, framesRead, TempOutputBuffer.data(), 0, framesRead);
			}
			else
			{
				for (i64 f = 0; f < framesRead; f++)
				{
					const f64 frameTimeSec = voiceStartTimeSec + (f * sampleDurationSec);
					for (u32 c = 0; c < OutputChannelCount; c++)
						TempOutputBuffer[(f * OutputChannelCount) + c] = LinearSampleAtTimeOrZero<i16>(frameTimeSec, c, rawSamples, providerSampleCount, sampleRateF64, OutputChannelCount);
				}
			}

			voiceData.TimePositionSec = voiceData.TimePositionSec + bufferDurationSec;
			if (hasReachedEnd && !playPastEnd)
				voiceData.TimePositionSec = (voiceData.Flags & VoiceFlags_Looping) ? 0.0 : FramesToTime(sourceData->Buffer.FrameCount, sampleRate).ToSec();

			CallbackApplyVoiceVolumeAndMixTempBufferIntoOutput(outputBuffer, framesRead, voiceData, sampleRate);
		}

		void CallbackAdjustBufferMasterVolume(i16* outputBuffer, const size_t sampleCount)
		{
			const f32 masterVolume = MasterVolume.load();
			for (size_t i = 0; i < sampleCount; i++)
				outputBuffer[i] = ScaleSampleI16Linear_Clamped(outputBuffer[i], masterVolume);
		}

		void CallbackUpdateLastPlayedSamplesRingBuffer(i16* outputBuffer, const size_t frameCount)
		{
			for (size_t f = 0; f < frameCount; f++)
			{
				for (u32 c = 0; c < OutputChannelCount; c++)
					LastPlayedSamplesRingBuffer[c][LastPlayedSamplesRingIndex] = outputBuffer[(f * OutputChannelCount) + c];

				if (LastPlayedSamplesRingIndex++ >= (LastPlayedSamplesRingBuffer[0].size() - 1))
					LastPlayedSamplesRingIndex = 0;
			}
		}

		void CallbackUpdateCallbackDurationRingBuffer(const Time duration)
		{
			CallbackDurationsRingBuffer[CallbackDurationRingIndex] = duration;
			if (CallbackDurationRingIndex++ >= (CallbackDurationsRingBuffer.size() - 1))
				CallbackDurationRingIndex = 0;
		}

		void RequestUpdateSmoothTimeForAllAliveVoices()
		{
			for (VoiceData& voiceData : VoicePool)
			{
				if (voiceData.Flags & VoiceFlags_Alive)
					voiceData.SmoothTime.RequestUpdate = true;
			}
		}

		void OnOpenStream()
		{
			RequestUpdateSmoothTimeForAllAliveVoices();
		}

		void OnCloseStream()
		{
			RequestUpdateSmoothTimeForAllAliveVoices();
		}

		void RenderAudioCallback(i16* outputBuffer, const u32 bufferFrameCountTarget, const u32 bufferChannelCount)
		{
			auto stopwatch = CPUStopwatch::StartNew();

			const u32 bufferFrameCount = Min<u32>(bufferFrameCountTarget, static_cast<u32>(MaxBufferFrameCount));
			const u32 bufferSampleCount = (bufferFrameCount * OutputChannelCount);
			assert(bufferFrameCountTarget <= MaxBufferFrameCount);

			CurrentBufferFrameSize = bufferFrameCountTarget;
			CallbackStreamTime = StreamTimeStopwatch.GetElapsed();
			CallbackFrequency = (CallbackStreamTime - LastCallbackStreamTime);
			LastCallbackStreamTime = CallbackStreamTime;

			CallbackClearOutPreviousCallBuffer(outputBuffer, bufferSampleCount);
			CallbackProcessVoices(outputBuffer, bufferFrameCount, bufferSampleCount);
			CallbackAdjustBufferMasterVolume(outputBuffer, bufferSampleCount);
			CallbackUpdateLastPlayedSamplesRingBuffer(outputBuffer, bufferFrameCount);
			CallbackUpdateCallbackDurationRingBuffer(stopwatch.Stop());
		}
	};

	AudioEngine::AudioEngine()
	{
		assert(this == &Engine && "Accidental AudioEngine copy (?)");
	}

	AudioEngine::~AudioEngine()
	{
		assert(impl == nullptr && "Forgot to call ApplicationShutdown() (?)");
		impl = nullptr;
	}

	void AudioEngine::ApplicationStartup()
	{
		assert(impl == nullptr && "ApplicationStartup() has already been called (?)");
		impl = std::make_unique<Impl>();

		SetBackend(Backend::Default);
		impl->ChannelMixer.TargetChannels = OutputChannelCount;
		impl->ChannelMixer.MixingBehavior = ChannelMixingBehavior::Combine;
		impl->ChannelMixer.MixBuffer.reserve(MaxBufferFrameCount * OutputChannelCount);
	}

	void AudioEngine::ApplicationShutdown()
	{
		if (impl != nullptr)
			StopCloseStream();
		impl = nullptr;
	}

	void AudioEngine::OpenStartStream()
	{
		if (impl->IsStreamOpenRunning)
			return;

		BackendStreamParam streamParam = {};
		streamParam.SampleRate = OutputSampleRate;
		streamParam.ChannelCount = OutputChannelCount;
		streamParam.DesiredFrameCount = impl->CurrentBufferFrameSize;
		streamParam.ShareMode = (impl->CurrentBackendType == Backend::WASAPI_Exclusive) ? StreamShareMode::Exclusive : StreamShareMode::Shared;

		if (impl->CurrentBackend == nullptr)
			impl->CurrentBackend = CreateBackendInterface(impl->CurrentBackendType);

		if (impl->CurrentBackend == nullptr)
			return;

		impl->OnOpenStream();

		const b8 openStreamSuccess = impl->CurrentBackend->OpenStartStream(streamParam, [this](i16* outputBuffer, const u32 bufferFrameCount, const u32 bufferChannelCount)
		{
			impl->RenderAudioCallback(outputBuffer, bufferFrameCount, bufferChannelCount);
		});

		if (openStreamSuccess)
			impl->StreamTimeStopwatch.Restart();

		impl->IsStreamOpenRunning = openStreamSuccess;
	}

	void AudioEngine::StopCloseStream()
	{
		if (!impl->IsStreamOpenRunning)
			return;

		if (impl->CurrentBackend != nullptr)
			impl->CurrentBackend->StopCloseStream();

		impl->OnCloseStream();
		impl->StreamTimeStopwatch.Stop();

		impl->IsStreamOpenRunning = false;
	}

	void AudioEngine::EnsureStreamRunning()
	{
		if (GetIsStreamOpenRunning())
			return;

		OpenStartStream();

		const b8 exclusiveAndFailedToStart = (impl->CurrentBackendType == Backend::WASAPI_Exclusive && !GetIsStreamOpenRunning());
		if (exclusiveAndFailedToStart)
		{
			// NOTE: Because *any* audio is probably always better than *no* audio
			SetBackend(Backend::WASAPI_Shared);
			OpenStartStream();
		}
	}

	std::future<SourceHandle> AudioEngine::LoadSourceFromFileAsync(std::string_view filePath)
	{
		return std::async(std::launch::async, [this, pathCopy = std::string(filePath)] { return LoadSourceFromFileSync(pathCopy); });
	}

	SourceHandle AudioEngine::LoadSourceFromFileSync(std::string_view filePath)
	{
		auto[fileContent, fileSize] = File::ReadAllBytes(filePath);
		return LoadSourceFromFileContentSync(Path::GetFileName(filePath), fileContent.get(), fileSize);
	}

	SourceHandle AudioEngine::LoadSourceFromFileContentSync(std::string_view fileName, const void* fileContent, size_t fileSize)
	{
		PCMSampleBuffer sampleBuffer;
		if (DecodeEntireFile(fileName, fileContent, fileSize, sampleBuffer) == DecodeFileResult::Sadge)
			return SourceHandle::Invalid;

		return LoadSourceFromBufferMove(fileName, std::move(sampleBuffer));
	}

	SourceHandle AudioEngine::LoadSourceFromBufferMove(std::string_view sourceName, PCMSampleBuffer bufferToMove)
	{
		const auto lock = std::scoped_lock(impl->VoiceRenderMutex);
		for (HandleBaseType index = 0; index < static_cast<HandleBaseType>(impl->LoadedSources.size()); index++)
		{
			const SourceHandle handleIt = IndexToSourceHandle(index);
			SourceData& sourceData = impl->LoadedSources[index];
			if (sourceData.SlotUsed)
				continue;

			sourceData.SlotUsed = true;
			sourceData.Buffer = std::move(bufferToMove);
			sourceData.BaseVolume = 1.0f;
			CopyStringViewIntoFixedBuffer(sourceData.Name, sourceName);

			return static_cast<SourceHandle>(index);
		}

#if PEEPO_DEBUG
		assert(!"Consider increasing MaxLoadedSources");
#endif

		return SourceHandle::Invalid;
	}

	void AudioEngine::UnloadSource(SourceHandle source)
	{
		if (source == SourceHandle::Invalid)
			return;

		const auto lock = std::scoped_lock(impl->VoiceRenderMutex);
		SourceData* sourceData = impl->TryGetSourceData(source, Impl::GetSourceDataParam::None);
		if (sourceData == nullptr)
			return;

		sourceData->SlotUsed = false;
		for (VoiceData& voice : impl->VoicePool)
		{
			if ((voice.Flags & VoiceFlags_Alive) && voice.Source == source)
				voice.Source = SourceHandle::Invalid;
		}
	}

	const PCMSampleBuffer* AudioEngine::GetSourceSampleBufferView(SourceHandle source)
	{
		const SourceData* data = impl->TryGetSourceData(source, Impl::GetSourceDataParam::None);
		return (data != nullptr) ? &data->Buffer : nullptr;
	}

	f32 AudioEngine::GetSourceBaseVolume(SourceHandle source)
	{
		return impl->GetSourceBaseVolume(source);
	}

	void AudioEngine::SetSourceBaseVolume(SourceHandle source, f32 value)
	{
		return impl->SetSourceBaseVolume(source, value);
	}

	std::string_view AudioEngine::GetSourceName(SourceHandle source)
	{
		return impl->GetSourceName(source);
	}

	void AudioEngine::SetSourceName(SourceHandle source, std::string_view newName)
	{
		return impl->SetSourceName(source, newName);
	}

	VoiceHandle AudioEngine::AddVoice(SourceHandle source, std::string_view name, b8 playing, f32 volume, b8 playPastEnd)
	{
		const auto lock = std::scoped_lock(impl->VoiceRenderMutex);

		for (size_t i = 0; i < impl->VoicePool.size(); i++)
		{
			VoiceData& voiceToUpdate = impl->VoicePool[i];
			if (voiceToUpdate.Flags & VoiceFlags_Alive)
				continue;

			voiceToUpdate.Flags = VoiceFlags_Alive;
			if (playing) voiceToUpdate.Flags |= VoiceFlags_Playing;
			if (playPastEnd) voiceToUpdate.Flags |= VoiceFlags_PlayPastEnd;

			voiceToUpdate.Source = source;
			voiceToUpdate.Volume = volume;
			voiceToUpdate.FramePosition = 0;
			voiceToUpdate.VolumeMap.StartVolume = 0.0f;
			voiceToUpdate.VolumeMap.EndVolume = 0.0f;
			CopyStringViewIntoFixedBuffer(voiceToUpdate.Name, name);

			return static_cast<VoiceHandle>(i);
		}

#if PEEPO_DEBUG
		assert(!"Consider increasing MaxSimultaneousVoices");
#endif

		return VoiceHandle::Invalid;
	}

	void AudioEngine::RemoveVoice(VoiceHandle voice)
	{
		if (voice == VoiceHandle::Invalid)
			return;

		// TODO: Think these locks through again
		const auto lock = std::scoped_lock(impl->VoiceRenderMutex);

		VoiceData* voiceData = impl->TryGetVoiceData(voice);
		if (voiceData != nullptr)
			voiceData->Flags = VoiceFlags_Dead;
	}

	void AudioEngine::PlayOneShotSound(SourceHandle source, std::string_view name, f32 volume)
	{
		if (source == SourceHandle::Invalid)
			return;

		const auto lock = std::scoped_lock(impl->VoiceRenderMutex);

		for (VoiceData& voiceToUpdate : impl->VoicePool)
		{
			if (voiceToUpdate.Flags & VoiceFlags_Alive)
				continue;

			voiceToUpdate.Flags = VoiceFlags_Alive | VoiceFlags_Playing | VoiceFlags_RemoveOnEnd;
			voiceToUpdate.Source = source;
			voiceToUpdate.Volume = volume;
			voiceToUpdate.FramePosition = 0;
			voiceToUpdate.VolumeMap.StartVolume = 0.0f;
			voiceToUpdate.VolumeMap.EndVolume = 0.0f;
			CopyStringViewIntoFixedBuffer(voiceToUpdate.Name, name);
			return;
		}
	}

	Backend AudioEngine::GetBackend() const
	{
		return impl->CurrentBackendType;
	}

	void AudioEngine::SetBackend(Backend value)
	{
		if (value == impl->CurrentBackendType)
			return;

		impl->CurrentBackendType = value;
		if (impl->IsStreamOpenRunning)
		{
			StopCloseStream();
			impl->CurrentBackend = CreateBackendInterface(value);
			OpenStartStream();
		}
		else
		{
			impl->CurrentBackend = CreateBackendInterface(value);
		}
	}

	b8 AudioEngine::GetIsStreamOpenRunning() const
	{
		return impl->IsStreamOpenRunning;
	}

	b8 AudioEngine::GetAllVoicesAreIdle() const
	{
		if (!impl->IsStreamOpenRunning)
			return true;

		for (size_t i = 0; i < impl->VoicePool.size(); i++)
		{
			const VoiceData& voice = impl->VoicePool[i];
			if ((voice.Flags & VoiceFlags_Alive) && (voice.Flags & VoiceFlags_Playing))
				return false;
		}

		return true;
	}

	f32 AudioEngine::GetMasterVolume() const
	{
		return impl->MasterVolume;
	}

	void AudioEngine::SetMasterVolume(f32 value)
	{
		impl->MasterVolume = value;
	}

	u32 AudioEngine::GetBufferFrameSize() const
	{
		return impl->CurrentBufferFrameSize;
	}

	void AudioEngine::SetBufferFrameSize(u32 bufferFrameCount)
	{
		bufferFrameCount = Clamp(bufferFrameCount, MinBufferFrameCount, MaxBufferFrameCount);

		if (bufferFrameCount == impl->CurrentBufferFrameSize)
			return;

		impl->CurrentBufferFrameSize = bufferFrameCount;

		if (GetIsStreamOpenRunning())
		{
			StopCloseStream();
			OpenStartStream();
		}
	}

	Time AudioEngine::GetCallbackFrequency() const
	{
		return impl->CallbackFrequency;
	}

	ChannelMixer& AudioEngine::GetChannelMixer()
	{
		return impl->ChannelMixer;
	}

	i64 AudioEngine::DebugGetTotalRenderedFrames() const
	{
		return impl->TotalRenderedFrames;
	}

	AudioEngine::DebugVoicesArray AudioEngine::DebugGetAllActiveVoices()
	{
		DebugVoicesArray out = {};
		for (size_t i = 0; i < impl->VoicePool.size(); i++)
		{
			const VoiceData& voice = impl->VoicePool[i];
			if (voice.Flags & VoiceFlags_Alive)
				out.Slots[out.Count++] = static_cast<VoiceHandle>(i);
		}
		return out;
	}

	AudioEngine::DebugSourcesArray AudioEngine::DebugGetAllLoadedSources()
	{
		DebugSourcesArray out = {};
		for (size_t i = 0; i < impl->LoadedSources.size(); i++)
		{
			const SourceData& source = impl->LoadedSources[i];
			if (source.SlotUsed)
				out.Slots[out.Count++] = static_cast<SourceHandle>(i);
		}
		return out;
	}

	i32 AudioEngine::DebugGetSourceVoiceInstanceCount(SourceHandle source)
	{
		if (impl->TryGetSourceData(source, Impl::GetSourceDataParam::None) == nullptr)
			return 0;

		i32 instanceCount = 0;
		for (size_t i = 0; i < impl->VoicePool.size(); i++)
		{
			const VoiceData& voice = impl->VoicePool[i];
			if (voice.Flags & VoiceFlags_Alive && voice.Source == source)
				instanceCount++;
		}
		return instanceCount;
	}

	std::array<Time, AudioEngine::CallbackDurationRingBufferSize> AudioEngine::DebugGetRenderPerformanceHistory()
	{
		return impl->CallbackDurationsRingBuffer;
	}

	std::array<std::array<i16, AudioEngine::LastPlayedSamplesRingBufferFrameCount>, AudioEngine::OutputChannelCount> AudioEngine::DebugGetLastPlayedSamples()
	{
		return impl->LastPlayedSamplesRingBuffer;
	}

	b8 Voice::IsValid() const
	{
		auto& impl = Engine.impl;
		return (impl->TryGetVoiceData(Handle) != nullptr);
	}

	f32 Voice::GetVolume() const
	{
		auto& impl = Engine.impl;

		if (VoiceData* voice = impl->TryGetVoiceData(Handle); voice != nullptr)
			return voice->Volume;
		return 0.0f;
	}

	void Voice::SetVolume(f32 value)
	{
		auto& impl = Engine.impl;

		if (VoiceData* voice = impl->TryGetVoiceData(Handle); voice != nullptr)
			voice->Volume = value;
	}

	f32 Voice::GetPlaybackSpeed() const
	{
		auto& impl = Engine.impl;

		if (const VoiceData* voice = impl->TryGetVoiceData(Handle); voice != nullptr)
		{
			if (voice->Flags & VoiceFlags_VariablePlaybackSpeed)
				return voice->PlaybackSpeed;
		}
		return 1.0f;
	}

	void Voice::SetPlaybackSpeed(f32 value)
	{
		auto& impl = Engine.impl;

		if (VoiceData* voice = impl->TryGetVoiceData(Handle); voice != nullptr)
		{
			const SourceData* source = impl->TryGetSourceData(voice->Source, AudioEngine::Impl::GetSourceDataParam::ValidateBuffer);
			const u32 sampleRate = (source != nullptr) ? source->Buffer.SampleRate : AudioEngine::OutputSampleRate;

			if (ApproxmiatelySame(value, 1.0f))
			{
				if (voice->Flags & VoiceFlags_VariablePlaybackSpeed)
					voice->FramePosition = TimeToFrames(Time::FromSec(voice->TimePositionSec), sampleRate);

				voice->Flags &= ~VoiceFlags_VariablePlaybackSpeed;
			}
			else
			{
				if (!(voice->Flags & VoiceFlags_VariablePlaybackSpeed))
					voice->TimePositionSec = FramesToTime(voice->FramePosition, sampleRate).ToSec();

				voice->Flags |= VoiceFlags_VariablePlaybackSpeed;
			}

			voice->PlaybackSpeed = value;
			voice->SmoothTime.RequestUpdate = true;
		}
	}

	Time Voice::GetPosition() const
	{
		auto& impl = Engine.impl;

		if (VoiceData* voice = impl->TryGetVoiceData(Handle); voice != nullptr)
		{
			const SourceData* source = impl->TryGetSourceData(voice->Source, AudioEngine::Impl::GetSourceDataParam::ValidateBuffer);
			const u32 sampleRate = (source != nullptr) ? source->Buffer.SampleRate : AudioEngine::OutputSampleRate;

			if (voice->Flags & VoiceFlags_VariablePlaybackSpeed)
				return Time::FromSec(voice->TimePositionSec);
			else
				return FramesToTime(voice->FramePosition, sampleRate);
		}
		return Time::Zero();
	}

	Time Voice::GetPositionSmooth() const
	{
		auto& impl = Engine.impl;

		if (const VoiceData* voice = impl->TryGetVoiceData(Handle); voice != nullptr)
		{
			if ((voice->Flags & VoiceFlags_Playing) && !voice->SmoothTime.RequestUpdate)
			{
				const f64 playbackSpeed = (voice->Flags & VoiceFlags_VariablePlaybackSpeed) ? voice->PlaybackSpeed.load() : 1.0;

				const CPUTime systemTimeNow = CPUTime::GetNow();
				const CPUTime systemTimeThen = CPUTime(voice->SmoothTime.BaseCPUTimeTicks);
				const Time baseVoiceTime = Time::FromSec(voice->SmoothTime.BaseVoiceTimeSec);

				const Time timeSincePlaybackStart = CPUTime::DeltaTime(systemTimeThen, systemTimeNow) * playbackSpeed;
				const Time adjustedVoiceTime = timeSincePlaybackStart + baseVoiceTime;

				return adjustedVoiceTime;
			}
			else
			{
				return GetPosition();
			}
		}
		return Time::Zero();
	}

	void Voice::SetPosition(Time value)
	{
		auto& impl = Engine.impl;

		if (VoiceData* voice = impl->TryGetVoiceData(Handle); voice != nullptr)
		{
			const SourceData* source = impl->TryGetSourceData(voice->Source, AudioEngine::Impl::GetSourceDataParam::ValidateBuffer);
			const u32 sampleRate = (source != nullptr) ? source->Buffer.SampleRate : AudioEngine::OutputSampleRate;
			voice->FramePosition = TimeToFrames(value, sampleRate);
			voice->TimePositionSec = value.ToSec();
			voice->SmoothTime.RequestUpdate = true;
		}
	}

	SourceHandle Voice::GetSource() const
	{
		auto& impl = Engine.impl;

		if (const VoiceData* voice = impl->TryGetVoiceData(Handle); voice != nullptr)
			return voice->Source;
		return SourceHandle::Invalid;
	}

	void Voice::SetSource(SourceHandle value)
	{
		auto& impl = Engine.impl;

		if (VoiceData* voice = impl->TryGetVoiceData(Handle); voice != nullptr)
			voice->Source = value;
	}

	Time Voice::GetSourceDuration() const
	{
		auto& impl = Engine.impl;

		if (const VoiceData* voice = impl->TryGetVoiceData(Handle); voice != nullptr)
		{
			if (const SourceData* source = impl->TryGetSourceData(voice->Source, AudioEngine::Impl::GetSourceDataParam::ValidateBuffer); source != nullptr)
				return FramesToTime(source->Buffer.FrameCount, source->Buffer.SampleRate);
		}
		return Time::Zero();
	}

	b8 Voice::GetIsPlaying() const
	{
		return GetInternalFlag(VoiceFlags_Playing);
	}

	void Voice::SetIsPlaying(b8 value)
	{
		// TODO: Allow manual render thread locking from outside code to then be used for playback sounds
		//		 This should then become lockfree again!

		// HACK: Prevent randomly "stacking" playback button sounds... need to find a better solution, definitely don't want any locks like this
		const auto lock = std::scoped_lock(Engine.impl->VoiceRenderMutex);

		SetInternalFlag(VoiceFlags_Playing, value);
	}

	b8 Voice::GetIsLooping() const
	{
		return GetInternalFlag(VoiceFlags_Looping);
	}

	void Voice::SetIsLooping(b8 value)
	{
		SetInternalFlag(VoiceFlags_Looping, value);
	}

	b8 Voice::GetPlayPastEnd() const
	{
		return GetInternalFlag(VoiceFlags_PlayPastEnd);
	}

	void Voice::SetPlayPastEnd(b8 value)
	{
		SetInternalFlag(VoiceFlags_PlayPastEnd, value);
	}

	b8 Voice::GetRemoveOnEnd() const
	{
		return GetInternalFlag(VoiceFlags_RemoveOnEnd);
	}

	void Voice::SetRemoveOnEnd(b8 value)
	{
		SetInternalFlag(VoiceFlags_RemoveOnEnd, value);
	}

	b8 Voice::GetPauseOnEnd() const
	{
		return GetInternalFlag(VoiceFlags_PauseOnEnd);
	}

	void Voice::SetPauseOnEnd(b8 value)
	{
		SetInternalFlag(VoiceFlags_PauseOnEnd, value);
	}

	std::string_view Voice::GetName() const
	{
		auto& impl = Engine.impl;

		if (const VoiceData* voice = impl->TryGetVoiceData(Handle); voice != nullptr)
			return FixedBufferStringView(voice->Name);
		return "VoiceHandle::Invalid";
	}

	void Voice::ResetVolumeMap()
	{
		if (VoiceData* voice = Engine.impl->TryGetVoiceData(Handle); voice != nullptr)
		{
			voice->VolumeMap.StartFrame = 0;
			voice->VolumeMap.EndFrame = 0;
			voice->VolumeMap.StartVolume = 0.0f;
			voice->VolumeMap.EndVolume = 0.0f;
		}
	}

	void Voice::SetVolumeMap(Time startTime, Time endTime, f32 startVolume, f32 endVolume)
	{
		auto& impl = Engine.impl;

		if (VoiceData* voice = impl->TryGetVoiceData(Handle); voice != nullptr)
		{
			const SourceData* source = impl->TryGetSourceData(voice->Source, AudioEngine::Impl::GetSourceDataParam::ValidateBuffer);
			const u32 sampleRate = (source != nullptr) ? source->Buffer.SampleRate : AudioEngine::OutputSampleRate;

			voice->VolumeMap.StartFrame = TimeToFrames(startTime, sampleRate);
			voice->VolumeMap.EndFrame = TimeToFrames(endTime, sampleRate);
			voice->VolumeMap.StartVolume = startVolume;
			voice->VolumeMap.EndVolume = endVolume;
		}
	}

	b8 Voice::GetInternalFlag(u16 flag) const
	{
		static_assert(sizeof(flag) == sizeof(VoiceFlags));
		const auto voiceFlag = static_cast<VoiceFlags>(flag);

		if (const VoiceData* voice = Engine.impl->TryGetVoiceData(Handle); voice != nullptr)
			return (voice->Flags & voiceFlag);
		return false;
	}

	void Voice::SetInternalFlag(u16 flag, b8 value)
	{
		static_assert(sizeof(flag) == sizeof(VoiceFlags));
		const auto voiceFlag = static_cast<VoiceFlags>(flag);

		if (VoiceData* voice = Engine.impl->TryGetVoiceData(Handle); voice != nullptr)
		{
			if (value)
				voice->Flags |= voiceFlag;
			else
				voice->Flags &= ~voiceFlag;

			if (flag & VoiceFlags_Playing)
				voice->SmoothTime.RequestUpdate = true;
		}
	}
}
