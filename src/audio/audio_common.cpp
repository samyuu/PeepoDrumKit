#include "audio_common.h"

namespace Audio
{
	i64 PCMSampleBuffer::ReadAtOrFillSilence(i64 inFrameOffset, i64 inFrameCount, i16 outSamples[]) const
	{
		// TODO: Handle channel count mismatch (?) 
		// assert(ChannelCount == 2);

		const i64 sourceChannelCount = ChannelCount;
		const i64 sourceFrameCount = FrameCount;
		const i64 sourceSampleCount = SampleCount();

		std::fill(outSamples, outSamples + (inFrameCount * sourceChannelCount), 0);

		if (InterleavedSamples == nullptr)
			return inFrameCount;

		if ((inFrameOffset + inFrameCount) <= 0 || inFrameOffset > sourceFrameCount)
			return inFrameCount;

		if (inFrameOffset < 0 && (inFrameOffset + inFrameCount) > 0)
		{
			const i64 nonSilentSamples = (inFrameCount + inFrameOffset) * sourceChannelCount;
			i16* nonSilentBuffer = (outSamples - (inFrameOffset * sourceChannelCount));

			std::copy(
				InterleavedSamples.get(),
				Min<i16*>(InterleavedSamples.get() + nonSilentSamples, InterleavedSamples.get() + sourceSampleCount),
				Min<i16*>(nonSilentBuffer, outSamples + (inFrameCount * sourceChannelCount)));
		}
		else
		{
			const i16* sampleSource = &InterleavedSamples[inFrameOffset * sourceChannelCount];
			const i64 framesRead = ((inFrameCount + inFrameOffset) > sourceFrameCount) ? (sourceFrameCount - inFrameOffset) : inFrameCount;

			std::copy(sampleSource, sampleSource + (framesRead * sourceChannelCount), outSamples);
		}

		return inFrameCount;
	}
}
