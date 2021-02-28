#ifndef FRESH_VECTOR4_H_INCLUDED
#define FRESH_VECTOR4_H_INCLUDED

// Adapted from WildMagic3.

#include "SerializeVector.h"
#include <ostream>
#include <limits>

namespace fr
{

	template <class Real>
	class Vector3;

	template <class Real>
	class Vector4
	{
	public:
		
		typedef Real element_type;
		static const size_t nComponents = 4;

		union
		{
			struct
			{
				Real x, y, z, w;
			};
			Real m_arrCoords[4];
		};
		
		
		// construction
		Vector4 ();  // Initialized to 0.
		explicit Vector4( Real f );			// Set all to f.
		Vector4 (Real fX, Real fY, Real fZ, Real fW);
		Vector4 (const Real* afTuple);
		Vector4 (const Vector4& rkV);
		Vector4 (const Vector3< Real >& vec3, Real w );
		
		void set( Real fX, Real fY, Real fZ, Real fW)		{ x = fX; y = fY; z = fZ; w = fW; }
		void setToZero()									{ set( 0, 0, 0, 0 ); }
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
		Real W () const;
		Real& W ();

		// assignment
		Vector4& operator= (const Vector4& rkV);
		Vector4& operator= (const Vector3< Real >& rkV);

		// comparison
		bool operator== (const Vector4& rkV) const;
		bool operator!= (const Vector4& rkV) const;
		bool operator<  (const Vector4& rkV) const;
		bool operator<= (const Vector4& rkV) const;
		bool operator>  (const Vector4& rkV) const;
		bool operator>= (const Vector4& rkV) const;

		// arithmetic operations
		Vector4 operator+ (const Vector4& rkV) const;
		Vector4 operator- (const Vector4& rkV) const;
		Vector4 operator* (const Vector4& rkV) const;
		Vector4 operator/ (const Vector4& rkV) const;
		Vector4 operator+ (Real fScalar) const;
		Vector4 operator- (Real fScalar) const;
		Vector4 operator* (Real fScalar) const;
		Vector4 operator/ (Real fScalar) const;
		Vector4 operator- () const;

		// arithmetic updates
		Vector4& operator+= (const Vector4& rkV);
		Vector4& operator-= (const Vector4& rkV);
		Vector4& operator*= (const Vector4& rkV);
		Vector4& operator/= (const Vector4& rkV);
		Vector4& operator+= (Real fScalar);
		Vector4& operator-= (Real fScalar);
		Vector4& operator*= (Real fScalar);
		Vector4& operator/= (Real fScalar);

		// vector operations
		Real length () const;
		Real lengthSquared () const;
		Real dot (const Vector4& rkV) const;
		Real dotAs2 (const Vector4& rkV) const;
		Real dotAs3 (const Vector4& rkV) const;
		Real normalize ();
		Vector4 crossAsVec3(const Vector4& rkV) const;

		// special vectors
		static const Vector4 ZERO;
		static const Vector4 UNIT_X;
		static const Vector4 UNIT_Y;
		static const Vector4 UNIT_Z;
		static const Vector4 UNIT_W;

	private:
		// support for comparisons
		int compareArrays (const Vector4& rkV) const;
	};

	// arithmetic operations
	template <class Real>
	Vector4<Real> operator* (Real fScalar, const Vector4<Real>& rkV);

	template <class Real>
	std::ostream& operator<< (std::ostream& out, const Vector4<Real>& vec )
	{
		return SerializeVector::write( out, vec );
	}
	template < class Real >
	std::istream& operator>>( std::istream& in, Vector4<Real>& vec )
	{
		return SerializeVector::read( in, vec );
	}

	typedef Vector4<int> Vector4i;
	typedef Vector4<unsigned int> Vector4ui;
	typedef Vector4<float> Vector4f;
	typedef Vector4<double> Vector4d;

}

#include "Vector4.inl.h"



#endif
