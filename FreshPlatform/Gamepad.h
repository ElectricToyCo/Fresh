//
//  Gamepad.h
//  Fresh
//
//  Created by Jeff Wofford on 4/14/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_Gamepad_h
#define Fresh_Gamepad_h

#include "EventDispatcher.h"

namespace fr
{
	class Gamepad : public EventDispatcher
	{
		FRESH_DECLARE_CLASS( Gamepad, EventDispatcher )
	public:

		// The Gamepad class assumes and imposes an Xbox 360 controller.
		// Other gamepad types may work, but we will talk about their axes and buttons
		// as if they were on a Xbox 360 controller.
		// Xbox 360 controls that are unsupported on a given gamepad will simply return
		// 0 or false.
		// Gamepads with controls that go beyond the Xbox 360 controlls will be unavailable.
		// It is therefore the burden of this class to make all controller types work nicely
		// from the perspective of the player while imposing an Xbox 360 paradigm on all
		// controllers.

		enum class Button
		{
			A,		// South: PS "X"
			B,		// East:  PS "O"
			X,		// West:  PS "square"
			Y,		// North: PS "triangle"
			DPadRight,
			DPadDown,
			DPadLeft,
			DPadUp,
			LStick,
			RStick,
			LBumper,
			RBumper,
			Back,
			Start,
			NUM
		};

		enum class Axis
		{
			LX,
			LY,
			RX,
			RY,
			LTrigger,
			RTrigger,
			NUM
		};

		// Event types.
		//
		static const char* BUTTON_DOWN;
		static const char* BUTTON_UP;
		static const char* AXIS_MOVED;

		virtual ~Gamepad();

		SYNTHESIZE_GET( bool, attached )

		bool button( Button button ) const;
		// REQUIRES( attached() && index < numButtons() );

		float axis( Axis axis ) const;
		// REQUIRES( attached() && index < numAxes() );
		// PROMISES( -1.0f <= result && result <= 1.0f );

		// FOR INTERNAL USE.
		//
		void create( void* payload, std::map< size_t, Button >&& buttonMapping, std::map< size_t, Axis >&& axisMapping );
		void setButtonValue( size_t index, bool value );
		void setAxisValue( size_t index, float value );
		SYNTHESIZE_GET( void*, payload );

	protected:

		void construct();

		void update();
		// REQUIRES( attached() );

		void updateStates();

	private:

		bool m_reportedButtons[ size_t( Button::NUM ) ] = { false };		// The values last dispatched to the user.
		float m_reportedAxes[ size_t( Axis::NUM ) ] = { 0.0f };

		bool m_gatheringButtons[ size_t( Button::NUM ) ] = { false };		// The values currently being accumulated through interaction with the hardware and/or OS.
		float m_gatheringAxes[ size_t( Axis::NUM ) ] = { 0.0f };

		std::map< size_t, Button > m_hardwareButtonMap;	// Maps the internal integer-based button number as dictated by the hardware to the equivalent interface Button enum value. Unmapped indices represent unsupported controls.
		std::map< size_t, Axis > m_hardwareAxisMap;		// Likewise for axes.

		bool m_attached = false;

		void* m_payload = nullptr;		// Points to malloc'd object with platform-specific data.

		friend class GamepadManager;
	};

	//////////////////////////////////////////////////

	class EventGamepad : public Event
	{
	public:

		EventGamepad( TypeRef type_,
					 Gamepad::ptr target_,
					 Object::ptr currentTarget = nullptr
					 )
		:	Event( type_, target_, currentTarget )
		,	m_gamepadTarget( target_ )
		{}

		EventGamepad( const EventGamepad& event, Phase newPhase )
		:	Event( event, newPhase )
		,	m_gamepadTarget( event.m_gamepadTarget )
		{}

		SYNTHESIZE_GET( Gamepad::ptr, gamepadTarget );

	private:

		Gamepad::ptr m_gamepadTarget;

	};

	class EventGamepadButton : public EventGamepad
	{
	public:
		EventGamepadButton( TypeRef type_,
						   Gamepad::ptr target_,
						   Gamepad::Button whichButton,
						   Object::ptr currentTarget = nullptr )
		:	EventGamepad( type_, target_, currentTarget )
		,	m_button( whichButton )
		{}

		EventGamepadButton( const EventGamepadButton& event, Phase newPhase )
		:	EventGamepad( event, newPhase )
		,	m_button( event.m_button )
		{}

		SYNTHESIZE_GET( Gamepad::Button, button );

	private:

		Gamepad::Button m_button;
	};

	class EventGamepadAxis : public EventGamepad
	{
	public:
		EventGamepadAxis( TypeRef type_,
						   Gamepad::ptr target_,
						   Gamepad::Axis whichAxis,
						   float newValue,
						   float oldValue,
						   Object::ptr currentTarget = nullptr )
		:	EventGamepad( type_, target_, currentTarget )
		,	m_axis( whichAxis )
		,	m_newValue( newValue )
		,	m_oldValue( oldValue )
		{}

		EventGamepadAxis( const EventGamepadAxis& event, Phase newPhase )
		:	EventGamepad( event, newPhase )
		,	m_axis( event.m_axis )
		,	m_newValue( event.m_newValue )
		,	m_oldValue( event.m_oldValue )
		{}

		SYNTHESIZE_GET( Gamepad::Axis, axis );
		SYNTHESIZE_GET( float, newValue );
		SYNTHESIZE_GET( float, oldValue );

	private:

		Gamepad::Axis m_axis;
		float m_newValue;
		float m_oldValue;
	};

	//////////////////////////////////////////////////

	class GamepadManager : public EventDispatcher
	{
		FRESH_DECLARE_CLASS( GamepadManager, EventDispatcher )
	public:

		// Event types.
		//
		static const char* GAMEPAD_ATTACHED;
		static const char* GAMEPAD_DETACHED;

		size_t numAttachedGamepads() const;

		Gamepad::ptr gamepadAt( size_t index ) const;
		// REQUIRES( index < numAttachedGamepads() );
		// PROMISES( result );

		size_t gamepadIndex( Gamepad::ptr gamepad ) const;

		void update();	// Call me once per "tick" or "update".

		// FOR INTERNAL USE.
		//
		Gamepad::ptr createGamepad();
		void onGamepadAttached( Gamepad::ptr gamepad );
		void onGamepadDetached( Gamepad::ptr gamepad );

	protected:

		void construct();
		void destroy();

		void updateSystem();
		void updateGamepads();

	private:

		std::vector< Gamepad::ptr > m_gamepads;

	};

}

#endif
