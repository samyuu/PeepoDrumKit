#pragma once
#include "core_types.h"
#include <vector>

struct Beat
{
	// NOTE: Meaning a 4/4 bar can be meaningfully subdivided into 192 parts, as is a common convention for rhythm games
	static constexpr i32 TicksPerBeat = (192 / 4);

	i32 Ticks;

	Beat() = default;
	constexpr explicit Beat(i32 ticks) : Ticks(ticks) {}

	constexpr f64 BeatsFraction() const { return static_cast<f64>(Ticks) / static_cast<f64>(TicksPerBeat); }

	static constexpr Beat Zero() { return Beat(0); }
	static constexpr Beat FromTicks(i32 ticks) { return Beat(ticks); }
	static constexpr Beat FromBeats(i32 beats) { return Beat(TicksPerBeat * beats); }
	static constexpr Beat FromBars(i32 bars, i32 beatsPerBar = 4) { return FromBeats(bars * beatsPerBar); }
	static constexpr Beat FromBeatsFraction(f64 fraction) { return FromTicks(static_cast<i32>(Round(fraction * static_cast<f64>(TicksPerBeat)))); }

	constexpr b8 operator==(const Beat& other) const { return Ticks == other.Ticks; }
	constexpr b8 operator!=(const Beat& other) const { return Ticks != other.Ticks; }
	constexpr b8 operator<=(const Beat& other) const { return Ticks <= other.Ticks; }
	constexpr b8 operator>=(const Beat& other) const { return Ticks >= other.Ticks; }
	constexpr b8 operator<(const Beat& other) const { return Ticks < other.Ticks; }
	constexpr b8 operator>(const Beat& other) const { return Ticks > other.Ticks; }

	constexpr Beat operator+(const Beat& other) const { return Beat(Ticks + other.Ticks); }
	constexpr Beat operator-(const Beat& other) const { return Beat(Ticks - other.Ticks); }
	constexpr Beat operator*(const i32 ticks) const { return Beat(Ticks * ticks); }
	constexpr Beat operator/(const i32 ticks) const { return Beat(Ticks / ticks); }
	constexpr Beat operator%(const Beat& other) const { return Beat(Ticks % other.Ticks); }

	constexpr Beat& operator+=(const Beat& other) { (Ticks += other.Ticks); return *this; }
	constexpr Beat& operator-=(const Beat& other) { (Ticks -= other.Ticks); return *this; }
	constexpr Beat& operator*=(const i32 ticks) { (Ticks *= ticks); return *this; }
	constexpr Beat& operator/=(const i32 ticks) { (Ticks *= ticks); return *this; }
	constexpr Beat& operator%=(const Beat& other) { (Ticks %= other.Ticks); return *this; }

	constexpr Beat operator-() const { return Beat(-Ticks); }
};

inline Beat FloorBeatToGrid(Beat beat, Beat grid) { return Beat::FromTicks(static_cast<i32>(Floor(static_cast<f64>(beat.Ticks) / static_cast<f64>(grid.Ticks))) * grid.Ticks); }
inline Beat RoundBeatToGrid(Beat beat, Beat grid) { return Beat::FromTicks(static_cast<i32>(Round(static_cast<f64>(beat.Ticks) / static_cast<f64>(grid.Ticks))) * grid.Ticks); }
inline Beat CeilBeatToGrid(Beat beat, Beat grid) { return Beat::FromTicks(static_cast<i32>(Ceil(static_cast<f64>(beat.Ticks) / static_cast<f64>(grid.Ticks))) * grid.Ticks); }
constexpr Beat GetGridBeatSnap(i32 gridBarDivision) { return Beat::FromBars(1) / gridBarDivision; }
constexpr b8 IsTupletBarDivision(i32 gridBarDivision) { return (gridBarDivision % 3 == 0); }

struct Tempo
{
	f32 BPM = 0.0f;

	constexpr Tempo() = default;
	constexpr explicit Tempo(f32 bpm) : BPM(bpm) {}
};

constexpr Tempo SafetyCheckTempo(Tempo v) { return (v.BPM <= -1.0f) ? Tempo(-v.BPM) : Tempo(ClampBot(v.BPM, 1.0f)); }

