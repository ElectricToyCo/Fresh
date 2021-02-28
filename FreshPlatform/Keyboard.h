//
//  Keyboard.h
//  Fresh
//
//  Created by Jeff Wofford on 3/21/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_Keyboard_h
#define Fresh_Keyboard_h

#include "FreshEssentials.h"
#include "FreshDebug.h"
#include "Archive.h"

namespace fr
{
	
	class Keyboard
	{
	public:
		
		enum Key
		{
			Unsupported = 0,
			Backspace = 8,
			Tab = 9,
			NumpadClear = 12,
			Enter = 13,
			Shift = 16,
			CtrlCommand = 17,
			AltOption = 18,
			PauseBreak = 19,		// PC Only
			CapsLock = 20,
			Escape = 27,
			Space = 32,
			PageUp = 33,
			PageDown = 34,
			End = 35,
			Home = 36,
			LeftArrow = 37,
			UpArrow = 38,
			RightArrow = 39,
			DownArrow = 40,
			Insert = 45,
			Delete = 46,
			Alpha0 = 48,
			Alpha1 = 49,
			Alpha2 = 50,
			Alpha3 = 51,
			Alpha4 = 52,
			Alpha5 = 53,
			Alpha6 = 54,
			Alpha7 = 55,
			Alpha8 = 56,
			Alpha9 = 57,
			A = 65,
			B = 66,
			C = 67,
			D = 68,
			E = 69,
			F = 70,
			G = 71,
			H = 72,
			I = 73,
			J = 74,
			K = 75,
			L = 76,
			M = 77,
			N = 78,
			O = 79,
			P = 80,
			Q = 81,
			R = 82,
			S = 83,
			T = 84,
			U = 85,
			V = 86,
			W = 87,
			X = 88,
			Y = 89,
			Z = 90,
			Numpad0 = 96,
			Numpad1 = 97,
			Numpad2 = 98,
			Numpad3 = 99,
			Numpad4 = 100,
			Numpad5 = 101,
			Numpad6 = 102,
			Numpad7 = 103,
			Numpad8 = 104,
			Numpad9 = 105,
			NumpadMultiply = 106,
			NumpadAdd = 107,
			NumpadSubtract = 109,
			NumpadDecimal = 110,
			NumpadDivide = 111,
			F1 = 112,
			F2 = 113,
			F3 = 114,
			F4 = 115,
			F5 = 116,
			F6 = 117,
			F7 = 118,
			F8 = 119,
			F9 = 120,
			F10 = 121,
			F11 = 122,
			F12 = 123,
			F13 = 124,
			F14 = 125,
			F15 = 126,
			NumLock = 144,
			ScrollLock = 145,
			LeftShift = 160,
			RightShift = 161,
			LeftCtrlCommand = 162,
			RightCtrlCommand = 163,
			LeftAltOption = 164,
			RightAltOption = 165,
			NumpadEnter = 169,
			NumpadEqual = 170,
			Semicolon = 186,
			Equal = 187,
			Comma = 188,
			Minus = 189,
			Period = 190,
			Slash = 191,
			Backtick = 192,
			RightBracket = 219,
			Backslash = 220,
			LeftBracket = 221,
			Quote = 222,
			MAX_KEYS = 256
		};
		
		static bool isKeyDown( Key key );
		
		// Don't call this. Used by the system.
		//
		static void onKeyStateChanged( Key key, bool newState );
	};

#define STREAM_CASE( keyname ) case Keyboard::keyname: out << #keyname; break;
	
