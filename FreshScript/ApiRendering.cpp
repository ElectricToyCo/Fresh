//
//  ApiRendering.cpp
//  Fresh
//
//  Created by Jeff Wofford on 6/24/18.
//  Copyright (c) 2018 Jeff Wofford. All rights reserved.
//

#include "FantasyConsole.h"
#include "ApiImplementation.h"
#include "Stage.h"
#include "FreshVector.h"
#include "Color.h"
using namespace fr;
using namespace luacpp;

namespace fr
{		
	LUA_FUNCTION( cls, 0 )
	void FantasyConsole::cls( uint color )
	{
		DEFAULT( color, Color::Black );
		
		m_screen->clear( color );		
		m_nextDefaultPrintPosition.setToZero();
	}

	LUA_FUNCTION( camera, 0 )
	void FantasyConsole::camera( real x, real y )
	{
		DEFAULT( x, 0 );
		DEFAULT( y, x );

		m_cameraTranslation.set( -x, -y );
	}

	LUA_FUNCTION( rect, 4 )
	void FantasyConsole::rect( real left, real top, real right, real bottom, uint color, real lineThickness )
	{
		DEFAULT( color, Color::White );
		SANITIZE( lineThickness, 1.0f, 0.1f, 0.5f * std::min( std::abs( right - left ), std::abs( bottom - top )));
		
		const auto texCoords = blankTexCoords();
		
		quad( fr::rect{ left, top, left + lineThickness, bottom }, texCoords, Color{ color } );
		quad( fr::rect{ left + lineThickness, bottom - lineThickness, right - lineThickness, bottom }, texCoords, Color{ color } );
		quad( fr::rect{ right - lineThickness, top, right, bottom }, texCoords, Color{ color } );
		quad( fr::rect{ left + lineThickness, top, right - lineThickness, top + lineThickness }, texCoords, Color{ color } );
	}
	
	LUA_FUNCTION( rectfill, 4 )
	void FantasyConsole::rectfill( real left, real top, real right, real bottom, uint color )
	{
		DEFAULT( color, Color::White );
		
		const auto texCoords = blankTexCoords();
		quad( fr::rect{ left, top, right, bottom }, texCoords, Color{ color } );
	}
	
	LUA_FUNCTION( arc, 4 )
	void FantasyConsole::arc( real x, real y, real radius, real arcLength, real rotation, uint color, real lineThickness, int segments )
	{
		SANITIZE( radius, 1.0f, 0.0f, std::numeric_limits< real >::max() );
		SANITIZE( arcLength, 90.0f, 0.0f, 360.0f );
		SANITIZE_WRAP( rotation, 0.0f, 0.0f, 360.0f );
		DEFAULT( color, Color::White );
		SANITIZE( lineThickness, 1.0f, 0.1f, radius );
		SANITIZE( segments, 24, 3, 256 );
		
		const auto realColor = Color{ color };
		
		const vec2 center{ x, y };
		vec2 outerSpoke{ radius, 0 };
		vec2 innerSpoke{ radius - lineThickness, 0 };
		
		outerSpoke = outerSpoke.getRotated( rotation );
		innerSpoke = innerSpoke.getRotated( rotation );
		
		const angle sliceArc{ arcLength / segments };
		
		const auto texCoords = blankTexCoords();
		
		for( int i = 0; i < segments; ++i )
		{
			vertex( center + outerSpoke.getRotated( sliceArc * ( i     )), texCoords.ulCorner(), realColor );
			vertex( center + innerSpoke.getRotated( sliceArc * ( i     )), texCoords.blCorner(), realColor );
			vertex( center + outerSpoke.getRotated( sliceArc * ( i + 1 )), texCoords.urCorner(), realColor );
			
			vertex( center + innerSpoke.getRotated( sliceArc * ( i     )), texCoords.blCorner(), realColor );
			vertex( center + outerSpoke.getRotated( sliceArc * ( i + 1 )), texCoords.urCorner(), realColor );
			vertex( center + innerSpoke.getRotated( sliceArc * ( i + 1 )), texCoords.brCorner(), realColor );
		}
	}

	LUA_FUNCTION( arcfill, 4 )
	void FantasyConsole::arcfill( real x, real y, real radius, real arcLength, real rotation, uint color, int segments )
	{
		arc( x, y, radius, arcLength, rotation, color, radius, segments );
	}
	
