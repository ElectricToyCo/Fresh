//
//  Segment.h
//  Fresh
//
//  Created by Jeff Wofford on 12/4/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_Segment_h
#define Fresh_Segment_h

#include "FreshVector.h"

namespace fr
{
	
	typedef std::pair< vec2, vec2 > Segment;
	typedef std::vector< Segment > Segments;
	
	real getRadius( const Segments& blockers );
	void createTrianglesForBlocker( const Segment& blocker, const vec2& lightPos, real lightRadius, std::vector< vec2 >& outTrianglePoints, real lightSpotHalfArc, const vec2& lightSpotDirection );

}

#endif
