#pragma once
#include "core_types.h"
#include "audio_common.h"

// TODO: Texture cache (create interface for uploading texture pixels to have a clean separation from the actual rendering?)

namespace Audio
{
	constexpr i16 AverageTwoI16SamplesTogether(i16 a, i16 b) { return static_cast<i16>((static_cast<i32>(a) + static_cast<i32>(b)) / 2); };

	struct WaveformMip
	{
		size_t PowerOfTwoSampleCount = {};
		Time TimePerSample = {};
		f64 SamplesPerSecond = {};
		std::vector<i16> AbsoluteSamples {};

		inline i16 SampleAtIndexOrZero(size_t sampleIndex) const
		{
			return (sampleIndex < AbsoluteSamples.size()) ? AbsoluteSamples[sampleIndex] : static_cast<i16>(0);
		}

		inline i16 LinearSampleAtTimeOrZero(Time time) const
		{
			const f32 sampleIndexF32 = static_cast<f32>(time.Seconds * SamplesPerSecond);
			const f32 sampleIndexFraction = sampleIndexF32 - static_cast<i32>(sampleIndexF32);

			const size_t sampleIndexLo = (sampleIndexF32 <= 0.0f) ? 0 : static_cast<size_t>(sampleIndexF32);
			const size_t sampleIndexHi = (sampleIndexLo + 1);

			const i16 sampleLo = SampleAtIndexOrZero(sampleIndexLo);
			const i16 sampleHi = SampleAtIndexOrZero(sampleIndexHi);

			const f32 normalizedSampleLo = static_cast<f32>(sampleLo / static_cast<f32>(I16Max));
			const f32 normalizedSampleHi = static_cast<f32>(sampleHi / static_cast<f32>(I16Max));
			const f32 normalizedSample = Lerp(normalizedSampleLo, normalizedSampleHi, sampleIndexFraction);

			return static_cast<i16>(normalizedSample * static_cast<f32>(I16Max));
		}

		f32 AverageNormalizedSampleInTimeRange(Time startTime, Time endTime) const
		{
			if (TimePerSample.Seconds <= 0.0) { assert(false); return 0.0f; }

			i32 sampleSum = 0, sampleCount = 0;
			for (Time timeIt = startTime; timeIt < endTime; timeIt += TimePerSample) { sampleSum += LinearSampleAtTimeOrZero(timeIt); sampleCount++; }

			const i32 sampleAverage = (sampleSum / Max(sampleCount, 1));
			return sampleAverage / static_cast<f32>(I16Max);
		}

		void Clear()
		{
			PowerOfTwoSampleCount = {};
			TimePerSample = {};
			SamplesPerSecond = {};
			AbsoluteSamples.clear();
		}
	};

	struct WaveformMipChain
	{
		static constexpr size_t MaxMipLevels = 24;
		static constexpr size_t MinMipSampleCount = 256;

		WaveformMip AllMips[MaxMipLevels] {};

		inline b8 IsEmpty() const
		{
			return AllMips[0].PowerOfTwoSampleCount == 0;
		}

		inline Time GetDuration() const
		{
			return Time::FromSec(static_cast<f64>(AllMips[0].AbsoluteSamples.size()) / AllMips[0].SamplesPerSecond);
		}

		inline i32 GetUsedMipCount() const
		{
			for (i32 i = 0; i < static_cast<i32>(MaxMipLevels); i++)
			{
				if (AllMips[i].PowerOfTwoSampleCount == 0)
					return i;
			}
			return static_cast<i32>(MaxMipLevels);
		}

		inline const WaveformMip& FindClosestMip(Time timePerPixel) const
		{
			const WaveformMip* closestMip = &AllMips[0];
			for (size_t i = 1; i < MaxMipLevels; i++)
			{
				if (AllMips[i].PowerOfTwoSampleCount == 0)
					break;
				if (Absolute((AllMips[i].TimePerSample - timePerPixel).Seconds < Absolute((closestMip->TimePerSample - timePerPixel).Seconds)))
					closestMip = &AllMips[i];
			}
			return *closestMip;
		}

		inline f32 GetAmplitudeAt(const WaveformMip& mip, Time time, Time timePerPixel) const
		{
			return mip.AverageNormalizedSampleInTimeRange(time, time + timePerPixel);
		}

