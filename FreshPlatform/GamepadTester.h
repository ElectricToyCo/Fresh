//
//  GamepadTester.h
//  Fresh
//
//  Created by Jeff Wofford on 4/14/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_GamepadTester_h
#define Fresh_GamepadTester_h

#include "Gamepad.h"
#include "MovieClip.h"
#include "TextField.h"

namespace fr
{
	
	class ControlRepresentation : public MovieClip
	{
		FRESH_DECLARE_CLASS( ControlRepresentation, MovieClip );
	public:

		void nameText( const std::string& name_ );

	protected:
		
		TextField& nameTextField() const;

	};
	
	class AxisRepresentation : public ControlRepresentation
	{
		FRESH_DECLARE_CLASS( AxisRepresentation, ControlRepresentation );
	public:
		
		void value( float value_ );
		void oldValue( float value_ );
		
	protected:
		
		DisplayObject& valueRep() const;
		DisplayObject& oldValueRep() const;
	};
	
	class ButtonRepresentation : public ControlRepresentation
	{
		FRESH_DECLARE_CLASS( ButtonRepresentation, ControlRepresentation );
	public:
		
		void value( bool down );
	};
	
	class GamepadRepresentation : public MovieClip
	{
		FRESH_DECLARE_CLASS( GamepadRepresentation, MovieClip );
	public:
		
		void setup( Gamepad::ptr gamepad_ );
		SYNTHESIZE_GET( Gamepad::ptr, gamepad );
		
	protected:
		
		DisplayObjectContainer& axisHost() const;
		DisplayObjectContainer& buttonHost() const;
		
	private:
		
		VAR( ClassInfo::cptr, m_axisRepresentationClass );
		VAR( ClassInfo::cptr, m_buttonRepresentationClass );
		VAR( vec2, m_axisSpacing );
		VAR( vec2, m_buttonSpacing );
		
		Gamepad::ptr m_gamepad;
		
		FRESH_DECLARE_CALLBACK( GamepadRepresentation, onButtonDown, EventGamepadButton );
		FRESH_DECLARE_CALLBACK( GamepadRepresentation, onButtonUp, EventGamepadButton );
		FRESH_DECLARE_CALLBACK( GamepadRepresentation, onAxisMoved, EventGamepadAxis );
	};
	
	class GamepadTester : public MovieClip
	{
		FRESH_DECLARE_CLASS( GamepadTester, MovieClip );
	public:

		virtual void postLoad() override;
		
	protected:
		
		TextField& log() const;
		DisplayObjectContainer& host() const;
		
	private:
		
		VAR( ClassInfo::cptr, m_gamepadRepresentationClass );
		VAR( vec2, m_gamepadSpacing );

		FRESH_DECLARE_CALLBACK( GamepadTester, onGamepadAttached, EventGamepad );
		FRESH_DECLARE_CALLBACK( GamepadTester, onGamepadDetached, EventGamepad );
	};
	
}

#endif
