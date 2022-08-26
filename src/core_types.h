#pragma once
#include <stdint.h>
#include <limits>
#include <cfloat>
#include <cmath>

using i8 = int8_t;
using u8 = uint8_t;

using i16 = int16_t;
using u16 = uint16_t;

using i32 = int32_t;
using u32 = uint32_t;

using i64 = int64_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

using b8 = bool;

constexpr i32 BitsPerByte = CHAR_BIT;

constexpr i8 I8Min = INT8_MIN;
constexpr i8 I8Max = INT8_MAX;
constexpr u8 U8Min = 0;
constexpr u8 U8Max = UINT8_MAX;

constexpr i16 I16Min = INT16_MIN;
constexpr i16 I16Max = INT16_MAX;
constexpr u16 U16Min = 0;
constexpr u16 U16Max = UINT16_MAX;

constexpr i32 I32Min = INT32_MIN;
constexpr i32 I32Max = INT32_MAX;
constexpr u32 U32Min = 0;
constexpr u32 U32Max = UINT32_MAX;

constexpr i64 I64Min = INT64_MIN;
constexpr i64 I64Max = INT64_MAX;
constexpr u64 U64Min = 0;
constexpr u64 U64Max = UINT64_MAX;

constexpr f32 F32Min = FLT_MIN;
constexpr f32 F32Max = FLT_MAX;

constexpr f64 F64Min = DBL_MIN;
constexpr f64 F64Max = DBL_MAX;

// NOTE: Null terminated sequence of characters (always assumed to be UTF-8)
using cstr = const char*;

#include <assert.h>
#include <type_traits>

// NOTE: Example: defer { DoEndOfScopeCleanup(); };
#ifndef defer
struct defer_dummy {};
template <class F> struct deferrer { F f; ~deferrer() { f(); } };
template <class F> __forceinline deferrer<F> operator*(defer_dummy, F f) { return { f }; }
#define DEFER_DETAIL(LINE) zz_defer##LINE
#define DEFER(LINE) DEFER_DETAIL(LINE)
#define defer auto DEFER(__LINE__) = ::defer_dummy {} *[&]()
#endif /* defer */

// NOTE: Example: ForceConsteval<ExampleConstexprHash("example")>
template <auto ConstexprExpression>
constexpr auto ForceConsteval = ConstexprExpression;

// NOTE: Specifically to be used with ForEachX(perXFunc) style iterator functions
enum class ControlFlow : u8 { Break, Continue };

// NOTE: Assumes the enum class EnumType { ..., Count }; convention to be used everywhere
template <typename EnumType>
constexpr size_t EnumCount = static_cast<size_t>(EnumType::Count);

template <typename EnumType>
constexpr i32 EnumCountI32 = static_cast<i32>(EnumType::Count);

template <typename EnumType>
constexpr __forceinline size_t EnumToIndex(EnumType enumValue)
{
	static_assert(std::is_enum_v<EnumType>);
	return static_cast<size_t>(enumValue);
}

// NOTE: Basically "++enum", specifically to be used inside for loops. Return void to not add any confusion with the inOut& param
//		 Example: for (CoolEnum enum = {}; enum < CoolEnum::Count; IncrementEnum(enum)) { ... }
template <typename EnumType>
constexpr __forceinline void IncrementEnum(EnumType& inOutEnum)
{
	static_assert(std::is_enum_v<EnumType>);
	inOutEnum = static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(inOutEnum) + 1);
}

// NOTE: Example: ArrayCount("example_string_literal")
template <typename ArrayType>
constexpr __forceinline size_t ArrayCount(const ArrayType& cStyleArray)
{
	static_assert(std::is_array_v<ArrayType>);
	return (sizeof(cStyleArray) / sizeof(cStyleArray[0]));
}

template <typename ArrayType>
constexpr __forceinline i32 ArrayCountI32(const ArrayType& cStyleArray)
{
	return static_cast<i32>(ArrayCount(cStyleArray));
}

template <typename IndexType, typename ArrayType>
constexpr __forceinline b8 InBounds(const IndexType index, const ArrayType& array)
{
	static_assert(std::is_integral_v<IndexType>);
	using SizeType = decltype(array.size());

	if constexpr (std::is_signed_v<IndexType>)
		return (index >= 0 && static_cast<SizeType>(index) < array.size());
	else
		return (index < array.size());
}

