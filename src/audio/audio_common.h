#pragma once
#include "core_types.h"
#include <memory>
#include <vector>

namespace Audio
{
	constexpr f32 ConvertSampleI16ToF32(i16 v) { return static_cast<f32>(v) / static_cast<f32>(I16Max); }
	constexpr i16 ConvertSampleF32ToI16(f32 v) { return static_cast<i16>(v * static_cast<f32>(I16Max)); }

	constexpr Time FramesToTime(i64 frames, i64 sampleRate) { return Time::FromSeconds(static_cast<f64>(frames) / static_cast<f64>(sampleRate)); }
	constexpr Time FramesToTimeOrZero(i64 frames, i64 sampleRate) { return (sampleRate != 0) ? FramesToTime(frames, sampleRate) : Time::Zero(); }

	constexpr i64 TimeToFrames(Time time, i64 sampleRate) { return static_cast<i64>(time.TotalSeconds() * static_cast<f64>(sampleRate)); }
	constexpr i64 TimeToFramesOrZero(Time time, i64 sampleRate) { return (sampleRate != 0) ? TimeToFramesOrZero(time, sampleRate) : 0; }

	inline f32 ClampLinearVolume(f32 linear) { return Clamp(linear, 0.0f, 1.0f); }
	inline f32 DecibelToLinearVolume(f32 decibel) { return ::powf(10.0f, 0.05f * decibel); }
	inline f32 LinearVolumeToDecibel(f32 linear) { return 20.0f * ::log10f(linear); }
	inline f32 LinearVolumeToSquare(f32 linear) { return ::sqrtf(linear); }
	inline f32 SquareToLinearVolume(f32 power) { return (power * power); }

	constexpr i16 ClampSampleI16(i32 v) { return static_cast<i16>(Clamp<i32>(v, static_cast<i32>(I16Min + 1), static_cast<i32>(I16Max - 1))); }
	constexpr i16 MixSamplesI16_Clamped(i32 a, i32 b) { return ClampSampleI16(a + b); }

	// TODO: Should probably do all of the scaling as i32s directly without float conversions but oh well...
	constexpr i32 ScaleSampleI16Linear_AsI32(i32 v, f32 linear) { return static_cast<i32>(static_cast<f32>(v) * linear); }
	constexpr i16 ScaleSampleI16Linear_Clamped(i32 v, f32 linear) { return ClampSampleI16(ScaleSampleI16Linear_AsI32(v, linear)); }

	struct PCMSampleBuffer
	{
		u32 ChannelCount;
		u32 SampleRate;
		i64 FrameCount;
		std::unique_ptr<i16[]> InterleavedSamples;

		constexpr size_t SampleCount() const { return (FrameCount * ChannelCount); }
		constexpr size_t ByteSize() const { return (FrameCount * ChannelCount * sizeof(i16)); }
		i64 ReadAtOrFillSilence(i64 inFrameOffset, i64 inFrameCount, i16 outSamples[]) const;
	};

	enum class ChannelMixingBehavior : u8
	{
		Combine,
		IgnoreTrailing,
		IgnoreLeading,
		Count
	};

	constexpr const char* ChannelMixingBehaviorNames[EnumCount<ChannelMixingBehavior>] =
	{
		"Combine",
		"Ignore Trailing",
		"Ignore Leading",
	};

	// TODO: REFACTOR ALL OF THIS
	struct ChannelMixer
	{
		ChannelMixingBehavior MixingBehavior = ChannelMixingBehavior::Combine;
		u32 TargetChannels = 2;
		std::vector<i16> MixBuffer;

		i64 MixChannels(PCMSampleBuffer& buffer, i16 bufferToFill[], i64 frameOffset, i64 framesToRead);
		i64 MixChannels(u32 sourceChannels, i16 sampleSwapBuffer[], i64 framesRead, i16 bufferToFill[], i64 frameOffset, i64 framesToRead);

		inline i16* GetMixSampleBufferWithMinSize(size_t minSampleCount) { if (MixBuffer.size() < minSampleCount) MixBuffer.resize(minSampleCount); return MixBuffer.data(); }
	};

	inline i64 ChannelMixer::MixChannels(PCMSampleBuffer& buffer, i16 bufferToFill[], i64 frameOffset, i64 framesToRead)
	{
		const u32 sourceChannels = buffer.ChannelCount;
		i16* mixBuffer = GetMixSampleBufferWithMinSize(framesToRead * sourceChannels);

		const i64 framesRead = buffer.ReadAtOrFillSilence(frameOffset, framesToRead, mixBuffer);
		return MixChannels(sourceChannels, mixBuffer, framesRead, bufferToFill, frameOffset, framesToRead);
	}

