//
//  FreshVector.h
//  Fresh
//
//  Created by Jeff Wofford on 3/14/11.
//  Copyright 2011 Jeff Wofford. All rights reserved.
//

#pragma once


#include "FreshMath.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Rectangle.h"
#include "Matrix4.h"
#include <vector>

namespace fr
{
	
    typedef Vector2< real > vec2;
	typedef Vector2< int > vec2i;
	typedef Vector2< uint > vec2ui;
    typedef Vector3< real > vec3;
    typedef Vector4< real > vec4;
    typedef Matrix4< real > mat4;
	
    typedef Rectangle< real > rect;
	
	template< typename Real >
	Vector2< Real > randInRange( const Vector2< Real >& a, const Vector2< Real >& b )
	{
		Vector2< Real > result;
		
		for( size_t i = 0; i < Vector2< Real >::nComponents; ++i )
		{
			result[ i ] = randInRange( a[ i ], b[ i ] );
		}
		return result;
	}
	
	template< typename Real >
	Vector3< Real > randInRange( const Vector3< Real >& a, const Vector3< Real >& b )
	{
		Vector3< Real > result;
		
		for( size_t i = 0; i < Vector3< Real >::nComponents; ++i )
		{
			result[ i ] = randInRange( a[ i ], b[ i ] );
		}
		return result;
	}
	
	template< typename Real >
	Vector4< Real > randInRange( const Vector4< Real >& a, const Vector4< Real >& b )
	{
		Vector4< Real > result;
		
		for( size_t i = 0; i < Vector4< Real >::nComponents; ++i )
		{
			result[ i ] = randInRange( a[ i ], b[ i ] );
		}
		return result;
	}
	
	class Direction
	{
	public:
		
		enum class Cardinal
		{
			East,				// 0 => binary 00
			South,				// 1 => binary 01
			West,				// 2 => binary 10
			North				// 3 => binary 11
		};
		
		static const int NUM_DIRECTIONS = 4;
		
		Direction( Cardinal cardinal = Cardinal::East )
		:	m_cardinal( static_cast< int >( cardinal ))
		{
			ASSERT( valid() );
		}
		
		Direction( const Direction& other ) : m_cardinal( other.m_cardinal ) {}
		
		template< typename Real >
		explicit Direction( const Vector2< Real >& vec )
		{
			if( vec.isZero() )
			{
				invalidate();
			}
			else
			{
				const int axis = vec.majorAxis();
				const Real axisValue = vec.majorAxisValue();
				const int sign = axisValue < 0 ? 2 : 0;
				
				m_cardinal = axis | sign;
			}
		}
		
		Direction& operator=( Cardinal cardinal )
		{
			m_cardinal = static_cast< int >( cardinal );
			ASSERT( valid() );
			return *this;
		}
		
		Direction& operator=( const Direction& other )
		{
			m_cardinal = other.m_cardinal;
			return *this;
		}
		
		int axis() const									{ ASSERT( valid() ); return m_cardinal & 1; }
		int sign() const									{ ASSERT( valid() ); return ( m_cardinal & 2 ) ? -1 : 1; }
		int index() const									{ ASSERT( valid() ); return m_cardinal; }
		
		Direction& operator++()
		{
			ASSERT( valid() );
			++m_cardinal;
			if( m_cardinal >= 4 )
			{
				// Invalidate.
				m_cardinal = -1;
			}
			return *this;
		}
		
		Direction& operator--()
		{
			ASSERT( valid() );
			--m_cardinal;		// Automatically invalidates.
			return *this;
		}
		
		Direction& operator-()		// Unary negation flips a valid direction.
		{
			ASSERT( valid() );
			rotate( 2 );
			return *this;
		}
		
		template< typename Real >
		operator Vector2< Real >() const
		{
			ASSERT( valid() );
			Vector2< Real > vec;
			vec[ axis() ] = sign();
			return vec;
		}
		
		bool operator==( Direction other ) const
		{
			return m_cardinal == other.m_cardinal;
		}
		
		bool operator!=( Direction other ) const
		{
			return !operator==( other );
		}		
		
		operator Cardinal() const
		{
			ASSERT( valid() );
			return Cardinal( m_cardinal );
		}
		
		angle getAngle() const
		{
			ASSERT( valid() );
			return fr::angle( 90 * m_cardinal );
		}
		