struct TimeSignature
{
	i32 Numerator = 4;
	i32 Denominator = 4;

	constexpr TimeSignature() = default;
	constexpr TimeSignature(i32 numerator, i32 denominator) : Numerator(numerator), Denominator(denominator) {}

	constexpr i32 GetBeatsPerBar() const { return Numerator; }
	constexpr Beat GetDurationPerBeat() const { return Beat::FromBars(1) / Denominator; }
	constexpr Beat GetDurationPerBar() const { return GetDurationPerBeat() * GetBeatsPerBar(); }

	constexpr b8 operator==(const TimeSignature& other) const { return (Numerator == other.Numerator) && (Denominator == other.Denominator); }
	constexpr b8 operator!=(const TimeSignature& other) const { return (Numerator != other.Numerator) || (Denominator != other.Denominator); }
};

constexpr b8 IsTimeSignatureSupported(TimeSignature v) { return (v.Numerator > 0 && v.Denominator > 0) && (Beat::FromBars(1).Ticks % v.Denominator) == 0; }

struct TempoChange
{
	constexpr TempoChange() = default;
	constexpr TempoChange(Beat beat, Tempo tempo) : Beat(beat), Tempo(tempo) {}

	Beat Beat = {};
	Tempo Tempo = {};
	b8 IsSelected = false;
};

struct TimeSignatureChange
{
	constexpr TimeSignatureChange() = default;
	constexpr TimeSignatureChange(Beat beat, TimeSignature signature) : Beat(beat), Signature(signature) {}

	Beat Beat = {};
	TimeSignature Signature = {};
	b8 IsSelected = false;
};

// NOTE: Beat accessor function specifically to be used inside templates without coupling to a specific struct member name
constexpr Beat GetBeat(const TimeSignatureChange& v) { return v.Beat; }
constexpr Beat GetBeat(const TempoChange& v) { return v.Beat; }
constexpr Beat GetBeatDuration(const TimeSignatureChange& v) { return Beat::Zero(); }
constexpr Beat GetBeatDuration(const TempoChange& v) { return Beat::Zero(); }

template <typename T>
struct BeatSortedForwardIterator
{
	size_t LastIndex = 0;

	const T* Next(const std::vector<T>& sortedList, Beat nextBeat);
	inline T* Next(std::vector<T>& sortedList, Beat nextBeat) { return const_cast<T*>(Next(std::as_const(sortedList), nextBeat)); }
};

template <typename T>
struct BeatSortedList
{
	std::vector<T> Sorted;

public:
	T* TryFindLastAtBeat(Beat beat);
	T* TryFindExactAtBeat(Beat beat);
	const T* TryFindLastAtBeat(Beat beat) const;
	const T* TryFindExactAtBeat(Beat beat) const;

	T* TryFindOverlappingBeat(Beat beat);
	T* TryFindOverlappingBeat(Beat beatStart, Beat beatEnd);
	const T* TryFindOverlappingBeat(Beat beat) const;
	const T* TryFindOverlappingBeat(Beat beatStart, Beat beatEnd) const;

	void InsertOrUpdate(T valueToInsertOrUpdate);
	void RemoveAtBeat(Beat beatToFindAndRemove);
	void RemoveAtIndex(size_t indexToRemove);

	inline b8 empty() const { return Sorted.empty(); }
	inline auto begin() { return Sorted.begin(); }
	inline auto end() { return Sorted.end(); }
	inline auto begin() const { return Sorted.begin(); }
	inline auto end() const { return Sorted.end(); }
	inline T* data() { return Sorted.data(); }
	inline const T* data() const { return Sorted.data(); }
	inline size_t size() const { return Sorted.size(); }
	inline T& operator[](size_t index) { return Sorted[index]; }
	inline const T& operator[](size_t index) const { return Sorted[index]; }
};

struct TempoMapAccelerationStructure
{
	// NOTE: Pre calculated beat times up to the last tempo change
	std::vector<Time> BeatTickToTimes;
	std::vector<TempoChange> TempoBuffer;
	f64 FirstTempoBPM = 0.0, LastTempoBPM = 0.0;