	LUA_FUNCTION( circ, 3 )
	void FantasyConsole::circ( real x, real y, real radius, uint color, real lineThickness, int segments )
	{
		arc( x, y, radius, 360.0f, 0, color, lineThickness, segments );
	}

	LUA_FUNCTION( circfill, 3 )	
	void FantasyConsole::circfill( real x, real y, real radius, uint color, int segments )
	{
		circ( x, y, radius, color, radius, segments );
	}
	
	LUA_FUNCTION( line, 4 )	
	void FantasyConsole::line( real x0, real y0, real x1, real y1, uint color, real lineThickness, std::string capType )
	{
		DEFAULT( color, Color::White );
		SANITIZE( lineThickness, 1.0f, 0.1f, std::numeric_limits< real >::max() );
		DEFAULT( capType, "butt" );
		
		vec2 p( x0, y0 );
		vec2 q( x1, y1 );

		vec2 delta;
		real len;
		vec2 norm;
		vec2 perp;
		
		const auto calculateAncillaries = [&]() {
			delta = q - p;
			len = delta.length();
			norm = delta.normal();
			perp = norm.getPerpendicular();
		};
		
		calculateAncillaries();
		
		if( capType == "square" )
		{
			capType = "butt";
			
			p = q - ( norm * ( len + lineThickness * 0.5f ));
			q = p + ( norm * ( len + lineThickness ));
			calculateAncillaries();
		}
		
		// Draw "butt"-type line.
		//
		const auto realColor = Color{ color };
		const auto texCoords = blankTexCoords();
		const auto lateralOffset = perp * lineThickness;
		vertex( p - lateralOffset, texCoords.ulCorner(), realColor );
		vertex( p + lateralOffset, texCoords.urCorner(), realColor );
		vertex( q - lateralOffset, texCoords.blCorner(), realColor );

		vertex( p + lateralOffset, texCoords.urCorner(), realColor );
		vertex( q - lateralOffset, texCoords.blCorner(), realColor );
		vertex( q + lateralOffset, texCoords.brCorner(), realColor );

		if( capType == "round" )
		{
			const auto angle = delta.angleDegrees();
			arcfill( p.x, p.y, lineThickness, 180, angle + 90, color, 24 );
			arcfill( q.x, q.y, lineThickness, 180, angle - 90, color, 24 );
		}
		else if( capType != "butt" )
		{
			console_trace( "`line()`: Unrecognized cap type '" << capType << "'." );
		}
	}
	
	LUA_FUNCTION( print, 0 )	
	void FantasyConsole::print( std::string message, real initialX, real y, uint color, int font, real scale, bool noNewline )
	{
		DEFAULT( message, "" );
		DEFAULT( initialX, m_nextDefaultPrintPosition.x );
		DEFAULT( y, m_nextDefaultPrintPosition.y );
		DEFAULT( color, Color::White );
		SANITIZE( font, 0, 0, numFonts() - 1 );
		SANITIZE( scale, 1.0f, 0.0f, 128.0f );
		
		real x = initialX;

		if( scale < 0.001f )
		{
			return;
		}
        
        scale *= m_globalTextScale;
		
		const real lineSpacing = linePadding() * scale;
		const auto glyphDims = vector_cast< real >( fontGlyphTexelSize() ) * scale;
		
		const real lineHeight = glyphDims.y + lineSpacing;
		
		if( lineHeight < 0.001f )
		{
			return;
		}

		for( char c : message )
		{
			const fr::rect positions{ x, y, x + glyphDims.x, y + glyphDims.y };
			const auto texCoords = glyphTexCoords( font, c );

			quad( positions, texCoords, color );
			
			x = positions.right();
		}		

		if( !noNewline )	// Sorry about the double negative. Supports the desired default behavior.
		{
			m_nextDefaultPrintPosition.x = initialX;
			m_nextDefaultPrintPosition.y = y + lineHeight;
		}
		else
		{
			m_nextDefaultPrintPosition.x = x;
			m_nextDefaultPrintPosition.y = y;
		}
	}	
	
