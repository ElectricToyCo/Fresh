
//  DisplayFilter.h
//  Fresh
//
//  Created by Jeff Wofford on 8/29/15.
//  Copyright (c) 2015 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_DisplayFilter_h
#define Fresh_DisplayFilter_h

#include "Texture.h"
#include "ShaderProgram.h"
#include "SimpleMesh.h"
#include "RenderTarget.h"

namespace fr
{
	
	class DisplayFilter : public Object
	{
		FRESH_DECLARE_CLASS( DisplayFilter, Object );
	public:

		SYNTHESIZE( Renderer::BlendMode, blendMode );
		SYNTHESIZE( Color, color );

		void addFilter( DisplayFilter::ptr filter );
//		REQUIRES( filter );
//		REQUIRES( filter != this );
//		REQUIRES( !hasDescendant( filter ));
//		PROMISES( hasDescendant( filter ));

		bool hasDescendant( DisplayFilter::ptr filter ) const;
		
		virtual void render( Texture::ptr textureToFilter, const rect& renderRect, TimeType relativeFrameTime );

	protected:
		
		DVAR( Renderer::BlendMode, m_blendMode, Renderer::BlendMode::AlphaPremultiplied );
		DVAR( Color, m_color, Color::White );
		VAR( std::vector< DisplayFilter::ptr >, m_filters );	// If non-empty, the current filter will render to a render target, then pass itself on to these.
		
		virtual ShaderProgram::ptr shaderProgram( Texture::ptr textureToFilter ) const;
		virtual void establishMesh( const rect& renderRect );
		
		virtual void draw( Texture::ptr textureToFilter, TimeType relativeFrameTime );
		// REQUIRES( textureToFilter );

		virtual vec2 rectExpansion() const;					// Per axis. Negative values contract.
		
		virtual RenderTarget::ptr beginSelfCapture( const rect& renderRect );
		virtual void endSelfCapture( RenderTarget::ptr renderTarget, const rect& renderRect, TimeType relativeFrameTime );

	private:
		
		SimpleMesh::ptr m_mesh;
		rect m_lastRenderRect;				// Used to determine when the mesh needs to be recreated.
		vec2 m_lastRectExpansion;
	};
	
}

#endif
