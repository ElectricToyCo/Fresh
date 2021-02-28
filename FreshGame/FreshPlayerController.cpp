//
//  FreshPlayerController.cpp
//  Fresh
//
//  Created by Jeff Wofford on 12/3/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//

#include "FreshPlayerController.h"
#include "FreshActor.h"
#include "Camera.h"
#include "Stage.h"
#include "Application.h"

namespace fr
{	
	FRESH_DEFINE_CLASS( FreshPlayerController )
	DEFINE_VAR( FreshPlayerController, size_t, m_gamepadIndex );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( FreshPlayerController )
	
	void FreshPlayerController::possess( FreshActor& actor )
	{
		Super::possess( actor );

		ASSERT( host() );
		
		host()->stage().addEventListener( EventKeyboard::KEY_DOWN, FRESH_CALLBACK( onStageKeyDown ));
		host()->stage().addEventListener( EventKeyboard::KEY_UP, FRESH_CALLBACK( onStageKeyUp ));

		host()->stage().addEventListener( EventTouch::TOUCH_BEGIN, FRESH_CALLBACK( onStageTouchDown ));
		host()->stage().addEventListener( EventTouch::TOUCH_MOVE, FRESH_CALLBACK( onStageTouchMove ));
		host()->stage().addEventListener( EventTouch::TOUCH_END, FRESH_CALLBACK( onStageTouchUp ));

		auto& gamepadManager = Application::instance().gamepadManager();
		
		gamepadManager.addEventListener( GamepadManager::GAMEPAD_ATTACHED, FRESH_CALLBACK( onGamepadJoined ));
		gamepadManager.addEventListener( GamepadManager::GAMEPAD_DETACHED, FRESH_CALLBACK( onGamepadLeft ));
		
		if( !m_gamepad && m_gamepadIndex < gamepadManager.numAttachedGamepads())
		{
			setupGamepad( gamepadManager.gamepadAt( m_gamepadIndex ));
		}
	}
	
	void FreshPlayerController::unpossess( FreshActor& actor )
	{
		ASSERT( host() );
		
		host()->stage().removeEventListener( EventKeyboard::KEY_DOWN, FRESH_CALLBACK( onStageKeyDown ));
		host()->stage().removeEventListener( EventKeyboard::KEY_UP, FRESH_CALLBACK( onStageKeyUp ));
		
		host()->stage().removeEventListener( EventTouch::TOUCH_BEGIN, FRESH_CALLBACK( onStageTouchDown ));
		host()->stage().removeEventListener( EventTouch::TOUCH_MOVE, FRESH_CALLBACK( onStageTouchMove ));
		host()->stage().removeEventListener( EventTouch::TOUCH_END, FRESH_CALLBACK( onStageTouchUp ));
		
		Application::instance().gamepadManager().removeEventListener( GamepadManager::GAMEPAD_ATTACHED, FRESH_CALLBACK( onGamepadJoined ));
		Application::instance().gamepadManager().removeEventListener( GamepadManager::GAMEPAD_DETACHED, FRESH_CALLBACK( onGamepadLeft ));
		
		Super::unpossess( actor );
	}

	void FreshPlayerController::update()
	{
		updateMovementControls();
		Super::update();
	}
	
	void FreshPlayerController::updateMovementControls()
	{
		if( const auto myHost = host() )
		{
			vec2 movementImpulse;
			
			// TODO make configurable.
			if( Keyboard::isKeyDown( Keyboard::LeftArrow ))
			{
				movementImpulse.x -= 1;
			}
			if( Keyboard::isKeyDown( Keyboard::RightArrow ))
			{
				movementImpulse.x += 1;
			}
			if( Keyboard::isKeyDown( Keyboard::UpArrow ))
			{
				movementImpulse.y -= 1;
			}
			if( Keyboard::isKeyDown( Keyboard::DownArrow ))
			{
				movementImpulse.y += 1;
			}
			movementImpulse.normalize();
			
			if( m_gamepad )
			{
				movementImpulse.x += m_gamepad->axis( Gamepad::Axis::LX );
				movementImpulse.y += m_gamepad->axis( Gamepad::Axis::LY );
			}
			
			if( !movementImpulse.isZero() )
			{
				stopTravel();
				myHost->applyControllerImpulse( movementImpulse );
			}
		}
	}
	
