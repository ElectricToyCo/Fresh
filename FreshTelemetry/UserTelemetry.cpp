//
//  UserTelemetry.cpp
//  Fresh
//
//  Created by Jeff Wofford on 5/16/12.
//  Copyright (c) 2012 jeffwofford.com. All rights reserved.
//

#include "UserTelemetry.h"
#include "FreshTime.h"
#include "FreshEssentials.h"
#include "CommandProcessor.h"
#include <vector>
#include <sstream>
#include <iomanip>
#include <ctime>

#include "curl/curl.h"

namespace
{
	
	const char MORATORIUM_KEY[] = "telemetry_moratorium_expiration";
	
	class ResultBuffer
	{
	public:
		
		static size_t writeMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
		{
			size_t realsize = size * nmemb;
			
			if( realsize > 0 )
			{
				ASSERT( userp );
				ResultBuffer& resultBuffer = *reinterpret_cast< ResultBuffer* >( userp );
				auto& buffer = resultBuffer.m_buffer;
				
				size_t newBegin = buffer.size();
				buffer.resize( buffer.size() + realsize );
				
				std::memcpy( reinterpret_cast< void* >( &( buffer[ newBegin ])), contents, realsize );
			}
			
			return realsize;
		}
		
		std::string result() const
		{
			std::string strBuffer;
			strBuffer.assign( m_buffer.begin(), m_buffer.end() );			
			return strBuffer;
		}
		
	private:
		
		std::vector< char > m_buffer;
	};
	
	
	std::string httpPost( const char* url, const char* fields, bool post = false )
	{				
		CURL* curl = curl_easy_init();
		ASSERT( curl );
			
		if( post )
		{
			// POST
			curl_easy_setopt( curl, CURLOPT_POSTFIELDS, fields );
			curl_easy_setopt( curl, CURLOPT_POSTFIELDSIZE, (long) strlen( fields ));
			curl_easy_setopt( curl, CURLOPT_URL, url );
		}
		else
		{
			// GET
			std::string extendedUrl( url );
			extendedUrl += "?";
			extendedUrl += fields;
			curl_easy_setopt( curl, CURLOPT_URL, extendedUrl.c_str() );
		}
		
		curl_easy_setopt( curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
		
		ResultBuffer buffer;
		
		curl_easy_setopt( curl, CURLOPT_WRITEDATA, reinterpret_cast< void* >( &buffer ));
		curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, ResultBuffer::writeMemoryCallback );			
		curl_easy_setopt( curl, CURLOPT_TIMEOUT, 4 );
		
		CURLcode result = curl_easy_perform( curl );

		curl_easy_cleanup( curl );
		
		if( result == 0 )
		{
			return buffer.result();
		}
		
		return "";
	}	
	
	size_t curlWriteNoOp(void *contents, size_t size, size_t nmemb, void *userp )
	{
		return size * nmemb;
	}
	
	CURL* httpPostAsync( CURLM* curlMulti, const char* url, const char* fields, bool post = false )
	{
		ASSERT( curlMulti );
		
		CURL* curl = curl_easy_init();
		ASSERT( curl );

		if( post )
		{
			// POST
			curl_easy_setopt( curl, CURLOPT_POSTFIELDS, fields );
			curl_easy_setopt( curl, CURLOPT_POSTFIELDSIZE, (long) strlen( fields ));
			curl_easy_setopt( curl, CURLOPT_URL, url );
		}
		else
		{
			// GET
			std::string extendedUrl( url );
			extendedUrl += "?";
			extendedUrl += fields;
			curl_easy_setopt( curl, CURLOPT_URL, extendedUrl.c_str() );
		}
		
		curl_easy_setopt( curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
		curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, curlWriteNoOp );			
		
		// Add the easy to the multi.
		//
		curl_multi_add_handle( curlMulti, curl );
		
		return curl;
	}
	
	void runAndCleanupCurlMultiEasies( std::vector< CURL* >& curlEasies, CURLM* curlMulti, bool forceCleanupEasies = false )
	{
		ASSERT( curlMulti );

		int nRunningHandles;
		curl_multi_perform( curlMulti, &nRunningHandles );

		if( nRunningHandles == 0 || forceCleanupEasies )
		{
			for( auto iterCurl = curlEasies.begin(); iterCurl != curlEasies.end(); ++iterCurl )
			{
				curl_multi_remove_handle( curlMulti, *iterCurl );
				curl_easy_cleanup( *iterCurl );
			}
			
			curlEasies.clear();
		}
	}
	
}

namespace fr
{
	class UserTelemetryImpl
	{
	public:
		CURLM* curlMulti = nullptr;
		std::vector< CURL* > curlEasies;
	};

	UserTelemetry::UserTelemetry( string_ref url )
	:	Singleton< UserTelemetry >( this )
	,	m_serverUrl( url )
	,	m_session( INVALID_ID )
	,	m_lastSession( INVALID_ID )
	,	m_sessionStartTime( -1.0 )
	,	m_impl( new UserTelemetryImpl() )
	{
		REQUIRES( m_serverUrl.empty() == false );
		curl_global_init( CURL_GLOBAL_NOTHING );
		
		m_impl->curlMulti = curl_multi_init();
	}
	
