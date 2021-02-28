//
//  Angle.h
//  Fresh
//
//  Created by Jeff Wofford on 12/13/11.
//  Copyright (c) 2011 jeffwofford.com. All rights reserved.
//

#ifndef Fresh_Angle_h
#define Fresh_Angle_h

#include "CoreCppCompatability.h"
#include <cmath>
#include <iostream>
#include <iomanip>

namespace fr
{
	// An Angle is useful for representing a 2D orientation. Angles are normalized within the range [-180,180[.
	// Angles implicitly convert from (but not *to*) any numeric type, which is interpreted as degrees.
	// The explicit fromDegrees() members provide conversion to other numeric types. Conversion to and from 
	// radians is provided through members fromRadians() and toRadians().
	//
	// Subtraction and comparison of angles is tricky. Clearly 10deg is > than 0deg, and 10deg - 0deg = 10deg.
	// Likewise 10deg > -5deg and 10deg - -5deg = 15deg.
	// Things get stickier at the "far" end--close to -180 and +180 deg.
	// Certainly 180deg - -180deg is 0deg, because 360deg is always "wrapped" to 0. (Or seen another way,
	// +180deg is always wrapped to -180deg.)
	// By the same logic, 170 - -170 = -20 (that is, +340 wrapped to the range [-180,180[).
	// According to general arithmetic rules, for any numbers a and b, if a - b < 0, then b > a.
	// Therefore 170 < -170!
	// Just to confirm, -170 - 170 = 20 (-340 wrapped). Therefore -170 > 170. Weird, but consistent.
	
	template< typename BaseT >
	class Angle
	{
	public:
		
		Angle() : Angle( 0U ) {}
		
		template< typename Real >
		Angle( Real angleDegrees );
		
		template< typename Real >
		void fromDegrees( Real degrees );

		template< typename Real >
		void fromRadians( Real radians );
		
		template< typename Real >
		Real toDegrees() const;

		template< typename Real >
		Real toRadians() const;
		
		void fromNormalized( float n );
		
		bool  operator==( Angle a ) const;
		bool  operator!=( Angle a ) const;
		bool  operator> ( Angle a ) const;
		bool  operator< ( Angle a ) const;
		bool  operator>=( Angle a ) const;
		bool  operator<=( Angle a ) const;
		
		Angle operator+ ( Angle a ) const;
		Angle operator- ( Angle a ) const;
		Angle operator+=( Angle a );
		Angle operator-=( Angle a );
		Angle operator* ( Angle a ) const;
		Angle operator/ ( Angle a ) const;
		Angle operator*=( Angle a );
		Angle operator/=( Angle a );

		template< typename Real >
		Angle operator+ ( Real a ) const;
		template< typename Real >
		Angle operator- ( Real a ) const;
		template< typename Real >
		Angle operator* ( Real a ) const;
		template< typename Real >
		Angle operator/ ( Real a ) const;
		template< typename Real >
		Angle operator+=( Real a );
		template< typename Real >
		Angle operator-=( Real a );
		template< typename Real >
		Angle operator*=( Real a );
		template< typename Real >
		Angle operator/=( Real a );

		Angle operator++();
		Angle operator++( int );
		Angle operator-() const;
		
		template< typename Real >
		static Angle FromRadians( Real radians );
		
	private:
		
		static const float MIN_RADIANS;
		static const float MAX_RADIANS;
		static const float MIN_DEGREES;
		static const float MAX_DEGREES;
		
		static BaseT MAX_VALUE() { return std::numeric_limits< BaseT >::max(); }
		
		BaseT value;
		
		template< typename AngleT >
		friend std::ostream& operator<<( std::ostream& out, const Angle< AngleT > a );
		template< typename AngleT >
		friend std::istream& operator>>( std::istream& in,  Angle< AngleT >& a );
		
		template< typename AngleT, typename Real >
		friend Angle< AngleT > operator+ ( Real r, const Angle< AngleT > a );
		template< typename AngleT, typename Real >
		friend Angle< AngleT > operator- ( Real r, const Angle< AngleT > a );
		template< typename AngleT, typename Real >
		friend Angle< AngleT > operator* ( Real r, const Angle< AngleT > a );
		template< typename AngleT, typename Real >
		friend Angle< AngleT > operator/ ( Real r, const Angle< AngleT > a );
		