	void FreshPlayerController::setupGamepad( Gamepad::ptr gamepad )
	{
		m_gamepad = gamepad;
		
		if( m_gamepad )
		{
			m_gamepad->addEventListener( Gamepad::BUTTON_DOWN, FRESH_CALLBACK( onGamepadButtonDown ));
			m_gamepad->addEventListener( Gamepad::BUTTON_UP, FRESH_CALLBACK( onGamepadButtonUp ));
			m_gamepad->addEventListener( Gamepad::AXIS_MOVED, FRESH_CALLBACK( onGamepadAxisMoved ));
		}
	}
	
	void FreshPlayerController::onActionKeyDown( Keyboard::Key key )
	{
		host()->jump( 1.0 );
	}
	
	void FreshPlayerController::onActionButtonDown( Gamepad::Button button )
	{
		host()->jump( 1.0 );
	}
	
	FRESH_DEFINE_CALLBACK( FreshPlayerController, onStageKeyDown, EventKeyboard )
	{
		if( host() && ( event.key() == Keyboard::Space || event.key() == Keyboard::Shift || event.key() == Keyboard::X || event.key() == Keyboard::Z ))
		{
			onActionKeyDown( event.key() );
		}
	}
	
	FRESH_DEFINE_CALLBACK( FreshPlayerController, onStageKeyUp, EventKeyboard )
	{}

	FRESH_DEFINE_CALLBACK( FreshPlayerController, onGamepadJoined, EventGamepad )
	{
		// We interested in this gamepad?
		//
		if( Application::instance().gamepadManager().gamepadIndex( event.gamepadTarget() ) == m_gamepadIndex )
		{
			setupGamepad( event.gamepadTarget() );
		}
	}
	
	FRESH_DEFINE_CALLBACK( FreshPlayerController, onGamepadLeft, EventGamepad )
	{
		if( event.gamepadTarget() == m_gamepad )
		{
			if( m_gamepad )
			{
				m_gamepad->removeEventListener( Gamepad::BUTTON_DOWN, FRESH_CALLBACK( onGamepadButtonDown ));
				m_gamepad->removeEventListener( Gamepad::BUTTON_UP, FRESH_CALLBACK( onGamepadButtonUp ));
				m_gamepad->removeEventListener( Gamepad::AXIS_MOVED, FRESH_CALLBACK( onGamepadAxisMoved ));
			}
			m_gamepad = nullptr;
		}
	}
	
	FRESH_DEFINE_CALLBACK( FreshPlayerController, onGamepadButtonDown, EventGamepadButton )
	{
		if( host() && ( event.button() == Gamepad::Button::A || event.button() == Gamepad::Button::B || event.button() == Gamepad::Button::X || event.button() == Gamepad::Button::Y ))
		{
			onActionButtonDown( event.button() );
		}
	}
	
	FRESH_DEFINE_CALLBACK( FreshPlayerController, onGamepadButtonUp, EventGamepadButton )
	{}
	
	FRESH_DEFINE_CALLBACK( FreshPlayerController, onGamepadAxisMoved, EventGamepadAxis )
	{}
	
	FRESH_DEFINE_CALLBACK( FreshPlayerController, onStageTouchDown, EventTouch )
	{
		// TODO
	}

	FRESH_DEFINE_CALLBACK( FreshPlayerController, onStageTouchMove, EventTouch )
	{
		// TODO
	}

	FRESH_DEFINE_CALLBACK( FreshPlayerController, onStageTouchUp, EventTouch )
	{
		// TODO
	}
}

