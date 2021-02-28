//
//  FilterBlur.cpp
//  Fresh
//
//  Created by Jeff Wofford on 11/27/15.
//  Copyright (c) 2015 Jeff Wofford. All rights reserved.
//

#include "FilterBlur.h"

namespace fr
{	
	FRESH_DEFINE_CLASS( FilterBlur )
	DEFINE_VAR( FilterBlur, vec2, m_blurOffset );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( FilterBlur )

	void FilterBlur::render( Texture::ptr textureToFilter, const rect& renderRect, TimeType relativeFrameTime )
	{
		// Construct the vertical child blur if needed.
		//
		if( m_filters.empty() )
		{
			auto vertBlur = createObject< FilterBlur1D >();
			vertBlur->axis( 1 );
			vertBlur->offset( m_blurOffset.y );
			
			m_filters.push_back( vertBlur );
		}
		ASSERT( m_filters.size() == 1 );
		
		Super::render( textureToFilter, renderRect, relativeFrameTime );
	}
	
	void FilterBlur::draw( Texture::ptr textureToFilter, TimeType relativeFrameTime )
	{
		axis( 0 );
		offset( m_blurOffset.x );
		
		auto vertBlur = m_filters[ 0 ]->as< FilterBlur1D >();
		ASSERT( vertBlur );
		vertBlur->size( size() );
		
		const auto stowedBlendMode = m_blendMode;
		const auto stowedColor = m_color;
		
		vertBlur->blendMode( m_blendMode );
		vertBlur->color( m_color );
		vertBlur->glow( glow() );
		
		m_color = Color::White;
		m_blendMode = Renderer::BlendMode::AlphaPremultiplied;

		Super::draw( textureToFilter, relativeFrameTime );
		
		m_blendMode = stowedBlendMode;
		m_color = stowedColor;
	}
}

