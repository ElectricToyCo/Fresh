#ifndef FRESH_MATH_H_INCLUDED_
#define FRESH_MATH_H_INCLUDED_

#include <cstdlib>		// For RAND_MAX and std::abs(int)
#include <cmath>		// For sin, cos
#include <cfloat>
#include <limits>
#include <iostream>
#include "FreshEssentials.h"
#include "FreshRange.h"
#include "FreshRandom.h"
#include "FreshDebug.h"

namespace fr
{
	using std::abs;					// To avoid accidental calls to ::abs( int ) when std::abs( float ) is intended.
	using std::min;
	using std::max;

	typedef unsigned int uint;		// Helps to resemble ActionScript 3.0 code.
	typedef unsigned short ushort;
	typedef unsigned char byte;

	typedef double TimeType;
	
#ifdef FRESH_REAL_USES_DOUBLES
		
	typedef double real;			// Helps to resemble ActionScript 3.0 code. Some games might choose to make this a float rather than double.
		
#else

	typedef float real;			// Helps to resemble ActionScript 3.0 code. Some games might choose to make this a float rather than double.

#endif

	const real PI = static_cast< real >( 3.1415926538 );
	const real TWO_PI = PI * static_cast< real >( 2.0 );
	const real HALF_PI = PI * static_cast< real >( 0.5 );
	const real Infinity = std::numeric_limits< real >::infinity();

	//////////////////////////////////////////////////////
	
	// Assume that you're making a rectangular grid consisting of nColumns columns
	// that will hold nElements cells. How many rows do you need?
	// This function answers that question.
	//
	inline int getNumRowsForColumns( const int nElements, const int nColumns )
	{
		assert( nColumns > 0 );
		return ( nElements + nColumns - 1 ) / nColumns;
	}
	
	// Given *count* elements (e.g. in a row of elements),
	// returns the amount you should offset the whole row
	// in order to center it, taking odd/even effects into
	// account. Should be an obvious calculation, but I
	// find myself constantly having to re-invent it.
	//
	template< typename count_t >
	inline real centeringOffset( count_t count )
	{
		return fr::max( count_t( 0 ), count - 1 ) * real( -0.5 );
	}
		
	inline bool isPowerOfTwo( int x )
	{
		if( x < 1 ) return false;
		return ( x & ( x - 1 )) == 0;
	}
	
	inline bool isEven( int x )
	{
		return ( x & 1 ) == 0;
	}

	inline bool isNaN( real x )
	{
		return std::isnan( x );
	}
	
	template< typename Real >
	inline bool isInfinite( Real x )
	{
#if ANDROID	// std::isinf() is broken on Android: doesn't detect std::numeric_limits<float>::infinity(), for example.
		return __isinff( x );
#else
		return std::isinf( x );
#endif
	}
	
	inline unsigned int getNearestPowerOfTwoGreaterThanOrEqualTo( unsigned int num )
	{
		// From http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
		num--;
		num |= num >> 0x0001;
		num |= num >> 0x0002;
		num |= num >> 0x0004;
		num |= num >> 0x0008;
		num |= num >> 0x0010;
		return num + 1;
	}
				
	template< typename T >
	inline T degreesToRadians( T degrees )
	{
		return degrees / 180.0f * PI;
	}

	template< typename T >
	inline T radiansToDegrees( T radians )
	{
		return radians / PI * 180.0f;
	}
	
	template< typename T >
	constexpr inline T fast_fmod( T x, T y )
	{
		// REQUIRES( y > 0 );
		return x - int( x / y ) * y;
	}

	template< typename T >
	inline void sinCosFromRadians( T radians, T& outSin, T& outCos )
	{
		outSin = std::sin( radians );
		outCos = std::cos( radians );
	}

	template< typename T >
	inline void sinCosFromDegrees( T degrees, T& outSin, T& outCos )
	{
		sinCosFromRadians( degreesToRadians( degrees ), outSin, outCos );
	}
	
	template< typename T >
	inline T roundToNearest( T a, T denomination )
	{
		return std::floor(( a + T( 0.5 ) * denomination ) / denomination ) * denomination;
	}
	
	template< typename T >
	inline T round( T a )
	{
		return roundToNearest( a, T( 1 ));
	}
	
	template< typename T >
	inline T sign( T a )
	{
		static_assert( std::numeric_limits<T>::is_signed, "Can't call sign() with an unsigned type." );
		return a < 0 ? -1 : ( a > 0 ? 1 : 0 );
	}

	template< typename T >
	inline T signNoZero( T a )
	{
		static_assert( std::numeric_limits<T>::is_signed, "Can't call signNoZero() with an unsigned type." );
		return a < 0 ? (T) -1 : (T) 1;
	}
		
