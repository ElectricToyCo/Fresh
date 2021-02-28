/*
 *  Event.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/30/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_EVENT_H_INCLUDED
#define FRESH_EVENT_H_INCLUDED

#include "Object.h"
#include <string>

namespace fr
{
	class Event
	{
	public:
		
		typedef std::string Type;
		typedef const Type& TypeRef;
		
		enum Phase
		{
			Capturing,
			AtTarget,
			Bubbling
		};
		
		Event( TypeRef type_, Object::ptr target_, Object::ptr currentTarget = nullptr )
		:	m_phase( Capturing )
		,	m_type( type_ )
		,	m_target( target_ )
		,	m_currentTarget( currentTarget )
		{}

		Event( const Event& event, Object::ptr target_ )
		:	m_phase( event.m_phase )
		,	m_type( event.m_type )
		,	m_target( target_ )
		,	m_currentTarget( event.m_currentTarget )
		{}
		
		Event( const Event& event, Phase newPhase )
		:	m_phase( newPhase )
		,	m_type( event.m_type )
		,	m_target( event.m_target )
		,	m_currentTarget( event.m_currentTarget )
		{}
		
		SYNTHESIZE_GET( TypeRef, type )
		SYNTHESIZE_GET( Object::ptr, target )
		SYNTHESIZE( Object::ptr, currentTarget )
		SYNTHESIZE( Phase, phase )
		
	private:
		
		Phase m_phase;
		Type m_type;
		Object::ptr m_target;
		Object::ptr m_currentTarget;
		
	};
}

#endif
