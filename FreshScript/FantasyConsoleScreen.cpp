//
//  FantasyConsoleScreen.cpp
//  Fresh
//
//  Created by Jeff Wofford on 8/8/18.
//  Copyright (c) 2018 Jeff Wofford. All rights reserved.
//

#include "FantasyConsoleScreen.h"
#include "Stage.h"

namespace fr
{
	FRESH_DEFINE_CLASS( FantasyConsoleScreen )

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( FantasyConsoleScreen )

	void FantasyConsoleScreen::create( const vec2i& pixelSize )
	{
		REQUIRES( 0 < pixelSize.x && pixelSize.x <= 4096 && 0 < pixelSize.y && pixelSize.y <= 4096 );

		if( !m_renderTarget )
        {
            m_renderTarget = createObject< RenderTarget >();
        }

        if( size() != pixelSize )
        {
            m_renderTarget->create( pixelSize.x, pixelSize.y, RenderTarget::BufferFormat{ RenderTarget::ColorComponentType::UnsignedByte, RenderTarget::OutputType::Texture } );
        }
	}

	vec2i FantasyConsoleScreen::size() const
	{
		return vec2i( m_renderTarget->width(), m_renderTarget->height() );
	}

	void FantasyConsoleScreen::beginFrame()
	{
		m_renderTarget->beginCapturing();

		auto& renderer = Renderer::instance();
		renderer.pushMatrix( Renderer::MAT_ModelView );
		renderer.setMatrixToIdentity( Renderer::MAT_ModelView );

		renderer.pushMatrix( Renderer::MAT_Projection );
		renderer.setOrthoProjection( 0, size().x, size().y, 0 );
	}

	void FantasyConsoleScreen::clear( const Color& color )
	{
        auto& renderer = Renderer::instance();

		const bool wasCapturing = m_renderTarget->isCapturing();
		if( !wasCapturing )
		{
        	m_renderTarget->beginCapturing();
		}

		const auto retainedClearColor = renderer.clearColor();

		renderer.clearColor( color );
		renderer.clear();

		renderer.clearColor( retainedClearColor );

		if( !wasCapturing )
		{
			m_renderTarget->endCapturing();
		}
	}

	void FantasyConsoleScreen::endFrame()
	{
		m_renderTarget->endCapturing();

		auto& renderer = Renderer::instance();
		renderer.popMatrix( Renderer::MAT_ModelView );
		renderer.popMatrix( Renderer::MAT_Projection );
	}

	Texture::ptr FantasyConsoleScreen::texture() const
	{
		return m_renderTarget->getCapturedTexture();
	}
}

