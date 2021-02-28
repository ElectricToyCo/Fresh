/*
 *  DisplayObjectState.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 7/8/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#include "DisplayObjectState.h"
#include "DisplayObject.h"
#include "DisplayObjectContainer.h"
#include "ObjectStreamFormatter.h"

#if !TARGET_OS_IPHONE
#	define STATES_GRAB_OBJECT_FRAMES 1		// Needed for editor on the desktop
#endif

namespace
{
	template< typename T >
	inline std::ostream& write( std::ostream& out, const char* propName, const T& propValue, bool prependComma = true )
	{
		out << ( prependComma ? ", " : " " ) << "\"" << propName << "\":\"" << propValue << "\"";
		return out;
	}

#	define WRITE( prop )				if( instance.m_##prop != decltype( instance.m_##prop ){} ) {  write( out, #prop, instance.m_##prop, written++ != 0 ); }
#	define WRITE_DEF( prop, default )	if( instance.m_##prop != default ) {  write( out, #prop, instance.m_##prop, written++ != 0 ); }
	
}

namespace fr
{

	std::ostream& operator<<( std::ostream& out, const DisplayObjectState& instance )
	{
		out << "{";
		
		size_t written = 0;
		
		WRITE( position );
		WRITE_DEF( scale, vec2( 1 ));
		WRITE( rotation );
		WRITE_DEF( color, Color::White );
		WRITE_DEF( colorAdditive, Color::Invisible );
		WRITE( pivot );
		WRITE( frame );
		out << "}";
		return out;
	}
	
#define READ( prop ) if( propName == #prop ) propValueStream >> instance.m_##prop; else

	std::istream& operator>>( std::istream& in, DisplayObjectState& instance )
	{
		try
		{
			in >> std::ws;
			if( in.peek() == '{' )
			{
				in.get();
				in >> std::ws;
				
				while( in.peek() != '}' && in.peek() != std::char_traits< char >::eof() )
				{
					if( in.get() == '\"' )
					{
						std::string propName;
						std::getline( in, propName, '\"' );

						trim( propName );

						in >> std::ws;
						if( in.get() != ':' )
						{
							throw;
						}
						
						in >> std::ws;
						
						if( in.get() == '\"' )
						{
							std::string propValue;
							std::getline( in, propValue, '\"' );
							
							Destringifier propValueStream( propValue );
							
							READ( position )
							READ( scale )
							READ( rotation )
							READ( color )
							READ( colorAdditive )
							READ( pivot )
							READ( frame )
							{
								throw;
							}
							
							in >> std::ws;
							
							auto trailing = in.get();
							
							if( trailing == ',' )
							{
								in >> std::ws;
							}
							else if( trailing == '}' )
							{
								break;
							}
						}
						else
						{
							throw;
						}
					}
					else
					{
						throw;
					}
				}
			}
		}
		catch( ... )
		{
			dev_error( "DisplayObjectState had unexpected format." );
		}
		return in;
	}
	
#define DECLARE_TWEENER_OBJECT( tweenerType ) const Tweener##tweenerType< DisplayObjectState > DisplayObjectState::tweener##tweenerType;
#define DECLARE_TWEENER_OBJECTS( tweenerType ) \
	const Tweener##tweenerType##EaseIn< DisplayObjectState > DisplayObjectState::tweener##tweenerType##EaseIn;	\
	const Tweener##tweenerType##EaseOut< DisplayObjectState > DisplayObjectState::tweener##tweenerType##EaseOut;	\
	const Tweener##tweenerType##EaseInOut< DisplayObjectState > DisplayObjectState::tweener##tweenerType##EaseInOut;

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

	DisplayObjectState DisplayObjectState::getTweenedStateTo( const DisplayObjectState& destination, TimeType normalizedTime, const Tweener< DisplayObjectState >& tweener, bool useRotationTweening ) const
	{
		REQUIRES( 0 <= normalizedTime && normalizedTime <= 1.0 );
		
		DisplayObjectState state = tweener( *this, destination, normalizedTime );
		
		// Color needs special tweening. TODO why?!
		//
		state.color( m_color.lerp( destination.m_color, normalizedTime ));
		state.colorAdditive( m_colorAdditive.lerp( destination.m_colorAdditive, normalizedTime ));
		
		if( !useRotationTweening )
		{
			angle before = m_rotation;
			angle after = destination.m_rotation;
			angle tweened = lerp( before, after, normalizedTime );
			state.m_rotation = tweened.toDegrees< decltype( state.m_rotation ) >();
		}
		
		return state;
	}

	void DisplayObjectState::applyStateTo( DisplayObject& object ) const
	{
		object.position( m_position );
		object.scale( m_scale );
		object.rotation( m_rotation );
		object.color( m_color );
		object.colorAdditive( m_colorAdditive );
		object.pivot( m_pivot );
#if STATES_GRAB_OBJECT_FRAMES		// This is a performance optimization for apps that don't animate frames.
		object.frame( m_frame );
#endif
	}

	void DisplayObjectState::setStateFrom( const DisplayObject& object )
	{
		m_position = object.position();
		m_scale = object.scale();
		m_rotation = object.rotation().toDegrees< decltype( m_rotation ) >();
		m_color = object.color();		
		m_colorAdditive = object.colorAdditive();		
		m_pivot = object.pivot();
#if STATES_GRAB_OBJECT_FRAMES
		m_frame = object.frame();
#endif
	}

	DisplayObjectState DisplayObjectState::operator+( const DisplayObjectState& other ) const
	{
		return DisplayObjectState( m_position + other.m_position,
								   m_scale + other.m_scale,
								   m_rotation + other.m_rotation,
								   m_color + other.m_color,
								   m_pivot + other.m_pivot,
								   m_colorAdditive + other.m_colorAdditive,
								   rect( m_frame.ulCorner() + other.m_frame.ulCorner(), m_frame.brCorner() + other.m_frame.brCorner() ));
	}

	DisplayObjectState DisplayObjectState::operator-( const DisplayObjectState& other ) const
	{
		return DisplayObjectState( m_position - other.m_position,
								  m_scale - other.m_scale,
								  m_rotation - other.m_rotation,
								  m_color - other.m_color,
								  m_pivot - other.m_pivot,
								  m_colorAdditive - other.m_colorAdditive,
								  rect( m_frame.ulCorner() - other.m_frame.ulCorner(), m_frame.brCorner() - other.m_frame.brCorner() ));
	}

	DisplayObjectState DisplayObjectState::operator*( const DisplayObjectState& other ) const
	{
		return DisplayObjectState( m_position * other.m_position,
								  m_scale * other.m_scale,
								  m_rotation * other.m_rotation,
								  m_color * other.m_color,
								  m_pivot * other.m_pivot,
								  m_colorAdditive * other.m_colorAdditive,
								  rect( m_frame.ulCorner() * other.m_frame.ulCorner(), m_frame.brCorner() * other.m_frame.brCorner() ));
	}

	DisplayObjectState DisplayObjectState::operator/( const DisplayObjectState& other ) const
	{
		return DisplayObjectState( m_position / other.m_position,
								  m_scale / other.m_scale,
								  m_rotation / other.m_rotation,
								  m_color / other.m_color,
								  m_pivot / other.m_pivot,
								  m_colorAdditive / other.m_colorAdditive,
								  rect( m_frame.ulCorner() / other.m_frame.ulCorner(), m_frame.brCorner() / other.m_frame.brCorner() ));
	}

	DisplayObjectState DisplayObjectState::operator*( real scalar ) const
	{
		return DisplayObjectState( m_position * scalar,
								  m_scale * scalar,
								  m_rotation * scalar,
								  m_color * scalar,
								  m_pivot * scalar,
								  m_colorAdditive * scalar,
								  rect( m_frame.ulCorner() * scalar, m_frame.brCorner() * scalar ));
	}

	DisplayObjectState DisplayObjectState::operator/( real scalar ) const
	{
		return DisplayObjectState( m_position / scalar,
								  m_scale / scalar,
								  m_rotation / scalar,
								  m_color / scalar,
								  m_pivot / scalar,
								  m_colorAdditive / scalar,
								  rect( m_frame.ulCorner() / scalar, m_frame.brCorner() / scalar ));
	}
	
	DisplayObjectState DisplayObjectState::operator-() const
	{
		return DisplayObjectState( -m_position,
								   -m_scale,
								   -m_rotation,
								   -m_color,
								   -m_pivot,
								   -m_colorAdditive,
								  rect( -m_frame.ulCorner(), -m_frame.brCorner() ));
	}
	
	bool DisplayObjectState::operator==( const DisplayObjectState& other ) const
	{
		return  m_position == other.m_position &&
			    m_scale == other.m_scale &&
			    m_rotation == other.m_rotation &&
				m_color == other.m_color &&
				m_colorAdditive == other.m_colorAdditive &&
				m_pivot == other.m_pivot &&
				m_frame == other.m_frame;
	}

	// TODO slow. Map would be faster.

#define TWEENER_NAME_CASE( tweenerName )	if( name == #tweenerName ) return &tweener##tweenerName;
#define TWEENER_NAME_CASES( tweenerName )	\
	TWEENER_NAME_CASE( tweenerName##EaseIn )	\
	TWEENER_NAME_CASE( tweenerName##EaseOut )	\
	TWEENER_NAME_CASE( tweenerName##EaseInOut )

	const Tweener< DisplayObjectState >* DisplayObjectState::getTweenerByName( const std::string& name )
	{
		TWEENER_NAME_CASE( Linear )
		TWEENER_NAME_CASES( Sin )
		TWEENER_NAME_CASES( Quad )
		TWEENER_NAME_CASES( Cubic )
		TWEENER_NAME_CASES( Quart )
		TWEENER_NAME_CASES( Quint )
		TWEENER_NAME_CASES( Expo )
		TWEENER_NAME_CASES( Circ )
		TWEENER_NAME_CASES( Back )
		TWEENER_NAME_CASES( Elastic )
		TWEENER_NAME_CASES( Bounce )
		
		dev_warning( "Unrecognized tweener name " << name << "." );
		
		return nullptr;								// Unrecognized.
	}
	
#undef TWEENER_NAME_CASE
#undef TWEENER_NAME_CASES
	
#define TWEENER_NAME_CASE( tweenerName )	if( &tweener##tweenerName == &tweener ) return #tweenerName;
#define TWEENER_NAME_CASES( tweenerName )	\
	TWEENER_NAME_CASE( tweenerName##EaseIn )	\
	TWEENER_NAME_CASE( tweenerName##EaseOut )	\
	TWEENER_NAME_CASE( tweenerName##EaseInOut )
	
	std::string DisplayObjectState::getTweenerName( const Tweener< DisplayObjectState >& tweener )
	{
		TWEENER_NAME_CASE( Linear )
		TWEENER_NAME_CASES( Sin )
		TWEENER_NAME_CASES( Quad )
		TWEENER_NAME_CASES( Cubic )
		TWEENER_NAME_CASES( Quart )
		TWEENER_NAME_CASES( Quint )
		TWEENER_NAME_CASES( Expo )
		TWEENER_NAME_CASES( Circ )
		TWEENER_NAME_CASES( Back )
		TWEENER_NAME_CASES( Elastic )
		TWEENER_NAME_CASES( Bounce )
		
		return "";								// Unrecognized.
	}
}
