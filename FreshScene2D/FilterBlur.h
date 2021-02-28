//
//  FilterBlur.h
//  Fresh
//
//  Created by Jeff Wofford on 11/27/15.
//  Copyright (c) 2015 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_FilterBlur_h
#define Fresh_FilterBlur_h

#include "FilterBlur1D.h"

namespace fr
{
	
	class FilterBlur : public FilterBlur1D
	{
		FRESH_DECLARE_CLASS( FilterBlur, FilterBlur1D );
	protected:

		virtual void render( Texture::ptr textureToFilter, const rect& renderRect, TimeType relativeFrameTime ) override;
		virtual void draw( Texture::ptr textureToFilter, TimeType relativeFrameTime ) override;
		
	private:
		
		VAR( vec2, m_blurOffset );
	};
	
}

#endif
