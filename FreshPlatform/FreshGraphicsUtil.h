//
//  FreshGraphicsUtil.h
//  Fresh
//
//  Created by Jeff Wofford on 11/29/11.
//  Copyright (c) 2011 jeffwofford.com. All rights reserved.
//

#ifndef TestAppGraphics_FreshGraphicsUtil_h
#define TestAppGraphics_FreshGraphicsUtil_h

#include "FreshVector.h"

namespace fr
{
	
	struct VertexPos3Col
	{
		vec3 pos;
		vec2 texCoord;
		vec4 color;
	};

	void drawQuad2D();
	void drawOrigin( real length = 1.0f );	
	void drawWorldGrid( const Vector3i& gridSize );

}

#endif
