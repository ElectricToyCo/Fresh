/*
 *  FreshTime.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 7/17/08.
 *  Copyright 2008 jeffwofford.com. All rights reserved.
 *
 */

#include "FreshTime.h"
#include "FreshDebug.h"
#include "FreshException.h"
#include <sstream>
#include <iomanip>
#include <chrono>
using namespace std::chrono;

namespace fr
{
	double getAbsoluteTimeSeconds()
	{
		return clocksToSeconds( getAbsoluteTimeClocks() );
	}

	double clocksToSeconds( SystemClock clocks )
	{
		return duration_cast< duration< double >>( nanoseconds( clocks )).count();
	}
	
	SystemClock secondsToClocks( double seconds )
	{
		return duration_cast< nanoseconds >( duration< double >( seconds )).count();
	}

	SystemClock getAbsoluteTimeClocks()
	{
		static auto start = high_resolution_clock::now();
		
		auto now = high_resolution_clock::now() - start;
		return duration_cast< nanoseconds >( now ).count();
	}

	void accurateSleep( SystemClock clocks )
	{
		const SystemClock targetWakeTime = getAbsoluteTimeClocks() + clocks;
		
		while( getAbsoluteTimeClocks() < targetWakeTime )
		{
			// Do nothing. This is BAD, BAD operating system citizenship.
		}
	}

	std::string getStandardTimeDisplay( const std::time_t& time )
	{
		// Format YYYY-MM-DD HH:MM:SS
		
		std::ostringstream result;
		struct tm* timeElements = std::localtime( &time );
		
		result << ( 1900 + timeElements->tm_year ) << "-";
		result << std::setw( 2 ) << std::setfill( '0' ) << ( timeElements->tm_mon + 1 ) << "-";
		result << std::setw( 2 ) << std::setfill( '0' ) << timeElements->tm_mday << " ";
		result << std::setw( 2 ) << std::setfill( '0' ) << timeElements->tm_hour << "-";
		result << std::setw( 2 ) << std::setfill( '0' ) << timeElements->tm_min << "-";
		result << std::setw( 2 ) << std::setfill( '0' ) << timeElements->tm_sec;
		
		return result.str();
	}

	std::time_t timeFromStandardFormat( const std::string& timeString )
	{
		struct std::tm tm{ 0 };
		auto matchedFields = std::sscanf( timeString.c_str(), "%4d-%2d-%2d %2d-%2d-%2d",
					&tm.tm_year,
					&tm.tm_mon,
					&tm.tm_mday,
					&tm.tm_hour,
					&tm.tm_min,
					&tm.tm_sec );
		
		if( matchedFields != 6 )
		{
			FRESH_THROW( FreshException, "Invalid time string format: '" << timeString << "'." );
		}
		
		tm.tm_year -= 1900;
		tm.tm_mon -= 1;
		
		tm.tm_isdst = -1;
		
		return std::mktime( &tm );		
	}
	
	void CodeTimer::report()
	{
		const auto durationMilliseconds = clocksToSeconds( getAbsoluteTimeClocks() - m_startTime ) * 1000.0;
		release_trace( m_label << " took " << std::fixed << std::setprecision( 3 ) << durationMilliseconds << "ms" );
	}

}
