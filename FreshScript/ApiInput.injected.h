//
//  ApiInput.h
//  Fresh
//
//  Created by Jeff Wofford on 6/26/18.
//  Copyright (c) 2018 Jeff Wofford. All rights reserved.
//

public:

int touchx();
int touchy();
int touchupx();
int touchupy();
real wheelx();
real wheely();

bool btn( int button, int player );
bool btnp( int button, int player );

real joy( int axis, int player );

// Keyboard mappings are ASCII. Note also these special key mappings.
//
// Home = 2
// End = 3
// PageUp = 11
// PageDown = 12
// Shift = 14
// LeftArrow = 17
// RightArrow = 18
// UpArrow = 19
// DownArrow = 20
// CtrlCommand = 26
//
bool key( int keycode );
bool keydown( int keycode );
bool keyup( int keycode );

protected:

void setJoystickState( size_t player, size_t axis, real value );

private:

bool m_touchDown = false;
vec2i m_touchPos;
vec2 m_wheelDelta;

std::vector< std::vector< bool >> m_buttonsDown;
std::vector< std::vector< bool >> m_buttonsDownPrev;
std::vector< bool > m_keysDown;
std::vector< bool > m_keysDownPrev;

std::vector< std::vector< real >> m_joystickStates;
