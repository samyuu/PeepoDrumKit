#include "core_beat.h"
#include <algorithm>

Time TempoMapAccelerationStructure::ConvertBeatToTimeUsingLookupTableIndexing(Beat beat) const
{
	const i32 beatTickToTimesCount = static_cast<i32>(BeatTickToTimes.size());
	const i32 totalBeatTicks = beat.Ticks;

	if (totalBeatTicks < 0) // NOTE: Negative tick (tempo changes are assumed to only be positive)
	{
		// NOTE: Calculate the duration of a Beat at the first tempo
		const Time firstTickDuration = Time::FromSeconds((60.0 / FirstTempoBPM) / Beat::TicksPerBeat);

		// NOTE: Then scale by the negative tick
		return firstTickDuration * totalBeatTicks;
	}
	else if (totalBeatTicks >= beatTickToTimesCount) // NOTE: Tick is outside the defined tempo map
	{
		// NOTE: Take the last calculated time
		const Time lastTime = GetLastCalculatedTime();

		// NOTE: Calculate the duration of a Beat at the last used tempo
		const Time lastTickDuration = Time::FromSeconds((60.0 / LastTempoBPM) / Beat::TicksPerBeat);

		// NOTE: Then scale by the remaining ticks
		const i32 remainingTicks = (totalBeatTicks - beatTickToTimesCount) + 1;
		return lastTime + (lastTickDuration * remainingTicks);
	}
	else // NOTE: Use the pre calculated lookup table directly
	{
		return BeatTickToTimes.at(totalBeatTicks);
	}
}

Beat TempoMapAccelerationStructure::ConvertTimeToBeatUsingLookupTableBinarySearch(Time time) const
{
	const i32 beatTickToTimesCount = static_cast<i32>(BeatTickToTimes.size());
	const Time lastTime = GetLastCalculatedTime();

	if (time < Time::FromSeconds(0.0)) // NOTE: Negative time
	{
		// NOTE: Calculate the duration of a Beat at the first tempo
		const Time firstTickDuration = Time::FromSeconds((60.0 / FirstTempoBPM) / Beat::TicksPerBeat);

		// NOTE: Then the time by the negative tick, this is assuming all tempo changes happen on positive ticks
		return Beat(static_cast<i32>(time / firstTickDuration));
	}
	else if (time >= lastTime) // NOTE: Tick is outside the defined tempo map
	{
		const Time timePastLast = (time - lastTime);

		// NOTE: Each tick past the end has a duration of this value
		const Time lastTickDuration = Time::FromSeconds((60.0 / LastTempoBPM) / Beat::TicksPerBeat);

		// NOTE: So we just have to divide the remaining ticks by the duration
		const f64 ticks = (timePastLast / lastTickDuration);

		// NOTE: And add it to the last tick
		return Beat(static_cast<i32>(beatTickToTimesCount + ticks - 1));
	}
	else // NOTE: Perform a binary search
	{
		i32 left = 0, right = beatTickToTimesCount - 1;

		while (left <= right)
		{
			const i32 mid = (left + right) / 2;

			if (time < BeatTickToTimes[mid])
				right = mid - 1;
			else if (time > BeatTickToTimes[mid])
				left = mid + 1;
			else
				return Beat::FromTicks(mid);
		}

		return Beat::FromTicks((BeatTickToTimes[left] - time) < (time - BeatTickToTimes[right]) ? left : right);
	}
}

Time TempoMapAccelerationStructure::GetLastCalculatedTime() const
{
	return BeatTickToTimes.empty() ? Time::Zero() : BeatTickToTimes.back();
}

void TempoMapAccelerationStructure::Rebuild(const std::vector<TempoChange>& tempoChanges)
{
	assert(tempoChanges.size() > 0);
	BeatTickToTimes.resize(tempoChanges.back().Beat.Ticks + 1);

	f64 lastEndTime = 0.0;
	for (size_t tempoChangeIndex = 0; tempoChangeIndex < tempoChanges.size(); tempoChangeIndex++)
	{
		const TempoChange& tempoChange = tempoChanges[tempoChangeIndex];

		const f64 bpm = tempoChange.Tempo.BPM;
		const f64 beatDuration = (60.0 / bpm);
		const f64 tickDuration = (beatDuration / Beat::TicksPerBeat);

		const bool isSingleOrLastTempo = (tempoChanges.size() == 1) || (tempoChangeIndex == (tempoChanges.size() - 1));
		const size_t timesCount = isSingleOrLastTempo ? BeatTickToTimes.size() : (tempoChanges[tempoChangeIndex + 1].Beat.Ticks);

		for (size_t i = 0, t = tempoChange.Beat.Ticks; t < timesCount; t++)
			BeatTickToTimes[t] = Time::FromSeconds((tickDuration * i++) + lastEndTime);

		if (tempoChanges.size() > 1)
			lastEndTime = BeatTickToTimes[timesCount - 1].TotalSeconds() + tickDuration;

		FirstTempoBPM = (tempoChangeIndex == 0) ? bpm : FirstTempoBPM;
		LastTempoBPM = bpm;
	}
}

