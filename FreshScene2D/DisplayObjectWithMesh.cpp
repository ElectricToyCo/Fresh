//
//  DisplayObjectWithMesh.cpp
//  Fresh
//
//  Created by Jeff Wofford on 12/7/11.
//  Copyright (c) 2011 jeffwofford.com. All rights reserved.
//

#include "DisplayObjectWithMesh.h"

#ifdef FRESH_DEBUG_HITTESTS
#	define trace_hittest( expr ) trace( expr )
#else
#	define trace_hittest( expr )
#endif

namespace fr
{
	FRESH_DEFINE_CLASS( DisplayObjectWithMesh )

	DEFINE_VAR_FLAG( DisplayObjectWithMesh, SimpleMesh::ptr, m_mesh, PropFlag::LoadDefault );

	DEFINE_VAR_FLAG( DisplayObjectWithMesh, Texture::ptr, m_texture, PropFlag::LoadDefault );
	DEFINE_VAR( DisplayObjectWithMesh, rect, m_textureWindow );
	DEFINE_VAR( DisplayObjectWithMesh, RenderTarget::ptr, m_renderTargetTexture );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( DisplayObjectWithMesh )

	void DisplayObjectWithMesh::texture( Texture::ptr texture )
	{
		m_texture = texture;
	}

	void DisplayObjectWithMesh::setTextureByName( const char* szTextureName )
	{
		if( !szTextureName || strlen( szTextureName ) == 0 )
		{
			texture( nullptr );
		}
		else
		{
			texture( Renderer::instance().createTexture( szTextureName ));
		}
	}

	Texture::ptr DisplayObjectWithMesh::texture() const
	{
		return m_texture;
	}

	void DisplayObjectWithMesh::draw( TimeType relativeFrameTime, RenderInjector* injector )
	{
		if( !injector || !injector->draw( relativeFrameTime, *this ))
		{
			drawMesh( relativeFrameTime );
		}
		Super::draw( relativeFrameTime, injector );
	}

	Renderer::BlendMode DisplayObjectWithMesh::calculatedBlendMode() const
	{
		if( effectiveTexture() && blendMode() == Renderer::BlendMode::None )
		{
			return Renderer::getBlendModeForTextureAlphaUsage( effectiveTexture()->alphaUsage() );
		}
		else
		{
			return Super::calculatedBlendMode();
		}
	}

	void DisplayObjectWithMesh::drawMesh( TimeType relativeFrameTime )
	{
		if( m_mesh && m_mesh->isReadyToDraw() )
		{
			Renderer& renderer = Renderer::instance();

			renderer.applyTexture( effectiveTexture() );	// Might be null. That's okay.

			// Transform for texture window.
			//
			bool pushedTextureMatrix = false;
			if( m_textureWindow.left() != 0.0f || m_textureWindow.top() != 0.0f )
			{
				if( !pushedTextureMatrix )
				{
					renderer.pushMatrix( Renderer::MAT_Texture );
					pushedTextureMatrix = true;
				}

				renderer.translate( m_textureWindow.left(), m_textureWindow.top(), Renderer::MAT_Texture );
			}
			if( m_textureWindow.width() != 1.0f || m_textureWindow.height() != 1.0f )
			{
				if( !pushedTextureMatrix )
				{
					renderer.pushMatrix( Renderer::MAT_Texture );
					pushedTextureMatrix = true;
				}

				renderer.scale( m_textureWindow.width(), m_textureWindow.height(), Renderer::MAT_Texture );
			}

			// Draw.
			//
			renderer.updateUniformsForCurrentShaderProgram( this );
			m_mesh->draw();

			if( pushedTextureMatrix )
			{
				renderer.popMatrix( Renderer::MAT_Texture );
			}
		}
	}

	rect DisplayObjectWithMesh::localBounds() const
	{
		rect bounds = DisplayObjectContainer::localBounds();
		if( m_mesh )
		{
			bounds.growToEncompass( m_mesh->bounds() );
		}

		if( !bounds.isWellFormed() )	// True if no bounds or an uninitialized mesh.
		{
			bounds.set( 0, 0, 0, 0 );
		}

		return bounds;
	}

	bool DisplayObjectWithMesh::hitTestPoint( const vec2& localLocation, HitTestFlags flags ) const
	{
		trace_hittest( toString() << ".DisplayObjectWithMesh::" << FRESH_CURRENT_FUNCTION << "(" << localLocation << "," << flags << "):" );

		if( rejectsOnTheBasisOfHitFlags( flags ))
		{
			trace_hittest( "DisplayObjectWithMesh rejecting because of flags" );
			return false;
		}

		if( !hitTestMask( localLocation, flags ))
		{
			return false;
		}

		// Does our mesh touch the location?
		if( hitTestPointAgainstMesh( localLocation ))
		{
			trace_hittest( "DisplayObjectWithMesh touched mesh bounds." );
			return true;
		}

		if( Super::hitTestPoint( localLocation, flags ))
		{
			trace_hittest( "DisplayObjectWithMesh touched via Super." );
			return true;
		}

		trace_hittest( "DisplayObjectWithMesh found no touch." );
		return false;
	}

	bool DisplayObjectWithMesh::hitTestPointAgainstMesh( const vec2& localLocation ) const
	{
		// Does our mesh touch the location?
		if( m_mesh && m_mesh->bounds().doesEnclose( localLocation, true ))
		{
			trace_hittest( "DisplayObjectWithMesh touched mesh bounds." );
			return true;
		}
		return false;
	}
}
