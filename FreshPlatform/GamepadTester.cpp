//
//  GamepadTester.cpp
//  Fresh
//
//  Created by Jeff Wofford on 4/14/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//

#include "GamepadTester.h"
#include "Application.h"

namespace
{
	template< typename T = fr::DisplayObject >
	T& getExpectedDescendant( const fr::DisplayObjectContainer& host, const std::string& nameSubstring )
	{
		fr::SmartPtr< T > descendant = host.getDescendantByName< T >( nameSubstring, fr::DisplayObjectContainer::NameSearchPolicy::Substring );
		ASSERT( descendant );
		return *descendant;
	}
}

namespace fr
{

	FRESH_DEFINE_CLASS( ControlRepresentation );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( ControlRepresentation )
	
	void ControlRepresentation::nameText( const std::string& name_ )
	{
		nameTextField().text( name_ );
	}
	
	TextField& ControlRepresentation::nameTextField() const
	{
		return getExpectedDescendant< TextField >( *this, "_name" );
	}

	///////////////////////////////////////////
	FRESH_DEFINE_CLASS( AxisRepresentation )
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( AxisRepresentation )

	void AxisRepresentation::value( float value_ )
	{
		vec2 pos;
		pos[ 0 ] = value_;
		valueRep().position( pos );
	}

	void AxisRepresentation::oldValue( float value_ )
	{
		vec2 pos;
		pos[ 0 ] = value_;
		oldValueRep().position( pos );
	}
	
	DisplayObject& AxisRepresentation::valueRep() const
	{
		return getExpectedDescendant( *this, "_new_value" );
	}
	
	DisplayObject& AxisRepresentation::oldValueRep() const
	{
		return getExpectedDescendant( *this, "_old_value" );
	}
	
	///////////////////////////////////////////
	
	FRESH_DEFINE_CLASS( ButtonRepresentation )
	
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( ButtonRepresentation )

	void ButtonRepresentation::value( bool down )
	{
		gotoAndStop( down ? "down" : "up" );
	}

	///////////////////////////////////////////
	
	FRESH_DEFINE_CLASS( GamepadRepresentation )
	DEFINE_VAR( GamepadRepresentation, ClassInfo::cptr, m_axisRepresentationClass );
	DEFINE_VAR( GamepadRepresentation, ClassInfo::cptr, m_buttonRepresentationClass );
	DEFINE_VAR( GamepadRepresentation, vec2, m_axisSpacing );
	DEFINE_VAR( GamepadRepresentation, vec2, m_buttonSpacing );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( GamepadRepresentation )

	DisplayObjectContainer& GamepadRepresentation::axisHost() const
	{
		return getExpectedDescendant< DisplayObjectContainer >( *this, "_axis_host" );
	}
	
	DisplayObjectContainer& GamepadRepresentation::buttonHost() const
	{
		return getExpectedDescendant< DisplayObjectContainer >( *this, "_button_host" );
	}
	
	void GamepadRepresentation::setup( Gamepad::ptr gamepad_ )
	{
		ASSERT( gamepad_ );
		m_gamepad = gamepad_;
		
		m_gamepad->addEventListener( Gamepad::BUTTON_DOWN, FRESH_CALLBACK( onButtonDown ));
		m_gamepad->addEventListener( Gamepad::BUTTON_UP, FRESH_CALLBACK( onButtonUp ));
		m_gamepad->addEventListener( Gamepad::AXIS_MOVED, FRESH_CALLBACK( onAxisMoved ));

		if( m_axisRepresentationClass )
		{
			for( size_t i = 0; i < size_t( Gamepad::Axis::NUM ); ++i )
			{
				auto axis = createObject< AxisRepresentation >( &getTransientPackage(), *m_axisRepresentationClass );
				axis->position( m_axisSpacing * real( i ));
				axisHost().addChild( axis );
			}
		}
		
		if( m_buttonRepresentationClass )
		{
			for( size_t i = 0; i < size_t( Gamepad::Button::NUM ); ++i )
			{
				auto button = createObject< ButtonRepresentation >( &getTransientPackage(), *m_buttonRepresentationClass );
				button->position( m_buttonSpacing * real( i ));
				buttonHost().addChild( button );
			}
		}
	}