	inline i64 ChannelMixer::MixChannels(u32 sourceChannels, i16 mixBuffer[], i64 framesRead, i16 bufferToFill[], i64 frameOffset, i64 framesToRead)
	{
		const i64 samplesRead = framesRead * sourceChannels;

		if (sourceChannels < TargetChannels)
		{
			// NOTE: Duplicate existing channel(s)
			size_t targetIndex = 0;
			for (i64 i = 0; i < samplesRead; i++)
			{
				for (size_t c = 0; c < TargetChannels; c++)
					bufferToFill[targetIndex++] = mixBuffer[i];
			}
		}
		else
		{
			if (sourceChannels < 4 || TargetChannels != 2)
			{
				// TODO: Implement a more generic solution (?)
				std::fill(bufferToFill, bufferToFill + (framesToRead * TargetChannels), 0);
				return framesToRead;
			}

			int swapBufferIndex = 0;

			switch (MixingBehavior)
			{
			case ChannelMixingBehavior::IgnoreTrailing:
			{
				for (i64 i = 0; i < framesRead * TargetChannels;)
				{
					bufferToFill[i++] = mixBuffer[swapBufferIndex + 0];
					bufferToFill[i++] = mixBuffer[swapBufferIndex + 1];
					swapBufferIndex += sourceChannels;
				}
				break;
			}
			case ChannelMixingBehavior::IgnoreLeading:
			{
				for (i64 i = 0; i < framesRead * TargetChannels;)
				{
					bufferToFill[i++] = mixBuffer[swapBufferIndex + 2];
					bufferToFill[i++] = mixBuffer[swapBufferIndex + 3];
					swapBufferIndex += sourceChannels;
				}
				break;
			}
			case ChannelMixingBehavior::Combine:
			{
				for (i64 i = 0; i < framesRead * TargetChannels;)
				{
					bufferToFill[i++] = MixSamplesI16_Clamped(mixBuffer[swapBufferIndex + 0], mixBuffer[swapBufferIndex + 2]);
					bufferToFill[i++] = MixSamplesI16_Clamped(mixBuffer[swapBufferIndex + 1], mixBuffer[swapBufferIndex + 3]);
					swapBufferIndex += sourceChannels;
				}
				break;
			}

			default:
				return 0;
			}
		}

		return framesRead;
	}

	constexpr f64 LerpF64(f64 start, f64 end, f64 t) { return ((1.0 - t) * start) + (t * end); }

	template <typename SampleType>
	constexpr SampleType SampleAtFrameIndexOrZero(i64 atFrame, u32 atChannel, const SampleType* sampels, size_t sampleCount, u32 channelCount)
	{
		const size_t sampleIndex = static_cast<size_t>((atFrame * channelCount) + atChannel);
		return (sampleIndex < sampleCount) ? sampels[sampleIndex] : static_cast<SampleType>(0);
	}

	// TODO: Switch to f32 instead (?)
	template <typename SampleType>
	constexpr SampleType LinearSampleAtTimeOrZero(f64 atSecond, u32 atChannel, const SampleType* samples, size_t sampleCount, f64 sampleRate, u32 channelCount)
	{
		const f64 frameFaction = (atSecond * sampleRate);
		const i64 startFrame = static_cast<i64>(frameFaction);
		const i64 endFrame = (startFrame + 1);

		const SampleType startValue = SampleAtFrameIndexOrZero<SampleType>(startFrame, atChannel, samples, sampleCount, channelCount);
		const SampleType endValue = SampleAtFrameIndexOrZero<SampleType>(endFrame, atChannel, samples, sampleCount, channelCount);
		const f64 inbetween = (frameFaction - static_cast<f64>(startFrame));

		static constexpr f64 maxSampleValueF64 = static_cast<f64>(std::numeric_limits<SampleType>::max());
		const f64 normalizedStart = static_cast<f64>(startValue / maxSampleValueF64);
		const f64 normalizedEnd = static_cast<f64>(endValue / maxSampleValueF64);

		const f64 normalizedResult = LerpF64(normalizedStart, normalizedEnd, inbetween);
		const f64 clampedResult = Clamp(normalizedResult, -1.0, 1.0);

		const SampleType sampleTypeResult = static_cast<SampleType>(clampedResult * maxSampleValueF64);
		return sampleTypeResult;
	}

	// NOTE: Low quallity linear resampling lacking a low pass filter, should however still be better than having sped up audio for now
	template <typename SampleType>
	void LinearlyResampleBuffer(std::unique_ptr<SampleType[]>& inOutSamples, size_t& inOutFrameCount, u32& inOutSampleRate, const u32 inChannelCount, const u32 targetSampleRate)
	{
		if (inOutSampleRate == targetSampleRate) { assert(false); return; }

		const SampleType* inSamples = inOutSamples.get();
		const size_t inSampleCount = (inOutFrameCount * inChannelCount);

		const f64 sourceRate = static_cast<f64>(inOutSampleRate);
		const f64 targetRate = static_cast<f64>(targetSampleRate);

		const size_t inFrameCount = (inSampleCount / inChannelCount);
		const size_t outFrameCount = static_cast<size_t>(inFrameCount * targetRate / sourceRate + 0.5);

		const size_t outSampleCount = (outFrameCount * inChannelCount);
		// auto outSamples = std::make_unique<SampleType[]>(outSampleCount);
		auto outSamples = std::unique_ptr<SampleType[]>(new SampleType[outSampleCount]);

		const f64 outFrameToSecond = (1.0 / targetRate);
		SampleType* outSampleWriteHead = outSamples.get();

		for (size_t frame = 0; frame < outFrameCount; frame++)
		{
			const f64 second = (static_cast<f64>(frame) * outFrameToSecond);

			for (u32 channel = 0; channel < inChannelCount; channel++)
				outSampleWriteHead[channel] = LinearSampleAtTimeOrZero<SampleType>(second, channel, inSamples, inSampleCount, sourceRate, inChannelCount);

			outSampleWriteHead += inChannelCount;
		}

		inOutSamples = std::move(outSamples);
		inOutFrameCount = outFrameCount;
		inOutSampleRate = targetSampleRate;
	}

	template <typename SampleType>
	void LinearlyResampleBuffer(std::unique_ptr<SampleType[]>& inOutSamples, i64& inOutFrameCount, u32& inOutSampleRate, const u32 inChannelCount, const u32 targetSampleRate)
	{
		size_t inOutFrameCountSizeT = static_cast<size_t>(inOutFrameCount);
		Audio::LinearlyResampleBuffer(inOutSamples, inOutFrameCountSizeT, inOutSampleRate, inChannelCount, targetSampleRate);
		inOutFrameCount = static_cast<i64>(inOutFrameCountSizeT);
	}
}
