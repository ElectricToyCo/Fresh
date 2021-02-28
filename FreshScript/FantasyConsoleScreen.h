//
//  FantasyConsoleScreen.h
//  Fresh
//
//  Created by Jeff Wofford on 8/8/18.
//  Copyright (c) 2018 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_FantasyConsoleScreen_h
#define Fresh_FantasyConsoleScreen_h

#include "Renderer.h"
#include "RenderTarget.h"
#include "SimpleMesh.h"

namespace fr
{
	
	class FantasyConsoleScreen : public Object
	{
		FRESH_DECLARE_CLASS( FantasyConsoleScreen, Object );
	public:

		void create( const vec2i& pixelSize );
		
		vec2i size() const;
		
		void beginFrame();
		void clear( const Color& color = Color::Black );
		void endFrame();
		
		Texture::ptr texture() const;
		
	private:
		
		RenderTarget::ptr m_renderTarget;
	};
	
}

#endif
