/*
 *  SimpleButton.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/30/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#include "SimpleButton.h"
#include "Objects.h"
#include "Stage.h"
#include "Profiler.h"
#include "FreshTime.h"
#include "CommandProcessor.h"
#include "AudioSystem.h"

#if FRESH_TELEMETRY_ENABLED
#	include "../FreshTelemetry/UserTelemetry.h"
#endif

#if DEV_MODE && 0
#	define trace_tap( expr ) trace( expr )
#else
#	define trace_tap( expr )
#endif

namespace fr
{	
	FRESH_DEFINE_CLASS( SimpleButton )
	DEFINE_VAR( SimpleButton, Object::wptr, m_onTapCallee );
	DEFINE_VAR( SimpleButton, std::string, m_onTapCalleeMethodExpression );
	DEFINE_VAR( SimpleButton, Texture::ptr, m_textureOut );
	DEFINE_VAR( SimpleButton, Texture::ptr, m_textureDisabled );
	DEFINE_VAR( SimpleButton, Texture::ptr, m_textureDown );
	DEFINE_VAR( SimpleButton, Texture::ptr, m_textureUp );
	DEFINE_VAR( SimpleButton, std::string, m_soundNameTapped );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( SimpleButton )
	
	SimpleButton::SimpleButton( const ClassInfo& assignedClassInfo, NameRef objectName /* = DEFAULT_OBJECT_NAME */ )
	:	MovieClip( assignedClassInfo, objectName )
	,	m_enabled( true )
	{
		isDragEnabled( true );
		minStageDistanceForDrag( 200.0f );
		minRealDurationForDrag( 0 );
		stop();	

		doctorClass< SimpleButton >( [&]( ClassInfo& classInfo, SimpleButton& defaultObject )
										   {
											   DOCTOR_PROPERTY( isDragEnabled )
											   DOCTOR_PROPERTY( minStageDistanceForDrag )
											   DOCTOR_PROPERTY( minRealDurationForDrag )
											   classInfo.overrideProperty( "isPlaying" );
										   } );
	}

	void SimpleButton::enabled( bool enabled )
	{
		if( m_enabled != enabled )
		{
			m_enabled = enabled;
			
			restoreDefaultState();
		}
	}

	void SimpleButton::finalizeKeyframes()
	{
		restoreDefaultState();
	}

	void SimpleButton::onTouchBegin( const EventTouch& event )
	{
		if( !isTouchable() )
		{
			return;
		}

		press();
		
		Super::onTouchBegin( event );
	}

	void SimpleButton::onTouchEndAnywhere( const EventTouch& event )
	{
		TIMER_AUTO( SimpleButton::onTouchEndAnywhere )

		trace_tap( this << " phase" << event.phase() );

		restoreDefaultState();
		
		Super::onTouchEndAnywhere( event );
	}

	bool SimpleButton::isTouchable() const
	{ 
		// Buttons not touchable if disabled.
		//
		return Super::isTouchable() && m_enabled;
	}

	void SimpleButton::restoreDefaultState()
	{
		if( m_enabled )
		{
			gotoKeyframe( "out" );
		}
		else
		{
			if( hasKeyframe( "disabled" ))
			{
				gotoKeyframe( "disabled" );
			}
		}	
	}

	void SimpleButton::press()
	{
		if( hasKeyframe( "down" ))
		{
			gotoKeyframe( "down" );
		}
	}
	
	void SimpleButton::tapButton()
	{
		trace_tap( this << " button tapped" );
		
		if( hasKeyframe( "up" ))
		{
			gotoKeyframe( "up" );
		}
		else
		{
			gotoKeyframe( "out" );
		}
		
		if( m_onTapCallee )
		{
			std::istringstream methodExpression( m_onTapCalleeMethodExpression );
			std::string methodName;
			methodExpression >> methodName;
			trim( methodName );
			if( !methodName.empty() )
			{
				m_onTapCallee->call( methodName, methodExpression );
			}
			else
			{
				dev_warning( this << " has on tap callee " << m_onTapCallee << " but no method expression." );
			}
		}
		else if( !m_onTapCalleeMethodExpression.empty() )
		{
			dev_warning( this << " has a method expression '" << m_onTapCalleeMethodExpression << "' but no callee." );
		}
		
		if( !m_soundNameTapped.empty() )
		{
			if( AudioSystem::ready() ) AudioSystem::playSound( m_soundNameTapped );
		}
	}
	
	void SimpleButton::onTapped( const EventTouch& event )
	{
		TIMER_AUTO( SimpleButton::OnTapped )

		tapButton();
		
		Super::onTapped( event );
	}
	
#define SETUP_KEYFRAME( which, Which )	\
	if( !hasKeyframe( #which ))	\
	{	\
		Keyframe keyframe;	\
		if( m_texture##Which )	\
		{	\
			keyframe.texture( m_texture##Which );	\
		}	\
		else	\
		{	\
			keyframe.texture( texture() );	\
		}	\
		if( keyframe.texture() ) \
		{	\
			setKeyframe( #which, keyframe );	\
		}	\
	}
	
	void SimpleButton::onAddedToParent()
	{
		Super::onAddedToParent();
		
		SETUP_KEYFRAME( out, Out )
		SETUP_KEYFRAME( disabled, Disabled )
		SETUP_KEYFRAME( down, Down )
		SETUP_KEYFRAME( up, Up )

		finalizeKeyframes();
	}
	
#undef SETUP_KEYFRAME
}
