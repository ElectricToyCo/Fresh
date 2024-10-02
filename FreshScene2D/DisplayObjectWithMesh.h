//
//  DisplayObjectWithMesh.h
//  Fresh
//
//  Created by Jeff Wofford on 12/7/11.
//  Copyright (c) 2011 jeffwofford.com. All rights reserved.
//

#ifndef Fresh_DisplayObjectWithMesh_h
#define Fresh_DisplayObjectWithMesh_h

#include "DisplayObjectContainer.h"
#include "SimpleMesh.h"
#include "Rectangle.h"
#include "Texture.h"
#include "RenderTarget.h"

namespace fr
{
	class DisplayObjectWithMesh : public DisplayObjectContainer
	{
	public:

		SYNTHESIZE( SimpleMesh::ptr, mesh )

		Texture::ptr texture() const;
		virtual void texture( Texture::ptr texture );
		void setTextureByName( const std::string& strTextureName )	{ setTextureByName( strTextureName.c_str() ); }
		void setTextureByName( const char* szTextureName );
		// If the texture name is null or 0-length, sets the texture to null.

		SYNTHESIZE( rect, textureWindow );

		virtual Renderer::BlendMode calculatedBlendMode() const override;

		virtual rect localBounds() const override;

		virtual bool hitTestPoint( const vec2& localLocation, HitTestFlags flags ) const override;

	protected:

		Texture::ptr effectiveTexture() const { return m_renderTargetTexture ? m_renderTargetTexture->getCapturedTexture() : m_texture; }

		virtual void draw( TimeType relativeFrameTime, RenderInjector* injector ) override;
		virtual void drawMesh( TimeType relativeFrameTime );
		bool hitTestPointAgainstMesh( const vec2& localLocation ) const;

	private:

		VAR( SimpleMesh::ptr, m_mesh );

		VAR( Texture::ptr, m_texture );
		DVAR( rect, m_textureWindow, rect( 0, 0, 1.0f, 1.0f ) );

		VAR( RenderTarget::ptr, m_renderTargetTexture ); // if both m_texture and m_renderTargetTexture are non-null renderTargetTexture wins.

		FRESH_DECLARE_CLASS( DisplayObjectWithMesh, DisplayObjectContainer )

	};
}

#endif