	LUA_FUNCTION( spr, 3 )	
	void FantasyConsole::spr( int spriteIndex, real x, real y, int spritesWid, int spritesHgt, bool flipX, bool flipY, uint color, uint additiveColor )
	{
		const vec2i spritesDims = spritesDimensions();
		
		SANITIZE( spriteIndex, 0, 0, maxSpriteIndex() );
		const vec2i spriteULCorner = spriteIndexToSpritePos( spriteIndex );		
		
		// Clamp width and height to fit inside spritesheet proper.
		SANITIZE( spritesHgt, spritesWid, 1, spritesDims.y - spriteULCorner.y );	// Height first because it depends on the input width.
		SANITIZE( spritesWid, 1, 1, spritesDims.x - spriteULCorner.x );
		
		const auto texelUL = spriteSheetPosToTexel( spriteULCorner );
		const auto texelSize = spriteSheetPosToTexel( vec2i( spritesWid, spritesHgt )); 
		
		sspr( texelUL.x, texelUL.y, texelSize.x, texelSize.y, x, y, LuaDefault< real >::value, LuaDefault< real >::value, flipX, flipY, color, additiveColor );
	}
	
	LUA_FUNCTION( sspr, 6 )
	void FantasyConsole::sspr( int sx, int sy, int sw, int sh, real dx, real dy, real dw, real dh, bool flipX, bool flipY, uint color, uint additiveColor )
	{
		const vec2i texelDims = spriteSheetPosToTexel( spritesDimensions() );
		
		SANITIZE( sx, 0, 0, texelDims.x - 1 );
		SANITIZE( sy, 0, 0, texelDims.y - 1 );
		SANITIZE( sh, sw, 0, texelDims.y - sy );	// Height first because it depends on the input width.
		SANITIZE( sw, 0, 0, texelDims.x - sx );
		DEFAULT( dw, sw );
		DEFAULT( dh, sh );
		DEFAULT( flipX, false );
		DEFAULT( flipY, false );
		DEFAULT( color, Color::White );
		DEFAULT( additiveColor, 0 );
		
		const vec2i spriteTexelSize( sw, sh );
		
		const vec2i spriteTexelULCorner( sx, sy );
		const vec2i spriteTexelBRCorner = spriteTexelULCorner + spriteTexelSize;
		
		const vec2 pixelSize( dw, dh );
		
		const vec2 screenULCorner{ dx, dy };
		fr::rect positions{ screenULCorner, screenULCorner + pixelSize };
		
		fr::rect texCoords{ texelsToTexCoords( spriteTexelULCorner ), texelsToTexCoords( spriteTexelBRCorner ) };
		
		if( flipX )
		{
			const auto temp = texCoords.left();
			texCoords.left( texCoords.right() );
			texCoords.right( temp );
		}
		
		if( flipY )
		{
			const auto temp = texCoords.top();
			texCoords.top( texCoords.bottom() );
			texCoords.bottom( temp );
		}
		
		quad( positions, texCoords, color, additiveColor );
	}
	
	LUA_FUNCTION( fget, 1 )
	int FantasyConsole::fget( int spriteIndex, int mask )
	{
		SANITIZE( spriteIndex, 0, 0, maxSpriteIndex() );
		DEFAULT( mask, 0xFF );

        if( spriteIndex < static_cast< int >( m_spriteFlags.size() ))
		{
			return m_spriteFlags[ spriteIndex ] & mask;
		}
		else
		{
			return 0;
		}
	}
	
	LUA_FUNCTION( fset, 2 )
	void FantasyConsole::fset( int spriteIndex, int flagIndexOrBitfield, int valueOrUndefined )
	{
		SANITIZE( spriteIndex, 0, 0, maxSpriteIndex() );
		SANITIZE_WRAP( flagIndexOrBitfield, 0, 0, 8 );

        if( spriteIndex < maxSpriteIndex() )
		{
			m_spriteFlags.resize( std::max( m_spriteFlags.size(), static_cast< size_t >( spriteIndex + 1 )), 0 );
			
			int bitfield = fget( spriteIndex, 0xFF );
			
			if( luacpp::isDefault( valueOrUndefined ))
			{
				// Interpret flagIndex as the bitfield.
				//
				bitfield = flagIndexOrBitfield;
			}
			else
			{
				const int bitInQuestion = 1 << ( flagIndexOrBitfield - 1 );
				if( valueOrUndefined )
				{
					bitfield |=  bitInQuestion;
				}
				else
				{
					bitfield &= ~bitInQuestion;
				}
			}
			
			m_spriteFlags[ spriteIndex ] = bitfield;
		}
	}
	
	LUA_FUNCTION( fillp, 0 )
	void FantasyConsole::fillp( int bitpattern )
	{
		DEFAULT( bitpattern, 0 );
		
		setDitherPattern( bitpattern );
	}
}

