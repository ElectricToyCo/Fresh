//
//  TimeServer.h
//  Fresh
//
//  Created by Jeff Wofford on 7/24/12.
//  Copyright (c) 2012 jeffwofford.com. All rights reserved.
//

#ifndef Fresh_TimeServer_h
#define Fresh_TimeServer_h

#include "FreshMath.h"
#include "EventDispatcher.h"
#include <unordered_map>

namespace fr
{
	
	class CallbackScheduler : public EventDispatcher
	{
	public:
		
		virtual void scheduleCallback( TimeType when, CallbackFunctionAbstract::ptr fnCallback );
		// REQUIRES( fnCallback );
		// REQUIRES( delay >= 0 );
		virtual void cancelCallback( CallbackFunctionAbstract::ptr fnCallback );
		// REQUIRES( fnCallback );
		// Does not require that the callback be previously scheduled. In this case it does nothing.

		virtual bool hasScheduledCallback( CallbackFunctionAbstract::ptr fnCallback );
		
		virtual size_t scheduleCallback( TimeType when, std::function< void() >&& callback );
		// REQUIRES( delay >= 0 );
		// PROMISES( hasScheduleCallack( result ));
		virtual void cancelCallback( size_t callbackId );
		// Does not require that the callback be previously scheduled. In this case it does nothing.
		// PROMISES( !hasScheduleCallack( result ));
		
		virtual bool hasScheduledCallback( size_t callbackId );

		virtual void updateScheduledCallbacks( TimeType now );

	protected:
		
		virtual void updateScheduledFunctionCallbacks( TimeType now );

	private:
		
		typedef std::pair< TimeType, CallbackFunctionAbstract::wptr > ScheduledCallback;		
		typedef std::pair< TimeType, std::function< void() >> ScheduledFunctionCallback;
		
		std::list< ScheduledCallback > m_scheduledCallbacks;		// The real is the scheduled time, not the delay
		std::unordered_map< size_t, ScheduledFunctionCallback > m_scheduledFunctionCallbacks;
		bool m_isTraversingScheduledCallbacks = false;
		
		size_t m_nFunctionCallbacksCreated = 0;
		
		FRESH_DECLARE_CLASS( CallbackScheduler, EventDispatcher )
	};
	
	///////////////////////////////////////////
	
	class TimeServer
	{
	public:
		
		static const char* SCHEDULED;		// Event type used by scheduled callbacks.
		
		explicit TimeServer( CallbackScheduler::ptr scheduler = nullptr ) : m_scheduler( scheduler ) {}
		
		virtual ~TimeServer() {}
		
		virtual TimeType time() const = 0;

		virtual void scheduleCallback( EventDispatcher::CallbackFunctionAbstract::ptr fnCallback, TimeType delay );
		// REQUIRES( fnCallback );
		// REQUIRES( delay >= 0 );
		virtual void cancelCallback( EventDispatcher::CallbackFunctionAbstract::ptr fnCallback );
		// REQUIRES( fnCallback );
		// Does not require that the callback be previously scheduled. In this case it does nothing.

		virtual bool hasScheduledCallback( EventDispatcher::CallbackFunctionAbstract::ptr fnCallback );

		virtual size_t scheduleCallback( TimeType delay, std::function< void() >&& callback );
		// REQUIRES( delay >= 0 );
		// PROMISES( hasScheduledCallback( result ));
		virtual void cancelCallback( size_t callbackId );
		// Does not require that the callback be previously scheduled. In this case it does nothing.
		// PROMISES( !hasScheduledCallback( callbackId ));

		virtual bool hasScheduledCallback( size_t callbackId );

		virtual void updateScheduledCallbacks();

	private:
		
		CallbackScheduler::ptr m_scheduler;
		
		FRESH_PREVENT_COPYING( TimeServer )
	};
}

#endif