		void rotate( int quarterTurns )
		{
			ASSERT( valid() );
			m_cardinal = ( m_cardinal + quarterTurns ) & 0x3;		// Wrap in range [0,4[.
		}
		
		bool valid() const
		{
			return 0 <= m_cardinal && m_cardinal < 4;
		}
		
		void invalidate()
		{
			m_cardinal = -1;
			ASSERT( !valid() );
		}
		
		uint bit() const
		{
			if( valid() )
			{
				return 1 << m_cardinal;
			}
			else
			{
				return 0;
			}
		}
		
		std::string name() const
		{
			ASSERT( valid() );			
			static const std::vector< std::string > names = { "East", "South", "West", "North" };
			return names[ index() ];
		}
		
	private:
		int m_cardinal;
		
		friend std::ostream& operator<<( std::ostream& out, Direction d );
		friend std::istream& operator>>( std::istream& in,  Direction& a );
		
	};

	inline std::ostream& operator<<( std::ostream& out, Direction d )
	{
		out << d.m_cardinal;
		return out;
	}
	
	inline std::istream& operator>>( std::istream& in,  Direction& d )
	{
		in >> d.m_cardinal;
		return in;
	}

	
	struct Intersection
	{
		enum Type
		{
			Outside,
			Inside,
			Tangent,
			Ordinary,
			Unknown
		} type;
		
		std::vector< vec2 > points;
		
		Intersection( Type type_ = Unknown ) : type( type_ )
		{
			points.reserve( 1 );
		}
	};
	
	template< typename vec_t >
	vec_t abs( const vec_t& a )
	{
		vec_t result( a );
		for( size_t i = 0; i < vec_t::nComponents; ++i )
		{
			result[ i ] = std::abs( result[ i ] );
		}
		return result;
	}

	template< typename vec_t >
	vec_t max( const vec_t& a, const vec_t& b )
	{
		vec_t result;
		for( size_t i = 0; i < vec_t::nComponents; ++i )
		{
			result[ i ] = fr::max( a[ i ], b[ i ] );
		}
		return result;
	}

	template< typename vec_t >
	vec_t min( const vec_t& a, const vec_t& b )
	{
		vec_t result;
		for( size_t i = 0; i < vec_t::nComponents; ++i )
		{
			result[ i ] = fr::max( a[ i ], b[ i ] );
		}
		return result;
	}
	
	template< typename vec_t >
	vec_t clampVector( const vec_t& a, const vec_t& minimum, const vec_t& maximum )
	{
		return fr::min( fr::max( a, minimum ), maximum );
	}

	template< typename vec_t >
	real distanceSquared( const vec_t& a, const vec_t& b )
	{
		return (a - b).lengthSquared();
	}
	
	template< typename vec_t >
	real distance( const vec_t& a, const vec_t& b )
	{
		return std::sqrt( distanceSquared( a, b ));
	}
	
	template< typename real_out, typename real_in >
	Vector2< real_out > vector_cast( const Vector2< real_in >& in )
	{
		return Vector2< real_out >( static_cast< real_out >( in.x ), static_cast< real_out >( in.y ) );
	}
	
	template< typename real_out, typename real_in >
	Vector3< real_out > vector_cast( const Vector3< real_in >& in )
	{
		return Vector3< real_out >( static_cast< real_out >( in.x ), static_cast< real_out >( in.y ), static_cast< real_out >( in.z ) );
	}
	
	template< typename real_out, typename real_in >
	Vector4< real_out > vector_cast( const Vector4< real_in >& in )
	{
		return Vector4< real_out >( static_cast< real_out >( in.x ), static_cast< real_out >( in.y ), static_cast< real_out >( in.z ), static_cast< real_out >( in.w ) );
	}
	
