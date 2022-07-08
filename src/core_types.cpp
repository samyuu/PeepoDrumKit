#include "core_types.h"
#include <stdio.h>

static constexpr Time RoundToMilliseconds(Time value) { return Time::FromSeconds((value.Seconds * 1000.0 + 0.5) * 0.001); }

i32 Time::ToString(char* outBuffer, size_t bufferSize) const
{
	assert(bufferSize >= sizeof(FormatBuffer::Data));

	static constexpr Time maxDisplayableTime = Time::FromSeconds(3599.999999);
	static constexpr const char invalidFormatString[] = "--:--.---";

	const f64 msRoundSeconds = RoundToMilliseconds(Time::FromSeconds(Absolute(Seconds))).Seconds;
	if (::isnan(msRoundSeconds) || ::isinf(msRoundSeconds))
	{
		// NOTE: Array count of a string literal char array already accounts for the null terminator
		memcpy(outBuffer, invalidFormatString, ArrayCount(invalidFormatString));
		return static_cast<i32>(ArrayCount(invalidFormatString) - 1);
	}

	const f64 msRoundSecondsAbs = Min(msRoundSeconds, maxDisplayableTime.TotalSeconds());
	const f64 min = Floor(Mod(msRoundSecondsAbs, 3600.0) / 60.0);
	const f64 sec = Mod(msRoundSecondsAbs, 60.0);
	const f64 ms = (sec - Floor(sec)) * 1000.0;

	const char signPrefix[2] = { (Seconds < 0.0) ? '-' : '\0', '\0' };
	return sprintf_s(outBuffer, bufferSize, "%s%02d:%02d.%03d", signPrefix, static_cast<i32>(min), static_cast<i32>(sec), static_cast<i32>(ms));
}

Time::FormatBuffer Time::ToString() const
{
	FormatBuffer buffer;
	ToString(buffer.Data, ArrayCount(buffer.Data));
	return buffer;
}

Time Time::FromString(const char* inBuffer)
{
	if (inBuffer == nullptr || inBuffer[0] == '\0')
		return Time::Zero();

	bool isNegative = false;
	if (inBuffer[0] == '-') { isNegative = true; inBuffer++; }
	else if (inBuffer[0] == '+') { isNegative = false; inBuffer++; }

	i32 min = 0, sec = 0, ms = 0;
	sscanf_s(inBuffer, "%02d:%02d.%03d", &min, &sec, &ms);

	min = Clamp(min, 0, 59);
	sec = Clamp(sec, 0, 59);
	ms = Clamp(ms, 0, 999);

	const f64 outSeconds = (static_cast<f64>(min) * 60.0) + static_cast<f64>(sec) + (static_cast<f64>(ms) * 0.001);
	return Time::FromSeconds(isNegative ? -outSeconds : outSeconds);
}

#include <Windows.h>

static i64 Win32GetPerformanceCounterTicksPerSecond()
{
	::LARGE_INTEGER frequency = {};
	::QueryPerformanceFrequency(&frequency);
	return frequency.QuadPart;
}

static i64 Win32GetPerformanceCounterTicksNow()
{
	::LARGE_INTEGER timeNow = {};
	::QueryPerformanceCounter(&timeNow);
	return timeNow.QuadPart;
}

static struct Win32PerformanceCounterData
{
	i64 TicksPerSecond = Win32GetPerformanceCounterTicksPerSecond();
	i64 TicksOnProgramStartup = Win32GetPerformanceCounterTicksNow();
} Win32GlobalPerformanceCounter = {};

CPUTime CPUTime::GetNow()
{
	return CPUTime { Win32GetPerformanceCounterTicksNow() - Win32GlobalPerformanceCounter.TicksOnProgramStartup };
}

CPUTime CPUTime::GetNowAbsolute()
{
	return CPUTime { Win32GetPerformanceCounterTicksNow() };
}

Time CPUTime::DeltaTime(const CPUTime& startTime, const CPUTime& endTime)
{
	const i64 deltaTicks = (endTime.Ticks - startTime.Ticks);
	return Time::FromSeconds(static_cast<f64>(deltaTicks) / static_cast<f64>(Win32GlobalPerformanceCounter.TicksPerSecond));
}
