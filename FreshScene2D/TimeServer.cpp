//
//  TimeServer.cpp
//  Fresh
//
//  Created by Jeff Wofford on 7/24/12.
//  Copyright (c) 2012 jeffwofford.com. All rights reserved.
//

#include "TimeServer.h"
#include "Objects.h"

namespace fr
{
	const char* TimeServer::SCHEDULED = "Scheduled";

	FRESH_DEFINE_CLASS( CallbackScheduler )
	
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( CallbackScheduler )
	
	void CallbackScheduler::scheduleCallback( TimeType when, EventDispatcher::CallbackFunctionAbstract::ptr fnCallback )
	{
		REQUIRES( fnCallback );
		
		ASSERT( !m_isTraversingScheduledCallbacks );
		
		m_scheduledCallbacks.push_back( { when, fnCallback });
	}
	
	void CallbackScheduler::cancelCallback( EventDispatcher::CallbackFunctionAbstract::ptr fnCallback )
	{
		REQUIRES( fnCallback );
		
		ASSERT( !m_isTraversingScheduledCallbacks );
		
		m_isTraversingScheduledCallbacks = true;
		for( auto iter = m_scheduledCallbacks.begin(); iter != m_scheduledCallbacks.end(); ++iter )
		{
			if( fnCallback == iter->second )
			{
				m_scheduledCallbacks.erase( iter );
				break;
			}
		}
		m_isTraversingScheduledCallbacks = false;
	}

	bool CallbackScheduler::hasScheduledCallback( CallbackFunctionAbstract::ptr fnCallback )
	{
		for( auto iter = m_scheduledCallbacks.begin(); iter != m_scheduledCallbacks.end(); ++iter )
		{
			if( fnCallback == iter->second )
			{
				return true;
			}
		}
		return false;
	}
	
	size_t CallbackScheduler::scheduleCallback( TimeType when, std::function< void() >&& callback )
	{
		ASSERT( !m_isTraversingScheduledCallbacks );
		
		const auto callbackId = m_nFunctionCallbacksCreated;
		m_nFunctionCallbacksCreated += 1;
		
		m_scheduledFunctionCallbacks[ callbackId ] = { when, std::move( callback ) };
		
		PROMISES( hasScheduledCallback( callbackId ));
		return callbackId;
	}
	
	void CallbackScheduler::cancelCallback( size_t callbackId )
	{
		ASSERT( !m_isTraversingScheduledCallbacks );

		const auto iter = m_scheduledFunctionCallbacks.find( callbackId );
		if( iter != m_scheduledFunctionCallbacks.end() )
		{
			m_scheduledFunctionCallbacks.erase( iter );
		}

		PROMISES( !hasScheduledCallback( callbackId ));
	}
	
	bool CallbackScheduler::hasScheduledCallback( size_t callbackId )
	{
		return m_scheduledFunctionCallbacks.find( callbackId ) != m_scheduledFunctionCallbacks.end();
	}
	
	void CallbackScheduler::updateScheduledCallbacks( TimeType now )
	{
		ASSERT( !m_isTraversingScheduledCallbacks );
		
		Event eventCallback( TimeServer::SCHEDULED, this );
		
		// Make a copy of the list so that the main list is freed-up for getting fiddled with during traversal.
		//
		
		auto listCallbacks = m_scheduledCallbacks;
		
		auto beginning = listCallbacks.begin();
		
		for( ; beginning != listCallbacks.end(); ++beginning )
		{
			if( beginning->second )
			{
				if( beginning->first <= now )
				{
					// Call the callback.
					//
					(*beginning->second)( &eventCallback );
				}
			}			
		}
		
		// Erase expired and null callbacks.
		//
		
		beginning = m_scheduledCallbacks.begin();
		auto result = beginning;
		
		for( ; beginning != m_scheduledCallbacks.end(); ++beginning )
		{
			if( beginning->second )
			{
				if( beginning->first > now )
				{
					// Non-null, non-expired callback. Keep it.
					//
					*result++ = *beginning;				
				}
			}			
		}
		
		m_scheduledCallbacks.erase( result, m_scheduledCallbacks.end() );
		
		updateScheduledFunctionCallbacks( now );
	}
	
	void CallbackScheduler::updateScheduledFunctionCallbacks( TimeType now )
	{
		for( auto iter = m_scheduledFunctionCallbacks.begin(); iter != m_scheduledFunctionCallbacks.end(); /* increment within */ )
		{
			const auto& idTimeFunction = *iter;
			const auto& timeFunction = idTimeFunction.second;
			const auto& time = timeFunction.first;
			const auto& function = timeFunction.second;

			if( time <= now )
			{
				// Call the callback.
				//
				function();
				
				// Clear it from the list.
				//
				iter = m_scheduledFunctionCallbacks.erase( iter );
			}
			else
			{
				++iter;
			}
		}
	}
	
	////////////////////////////////////////////////////
	
	void TimeServer::scheduleCallback( EventDispatcher::CallbackFunctionAbstract::ptr fnCallback, TimeType delay )
	{
		REQUIRES( fnCallback );
		REQUIRES( delay >= 0 );
		ASSERT( m_scheduler );
		m_scheduler->scheduleCallback( time() + delay, fnCallback );
	}
	
	void TimeServer::cancelCallback( EventDispatcher::CallbackFunctionAbstract::ptr fnCallback )
	{		
		REQUIRES( fnCallback );
		ASSERT( m_scheduler );
		m_scheduler->cancelCallback( fnCallback );
	}

	bool TimeServer::hasScheduledCallback( EventDispatcher::CallbackFunctionAbstract::ptr fnCallback )
	{
		REQUIRES( fnCallback );
		ASSERT( m_scheduler );
		return m_scheduler->hasScheduledCallback( fnCallback );
	}

	size_t TimeServer::scheduleCallback( TimeType delay, std::function< void() >&& callback )
	{
		REQUIRES( delay >= 0 );
		const auto result = m_scheduler->scheduleCallback( time() + delay, std::move( callback ));
		PROMISES( hasScheduledCallback( result ));
		return result;
	}
	
	void TimeServer::cancelCallback( size_t callbackId )
	{
		m_scheduler->cancelCallback( callbackId );
		PROMISES( !hasScheduledCallback( callbackId ));
	}
	
	bool TimeServer::hasScheduledCallback( size_t callbackId )
	{
		ASSERT( m_scheduler );
		return m_scheduler->hasScheduledCallback( callbackId );
	}

	void TimeServer::updateScheduledCallbacks()
	{
		ASSERT( m_scheduler );
		m_scheduler->updateScheduledCallbacks( time() );
	}
	
}

