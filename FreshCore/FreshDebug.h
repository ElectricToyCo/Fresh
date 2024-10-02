// Copyright 2006 Jeff Wofford all rights reserved.
//
// Includes handy functions for asserting, verifying, promising, and requiring expressions. Also offers a simple trace() macro (platform independent) for sending text to the debug window.
//

#ifndef FRESH_DEBUG_H_INCLUDED
#define FRESH_DEBUG_H_INCLUDED

#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>
#include <functional>

#ifdef _WIN32
#	ifdef _MSC_VER
#		include <crtdbg.h>
#	else
#		define _ASSERT(expr) ((void)0)

#		define _ASSERTE(expr) ((void)0)
#	endif
#else
#	include <unistd.h>		// For getpid()
#	include <signal.h>		// For kill()
#endif

#include "CoreCppCompatability.h"

#if ANDROID		// Android's assert() is lame. Replace it with something sensible.
#	define FRESH_IMPLEMENTS_OWN_ASSERT 1
#endif

#define fresh_assert assert

#if FRESH_IMPLEMENTS_OWN_ASSERT

#	include "FreshException.h"

#	undef fresh_assert
#	define fresh_assert( e ) ((e) ? ((void)0) : ( fr::assertionFailure( __FILE__, __LINE__, __func__, #e )))

namespace fr
{
	FRESH_DEFINE_EXCEPTION_TYPE( ExceptionAssertionFailed )
	void assertionFailure( const char* file, int line, const char* func, const char* message );
}

#endif

#ifdef DEBUG

#	define ASSERT fresh_assert
#	define ASSERT_MSG( expr, msg )	if( !( expr )) { dev_trace( "ASSERTION FAILED in " << FRESH_CURRENT_FUNCTION << ": " << msg ); ASSERT( expr ); }
#	define VERIFY ASSERT

#else

	// For ((void)0) see http://stackoverflow.com/questions/2198950/why-is-void-0-a-no-operation-in-c-and-c
#	define ASSERT( x ) ((void)0)
#	define ASSERT_MSG( expr, msg ) ((void)0)
#	define VERIFY( x ) (x)

#endif

#define REQUIRES ASSERT
#define PROMISES ASSERT

namespace fr
{

	// Ease-of-use string creation macro
	//
#define createString( expr ) ([&](){ std::ostringstream s; s << expr; return s.str(); }())

	// LOGGING FUNCTIONALITY
	//
	namespace DevLog
	{
		size_t numWarnings();
		size_t numErrors();
		void resetErrorAndWarningCount();

		void logTrace( const char* szFile, int line, const char* szContext, const char* szMessage, std::ostream& stream = std::cout );
		void logWarning( const char* szFile, int line, const char* szContext, const char* szMessage );
		void logError( const char* szFile, int line, const char* szContext, const char* szMessage );

		void setLogStream( std::unique_ptr< std::ostream >&& log );
		void flushLogStream();
	}

#	define release_trace( message ) { std::ostringstream ss; ss << message;	\
	fr::DevLog::logTrace( __FILE__, __LINE__, FRESH_CURRENT_FUNCTION, ss.str().c_str() ); }

#	define release_warning( message ) { std::ostringstream ss; ss << message;	\
	fr::DevLog::logWarning( __FILE__, __LINE__, FRESH_CURRENT_FUNCTION, ss.str().c_str() ); }

#	define release_error( message ) { std::ostringstream ss; ss << message;	\
	fr::DevLog::logError( __FILE__, __LINE__, FRESH_CURRENT_FUNCTION, ss.str().c_str() ); }


#if DEV_MODE
#	define dev_trace( message ) release_trace( message )
#	define dev_warning( message ) release_warning( message )
#	define dev_error( message ) release_error( message )
#else
#	define dev_trace( message )
#	define dev_warning( message )
#	define dev_error( message )
#endif

	// "SOFT" DEBUG BREAKPOINT SUPPORT
	//
	class Object;
	namespace Breakpoint
	{
		typedef std::string Tag;
		typedef const std::string& TagRef;

		void set( TagRef tag, const std::string& classNameFilter, const std::string& objectNameFilter );
		void clear( TagRef tag );
		void each( std::function< void( TagRef tag, const std::string& classNameFilter, const std::string& objectNameFilter ) >&& fn );
		bool matches( TagRef tag, const Object& object );
	}

#if DEV_MODE && FRESH_BREAKPOINTS_ENABLED
#	define checkDebugBreakpoint( tag, obj )	if( fr::Breakpoint::matches( tag, obj )) DebugBreakpoint();
#else
#	define checkDebugBreakpoint( tag, obj )
#endif

}	// end namespace fr

// DEBUG BREAKPOINT
//
// Defined as a macro to encourage the debugger to keep the source view at the call point.
//
#ifdef _WIN32
#	define DebugBreakpoint() __debugbreak()
#else
#	define DebugBreakpoint() ::raise( SIGINT );
#endif


#endif