	template< typename T >
	inline T clamp( T a, T minimum, T maximum )
	{
		return fr::min( fr::max( a, minimum ), maximum );
	}

	template< typename T >
	inline T reflectiveClamp( T a, T minimum, T maximum )
	{
		while( a < minimum || a > maximum )
		{
			if( a < minimum )
			{
				a = minimum + ( minimum - a );
			}
			if( a > maximum )
			{
				a = maximum - ( a - maximum );
			}
		}
		
		return a;
	}
	
	inline float invSqrt( float x )
	{
		// http://en.wikipedia.org/wiki/Fast_inverse_square_root
		union {
			float f;
			int i;
		} tmp;
		tmp.f = x;
		tmp.i = 0x5f3759df - (tmp.i >> 1);
		float y = tmp.f;
		return y * (1.5f - 0.5f * x * y * y);
	}	

	template< typename T >
	inline bool isEqual( T a, T b, T epsilon = std::numeric_limits< T >::epsilon() )
	{
		return std::abs( a - b ) < epsilon;
	}
	
	template< typename T >
	inline T wrap( T a, T minInclusive, T maxExclusive )					// See http://stackoverflow.com/questions/707370/clean-efficient-algorithm-for-wrapping-integers-in-c	
	{
		ASSERT( maxExclusive > minInclusive );
		const T range = maxExclusive - minInclusive;
		if( a < minInclusive )
		{
			a += range * (( minInclusive - a ) / range + T( 1 ));
		}		
		return minInclusive + ( a - minInclusive ) % range;
	}
	
	// See http://stackoverflow.com/questions/4633177/c-how-to-wrap-a-float-to-the-interval-pi-pi
	inline float modulo( float x, float y )
	{
		return x - y * std::floor( x / y );
	}
	
	inline double modulo( double x, double y )
	{
		return x - y * std::floor( x / y );
	}
	
	template<>
	inline float wrap( float a, float min, float max )
	{
		ASSERT( max > min );
		const float range = max - min;
		return min + modulo( a - min, range );
	}
	
	template<>
	inline double wrap( double a, double min, double max )
	{
		ASSERT( max > min );
		const double range = max - min;
		return min + modulo( a - min, range );
	}
		
	template< typename T, typename TAlpha >
	inline T lerp( T a, T b, TAlpha alpha )
	{
		return a + static_cast< T >(( b - a ) * alpha );
	}

	// lerp: Unsigned specializations.
	//
	template< typename TAlpha >
	inline uint lerp( uint a, uint b, TAlpha alpha )
	{
		return static_cast< uint >( a + static_cast< int >(( static_cast< int >( b ) - static_cast< int >( a )) * alpha ));
	}
	
	template< typename TAlpha >
	inline unsigned char lerp( unsigned char a, unsigned char b, TAlpha alpha )
	{
		return static_cast< unsigned char >( a + static_cast< int >(( static_cast< int >( b ) - static_cast< int >( a )) * alpha ));
	}
	
	template< typename T, typename TAlpha >
	inline T lerp( const Range< T >& range, TAlpha alpha )
	{
		return lerp( range.min, range.max, alpha );
	}
	
	template< typename T, typename TAlpha >
	inline T sinLerp( T a, T b, TAlpha alpha )
	{
		return a + (b - a) * (( TAlpha( 1 ) + std::sin(( alpha - TAlpha( 0.5 )) * TAlpha( PI ))) * TAlpha( 0.5 ));
	}
		
	template< typename T >
	inline T proportion( T x, T min, T max )
	{
		return ( x - min ) / ( max - min );
	}

	template< typename T >
	inline T proportion( T x, const Range< T >& range )
	{
		return proportion( x, range.min, range.max );
	}

	template< typename T >
	inline T average( T a, T b )
	{
		return lerp( a, b, 0.5f );
	}

	template< typename Real >
	inline Real scaleSinToNormal( Real sinValue )
	{
		return ( sinValue + (Real)1.0 ) / (Real)2.0;
	}

	template< typename T >
	bool signedCompare( T x, T y, T sign )
	{
		if( sign > 0 )
		{
			return x > y;
		}
		else if( sign < 0 )
		{
			return x < y;
		}
		else
		{
			return x == y;
		}
	}
	
	template< typename T >
	bool signedCompareOrEqual( T x, T y, T sign )
	{
		if( sign > 0 )
		{
			return x >= y;
		}
		else if( sign < 0 )
		{
			return x <= y;
		}
		else
		{
			return x == y;
		}
	}
	
