/*
 *  Font.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 8/7/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#include "Font.h"
#include "FreshFile.h"
#include "Texture.h"
#include "Renderer.h"
#include "Objects.h"
#include "Application.h"
#include <fstream>

namespace
{
	const int DEFAULT_GLYPH_GRID_SIZE = 16;	
}

namespace fr
{

	FRESH_DEFINE_CLASS( FontLoader )
	DEFINE_VAR( FontLoader, std::string, m_fontMetricsFilePath );
	DEFINE_VAR( FontLoader, float, m_characterSpacingScalar );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( FontLoader )

	FontLoader::FontLoader( const fr::ClassInfo& assignedClassInfo, fr::Object::NameRef objectName )
	:	Super( assignedClassInfo, objectName )
	{
		m_assetClass = getClass( "Font" );
		
		doctorClass< FontLoader >( [&]( ClassInfo& classInfo, FontLoader& defaultObject )
									  {
										  DOCTOR_PROPERTY_ASSIGN( assetClass )
									  } );
	}

	////////////////////////////////////////////
	
	FRESH_DEFINE_CLASS( Font )

	Font::Font( CreateInertObject c )
	:	Super( c )
	{
		setAllGlyphWidths( 0 );
	}
	
	Font::Font( const ClassInfo& assignedClassInfo, NameRef objectName /* = DEFAULT_OBJECT_NAME */ )
	:	Asset( assignedClassInfo, objectName )
	{
		setAllGlyphWidths( 0 );
	}

	Font::ptr Font::getFont( const std::string& fontTextureName )
	{
		Font::ptr font = createOrGetObject< Font >( fontTextureName );
		PROMISES( font );	
		return font;
	}

	size_t Font::getMemorySize() const
	{
		return sizeof( *this ) + ( m_texture ? m_texture->getMemorySize() : 0 );
	}

	void Font::loadFile( const std::string& texturePath, const std::string& metricsPath, float spacingScalar )
	{	
		REQUIRES( !texturePath.empty() );
		setTextureByName( texturePath );
		
		if( !metricsPath.empty() )
		{
			loadMetricsFile( metricsPath );
		}
		
		m_spacingScalar = spacingScalar;
	}

	void Font::texture( Texture::ptr tex )
	{
		m_texture = tex;
	}

	void Font::setTextureByName( const std::string& texName )
	{
		Texture::ptr tex = Renderer::instance().createTexture( texName );
		ASSERT( tex );
		texture( tex );
	}

	SmartPtr< Texture > Font::texture() const
	{
		return m_texture;
	}

	void Font::setGlyphWidths( unsigned char glyphTexelWidths[ NUM_GLYPHS ] )
	{
		REQUIRES( glyphTexelWidths != 0 );
		
		for( size_t i = 0; i < NUM_GLYPHS; ++i )
		{
			m_glyphWidths[ i ] = glyphTexelWidths[ i ];
		}
	}

	void Font::setAllGlyphWidths( int texelWidth )
	{
		for( size_t i = 0; i < NUM_GLYPHS; ++i ) 
		{
			m_glyphWidths[ i ] = texelWidth;
		}
	}

	void Font::loadMetricsFile( const std::string& metricsFileName )
	{
		REQUIRES( texture() );	// Needs a texture to already have been set.
		
		path resourcePath = fr::getResourcePath( metricsFileName );
		ASSERT( !resourcePath.empty() );
		
		std::ifstream inFile( resourcePath.c_str(), std::ios_base::in | std::ios_base::binary );
		ASSERT( !inFile.bad() );

		inFile.read( (char*) m_glyphWidths, NUM_GLYPHS );
		
		ASSERT( !inFile.fail() );		// Didn't try to read too much.
	}

	float Font::getGlyphWidth( unsigned char asciiCode ) const
	{
		return m_glyphWidths[ asciiCode ] * m_spacingScalar;
	}

	Vector2ui Font::getGlyphCellDimensions() const
	{
		REQUIRES( texture() );
		
		return m_texture->dimensions() / DEFAULT_GLYPH_GRID_SIZE;
	}

	void Font::getGlyphTexCoords( unsigned char asciiCode, Vector2f& outUVTranslation ) const
	{
		outUVTranslation.set( ( asciiCode % DEFAULT_GLYPH_GRID_SIZE ) / (float)DEFAULT_GLYPH_GRID_SIZE, ( asciiCode / DEFAULT_GLYPH_GRID_SIZE ) / (float)DEFAULT_GLYPH_GRID_SIZE );
	}
}