		template< typename T >
		constexpr inline T fast_fmod( T x, T y ) const
		{
			// REQUIRES( y > 0 );
			return x - int( x / y ) * y;
		}
	};
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	typedef Angle< signed int > angle;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	template< typename BaseT >
	const float Angle< BaseT >::MIN_RADIANS = -3.1415926538f;
	template< typename BaseT >
	const float Angle< BaseT >::MAX_RADIANS =  3.1415926538f;
	template< typename BaseT >
	const float Angle< BaseT >::MIN_DEGREES = -180.0f;
	template< typename BaseT >
	const float Angle< BaseT >::MAX_DEGREES =  180.0f;

	template< typename BaseT >
	inline Angle< BaseT > abs( Angle< BaseT > a )
	{
		return a < angle( 0 ) ? -a : a;
	}
	
	template< typename BaseT >
	inline float sign( Angle< BaseT > a )
	{
		return a > angle( 0 ) ? 1 : a < angle( 0 ) ? -1 : 0;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	inline float angle_utility_modulo( float x, float y )
	{
		return x - y * std::floor( x / y );
	}
	
	inline float angle_utility_wrap( float a, float min, float max )
	{
		ASSERT( max > min );
		const float range = max - min;
		return min + angle_utility_modulo( a, range );
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	// Static factory
	//
	template< typename BaseT >
	template< typename Real >
	Angle< BaseT > Angle< BaseT >::FromRadians( Real radians )
	{
		Angle< BaseT > a;
		a.fromRadians( radians );
		return a;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	template< typename BaseT >
	template< typename Real >
	Angle< BaseT >::Angle( Real angleDegrees )
	{
		fromDegrees( angleDegrees );
	}
	
	template< typename BaseT >
	void Angle< BaseT >::fromNormalized( float n )
	{
		value = static_cast< BaseT >( angle_utility_wrap( n, -1.0f, 1.0f ) * MAX_VALUE() );
	}

	template< typename BaseT >
	template< typename Real >
	void Angle< BaseT >::fromDegrees( Real degrees )
	{
		fromNormalized(
			 ( fast_fmod( (float) degrees - MIN_DEGREES, MAX_DEGREES - MIN_DEGREES ))
			 / MAX_DEGREES );	
	}
	
	template< typename BaseT >
	template< typename Real >
	void Angle< BaseT >::fromRadians( Real radians )
	{
		fromNormalized(
			   ( fast_fmod( (float) radians - MIN_RADIANS, MAX_RADIANS - MIN_RADIANS ))
			   / MAX_RADIANS );	
	}
	
	template< typename BaseT >
	template< typename Real >
	Real Angle< BaseT >::toDegrees() const
	{
		return static_cast< Real >( static_cast< float >( value ) / MAX_VALUE() * static_cast< float >( MAX_DEGREES ));
	}
	
	template< typename BaseT >
	template< typename Real >
	Real Angle< BaseT >::toRadians() const
	{
		return static_cast< Real >( static_cast< float >( value ) / MAX_VALUE() * static_cast< float >( MAX_RADIANS ));
	}
	
	template< typename BaseT >
	bool  Angle< BaseT >::operator==( Angle a ) const
	{
		return value == a.value;
	}
	
	template< typename BaseT >
	bool  Angle< BaseT >::operator!=( Angle a ) const
	{
		return !operator==( a );
	}
	
	template< typename BaseT >
	bool  Angle< BaseT >::operator> ( Angle a ) const
	{
		return ( value - a.value ) > 0;
	}
	
	template< typename BaseT >
	bool  Angle< BaseT >::operator< ( Angle a ) const
	{
		return ( value - a.value ) < 0;
	}
	
	template< typename BaseT >
	bool  Angle< BaseT >::operator>=( Angle a ) const
	{
		return operator>( a ) || operator==( a );
	}
	
	template< typename BaseT >
	bool  Angle< BaseT >::operator<=( Angle a ) const
	{
		return operator<( a ) || operator==( a );
	}

	template< typename BaseT >
	Angle< BaseT > Angle< BaseT >::operator+ ( Angle a ) const
	{
		Angle result;
		result.value = value + a.value;
		return result;
	}
	
	template< typename BaseT >
	Angle< BaseT > Angle< BaseT >::operator- ( Angle a ) const
	{
		Angle result;
		result.value = value - a.value;
		return result;
	}
	
	template< typename BaseT >
	Angle< BaseT > Angle< BaseT >::operator+=( Angle a )
	{
		value += a.value;
		return *this;
	}
	
	template< typename BaseT >
	Angle< BaseT > Angle< BaseT >::operator-=( Angle a )
	{
		value -= a.value;
		return *this;
	}
	
	template< typename BaseT >
	Angle< BaseT > Angle< BaseT >::operator* ( Angle a ) const
	{
		Angle result;
		result.value = value * a.value;
		return result;
	}
	
	template< typename BaseT >
	Angle< BaseT > Angle< BaseT >::operator/ ( Angle a ) const
	{
		Angle result;
		result.value = value / a.value;
		return result;
	}
	
	template< typename BaseT >
	Angle< BaseT > Angle< BaseT >::operator*=( Angle a )
	{
		value *= a.value;
		return *this;
	}
	
	template< typename BaseT >
	Angle< BaseT > Angle< BaseT >::operator/=( Angle a )
	{
		value /= a.value;
		return *this;
	}
	template< typename BaseT >
	template< typename Real >
	Angle< BaseT > Angle< BaseT >::operator+ ( Real a ) const
	{
		Angle result;
		result.value = value + a;
		return result;
	}
	
	template< typename BaseT >
	template< typename Real >
	Angle< BaseT > Angle< BaseT >::operator- ( Real a ) const
	{
		Angle result;
		result.value = value - a;
		return result;
	}
	
	template< typename BaseT >
	template< typename Real >
	Angle< BaseT > Angle< BaseT >::operator* ( Real a ) const
	{
		Angle result;
		result.value = value * a;
		return result;
	}
	
	template< typename BaseT >
	template< typename Real >
	Angle< BaseT > Angle< BaseT >::operator/ ( Real a ) const
	{
		Angle result;
		result.value = value / a;
		return result;
	}
	
	template< typename BaseT >
	template< typename Real >
	Angle< BaseT > Angle< BaseT >::operator+=( Real a )
	{
		value = static_cast< BaseT >( value + a );
		return *this;
	}
	
	template< typename BaseT >
	template< typename Real >
	Angle< BaseT > Angle< BaseT >::operator-=( Real a )
	{
		value = static_cast< BaseT >( value - a );
		return *this;
	}
	
	template< typename BaseT >
	template< typename Real >
	Angle< BaseT > Angle< BaseT >::operator*=( Real a )
	{
		value = static_cast< BaseT >( value * a );
		return *this;
	}
	
	template< typename BaseT >
	template< typename Real >
	Angle< BaseT > Angle< BaseT >::operator/=( Real a )
	{
		value = static_cast< BaseT >( value / a );
		return *this;
	}
	
	template< typename BaseT >
	Angle< BaseT > Angle< BaseT >::operator++()
	{
		++value;
		return *this;
	}
	
	template< typename BaseT >
	Angle< BaseT > Angle< BaseT >::operator++( int )
	{
		Angle copy( *this );
		++value;
		return copy;
	}
	
	template< typename BaseT >
	Angle< BaseT > Angle< BaseT >::operator-() const
	{
		Angle result;
		result.value = -value;
		return result;
	}

	template< typename BaseT >
	std::ostream& operator<<( std::ostream& out, const Angle< BaseT > a )
	{
		const float degrees = a.template toDegrees< float >();
		out << std::fixed << degrees << std::resetiosflags( std::ios_base::fixed );
		return out;
	}

	template< typename BaseT >
	std::istream& operator>>( std::istream& in,  Angle< BaseT >& a )
	{
		float degrees = 0;
		in >> degrees;
		a.fromDegrees( degrees );
		return in;
	}

	template< typename AngleT, typename Real >
	Angle< AngleT > operator+ ( Real r, const Angle< AngleT > a )
	{
		Angle< AngleT > result( a );
		result.value = r + a.value;
		return result;
	}
	
	template< typename AngleT, typename Real >
	Angle< AngleT > operator- ( Real r, const Angle< AngleT > a )
	{
		Angle< AngleT > result;
		result.value = r - a.value;
		return result;
	}
	
	template< typename AngleT, typename Real >
	Angle< AngleT > operator* ( Real r, const Angle< AngleT > a )
	{
		Angle< AngleT > result( a );
		result.value = r * a.value;
		return result;
	}
	
	template< typename AngleT, typename Real >
	Angle< AngleT > operator/ ( Real r, const Angle< AngleT > a )
	{
		Angle< AngleT > result( a );
		result.value = r / a.value;
		return result;
	}

}

#endif
