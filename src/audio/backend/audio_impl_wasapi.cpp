#include "audiO_backend.h"
#include "audio/audio_common.h"

#include <stdio.h>
#include <atomic>

#include <Windows.h>
#include <Audioclient.h>
#include <Audiopolicy.h>
#include <mmdeviceapi.h>
#include <wrl.h>
#include <avrt.h>

#pragma comment(lib, "avrt.lib")

using Microsoft::WRL::ComPtr;

namespace Audio
{
	static thread_local struct Win32ThreadLocalData
	{
		i32 CoInitializeCount = 0;
		HRESULT CoInitializeResult = {};
	} Win32ThreadLocal = {};

	static HRESULT Win32ThreadLocalCoInitializeOnce()
	{
		if (Win32ThreadLocal.CoInitializeCount++ == 0)
			Win32ThreadLocal.CoInitializeResult = ::CoInitialize(nullptr);

		return Win32ThreadLocal.CoInitializeResult;
	}

	static HRESULT Win32ThreadLocalCoUnInitializeIfLast()
	{
		if (Win32ThreadLocal.CoInitializeCount <= 0)
		{
			assert(false);
			return E_ABORT;
		}

		if (Win32ThreadLocal.CoInitializeCount-- == 1)
			::CoUninitialize();

		return S_OK;
	}

