#include "audio_file_formats.h"
#include "core_io.h"

#define DR_MP3_IMPLEMENTATION
#include "3rdparty/dr_mp3.h"

#define DR_FLAC_IMPLEMENTATION
#include "3rdparty/dr_flac.h"

#define DR_WAV_IMPLEMENTATION
#include "3rdparty/dr_wav.h"

// #define STB_VORBIS_NO_PUSHDATA_API
// #define STB_VORBIS_NO_PULLDATA_API
// #define STB_VORBIS_NO_STDIO
// #include "3rdparty/stb_vorbis.c"

// TODO: Forward declare because visual studio is having a stroke parsing the C header (something about the typedef union { ... } Floor; ???)
//		 even though it was working perfectly fine in a different C++ project before :PeepoShrug:
extern "C" int stb_vorbis_decode_memory(const unsigned char* mem, int len, int* channels, int* sample_rate, short** output);

namespace Audio
{
	SupportedFileFormat TryToDetermineFileFormatFromExtension(std::string_view fileName)
	{
		for (size_t i = 0; i < EnumCount<SupportedFileFormat>; i++)
		{
			if (ASCII::EndsWithInsensitive(fileName, SupportedFileFormatExtensions[i]))
				return static_cast<SupportedFileFormat>(i);
		}
		return SupportedFileFormat::Count;
	}

	DecodeFileResult DecodeEntireFile(std::string_view fileNameWithExtension, const void* inFileContent, size_t inFileSize, PCMSampleBuffer& outBuffer)
	{
		outBuffer = {};

		if (inFileContent == nullptr || inFileSize == 0)
			return DecodeFileResult::Sadge;

		// TODO: Check for magic bytes instead, though shouldn't really matter for now
		const SupportedFileFormat fileFormat = TryToDetermineFileFormatFromExtension(fileNameWithExtension);
		if (fileFormat == SupportedFileFormat::Count)
			return DecodeFileResult::Sadge;

		switch (fileFormat)
		{
		case SupportedFileFormat::OggVorbis:
		{
			i32 outChannels = {};
			i32 outSampleRate = {};
			i16* outSamplesI16 = {};
			i32 outFrameCount = ::stb_vorbis_decode_memory(static_cast<const unsigned char*>(inFileContent), static_cast<int>(inFileSize), &outChannels, &outSampleRate, &outSamplesI16);
			defer { ::free(outSamplesI16); };
			if (outSamplesI16 == nullptr || outFrameCount < 0)
				return DecodeFileResult::Sadge;

			const size_t totalSampleCount = static_cast<size_t>(outFrameCount * outChannels);
			outBuffer.ChannelCount = static_cast<u32>(outChannels);
			outBuffer.SampleRate = static_cast<u32>(outSampleRate);
			outBuffer.FrameCount = static_cast<i64>(outFrameCount);
			// outBuffer.InterleavedSamples = std::make_unique<i16[]>(totalSampleCount);
			outBuffer.InterleavedSamples = std::unique_ptr<i16[]>(new i16[totalSampleCount]);
			if (outBuffer.InterleavedSamples == nullptr)
				return DecodeFileResult::Sadge;

			// TODO: Prevent copy by.. overwriting malloc? or manually read chuncks
			::memcpy(outBuffer.InterleavedSamples.get(), outSamplesI16, totalSampleCount * sizeof(i16));
		} break;

		case SupportedFileFormat::WAV:
		{
			u32 outChannels = {};
			u32 outSampleRate = {};
			u64 outFrameCount = {};
			i16* outSamplesI16 = ::drwav_open_memory_and_read_pcm_frames_s16(inFileContent, inFileSize, &outChannels, &outSampleRate, &outFrameCount, nullptr);
			defer { ::drwav_free(outSamplesI16, nullptr); };
			if (outSamplesI16 == nullptr)
				return DecodeFileResult::Sadge;

			const size_t totalSampleCount = static_cast<size_t>(outFrameCount * outChannels);
			outBuffer.ChannelCount = static_cast<u32>(outChannels);
			outBuffer.SampleRate = static_cast<u32>(outSampleRate);
			outBuffer.FrameCount = static_cast<i64>(outFrameCount);
			// outBuffer.InterleavedSamples = std::make_unique<i16[]>(totalSampleCount);
			outBuffer.InterleavedSamples = std::unique_ptr<i16[]>(new i16[totalSampleCount]);
			if (outBuffer.InterleavedSamples == nullptr)
				return DecodeFileResult::Sadge;

			// TODO: Prevent copy by passing down custom allocator or manually read chuncks
			::memcpy(outBuffer.InterleavedSamples.get(), outSamplesI16, totalSampleCount * sizeof(i16));
		} break;

		case SupportedFileFormat::FLAC:
		{
			u32 outChannels = {};
			u32 outSampleRate = {};
			u64 outFrameCount = {};
			i16* outSamplesI16 = ::drflac_open_memory_and_read_pcm_frames_s16(inFileContent, inFileSize, &outChannels, &outSampleRate, &outFrameCount, nullptr);
			defer { ::drflac_free(outSamplesI16, nullptr); };
			if (outSamplesI16 == nullptr)
				return DecodeFileResult::Sadge;

			const size_t totalSampleCount = static_cast<size_t>(outFrameCount * outChannels);
			outBuffer.ChannelCount = static_cast<u32>(outChannels);
			outBuffer.SampleRate = static_cast<u32>(outSampleRate);
			outBuffer.FrameCount = static_cast<i64>(outFrameCount);
			// outBuffer.InterleavedSamples = std::make_unique<i16[]>(totalSampleCount);
			outBuffer.InterleavedSamples = std::unique_ptr<i16[]>(new i16[totalSampleCount]);
			if (outBuffer.InterleavedSamples == nullptr)
				return DecodeFileResult::Sadge;

			// TODO: Prevent copy by passing down custom allocator or manually read chuncks
			::memcpy(outBuffer.InterleavedSamples.get(), outSamplesI16, totalSampleCount * sizeof(i16));
		} break;

		case SupportedFileFormat::MP3:
		{
			::drmp3_config outConfig = {};
			::drmp3_uint64 outFrameCount = {};
			::drmp3d_sample_t* outSamplesI16 = ::drmp3_open_memory_and_read_pcm_frames_s16(inFileContent, inFileSize, &outConfig, &outFrameCount, nullptr);
			defer { ::drmp3_free(outSamplesI16, nullptr); };
			if (outSamplesI16 == nullptr)
				return DecodeFileResult::Sadge;

			const size_t totalSampleCount = static_cast<size_t>(outFrameCount * outConfig.channels);
			outBuffer.ChannelCount = static_cast<u32>(outConfig.channels);
			outBuffer.SampleRate = static_cast<u32>(outConfig.sampleRate);
			outBuffer.FrameCount = static_cast<i64>(outFrameCount);
			// outBuffer.InterleavedSamples = std::make_unique<i16[]>(totalSampleCount);
			outBuffer.InterleavedSamples = std::unique_ptr<i16[]>(new i16[totalSampleCount]);
			if (outBuffer.InterleavedSamples == nullptr)
				return DecodeFileResult::Sadge;

			// TODO: Prevent copy by passing down custom allocator or manually read chuncks
			::memcpy(outBuffer.InterleavedSamples.get(), outSamplesI16, totalSampleCount * sizeof(i16));
		} break;

		default:
		{
			assert(!"Unhandled file format switch case");
			return DecodeFileResult::Sadge;
		} break;
		}

		return DecodeFileResult::FeelsGoodMan;
	}
}
