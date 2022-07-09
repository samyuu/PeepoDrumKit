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

	constexpr bool operator==(const Beat& other) const { return Ticks == other.Ticks; }
	constexpr bool operator!=(const Beat& other) const { return Ticks != other.Ticks; }
	constexpr bool operator<=(const Beat& other) const { return Ticks <= other.Ticks; }
	constexpr bool operator>=(const Beat& other) const { return Ticks >= other.Ticks; }
	constexpr bool operator<(const Beat& other) const { return Ticks < other.Ticks; }
	constexpr bool operator>(const Beat& other) const { return Ticks > other.Ticks; }

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
constexpr auto IsTupletBarDivision(i32 gridBarDivision) -> bool { return (gridBarDivision % 3 == 0); };

struct Tempo
{
	f32 BPM = 0.0f;

	constexpr Tempo() = default;
	constexpr explicit Tempo(f32 bpm) : BPM(bpm) {}
};

struct TimeSignature
{
	i32 Numerator = 4;
	i32 Denominator = 4;

	constexpr TimeSignature() = default;
	constexpr TimeSignature(i32 numerator, i32 denominator) : Numerator(numerator), Denominator(denominator) {}

	constexpr i32 GetBeatsPerBar() const { return Numerator; }
	constexpr Beat GetDurationPerBeat() const { return Beat::FromBars(1) / Denominator; }
	constexpr Beat GetDurationPerBar() const { return GetDurationPerBeat() * GetBeatsPerBar(); }

	constexpr bool operator==(const TimeSignature& other) const { return (Numerator == other.Numerator) && (Denominator == other.Denominator); }
	constexpr bool operator!=(const TimeSignature& other) const { return (Numerator != other.Numerator) || (Denominator != other.Denominator); }
};

constexpr bool IsTimeSignatureSupported(TimeSignature v) { return (v.Numerator > 0 && v.Denominator > 0) && (Beat::FromBars(1).Ticks % v.Denominator) == 0; }

struct TempoChange
{
	static constexpr Tempo Default = Tempo(160.0);
	constexpr TempoChange() = default;
	constexpr TempoChange(Beat beat, Tempo tempo) : Beat(beat), Tempo(tempo) {}

	Beat Beat = {};
	Tempo Tempo = {};
};

struct TimeSignatureChange
{
	static constexpr TimeSignature Default = TimeSignature(4, 4);
	constexpr TimeSignatureChange() = default;
	constexpr TimeSignatureChange(Beat beat, TimeSignature signature) : Beat(beat), Signature(signature) {}

	Beat Beat = {};
	TimeSignature Signature = {};
};

// NOTE: Beat accessor function specifically to be used inside templates without coupling to a specific struct member name
constexpr Beat GetBeat(const TimeSignatureChange& v) { return v.Beat; }
constexpr Beat GetBeat(const TempoChange& v) { return v.Beat; }

template <typename T>
struct BeatSortedForwardIterator
{
	size_t LastIndex = 0;

	inline const T* Next(const std::vector<T>& sortedList, Beat nextBeat)
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

	inline T* Next(std::vector<T>& sortedList, Beat nextBeat) { return const_cast<T*>(Next(std::as_const(sortedList), nextBeat)); }
};

struct TempoMapAccelerationStructure
{
	// NOTE: Pre calculated beat times up to the last tempo change
	std::vector<Time> BeatTickToTimes;
	f64 FirstTempoBPM = 0.0, LastTempoBPM = 0.0;

	Time ConvertBeatToTimeUsingLookupTableIndexing(Beat beat) const;
	Beat ConvertTimeToBeatUsingLookupTableBinarySearch(Time time) const;

	Time GetLastCalculatedTime() const;
	void Rebuild(const std::vector<TempoChange>& tempoChanges);
};

struct SortedTempoMap
{
	// NOTE: These must always remain sorted, non-empty and only have changes with (Beat.Ticks >= 0)
	std::vector<TempoChange> TempoChanges;
	std::vector<TimeSignatureChange> SignatureChanges;
	TempoMapAccelerationStructure AccelerationStructure;

public:
	inline SortedTempoMap() { Reset(); }
	void Reset();

	void TempoInsertOrUpdate(TempoChange tempoChangeToInsertOrUpdate);
	void TempoRemoveAtIndex(size_t indexToRemove);
	inline void TempoRemoveAtBeat(Beat beatToFindAndRemove) { TempoRemoveAtIndex(TempoFindIndexWithExactBeat(beatToFindAndRemove)); }

	void SignatureInsertOrUpdate(TimeSignatureChange signatureChangeToInsertOrUpdate);
	void SignatureRemoveAtIndex(size_t indexToRemove);
	inline void SignatureRemoveAtBeat(Beat beatToFindAndRemove) { SignatureRemoveAtIndex(SignatureFindIndexWithExactBeat(beatToFindAndRemove)); }

	// NOTE: Must manually be called every time a TempoChange has been edited otherwise Beat <-> Time conversions will be incorrect
	inline void RebuildAccelerationStructure() { AccelerationStructure.Rebuild(TempoChanges); }
	inline Time BeatToTime(Beat beat) const { return AccelerationStructure.ConvertBeatToTimeUsingLookupTableIndexing(beat); }
	inline Beat TimeToBeat(Time time) const { return AccelerationStructure.ConvertTimeToBeatUsingLookupTableBinarySearch(time); }

public:
	size_t TempoFindLastIndexAtBeat(Beat beat) const;
	size_t TempoFindIndexWithExactBeat(Beat exactBeat) const;
	TempoChange& TempoFindLastAtBeat(Beat beat) { return TempoChanges[TempoFindLastIndexAtBeat(beat)]; }
	const TempoChange& TempoFindLastAtBeat(Beat beat) const { return TempoChanges[TempoFindLastIndexAtBeat(beat)]; }

	size_t SignatureFindLastIndexAtBeat(Beat beat) const;
	size_t SignatureFindIndexWithExactBeat(Beat exactBeat) const;
	TimeSignatureChange& SignatureFindLastAtBeat(Beat beat) { return SignatureChanges[SignatureFindLastIndexAtBeat(beat)]; }
	const TimeSignatureChange& SignatureFindLastAtBeat(Beat beat) const { return SignatureChanges[SignatureFindLastIndexAtBeat(beat)]; }

	struct ForEachBeatBarData { TimeSignature Signature; Beat Beat; i32 BarIndex; bool IsBar; };
	template <typename Func>
	inline void ForEachBeatBar(Func perBeatBarFunc) const
	{
		BeatSortedForwardIterator<TimeSignatureChange> signatureChangeIt {};
		Beat beatIt = {};

		for (i32 barIndex = 0; /*barIndex < MAX_BAR_COUNT*/; barIndex++)
		{
			const TimeSignatureChange* thisChange = signatureChangeIt.Next(SignatureChanges, beatIt);
			const TimeSignature thisSignature = (thisChange == nullptr) ? SignatureChanges[0].Signature : thisChange->Signature;

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
