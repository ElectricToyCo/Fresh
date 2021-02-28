//
//  DisplayFilter.cpp
//  Fresh
//
//  Created by Jeff Wofford on 8/29/15.
//  Copyright (c) 2015 Jeff Wofford. All rights reserved.
//

#include "DisplayFilter.h"

namespace fr
{	
	FRESH_DEFINE_CLASS( DisplayFilter )
	DEFINE_VAR( DisplayFilter, Renderer::BlendMode, m_blendMode );
	DEFINE_VAR( DisplayFilter, Color, m_color );
	DEFINE_VAR( DisplayFilter, std::vector< DisplayFilter::ptr >, m_filters );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( DisplayFilter )

	void DisplayFilter::addFilter( DisplayFilter::ptr filter )
	{
		REQUIRES( filter );
		REQUIRES( filter != this );
		REQUIRES( !hasDescendant( filter ));
		
		m_filters.push_back( filter );
		
		PROMISES( hasDescendant( filter ));
	}
	
	bool DisplayFilter::hasDescendant( DisplayFilter::ptr filter ) const
	{
		REQUIRES( filter );
		
		if( filter == this )
		{
			return true;
		}
		
		for( const auto& child: m_filters )
		{
			if( child == filter || child->hasDescendant( filter ))
			{
				return true;
			}
		}
		return false;
	}
	
	void DisplayFilter::render( Texture::ptr textureToFilter, const rect& renderRect, TimeType relativeFrameTime )
	{
		if( textureToFilter && renderRect.dimensions().isZero() == false )
		{
			establishMesh( renderRect );
			
			RenderTarget::ptr captureTarget;
			if( !m_filters.empty() )
			{
				captureTarget = beginSelfCapture( renderRect );
			}
			
			draw( textureToFilter, relativeFrameTime );
			
			endSelfCapture( captureTarget, renderRect, relativeFrameTime );
		}
	}

	vec2 DisplayFilter::rectExpansion() const
	{
		return vec2::ZERO;
	}
	
	RenderTarget::ptr DisplayFilter::beginSelfCapture( const rect& renderRect )
	{
		auto renderTarget = createObject< RenderTarget >();
		renderTarget->clearColor( Color::Invisible );
		renderTarget->doInitialClearOnCapture( true );
		
		// Establish the filter rendering target.
		//
		RenderTarget::BufferFormat format{ RenderTarget::ColorComponentType::UnsignedByte, RenderTarget::OutputType::Texture };
		
		vec2i adjustedRenderSize = vector_cast< int >( renderRect.dimensions() );
		for( int i = 0; i < 2; ++i )
		{
			adjustedRenderSize[ i ] = clamp( adjustedRenderSize[ i ], 1, Texture::maxAllowedSize() );
		}
		
		renderTarget->create( adjustedRenderSize.x, adjustedRenderSize.y, format );
		
		renderTarget->beginCapturing();
		
		// Adjust further rendering to sit appropriately within the render target area.
		//
		auto& renderer = Renderer::instance();
		renderer.pushMatrix( Renderer::MAT_ModelView );
		renderer.setMatrixToIdentity( Renderer::MAT_ModelView );
		
		renderer.pushMatrix( Renderer::MAT_Projection );
		renderer.setOrthoProjection( renderRect.left(), renderRect.right(), renderRect.bottom(), renderRect.top() );
		
		return renderTarget;
	}
	
	void DisplayFilter::endSelfCapture( RenderTarget::ptr renderTarget, const rect& renderRect, TimeType relativeFrameTime )
	{
		if( renderTarget )
		{
			ASSERT( renderTarget->isCapturing() );
			renderTarget->endCapturing();
			
			Texture::ptr capturedTexture = renderTarget->getCapturedTexture();
			
			auto& renderer = Renderer::instance();
			
			renderer.popMatrix( Renderer::MAT_ModelView );
			renderer.popMatrix( Renderer::MAT_Projection );
			
			renderer.pushMatrix( Renderer::MAT_Texture );
			renderer.setMatrixToIdentity( Renderer::MAT_Texture );
			
			rect adjustedRect = m_lastRenderRect;
			adjustedRect.expandContract( m_lastRectExpansion );
			
			for( auto filter : m_filters )
			{
				ASSERT( filter );
				filter->render( capturedTexture, adjustedRect, relativeFrameTime );
			}
			
			renderer.popMatrix( Renderer::MAT_Texture );
		}
	}
	
	void DisplayFilter::establishMesh( const rect& renderRect )
	{
		const auto expansion = rectExpansion();
		
		if( renderRect != m_lastRenderRect || expansion != m_lastRectExpansion )
		{
			rect adjustedRect = renderRect;
			adjustedRect.expandContract( expansion );
			
			std::vector< vec2 > points;
			points.emplace_back( adjustedRect.ulCorner() );
			points.emplace_back( proportion( adjustedRect.ulCorner(), renderRect.ulCorner(), renderRect.brCorner() ));					// TexCoords
			points.emplace_back( adjustedRect.urCorner() );
			points.emplace_back( proportion( adjustedRect.urCorner(), renderRect.ulCorner(), renderRect.brCorner() ));					// TexCoords
			points.emplace_back( adjustedRect.blCorner() );
			points.emplace_back( proportion( adjustedRect.blCorner(), renderRect.ulCorner(), renderRect.brCorner() ));					// TexCoords
			points.emplace_back( adjustedRect.brCorner() );
			points.emplace_back( proportion( adjustedRect.brCorner(), renderRect.ulCorner(), renderRect.brCorner() ));					// TexCoords
			
			m_mesh = createObject< SimpleMesh >();
			m_mesh->create( Renderer::PrimitiveType::TriangleStrip,
						 points,
						 Renderer::instance().createOrGetVertexStructure( "VS_Pos2TexCoord2" ),
						 2 );
			m_mesh->calculateBounds( points, 2 );
			
			m_lastRenderRect = renderRect;
			m_lastRectExpansion = expansion;
		}
	}

	ShaderProgram::ptr DisplayFilter::shaderProgram( Texture::ptr textureToFilter ) const
	{
		return getObject< ShaderProgram >( "PlainVanilla" );
	}

	void DisplayFilter::draw( Texture::ptr textureToFilter, TimeType relativeFrameTime )
	{
		REQUIRES( textureToFilter );
		
		auto shaderProgram = this->shaderProgram( textureToFilter );
		
		if( m_mesh && shaderProgram )
		{
			Renderer& renderer = Renderer::instance();

			textureToFilter->setClampMode( Texture::ClampMode::Clamp, Texture::ClampMode::Clamp );
			textureToFilter->filterMode( Texture::FilterMode::Nearest );		// TODO: Make configurable
			renderer.applyTexture( textureToFilter );
			
			renderer.useShaderProgram( shaderProgram );
			
			renderer.setBlendMode( m_blendMode );
			
			renderer.pushColor();

			// TODO State tweening
			
			renderer.multiplyColor( m_color );

			// Draw.
			//
			renderer.updateUniformsForCurrentShaderProgram( this );
			m_mesh->draw();
			
			renderer.popColor();
		}
	}
}