	template< class ElementT, class RandomAccessIterator, typename CompareFunction  >
	ElementT calculateMedian( RandomAccessIterator first, RandomAccessIterator last, CompareFunction&& fnCompare )
	{		
		std::sort( first, last, fnCompare );
		
		size_t nElements = last - first + 1;
		size_t iMiddleElement = nElements / 2;
		
		if(( nElements & 1 ) == 0 )
		{
			// Do we have an even number of pieces? If so, return the average between the two middle ones.
			//
			return ( *(first + iMiddleElement - 1) + *(first + iMiddleElement) ) * 0.5;
		}
		else
		{
			return *(first + iMiddleElement);
		}
	}
	
	template< class ElementT, class RandomAccessIterator >
	void calculateRootMeanSquared( ElementT& result, RandomAccessIterator first, RandomAccessIterator last )
	{		
		for( RandomAccessIterator iter = first; iter != last; ++iter )
		{
			result += (*iter) * (*iter);
		}
		
		result = sqrt( result / (last - first) );
	}
	
	template< class ElementT, class RandomAccessIterator >
	void calculateArithmeticMean( ElementT& result, RandomAccessIterator first, RandomAccessIterator last )
	{		
		for( RandomAccessIterator iter = first; iter != last; ++iter )
		{
			result += (*iter);
		}
		
		result /= (last - first);
	}
	
	template< class ElementT, class RandomAccessIterator, typename CompareFunction, typename WeightType >
	void calculateCurveWeightedMean( ElementT& result, RandomAccessIterator first, RandomAccessIterator last, CompareFunction&& fnCompare, WeightType minWeight = 0 )
	{		
		std::sort( first, last, fnCompare );
		
		const WeightType totalProgress = WeightType( last - 1 - first );
		
		if( totalProgress > 0 )
		{
			WeightType sumWeights = 0;
			
			for( RandomAccessIterator iter = first; iter != last; ++iter )
			{
				WeightType normalizedProgress = (( iter - first ) / totalProgress );
				
				WeightType weight = lerp( WeightType( 0.5 ) * ( WeightType( 1.0 ) + std::sin( -HALF_PI + normalizedProgress * TWO_PI )), minWeight, 1.0f );
				result += (*iter) * weight;
				
				sumWeights += weight;
			}
			
			ASSERT( sumWeights > 0 );
			
			result /= sumWeights;
		}
		else
		{
			result = *first;
		}
	}

	template< typename T, typename Real >
	T calculateSpringForce( const T& posDelta, Real distance, Real velDeltaDotPosDelta, Real stiffness, Real damping, Real restDistance = 0 )
	{
		REQUIRES( stiffness >= 0 );
		
		if( distance <= std::numeric_limits< Real >::epsilon() )
		{
			return T();
		}
		
		return
			( stiffness * ( distance - restDistance ) +
			damping * ( velDeltaDotPosDelta / distance )) * ( posDelta / distance );
	}
		
	template< typename TimeT, typename T >
	bool compareTimeKeyframes( const std::pair< TimeT, T >& a, const std::pair< TimeT, T >& b )
	{
		return a.first < b.first;
	}	
	
	template< typename ValueT, typename IteratorT, typename TimeT >
	void getKeyframedValue( IteratorT begin, IteratorT end, TimeT time, ValueT& outResult, bool interpolated = true, bool ifUninterpolatedRoundUp = true )
	{
		if( begin != end )
		{
			outResult = begin->second;
		
			if( time > begin->first )
			{
				while( begin != end )
				{
					IteratorT next = begin;
					++next;
					if( next != end )	// At least two elements away from the end.
					{
						// Find the pair of keyframes between which the time currently sits.
						//
						if( time <= next->first )
						{
							// Find the relative distance between these two frames [ i ] and [ i + 1 ].
							//
							TimeT relativeAlpha = static_cast< TimeT >(( time - begin->first ) / ( next->first - begin->first ));
							
							// Modulate the result based on the interpolated point.
							//
							if( interpolated )
							{
								outResult = lerp( begin->second, next->second, relativeAlpha );
							}
							else
							{
								outResult = ifUninterpolatedRoundUp ? next->second : begin->second;
							}
							break;
						}
					}
					else
					{
						// We've reached the last item.
						//
						outResult = begin->second;
						break;
					}
					
					++begin;
				}
			}
		}
	}
    
	template< typename PosT, typename DistT, typename fnClear >
	PosT findClearLocationByBisection( const PosT& clearLocation, const PosT& direction, DistT solidDistance, const fnClear isClear, DistT minAdjustment, DistT radius = DistT( 0 ) )
	{
		// We assume that the location at clearLocation is clear and the location at clearLocation + direction * solidDistance is solid.
		// This function returns the point that is on or between these two locations that is clear but as close as possible (as defined by minAdjustment) to solid.
		
		real clearDistance = 0;
		
		while( std::abs( solidDistance - clearDistance ) >= minAdjustment )
		{
			DistT distance(( clearDistance + solidDistance ) * DistT( 0.5 ));	// Midpoint.
			PosT testLocation( clearLocation + direction * distance );
			
			if( isClear( testLocation, radius ))
			{
				clearDistance = distance;
			}
			else
			{
				solidDistance = distance;
			}
		}
		
		return clearLocation + direction * clearDistance;
	}
	
