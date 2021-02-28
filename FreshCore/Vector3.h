#ifndef FRESH_VECTOR3_H_INCLUDED
#define FRESH_VECTOR3_H_INCLUDED

// Modified from WildMagic3 libraries.

#include "SerializeVector.h"
#include <ostream>
#include <limits>

namespace fr
{

	template <class Real>
	class Vector3
	{
	public:

		typedef Real element_type;
		static const size_t nComponents = 3;

		union
		{
			struct
			{
				Real x, y, z;
			};
			Real m_arrCoords[3];
		};
		
		// construction
		Vector3 ();  // Initialized to 0
		explicit Vector3( Real f );					// Sets all to f.
		Vector3 (Real fX, Real fY, Real fZ);
		Vector3 (const Real* afTuple);
		Vector3 (const Vector3& rkV);

		void set(Real fX, Real fY, Real fZ);
		void setToZero()								{ set( 0, 0, 0 ); }

		bool isZero( Real epsilon = std::numeric_limits< Real >::epsilon() ) const;
		
		// coordinate access
		operator const Real* () const;
		operator Real* ();
		Real operator[] (size_t i) const;
		Real& operator[] (size_t i);
		Real operator[] (int i) const						{ return operator[]( static_cast< size_t >( i )); }
		Real& operator[] (int i)							{ return operator[]( static_cast< size_t >( i )); }
		Real X () const;
		Real& X ();
		Real Y () const;
		Real& Y ();
		Real Z () const;
		Real& Z ();

		// assignment
		Vector3& operator= (const Vector3& rkV);

		// comparison
		bool operator== (const Vector3& rkV) const;
		bool operator!= (const Vector3& rkV) const;
		bool operator<  (const Vector3& rkV) const;
		bool operator<= (const Vector3& rkV) const;
		bool operator>  (const Vector3& rkV) const;
		bool operator>= (const Vector3& rkV) const;

		// arithmetic operations
		Vector3 operator+ (const Vector3& rkV) const;
		Vector3 operator- (const Vector3& rkV) const;
		Vector3 operator* (const Vector3& rkV) const;
		Vector3 operator/ (const Vector3& rkV) const;
		Vector3 operator* (Real fScalar) const;
		Vector3 operator/ (Real fScalar) const;
		Vector3 operator- () const;

		// arithmetic updates
		Vector3& operator+= (const Vector3& rkV);
		Vector3& operator-= (const Vector3& rkV);
		Vector3& operator*= (const Vector3& rkV);
		Vector3& operator/= (const Vector3& rkV);
		Vector3& operator*= (Real fScalar);
		Vector3& operator/= (Real fScalar);

		void rotateAboutX( Real degrees );
		void rotateAboutY( Real degrees );
		void rotateAboutZ( Real degrees );
		void rotateByEulerDegrees( const Vector3& eulerAngles );

		// vector operations
		Real length () const;
		Real lengthSquared () const;
		Real dot (const Vector3& rkV) const;
		Vector3 normal() const;
		Real normalize ();

		// The cross products are computed using the right-handed rule.  Be aware
		// that some graphics APIs use a left-handed rule.  If you have to compute
		// a cross product with these functions and send the result to the API
		// that expects left-handed, you will need to change sign on the vector
		// (replace each component value c by -c).
		Vector3 cross (const Vector3& rkV) const;
		Vector3 unitCross (const Vector3& rkV) const;

		// Gram-Schmidt orthonormalization.  Take linearly independent vectors
		// U, V, and W and compute an orthonormal set (unit length, mutually
		// perpendicular).
		static void orthonormalize (Vector3& rkU, Vector3& rkV, Vector3& rkW);
		static void orthonormalize (Vector3* akV);

		static void computeExtremes (int iVQuantity, const Vector3* akPoint,
			Vector3& rkMin, Vector3& rkMax);

		static const Vector3 ZERO;
		static const Vector3 UNIT_X;
		static const Vector3 UNIT_Y;
		static const Vector3 UNIT_Z;

	private:
		
		// support for comparisons
		int compareArrays (const Vector3& rkV) const;
	};

	// arithmetic operations
	template <class Real>
	Vector3<Real> operator* (Real fScalar, const Vector3<Real>& rkV);

	template <class Real>
	std::ostream& operator<< (std::ostream& out, const Vector3<Real>& vec )
	{
		return SerializeVector::write( out, vec );
	}
	template < class Real >
	std::istream& operator>>( std::istream& in, Vector3<Real>& vec )
	{
		return SerializeVector::read( in, vec );
	}

	typedef Vector3<int> Vector3i;
	typedef Vector3<float> Vector3f;
	typedef Vector3<double> Vector3d;

}		// END namespace fr


#include "Vector3.inl.h"



#endif
