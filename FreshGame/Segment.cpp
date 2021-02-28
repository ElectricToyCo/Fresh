//
//  Segment.cpp
//  Fresh
//
//  Created by Jeff Wofford on 12/4/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//

#include "Segment.h"

namespace
{
	using namespace fr;

	// Utility function for createTrianglesForBlocker().
	//
	inline vec2 getProjectedPoint( const vec2& point, real radius )
	{
		vec2 normal = point.normal();
		ASSERT( !normal.isZero() );
		
		return normal * radius / std::abs( normal.majorAxisValue() );
	}
	
}

namespace fr
{	
	real getRadius( const Segments& blockers )
	{
		real maxDistanceSquared = std::numeric_limits< real >::min();
		for( const auto& blocker : blockers )
		{
			const real distSquared = blocker.first.lengthSquared();
			
			if( distSquared > maxDistanceSquared )
			{
				maxDistanceSquared = distSquared;
			}
		}
		
		return std::sqrt( maxDistanceSquared );
	}
	
	void createTrianglesForBlocker( const Segment& blocker, const vec2& lightPos, real lightRadius, std::vector< vec2 >& outTrianglePoints, real lightSpotHalfArc, const vec2& lightSpotDirection )
	{
		const real lightRadiusSquared = lightRadius * lightRadius;
		
		const vec2 blockerDir(( blocker.second - blocker.first ).normal() );
		vec2 blockerNormal( blockerDir );
		blockerNormal.quickRot90();
		
		// Move the blocker into light space and also
		// de-const the points in anticipation of possible modification (due to clipping).
		//
		vec2 points[] = { blocker.first - lightPos, blocker.second - lightPos };
		
		// Does this blocker face the light?
		//
		if( points[ 0 ].dot( blockerNormal ) >= 0 )	// points[ 0 ] is arbitrary. Could choose either point.
		{
			// Nope. Backface culled. Get outta here.
			//
			return;
		}
		
		// For spotlights, is the blocker entirely outside the spot cone?
		//
		if( lightSpotHalfArc > 0 )
		{
			ASSERT( lightSpotHalfArc < 90.0f );
			
			const real cosSpotHalfArc = std::cos( degreesToRadians( lightSpotHalfArc ));
			
			const vec2 pointNormalA = points[ 0 ].normal();
			real blockerPointCosAngleA = lightSpotDirection.dot( pointNormalA );
			
			if( blockerPointCosAngleA < cosSpotHalfArc )
			{
				// This point is outside the cone. What about the other?
				
				const vec2 pointNormalB = points[ 1 ].normal();
				real blockerPointCosAngleB = lightSpotDirection.dot( pointNormalB );
				
				if( blockerPointCosAngleB < cosSpotHalfArc )
				{
					// Both points are outside of the cone, but are they both on the same side?
					
					vec2 lightPerpendicularDirection( lightSpotDirection );
					lightPerpendicularDirection.quickRot90();
					
					if( signNoZero( pointNormalA.dot( lightPerpendicularDirection )) ==
					   signNoZero( pointNormalB.dot( lightPerpendicularDirection )))
					{
						// Both points are outside the cone, both on the same side.
						// Don't generate geometry for this blocker for this light.
						//
						return;
					}
				}
			}
			
		}
		
		// Is the blocker at all within the light?
		//
		bool isOutside[ 2 ];
		bool isAtLeastOneOutside = false;
		for( int i = 0; i < 2; ++i )
		{
			isOutside[ i ] = points[ i ].lengthSquared() > lightRadiusSquared;
			isAtLeastOneOutside = isOutside[ i ] || isAtLeastOneOutside;
		}
		
		if( isAtLeastOneOutside )
		{
			// At least one of them is beyond. Cull or clip.
			//
			if( isOutside[ 0 ] && isOutside[ 1 ] )
			{
				// Both are beyond. Cull this blocker.
				//
				return;
			}
			else
			{
				// Just one of them is. Clip it to be within the radius.
				//
				const int whichOutside = isOutside[ 0 ] ? 0 : 1;
				const int whichInside = !whichOutside;
				ASSERT( !isOutside[ whichInside ] );	// The other one had better not be Outside.
				
				Intersection intersection;
				findIntersectionCircleSegment( vec2::ZERO, lightRadius, points[ whichOutside ], points[ whichInside ], intersection );
				if( intersection.type == Intersection::Ordinary && intersection.points.size() == 1 )
				{
					// TODO not handling Tangent case.
					
					points[ whichOutside ] = intersection.points.front();
				}
			}
		}
		//
		// The points are now both within the light radius.
		
		// Cast rays through each point to a position on the light radius.
		//
		vec2 farPoints[ 2 ];
		Vector2i directions[ 2 ];
		
		for( int i = 0; i < 2; ++i )
		{
			farPoints[ i ] = getProjectedPoint( points[ i ], lightRadius );
			directions[ i ] = Vector2i( (int) farPoints[ i ].x, (int) farPoints[ i ].y );
			directions[ i ].snapToMajorAxis();
			directions[ i ] /= std::abs( directions[ i ].majorAxisValue() );	// Cheap normalize.
		}
		
		//
		// Now add the points in sequence in order to form triangles.
		// We also have to add corner points when needed in order to avoid short-cutting the edge of the light.
		//
		
		// Emit the first edge.
		//
		outTrianglePoints.push_back( points[ 0 ] );
		outTrianglePoints.push_back( farPoints[ 0 ] );
		
		//
		// Emit intermediaries.
		//
		
		// Walk around the rectangle so that intervening corners are covered.
		//
		while( directions[ 0 ] != directions[ 1 ] )
		{
			// Scale the intermediate to sit on next clockwise corner of the light rect.
			// The intermediate is *not* a normal.
			//
			Vector2i perpendicular( directions[ 0 ] );
			perpendicular.quickRot90();
			Vector2i cornerDir( directions[ 0 ] + perpendicular );
			
			const vec2 intermediate = vec2( cornerDir.x, cornerDir.y ) * (real) lightRadius;
			
			// Close the preceding triangle.
			//
			outTrianglePoints.push_back( intermediate );
			
			// Begin the next triangle.
			//
			outTrianglePoints.push_back( points[ 0 ] );
			outTrianglePoints.push_back( intermediate );
			
			// Progress to the next side of the light rect.
			//
			directions[ 0 ] = perpendicular;
		}
		
		// Close the last preceding triangle.
		//
		outTrianglePoints.push_back( farPoints[ 1 ] );
		
		// Emit the last triangle.
		//
		outTrianglePoints.push_back( points[ 0 ] );
		outTrianglePoints.push_back( farPoints[ 1 ] );
		outTrianglePoints.push_back( points[ 1 ] );
	}
}