	inline void findIntersectionCircleSegment( const vec2& circleCenter, real radius, const vec2& linePointA, const vec2& linePointB, Intersection& outResult )
	{
		vec2 lineDelta( linePointB - linePointA );
		
		real a  = lineDelta.dot( lineDelta );
		
		real b  = 2.0f *
		( lineDelta.x * (linePointA.x - circleCenter.x ) +
		 lineDelta.y * (linePointA.y - circleCenter.y ));
		
		real c = circleCenter.dot( circleCenter ) + linePointA.dot( linePointA ) -
		2.0f * circleCenter.dot( linePointA ) - radius * radius;
		
		real deter = b * b - 4.0f * a * c;
		
		if( deter < 0 )
		{
			outResult.type = Intersection::Outside;
		}
		else if( deter == 0 )
		{
			outResult.type = Intersection::Tangent;
			// TODO: should calculate this point
		}
		else
		{
			real e  = std::sqrt( deter );
			real u1 = ( -b + e ) / ( 2.0f * a );
			real u2 = ( -b - e ) / ( 2.0f * a );
			
			if ( (u1 < 0 || u1 > 1) && (u2 < 0 || u2 > 1) )
			{
				if ( (u1 < 0 && u2 < 0) || (u1 > 1 && u2 > 1) )
				{
					outResult.type = Intersection::Outside;
				}
				else
				{
					outResult.type = Intersection::Inside;
				}
			}
			else
			{
				outResult.type = Intersection::Ordinary;
				
				if( 0 <= u1 && u1 <= 1.0f )
				{
					outResult.points.push_back( lerp( linePointA, linePointB, u1 ) );
				}
				
				if( 0 <= u2 && u2 <= 1.0f )
				{
					outResult.points.push_back( lerp( linePointA, linePointB, u2 ) );
				}
			}
		}
	}
	
	inline bool doesRectIntersectCircle( const rect& rectangle, const vec2& circleCenter, real radius )
	{
		if( rectangle.doesEnclose( circleCenter ))
		{
			return true;
		}
		else
		{
			// Check each edge against the circle.
			//
			for( int s = 0; s < 4; ++s )
			{
				const int e = ( s + 1 ) & 3;		// & 3 is equivalent to % 4
				
				Intersection intersection;
				findIntersectionCircleSegment( circleCenter, radius, rectangle.corner( s ), rectangle.corner( e ), intersection );
				if( intersection.type == Intersection::Inside || intersection.type == Intersection::Tangent || intersection.type == Intersection::Ordinary )
				{
					return true;
				}
			}
			return false;
		}
	}
	
	inline bool isPointBehindSegment( const vec2& a, const vec2& b, const vec2& p )
	{
		return signNoZero( vec2( b.y - a.y, a.x - b.x ).dot( p - b )) < 0;
	}
	
	inline bool isPointInTriangle( const vec2& tri0, const vec2& tri1, const vec2& tri2, const vec2& p )
	{
		// Explanation at http://www.mochima.com/articles/cuj_geometry_article/cuj_geometry_article.html. See "Testing for Point Inclusion in a Triangle".
		
		// Limb 0 to 1.
		{
			if( isPointBehindSegment( tri1, tri0, p ))
			{
				return false;
			}
		}
		// Limb 1 to 2.
		{
			if( isPointBehindSegment( tri2, tri1, p ))
			{
				return false;
			}
		}
		// Limb 2 to 0.
		{
			if( isPointBehindSegment( tri0, tri2, p ))
			{
				return false;
			}
		}
		
		return true;
	}
    
	// Returns the amount that you would have to multiply dimensionsToFit by in order to make a rectangle
	// of dimensionsToFit dimensions fit into a rectangle of dimensionsToFitInto without changing the
	// aspect ratio of dimensionsToFit. This finds a perfect fit: that is, after the multiplication,
	// one of the coordinates in dimensionsToFit will equal one of the coordinates in dimensionsToFitInto.
	//
	// NOTE: If Real is an integer type this function is unlikely to work well because the return value will
	// lack sufficient precision.
	//
	template< typename Real >
	Real getFitRatio( const Vector2< Real >& dimensionsToFit, const Vector2< Real >& dimensionsToFitInto )
	{
		REQUIRES( dimensionsToFit.x > 0 && dimensionsToFit.y > 0 );
		REQUIRES( dimensionsToFitInto.x > 0 && dimensionsToFitInto.y > 0 );
		
		const Vector2< Real > dimensionRatios( dimensionsToFitInto / dimensionsToFit );
		
		Real result = dimensionRatios.minorAxisValue();	// Returns the smaller of x or y.
		PROMISES( result > 0 );

        return result;
	}
	
