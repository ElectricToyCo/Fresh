#include "FreshDebug.h"
#include "SerializeVector.h"
#include "FreshMath.h"
#include <cmath>
#include <sstream>

// Modified from WildMagic3 libraries.

namespace fr
{
	
	template<> const Vector2<int> Vector2<int>::ZERO;
	template<> const Vector2<int> Vector2<int>::UNIT_X;
	template<> const Vector2<int> Vector2<int>::UNIT_Y;
	;
	template<> const Vector2<unsigned int> Vector2<unsigned int>::ZERO;
	template<> const Vector2<unsigned int> Vector2<unsigned int>::UNIT_X;
	template<> const Vector2<unsigned int> Vector2<unsigned int>::UNIT_Y;
	
	template<> const Vector2<float> Vector2<float>::ZERO;
	template<> const Vector2<float> Vector2<float>::UNIT_X;
	template<> const Vector2<float> Vector2<float>::UNIT_Y;
	
	template<> const Vector2<double> Vector2<double>::ZERO;
	template<> const Vector2<double> Vector2<double>::UNIT_X;
	template<> const Vector2<double> Vector2<double>::UNIT_Y;

	template< class Real >
	ALWAYS_INLINE Vector2< Real >::Vector2()
	{
		x = y = 0;
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real >::Vector2( Real f )
	{
		x = y = f;
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real >::Vector2( Real fX, Real fY )
	{
		x = fX;
		y = fY;
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real >::Vector2( const Vector2& vec )
	{
		x = vec.x;
		y = vec.y;
	}

	template< class Real >
	ALWAYS_INLINE void Vector2< Real >::set( Real fX, Real fY )
	{
		x = fX;
		y = fY;
	}

	template< class Real >
	ALWAYS_INLINE Real Vector2< Real >::operator[]( size_t i ) const
	{
		REQUIRES( 0 <= i && i <= 1 );
		return (&x)[i];
	}

	template< class Real >
	ALWAYS_INLINE Real& Vector2< Real >::operator[]( size_t i )
	{
		REQUIRES( 0 <= i && i <= 1 );
		return (&x)[i];
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real >& Vector2< Real >::operator=( const Vector2& vec )
	{
		x = vec.x;
		y = vec.y;
		return *this;
	}

	template< class Real >
	ALWAYS_INLINE int Vector2< Real >::compareArrays( const Vector2& vec ) const
	{
		return std::memcmp( (&x), &(vec.x), 2 * sizeof(Real) );
	}

	template< class Real >
	ALWAYS_INLINE bool Vector2< Real >::operator==( const Vector2& vec ) const
	{
		return compareArrays( vec ) == 0;
	}

	template< class Real >
	ALWAYS_INLINE bool Vector2< Real >::operator!=( const Vector2& vec ) const
	{
		return compareArrays( vec ) != 0;
	}

	template< class Real >
	ALWAYS_INLINE bool Vector2< Real >::operator<( const Vector2& vec ) const
	{
		return compareArrays( vec ) < 0;
	}

	template< class Real >
	ALWAYS_INLINE bool Vector2< Real >::operator<=( const Vector2& vec ) const
	{
		return compareArrays( vec ) <= 0;
	}

	template< class Real >
	ALWAYS_INLINE bool Vector2< Real >::operator>( const Vector2& vec ) const
	{
		return compareArrays( vec ) > 0;
	}

	template< class Real >
	ALWAYS_INLINE bool Vector2< Real >::operator>=( const Vector2& vec ) const
	{
		return compareArrays( vec ) >= 0;
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real > Vector2< Real >::operator+( const Vector2& vec ) const
	{
		return Vector2(
			x+vec.x,
			y+vec.y);
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real > Vector2< Real >::operator-( const Vector2& vec ) const
	{
		return Vector2(
			x-vec.x,
			y-vec.y);
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real > Vector2< Real >::operator*( const Vector2& vec ) const
	{
		return Vector2( 
			x * vec.x,
			y * vec.y );
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real > Vector2< Real >::operator/( const Vector2& vec ) const
	{
		return Vector2( 
			x / vec.x,
			y / vec.y );
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real > Vector2< Real >::operator+( Real scalar) const 
	{
		return Vector2(
					   scalar+x,
					   scalar+y);
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real > Vector2< Real >::operator-( Real scalar) const 
	{
		return Vector2(
					   x-scalar,
					   y-scalar);
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real > Vector2< Real >::operator*( Real scalar) const 
	{
		return Vector2(
			scalar * x,
			scalar * y);
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real > Vector2< Real >::operator/( Real scalar) const 
	{
		REQUIRES( scalar != (Real) 0.0 );

		return Vector2(
					   x / scalar,
					   y / scalar);
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real > Vector2< Real >::operator-() const
	{
		return Vector2< Real >(
			-x,
			-y);
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real > operator*( Real scalar, const Vector2< Real >& vec )
	{
		return Vector2< Real >(
			scalar * vec[0],
			scalar * vec[1]);
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real > operator/( Real scalar, const Vector2< Real >& vec )
	{
		return Vector2< Real >(
							   scalar / vec[0],
							   scalar / vec[1]);
	}
	
	template< class Real >
	ALWAYS_INLINE Vector2< Real >& Vector2< Real >::operator+=( const Vector2& vec )
	{
		x += vec.x;
		y += vec.y;
		return *this;
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real >& Vector2< Real >::operator-=( const Vector2& vec )
	{
		x -= vec.x;
		y -= vec.y;
		return *this;
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real >& Vector2< Real >::operator*=( const Vector2& vec )
	{
		x *= vec.x;
		y *= vec.y;
		return *this;
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real >& Vector2< Real >::operator/=( const Vector2& vec )
	{
		x /= vec.x;
		y /= vec.y;
		return *this;
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real >& Vector2< Real >::operator+=( Real scalar )
	{
		x += scalar;
		y += scalar;
		return *this;
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real >& Vector2< Real >::operator-=( Real scalar )
	{
		x -= scalar;
		y -= scalar;
		return *this;
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real >& Vector2< Real >::operator*=( Real scalar )
	{
		x *= scalar;
		y *= scalar;
		return *this;
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real >& Vector2< Real >::operator/=( Real scalar )
	{
		REQUIRES( scalar != (Real) 0.0 );

		Real invScalar = ((Real)1.0)/scalar;
		x *= invScalar;
		y *= invScalar;

		return *this;
	}

	template<>
	ALWAYS_INLINE Vector2< int >& Vector2< int >::operator/=( int scalar )
	{
		REQUIRES( scalar != 0 );
		
		x /= scalar;
		y /= scalar;
		
		return *this;
	}

	template<>
	ALWAYS_INLINE Vector2< unsigned int >& Vector2< unsigned int >::operator/=( unsigned int scalar )
	{
		REQUIRES( scalar != 0 );
		
		x /= scalar;
		y /= scalar;
		
		return *this;
	}

	template< class Real >
	ALWAYS_INLINE Real Vector2< Real >::length () const
	{
		return static_cast< Real >( std::sqrt(
			x * x +
			y * y ));
	}

	template< class Real >
	ALWAYS_INLINE Real Vector2< Real >::lengthSquared() const
	{
		return
			x * x +
			y * y;
	}

	template< class Real >
	ALWAYS_INLINE Real Vector2< Real >::dot( const Vector2& vec ) const
	{
		return
			x * vec.x +
			y * vec.y;
	}

	template< class Real >
	ALWAYS_INLINE void Vector2< Real >::setToZero()
	{
		x = y = 0;
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real > Vector2< Real >::normal() const
	{
		Vector2< Real > result( *this );
		result.normalize();
		return result;
	}

	template< class Real >
	ALWAYS_INLINE Real Vector2< Real >::normalize()
	{
		Real magnitude = length();

		if( magnitude > std::numeric_limits< Real>::epsilon() )
		{
			Real invMagnitude = ((Real)1.0)/magnitude;
			x *= invMagnitude;
			y *= invMagnitude;
		}
		else
		{
			magnitude = (Real)0.0;
			x = (Real)0.0;
			y = (Real)0.0;
		}

		return magnitude;
	}

	template<>
	inline int Vector2< int >::normalize()
	{
		int magnitude = length();
		
		if( magnitude > 0 )
		{
			x /= magnitude;
			y /= magnitude;
		}
		else
		{
			magnitude = 0;
			x = 0;
			y = 0;
		}
		
		return magnitude;
	}

	template<>
	inline unsigned int Vector2< unsigned int >::normalize()
	{
		unsigned int magnitude = length();
		
		if( magnitude > 0 )
		{
			x /= magnitude;
			y /= magnitude;
		}
		else
		{
			magnitude = 0;
			x = 0;
			y = 0;
		}
		
		return magnitude;
	}

	template< class Real >
	ALWAYS_INLINE bool Vector2< Real >::isZero( Real epsilon ) const
	{
		return std::abs( x ) <= epsilon && std::abs( y ) <= epsilon;
	}

	template<>
	ALWAYS_INLINE  bool Vector2< unsigned int >::isZero( unsigned int epsilon ) const
	{
		return x <= epsilon && y <= epsilon;
	}
	
	template< class Real >
	ALWAYS_INLINE bool Vector2< Real >::isFinite() const
	{
		return !isInfinite( std::abs( x )) && !isInfinite( std::abs( y ));
	}
	
	template< class Real >
	ALWAYS_INLINE void Vector2< Real >::rotate( fr::angle a )
	{
		// TODO use angle's cos/sin functions.
		// TODO watch out for ordinal types
		
		Real angleRadians = a.toRadians< Real >();
		
		Real cosR = std::cos( angleRadians );
		Real sinR = std::sin( angleRadians );			
		Real newX = cosR * x - sinR * y;
		y = sinR * x + cosR * y;
		x = newX;
	}

	template< class Real >
	ALWAYS_INLINE void Vector2< Real >::invert()
	{
		x = -x;
		y = -y;
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real > Vector2< Real >::getInverse() const
	{
		return Vector2( -x, -y );
	}

	template< class Real >
	ALWAYS_INLINE void Vector2< Real >::quickRot90()
	{
		Real temp = x;
		x = -y;
		y = temp;
	}

	template< class Real >
	ALWAYS_INLINE Real Vector2< Real >::majorAxisValue() const
	{
		return (&x)[ majorAxis() ];
	}

	template< class Real >
	ALWAYS_INLINE Real Vector2< Real >::minorAxisValue() const
	{
		return (&x)[ minorAxis() ];
	}

	template< class Real >
	ALWAYS_INLINE int Vector2< Real >::majorAxis() const
	{
		return ( std::abs( x ) >= std::abs( y )) ? 0 : 1;
	}

	template< class Real >
	ALWAYS_INLINE int Vector2< Real >::minorAxis() const
	{
		return ( std::abs( x ) < std::abs( y )) ? 0 : 1;
	}

	template< class Real >
	ALWAYS_INLINE void Vector2< Real >::snapToMajorAxis()
	{
		(&x)[ minorAxis() ] = 0;
	}

	template< class Real >
	ALWAYS_INLINE void Vector2< Real >::snapToMinorAxis()
	{
		(&x)[ majorAxis() ] = 0;
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real > Vector2< Real >::getPerpendicular() const
	{
		return Vector2( y, -x );
	}

	template< class Real >
	ALWAYS_INLINE void Vector2< Real >::setToAngleNormal( fr::angle a )
	{
		const Real angleRadians = a.toRadians< Real >();
		x = std::cos( angleRadians );
		y = std::sin( angleRadians );
	}

	template< class Real >
	ALWAYS_INLINE Real Vector2< Real >::angleRadians() const
	{
		return std::atan2( y, x );
	}

	template< class Real >
	ALWAYS_INLINE Real Vector2< Real >::angleDegrees() const
	{
		return radiansToDegrees( angleRadians() );
	}

	template< class Real >
	ALWAYS_INLINE fr::angle Vector2< Real >::angle() const
	{
		fr::angle a;
		a.fromRadians( angleRadians() );
		return a;
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real > sqrt( const Vector2< Real >& other )
	{
		return Vector2< Real >( std::sqrt( other.x ), std::sqrt( other.y ) );
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real > abs( const Vector2< Real >& vec )
	{
		return Vector2< Real >( std::abs( vec.x ), std::abs( vec.y ));
	}

	template< class Real >
	ALWAYS_INLINE Vector2< Real > min( const Vector2< Real >& a, const Vector2< Real >& b )
	{
		return Vector2< Real >( std::min( a.x, b.x ), std::min( a.y, b.y ));
	}
	
	template< class Real >
	ALWAYS_INLINE Vector2< Real > max( const Vector2< Real >& a, const Vector2< Real >& b )
	{
		return Vector2< Real >( std::max( a.x, b.x ), std::max( a.y, b.y ));
	}

	template< class Real >
	ALWAYS_INLINE std::string Vector2< Real >::toString() const
	{
		std::ostringstream out;
		out << this;
		return out.str();
	}
}

