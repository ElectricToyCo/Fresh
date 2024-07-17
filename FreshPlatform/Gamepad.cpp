//
//  Gamepad.cpp
//  Fresh
//
//  Created by Jeff Wofford on 4/14/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//

#include "Gamepad.h"

namespace fr
{	
	FRESH_DEFINE_CLASS( Gamepad )
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( Gamepad )

	const char* Gamepad::BUTTON_DOWN = "GamepadButtonDown";
	const char* Gamepad::BUTTON_UP = "GamepadButtonUp";
	const char* Gamepad::AXIS_MOVED = "GamepadAxisMoved";

	bool Gamepad::button( Button index ) const
	{
		REQUIRES( attached() );
		return m_reportedButtons[ size_t( index ) ];
	}
	
	float Gamepad::axis( Axis index ) const
	{
		REQUIRES( attached() );
		
		const auto result = m_reportedAxes[ size_t( index ) ];
		PROMISES( -1.0f <= result && result <= 1.0f );
		return result;
	}

	void Gamepad::create( void* payload, std::map< size_t, Button >&& buttonMapping, std::map< size_t, Axis >&& axisMapping )
	{
		REQUIRES( !attached() );
		REQUIRES( payload );
		
		m_payload = payload;		
		m_attached = true;
		
		m_hardwareButtonMap = std::move( buttonMapping );
		m_hardwareAxisMap = std::move( axisMapping );
	}
	
	void Gamepad::setAxisValue( size_t index, float value )
	{
		REQUIRES( attached() );
		REQUIRES( -1.0f <= value && value <= 1.0f );
		
		auto iter = m_hardwareAxisMap.find( index );
		if( iter != m_hardwareAxisMap.end() )
		{
			ASSERT( iter->second < Gamepad::Axis::NUM );
			m_gatheringAxes[ size_t( iter->second ) ] = value;
		}
	}
	
	void Gamepad::setButtonValue( size_t index, bool value )
	{
		REQUIRES( attached() );

		auto iter = m_hardwareButtonMap.find( index );
		if( iter != m_hardwareButtonMap.end() )
		{
			ASSERT( iter->second < Gamepad::Button::NUM );
			m_gatheringButtons[ static_cast< size_t >( iter->second ) ] = value;
		}
	}
	
	void Gamepad::update()
	{
		REQUIRES( attached() );
		
		updateStates();
		
		if( attached() )
		{
			// Report changes.
			//
			for( size_t i = 0; i < size_t( Button::NUM ); ++i )
			{
				if( m_reportedButtons[ i ] != m_gatheringButtons[ i ] )
				{
					EventGamepadButton event( m_gatheringButtons[ i ] ? BUTTON_DOWN : BUTTON_UP, this, Button( i ));
					dispatchEvent( &event );
				}
			}
			
			for( size_t i = 0; i < size_t( Axis::NUM ); ++i )
			{
				if( m_reportedAxes[ i ] != m_gatheringAxes[ i ] )
				{
					EventGamepadAxis event( AXIS_MOVED, this, Axis( i ), m_gatheringAxes[ i ], m_reportedAxes[ i ] );
					dispatchEvent( &event );
				}
			}
			
			std::copy_n( m_gatheringButtons, size_t( Button::NUM ), m_reportedButtons ) ;
			std::copy_n( m_gatheringAxes, size_t( Axis::NUM ), m_reportedAxes ) ;
		}
	}
	
	//////////////////////////////////////////////////
	
	FRESH_DEFINE_CLASS( GamepadManager )
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( GamepadManager )
	FRESH_CUSTOM_STANDARD_CONSTRUCTOR_NAMING( GamepadManager )
	{
		construct();
	}

	const char* GamepadManager::GAMEPAD_ATTACHED = "GamepadAttached";
	const char* GamepadManager::GAMEPAD_DETACHED = "GamepadDetached";

	size_t GamepadManager::numAttachedGamepads() const
	{
		return m_gamepads.size();
	}
	
	Gamepad::ptr GamepadManager::gamepadAt( size_t index ) const
	{
		REQUIRES( index < numAttachedGamepads() );
		
		auto result = m_gamepads[ index ];
		
		PROMISES( result );
		
		return result;
	}
	
	size_t GamepadManager::gamepadIndex( Gamepad::ptr gamepad ) const
	{
		REQUIRES( gamepad );
		
		auto iter = std::find( m_gamepads.begin(), m_gamepads.end(), gamepad );
		ASSERT( iter != m_gamepads.end() );
		
		return iter - m_gamepads.begin();
	}
	
	void GamepadManager::update()
	{
		updateSystem();
		updateGamepads();
	}
	
	void GamepadManager::updateGamepads()
	{
		for( auto gamepad : m_gamepads )
		{
			ASSERT( gamepad );
			if( gamepad->attached() )
			{
				gamepad->update();
				
				if( !gamepad->attached() )
				{
					EventGamepad event( GAMEPAD_DETACHED, gamepad );
					dispatchEvent( &event );
				}
			}
		}
		
		// Remove detached gamepads.
		//
		removeElements( m_gamepads, []( const Gamepad::ptr& gamepad ) { return !gamepad->attached(); } );
	}
	
	Gamepad::ptr GamepadManager::createGamepad()
	{
		auto gamepad = createObject< Gamepad >();
		m_gamepads.push_back( gamepad );
		return gamepad;
	}
	
	void GamepadManager::onGamepadAttached( Gamepad::ptr gamepad )
	{
		REQUIRES( gamepad );
		REQUIRES( gamepad->attached() );
		
		EventGamepad event( GAMEPAD_ATTACHED, gamepad );
		dispatchEvent( &event );
	}
	
	void GamepadManager::onGamepadDetached( Gamepad::ptr gamepad )
	{
		REQUIRES( gamepad );
		REQUIRES( gamepad->attached() );
		
		EventGamepad event( GAMEPAD_DETACHED, gamepad );
		dispatchEvent( &event );
		
		m_gamepads.erase( std::find( m_gamepads.begin(), m_gamepads.end(), gamepad ));
	}
}