	// Returns the amount that you would have to multiply dimensionsToFill by in order to make a rectangle
	// of dimensionsToFill dimensions fill a rectangle of dimensionsToFitInto without changing the
	// aspect ratio of dimensionsToFill. The dimensionsToFill rectangle will then "fill" the
	// dimensionsToFitInto rectangle in the sense that neither of its coordinates will be smaller than
	// the corresponding coordinate in dimensionsToFitInto.
	//
	// NOTE: If Real is an integer type this function is unlikely to work well because the return value will
	// lack sufficient precision.
	//
	template< typename Real >
	Real getFillRatio( const Vector2< Real >& dimensionsToFill, const Vector2< Real >& dimensionsToFitInto )
	{
		REQUIRES( dimensionsToFill.x > 0 && dimensionsToFill.y > 0 );
		REQUIRES( dimensionsToFitInto.x > 0 && dimensionsToFitInto.y > 0 );
		
		const Vector2< Real > dimensionRatios( dimensionsToFitInto / dimensionsToFill );
        
		Real result = dimensionRatios.majorAxisValue();	// Returns the larger of x or y.
		PROMISES( result > 0 );
		return result;
	}
	
	template< typename Real >
	inline Vector2< Real > getPointAlongRay( const Vector2< Real >& origin, const Vector2< Real >& direction, Real distance )
	{
		return origin + direction * distance;
	}
	
    template< typename VectorT, typename Real >
	VectorT calculateSpringForceVector( const VectorT& posDelta, const VectorT& velDelta, Real stiffness, Real damping, Real restDistance = 0 )
	{
		return calculateSpringForce( posDelta, posDelta.length(), velDelta.dot( posDelta ), stiffness, damping, restDistance );
	}
    
	template< typename Real >
	Real calculateSpringForceReal( Real posDelta, Real velDelta, Real stiffness, Real damping, Real restDistance = 0 )
	{
		return calculateSpringForce( posDelta, std::abs( posDelta ), velDelta * posDelta, stiffness, damping, restDistance );
	}
	
    template< typename Real >
    void rotateDegrees( Vector2< Real >& vec, Real angleDegrees )
    {
        vec.rotate( degreesToRadians( angleDegrees ));
    }
	
    template< typename Real >
	Vector2< Real > makeRandomVector2( Real maxLength )
	{
		Vector2< Real > result;
		result.setToAngleNormal( randInRange( 0.0f, 360.0f ));
		result *= randInRange( 0.0f, maxLength );
		return result;
	}

	template< typename Real >
	Vector2< Real > makeRandomVector2( const Range< Real >& lengthRange )
	{
		Vector2< Real > result;
		result.setToAngleNormal( randInRange( 0.0f, 360.0f ));
		result *= randInRange( lengthRange );
		return result;
	}
	
	template< typename Real >
	bool colinear( const Vector2< Real >& a, const Vector2< Real >& b, const Vector2< Real >& c, Real epsilon = Real( -1 ))
	{
		// See http://stackoverflow.com/a/5009666/358475
		if( epsilon == Real( -1 ))
		{
			epsilon = std::abs( std::max( { a.x, a.y, b.x, b.y, c.x, c.y } )) * Real( 0.000001 );
		}
		return std::abs(( b.x - a.x ) * ( c.y - a.y ) - ( c.x - a.x ) * ( b.y - a.y )) <= epsilon;
	}
	
	template< typename Real >
	Vector2< Real > findClosestPointOnCapsule( const Vector2< Real >& point, const Vector2< Real >& capsuleDimensionsExcludingCaps, const Vector2< Real >& capsuleTranslation, Real capsuleRotationRadians )
	{
		// Transform the point into capsule space.
		//
		Vector2< Real > transformedPoint( point - capsuleTranslation );
		transformedPoint.rotate( -capsuleRotationRadians );
		
		// Find the nearest point on the line passing through the center of the capsule.
		//
		
		Vector2< Real > result;
		
		Vector2< Real > halfCapsuleDimensions( capsuleDimensionsExcludingCaps * 0.5f );
		const Real capRadius = halfCapsuleDimensions.x;
		
		if( transformedPoint.y >= -halfCapsuleDimensions.y && transformedPoint.y <= halfCapsuleDimensions.y )
		{
			// Nearest point is on or within cylinder.
			//
			result.set( sign( transformedPoint.x ) * std::min( std::abs( transformedPoint.x ), capRadius ),
					   transformedPoint.y );
		}
		else
		{
			//
			// Nearest point is above or below the cylinder (rectangle).
			//
			
			result = transformedPoint;
			
			// Translate the point into the space of the nearest endpoint.
			//
			Real rectangleBase( halfCapsuleDimensions.y );
			
			if( transformedPoint.y > halfCapsuleDimensions.y )
			{
				rectangleBase = -rectangleBase;
			}
			
			result.y += rectangleBase;
			
			// If outside of cap, normalize to sit on cap.
			//
			const Real capRadiusSquared = capRadius * capRadius;
			if( result.lengthSquared() > capRadiusSquared )
			{
				result.normalize();
				result *= capRadius;
			}
			
			// Move the point back out of endpoint space.
			//
			result.y -= rectangleBase;
		}
		
		// Transform the result back into world space.
		//
		result.rotate( capsuleRotationRadians );
		result += capsuleTranslation;
		
		return result;
	}
	
