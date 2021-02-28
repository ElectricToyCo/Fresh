#ifndef FRESH_VECTOR2_H_INCLUDED_
#define FRESH_VECTOR2_H_INCLUDED_

// Modified from WildMagic3 libraries.

#include "SerializeVector.h"
#include "FreshEssentials.h"
#include "Angle.h"
#include <limits>
#include <string>
#include <ostream>

namespace fr
{


	template <class Real>
	class Vector2
	{
	public:
		
		typedef Real element_type;		
		static const size_t nComponents = 2;

		Real x, y;

		Vector2();  // Initializes x and y to 0
		explicit Vector2( Real f );			// Sets both to f.
		Vector2( Real fX, Real fY );
		Vector2( const Vector2& other );

		void set( Real fX, Real fY );

		Real operator[]( size_t i ) const;
			// REQUIRES( 0 <= i && i <= 1 );
		Real& operator[]( size_t i );
			// REQUIRES( 0 <= i && i <= 1 );
		Real operator[] (int i) const						{ return operator[]( static_cast< size_t >( i )); }
		Real& operator[] (int i)							{ return operator[]( static_cast< size_t >( i )); }

		Vector2& operator=( const Vector2& other );

		bool operator==( const Vector2& other ) const;
		bool operator!=( const Vector2& other ) const;
		bool operator< ( const Vector2& other ) const;
		bool operator<=( const Vector2& other ) const;
		bool operator> ( const Vector2& other ) const;
		bool operator>=( const Vector2& other ) const;

		Vector2 operator+( const Vector2& other ) const;
		Vector2 operator-( const Vector2& other ) const;
		Vector2 operator*( const Vector2& other ) const;
		Vector2 operator/( const Vector2& other ) const;
		Vector2 operator+( Real fScalar ) const;
		Vector2 operator-( Real fScalar ) const;
		Vector2 operator*( Real fScalar ) const;
		Vector2 operator/( Real fScalar ) const;
			//  REQUIRES( fScalar != (Real) 0.0 );
		Vector2 operator-() const;

		Vector2& operator+=( const Vector2& other );
		Vector2& operator-=( const Vector2& other );
		Vector2& operator*=( const Vector2& other );
		Vector2& operator/=( const Vector2& other );
		Vector2& operator+=( Real fScalar ); 
		Vector2& operator-=( Real fScalar );
		Vector2& operator*=( Real fScalar ); 
		Vector2& operator/=( Real fScalar );
			//  REQUIRES( fScalar != (Real) 0.0 );

		Real length() const;
		Real lengthSquared() const;
		Real dot( const Vector2& other ) const;
		void setToZero();
		Vector2 normal() const;
		Real normalize();
		bool isZero( Real epsilon = std::numeric_limits< Real >::epsilon() ) const;
		bool isFinite() const;
		
		void rotate( fr::angle a );
		
		void invert();
		Vector2 getInverse() const;

		int majorAxis() const;
		int minorAxis() const;	
		
		Real majorAxisValue() const;
		Real minorAxisValue() const;	

		void snapToMajorAxis();
		void snapToMinorAxis();	
		void quickRot90();
		
		// Returns (y,-x)
		Vector2 getPerpendicular() const;
		
		void setToAngleNormal( fr::angle a );
		Real angleRadians() const;
		Real angleDegrees() const;
		fr::angle angle() const;
		
		Vector2 getRotated( fr::angle a ) const
		{
			Vector2 v( *this );
			v.rotate( a );
			return v;
		}
		
		static Vector2< Real > makeAngleNormal( fr::angle a )
		{
			Vector2< Real > result;
			result.setToAngleNormal( a );
			return result;
		}
		
		static Vector2< Real > makeFromPolar( const Vector2< Real >& degreesDist )
		{
			return makeAngleNormal( fr::angle( degreesDist.x )) * degreesDist.y;
		}
		
		std::string toString() const;

		static const Vector2 ZERO;
		static const Vector2 UNIT_X;
		static const Vector2 UNIT_Y;

	private:
		int compareArrays( const Vector2& other ) const;
			// For comparison functions
	};

	template <class Real>
	Vector2<Real> operator* (Real fScalar, const Vector2<Real>& vec );

	template <class Real>
	Vector2<Real> operator/ (Real fScalar, const Vector2<Real>& vec );
	
	template< class Real >
	Vector2< Real > sqrt( const Vector2< Real >& vec );
	
	template< class Real >
	Vector2< Real > abs( const Vector2< Real >& vec );
		
	template< class Real >
	Vector2< Real > min( const Vector2< Real >& a, const Vector2< Real >& b );
	
	template< class Real >
	Vector2< Real > max( const Vector2< Real >& a, const Vector2< Real >& b );
	
	template <class Real>
	std::ostream& operator<< (std::ostream& out, const Vector2<Real>& vec )
	{
		return SerializeVector::write( out, vec );
	}
	template < class Real >
	std::istream& operator>>( std::istream& in, Vector2<Real>& vec )
	{
		return SerializeVector::read( in, vec );
	}

	typedef Vector2<int> Vector2i;
	typedef Vector2<unsigned int> Vector2ui;
	typedef Vector2<float> Vector2f;
	typedef Vector2<double> Vector2d;


}		// END namespace fr

#include "Vector2.inl.h"

#endif
