#pragma once
#include "core_types.h"
#include "audio/audio_engine.h"

namespace PeepoDrumKit
{
	struct AudioTestWindow
	{
	public:
		AudioTestWindow() = default;
		~AudioTestWindow() { RemoveSourcePreviewVoice(); }

	public:
		void DrawGui(bool* isOpen);

	private:
		void AudioEngineTabContent();
		void ActiveVoicesTabContent();
		void LoadedSourcesTabContent();

		void StartSourcePreview(Audio::SourceHandle source, Time startTime = Time::Zero());
		void StopSourcePreview();

		void RemoveSourcePreviewVoice();

	private:
		bool sourcePreviewVoiceHasBeenAdded = false;
		Audio::Voice sourcePreviewVoice = Audio::VoiceHandle::Invalid;
		std::string voiceFlagsBuffer;
		u32 newBufferFrameCount = 64;
	};
}
