//
//  ApiInput.cpp
//  Fresh
//
//  Created by Jeff Wofford on 6/26/18.
//  Copyright (c) 2018 Jeff Wofford. All rights reserved.
//

#include "FantasyConsole.h"
#include "ApiImplementation.h"

namespace
{
	inline bool buttonStateDown( const std::vector< std::vector< bool >>& playerbuttons, int button, int player )
	{
		if( player < static_cast< int >( playerbuttons.size() ))
		{
			const auto& buttons = playerbuttons[ player ];

			if( 0 <= button && button < static_cast< int >( buttons.size() ))
			{
				return buttons[ button ];
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	inline bool keyStateDown( const std::vector< bool >& keys, int key )
	{
		if( 0 <= key && key < static_cast< int >( keys.size() ))
		{
			return keys[ key ];
		}
		else
		{
			return false;
		}
	}
}

namespace fr
{
	LUA_FUNCTION( touchx, 0 )
	int FantasyConsole::touchx()
	{
		return m_touchDown ? m_touchPos.x : -1;
	}

	LUA_FUNCTION( touchy, 0 )
	int FantasyConsole::touchy()
	{
		return m_touchDown ? m_touchPos.y : -1;
	}

	LUA_FUNCTION( touchupx, 0 )
	int FantasyConsole::touchupx()
	{
		return !m_touchDown ? m_touchPos.x : -1;
	}

	LUA_FUNCTION( touchupy, 0 )
	int FantasyConsole::touchupy()
	{
		return !m_touchDown ? m_touchPos.y : -1;
	}

    LUA_FUNCTION( wheelx, 0 )
    real FantasyConsole::wheelx()
    {
        return m_wheelDelta.x;
    }

    LUA_FUNCTION( wheely, 0 )
    real FantasyConsole::wheely()
    {
        return m_wheelDelta.y;
    }

	LUA_FUNCTION( btn, 1 )
	bool FantasyConsole::btn( int button, int player )
	{
		SANITIZE( player, 0, 0, 3 );

		return buttonStateDown( m_buttonsDown, button, player );
	}

	LUA_FUNCTION( btnp, 1 )
	bool FantasyConsole::btnp( int button, int player )
	{
		SANITIZE( player, 0, 0, 3 );

		return btn( button, player ) && !buttonStateDown( m_buttonsDownPrev, button, player );
	}

	void FantasyConsole::setJoystickState( size_t player, size_t axis, real value )
	{
		ASSERT( player < 4 );
		ASSERT( axis < 6 );

		m_joystickStates.resize( std::max( m_joystickStates.size(), player + 1 ));

		auto& axesForPlayer = m_joystickStates[ player ];

		axesForPlayer.resize( std::max( axesForPlayer.size(), axis + 1 ));

		axesForPlayer[ axis ] = value;
	}

	LUA_FUNCTION( joy, 1 )
	real FantasyConsole::joy( int axis, int player )
	{
		SANITIZE( axis, 0, 0, 1 );
		SANITIZE( player, 0, 0, 3 );

		if( player >= static_cast< int >( m_joystickStates.size() ))
		{
			return 0;
		}

		const auto& axes = m_joystickStates[ player ];
		if( axis >= static_cast< int >( axes.size() ))
		{
			return 0;
		}

		return axes[ axis ];
	}

	LUA_FUNCTION( key, 1 )
	bool FantasyConsole::key( int key )
	{
		return keyStateDown( m_keysDown, key );
	}

	LUA_FUNCTION( keydown, 1 )
	bool FantasyConsole::keydown( int theKey )
	{
		return key( theKey ) && !keyStateDown( m_keysDownPrev, theKey );
	}

	LUA_FUNCTION( keyup, 1 )
	bool FantasyConsole::keyup( int theKey )
	{
		return !key( theKey ) && keyStateDown( m_keysDownPrev, theKey );
	}
}