	Time ConvertBeatToTimeUsingLookupTableIndexing(Beat beat) const;
	Beat ConvertTimeToBeatUsingLookupTableBinarySearch(Time time) const;

	Time GetLastCalculatedTime() const;
	void Rebuild(const TempoChange* inTempoChanges, size_t inTempoCount);
};

// NOTE: Used when no other tempo / time signature change is defined (empty list or pre-first beat)
constexpr Tempo FallbackTempo = Tempo(120.0f);
constexpr TimeSignature FallbackTimeSignature = TimeSignature(4, 4);

using SortedTempoChangesList = BeatSortedList<TempoChange>;
using SortedSignatureChangesList = BeatSortedList<TimeSignatureChange>;

struct SortedTempoMap
{
	// NOTE: These must always remain sorted and only have changes with (Beat.Ticks >= 0)
	SortedTempoChangesList Tempo;
	SortedSignatureChangesList Signature;
	TempoMapAccelerationStructure AccelerationStructure;

public:
	inline SortedTempoMap() { RebuildAccelerationStructure(); }

	// NOTE: Must manually be called every time a TempoChange has been edited otherwise Beat <-> Time conversions will be incorrect
	inline void RebuildAccelerationStructure() { AccelerationStructure.Rebuild(Tempo.data(), Tempo.size()); }
	inline Time BeatToTime(Beat beat) const { return AccelerationStructure.ConvertBeatToTimeUsingLookupTableIndexing(beat); }
	inline Beat TimeToBeat(Time time) const { return AccelerationStructure.ConvertTimeToBeatUsingLookupTableBinarySearch(time); }

	struct ForEachBeatBarData { TimeSignature Signature; Beat Beat; i32 BarIndex; b8 IsBar; };
	template <typename Func>
	inline void ForEachBeatBar(Func perBeatBarFunc) const
	{
		BeatSortedForwardIterator<TimeSignatureChange> signatureChangeIt {};
		Beat beatIt = {};

		for (i32 barIndex = 0; /*barIndex < MAX_BAR_COUNT*/; barIndex++)
		{
			const TimeSignatureChange* thisChange = signatureChangeIt.Next(Signature.Sorted, beatIt);
			const TimeSignature thisSignature = (thisChange == nullptr) ? FallbackTimeSignature : thisChange->Signature;

			const i32 beatsPerBar = thisSignature.GetBeatsPerBar();
			const Beat durationPerBeat = thisSignature.GetDurationPerBeat();
			const Beat durationPerBar = (durationPerBeat * beatsPerBar);

			if (perBeatBarFunc(ForEachBeatBarData { thisSignature, beatIt, barIndex, true }) == ControlFlow::Break)
				return;
			beatIt += durationPerBeat;

			for (i32 beatIndexWithinBar = 1; beatIndexWithinBar < beatsPerBar; beatIndexWithinBar++)
			{
				if (perBeatBarFunc(ForEachBeatBarData { thisSignature, beatIt, barIndex, false }) == ControlFlow::Break)
					return;
				beatIt += durationPerBeat;
			}
		}
	}
};

template <typename T>
inline const T* BeatSortedForwardIterator<T>::Next(const std::vector<T>& sortedList, Beat nextBeat)
{
	const T* next = nullptr;
	for (size_t i = LastIndex; i < sortedList.size(); i++)
	{
		if (Beat beat = GetBeat(sortedList[i]); beat <= nextBeat)
			next = &sortedList[i];
		else if (beat > nextBeat)
			break;
	}
	if (next != nullptr)
		LastIndex = ArrayItToIndex(next, &sortedList[0]);
	return next;
}

template <typename T>
T* BeatSortedList<T>::TryFindLastAtBeat(Beat beat)
{
	return const_cast<T*>(static_cast<const BeatSortedList<T>*>(this)->TryFindLastAtBeat(beat));
}

template <typename T>
T* BeatSortedList<T>::TryFindExactAtBeat(Beat beat)
{
	return const_cast<T*>(static_cast<const BeatSortedList<T>*>(this)->TryFindExactAtBeat(beat));
}

