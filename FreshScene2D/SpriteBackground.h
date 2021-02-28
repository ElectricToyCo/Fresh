/*
 *  SpriteBackground.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/24/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_SPRITE_BACKGROUND_H_INCLUDED
#define FRESH_SPRITE_BACKGROUND_H_INCLUDED

#include "Sprite.h"

namespace fr
{

	class SpriteBackground : public Sprite
	{
		FRESH_DECLARE_CLASS( SpriteBackground, Sprite )
	public:
		
		SYNTHESIZE( DisplayObject::wptr, trackingObject )

		SYNTHESIZE( vec2, trackingTranslationScale );
		SYNTHESIZE( vec2, stageDimensions )
		
		virtual void texture( Texture::ptr texture_ ) override;
		
		virtual void update() override;
		
		virtual void draw( TimeType relativeFrameTime, RenderInjector* injector = nullptr ) override;
		
	protected:
		
		virtual void setupTransforms( TimeType relativeFrameTime ) override;	
		
	private:

		VAR( DisplayObject::wptr, m_trackingObject );	
				// The movement of this object determines the "slide" of the texture over the background sprite. 
				// Typically you would assign this to a "world" display object that also tracks with a camera.
		
		VAR( vec2, m_velocity );
		DVAR( vec2, m_trackingTranslationScale, vec2( 1.0f, 1.0f ));
		VAR( vec2, m_stageDimensions );		// If 0, these are gleaned from stage().stageDimensions();
		DVAR( bool, m_fillStageX, true );
		DVAR( bool, m_fillStageY, true );
		
	};
	
}


#endif
