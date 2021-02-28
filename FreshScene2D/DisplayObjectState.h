/*
 *  DisplayObjectState.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 7/8/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_DISPLAY_OBJECT_STATE_H_INCLUDED
#define FRESH_DISPLAY_OBJECT_STATE_H_INCLUDED

#include "FreshVector.h"
#include "Color.h"
#include "Object.h"
#include "Tweener.h"

namespace fr
{
	class DisplayObject;
	
	// Class DisplayObjectState is a simple class which embodies the state of a DisplayObject.
	// It is used by Keyframes.
	
#define DECLARE_TWEENER_OBJECT( tweenerType ) static const Tweener##tweenerType< DisplayObjectState > tweener##tweenerType;
#define DECLARE_TWEENER_OBJECTS( tweenerType ) \
	static const Tweener##tweenerType##EaseIn< DisplayObjectState > tweener##tweenerType##EaseIn;	\
	static const Tweener##tweenerType##EaseOut< DisplayObjectState > tweener##tweenerType##EaseOut;	\
	static const Tweener##tweenerType##EaseInOut< DisplayObjectState > tweener##tweenerType##EaseInOut;
	
	class DisplayObjectState
	{
	public:

		DECLARE_TWEENER_OBJECT( Linear )
		DECLARE_TWEENER_OBJECTS( Sin )
		DECLARE_TWEENER_OBJECTS( Quad )
		DECLARE_TWEENER_OBJECTS( Cubic )
		DECLARE_TWEENER_OBJECTS( Quart )
		DECLARE_TWEENER_OBJECTS( Quint )
		DECLARE_TWEENER_OBJECTS( Expo )
		DECLARE_TWEENER_OBJECTS( Circ )
		DECLARE_TWEENER_OBJECTS( Back )
		DECLARE_TWEENER_OBJECTS( Elastic )
		DECLARE_TWEENER_OBJECTS( Bounce )

		explicit DisplayObjectState( const vec2& p = vec2::ZERO, const vec2& s = vec2( 1.0f, 1.0f ), real r = 0, Color c = Color::White, const vec2& pivot = vec2::ZERO, Color a = 0, const rect& frame = rect(0,0,0,0) )
		:	m_position( p )
		,	m_scale( s )
		,	m_rotation( r )
		,	m_color( c )
		,	m_colorAdditive( a )
		,	m_pivot( pivot )
		,	m_frame( frame )
		{}

		SYNTHESIZE( vec2, position );
		SYNTHESIZE( vec2, scale );
		void scale( real scale ) 												{ m_scale.x = m_scale.y = scale; }
		SYNTHESIZE( real, rotation );
		SYNTHESIZE( Color, color );
		SYNTHESIZE( Color, colorAdditive );
		SYNTHESIZE( vec2, pivot );
		SYNTHESIZE( rect, frame );
		
		DisplayObjectState getTweenedStateTo( const DisplayObjectState& destination, TimeType normalizedTime, const Tweener< DisplayObjectState >& tweener, bool useRotationTweening = false ) const;
			// REQUIRES( 0 <= normalizedTime && normalizedTime <= 1.0 );
		
		void applyStateTo( DisplayObject& object ) const;
			// REQUIRES( object );
		
		void setStateFrom( const DisplayObject& object );
			// REQUIRES( object );
		
		DisplayObjectState operator+( const DisplayObjectState& other ) const;
		DisplayObjectState operator-( const DisplayObjectState& other ) const;
		DisplayObjectState operator*( const DisplayObjectState& other ) const;
		DisplayObjectState operator/( const DisplayObjectState& other ) const;
		
		DisplayObjectState operator*( real scalar ) const;
		DisplayObjectState operator/( real scalar ) const;
		
		DisplayObjectState operator-() const;
		
		bool operator==( const DisplayObjectState& other ) const;
		bool operator!=( const DisplayObjectState& other ) const				{ return !( *this == other ); }
		
		static const Tweener< DisplayObjectState >* getTweenerByName( const std::string& name );
		static std::string getTweenerName( const Tweener< DisplayObjectState >& tweener );

		inline static DisplayObjectState create( const DisplayObject& object )
		{
			DisplayObjectState state;
			state.setStateFrom( object );
			return state;
		}
		
		friend std::ostream& operator<<( std::ostream& out, const DisplayObjectState& instance );
		friend std::istream& operator>>( std::istream& out, DisplayObjectState& instance );
		
	private:
		
		vec2 m_position;
		vec2 m_scale = vec2( 1.0f, 1.0f );
		real m_rotation = 0;
		Color m_color = Color::White;
		Color m_colorAdditive = Color::Invisible;
		vec2 m_pivot;
		rect m_frame;
	};

}

#undef DECLARE_TWEENER_OBJECT
#undef DECLARE_TWEENER_OBJECTS



#endif