template <typename T>
static bool ValidateIsVectorSortedByBeat(const std::vector<T>& vectorWithBeatMember)
{
	return std::is_sorted(vectorWithBeatMember.begin(), vectorWithBeatMember.end(), [](const auto& a, const auto& b) { return a.Beat < b.Beat; });
}

// TODO: Optimize using binary search
template <typename T>
static size_t LinearlySearchSortedVectorForInsertionIndex(const std::vector<T>& sortedChanges, Beat beat)
{
	for (size_t i = 0; i < sortedChanges.size(); i++)
		if (beat <= sortedChanges[i].Beat) return i;
	return sortedChanges.size();
}

// TODO: Optimize using binary search
template <typename T>
static size_t LinearlySearchSortedVectorForExactBeat(const std::vector<T>& sortedChanges, Beat beat)
{
	for (size_t i = 0; i < sortedChanges.size(); i++)
		if (sortedChanges[i].Beat == beat) return i;
	return sortedChanges.size();
}

template <typename T>
static void InsertOrUpdateIntoSortedVector(std::vector<T>& sortedChanges, T changeToInsertOrUpdate)
{
	const size_t insertionIndex = LinearlySearchSortedVectorForInsertionIndex(sortedChanges, changeToInsertOrUpdate.Beat);
	if (InBounds(insertionIndex, sortedChanges))
	{
		if (T& existing = sortedChanges[insertionIndex]; existing.Beat == changeToInsertOrUpdate.Beat)
			existing = changeToInsertOrUpdate;
		else
			sortedChanges.insert(sortedChanges.begin() + insertionIndex, changeToInsertOrUpdate);
	}
	else
	{
		sortedChanges.push_back(changeToInsertOrUpdate);
	}

	assert(changeToInsertOrUpdate.Beat.Ticks >= 0);
	assert(ValidateIsVectorSortedByBeat(sortedChanges));
}

template <typename T>
static void RemoveAtIndexFromSortedVectorAndAddDefaultIfEmpty(std::vector<T>& sortedChanges, size_t indexToRemove, T defaultValue)
{
	if (InBounds(indexToRemove, sortedChanges))
	{
		sortedChanges.erase(sortedChanges.begin() + indexToRemove);
		if (sortedChanges.empty())
			sortedChanges.push_back(defaultValue);
	}
}

// TODO: Optimize using binary search
template <typename T>
static size_t LinearlySearchSortedVectorForChangeWithLowerBeat(const std::vector<T>& sortedChanges, Beat beat)
{
	assert(!sortedChanges.empty());
	if (sortedChanges.size() <= 1)
		return 0;

	for (size_t i = 0; i < sortedChanges.size() - 1; i++)
	{
		if (sortedChanges[i].Beat <= beat && sortedChanges[i + 1].Beat > beat)
			return i;
	}

	return sortedChanges.size() - 1;
}

void SortedTempoMap::Reset()
{
	TempoChanges.clear();
	TempoChanges.push_back(TempoChange(Beat(0), TempoChange::Default));
	SignatureChanges.clear();
	SignatureChanges.push_back(TimeSignatureChange(Beat(0), TimeSignatureChange::Default));
	RebuildAccelerationStructure();
}

void SortedTempoMap::TempoInsertOrUpdate(TempoChange tempoChangeToInsertOrUpdate) { InsertOrUpdateIntoSortedVector(TempoChanges, tempoChangeToInsertOrUpdate); }

void SortedTempoMap::TempoRemoveAtIndex(size_t indexToRemove) { RemoveAtIndexFromSortedVectorAndAddDefaultIfEmpty(TempoChanges, indexToRemove, TempoChange(Beat(0), TempoChange::Default)); }

void SortedTempoMap::SignatureInsertOrUpdate(TimeSignatureChange signatureChangeToInsertOrUpdate) { InsertOrUpdateIntoSortedVector(SignatureChanges, signatureChangeToInsertOrUpdate); }

void SortedTempoMap::SignatureRemoveAtIndex(size_t indexToRemove) { RemoveAtIndexFromSortedVectorAndAddDefaultIfEmpty(SignatureChanges, indexToRemove, TimeSignatureChange(Beat(0), TimeSignatureChange::Default)); }

size_t SortedTempoMap::TempoFindLastIndexAtBeat(Beat beat) const { return LinearlySearchSortedVectorForChangeWithLowerBeat(TempoChanges, beat); }

size_t SortedTempoMap::TempoFindIndexWithExactBeat(Beat exactBeat) const { return LinearlySearchSortedVectorForExactBeat(TempoChanges, exactBeat); }

size_t SortedTempoMap::SignatureFindLastIndexAtBeat(Beat beat) const { return LinearlySearchSortedVectorForChangeWithLowerBeat(SignatureChanges, beat); }

size_t SortedTempoMap::SignatureFindIndexWithExactBeat(Beat exactBeat) const { return LinearlySearchSortedVectorForExactBeat(SignatureChanges, exactBeat); }