		inline void GenerateEntireMipChainFromSampleBuffer(const PCMSampleBuffer& inSampleBuffer, u32 channelIndex, b8 includeFullSizeMip = false)
		{
			assert(inSampleBuffer.InterleavedSamples != nullptr && channelIndex < inSampleBuffer.ChannelCount);

			if (AllMips[0].PowerOfTwoSampleCount != 0)
				for (auto& mip : AllMips) mip.Clear();

			WaveformMip& baseMip = AllMips[0];
			baseMip.PowerOfTwoSampleCount = RoundUpToPowerOfTwo(static_cast<u32>(inSampleBuffer.FrameCount));
			baseMip.TimePerSample = Time::FromSec(1.0 / static_cast<f64>(inSampleBuffer.SampleRate));
			baseMip.SamplesPerSecond = static_cast<f64>(inSampleBuffer.SampleRate);

			if (includeFullSizeMip)
			{
				baseMip.AbsoluteSamples.resize(inSampleBuffer.FrameCount);
				for (size_t frameIndex = 0; frameIndex < static_cast<size_t>(inSampleBuffer.FrameCount); frameIndex++)
					baseMip.AbsoluteSamples[frameIndex] = Absolute(inSampleBuffer.InterleavedSamples[(frameIndex * inSampleBuffer.ChannelCount) + channelIndex]);
			}
			else // NOTE: No need to waste memory storing the full size mip if it won't even get sampled AND is already duplicated inside the source buffer
			{
				baseMip.PowerOfTwoSampleCount /= 2;
				baseMip.TimePerSample = baseMip.TimePerSample * 2.0;
				baseMip.SamplesPerSecond = baseMip.SamplesPerSecond / 2.0;
				baseMip.AbsoluteSamples.resize(baseMip.PowerOfTwoSampleCount);
				const size_t samplesToFill = ClampTop(baseMip.AbsoluteSamples.size(), static_cast<size_t>(inSampleBuffer.FrameCount / 2));
				for (size_t frameIndex = 0; frameIndex < samplesToFill; frameIndex++)
				{
					baseMip.AbsoluteSamples[frameIndex] = AverageTwoI16SamplesTogether(
						Absolute(inSampleBuffer.InterleavedSamples[(((frameIndex * 2 + 0) * inSampleBuffer.ChannelCount) + channelIndex)]),
						Absolute(inSampleBuffer.InterleavedSamples[(((frameIndex * 2 + 1) * inSampleBuffer.ChannelCount) + channelIndex)]));
				}
			}

			// NOTE: First loop (separated) to compute sample counts
			for (size_t i = 1; i < MaxMipLevels; i++)
			{
				const WaveformMip& parentMip = AllMips[i - 1];
				if (parentMip.PowerOfTwoSampleCount <= MinMipSampleCount)
					break;

				WaveformMip& newMip = AllMips[i];
				newMip.PowerOfTwoSampleCount = (parentMip.PowerOfTwoSampleCount / 2);
				newMip.TimePerSample = (parentMip.TimePerSample * 2.0);
				newMip.SamplesPerSecond = (parentMip.SamplesPerSecond / 2.0);
			}

			// TODO: Consider having single continuous sample buffer and store view pointer (or start index) inside each WaveformMip
			[[maybe_unused]] size_t totalSampleCountAcrossAllMips = 0;
			for (const WaveformMip& mip : AllMips)
				totalSampleCountAcrossAllMips += (&mip == &baseMip) ? baseMip.AbsoluteSamples.size() : mip.PowerOfTwoSampleCount;

			// NOTE: Second loop to then generate the actual mip sample buffers
			for (size_t i = 1; i < MaxMipLevels; i++)
			{
				const WaveformMip& parentMip = AllMips[i - 1];
				if (parentMip.PowerOfTwoSampleCount == 0)
					break;
				const size_t parentSampleCount = parentMip.AbsoluteSamples.size();
				const i16* parentSamples = parentMip.AbsoluteSamples.data();

				WaveformMip& thisMip = AllMips[i];
				thisMip.AbsoluteSamples.resize(thisMip.PowerOfTwoSampleCount);
				const size_t samplesToFill = ClampTop(thisMip.AbsoluteSamples.size(), (parentSampleCount / 2));
				for (size_t sampleIndex = 0; sampleIndex < samplesToFill; sampleIndex++)
				{
					// ... assert(parentSamples >= &parentMip.AbsoluteSamples.front() && parentSamples <= &parentMip.AbsoluteSamples.back());
					thisMip.AbsoluteSamples[sampleIndex] = AverageTwoI16SamplesTogether(parentSamples[0], parentSamples[1]);
					parentSamples += 2;
				}
			}
		}
	};
}