	inline std::ostream& operator<<( std::ostream& out, Keyboard::Key key )
	{
		switch( key )
		{
			STREAM_CASE( Unsupported )
			STREAM_CASE( Backspace )
			STREAM_CASE( Tab )
			STREAM_CASE( NumpadClear )
			STREAM_CASE( Enter )
			STREAM_CASE( Shift )
			STREAM_CASE( CtrlCommand )
			STREAM_CASE( AltOption )
			STREAM_CASE( PauseBreak )
			STREAM_CASE( CapsLock )
			STREAM_CASE( Escape )
			STREAM_CASE( Space )
			STREAM_CASE( PageUp )
			STREAM_CASE( PageDown )
			STREAM_CASE( End )
			STREAM_CASE( Home )
			STREAM_CASE( LeftArrow )
			STREAM_CASE( UpArrow )
			STREAM_CASE( RightArrow )
			STREAM_CASE( DownArrow )
			STREAM_CASE( Insert )
			STREAM_CASE( Delete )
			STREAM_CASE( Alpha0 )
			STREAM_CASE( Alpha1 )
			STREAM_CASE( Alpha2 )
			STREAM_CASE( Alpha3 )
			STREAM_CASE( Alpha4 )
			STREAM_CASE( Alpha5 )
			STREAM_CASE( Alpha6 )
			STREAM_CASE( Alpha7 )
			STREAM_CASE( Alpha8 )
			STREAM_CASE( Alpha9 )
			STREAM_CASE( A )
			STREAM_CASE( B )
			STREAM_CASE( C )
			STREAM_CASE( D )
			STREAM_CASE( E )
			STREAM_CASE( F )
			STREAM_CASE( G )
			STREAM_CASE( H )
			STREAM_CASE( I )
			STREAM_CASE( J )
			STREAM_CASE( K )
			STREAM_CASE( L )
			STREAM_CASE( M )
			STREAM_CASE( N )
			STREAM_CASE( O )
			STREAM_CASE( P )
			STREAM_CASE( Q )
			STREAM_CASE( R )
			STREAM_CASE( S )
			STREAM_CASE( T )
			STREAM_CASE( U )
			STREAM_CASE( V )
			STREAM_CASE( W )
			STREAM_CASE( X )
			STREAM_CASE( Y )
			STREAM_CASE( Z )
			STREAM_CASE( Numpad0 )
			STREAM_CASE( Numpad1 )
			STREAM_CASE( Numpad2 )
			STREAM_CASE( Numpad3 )
			STREAM_CASE( Numpad4 )
			STREAM_CASE( Numpad5 )
			STREAM_CASE( Numpad6 )
			STREAM_CASE( Numpad7 )
			STREAM_CASE( Numpad8 )
			STREAM_CASE( Numpad9 )
			STREAM_CASE( NumpadMultiply )
			STREAM_CASE( NumpadAdd )
			STREAM_CASE( NumpadSubtract )
			STREAM_CASE( NumpadDecimal )
			STREAM_CASE( NumpadDivide )
			STREAM_CASE( F1 )
			STREAM_CASE( F2 )
			STREAM_CASE( F3 )
			STREAM_CASE( F4 )
			STREAM_CASE( F5 )
			STREAM_CASE( F6 )
			STREAM_CASE( F7 )
			STREAM_CASE( F8 )
			STREAM_CASE( F9 )
			STREAM_CASE( F10 )
			STREAM_CASE( F11 )
			STREAM_CASE( F12 )
			STREAM_CASE( F13 )
			STREAM_CASE( F14 )
			STREAM_CASE( F15 )
			STREAM_CASE( NumLock )
			STREAM_CASE( ScrollLock )
			STREAM_CASE( LeftShift )
			STREAM_CASE( RightShift )
			STREAM_CASE( LeftCtrlCommand )
			STREAM_CASE( RightCtrlCommand )
			STREAM_CASE( LeftAltOption )
			STREAM_CASE( RightAltOption )
			STREAM_CASE( NumpadEnter )
			STREAM_CASE( NumpadEqual )
			STREAM_CASE( Semicolon )
			STREAM_CASE( Equal )
			STREAM_CASE( Comma )
			STREAM_CASE( Minus )
			STREAM_CASE( Period )
			STREAM_CASE( Slash )
			STREAM_CASE( Backtick )
			STREAM_CASE( RightBracket )
			STREAM_CASE( Backslash )
			STREAM_CASE( LeftBracket )
			STREAM_CASE( Quote )
							
			default:
				ASSERT( false );
				out << "<unknown-key>";
				break;
		}
		
		return out;
	}
	
#undef STREAM_CASE
	
#define STREAM_CASE( keyname ) if( word == #keyname ) key = Keyboard::keyname; else
	