template <typename IndexType, typename ArrayType, typename DefaultType>
constexpr __forceinline auto IndexOr(const IndexType index, ArrayType& array, DefaultType defaultValue)
{
	return InBounds(index, array) ? array[index] : defaultValue;
}

template <typename IndexType, typename ArrayType>
constexpr __forceinline auto* IndexOrNull(const IndexType index, ArrayType& array)
{
	return InBounds(index, array) ? &array[index] : nullptr;
}

template <typename T>
constexpr __forceinline size_t ArrayItToIndex(const T* itemWithinArray, const T* arrayBegin)
{
	const ptrdiff_t distanceFromBegin = (itemWithinArray - arrayBegin);
	assert(distanceFromBegin >= 0);
	return static_cast<size_t>(distanceFromBegin);
}

template <typename T>
constexpr __forceinline i32 ArrayItToIndexI32(const T* itemWithinArray, const T* arrayBegin)
{
	return static_cast<i32>(ArrayItToIndex(itemWithinArray, arrayBegin));
}

// NOTE: Only to be used for temporary POD* function arguments
template<typename T>
struct PtrArg
{
	T Value;
	inline explicit PtrArg(T v) : Value(v) {}

	inline constexpr operator T*() { return &Value; }
	inline constexpr operator const T*() const { return &Value; }
};

struct NonCopyable
{
	NonCopyable() = default;
	~NonCopyable() = default;

	NonCopyable(const NonCopyable&) = delete;
	NonCopyable& operator=(const NonCopyable&) = delete;
};

// NOTE: Breaking regular PascalCase naming convention for these basic vector types
//		 to treat them more like built-in types and because it's just easier to type

struct ivec2
{
	i32 x, y;

	ivec2() = default;
	explicit constexpr ivec2(i32 scalar) : x(scalar), y(scalar) {}
	constexpr ivec2(i32 x, i32 y) : x(x), y(y) {}
	constexpr ivec2(const ivec2& other) : x(other.x), y(other.y) {}

	constexpr b8 operator==(const ivec2& other) const { return (x == other.x) && (y == other.y); }
	constexpr b8 operator!=(const ivec2& other) const { return !(*this == other); }

	constexpr ivec2 operator+(const ivec2& other) const { return { (x + other.x), (y + other.y) }; }
	constexpr ivec2 operator-(const ivec2& other) const { return { (x - other.x), (y - other.y) }; }
	constexpr ivec2 operator*(const ivec2& other) const { return { (x * other.x), (y * other.y) }; }
	constexpr ivec2 operator/(const ivec2& other) const { return { (x / other.x), (y / other.y) }; }
	constexpr ivec2 operator*(const i32 scalar) const { return { (x * scalar), (y * scalar) }; }
	constexpr ivec2 operator/(const i32 scalar) const { return { (x / scalar), (y / scalar) }; }

	constexpr ivec2& operator+=(const ivec2& other) { *this = (*this + other); return *this; }
	constexpr ivec2& operator-=(const ivec2& other) { *this = (*this - other); return *this; }
	constexpr ivec2& operator*=(const ivec2& other) { *this = (*this * other); return *this; }
	constexpr ivec2& operator/=(const ivec2& other) { *this = (*this / other); return *this; }
	constexpr ivec2& operator*=(const i32 scalar) { *this = (*this * scalar); return *this; }
	constexpr ivec2& operator/=(const i32 scalar) { *this = (*this / scalar); return *this; }
	constexpr ivec2 operator-() const { return { -x, -y }; }

	inline i32& operator[](size_t index) { return (&x)[index]; }
	inline i32 operator[](size_t index) const { return (&x)[index]; }

	inline i32* data() { return &x; }
	inline const i32* data() const { return &x; }
};

struct vec2
{
	f32 x, y;

	vec2() = default;
	explicit constexpr vec2(f32 scalar) : x(scalar), y(scalar) {}
	constexpr vec2(f32 x, f32 y) : x(x), y(y) {}
	constexpr vec2(const vec2& other) : x(other.x), y(other.y) {}
	explicit constexpr vec2(const ivec2& other) : x(static_cast<f32>(other.x)), y(static_cast<f32>(other.y)) {}

