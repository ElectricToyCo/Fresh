/*
 *  EventTouch.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/30/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_EVENT_TOUCH_H_INCLUDED
#define FRESH_EVENT_TOUCH_H_INCLUDED

#include "Event.h"
#include "FreshVector.h"

namespace fr
{
	// A Touch event describes the state of an individual touch on the iPhone.
	// For multiple simultaneous touches, multiple EventTouch events are sent.
	//
	class EventTouch : public Event
	{
	public:
		
		typedef void* TouchId;
		
		static const char* TOUCH_BEGIN;
		static const char* TOUCH_MOVE;
		static const char* TOUCH_END;
		static const char* TOUCH_CANCELLED;
		static const char* WHEEL_MOVE;
		
		enum class TouchPhase		// Based on iPhone Touch phases. Not all seem useful.
		{
			Begin,
			Move,
			Stationary,
			End,
			Cancelled,
			WheelMove
		};
		
		EventTouch( TypeRef type_,
				   TouchId touchId_,
				   TouchPhase touchPhase_,
				   unsigned int tapCount_,
				   const vec2& location_,
				   const vec2& previousLocation_,
				   int nTouches_,
				   int iThisTouch_,
				   const vec2& wheelDelta = vec2::ZERO,
				   Object::ptr target = nullptr,
				   Object::ptr currentTarget = nullptr )
		:	Event( type_, target, currentTarget )
		,	m_touchId( touchId_ )
		,	m_touchPhase( touchPhase_ )
		,	m_tapCount( tapCount_ )
		,	m_location( location_ )
		,	m_previousLocation( previousLocation_ )
		,	m_wheelDelta( wheelDelta )
		,	m_nTouches( nTouches_ )
		,	m_iThisTouch( iThisTouch_ )		
		{}
		
		EventTouch( const EventTouch& other, Object::ptr target_, const vec2& location_, const vec2& previousLocation_ )
		:	Event( other, target_ )
		,	m_touchId( other.m_touchId )
		,	m_touchPhase( other.m_touchPhase )
		,	m_tapCount( other.m_tapCount )
		,	m_location( location_ )
		,	m_previousLocation( previousLocation_ )
		,	m_wheelDelta( other.m_wheelDelta )
		,	m_nTouches( other.m_nTouches )
		,	m_iThisTouch( other.m_iThisTouch )
		{}
		
		EventTouch( const EventTouch& other, Event::Phase newPhase )
		:	Event( other, newPhase )
		,	m_touchId( other.m_touchId )
		,	m_touchPhase( other.m_touchPhase )
		,	m_tapCount( other.m_tapCount )
		,	m_location( other.m_location )
		,	m_previousLocation( other.m_previousLocation )
		,	m_wheelDelta( other.m_wheelDelta )
		,	m_nTouches( other.m_nTouches )
		,	m_iThisTouch( other.m_iThisTouch )
		{}
		
		SYNTHESIZE_GET( TouchId, touchId )							// A unique, arbitrary id for this touch. This id persists through the touch's begin, movement, and end.
		SYNTHESIZE_GET( TouchPhase, touchPhase )					// Whether this touch is beginning, moving, or ending.
		SYNTHESIZE_GET( unsigned int, tapCount )					// How many taps are associated with this touch. For instance, after a double-tap, tapCount will be 2.
		SYNTHESIZE_GET( vec2, location )							// The location of the touch in local space to the receiver of this event.
		SYNTHESIZE_GET( vec2, previousLocation )					// The prior location of the touch in local space to the receiver of this event.
		SYNTHESIZE_GET( int, nTouches )								// How many touches are currently active.
		SYNTHESIZE_GET( int, iThisTouch )							// The index of this particular touch within the list of currently active touches.
		SYNTHESIZE_GET( vec2, wheelDelta )
		
	private:
		
		TouchId m_touchId;
		TouchPhase m_touchPhase;
		unsigned int m_tapCount;
		vec2 m_location;
		vec2 m_previousLocation;
		vec2 m_wheelDelta;
		int m_nTouches;
		int m_iThisTouch;
		
	};
}

#endif
