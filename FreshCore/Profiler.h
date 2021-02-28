// Copyright 2006 Jeff Wofford all rights reserved.
#pragma once

#include "Singleton.h"
#include <memory>

namespace fr
{

	class Profiler : public Singleton< Profiler >
	{
	public:

		class AutoTimer
		{
		public:
			AutoTimer( const char* szScope );
			//	REQUIRES( szScope != 0 && strlen( szScope ) > 0 );
			~AutoTimer();

		private:
			std::string m_szScope;
		};

		Profiler();
		~Profiler();
	
		void timerBegin( const char* szScope );
			//	REQUIRES( szScope != 0 && strlen( szScope ) > 0 );
		void timerEnd( const char* szScope );
			//	REQUIRES( szScope != 0 && strlen( szScope ) > 0 );

		void clear();

		void dumpAsText( std::ostream& stream ) const;

	private:
		std::unique_ptr< class ProfilerImpl > m_impl;
	};

#ifdef FRESH_PROFILER_ENABLED

	#define FR_PROFILER_CURRENT_FUNCTION __PRETTY_FUNCTION__

	#define TIMER_AUTO_FUNC	\
		fr::Profiler::AutoTimer auto_Timer_( FR_PROFILER_CURRENT_FUNCTION );

	#define TIMER_BEGIN( scope )	\
		fr::Profiler::instance().timerBegin( #scope );

	#define TIMER_END( scope )	\
		fr::Profiler::instance().timerEnd( #scope );

	#define TIMER_AUTO( scope )	\
		fr::Profiler::AutoTimer auto_Timer_( #scope );

#else

	#define TIMER_BEGIN( scope )
	#define TIMER_END( scope )
	#define TIMER_AUTO( scope )
	#define TIMER_AUTO_FUNC

#endif

}		// END namespace fr