	struct WASAPIBackend::Impl
	{
	public:
		b8 OpenStartStream(const BackendStreamParam& param, BackendRenderCallback callback)
		{
			if (isOpenRunning)
				return false;

			streamParam = param;
			renderCallback = std::move(callback);

			HRESULT error = S_OK;
			error = Win32ThreadLocalCoInitializeOnce();

			error = ::CoCreateInstance(__uuidof(::MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(::IMMDeviceEnumerator), &deviceEnumerator);
			if (FAILED(error))
			{
				printf(__FUNCTION__"(): Unable to create MMDeviceEnumerator. Error: 0x%X\n", error);
				return false;
			}

			error = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
			if (FAILED(error))
			{
				printf(__FUNCTION__"(): Unable to retrieve default audio endpoint. Error: 0x%X\n", error);
				return false;
			}

			error = device->Activate(__uuidof(::IAudioClient), CLSCTX_ALL, nullptr, &audioClient);
			if (FAILED(error))
			{
				printf(__FUNCTION__"(): Unable to activate audio client for device. Error: 0x%X\n", error);
				return false;
			}

			waveformat.wFormatTag = WAVE_FORMAT_PCM;
			waveformat.nChannels = streamParam.ChannelCount;
			waveformat.nSamplesPerSec = streamParam.SampleRate;
			waveformat.wBitsPerSample = sizeof(i16) * CHAR_BIT;
			waveformat.nBlockAlign = (waveformat.nChannels * waveformat.wBitsPerSample / CHAR_BIT);
			waveformat.nAvgBytesPerSec = (waveformat.nSamplesPerSec * waveformat.nBlockAlign);
			waveformat.cbSize = 0;

			static constexpr DWORD sharedStreamFlags = (AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY);
			static constexpr DWORD exclusiveStreamFlags = (AUDCLNT_STREAMFLAGS_EVENTCALLBACK);

			const AUDCLNT_SHAREMODE shareMode = (streamParam.ShareMode == StreamShareMode::Shared) ? AUDCLNT_SHAREMODE_SHARED : AUDCLNT_SHAREMODE_EXCLUSIVE;
			const DWORD streamFlags = (streamParam.ShareMode == StreamShareMode::Shared) ? sharedStreamFlags : exclusiveStreamFlags;

			// TODO: Account for user requested frame buffer size (?)
			error = audioClient->GetDevicePeriod(nullptr, &bufferTimeDuration);

			deviceTimePeriod = (streamParam.ShareMode == StreamShareMode::Shared) ? 0 : bufferTimeDuration;
			if (FAILED(error))
			{
				printf(__FUNCTION__"(): Unable to retrieve device period. Error: 0x%X\n", error);
				return false;
			}

			error = audioClient->Initialize(shareMode, streamFlags, bufferTimeDuration, deviceTimePeriod, &waveformat, nullptr);
			if (error == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED)
			{
				error = audioClient->GetBufferSize(&bufferFrameCount);
				if (FAILED(error))
				{
					printf(__FUNCTION__"(): Unable to get audio client buffer size. Error: 0x%X\n", error);
					return false;
				}

				bufferTimeDuration = static_cast<::REFERENCE_TIME>((10000.0 * 1000 / waveformat.nSamplesPerSec * bufferFrameCount) + 0.5);
				deviceTimePeriod = (streamParam.ShareMode == StreamShareMode::Shared) ? 0 : bufferTimeDuration;

				error = device->Activate(__uuidof(::IAudioClient), CLSCTX_ALL, nullptr, &audioClient);
				if (FAILED(error))
				{
					printf(__FUNCTION__"(): Unable to activate audio client for device. Error: 0x%X\n", error);
					return false;
				}

				error = audioClient->Initialize(shareMode, streamFlags, bufferTimeDuration, deviceTimePeriod, &waveformat, nullptr);
				if (FAILED(error))
				{
					printf(__FUNCTION__"(): Unable to initialize audio client. Error: 0x%X\n", error);
					return false;
				}
			}
			else if (FAILED(error))
			{
				printf(__FUNCTION__"(): Unable to initialize audio client. Error: 0x%X\n", error);
				return false;
			}

			audioClientEvent = ::CreateEventW(nullptr, false, false, nullptr);
			if (audioClientEvent == NULL)
			{
				printf(__FUNCTION__"(): Unable to create audio client event. Error: 0x%X\n", ::GetLastError());
				return false;
			}

			error = audioClient->SetEventHandle(audioClientEvent);
			if (FAILED(error))
			{
				printf(__FUNCTION__"(): Unable to set audio client event handle. Error: 0x%X\n", error);
				return false;
			}

			error = audioClient->GetService(__uuidof(::IAudioRenderClient), &renderClient);
			if (FAILED(error))
			{
				printf(__FUNCTION__"(): Unable to get audio render client. Error: 0x%X\n", error);
				return false;
			}

			error = audioClient->GetService(__uuidof(::ISimpleAudioVolume), &simpleAudioVolume);
			if (FAILED(error))
				printf(__FUNCTION__"(): Unable to get simple audio volume interface. Error: 0x%X\n", error);

			renderThread = ::CreateThread(nullptr, 0, [](LPVOID lpParameter) -> DWORD
			{
				return static_cast<DWORD>(reinterpret_cast<WASAPIBackend::Impl*>(lpParameter)->RenderThreadEntryPoint());
			}, static_cast<void*>(this), 0, nullptr);

			if (renderThread == NULL)
			{
				printf(__FUNCTION__"(): Unable to create render thread. Error: 0x%X\n", ::GetLastError());
				return false;
			}

			isOpenRunning = true;
			return true;
		}

		b8 StopCloseStream()
		{
			if (!isOpenRunning)
				return false;

			isOpenRunning = false;
			renderThreadStopRequested = true;
			if (renderThread != NULL)
			{
				::WaitForSingleObject(renderThread, INFINITE);
				::CloseHandle(renderThread);
				renderThread = NULL;
			}
			renderThreadStopRequested = false;

			if (audioClientEvent != NULL)
			{
				::CloseHandle(audioClientEvent);
				audioClientEvent = NULL;
			}

			simpleAudioVolume = nullptr;
			renderClient = nullptr;
			audioClient = nullptr;
			device = nullptr;
			deviceEnumerator = nullptr;

			return true;
		}

	public:
		b8 IsOpenRunning() const
		{
			return isOpenRunning;
		}

	public:
		u32 RenderThreadEntryPoint()
		{
			if (audioClient == nullptr || renderClient == nullptr)
			{
				printf(__FUNCTION__"(): Audio client uninitialized\n");
				return -1;
			}

			HRESULT error = S_OK;
			error = Win32ThreadLocalCoInitializeOnce();
			defer { Win32ThreadLocalCoUnInitializeIfLast(); };

			error = audioClient->GetBufferSize(&bufferFrameCount);
			if (FAILED(error))
			{
				printf(__FUNCTION__"(): Unable to get audio client buffer size. Error: 0x%X\n", error);
				return -1;
			}

			// NOTE: Load the first buffer with data before starting the stream to to reduce latency
			BYTE* tempOutputBuffer = nullptr;
			error = renderClient->GetBuffer(bufferFrameCount, &tempOutputBuffer);

			if (!FAILED(error))
				RenderThreadProcessOutputBuffer(reinterpret_cast<i16*>(tempOutputBuffer), bufferFrameCount, streamParam.ChannelCount);

			DWORD releaseBufferFlags = 0;
			error = renderClient->ReleaseBuffer(bufferFrameCount, releaseBufferFlags);

			// NOTE: Ask MMCSS to temporarily boost the thread priority to reduce glitches while the low-latency stream plays.
			proAudioTaskIndex = 0;
			proAudioTask = ::AvSetMmThreadCharacteristicsW(L"Pro Audio", &proAudioTaskIndex);
			if (proAudioTask == NULL)
				printf(__FUNCTION__"(): Unable to assign Pro Audio thread characteristics to render thread. Error: 0x%X\n", error);

			error = audioClient->Start();
			if (FAILED(error))
			{
				printf(__FUNCTION__"(): Unable to start audio client. Error: 0x%X\n", error);
				return -1;
			}

			while (releaseBufferFlags != AUDCLNT_BUFFERFLAGS_SILENT)
			{
				if (renderThreadStopRequested)
					break;

				const DWORD waitObjectResult = ::WaitForSingleObject(audioClientEvent, 2000);
				if (waitObjectResult != WAIT_OBJECT_0)
				{
					printf(__FUNCTION__"(): Audio client event timeout. Error: 0x%X\n", ERROR_TIMEOUT);
					break;
				}

				error = audioClient->GetBufferSize(&bufferFrameCount);
				if (FAILED(error) || bufferFrameCount == 0)
					continue;

				UINT32 remainingFrameCount = bufferFrameCount;
				if (streamParam.ShareMode == StreamShareMode::Shared)
				{
					UINT32 paddingFrameCount;
					error = audioClient->GetCurrentPadding(&paddingFrameCount);
					if (!FAILED(error))
						remainingFrameCount -= paddingFrameCount;
				}

				error = renderClient->GetBuffer(remainingFrameCount, &tempOutputBuffer);

				if (!FAILED(error))
					RenderThreadProcessOutputBuffer(reinterpret_cast<i16*>(tempOutputBuffer), remainingFrameCount, streamParam.ChannelCount);

				error = renderClient->ReleaseBuffer(remainingFrameCount, releaseBufferFlags);
			}

			error = audioClient->Stop();
			if (FAILED(error))
				printf(__FUNCTION__"(): Unable to stop audio client. Error: 0x%X\n", error);

			if (proAudioTask != NULL)
				::AvRevertMmThreadCharacteristics(proAudioTask);

			return 0;
		}

		void RenderThreadProcessOutputBuffer(i16* outputBuffer, const u32 frameCount, const u32 channelCount)
		{
			if (outputBuffer == nullptr)
				return;

			renderCallback(outputBuffer, frameCount, channelCount);

			if (applySharedSessionVolume && streamParam.ShareMode == StreamShareMode::Exclusive)
			{
				float sharedSessionVolume = 1.0f;
				const HRESULT error = (simpleAudioVolume != nullptr) ? simpleAudioVolume->GetMasterVolume(&sharedSessionVolume) : E_POINTER;

				if (!FAILED(error) && sharedSessionVolume >= 0.0f && sharedSessionVolume < 1.0f)
				{
					for (u32 i = 0; i < (frameCount * streamParam.ChannelCount); i++)
						outputBuffer[i] = ConvertSampleF32ToI16(ConvertSampleI16ToF32(outputBuffer[i]) * sharedSessionVolume);
				}
			}
		}

	private:
		BackendStreamParam streamParam = {};
		BackendRenderCallback renderCallback;

		std::atomic<b8> isOpenRunning = false;
		std::atomic<b8> renderThreadStopRequested = false;

		b8 applySharedSessionVolume = true;

		::WAVEFORMATEX waveformat = {};
		::REFERENCE_TIME bufferTimeDuration = {}, deviceTimePeriod = {};
		::UINT32 bufferFrameCount = {};

		::HANDLE renderThread = NULL;

		::DWORD proAudioTaskIndex = {};
		::HANDLE proAudioTask = {};

		::HANDLE audioClientEvent = NULL;
		ComPtr<::IMMDeviceEnumerator> deviceEnumerator = nullptr;
		ComPtr<::IMMDevice> device = nullptr;
		ComPtr<::IAudioClient> audioClient = nullptr;
		ComPtr<::IAudioRenderClient> renderClient = nullptr;
		ComPtr<::ISimpleAudioVolume> simpleAudioVolume = nullptr;
	};

	WASAPIBackend::WASAPIBackend() : impl(std::make_unique<Impl>()) {}
	WASAPIBackend::~WASAPIBackend() = default;
	b8 WASAPIBackend::OpenStartStream(const BackendStreamParam& param, BackendRenderCallback callback) { return impl->OpenStartStream(param, std::move(callback)); }
	b8 WASAPIBackend::StopCloseStream() { return impl->StopCloseStream(); }
	b8 WASAPIBackend::IsOpenRunning() const { return impl->IsOpenRunning(); }
}
