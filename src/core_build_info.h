#pragma once
#include "core_types.h"

namespace BuildInfo
{
	// NOTE: YOINK https://stackoverflow.com/questions/17739390/different-format-of-date-macro/70567530#70567530
	constexpr const char TimeFormattedAs_mm_hh_ss[] = __TIME__;
	constexpr const char DateFormattedAs_yyyy_MM_dd[] =
	{
		// Full year
		__DATE__[7], __DATE__[8], __DATE__[9], __DATE__[10],
		'/',

		// First month letter, Oct Nov Dec = '1' otherwise '0'
		(__DATE__[0] == 'O' || __DATE__[0] == 'N' || __DATE__[0] == 'D') ? '1' : '0',
		// Second month letter
		(__DATE__[0] == 'J') ? ((__DATE__[1] == 'a') ? '1' :       // Jan, Jun or Jul
								 ((__DATE__[2] == 'n') ? '6' : '7')) :
		(__DATE__[0] == 'F') ? '2' :                                // Feb 
		(__DATE__[0] == 'M') ? (__DATE__[2] == 'r') ? '3' : '5' :   // Mar or May
		(__DATE__[0] == 'A') ? (__DATE__[1] == 'p') ? '4' : '8' :   // Apr or Aug
		(__DATE__[0] == 'S') ? '9' :                                // Sep
		(__DATE__[0] == 'O') ? '0' :                                // Oct
		(__DATE__[0] == 'N') ? '1' :                                // Nov
		(__DATE__[0] == 'D') ? '2' :                                // Dec
		0,
		'/',

		// First day letter, replace space with digit
		__DATE__[4] == ' ' ? '0' : __DATE__[4],
		// Second day letter
		__DATE__[5],
	   '\0'
	};

	constexpr const char* CompilationTime() { return TimeFormattedAs_mm_hh_ss; }
	constexpr const char* CompilationDate() { return DateFormattedAs_yyyy_MM_dd; }
	constexpr const char* BuildConfiguration() { return PEEPO_DEBUG ? "Debug" : PEEPO_RELEASE ? "Release" : "Unknown"; }
}