	template< typename Real, typename IteratorT >
	Real findMinimumBoundingCircleRadiusWithCenter( const Vector2< Real >& circleCenter, IteratorT begin, IteratorT end )
	{
		auto iter = std::max_element( begin, end, [&]( const Vector2< Real >& a, const Vector2< Real >& b )
									 {
										 return distanceSquared( a, circleCenter ) < distanceSquared( b, circleCenter );
									 } );
		ASSERT( iter != end );
		return distance( *iter, circleCenter );
	}
	
	/**
	 * easy bounding circle (exact) by Jon Rokne
	 * from "Graphics Gems II"
	 */
	template< typename Real, typename IteratorT >
	void findMinimumBoundingCircle( IteratorT begin, IteratorT end, Vector2< Real >& outCircleCenter, Real& outCircleRadius )
	{
		Vector2< Real > P, Q, R;
		IteratorT pi, qi, ri;
		
		// Determine a point P with the smallest y value
		//
		static_assert( std::numeric_limits< Real >::has_infinity, "This function cannot be used for integers and other types that lack infinity." );
		real ymin = std::numeric_limits< Real >::infinity();
		
		for( IteratorT iter = begin; iter != end; ++iter )
		{
			const Vector2< Real >& v = *iter;
			if (v.y < ymin)
			{
				ymin = v.y;
				pi = iter;
				P = v;
			}
		}
		
		real dx0, dy0, dx1, dy1;
		
		// find a point Q such that the angle of the line segment
		// PQ with the x axis is minimal
		real dot_max = -std::numeric_limits< Real >::infinity();
		real dot;
		for( IteratorT iter = begin; iter != end; ++iter )
		{
			if (iter == pi) continue;
			
			const Vector2< Real >& v = *iter;
			dx0 = v.x - P.x;
			dy0 = v.y - P.y;
			
			dot = (dx0 < 0 ? -dx0 : dx0) / std::sqrt(dx0 * dx0 + dy0 * dy0);
			if (dot > dot_max)
			{
				dot_max = dot;
				Q  = v;
				qi = iter;
			}
		}
		
		for( IteratorT iter = begin; iter != end; ++iter )
		{
			dot_max = -std::numeric_limits< Real >::infinity();
			
			//find R such that the absolute value
			//of the angle PRQ is minimal
			for( IteratorT iterJ = begin; iterJ != end; ++iterJ )
			{
				if (iterJ == pi) continue;
				if (iterJ == qi) continue;
				
				const Vector2< Real >& v = *iterJ; 
				
				dx0 = P.x - v.x; dy0 = P.y - v.y;
				dx1 = Q.x - v.x; dy1 = Q.y - v.y;
				
				dot = (dx0 * dx1 + dy0 * dy1) / (std::sqrt(dx0 * dx0 + dy0 * dy0) * std::sqrt(dx1 * dx1 + dy1 * dy1));
				if (dot > dot_max)
				{				
					dot_max = dot;
					R  = v;
					ri = iterJ;
				}
			}
			
			//check for case 1 (angle PRQ is obtuse), the circle is determined
			//by two points, P and Q. radius = |(P-Q)/2|, center = (P+Q)/2
			if (dot_max < 0)
			{
				dx0 = P.x - Q.x;
				dy0 = P.y - Q.y;
				
				outCircleCenter.x = (P.x + Q.x) / 2.0f;
				outCircleCenter.y = (P.y + Q.y) / 2.0f;
				outCircleRadius = std::sqrt(((dx0 * dx0) / 4.0f) + ((dy0 * dy0) / 4.0f));
				return;
			}
			
			//check if angle RPQ is acute
			dx0 = R.x - P.x;
			dy0 = R.y - P.y;
			
			dx1 = Q.x - P.x;
			dy1 = Q.y - P.y;
			
			dot = (dx0 * dx1 + dy0 * dy1) / (std::sqrt(dx0 * dx0 + dy0 * dy0) * std::sqrt(dx1 * dx1 + dy1 * dy1));
			
			// if angle RPQ is 
			if (dot < 0)
			{
				P = R;
				P.x = R.x;
				P.y = R.y;
				continue;
			}
			
			// angle PQR is acute ?
			dx0 = P.x - Q.x;
			dy0 = P.y - Q.y;
			
			dx1 = R.x - Q.x;
			dy1 = R.y - Q.y;
			
			dot = (dx0 * dx1 + dy0 * dy1) / (std::sqrt(dx0 * dx0 + dy0 * dy0) * std::sqrt(dx1 * dx1 + dy1 * dy1));
			
			if (dot < 0)
			{
				Q = R;
				Q.x = R.x;
				Q.y = R.y;
				continue;
			}
			
			//all angles in PQR are acute; quit
			break;
		}
		
		real mPQx = (P.x + Q.x) / 2.0f;
		real mPQy = (P.y + Q.y) / 2.0f;
		real mQRx = (Q.x + R.x) / 2.0f;
		real mQRy = (Q.y + R.y) / 2.0f;
		
		real numer = -(-mPQy * R.y + mPQy * Q.y + mQRy * R.y - mQRy * Q.y - mPQx * R.x + mPQx * Q.x + mQRx * R.x - mQRx *Q.x);
		real denom =  (-Q.x * R.y + P.x * R.y - P.x * Q.y + Q.y * R.x - P.y * R.x + P.y * Q.x);
		
		real t = numer / denom;
		
		outCircleCenter.x  = -t * (Q.y - P.y) + mPQx;
		outCircleCenter.y  =  t * (Q.x - P.x) + mPQy;
		
		dx0 = outCircleCenter.x - P.x;
		dy0 = outCircleCenter.y - P.y;
		
		outCircleRadius = std::sqrt(dx0 * dx0 + dy0 * dy0);
	}

	
	template< typename IteratorT, typename fnMagnitude >
	real calcTotalLength( IteratorT begin, IteratorT end, const fnMagnitude& length )
	{
		real totalLength = 0;
		
		if( begin == end )
		{
			return totalLength;
		}
		
		IteratorT next = begin;
		++next;
		
		while( next != end )
		{
			totalLength += length( *next - *begin );
			
			++begin;
			++next;
		}
		
		return totalLength;
	}
	