	UserTelemetry::~UserTelemetry()
	{
		if( INVALID_ID != m_session )
		{
			endSession();
		}
		
		runAndCleanupCurlMultiEasies( m_impl->curlEasies, m_impl->curlMulti, true );
		curl_multi_cleanup( m_impl->curlMulti );
	}
	
	void UserTelemetry::startSession( string_ref username, string_ref platform, string_ref os, string_ref deviceID, string_ref versionBuild )
	{
		REQUIRES( INVALID_ID == m_session );
		
		if( isUnderMoratorium() )
		{
			return;
		}
		
		// Prepare HTTP POST fields.
		//
		std::ostringstream fieldBuilder;
		fieldBuilder << "username=" << urlEncode( username );
		fieldBuilder << "&platform=" << urlEncode( platform );
		fieldBuilder << "&os=" << urlEncode( os );
		fieldBuilder << "&device=" << urlEncode( deviceID );
		fieldBuilder << "&version=" << urlEncode( versionBuild );

		// Post the data via HTTP.
		//
		const std::string& strResult = httpPost(( m_serverUrl + "/startSession.php" ).c_str(), fieldBuilder.str().c_str() );
		
		trace( "Telemetry server returned result '" << strResult << "'." );
		
		m_session = INVALID_ID;
		
		if( !strResult.empty() )
		{
			std::istringstream resultParser( strResult );
			
			// Check for moratorium information.
			//
			if( strResult.find( "moratorium" ) != std::string::npos )
			{
				std::string token;
				resultParser >> token;
				
				if( !( token == "moratorium" ))
				{
					trace( "Telemetry startSession encountered unexpected moratorium output. Result: " << strResult );
				}
				else
				{
					// Relative or absolute moratorium?
					//
					resultParser >> std::ws;
					if( resultParser.peek() == '@' )
					{
						// Absolute.
						//
						resultParser.get();
						
						std::string moratoriumEndDate, moratoriumEndTime;			// YYYY-MM-DD HH:MM:SS
						resultParser >> moratoriumEndDate >> std::ws >> moratoriumEndTime;

						try
						{
							auto time = timeFromStandardFormat( moratoriumEndDate + moratoriumEndTime );
							scheduleMoratoriumAbsolute( time );
						}
						catch( ... )
						{
							trace( "Telemetry moratorium absolute date/time had invalid format. Result: " << strResult );
						}
					}
					else
					{
						// Relative. Number of minutes from now.
						//
						unsigned int minutes;
						resultParser >> minutes;
						
						scheduleMoratoriumFromNow( minutes );
					}
				}
			}
			else
			{
				// Parse result for session id.
				//
				resultParser >> m_session;
				
				if( resultParser.fail() )
				{
					trace( "Telemetry startSession failed. Result: " << strResult );
					m_session = INVALID_ID;
				}
			}
		}
		
		if( m_session != INVALID_ID )
		{
			m_sessionStartTime = getAbsoluteTimeSeconds();
			trace( "Telemetry began session " << m_session << " at time " << m_sessionStartTime );
		}
	}
	
	void UserTelemetry::endSession()
	{
		if( m_session == INVALID_ID )
		{
			return;
		}

		// Prepare HTTP POST fields.
		//
		std::ostringstream fieldBuilder;
		fieldBuilder << "session=" << m_session;
		
		// Post the data via HTTP.
		//
		m_impl->curlEasies.push_back( httpPostAsync( m_impl->curlMulti, ( m_serverUrl + "/endSession.php" ).c_str(), fieldBuilder.str().c_str() ));
		runAndCleanupCurlMultiEasies( m_impl->curlEasies, m_impl->curlMulti );
		
		m_session = INVALID_ID;
		m_sessionStartTime = -1.0;
	}
	
	void UserTelemetry::pauseSession()
	{
		if( m_session == INVALID_ID )
		{
			return;
		}

		m_lastSession = m_session;
		
		endSession();
	}
	
	void UserTelemetry::resumeSession()
	{
		if( m_session == INVALID_ID )
		{
			return;
		}
		
		REQUIRES( m_lastSession != INVALID_ID );
		
		// Prepare HTTP POST fields.
		//
		std::ostringstream fieldBuilder;
		fieldBuilder << "resumeSession=" << m_lastSession;
		
		// Post the data via HTTP.
		//
		const std::string& strResult = httpPost(( m_serverUrl + "/startSession.php" ).c_str(), fieldBuilder.str().c_str() );
		
		if( !strResult.empty() )
		{
			// Parse result for session id.
			//
			std::istringstream resultParser( strResult );
			resultParser >> m_session;
			
			if( resultParser.fail() )
			{
				m_session = INVALID_ID;
			}
		}
	}
	