	constexpr vec2 operator+(const vec2& other) const { return { (x + other.x), (y + other.y) }; }
	constexpr vec2 operator-(const vec2& other) const { return { (x - other.x), (y - other.y) }; }
	constexpr vec2 operator*(const vec2& other) const { return { (x * other.x), (y * other.y) }; }
	constexpr vec2 operator/(const vec2& other) const { return { (x / other.x), (y / other.y) }; }
	constexpr vec2 operator*(const f32 scalar) const { return { (x * scalar), (y * scalar) }; }
	constexpr vec2 operator/(const f32 scalar) const { return { (x / scalar), (y / scalar) }; }

	constexpr vec2& operator+=(const vec2& other) { *this = (*this + other); return *this; }
	constexpr vec2& operator-=(const vec2& other) { *this = (*this - other); return *this; }
	constexpr vec2& operator*=(const vec2& other) { *this = (*this * other); return *this; }
	constexpr vec2& operator/=(const vec2& other) { *this = (*this / other); return *this; }
	constexpr vec2& operator*=(const f32 scalar) { *this = (*this * scalar); return *this; }
	constexpr vec2& operator/=(const f32 scalar) { *this = (*this / scalar); return *this; }
	constexpr vec2 operator-() const { return { -x, -y }; }

	inline f32& operator[](size_t index) { return (&x)[index]; }
	inline f32 operator[](size_t index) const { return (&x)[index]; }

	inline f32* data() { return &x; }
	inline const f32* data() const { return &x; }
};

struct RGBA8
{
	u8 R, G, B, A;

	RGBA8() = default;
	explicit constexpr RGBA8(u8 scalar) : R(scalar), G(scalar), B(scalar), A(scalar) {}
	constexpr RGBA8(u8 x, u8 y, u8 z, u8 w) : R(x), G(y), B(z), A(w) {}
	constexpr RGBA8(const RGBA8& other) = default;
};

constexpr f32 PI = 3.14159265358979323846f;
constexpr f32 DegreesToRadians = 0.01745329251994329577f;
constexpr f32 RadiansToDegrees = 57.2957795130823208768f;

struct Angle
{
	f32 Radians;

	constexpr f32 ToRadians() const { return Radians; }
	constexpr f32 ToDegrees() const { return Radians * RadiansToDegrees; }
	static constexpr Angle FromRadians(f32 radians) { return Angle { radians }; }
	static constexpr Angle FromDegrees(f32 degrees) { return Angle { degrees * DegreesToRadians }; }

	constexpr Angle operator+(const Angle other) const { return { (Radians + other.Radians) }; }
	constexpr Angle operator-(const Angle other) const { return { (Radians - other.Radians) }; }
	constexpr Angle operator*(const Angle other) const { return { (Radians * other.Radians) }; }
	constexpr Angle operator/(const Angle other) const { return { (Radians / other.Radians) }; }
	constexpr Angle operator*(const f32 scalar) const { return { (Radians * scalar) }; }
	constexpr Angle operator/(const f32 scalar) const { return { (Radians / scalar) }; }

	constexpr Angle& operator+=(const Angle other) { *this = (*this + other); return *this; }
	constexpr Angle& operator-=(const Angle other) { *this = (*this - other); return *this; }
	constexpr Angle& operator*=(const Angle other) { *this = (*this * other); return *this; }
	constexpr Angle& operator/=(const Angle other) { *this = (*this / other); return *this; }
	constexpr Angle& operator*=(const f32 scalar) { *this = (*this * scalar); return *this; }
	constexpr Angle& operator/=(const f32 scalar) { *this = (*this / scalar); return *this; }
	constexpr Angle operator-() const { return { -Radians }; }
};

static_assert(sizeof(ivec2) == sizeof(i32[2]));
static_assert(sizeof(vec2) == sizeof(f32[2]));
static_assert(sizeof(RGBA8) == sizeof(u8[4]));
static_assert(sizeof(Angle) == sizeof(f32));

struct Rect
{
	vec2 TL;
	vec2 BR;

	Rect() = default;
	constexpr Rect(vec2 tl, vec2 br) : TL(tl), BR(br) {}
	static constexpr Rect FromTLSize(vec2 tl, vec2 size) { return Rect(tl, tl + size); }
	static constexpr Rect FromCenterSize(vec2 center, vec2 size) { vec2 half = (size * 0.5f); return Rect(center - half, center + half); }

