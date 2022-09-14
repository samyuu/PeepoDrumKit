#pragma once
#include "core_types.h"
#include "audio/audio_engine.h"

namespace PeepoDrumKit
{
	struct AudioTestWindow
	{
		AudioTestWindow() = default;
		~AudioTestWindow() { RemoveSourcePreviewVoice(); }

		void DrawGui();

	private:
		void AudioEngineTabContent();
		void ActiveVoicesTabContent();
		void LoadedSourcesTabContent();

		void StartSourcePreview(Audio::SourceHandle source, Time startTime = Time::Zero());
		void StopSourcePreview();

		void RemoveSourcePreviewVoice();

		b8 sourcePreviewVoiceHasBeenAdded = false;
		Audio::Voice sourcePreviewVoice = Audio::VoiceHandle::Invalid;
		std::string voiceFlagsBuffer;
		u32 newBufferFrameCount = 64;
	};
}
