/*
 *  TextField.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 7/22/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#include "TextField.h"
#include "Objects.h"
#include "Texture.h"
#include "Renderer.h"
#include "Stage.h"

namespace fr
{
	
	FRESH_DEFINE_CLASS( TextField )
	
	DEFINE_VAR( TextField, Font::ptr, m_font );
	DEFINE_VAR( TextField, std::string, m_text );
	DEFINE_VAR( TextField, real, m_fontSize );
	DEFINE_VAR( TextField, real, m_textSpacing );
	DEFINE_VAR( TextField, real, m_lineHeight );
	DEFINE_VAR( TextField, TextMetrics::Alignment, m_alignment );
	DEFINE_VAR( TextField, TextMetrics::VerticalAlignment, m_verticalAlignment );
	DEFINE_VAR( TextField, TextMetrics::Enforcement, m_enforcement );
	DEFINE_VAR( TextField, vec2, m_enforcedBounds );

	TextField::TextField( CreateInertObject c )
	:	DisplayObject( c )
	{
		isTouchEnabled( false );
	}
	
	TextField::TextField( const ClassInfo& assignedClassInfo, NameRef objectName /* = DEFAULT_OBJECT_NAME */ )
	:	DisplayObject( assignedClassInfo, objectName )
	{
		isTouchEnabled( false );
	}

	bool TextField::isPrintableCharacter( unsigned int c ) const
	{
		return c >= ' ' || c == '\n';
	}
	
	void TextField::setFont( const std::string& fontName )
	{
		m_font = Font::getFont( fontName );
	}

	size_t TextField::textSize() const
	{
		return m_text.size();
	}
	
	TextMetrics TextField::metrics() const
	{
		return TextMetrics( m_text.begin(), m_text.end(), m_fontSize, m_lineHeight, getGlyphPixelWidths(), m_enforcedBounds.x, m_enforcedBounds.y, m_enforcement, m_alignment, m_verticalAlignment );
	}
	
	void TextField::draw( TimeType relativeFrameTime, RenderInjector* injector )
	{
		if( !( !injector || !injector->draw( relativeFrameTime, *this )))
		{
			return;
		}
		
		DisplayObject::draw( relativeFrameTime, injector );
		
		if( !m_font || !m_font->texture() ) return;
		
		if( !m_cachedDrawBuffer )
		{
			constructDrawBuffer();
		}
		
		updateDrawBuffer();
		
		if( m_nCachedDrawVertices > 0 )
		{
			// Setup general rendering state for whole text block.
			//
			Renderer& renderer = Renderer::instance();

			renderer.pushMatrix( Renderer::MAT_ModelView );
			
			renderer.scale( vec2( m_fontSize, m_fontSize ));

			renderer.applyTexture( m_font->texture() );
			
			Renderer::BlendMode calculatedBlendMode = Renderer::getBlendModeForTextureAlphaUsage( m_font->texture()->alphaUsage() );
			if( blendMode() != Renderer::BlendMode::None )
			{
				calculatedBlendMode = Super::calculatedBlendMode();
			}

			renderer.setBlendMode( calculatedBlendMode );

			renderer.updateUniformsForCurrentShaderProgram( this );
			renderer.drawGeometry( Renderer::PrimitiveType::Triangles, m_cachedDrawBuffer, m_nCachedDrawVertices );
			
			renderer.popMatrix( Renderer::MAT_ModelView );					// Corresponding to the push for reseting starts of lines.
		}
	}

	void TextField::emitLine( std::vector< vec2 >& outPoints, std::string::const_iterator iterBegin, std::string::const_iterator iterEnd, vec2 baseTranslation )
	{
		const float DEFAULT_CHARACTER_UV_SIZE = 1.0f / 16.0f;
		
		const real glyphCellWidth = static_cast< real >( m_font->getGlyphCellDimensions().x );
		
		for( ; iterBegin != iterEnd; ++iterBegin )
		{
			unsigned char asciiCode = *iterBegin;

			// We're not expecting newlines in this line. It is a line after all.
			//
			ASSERT( asciiCode != '\n' );
			ASSERT( asciiCode != '\r' );
			
			// If this is a non-breaking space, force it to use the space character.
			//
			if( asciiCode == TextMetrics::NON_BREAKING_SPACE )
			{
				asciiCode = ' ';
			}
			
			// Calculate the UVs for this glyph. (All font bitmaps are assumed to be a 16x16 grid.)
			//
			Vector2f uvGlyphUL;
			m_font->getGlyphTexCoords( asciiCode, uvGlyphUL );
			
			// Draw the character.
			//
			outPoints.push_back( baseTranslation );																// LU pos
			outPoints.push_back( uvGlyphUL );																	// LU uv
			outPoints.push_back( baseTranslation + Vector2f( 0.0f, 1.0f ));										// LB pos
			outPoints.push_back( uvGlyphUL + Vector2f( 0.0f, DEFAULT_CHARACTER_UV_SIZE ));						// LB uv
			outPoints.push_back( baseTranslation+ Vector2f( 1.0f, 0.0f ));										// RU pos
			outPoints.push_back( uvGlyphUL + Vector2f( DEFAULT_CHARACTER_UV_SIZE, 0 ));							// RU uv
			outPoints.push_back( baseTranslation+ Vector2f( 1.0f, 0.0f ));										// RU pos
			outPoints.push_back( uvGlyphUL + Vector2f( DEFAULT_CHARACTER_UV_SIZE, 0 ));							// RU uv
			outPoints.push_back( baseTranslation + Vector2f( 0.0f, 1.0f ));										// LB pos
			outPoints.push_back( uvGlyphUL + Vector2f( 0.0f, DEFAULT_CHARACTER_UV_SIZE ));						// LB uv
			outPoints.push_back( baseTranslation + Vector2f( 1.0f, 1.0f ));										// RB pos
			outPoints.push_back( uvGlyphUL + Vector2f( DEFAULT_CHARACTER_UV_SIZE, DEFAULT_CHARACTER_UV_SIZE ));	// RB uv
			
			// Setup the position of the next character on the screen.
			//
			const real glyphWidth = m_font->getGlyphWidth( asciiCode ) / glyphCellWidth * m_textSpacing;
			baseTranslation += vec2( glyphWidth, 0 );
		}
	}
	
	const std::vector< float >& TextField::getGlyphPixelWidths() const
	{
		if( m_lastCachedGlyphWidthFont != m_font )
		{
			if( m_font )
			{
				const float glyphCellWidth = static_cast< float >( m_font->getGlyphCellDimensions().x );
				m_cachedGlyphWidths.resize( Font::NUM_GLYPHS );
				
				for( size_t i = 0; i < Font::NUM_GLYPHS; ++i )
				{
					m_cachedGlyphWidths[ i ] = m_font->getGlyphWidth( static_cast< unsigned char >( i )) / glyphCellWidth * m_textSpacing;
				}
			}
			else
			{
				m_cachedGlyphWidths.clear();
			}
			
			m_lastCachedGlyphWidthFont = m_font;
		}
		return m_cachedGlyphWidths;
	}

	rect TextField::localBounds() const
	{
		TextMetrics textMetrics( metrics() );
		
		return textMetrics.bounds();
	}

	bool TextField::hitTestPoint( const vec2& localLocation, HitTestFlags flags ) const
	{
		if( rejectsOnTheBasisOfHitFlags( flags ))
		{
			return false;
		}		

		if( !hitTestMask( localLocation, flags ))
		{
			return false;
		}
		
		const rect boundingRect( localBounds() );
		return boundingRect.doesEnclose( localLocation, true );
	}

	void TextField::constructDrawBuffer()
	{
		m_cachedDrawBuffer = createObject< VertexBuffer >();
		
		m_cachedDrawBuffer->addAttribute( "position", 2, VertexStructure::Float, VertexStructure::Position );
		m_cachedDrawBuffer->addAttribute( "texCoord", 2, VertexStructure::Float, VertexStructure::TexCoord );
		
		m_cachedDrawBuffer->associateWithVertexStructure( Stage::getPos2TexCoord2VertexStructure());
	}

	bool TextField::isDrawBufferDirty() const
	{
		return
			m_cachedText != m_text ||
			m_cachedFontSize != m_fontSize ||
			m_cachedTextSpacing != m_textSpacing ||
			m_cachedLineHeight != m_lineHeight ||
			m_cachedAlignment != m_alignment ||
			m_cachedEnforcement != m_enforcement ||
			m_cachedEnforcedBounds != m_enforcedBounds;
	}

	void TextField::recordDrawBufferTextState()
	{
		m_cachedText = m_text;
		m_cachedFontSize = m_fontSize;
		m_cachedTextSpacing = m_textSpacing;
		m_cachedLineHeight = m_lineHeight;
		m_cachedAlignment = m_alignment;
		m_cachedEnforcement = m_enforcement;
		m_cachedEnforcedBounds = m_enforcedBounds;
	}
	
	void TextField::updateDrawBuffer()
	{
		ASSERT( m_cachedDrawBuffer );
		
		if( isDrawBufferDirty() )
		{
			std::vector< vec2 > points;	// Position, texcoord, in that order.

			//
			// Emit each line.
			//
			
			if( m_fontSize > 0 )
			{
				TextMetrics textMetrics( metrics() );
				textMetrics.forEachLine( [&]( const TextMetrics::Line& line, size_t iLine )
										{
											emitLine( points, line.text.begin(), line.text.end(), vec2( line.bounds[ 0 ], line.bounds[ 1 ] ) / m_fontSize );
										} );
				
				if( points.size() > 1 && m_enforcedBounds.x > 0 )
				{
					// Scale the points if one of the Scale enforcements are chosen.
					//
					if( m_enforcement == TextMetrics::Enforcement::Scale ||
						m_enforcement == TextMetrics::Enforcement::ScaleDown ||
						m_enforcement == TextMetrics::Enforcement::ScaleUp )
					{
						const auto& lastPos = points[ points.size() - 2 ];		// The very last point is a UV coordinate.
						
						const real actualTextWidth = lastPos.x * m_fontSize;
						ASSERT( actualTextWidth > 0 );
						
						const real scale = m_enforcedBounds.x / actualTextWidth;
						
						if( m_enforcement == TextMetrics::Enforcement::Scale ||
						   ( m_enforcement == TextMetrics::Enforcement::ScaleDown && scale < 1.0f ) ||
						   ( m_enforcement == TextMetrics::Enforcement::ScaleUp && scale > 1.0f ))
						{
							// Actually scale the positions (not the UVs).
							//
							for( auto iter = points.begin(); iter < points.end(); iter += 2 )
							{
								*iter *= scale;
							}
						}
					}
				}
			}
			
			m_nCachedDrawVertices = points.size() / 2;
			
			if( m_nCachedDrawVertices > 0 )
			{
				m_cachedDrawBuffer->loadVertices( points.begin(), points.end() );
			}
			else
			{
				m_cachedDrawBuffer->clear();
			}
			
			recordDrawBufferTextState();
		}
	}
}