	constexpr vec2 GetCenter() const { return (TL + BR) * 0.5f; }
	constexpr vec2 GetSize() const { return (BR - TL); }
	constexpr f32 GetWidth() const { return (BR.x - TL.x); }
	constexpr f32 GetHeight() const { return (BR.y - TL.y); }
	constexpr f32 GetArea() const { return (BR.x - TL.x) * (BR.y - TL.y); }
	constexpr vec2 GetTL() const { return TL; }
	constexpr vec2 GetTR() const { return vec2(BR.x, TL.y); }
	constexpr vec2 GetBL() const { return vec2(TL.x, BR.y); }
	constexpr vec2 GetBR() const { return BR; }
	constexpr vec2 GetMin() const { return vec2(((BR.x < TL.x) ? BR.x : TL.x), ((BR.y < TL.y) ? BR.y : TL.y)); }
	constexpr vec2 GetMax() const { return vec2(((TL.x < BR.x) ? BR.x : TL.x), ((TL.y < BR.y) ? BR.y : TL.y)); }

	constexpr b8 Contains(vec2 point) const { return (point.x >= TL.x) && (point.y >= TL.y) && (point.x < BR.x) && (point.y < BR.y); }
	constexpr b8 Contains(const Rect& rect) const { return (rect.TL.x >= TL.x) && (rect.TL.y >= TL.y) && (rect.BR.x <= BR.x) && (rect.BR.y <= BR.y); }
	constexpr b8 Overlaps(const Rect& rect) const { return (rect.TL.y < BR.y) && (rect.BR.y > TL.y) && (rect.TL.x < BR.x) && (rect.BR.x > TL.x); }
};

inline f32 Floor(f32 value) { return ::floorf(value); }
inline f64 Floor(f64 value) { return ::floor(value); }
inline f32 Round(f32 value) { return ::roundf(value); }
inline f64 Round(f64 value) { return ::round(value); }
inline f32 Ceil(f32 value) { return ::ceilf(value); }
inline f64 Ceil(f64 value) { return ::ceil(value); }
inline vec2 Floor(vec2 value) { return { Floor(value.x), Floor(value.y) }; }
inline vec2 Round(vec2 value) { return { Round(value.x), Round(value.y) }; }
inline vec2 Ceil(vec2 value) { return { Ceil(value.x), Ceil(value.y) }; }

inline f32 Mod(f32 value, f32 modulo) { return ::fmodf(value, modulo); }
inline f64 Mod(f64 value, f64 modulo) { return ::fmod(value, modulo); }

constexpr u32 RoundUpToPowerOfTwo(u32 v) { v--; v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16; v++; return v; }

inline f32 Sin(Angle value) { return ::sinf(value.Radians); }
inline f32 Cos(Angle value) { return ::cosf(value.Radians); }

inline Angle Atan2(f32 y, f32 x) { return Angle::FromRadians(::atan2f(y, x)); }
inline Angle AngleBetween(vec2 pointA, vec2 pointB) { return Atan2(pointA.y - pointB.y, pointA.x - pointB.x); }

inline vec2 Rotate(vec2 point, f32 sin, f32 cos) { return vec2((point.x * cos) - (point.y * sin), (point.x * sin) + (point.y * cos)); }
inline vec2 Rotate(vec2 point, Angle angle) { return Rotate(point, Sin(angle), Cos(angle)); }
inline vec2 RotateAround(vec2 point, vec2 pivot, Angle angle) { return Rotate(point - pivot, angle) + pivot; }

inline i32 Sign(i32 value) { return (value < 0) ? -1 : (value > 0) ? 1 : 0; }
inline f32 Sign(f32 value) { return (value < 0.0f) ? -1.0f : (value > 0.0f) ? 1.0f : 0.0f; }
inline f64 Sign(f64 value) { return (value < 0.0) ? -1.0 : (value > 0.0) ? 1.0 : 0.0; }
inline i8  Absolute(i8  value) { return (value >= static_cast<i8>(0)) ? value : -value; }
inline i16 Absolute(i16 value) { return (value >= static_cast<i16>(0)) ? value : -value; }
inline i32 Absolute(i32 value) { return (value >= static_cast<i32>(0)) ? value : -value; }
inline i64 Absolute(i64 value) { return (value >= static_cast<i64>(0)) ? value : -value; }
inline f32 Absolute(f32 value) { return ::fabsf(value); }
inline f64 Absolute(f64 value) { return ::fabs(value); }

inline b8 ApproxmiatelySame(f32 a, f32 b, f32 threshold = 0.0001f) { return Absolute(a - b) < threshold; }
inline b8 ApproxmiatelySame(f64 a, f64 b, f64 threshold = 0.0001) { return Absolute(a - b) < threshold; }
inline b8 ApproxmiatelySame(vec2 a, vec2 b, f32 threshold = 0.0001f) { return ApproxmiatelySame(a.x, b.x, threshold) && ApproxmiatelySame(a.y, b.y, threshold); }

