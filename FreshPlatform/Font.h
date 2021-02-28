/*
 *  Font.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 8/7/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_FONT_H_INCLUDED
#define FRESH_FONT_H_INCLUDED

#include "Asset.h"
#include "Rectangle.h"

namespace fr
{
	class Texture;
	
	class Font : public Asset
	{
	public:

		static const size_t NUM_GLYPHS = 256;
		
		
		static SmartPtr< Font > getFont( const std::string& fontTextureName );
		
		virtual size_t getMemorySize() const override;
		
		virtual void loadFile( const std::string& texturePath, const std::string& metricsPath, float spacingScalar = 1.0f );
		// REQUIRES( !texturePath.empty() );

		
		void texture( SmartPtr< Texture > tex );
		void setTextureByName( const std::string& texName );
		
		SmartPtr< Texture > texture() const;
		
		void setGlyphWidths( unsigned char glyphTexelWidths[ 256 ] );		// Sets the width of each glyph in the ascii table as measured in texels.
		void setAllGlyphWidths( int texelWidth );
		
		void loadMetricsFile( const std::string& metricsFileName );
		
		float getGlyphWidth( unsigned char asciiCode ) const;
			// Returns the texel-space width of the glyph.
		
		Vector2ui getGlyphCellDimensions() const;
			// REQUIRES( GetTexture() != 0 );
			// The texel-space dimensions of a single cell. This depends on the size of the texture.
			// (It also depends on the number of columns and rows within the texture, but that number is always 16.
		
		void getGlyphTexCoords( unsigned char asciiCode, Vector2f& outUVTranslation ) const;
			// Returns the UV coordinates of the upper left corner of the given glyph.
		
		
	private:
		
		SmartPtr< Texture > m_texture;
		
		unsigned char m_glyphWidths[ NUM_GLYPHS ];
		
		float m_spacingScalar = 1.0f;
	
		FRESH_DECLARE_CLASS( Font, Asset )
	};
	
	class FontLoader : public AssetLoader
	{
	public:
		
		virtual void loadAsset( Asset::ptr asset ) override
		{
			REQUIRES( asset );
			
			Super::loadAsset( asset );

			Font::ptr font = dynamic_freshptr_cast< Font::ptr >( asset );
			ASSERT( font );
			
			font->loadFile( m_filePath, decoratePath( m_basePath, m_fontMetricsFilePath, "" ), m_characterSpacingScalar );
		}
		
	private:
		
		VAR( std::string, m_fontMetricsFilePath );
		DVAR( float, m_characterSpacingScalar, 1.0f );
		
		FRESH_DECLARE_CLASS( FontLoader, AssetLoader )
	};
	
}

#endif
