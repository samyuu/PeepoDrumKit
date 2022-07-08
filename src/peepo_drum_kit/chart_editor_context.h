#pragma once
#include "core_types.h"
#include "core_undo.h"
#include "chart.h"
#include "chart_sound_effects.h"
#include "audio/audio_engine.h"
#include "audio/audio_waveform.h"

namespace PeepoDrumKit
{
	struct BeatAndTime { Beat Beat; Time Time; };

	// NOTE: Essentially the shared data between all the chart editor components, split into a separate file away from the chart editor to avoid circular #includes
	struct ChartContext : NonCopyable
	{
		ChartProject Chart;
		std::string ChartFilePath;

		// NOTE: Must **ALWAYS** point to a valid course. If "Chart.Courses.empty()" then a new one should automatically be added at the start of the frame before anything else is updated
		ChartCourse* ChartSelectedCourse = nullptr;
		BranchType ChartSelectedBranch = BranchType::Normal;

		// NOTE: Specifically for accurate playback preview sound future-offset calculations
		Time CursorNonSmoothTimeThisFrame, CursorNonSmoothTimeLastFrame;
		// NOTE: Store cursor time as Beat while paused to avoid any floating point precision issues and make sure "SetCursorBeat(x); assert(GetCursorBeat() == x)"
		Beat CursorBeatWhilePaused = Beat::Zero();
		// NOTE: Specifically to skip hit animations for notes before this time point
		Time CursorTimeOnPlaybackStart = Time::Zero();

		// NOTE: Specifically for animations, accumulated program delta time, unrelated to GetCursorTime() etc.
		Time ElapsedProgramTimeSincePlaybackStarted = Time::Zero();
		Time ElapsedProgramTimeSincePlaybackStopped = Time::Zero();

		Audio::Voice SongVoice = Audio::VoiceHandle::Invalid;
		Audio::SourceHandle SongSource = Audio::SourceHandle::Invalid;
		SoundEffectsVoicePool SfxVoicePool;

		std::string SongSourceFilePath;
		Audio::WaveformMipChain SongWaveformL;
		Audio::WaveformMipChain SongWaveformR;
		f32 SongWaveformFadeAnimationCurrent = 0.0f;
		f32 SongWaveformFadeAnimationTarget = 0.0f;

		Undo::UndoHistory Undo;

	public:
		inline Time BeatToTime(Beat beat) const { return ChartSelectedCourse->TempoMap.BeatToTime(beat); }
		inline Beat TimeToBeat(Time time) const { return ChartSelectedCourse->TempoMap.TimeToBeat(time); }

		inline f32 GetPlaybackSpeed() { return SongVoice.GetPlaybackSpeed(); }
		inline void SetPlaybackSpeed(f32 newSpeed) { if (!ApproxmiatelySame(SongVoice.GetPlaybackSpeed(), newSpeed)) SongVoice.SetPlaybackSpeed(newSpeed); }

		inline bool GetIsPlayback() const
		{
			return SongVoice.GetIsPlaying();
		}

		inline void SetIsPlayback(bool newIsPlaying)
		{
			if (SongVoice.GetIsPlaying() == newIsPlaying)
				return;

			if (newIsPlaying)
			{
				CursorTimeOnPlaybackStart = (SongVoice.GetPosition() + Chart.SongOffset);
				SongVoice.SetIsPlaying(true);
			}
			else
			{
				SongVoice.SetIsPlaying(false);
				CursorBeatWhilePaused = ChartSelectedCourse->TempoMap.TimeToBeat((SongVoice.GetPosition() + Chart.SongOffset));
				SfxVoicePool.PauseAllFutureVoices();
			}
		}

		inline Time GetCursorTime() const
		{
			if (SongVoice.GetIsPlaying())
				return (SongVoice.GetPositionSmooth() + Chart.SongOffset);
			else
				return ChartSelectedCourse->TempoMap.BeatToTime(CursorBeatWhilePaused);
		}

		inline Beat GetCursorBeat() const
		{
			if (SongVoice.GetIsPlaying())
				return ChartSelectedCourse->TempoMap.TimeToBeat((SongVoice.GetPositionSmooth() + Chart.SongOffset));
			else
				return CursorBeatWhilePaused;
		}

		// NOTE: Specifically to make sure the cursor time and beat are both in sync (as the song voice position is updated asynchronously)
		inline BeatAndTime GetCursorBeatAndTime() const
		{
			if (SongVoice.GetIsPlaying()) { const Time t = (SongVoice.GetPositionSmooth() + Chart.SongOffset); return { ChartSelectedCourse->TempoMap.TimeToBeat(t), t }; }
			else { const Beat b = CursorBeatWhilePaused; return { b, ChartSelectedCourse->TempoMap.BeatToTime(b) }; }
		}

		inline void SetCursorTime(Time newTime)
		{
			SongVoice.SetPosition(newTime - Chart.SongOffset);
			CursorNonSmoothTimeThisFrame = CursorNonSmoothTimeLastFrame = newTime;
			CursorBeatWhilePaused = ChartSelectedCourse->TempoMap.TimeToBeat(newTime);
			CursorTimeOnPlaybackStart = newTime;
		}

		inline void SetCursorBeat(Beat newBeat)
		{
			const Time newTime = ChartSelectedCourse->TempoMap.BeatToTime(newBeat);
			SongVoice.SetPosition(newTime - Chart.SongOffset);
			CursorNonSmoothTimeThisFrame = CursorNonSmoothTimeLastFrame = newTime;
			CursorBeatWhilePaused = newBeat;
			CursorTimeOnPlaybackStart = newTime;
		}
	};
}