	inline std::istream& operator>>( std::istream& in, Keyboard::Key& key )
	{
		std::string word;
		in >> std::ws;
		fr::readToDelimiter( in, word, " =,|)}" );
		
		STREAM_CASE( Unsupported )
		STREAM_CASE( Backspace )
		STREAM_CASE( Tab )
		STREAM_CASE( NumpadClear )
		STREAM_CASE( Enter )
		STREAM_CASE( Shift )
		STREAM_CASE( CtrlCommand )
		STREAM_CASE( AltOption )
		STREAM_CASE( PauseBreak )
		STREAM_CASE( CapsLock )
		STREAM_CASE( Escape )
		STREAM_CASE( Space )
		STREAM_CASE( PageUp )
		STREAM_CASE( PageDown )
		STREAM_CASE( End )
		STREAM_CASE( Home )
		STREAM_CASE( LeftArrow )
		STREAM_CASE( UpArrow )
		STREAM_CASE( RightArrow )
		STREAM_CASE( DownArrow )
		STREAM_CASE( Insert )
		STREAM_CASE( Delete )
		STREAM_CASE( Alpha0 )
		STREAM_CASE( Alpha1 )
		STREAM_CASE( Alpha2 )
		STREAM_CASE( Alpha3 )
		STREAM_CASE( Alpha4 )
		STREAM_CASE( Alpha5 )
		STREAM_CASE( Alpha6 )
		STREAM_CASE( Alpha7 )
		STREAM_CASE( Alpha8 )
		STREAM_CASE( Alpha9 )
		STREAM_CASE( A )
		STREAM_CASE( B )
		STREAM_CASE( C )
		STREAM_CASE( D )
		STREAM_CASE( E )
		STREAM_CASE( F )
		STREAM_CASE( G )
		STREAM_CASE( H )
		STREAM_CASE( I )
		STREAM_CASE( J )
		STREAM_CASE( K )
		STREAM_CASE( L )
		STREAM_CASE( M )
		STREAM_CASE( N )
		STREAM_CASE( O )
		STREAM_CASE( P )
		STREAM_CASE( Q )
		STREAM_CASE( R )
		STREAM_CASE( S )
		STREAM_CASE( T )
		STREAM_CASE( U )
		STREAM_CASE( V )
		STREAM_CASE( W )
		STREAM_CASE( X )
		STREAM_CASE( Y )
		STREAM_CASE( Z )
		STREAM_CASE( Numpad0 )
		STREAM_CASE( Numpad1 )
		STREAM_CASE( Numpad2 )
		STREAM_CASE( Numpad3 )
		STREAM_CASE( Numpad4 )
		STREAM_CASE( Numpad5 )
		STREAM_CASE( Numpad6 )
		STREAM_CASE( Numpad7 )
		STREAM_CASE( Numpad8 )
		STREAM_CASE( Numpad9 )
		STREAM_CASE( NumpadMultiply )
		STREAM_CASE( NumpadAdd )
		STREAM_CASE( NumpadSubtract )
		STREAM_CASE( NumpadDecimal )
		STREAM_CASE( NumpadDivide )
		STREAM_CASE( F1 )
		STREAM_CASE( F2 )
		STREAM_CASE( F3 )
		STREAM_CASE( F4 )
		STREAM_CASE( F5 )
		STREAM_CASE( F6 )
		STREAM_CASE( F7 )
		STREAM_CASE( F8 )
		STREAM_CASE( F9 )
		STREAM_CASE( F10 )
		STREAM_CASE( F11 )
		STREAM_CASE( F12 )
		STREAM_CASE( F13 )
		STREAM_CASE( F14 )
		STREAM_CASE( F15 )
		STREAM_CASE( NumLock )
		STREAM_CASE( ScrollLock )
		STREAM_CASE( LeftShift )
		STREAM_CASE( RightShift )
		STREAM_CASE( LeftCtrlCommand )
		STREAM_CASE( RightCtrlCommand )
		STREAM_CASE( LeftAltOption )
		STREAM_CASE( RightAltOption )
		STREAM_CASE( NumpadEnter )
		STREAM_CASE( NumpadEqual )
		STREAM_CASE( Semicolon )
		STREAM_CASE( Equal )
		STREAM_CASE( Comma )
		STREAM_CASE( Minus )
		STREAM_CASE( Period )
		STREAM_CASE( Slash )
		STREAM_CASE( Backtick )
		STREAM_CASE( RightBracket )
		STREAM_CASE( Backslash )
		STREAM_CASE( LeftBracket )
		STREAM_CASE( Quote )
		
		// Final else clause
		{
			key = Keyboard::Unsupported;
		}
		return in;
	}
	
#undef STREAM_CASE

}

#endif
