/*
 *  EventDispatcher.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 7/2/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#include "EventDispatcher.h"
#include "Objects.h"
#include "Profiler.h"
#include "CommandProcessor.h"

namespace fr
{

	FRESH_DEFINE_CLASS( EventDispatcher )

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( EventDispatcher )
	
	void EventDispatcher::addEventListener( Event::TypeRef type, CallbackFunctionAbstract::ptr fnCallback, bool strongReference /* = false */ )
	{
		ASSERT( fnCallback );
		if( !hasEventListener( type, fnCallback ))
		{
			if( strongReference )
			{
				fnCallback->setHoldOwnerStrongly( true );
				
				m_listStrongListeners.push_back( std::make_pair( type, fnCallback ));
			}
			else
			{
				m_listWeakListeners.push_back( std::make_pair( type, fnCallback ));
			}
		}
	}

	void EventDispatcher::removeEventListener( Event::TypeRef type, CallbackFunctionAbstract::ptr fnCallback )
	{
		ASSERT( fnCallback );
		
		// Find the iterator to erase.
		//
		ListWeakListenersI iter = std::find( m_listWeakListeners.begin(), m_listWeakListeners.end(), std::make_pair( type, CallbackFunctionAbstract::wptr( fnCallback )) );
		if( iter != m_listWeakListeners.end() )
		{
			m_listWeakListeners.erase( iter );
		}
		else
		{
			ListStrongListenersI iterStrong = std::find( m_listStrongListeners.begin(), m_listStrongListeners.end(), std::make_pair( type, CallbackFunctionAbstract::ptr( fnCallback )) );
			if( iterStrong != m_listStrongListeners.end() )
			{
				( iterStrong->second )->setHoldOwnerStrongly( false );		
				m_listStrongListeners.erase( iterStrong );
			}
		}
	}

	bool EventDispatcher::hasEventListener( Event::TypeRef type, CallbackFunctionAbstract::cptr fnCallback ) const
	{
		// NOTE: We have to fiddle with const here (in a safe way) in order to allow the pair to be directly comparable to the stored pair (with pair::operator==()).
		
		const bool isWeak =  m_listWeakListeners.end() != std::find( m_listWeakListeners.begin(), m_listWeakListeners.end(),
																	std::make_pair( type, CallbackFunctionAbstract::wptr( const_cast< CallbackFunctionAbstract* >( fnCallback.get() )) ));
		
		return isWeak || m_listStrongListeners.end() != std::find( m_listStrongListeners.begin(), m_listStrongListeners.end(),
													   std::make_pair( type, CallbackFunctionAbstract::ptr( const_cast< CallbackFunctionAbstract* >( fnCallback.get() )) ));
	}

	void EventDispatcher::clearEventListeners()
	{
		m_listWeakListeners.clear();
		m_listStrongListeners.clear();
	}

	void EventDispatcher::clearEventListeners( Event::TypeRef type )
	{
		{
			ListWeakListenersI beginning = m_listWeakListeners.begin();
			ListWeakListenersI result = beginning;

			for( ; beginning != m_listWeakListeners.end(); ++beginning )
			{
				if( beginning->first != type )	// Doesn't match the type, so...
				{
					*result++ = *beginning;		// Stick it toward the front.
				}
			}

			m_listWeakListeners.erase( result, m_listWeakListeners.end() );
			
		}
		{
			ListStrongListenersI beginning = m_listStrongListeners.begin();
			ListStrongListenersI result = beginning;
			
			for( ; beginning != m_listStrongListeners.end(); ++beginning )
			{
				if( beginning->first != type )	// Doesn't match the type, so...
				{
					*result++ = *beginning;		// Stick it toward the front.
				}
				else
				{
					// Prepare to release this callback and callback owner
					( beginning->second )->setHoldOwnerStrongly( false );		
				}
			}
			
			m_listStrongListeners.erase( result, m_listStrongListeners.end() );	
		}
	}

	void EventDispatcher::debugPrintListeners() const
	{
		if( m_listWeakListeners.empty() == false )
		{
			trace( "Weak Listeners for " << toString() );
		}
		
		for( const auto& listener : m_listWeakListeners )
		{
			trace( "\t" << listener.first << " <= " << listener.second->owner()->toString() << "." << listener.second->toString() );
		}

		if( m_listStrongListeners.empty() == false )
		{
			trace( "Strong Listeners for " << toString() );
		}
		
		for( const auto& listener : m_listStrongListeners )
		{
			trace( "\t" << listener.first << " <= " << listener.second->owner()->toString() << "." << listener.second->toString() );
		}
	}
	
	void EventDispatcher::dispatchEvent( const Event* event )
	{
		TIMER_AUTO( EventDispatcher::dispatchEvent )

		// Maintain the list by removing null listeners.
		//
		clearNullEventListeners();

		if( !m_listWeakListeners.empty() )
		{
			ListWeakListeners listWeakListenersCopy = m_listWeakListeners;
			for( ListWeakListenersCI iter = listWeakListenersCopy.begin(); iter != listWeakListenersCopy.end(); ++iter )
			{
				if( event->type() == iter->first )
				{
					if( iter->second )
					{
						(*iter->second)( event );
					}
				}
			}
		}
		
		if( !m_listStrongListeners.empty() )
		{
			ListStrongListeners listStrongListenersCopy = m_listStrongListeners;
			for( ListStrongListenersCI iter = listStrongListenersCopy.begin(); iter != listStrongListenersCopy.end(); ++iter )
			{
				if( event->type() == iter->first )
				{
					ASSERT( iter->second );
					(*iter->second)( event );
				}
			}
		}
	}

	void EventDispatcher::clearNullEventListeners()
	{
		TIMER_AUTO( EventDispatcher::clearNullEventListeners )

		{
			ListWeakListenersI beginning = m_listWeakListeners.begin();
			ListWeakListenersI result = beginning;
			
			for( ; beginning != m_listWeakListeners.end(); ++beginning )
			{
				if( beginning->second )	// Non-null so...
				{
					*result++ = *beginning;		// Stick it toward the front (where things won't be deleted).
				}
			}
			
			m_listWeakListeners.erase( result, m_listWeakListeners.end() );
		}
		{
			ListStrongListenersI beginning = m_listStrongListeners.begin();
			ListStrongListenersI result = beginning;
			
			for( ; beginning != m_listStrongListeners.end(); ++beginning )
			{
				if( beginning->second )	// Non-null so...
				{
					*result++ = *beginning;		// Stick it toward the front (where things won't be deleted).
				}
			}
			
			m_listStrongListeners.erase( result, m_listStrongListeners.end() );
		}
	}
	
	void EventDispatcher::postLoad()
	{
		Super::postLoad();
		
		clearNullEventListeners();
		
		// Enforce strong ownership.
		//
		for( auto listenerPair : m_listStrongListeners )
		{
			listenerPair.second->setHoldOwnerStrongly( true );
		}
	}
}

