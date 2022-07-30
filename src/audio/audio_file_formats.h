#pragma once
#include "core_types.h"
#include "audio_common.h"
#include "core_string.h"

namespace Audio
{
	enum class SupportedFileFormat : u8
	{
		OggVorbis,
		WAV,
		FLAC,
		MP3,
		Count
	};

	constexpr const char* SupportedFileFormatExtensions[EnumCount<SupportedFileFormat>] =
	{
		".ogg",
		".wav",
		".flac",
		".mp3",
	};

	constexpr cstr SupportedFileFormatExtensionsPacked = ".ogg;.wav;.flac;.mp3";

	enum class DecodeFileResult : u8
	{
		FeelsGoodMan,
		Sadge,
		Count
	};

	SupportedFileFormat TryToDetermineFileFormatFromExtension(std::string_view fileName);

	// TODO: Also expose file streaming API if / when it's needed
	//		 but for now everything will be stored in one big continuous buffer since it greatly reduces complexity.
	//		 Instead of constantly reading a streamed file from disk it might also be an option to read the entire file upfront but then only decode chunks on demand (?)
	DecodeFileResult DecodeEntireFile(std::string_view fileNameWithExtension, const void* inFileContent, size_t inFileSize, PCMSampleBuffer& outBuffer);
}