	void UserTelemetry::enterContext( string_ref name )
	{
		if( m_session == INVALID_ID )
		{
			return;
		}
		
		REQUIRES( name.empty() == false );

		// Prepare HTTP POST fields.
		//
		std::ostringstream fieldBuilder;
		fieldBuilder << "name=" << urlEncode( name );
		fieldBuilder << "&session=" << m_session;
		
		if( !m_contexts.empty() )
		{
			fieldBuilder << "&parent=" << m_contexts.top();
		}
		
		
		// Post the data via HTTP.
		//
		const std::string& strResult = httpPost(( m_serverUrl + "/enterContext.php" ).c_str(), fieldBuilder.str().c_str() );
		
		if( !strResult.empty() )
		{
			// Parse result for session id.
			//
			std::istringstream resultParser( strResult );
			id context;
			resultParser >> context;
			
			if( !resultParser.fail() )
			{
				m_contexts.push( context );
			}
		}
	}
	
	void UserTelemetry::leaveContext()
	{
		if( m_session == INVALID_ID )
		{
			return;
		}
		
		if( m_contexts.empty() )	// Failed to acquire a context earlier.
		{
			return;
		}

		// Prepare HTTP POST fields.
		//
		std::ostringstream fieldBuilder;
		fieldBuilder << "context=" << m_contexts.top();
		
		// Post the data via HTTP.
		//
		m_impl->curlEasies.push_back( httpPostAsync( m_impl->curlMulti, ( m_serverUrl + "/leaveContext.php" ).c_str(), fieldBuilder.str().c_str() ));
		runAndCleanupCurlMultiEasies( m_impl->curlEasies, m_impl->curlMulti );
		
		m_contexts.pop();
	}
	
	void UserTelemetry::registerEvent( string_ref type, string_ref subjectType, string_ref subjectName, string_ref comment )
	{
		registerEventLocalized( type, subjectType, subjectName, comment );
	}
	
	void UserTelemetry::registerEvent( string_ref type, float x, float y, float z, string_ref subjectType, string_ref subjectName, string_ref comment )
	{
		registerEventLocalized( type, subjectType, subjectName, comment, &x, &y, &z );
	}

	void UserTelemetry::registerEventLocalized( string_ref type, string_ref subjectType, string_ref subjectName, string_ref comment, float* x, float* y, float* z )
	{
		if( !m_eventsEnabled || m_session == INVALID_ID )
		{
			return;
		}
		
		REQUIRES( type.empty() == false );

		// Calculate the event time and composite it into a string.
		//
		const double time = getAbsoluteTimeSeconds();
		const double totalSeconds = time - m_sessionStartTime;
		
		std::ostringstream strTime;
		strTime << std::fixed << std::setprecision( 2 ) << totalSeconds;
		
		// Prepare HTTP POST fields.
		//
		std::ostringstream fieldBuilder;
		fieldBuilder << "session=" << m_session;
		fieldBuilder << "&time=" << strTime.str();
		fieldBuilder << "&type=" << urlEncode( type );
		
		if( m_contexts.empty() == false )
		{
			fieldBuilder << "&context=" << m_contexts.top();
		}
		
		if( subjectType.empty() == false )
		{
			ASSERT( subjectName.empty() == false );
			fieldBuilder << "&subjectType=" << urlEncode( subjectType );
			fieldBuilder << "&subjectName=" << urlEncode( subjectName );
		}

		if( comment.empty() == false )
		{
			fieldBuilder << "&comment=" << urlEncode( comment );
		}
		
		if( x ) fieldBuilder << "&x=" << *x;
		if( y ) fieldBuilder << "&y=" << *y;
		if( z ) fieldBuilder << "&z=" << *z;
		
		// Post the data via HTTP.
		//
		m_impl->curlEasies.push_back( httpPostAsync( m_impl->curlMulti, ( m_serverUrl + "/registerEvent.php" ).c_str(), fieldBuilder.str().c_str() ));
		runAndCleanupCurlMultiEasies( m_impl->curlEasies, m_impl->curlMulti );
	}
	
	bool UserTelemetry::isUnderMoratorium() const
	{
		std::string moratoriumExpirationTime;
		bool foundPreference = loadPreference( MORATORIUM_KEY, moratoriumExpirationTime );
		if( foundPreference )
		{
			std::istringstream strTime( moratoriumExpirationTime );
			time_t time;
			strTime >> time;
			
			time_t secondsSinceJan_1_1970 = std::time( NULL );
			return time > secondsSinceJan_1_1970;
		}
		else
		{
			return false;
		}
	}
	
	void UserTelemetry::scheduleMoratoriumAbsolute( std::time_t secondsSince1970 )
	{
		// Record this moment as the next time we're allowed to contact the server.
		//
		std::ostringstream strTime;
		strTime << secondsSince1970;
		savePreference( MORATORIUM_KEY, strTime.str().c_str() );
		syncPreferences();
	}
				
	void UserTelemetry::scheduleMoratoriumFromNow( std::time_t durationMinutes )
	{
		// Get the current date/time.
		//
		time_t secondsSinceJan_1_1970 = std::time( NULL );
		secondsSinceJan_1_1970 += durationMinutes * 60;		// Minutes to seconds.

		scheduleMoratoriumAbsolute( secondsSinceJan_1_1970 );
	}
	
}