	template< typename vec_t, typename real_t >
	vec_t updateVerlet( const vec_t& position, vec_t& inOutLastPosition, real_t damping, const vec_t& acceleration, real_t deltaTime )
	{
		vec_t result = position * ( real_t( 2 ) - damping ) - inOutLastPosition * ( real_t( 1 ) - damping ) + ( acceleration * deltaTime * deltaTime );
		inOutLastPosition = position;
		return result;
	}
					   
	template< typename vec_t, typename real_t >
	vec_t evaluateHermite( const vec_t& start, const vec_t& end, const vec_t& startTangent, const vec_t& endTangent, real_t t )
	{
		return  start * (2 * t * t * t - 3 * t * t + 1 ) +
				startTangent * ( t * t * t - 2 * t * t + t ) +
				end * ( -2 * t * t * t + 3 * t * t ) +
				endTangent * ( t * t * t - t * t );
	}	

	template< typename vec_t, typename real_t >
	vec_t evaluateHermiteDerivative( const vec_t& start, const vec_t& end, const vec_t& startTangent, const vec_t& endTangent, real_t t )
	{
		return	startTangent + end*6*t - endTangent*2*t - start*6*t - startTangent*4*t - 
				end*6*t*t + endTangent*3*t*t + start*6*t*t + 
				startTangent*3*t*t;
	}
	
	template< typename vec_t, typename real_t, typename fnPerStep >
	void traverseHermite( const vec_t& start, const vec_t& end, const vec_t& startTangent, const vec_t& endTangent, real_t stepSize, const fnPerStep& fn )
	{
		for( real_t t = 0; t <= real_t( 1 ); t += stepSize )
		{
			fn( evaluateHermite( start, end, startTangent, endTangent, t ), evaluateHermiteDerivative(  start, end, startTangent, endTangent, t ));
		}
	}
	
	template< typename value_t, typename real_t, typename fnPerStep >
	struct FunctionStepper
	{
		FunctionStepper( real_t stepSize, const fnPerStep& fn, real initialSkipDistance = 0 )
		:	distanceStepSize( stepSize )
		,	perStepFunction( fn )
		,	nextStepDistance( initialSkipDistance )
		{}
		
		void operator()( const value_t& p, const value_t& t ) const 
		{
			const auto distanceTravelled = ( p - lastStepPoint ).length();
			
			if( totalDistanceTravelled >= 0 )
			{
				totalDistanceTravelled += distanceTravelled;
			}
			else
			{
				totalDistanceTravelled = 0;
			}
			
			if( totalDistanceTravelled >= nextStepDistance )
			{
				if( lastStepDistance >= 0 )
				{
					const real_t relativeDistance = ( totalDistanceTravelled - nextStepDistance ) / distanceTravelled;
					perStepFunction( lerp( p, lastStepPoint, relativeDistance ), 
									 lerp( t, lastStepTangent, relativeDistance ) );
				}
				else
				{
					perStepFunction( p, t );
				}
				
				lastStepDistance = nextStepDistance;
				nextStepDistance += distanceStepSize;
			}
			
			lastStepPoint = p;
			lastStepTangent = t;
		}
		
		real_t remainingStepDistance() const
		{
			if( totalDistanceTravelled >= 0 )
			{
				return distanceStepSize - ( nextStepDistance - totalDistanceTravelled );
			}
			else
			{
				return distanceStepSize;
			}
		}
		
	private:
		
		real_t distanceStepSize;
		fnPerStep perStepFunction;
		mutable real_t nextStepDistance = 0;
		mutable real_t lastStepDistance = 0;
		mutable real_t totalDistanceTravelled = -1;
		mutable value_t lastStepPoint;
		mutable value_t lastStepTangent;
	};
	
	template< typename vec_t, typename real_t, typename fnPerStep >
	real_t traverseHermiteByDistance( const vec_t& start, 
									 const vec_t& end, 
									 const vec_t& startTangent, 
									 const vec_t& endTangent, 
									 real_t integrationStepSize, 
									 real_t distanceStepSize, 
									 const fnPerStep& fn, 
									 real_t initialSkipDistance = 0 )
	{
		FunctionStepper< vec_t, real_t, fnPerStep > stepper( distanceStepSize, fn, initialSkipDistance );
		
		traverseHermite( start, end, startTangent, endTangent, integrationStepSize, stepper );
		
		return stepper.remainingStepDistance();
	}
	
}		// END namespace fr

#endif
