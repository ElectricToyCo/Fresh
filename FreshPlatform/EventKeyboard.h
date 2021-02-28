//
//  EventKeyboard.h
//  Fresh
//
//  Created by Jeff Wofford on 3/21/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_EventKeyboard_h
#define Fresh_EventKeyboard_h

#include "Event.h"
#include "Keyboard.h"

namespace fr
{
	
	class EventKeyboard : public Event
	{
	public:
		
		static const char* KEY_DOWN;
		static const char* KEY_UP;

		EventKeyboard( TypeRef type_,
					  Object::ptr target_,
					  unsigned int char_,
					  Keyboard::Key key_,
					  bool alt_, bool ctrl_, bool shift_,
					  bool _held,
					  Object::ptr currentTarget = nullptr
					  )
		:	Event( type_, target_, currentTarget )
		,	m_charCode( char_ )
		,	m_key( key_ )
		,	m_isAltOptionDown( alt_ )
		,	m_isCtrlCommandDown( ctrl_ )
		,	m_isShiftDown( shift_ )
		,	m_isAHeldRepeat( _held )
		{}
		
		EventKeyboard( const EventKeyboard& event, Phase newPhase )
		:	Event( event, newPhase )
		,	m_charCode( event.m_charCode )
		,	m_key( event.m_key )
		,	m_isAltOptionDown( event.m_isAltOptionDown )
		,	m_isCtrlCommandDown( event.m_isCtrlCommandDown )
		,	m_isShiftDown( event.m_isShiftDown )
		,	m_isAHeldRepeat( event.m_isAHeldRepeat )
		{}
		
		SYNTHESIZE( unsigned int, charCode )
		SYNTHESIZE( Keyboard::Key, key )
		
		SYNTHESIZE( bool, isAltOptionDown )
		SYNTHESIZE( bool, isCtrlCommandDown )
		SYNTHESIZE( bool, isShiftDown )
		SYNTHESIZE( bool, isAHeldRepeat)

	private:
		
		unsigned int m_charCode = 0;
		Keyboard::Key m_key;
		
		bool m_isAltOptionDown = false;		// PC: Alt; Mac: Option.
		bool m_isCtrlCommandDown = false;	// PC: Ctrl; Mac: either Control or Command.
		bool m_isShiftDown = false;
		bool m_isAHeldRepeat = false;
	};
	
}

#endif