	FRESH_DEFINE_CALLBACK( GamepadRepresentation, onButtonDown, EventGamepadButton )
	{
		auto button = buttonHost().getChildAt( size_t( event.button() ))->as< ButtonRepresentation >();
		ASSERT( button );
		button->value( true );
	}
	
	FRESH_DEFINE_CALLBACK( GamepadRepresentation, onButtonUp, EventGamepadButton )
	{
		auto button = buttonHost().getChildAt( size_t( event.button() ))->as< ButtonRepresentation >();
		ASSERT( button );
		button->value( false );
	}
	
	FRESH_DEFINE_CALLBACK( GamepadRepresentation, onAxisMoved, EventGamepadAxis )
	{
		auto axis = axisHost().getChildAt( size_t( event.axis() ))->as< AxisRepresentation >();
		ASSERT( axis );
		axis->oldValue( event.oldValue() );
		axis->value( event.newValue() );
	}

	///////////////////////////////////////////
	
	FRESH_DEFINE_CLASS( GamepadTester )
	DEFINE_VAR( GamepadTester, ClassInfo::cptr, m_gamepadRepresentationClass );
	DEFINE_VAR( GamepadTester, vec2, m_gamepadSpacing );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( GamepadTester )

	void GamepadTester::postLoad()
	{
		Super::postLoad();
		
		auto& gamepadManager = Application::instance().gamepadManager();
		
		gamepadManager.addEventListener( GamepadManager::GAMEPAD_ATTACHED, FRESH_CALLBACK( onGamepadAttached ));
		gamepadManager.addEventListener( GamepadManager::GAMEPAD_DETACHED, FRESH_CALLBACK( onGamepadDetached ));
		
		if( m_gamepadRepresentationClass )
		{
			for( size_t i = 0; i < gamepadManager.numAttachedGamepads(); ++i )
			{
				auto gamepadRepresentation = createObject< GamepadRepresentation >( &getTransientPackage(), *m_gamepadRepresentationClass );
				gamepadRepresentation->setup( gamepadManager.gamepadAt( i ));
				gamepadRepresentation->position( m_gamepadSpacing * host().numChildren() );
				host().addChild( gamepadRepresentation );
			}
		}
	}
		
	TextField& GamepadTester::log() const
	{
		return getExpectedDescendant< TextField >( *this, "_log" );
	}
	
	DisplayObjectContainer& GamepadTester::host() const
	{
		return getExpectedDescendant< DisplayObjectContainer >( *this, "_gamepad_host" );
	}
		
	FRESH_DEFINE_CALLBACK( GamepadTester, onGamepadAttached, EventGamepad )
	{
		log().text( "Attached gamepad." );
		
		if( m_gamepadRepresentationClass )
		{
			auto gamepadRepresentation = createObject< GamepadRepresentation >( &getTransientPackage(), *m_gamepadRepresentationClass );
			gamepadRepresentation->setup( event.gamepadTarget() );
			gamepadRepresentation->position( m_gamepadSpacing * host().numChildren() );
			host().addChild( gamepadRepresentation );
		}
	}
	
	FRESH_DEFINE_CALLBACK( GamepadTester, onGamepadDetached, EventGamepad )
	{
		log().text( "Detached gamepad." );
		
		// Find our representation of this gamepad.
		//
		for( size_t i = 0; i < host().numChildren(); ++i )
		{
			auto gamepadRepresentation = host().getChildAt( i )->as< GamepadRepresentation >();
			ASSERT( gamepadRepresentation );
			if( gamepadRepresentation->gamepad() == event.gamepadTarget() )
			{
				host().removeChildAt( i );
				break;
			}
		}
	}
}