	template< typename VectorT >
	inline real magnitude( const VectorT& v )
	{
		return v.length();
	}

	template< typename VectorT >
	inline real magnitudeSquared( const VectorT& v )
	{
		return v.lengthSquared();
	}
    
    inline void findCollisionNormalSimple( const vec2& overlaps, const vec2& delta, vec2& outHitNormal, int& outNormalAxis )
    {
        // Push this FreshActor away from the other rectangle along the thinner axis.
        //
        outNormalAxis = overlaps.minorAxis();
        
        const float adjustmentSign = sign( delta[ outNormalAxis ] );
        outHitNormal.set( 0, 0 );
        outHitNormal[ outNormalAxis ] = adjustmentSign;
    }
    
    inline void findCollisionNormalWithVelocity( const vec2& rectCenterA, const vec2& rectSizeA, const vec2& rectVelA, const vec2& rectCenterB, const vec2& rectSizeB, const vec2& rectVelB, const vec2& overlaps, const vec2& delta, vec2& outHitNormal, int& outNormalAxis )
    {
        const float EPSILON = 0.0001f;        // TODO arbitrary epsilon
        
        outNormalAxis = -1;
        
        const vec2 totalDimensions( rectSizeA * 0.5f + rectSizeB * 0.5f );
        
        // Consider the objects at their last positions.
        // How was that overlapping this in the last position?
        //
        const vec2 lastPosA = rectCenterA - rectVelA;
        const vec2 lastPosB = rectCenterB - rectVelB;

        const vec2 deltaLast = lastPosA - lastPosB;
        const vec2 axialDistancesLast( std::abs( deltaLast.x ), std::abs( deltaLast.y ));
        
        // Positive values indicate overlap.
        const vec2 overlapsLast( totalDimensions - axialDistancesLast );
        
        if( overlapsLast.x > 0 )
        {
            // Overlapping in X.
            
            if( overlapsLast.y > 0 )
            {
                // In the last position these objects were not clear of each other,
                // so we can't use this method.
                //
                // Use the "simple" method of colliding based on thinnest overlap distance.
                //
                return findCollisionNormalSimple( overlaps, delta, outHitNormal, outNormalAxis );
            }
            else
            {
                // Was overlapping in x, not y. Therefore the hitNormal must be vertical.
                //
                outNormalAxis = 1;
                outHitNormal.set( 0, sign( deltaLast.y ));
            }
        }
        else if( overlapsLast.y > 0 )
        {
            // Was overlapping in y, not x. Therefore the hitNormal must be horizontal.
            //
            outNormalAxis = 0;
            outHitNormal.set( sign( deltaLast.x ), 0 );
        }
        else
        {
            // No overlap in the last position (corners were closest).
            // Decide which edge the objects collided on by comparing the velocity to the
            // delta between their closest corners.
            //
            
            vec2 deltaLastSign = deltaLast;
            deltaLastSign.x = sign( deltaLastSign.x );
            deltaLastSign.y = sign( deltaLastSign.y );
            
            const vec2 thisClosestCorner = lastPosA + deltaLastSign * rectSizeA * 0.5f;
            const vec2 thatClosestCorner = lastPosB + deltaLastSign.getInverse() * rectSizeB * 0.5f;
            
            const vec2 deltaCorners = thisClosestCorner - thatClosestCorner;
            
            if( deltaCorners.isZero( EPSILON ))    // TODO arbitrary epsilon
            {
                // Corners were basically touching--too close to call.
                // Use the simple method.
                //
                return findCollisionNormalSimple( overlaps, delta, outHitNormal, outNormalAxis );
            }
            
            vec2 deltaCornersPerpendicular = deltaCorners;
            deltaCornersPerpendicular.quickRot90();
            
            const vec2 thatVelRel = rectVelB - rectVelA;
            if( thatVelRel.isZero( EPSILON ))    // TODO arbitrary epsilon
            {
                // No velocity to speak of. Use primitive resolution method. (This should be unusual.)
                //
                return findCollisionNormalSimple( overlaps, delta, outHitNormal, outNormalAxis );
            }
            
            // Determine how the corners moved relative to their axis of intersection.
            //
            const float dotProduct = deltaCorners.dot( thatVelRel );
            
            outHitNormal = deltaCorners;
            
            if( dotProduct > 0 )
            {
                outNormalAxis = outHitNormal.majorAxis();
                outHitNormal.snapToMajorAxis();
            }
            else
            {
                outNormalAxis = outHitNormal.minorAxis();
                outHitNormal.snapToMinorAxis();
            }
            
            outHitNormal.normalize();
            
            if( outHitNormal.isZero( EPSILON ))
            {
                // Very low velocity along the hit normal. Use simple method.
                //
                return findCollisionNormalSimple( overlaps, delta, outHitNormal, outNormalAxis );
            }
        }
    }
    
    inline bool findCollisionNormal( const vec2& rectCenterA, const vec2& rectSizeA, const vec2& rectVelA, const vec2& rectCenterB, const vec2& rectSizeB, const vec2& rectVelB, vec2& outHitNormal, int& outNormalAxis, float& outAdjustmentDistance )
    {
        const vec2 delta( rectCenterA - rectCenterB );
        const vec2 axialDistances( std::abs( delta.x ), std::abs( delta.y ));
        
        const vec2 overlaps( rectSizeB * 0.5f + rectSizeA * 0.5f - axialDistances );
        
        if( overlaps.x <= 0 || overlaps.y <= 0 )
        {
            // Doesn't overlap
            return false;
        }
        
        // Calculate the relative velocity of other.
        //
        findCollisionNormalWithVelocity( rectCenterA, rectSizeA, rectVelA, rectCenterB, rectSizeB, rectVelB, overlaps, delta, outHitNormal, outNormalAxis );
        
        // Find the adjustment distance on the hitNormal
        //
        assert( outNormalAxis >= 0 );
        outAdjustmentDistance = overlaps[ outNormalAxis ];
        
        return true;
    }
}
