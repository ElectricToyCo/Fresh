/*
 *  RayCaster.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 10/20/11.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#pragma once

namespace fr
{

	// General-purpose utility for walking a ray through an n-dimensional grid.
	// Used for e.g. rendering in Wolfenstein 3D, picking in voxel terrain
	// systems (e.g. Minecraft).

	template< int nAxes, typename real_t, typename realVec_t, typename matrix_t, real_t (*realVec_lengthSquared)( const realVec_t& ) >
	class RayCaster
	{
	public:

		struct Intersection
		{
			realVec_t	hitPoint;
			int			hitFaceAxis = 0;
			real_t		hitFaceSign = 0;
		};
		
		// Assumes that all matrix cell edges sit on integer values.
		//
		static bool findRayIntersection( 
			const matrix_t& matrixOfBools,	// Must support bool operator()( const realVec_t& pos, const realVec_t& fromDir ) const, returning true for "solid" (intersecting) entries.
			const realVec_t& rayOrigin,
			const realVec_t& rayDirection,	// Must be normalized.
			Intersection& outIntersection,
			const real_t maxDistance = std::numeric_limits< real_t >::infinity(),
			const realVec_t tileSize = realVec_t( 1.0 ),
			const bool searchForSolids = true		// If true, finds the first location that moves from non-intersecting to intersecting; if false, seaches from intersecting to non-intersecting.
		)
		{
			const real_t CELL_INSET_SCALAR = real_t( 1.01 );
			
			// Initialize the "next intercept" values based on the ray's
			// position and direction.
			//
			real_t nextIntercept[ nAxes ];		// One per axis.

			for( int axis = 0; axis < nAxes; ++axis )
			{
				nextIntercept[ axis ] = std::floor( rayOrigin[ axis ] / tileSize[ axis ] ) * tileSize[ axis ];
				if( rayDirection[ axis ] > 0 )
				{
					nextIntercept[ axis ] += tileSize[ axis ];
				}
				else if( rayDirection[ axis ] == 0 )
				{
					// Never intercepts.
					nextIntercept[ axis ] = std::numeric_limits< real_t >::infinity();
				}
			}

			//
			// Scan through, stopping at each intersection (a face or edge between two cells),
			// until we hit a solid cell or else go farther than the max distance.
			//

			realVec_t testPoint( rayOrigin );
			int lastInterceptAxis = -1;

			const real_t maxDistanceSquared = maxDistance * maxDistance;

			while( realVec_lengthSquared( testPoint - rayOrigin ) <= maxDistanceSquared )
			{
				// Have we hit a solid cell?
				//
				if( matrixOfBools( testPoint, testPoint - rayOrigin ) == searchForSolids )
				{
					// Yes. Return the intersection point.
					//
					outIntersection.hitPoint = testPoint;
					outIntersection.hitFaceAxis = lastInterceptAxis;	// Might be -1, in which case we started in a solid cell.
					outIntersection.hitFaceSign = 0;

					if( lastInterceptAxis >= 0 )
					{
						outIntersection.hitFaceSign = sign( -rayDirection[ lastInterceptAxis ] );
					}

					return true;
				}

				// Move to the next intersection point.
				//
				real_t nearestIntersectionDistance = std::numeric_limits< real_t >::infinity();
				lastInterceptAxis = -1;
				real_t nearestIntersectionDelta = 0;
				for( int axis = 0; axis < nAxes; ++axis )
				{
					if( nextIntercept[ axis ] < std::numeric_limits< real_t >::infinity() )
					{
						real_t delta = nextIntercept[ axis ] - testPoint[ axis ] ;
						real_t distToIntercept = std::abs( delta / rayDirection[ axis ] );
						if( distToIntercept > 0 && distToIntercept < nearestIntersectionDistance )
						{
							nearestIntersectionDistance = distToIntercept;
							lastInterceptAxis = axis;
							nearestIntersectionDelta = delta;
						}
					}
				}

				if( lastInterceptAxis < 0 )
				{
					// We were probably given an erroneous ray direction of (0,0,0). Stop now.
					break;
				}
				assert( rayDirection[ lastInterceptAxis ] );

				testPoint[ lastInterceptAxis ] += nearestIntersectionDelta * CELL_INSET_SCALAR;

				// Update the other coordinates.
				for( int axis = 0; axis < nAxes; ++axis )
				{
					if( lastInterceptAxis == axis ) continue;

					const real_t slope = rayDirection[ axis ] / rayDirection[ lastInterceptAxis ];

					testPoint[ axis ] += slope * nearestIntersectionDelta;
				}

				nextIntercept[ lastInterceptAxis ] += tileSize[ lastInterceptAxis ] * sign( rayDirection[ lastInterceptAxis ] );
			}
			
			return false;
		}
				
	private:

		template< typename T >
		static void swap( T& a, T& b )
		{
			T temp( a );
			a = b;
			b = temp;
		}

		template< typename T >
		static T sign( T x )
		{
			return x < 0 ? T( -1 ) : x > 0 ? T( 1 ) : T( 0 );
		}
	};


}

