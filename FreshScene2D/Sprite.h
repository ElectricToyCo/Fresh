/*
 *  Sprite.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/18/08.
 *  Copyright 2008 jeffwofford.com. All rights reserved.
 *
 */

#ifndef SPRITE_H_INCLUDED_
#define SPRITE_H_INCLUDED_

#include "Object.h"
#include "Vector2.h"
#include "DisplayObjectWithMesh.h"
#include "Graphics.h"
#include <vector>

namespace fr
{
	
	class Texture;
	
	class Sprite : public DisplayObjectWithMesh
	{
	public:
			
		Graphics& graphics() const;
				
		vec2 baseDimensions() const;
		vec2 getScaledDimensions() const;
		
		virtual void draw( TimeType relativeFrameTime, RenderInjector* injector ) override;
		
		virtual bool hitTestPoint( const vec2& localLocation, HitTestFlags flags ) const override;
		
		virtual rect localBounds() const override;

		virtual Renderer::BlendMode calculatedBlendMode() const override;
		
	protected:
		
		virtual void drawSprite( TimeType relativeFrameTime );
		
	private:
		
		mutable SmartPtr< Graphics > m_graphics;
		
		static SmartPtr< SimpleMesh > s_standardMesh;

		FRESH_DECLARE_CLASS( Sprite, DisplayObjectWithMesh )

	};
		
}

#endif
