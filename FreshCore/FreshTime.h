/*
 *  FreshTime.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 7/17/08.
 *  Copyright 2008 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_TIME_H_INCLUDED_
#define FRESH_TIME_H_INCLUDED_

#include <ctime>
#include <string>

namespace fr
{

	// The following functions are for more elaborate timing work, and attempt, on platforms where it is possible,
	// to deal in terms of "clocks" or high-performance system time measurements.

	typedef unsigned long long SystemClock;

	SystemClock getAbsoluteTimeClocks();

	double clocksToSeconds( SystemClock clocks );
	SystemClock secondsToClocks( double seconds );

	double getAbsoluteTimeSeconds();

	void accurateSleep( SystemClock clocks );

	// Functions for converting times to and from a standard string time format: YYYY-MM-DD HH:MM:SS
	std::string getStandardTimeDisplay( const std::time_t& time );
	std::time_t timeFromStandardFormat( const std::string& timeString );
	

	// Simple, helpful profiler.
	
	class CodeTimer
	{
	public:
		
		explicit CodeTimer( const std::string& label ) : m_label( label ), m_startTime( getAbsoluteTimeClocks() ) {}
		
		void report();
		
	private:
		
		std::string m_label;
		SystemClock m_startTime;
	};
	
#if DEV_MODE
	
#	define	START_TIMER( label ) CodeTimer timer_( #label );
#	define REPORT_TIMER() timer_.report();
	
#else
	
#	define	START_TIMER( label )
#	define REPORT_TIMER()
	
#endif
	
}

#endif
