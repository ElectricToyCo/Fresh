//
//  FreshDebug.cpp
//  Fresh
//
//  Created by Jeff Wofford on 11/2/12.
//
//

#include "FreshDebug.h"
#include "FreshEssentials.h"
#include "FreshFile.h"
#include "FreshTime.h"
#include "Object.h"
#include <string>
#include <iomanip>
#include <unordered_map>

#if ANDROID
#	include <android/log.h>
#	define ANDROID_LOG( message ) __android_log_print( ANDROID_LOG_INFO, "Fresh", "%s", message )
#endif

namespace
{
	size_t g_nWarnings = 0;
	size_t g_nErrors = 0;
	
	std::unique_ptr< std::ostream > g_log;
	
#if DEV_MODE && FRESH_BREAKPOINTS_ENABLED
	std::unordered_map< std::string, fr::ObjectId > g_debugBreakpoints;
#endif
}

namespace fr
{
	
#if FRESH_IMPLEMENTS_OWN_ASSERT
	
	void assertionFailure( const char* file, int line, const char* func, const char* message )
	{
		fr::DevLog::logError( file, line, func, createString( "ASSERTION FAILED: '" << message << "'" ).c_str() );
		
		// See http://hg.mozilla.org/mozilla-central/file/98fa9c0cff7a/js/src/jsutil.cpp#l66 for a multi-platform assert() implementation supporting
		// debugger resuming.
		
		// TODO not sure if I want an exception or a signal. I'll do both.
		//
		::raise( SIGABRT );
		throw ExceptionAssertionFailed{ file, line, func, message };
	}
	
#endif		// FRESH_IMPLEMENTS_OWN_ASSERT
	
	namespace DevLog
	{
		size_t numWarnings()
		{
			return g_nWarnings;
		}
		size_t numErrors()
		{
			return g_nErrors;
		}
		void resetErrorAndWarningCount()
		{
			g_nWarnings = g_nErrors = 0;
		}
		
		void setLogStream( std::unique_ptr< std::ostream >&& log )
		{
			g_log = std::move( log );
		}
		
		void flushLogStream()
		{
			if( g_log )
			{
				g_log->flush();
			}
		}
		
		void logTrace( const char* szFile, int line, const char* szContext, const char* szMessage )
		{
			// Truncate the filename.
			//
			std::string fileName( szFile );
			auto slashPos = fileName.find_last_of( "/\\" );
			if( slashPos < fileName.size() )
			{
				fileName.erase( 0, slashPos + 1 );
			}
			
			std::ostringstream ss;
			ss << "[" << std::fixed << std::setprecision( 3 ) << getAbsoluteTimeSeconds() << "] {" << fileName << "(" << line << ") - " << szContext << "()} " << szMessage << std::endl;
#ifdef _WIN32
			OutputDebugString( ss.str().c_str() );
#elif ANDROID
			ANDROID_LOG( ss.str().c_str() );
#endif
			std::clog << ss.str();
			
			if( g_log )
			{
				*g_log << ss.str();
			}
		}
		
		void logWarning( const char* szFile, int line, const char* szContext, const char* szMessage )
		{
			logTrace( szFile, line, szContext, ( std::string( "FRESH WARNING: " ) + szMessage ).c_str());
			
			++g_nWarnings;
		}
		
		void logError( const char* szFile, int line, const char* szContext, const char* szMessage )
		{
			logTrace( szFile, line, szContext, ( std::string( "FRESH ERROR: " ) + szMessage ).c_str());
			
			++g_nErrors;
		}
	}
	
	namespace Breakpoint
	{
		void set( TagRef tag, const std::string& classNameFilter, const std::string& objectNameFilter )
		{
#if DEV_MODE && FRESH_BREAKPOINTS_ENABLED
			g_debugBreakpoints[ tag ] = ObjectId( classNameFilter, objectNameFilter );
#endif
		}
		
		void clear( TagRef tag )
		{
#if DEV_MODE && FRESH_BREAKPOINTS_ENABLED
			g_debugBreakpoints[ tag ] = ObjectId::NULL_OBJECT;
#endif
		}
		
		void each( std::function< void( TagRef tag, const std::string& classNameFilter, const std::string& objectNameFilter ) >&& fn )
		{
#if DEV_MODE && FRESH_BREAKPOINTS_ENABLED
			for( const auto& member : g_debugBreakpoints )
			{
				fn( member.first.c_str(), member.second.className(), member.second.objectName() );
			}
#endif
		}
		
		bool matches( TagRef tag, const Object& object )
		{
#if DEV_MODE && FRESH_BREAKPOINTS_ENABLED
			const auto& breakpoint = g_debugBreakpoints[ tag ];		// Might create the registration.
			return breakpoint && object.matchesFilters( breakpoint.className(), breakpoint.objectName() );
#else
			return false;
#endif
		}
	}
}