template <typename T>
const T* BeatSortedList<T>::TryFindLastAtBeat(Beat beat) const
{
	// TODO: Optimize using binary search
	if (Sorted.size() == 0)
		return nullptr;
	if (Sorted.size() == 1)
		return (GetBeat(Sorted[0]) <= beat) ? &Sorted[0] : nullptr;
	if (beat < GetBeat(Sorted[0]))
		return nullptr;
	for (size_t i = 0; i < Sorted.size() - 1; i++)
	{
		if (GetBeat(Sorted[i]) <= beat && GetBeat(Sorted[i + 1]) > beat)
			return &Sorted[i];
	}
	return &Sorted.back();
}

template <typename T>
const T* BeatSortedList<T>::TryFindExactAtBeat(Beat beat) const
{
	// TODO: Optimize using binary search
	for (const T& v : Sorted)
	{
		if (GetBeat(v) == beat)
			return &v;
	}
	return nullptr;
}

template <typename T>
T* BeatSortedList<T>::TryFindOverlappingBeat(Beat beat)
{
	return const_cast<T*>(static_cast<const BeatSortedList<T>*>(this)->TryFindOverlappingBeat(beat));
}

template <typename T>
T* BeatSortedList<T>::TryFindOverlappingBeat(Beat beatStart, Beat beatEnd)
{
	return const_cast<T*>(static_cast<const BeatSortedList<T>*>(this)->TryFindOverlappingBeat(beatStart, beatEnd));
}

template <typename T>
const T* BeatSortedList<T>::TryFindOverlappingBeat(Beat beat) const
{
	return TryFindOverlappingBeat(beat, beat);
}

template <typename T>
const T* BeatSortedList<T>::TryFindOverlappingBeat(Beat beatStart, Beat beatEnd) const
{
	// TODO: Optimize using binary search
	const T* found = nullptr;
	for (const T& v : Sorted)
	{
		// NOTE: Only break after a large beat has been found to correctly handle long notes with other notes "inside"
		//		 (even if they should't be placable in the first place)
		if (GetBeat(v) <= beatEnd && beatStart <= (GetBeat(v) + GetBeatDuration(v)))
			found = &v;
		else if ((GetBeat(v) + GetBeatDuration(v)) > beatEnd)
			break;
	}
	return found;
}

template <typename T>
inline size_t LinearlySearchForInsertionIndex(const BeatSortedList<T>& sortedList, Beat beat)
{
	// TODO: Optimize using binary search
	for (size_t i = 0; i < sortedList.size(); i++)
		if (beat <= GetBeat(sortedList[i])) return i;
	return sortedList.size();
}

template <typename T>
inline b8 ValidateIsSortedByBeat(const BeatSortedList<T>& sortedList)
{
	return std::is_sorted(sortedList.begin(), sortedList.end(), [](const T& a, const T& b) { return GetBeat(a) < GetBeat(b); });
}

template <typename T>
void BeatSortedList<T>::InsertOrUpdate(T valueToInsertOrUpdate)
{
	const size_t insertionIndex = LinearlySearchForInsertionIndex(*this, GetBeat(valueToInsertOrUpdate));
	if (InBounds(insertionIndex, Sorted))
	{
		if (T& existing = Sorted[insertionIndex]; GetBeat(existing) == GetBeat(valueToInsertOrUpdate))
			existing = valueToInsertOrUpdate;
		else
			Sorted.insert(Sorted.begin() + insertionIndex, valueToInsertOrUpdate);
	}
	else
	{
		Sorted.push_back(valueToInsertOrUpdate);
	}

#if PEEPO_DEBUG
	assert(GetBeat(valueToInsertOrUpdate).Ticks >= 0);
	assert(ValidateIsSortedByBeat(*this));
#endif
}

template <typename T>
void BeatSortedList<T>::RemoveAtBeat(Beat beatToFindAndRemove)
{
	if (const T* foundAtBeat = TryFindExactAtBeat(beatToFindAndRemove); foundAtBeat != nullptr)
		RemoveAtIndex(ArrayItToIndex(foundAtBeat, &Sorted[0]));
}

template <typename T>
void BeatSortedList<T>::RemoveAtIndex(size_t indexToRemove)
{
	if (InBounds(indexToRemove, Sorted))
		Sorted.erase(Sorted.begin() + indexToRemove);
}
