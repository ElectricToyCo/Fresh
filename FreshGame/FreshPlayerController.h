//
//  FreshPlayerController.h
//  Fresh
//
//  Created by Jeff Wofford on 12/3/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_FreshPlayerController_h
#define Fresh_FreshPlayerController_h

#include "FreshActorController.h"
#include "EventKeyboard.h"
#include "EventTouch.h"
#include "Gamepad.h"

namespace fr
{
	
	class FreshPlayerController : public FreshActorController
	{
		FRESH_DECLARE_CLASS( FreshPlayerController, FreshActorController );
	public:

		virtual void possess( FreshActor& actor ) override;
		virtual void unpossess( FreshActor& actor ) override;
		virtual void update() override;

	protected:
		
		virtual void updateMovementControls();

		virtual void onActionKeyDown( Keyboard::Key key );
		virtual void onActionButtonDown( Gamepad::Button button );
		
		SYNTHESIZE_GET( Gamepad::wptr, gamepad );
		
		void setupGamepad( Gamepad::ptr gamepad );
		
		FRESH_DECLARE_CALLBACK( FreshPlayerController, onStageKeyDown, EventKeyboard );
		FRESH_DECLARE_CALLBACK( FreshPlayerController, onStageKeyUp, EventKeyboard );
		FRESH_DECLARE_CALLBACK( FreshPlayerController, onStageTouchDown, EventTouch );
		FRESH_DECLARE_CALLBACK( FreshPlayerController, onStageTouchMove, EventTouch );
		FRESH_DECLARE_CALLBACK( FreshPlayerController, onStageTouchUp, EventTouch );
		FRESH_DECLARE_CALLBACK( FreshPlayerController, onGamepadJoined, EventGamepad );
		FRESH_DECLARE_CALLBACK( FreshPlayerController, onGamepadLeft, EventGamepad );
		FRESH_DECLARE_CALLBACK( FreshPlayerController, onGamepadButtonDown, EventGamepadButton );
		FRESH_DECLARE_CALLBACK( FreshPlayerController, onGamepadButtonUp, EventGamepadButton );
		FRESH_DECLARE_CALLBACK( FreshPlayerController, onGamepadAxisMoved, EventGamepadAxis );
		
	private:
		
		DVAR( size_t, m_gamepadIndex, 0 );
		
		Gamepad::wptr m_gamepad;
		
	};
	
}

#endif
