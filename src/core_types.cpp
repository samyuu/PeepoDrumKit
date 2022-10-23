#include "core_types.h"
#include <stdio.h>
#include <time.h>

static_assert(BitsPerByte == 8);
static_assert((sizeof(u8) * BitsPerByte) == 8 && (sizeof(i8) * BitsPerByte) == 8);
static_assert((sizeof(u16) * BitsPerByte) == 16 && (sizeof(i16) * BitsPerByte) == 16);
static_assert((sizeof(u32) * BitsPerByte) == 32 && (sizeof(i32) * BitsPerByte) == 32);
static_assert((sizeof(u64) * BitsPerByte) == 64 && (sizeof(i64) * BitsPerByte) == 64);
static_assert((sizeof(f32) * BitsPerByte) == 32 && (sizeof(f64) * BitsPerByte) == 64);
static_assert((sizeof(b8) * BitsPerByte) == 8);

Rect FitInsideFixedAspectRatio(Rect rectToFitInside, f32 targetAspectRatio)
{
	static constexpr f32 roundingAdd = 0.0f; // 0.5f;

	const vec2 rectSize = rectToFitInside.GetSize();
	const f32 rectAspectRatio = (rectSize.x / rectSize.y);

	// NOTE: Taller than wide -> bars on top / bottom
	if (rectAspectRatio <= targetAspectRatio)
	{
		const f32 presentHeight = Round((rectSize.x / targetAspectRatio) + roundingAdd);
		const f32 barHeight = Round((rectSize.y - presentHeight) / 2.0f);
		rectToFitInside.TL.y += barHeight;
		rectToFitInside.BR.y += barHeight;
		rectToFitInside.BR.y = rectToFitInside.TL.y + presentHeight;
	}
	else // NOTE: Wider than tall -> bars on left / right
	{
		const f32 presentWidth = Floor((rectSize.y * targetAspectRatio) + roundingAdd);
		const f32 barWidth = Floor((rectSize.x - presentWidth) / 2.0f);
		rectToFitInside.TL.x += barWidth;
		rectToFitInside.BR.x += barWidth;
		rectToFitInside.BR.x = rectToFitInside.TL.x + presentWidth;
	}

	return rectToFitInside;
}

Rect FitInsideFixedAspectRatio(Rect rectToFitInside, vec2 targetSize)
{
	return FitInsideFixedAspectRatio(rectToFitInside, (targetSize.x / targetSize.y));
}

static constexpr Time RoundToMilliseconds(Time value) { return Time::FromSec((value.Seconds * 1000.0 + 0.5) * 0.001); }

i32 Time::ToString(char* outBuffer, size_t bufferSize) const
{
	assert(outBuffer != nullptr && bufferSize >= sizeof(FormatBuffer::Data));

	static constexpr Time maxDisplayableTime = Time::FromSec(3599.999999);
	static constexpr const char invalidFormatString[] = "--:--.---";

	const f64 msRoundSeconds = RoundToMilliseconds(Time::FromSec(Absolute(Seconds))).Seconds;
	if (::isnan(msRoundSeconds) || ::isinf(msRoundSeconds))
	{
		// NOTE: Array count of a string literal char array already accounts for the null terminator
		memcpy(outBuffer, invalidFormatString, ArrayCount(invalidFormatString));
		return static_cast<i32>(ArrayCount(invalidFormatString) - 1);
	}

	const f64 msRoundSecondsAbs = Min(msRoundSeconds, maxDisplayableTime.ToSec());
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

Time Time::FromString(cstr inBuffer)
{
	if (inBuffer == nullptr || inBuffer[0] == '\0')
		return Time::Zero();

	b8 isNegative = false;
	if (inBuffer[0] == '-') { isNegative = true; inBuffer++; }
	else if (inBuffer[0] == '+') { isNegative = false; inBuffer++; }

	i32 min = 0, sec = 0, ms = 0;
	sscanf_s(inBuffer, "%02d:%02d.%03d", &min, &sec, &ms);

	min = Clamp(min, 0, 59);
	sec = Clamp(sec, 0, 59);
	ms = Clamp(ms, 0, 999);

	const f64 outSeconds = (static_cast<f64>(min) * 60.0) + static_cast<f64>(sec) + (static_cast<f64>(ms) * 0.001);
	return Time::FromSec(isNegative ? -outSeconds : outSeconds);
}

Date Date::GetToday()
{
	const time_t inTimeNow = ::time(nullptr); tm outDateNow;
	const errno_t timeToDateError = ::localtime_s(&outDateNow, &inTimeNow);
	if (timeToDateError != 0)
		return Date::Zero();

	Date result = {};
	result.Year = static_cast<i16>(outDateNow.tm_year + 1900);
	result.Month = static_cast<i8>(outDateNow.tm_mon + 1);
	result.Day = static_cast<i8>(outDateNow.tm_mday);
	return result;
}

Date TestDateToday = Date::GetToday();

i32 Date::ToString(char* outBuffer, size_t bufferSize, char separator) const
{
	assert(outBuffer != nullptr && bufferSize >= sizeof(FormatBuffer::Data));
	const u32 yyyy = Clamp<u32>(Year, 0, 9999);
	const u32 mm = Clamp<u32>(Month, 0, 12);
	const u32 dd = Clamp<u32>(Day, 0, 31);
	return sprintf_s(outBuffer, bufferSize, "%04u%c%02u%c%02u", yyyy, separator, mm, separator, dd);
}

Date::FormatBuffer Date::ToString(char separator) const
{
	FormatBuffer buffer;
	ToString(buffer.Data, ArrayCount(buffer.Data), separator);
	return buffer;
}

Date Date::FromString(cstr inBuffer, char separator)
{
	assert(separator == '/' || separator == '-' || separator == '_');
	char formatString[] = "%04u_%02u_%02u";
	formatString[4] = separator;
	formatString[9] = separator;

	u32 yyyy = 0, mm = 0, dd = 0;
	sscanf_s(inBuffer, formatString, &yyyy, &mm, &dd);

	Date result = {};
	result.Year = static_cast<i16>(Clamp<u32>(yyyy, 0, 9999));
	result.Month = static_cast<i8>(Clamp<u32>(mm, 0, 12));
	result.Day = static_cast<i8>(Clamp<u32>(dd, 0, 31));
	return result;
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
	return Time::FromSec(static_cast<f64>(deltaTicks) / static_cast<f64>(Win32GlobalPerformanceCounter.TicksPerSecond));
}
