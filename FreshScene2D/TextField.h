/*
 *  TextField.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 7/22/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_TEXT_FIELD_H_INCLUDED
#define FRESH_TEXT_FIELD_H_INCLUDED

#include "../FreshPlatform/Font.h"
#include "DisplayObject.h"
#include "TextMetrics.h"

namespace fr 
{
	
	class TextField : public DisplayObject
	{
	public:
		
		void setFont( const std::string& fontName );
		
		SYNTHESIZE_GET( Font::ptr, font );
		
		SYNTHESIZE( std::string, text );
		SYNTHESIZE( real, fontSize );
		SYNTHESIZE( real, textSpacing );
		SYNTHESIZE( real, lineHeight );
		SYNTHESIZE( TextMetrics::Alignment, alignment );
		SYNTHESIZE( TextMetrics::Enforcement, enforcement );
		SYNTHESIZE( vec2, enforcedBounds );
		
		TextMetrics metrics() const;
		
		size_t textSize() const;

		bool isPrintableCharacter( unsigned int c ) const;
		
		virtual rect localBounds() const override;
		
		virtual bool hitTestPoint( const vec2& localLocation, HitTestFlags flags ) const override;
		
	protected:
		
		virtual void draw( TimeType relativeFrameTime, RenderInjector* injector ) override;
		virtual void emitLine( std::vector< vec2 >& outPoints, std::string::const_iterator iterBegin, std::string::const_iterator iterEnd, vec2 baseTranslation );

		const std::vector< float >& getGlyphPixelWidths() const;
		
		void updateDrawBuffer();
		void constructDrawBuffer();
		
		bool isDrawBufferDirty() const;
		void recordDrawBufferTextState();
		
	private:
		
		VAR( Font::ptr, m_font );
		
		VAR( std::string, m_text );
		
		DVAR( real, m_fontSize, 24.0f );
		DVAR( real, m_textSpacing, 1.0f );
		DVAR( real, m_lineHeight, 1.0f );
		DVAR( TextMetrics::Alignment, m_alignment, TextMetrics::Alignment::Left );
		DVAR( TextMetrics::VerticalAlignment, m_verticalAlignment, TextMetrics::VerticalAlignment::Top );
		DVAR( TextMetrics::Enforcement, m_enforcement, TextMetrics::Enforcement::Wrap );
		
		VAR( vec2, m_enforcedBounds );
		
		VertexBuffer::ptr m_cachedDrawBuffer;
		size_t m_nCachedDrawVertices = 0;
				
		mutable std::vector< float > m_cachedGlyphWidths;
		mutable Font::wptr m_lastCachedGlyphWidthFont;

		std::string m_cachedText;
		real m_cachedFontSize = 0;
		real m_cachedTextSpacing = 0;
		real m_cachedLineHeight = 0;
		TextMetrics::Alignment m_cachedAlignment = TextMetrics::Alignment::Left;
		TextMetrics::Enforcement m_cachedEnforcement = TextMetrics::Enforcement::Wrap;
		vec2 m_cachedEnforcedBounds;
		
		FRESH_DECLARE_CLASS( TextField, DisplayObject )
	};
	
}

#endif
