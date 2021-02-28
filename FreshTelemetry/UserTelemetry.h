//
//  UserTelemetry.h
//  Fresh
//
//  Created by Jeff Wofford on 5/16/12.
//  Copyright (c) 2012 jeffwofford.com. All rights reserved.
//

#ifndef Fresh_UserTelemetry_h
#define Fresh_UserTelemetry_h

#include "Singleton.h"
#include <stack>

namespace fr
{
	
	class UserTelemetry : public Singleton< UserTelemetry >
	{
	public:
		
		typedef int id;
		typedef std::string string;
		typedef const string& string_ref;
		static const id INVALID_ID = -1;
		
		UserTelemetry( string_ref url );
		~UserTelemetry();
		
		bool eventsEnabled() const					{ return m_eventsEnabled; }
		void eventsEnabled( bool enable )			{ m_eventsEnabled = enable; }
		
		void startSession( string_ref username, string_ref platform, string_ref os, string_ref deviceID, string_ref versionBuild );
		void endSession();
		void pauseSession();		// Records e.g. iPhone task switching
		void resumeSession();
		void enterContext( string_ref name );
		void leaveContext();
		void registerEvent( string_ref type, string_ref subjectType = "", string_ref subjectName = "", string_ref comment = "" );
		void registerEvent( string_ref type, float x, float y, float z, string_ref subjectType = "", string_ref subjectName = "", string_ref comment = "" );
		
	private:
		
		std::string m_serverUrl;
		id m_session;
		id m_lastSession;
		std::stack< id > m_contexts;
		double m_sessionStartTime;
		
		std::unique_ptr< class UserTelemetryImpl > m_impl;
		
		bool m_eventsEnabled = true;
		
		void registerEventLocalized( string_ref type, string_ref subjectType = "", string_ref subjectName = "", string_ref comment = "", float* x = nullptr, float* y = nullptr, float* z = nullptr );
		
		bool isUnderMoratorium() const;
		void scheduleMoratoriumFromNow( std::time_t durationMinutes );
		void scheduleMoratoriumAbsolute( std::time_t secondsSince1970 );
		
		FRESH_PREVENT_COPYING( UserTelemetry )
	};
	
}

#if FRESH_TELEMETRY_ENABLED

#	define TELEMETRY_CONTEXT( name ) if( fr::UserTelemetry::doesExist() ) fr::UserTelemetry::instance().enterContext( name ); 
#	define TELEMETRY_CONTEXT_LEAVE() if( fr::UserTelemetry::doesExist() ) fr::UserTelemetry::instance().leaveContext(); 
#	define TELEMETRY_EVENT( type ) if( fr::UserTelemetry::doesExist() ) fr::UserTelemetry::instance().registerEvent( type );
#	define TELEMETRY_EVENT_LOC( type, x, y ) if( fr::UserTelemetry::doesExist() ) fr::UserTelemetry::instance().registerEvent( type, x, y, 0 );
#	define TELEMETRY_EVENT_LOC3( type, x, y, z ) if( fr::UserTelemetry::doesExist() ) fr::UserTelemetry::instance().registerEvent( type, x, y, z );
#	define TELEMETRY_EVENT_LOC_SUBJECT( type, x, y, subjectType, subjectName ) if( fr::UserTelemetry::doesExist() ) fr::UserTelemetry::instance().registerEvent( type, x, y, 0, subjectType, subjectName );
#	define TELEMETRY_EVENT_SUBJECT( type, subjectType, subjectName ) if( fr::UserTelemetry::doesExist() ) fr::UserTelemetry::instance().registerEvent( type, subjectType, subjectName );

#else

#	define TELEMETRY_CONTEXT( name )
#	define TELEMETRY_CONTEXT_LEAVE()
#	define TELEMETRY_EVENT( type )
#	define TELEMETRY_EVENT_LOC( type, x, y )
#	define TELEMETRY_EVENT_LOC3( type, x, y, z )
#	define TELEMETRY_EVENT_LOC_SUBJECT( type, x, y, subjectType, subjectName )
#	define TELEMETRY_EVENT_SUBJECT( type, subjectType, subjectName )

#endif


#endif