constexpr f32 ToPercent(f32 value) { return (value * 100.0f); }
constexpr f32 FromPercent(f32 percent) { return (percent * 0.01f); }

template <typename T> constexpr T Min(T a, T b) { return (b < a) ? b : a; }
template <> constexpr ivec2 Min<ivec2>(ivec2 a, ivec2 b) { return { Min(a.x, b.x), Min(a.y, b.y) }; }
template <> constexpr vec2 Min<vec2>(vec2 a, vec2 b) { return { Min(a.x, b.x), Min(a.y, b.y) }; }

template <typename T> constexpr T Max(T a, T b) { return (a < b) ? b : a; }
template <> constexpr ivec2 Max<ivec2>(ivec2 a, ivec2 b) { return { Max(a.x, b.x), Max(a.y, b.y) }; }
template <> constexpr vec2 Max<vec2>(vec2 a, vec2 b) { return { Max(a.x, b.x), Max(a.y, b.y) }; }

template <typename T> constexpr T Clamp(T value, T min, T max) { return Min<T>(Max<T>(value, min), max); }
template <typename T> constexpr T ClampBot(T value, T min) { return Max<T>(value, min); }
template <typename T> constexpr T ClampTop(T value, T max) { return Min<T>(value, max); }

template <typename T>
constexpr T Lerp(T start, T end, f32 t) { return (1.0f - t) * start + (t * end); }

template <typename T>
constexpr T LerpClamped(T start, T end, f32 t) { return Lerp<T>(start, end, Clamp(t, 0.0f, 1.0f)); }

template <typename T>
constexpr T ConvertRange(T oldStart, T oldEnd, T newStart, T newEnd, T value) { return (newStart + ((value - oldStart) * (newEnd - newStart) / (oldEnd - oldStart))); }

// NOTE: It's easy to accidentally misuse these in cases where (end < start) resulting in "incorrect" clamps
template <typename T>
constexpr T ConvertRangeClampInput(T oldStart, T oldEnd, T newStart, T newEnd, T value) { return ConvertRange<T>(oldStart, oldEnd, newStart, newEnd, Clamp<T>(value, oldStart, oldEnd)); }
template <typename T>
constexpr T ConvertRangeClampOutput(T oldStart, T oldEnd, T newStart, T newEnd, T value) { return Clamp<T>(ConvertRange<T>(oldStart, oldEnd, newStart, newEnd, value), newStart, newEnd); }

#if 0 // TODO: Do all this fun stuff
inline vec2 Normalize(vec2 value) { ...; }
inline f32 Dot(vec2 a, vec2 b) { ... }
inline f32 Length(vec2 value) { ... }
inline f32 LengthSqr(vec2 value) { ... }
inline f32 Distance(f32 a, f32 b) { return Absolute(a - b); }
inline f32 Distance(vec2 a, vec2 b) { ... }
inline vec2 LookAtDirection(vec2 from, vec2 target) { return Normalize(target - from); }
inline vec2 Reflect(vec2 value, vec2 normal) { ... }
#endif

// NOTE: Just a quick wrapper for std::remove_if from <algorithm>, also breaking naming convention because this should really just be part of the standard
template <typename T, typename Pred>
inline void erase_remove_if(T& container, Pred pred)
{
	container.erase(std::remove_if(container.begin(), container.end(), pred), container.end());
}

struct Time
{
	f64 Seconds;

	Time() = default;
	constexpr explicit Time(f64 sec) : Seconds(sec) {}

	static constexpr Time Zero() { return Time(0.0); }
	static constexpr Time FromMinutes(f64 min) { return Time(min * 60.0); }
	static constexpr Time FromSeconds(f64 sec) { return Time(sec); }
	static constexpr Time FromMilliseconds(f64 ms) { return Time(ms / 1000.0); }
	static constexpr Time FromFrames(f64 frames, f64 fps = 60.0) { return Time(frames / fps); }

	constexpr f64 TotalMinutes() const { return Seconds / 60.0; }
	constexpr f64 TotalSeconds() const { return Seconds; }
	constexpr f64 TotalMilliseconds() const { return Seconds * 1000.0; }
	constexpr f32 ToFrames(f32 fps = 60.0f) const { return static_cast<f32>(Seconds * static_cast<f64>(fps)); }

	constexpr b8 operator==(const Time& other) const { return Seconds == other.Seconds; }
	constexpr b8 operator!=(const Time& other) const { return Seconds != other.Seconds; }
	constexpr b8 operator<=(const Time& other) const { return Seconds <= other.Seconds; }
	constexpr b8 operator>=(const Time& other) const { return Seconds >= other.Seconds; }
	constexpr b8 operator<(const Time& other) const { return Seconds < other.Seconds; }
	constexpr b8 operator>(const Time& other) const { return Seconds > other.Seconds; }

	constexpr Time operator+(const Time other) const { return FromSeconds(Seconds + other.Seconds); }
	constexpr Time operator-(const Time other) const { return FromSeconds(Seconds - other.Seconds); }

	constexpr Time& operator+=(const Time& other) { Seconds += other.Seconds; return *this; }
	constexpr Time& operator-=(const Time& other) { Seconds -= other.Seconds; return *this; }

	constexpr Time operator*(f64 other) const { return FromSeconds(Seconds * other); }
	constexpr Time operator*(i32 other) const { return FromSeconds(Seconds * other); }

	constexpr f64 operator/(Time other) const { return Seconds / other.Seconds; }
	constexpr f64 operator/(f64 other) const { return Seconds / other; }
	constexpr f64 operator/(i32 other) const { return Seconds / other; }

	constexpr Time operator+() const { return Time(+Seconds); }
	constexpr Time operator-() const { return Time(-Seconds); }

	// NOTE: Enough to store "(-)mm:ss.fff"
	struct FormatBuffer { char Data[12]; };
	i32 ToString(char* outBuffer, size_t bufferSize) const;
	FormatBuffer ToString() const;
	static Time FromString(cstr inBuffer);
};

struct Date
{
	// NOTE: This struct is only intended for quick string formatting and version checks so the storage format matches that of how a human would write it
	i16 Year; // NOTE: [0000-9999]
	i8 Month; // NOTE: [01-12]
	i8 Day;   // NOTE: [01-31]

	static constexpr Date Zero() { return Date {}; }
	constexpr b8 operator==(const Date& o) const { return (Year == o.Year) && (Month == o.Month) && (Day == o.Day); }
	constexpr b8 operator!=(const Date& o) const { return !(*this == o); }
	constexpr b8 operator<=(const Date& o) const { return (*this < o) || (*this == o); }
	constexpr b8 operator>=(const Date& o) const { return (*this > o) || (*this == o); }
	constexpr b8 operator<(const Date& o) const { return (Year < o.Year || Year == o.Year && (Month < o.Month || Month == o.Month && (Day < o.Day))); }
	constexpr b8 operator>(const Date& o) const { return !(*this < o) && (*this != o); }

	static Date GetToday();

	// NOTE: Enough to store "yyyy/MM/dd"
	struct FormatBuffer { char Data[12]; };
	i32 ToString(char* outBuffer, size_t bufferSize, char separator) const;
	FormatBuffer ToString(char separator = '/') const;
	static Date FromString(cstr inBuffer, char separator = '/');
};

static_assert(sizeof(Date) == sizeof(u32));

struct CPUTime
{
	i64 Ticks;

	CPUTime() = default;
	constexpr explicit CPUTime(i64 ticks) : Ticks(ticks) {}

	static CPUTime GetNow();
	static CPUTime GetNowAbsolute();
	static Time DeltaTime(const CPUTime& startTime, const CPUTime& endTime);
};

struct CPUStopwatch
{
	b8 IsRunning = false;
	CPUTime StartTime = {};

	void Start() { if (!IsRunning) { StartTime = CPUTime::GetNow(); IsRunning = true; } }
	Time Restart() { const Time elapsed = Stop(); Start(); return elapsed; }
	Time Stop() { const Time elapsed = GetElapsed(); IsRunning = false; StartTime = {}; return elapsed; }
	Time GetElapsed() const { return IsRunning ? CPUTime::DeltaTime(StartTime, CPUTime::GetNow()) : Time::Zero(); }

	static CPUStopwatch StartNew() { CPUStopwatch out; out.Start(); return out; }
};

enum class Endianness : u8
{
	Little,
	Big,
	Native = Little,
	Count
};

struct SemanticVersion
{
	u32 Major;
	u32 Minor;
	u32 Patch;
};
