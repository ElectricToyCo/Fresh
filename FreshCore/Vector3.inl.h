// Modified from WildMagic3 libraries.
#include "FreshDebug.h"
#include <cmath>

namespace fr
{
	
	template <class Real>
	Vector3<Real>::Vector3()
	{
		m_arrCoords[0] = m_arrCoords[1] = m_arrCoords[2] = 0;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector3<Real>::Vector3 (Real f)
	{
		m_arrCoords[0] = m_arrCoords[1] = m_arrCoords[2] = f;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector3<Real>::Vector3 (Real fX, Real fY, Real fZ)
	{
		m_arrCoords[0] = fX;
		m_arrCoords[1] = fY;
		m_arrCoords[2] = fZ;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector3<Real>::Vector3 (const Real* afTuple)
	{
		m_arrCoords[0] = afTuple[0];
		m_arrCoords[1] = afTuple[1];
		m_arrCoords[2] = afTuple[2];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector3<Real>::Vector3 (const Vector3& rkV)
	{
		m_arrCoords[0] = rkV.m_arrCoords[0];
		m_arrCoords[1] = rkV.m_arrCoords[1];
		m_arrCoords[2] = rkV.m_arrCoords[2];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	void Vector3<Real>::set(Real fX, Real fY, Real fZ)
	{
		m_arrCoords[0] = fX;
		m_arrCoords[1] = fY;
		m_arrCoords[2] = fZ;
	}
	//----------------------------------------------------------------------------
	template< class Real >
	bool Vector3< Real >::isZero( Real epsilon ) const
	{
		return std::abs( m_arrCoords[0] ) <= epsilon && 
			std::abs( m_arrCoords[1] ) <= epsilon &&
			std::abs( m_arrCoords[2] ) <= epsilon;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector3<Real>::operator const Real* () const
	{
		return m_arrCoords;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector3<Real>::operator Real* ()
	{
		return m_arrCoords;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real Vector3<Real>::operator[] (size_t i) const
	{
		ASSERT(0 <= i && i <= 2);
		return m_arrCoords[i];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real& Vector3<Real>::operator[] (size_t i)
	{
		ASSERT(0 <= i && i <= 2);
		return m_arrCoords[i];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real Vector3<Real>::X () const
	{
		return m_arrCoords[0];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real& Vector3<Real>::X ()
	{
		return m_arrCoords[0];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real Vector3<Real>::Y () const
	{
		return m_arrCoords[1];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real& Vector3<Real>::Y ()
	{
		return m_arrCoords[1];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real Vector3<Real>::Z () const
	{
		return m_arrCoords[2];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real& Vector3<Real>::Z ()
	{
		return m_arrCoords[2];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector3<Real>& Vector3<Real>::operator= (const Vector3& rkV)
	{
		m_arrCoords[0] = rkV.m_arrCoords[0];
		m_arrCoords[1] = rkV.m_arrCoords[1];
		m_arrCoords[2] = rkV.m_arrCoords[2];
		return *this;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	int Vector3<Real>::compareArrays (const Vector3& rkV) const
	{
		return memcmp(m_arrCoords,rkV.m_arrCoords,3*sizeof(Real));
	}
	//----------------------------------------------------------------------------
	template <class Real>
	bool Vector3<Real>::operator== (const Vector3& rkV) const
	{
		return compareArrays(rkV) == 0;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	bool Vector3<Real>::operator!= (const Vector3& rkV) const
	{
		return compareArrays(rkV) != 0;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	bool Vector3<Real>::operator< (const Vector3& rkV) const
	{
		return compareArrays(rkV) < 0;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	bool Vector3<Real>::operator<= (const Vector3& rkV) const
	{
		return compareArrays(rkV) <= 0;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	bool Vector3<Real>::operator> (const Vector3& rkV) const
	{
		return compareArrays(rkV) > 0;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	bool Vector3<Real>::operator>= (const Vector3& rkV) const
	{
		return compareArrays(rkV) >= 0;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector3<Real> Vector3<Real>::operator+ (const Vector3& rkV) const
	{
		return Vector3(
			m_arrCoords[0]+rkV.m_arrCoords[0],
			m_arrCoords[1]+rkV.m_arrCoords[1],
			m_arrCoords[2]+rkV.m_arrCoords[2]);
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector3<Real> Vector3<Real>::operator- (const Vector3& rkV) const
	{
		return Vector3(
			m_arrCoords[0]-rkV.m_arrCoords[0],
			m_arrCoords[1]-rkV.m_arrCoords[1],
			m_arrCoords[2]-rkV.m_arrCoords[2]);
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector3<Real> Vector3<Real>::operator* (const Vector3& rkV) const
	{
		return Vector3(
			m_arrCoords[0] * rkV.m_arrCoords[0],
			m_arrCoords[1] * rkV.m_arrCoords[1],
			m_arrCoords[2] * rkV.m_arrCoords[2]);
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector3<Real> Vector3<Real>::operator/ (const Vector3& rkV) const
	{
		return Vector3(
			m_arrCoords[0] / rkV.m_arrCoords[0],
			m_arrCoords[1] / rkV.m_arrCoords[1],
			m_arrCoords[2] / rkV.m_arrCoords[2]);
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector3<Real> Vector3<Real>::operator* (Real fScalar) const
	{
		return Vector3(
			fScalar*m_arrCoords[0],
			fScalar*m_arrCoords[1],
			fScalar*m_arrCoords[2]);
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector3<Real> Vector3<Real>::operator/ (Real fScalar) const
	{
		Vector3 kQuot;

		Real fInvScalar = ((Real)1.0)/fScalar;
		kQuot.m_arrCoords[0] = fInvScalar*m_arrCoords[0];
		kQuot.m_arrCoords[1] = fInvScalar*m_arrCoords[1];
		kQuot.m_arrCoords[2] = fInvScalar*m_arrCoords[2];

		return kQuot;
	}
	//----------------------------------------------------------------------------
	template <>
	inline Vector3<int> Vector3<int>::operator/ (int fScalar) const
	{
		Vector3 kQuot;
		
		kQuot.m_arrCoords[0] = m_arrCoords[0] / fScalar;
		kQuot.m_arrCoords[1] = m_arrCoords[1] / fScalar;
		kQuot.m_arrCoords[2] = m_arrCoords[2] / fScalar;
		
		return kQuot;
	}
	//----------------------------------------------------------------------------
	template <>
	inline Vector3<unsigned int> Vector3<unsigned int>::operator/ (unsigned int fScalar) const
	{
		Vector3 kQuot;
		
		kQuot.m_arrCoords[0] = m_arrCoords[0] / fScalar;
		kQuot.m_arrCoords[1] = m_arrCoords[1] / fScalar;
		kQuot.m_arrCoords[2] = m_arrCoords[2] / fScalar;
		
		return kQuot;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector3<Real> Vector3<Real>::operator- () const
	{
		return Vector3(
			-m_arrCoords[0],
			-m_arrCoords[1],
			-m_arrCoords[2]);
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector3<Real> operator* (Real fScalar, const Vector3<Real>& rkV)
	{
		return Vector3<Real>(
			fScalar*rkV[0],
			fScalar*rkV[1],
			fScalar*rkV[2]);
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector3<Real>& Vector3<Real>::operator+= (const Vector3& rkV)
	{
		m_arrCoords[0] += rkV.m_arrCoords[0];
		m_arrCoords[1] += rkV.m_arrCoords[1];
		m_arrCoords[2] += rkV.m_arrCoords[2];
		return *this;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector3<Real>& Vector3<Real>::operator-= (const Vector3& rkV)
	{
		m_arrCoords[0] -= rkV.m_arrCoords[0];
		m_arrCoords[1] -= rkV.m_arrCoords[1];
		m_arrCoords[2] -= rkV.m_arrCoords[2];
		return *this;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector3<Real>& Vector3<Real>::operator*= (const Vector3& rkV)
	{
		m_arrCoords[0] *= rkV.m_arrCoords[0];
		m_arrCoords[1] *= rkV.m_arrCoords[1];
		m_arrCoords[2] *= rkV.m_arrCoords[2];
		return *this;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector3<Real>& Vector3<Real>::operator/= (const Vector3& rkV)
	{
		m_arrCoords[0] /= rkV.m_arrCoords[0];
		m_arrCoords[1] /= rkV.m_arrCoords[1];
		m_arrCoords[2] /= rkV.m_arrCoords[2];
		return *this;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector3<Real>& Vector3<Real>::operator*= (Real fScalar)
	{
		m_arrCoords[0] *= fScalar;
		m_arrCoords[1] *= fScalar;
		m_arrCoords[2] *= fScalar;
		return *this;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector3<Real>& Vector3<Real>::operator/= (Real fScalar)
	{
		Real fInvScalar = ((Real)1.0)/fScalar;
		m_arrCoords[0] *= fInvScalar;
		m_arrCoords[1] *= fInvScalar;
		m_arrCoords[2] *= fInvScalar;

		return *this;
	}
	//----------------------------------------------------------------------------
	template <>
	inline Vector3<int>& Vector3<int>::operator/= (int fScalar)
	{
		m_arrCoords[0] /= fScalar;
		m_arrCoords[1] /= fScalar;
		m_arrCoords[2] /= fScalar;
		
		return *this;
	}
	//----------------------------------------------------------------------------
	template <>
	inline Vector3<unsigned int>& Vector3<unsigned int>::operator/= (unsigned int fScalar)
	{
		m_arrCoords[0] /= fScalar;
		m_arrCoords[1] /= fScalar;
		m_arrCoords[2] /= fScalar;
		
		return *this;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	void Vector3<Real>::rotateAboutX( Real degrees )
	{
		Real sin, cos;
		sinCosFromRadians( degreesToRadians( degrees ), sin, cos );

		Vector3< Real > matrix[ 3 ];
		matrix[ 0 ][ 0 ] = (Real) 1.0;	matrix[ 1 ][ 0 ] = 0;		matrix[ 2 ][ 0 ] = 0;
		matrix[ 0 ][ 1 ] = (Real) 0;	matrix[ 1 ][ 1 ] = cos;		matrix[ 2 ][ 1 ] = sin;
		matrix[ 0 ][ 2 ] = (Real) 0;	matrix[ 1 ][ 2 ] = -sin;	matrix[ 2 ][ 2 ] = cos;

		Vector3< Real > result;

		result.x = x * matrix[ 0 ][ 0 ] + y * matrix[ 0 ][ 1 ] + z * matrix[ 0 ][ 2 ];
		result.y = x * matrix[ 1 ][ 0 ] + y * matrix[ 1 ][ 1 ] + z * matrix[ 1 ][ 2 ];
		result.z = x * matrix[ 2 ][ 0 ] + y * matrix[ 2 ][ 1 ] + z * matrix[ 2 ][ 2 ];

		*this = result;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	void Vector3<Real>::rotateAboutY( Real degrees )
	{
		Real sin, cos;
		sinCosFromRadians( degreesToRadians( degrees ), sin, cos );

		Vector3< Real > matrix[ 3 ];
		matrix[ 0 ][ 0 ] = cos;		matrix[ 1 ][ 0 ] = 0;			matrix[ 2 ][ 0 ] = -sin;
		matrix[ 0 ][ 1 ] = 0;		matrix[ 1 ][ 1 ] = (Real) 1.0;	matrix[ 2 ][ 1 ] = 0;
		matrix[ 0 ][ 2 ] = sin;		matrix[ 1 ][ 2 ] = 0;			matrix[ 2 ][ 2 ] = cos;

		Vector3< Real > result;

		result.x = x * matrix[ 0 ][ 0 ] + y * matrix[ 0 ][ 1 ] + z * matrix[ 0 ][ 2 ];
		result.y = x * matrix[ 1 ][ 0 ] + y * matrix[ 1 ][ 1 ] + z * matrix[ 1 ][ 2 ];
		result.z = x * matrix[ 2 ][ 0 ] + y * matrix[ 2 ][ 1 ] + z * matrix[ 2 ][ 2 ];

		*this = result;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	void Vector3<Real>::rotateAboutZ( Real degrees )
	{
		Real sin, cos;
		sinCosFromRadians( degreesToRadians( degrees ), sin, cos );

		Vector3< Real > matrix[ 3 ];
		matrix[ 0 ][ 0 ] = cos;		matrix[ 1 ][ 0 ] = sin;		matrix[ 2 ][ 0 ] = 0;
		matrix[ 0 ][ 1 ] = -sin;	matrix[ 1 ][ 1 ] = cos;		matrix[ 2 ][ 1 ] = 0;
		matrix[ 0 ][ 2 ] = 0;		matrix[ 1 ][ 2 ] = 0;		matrix[ 2 ][ 2 ] = (Real) 1.0;

		Vector3< Real > result;

		result.x = x * matrix[ 0 ][ 0 ] + y * matrix[ 0 ][ 1 ] + z * matrix[ 0 ][ 2 ];
		result.y = x * matrix[ 1 ][ 0 ] + y * matrix[ 1 ][ 1 ] + z * matrix[ 1 ][ 2 ];
		result.z = x * matrix[ 2 ][ 0 ] + y * matrix[ 2 ][ 1 ] + z * matrix[ 2 ][ 2 ];

		*this = result;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	void Vector3<Real>::rotateByEulerDegrees( const Vector3& eulerAngles )
	{
		rotateAboutX( eulerAngles.X() );
		rotateAboutY( eulerAngles.Y() );
		rotateAboutZ( eulerAngles.Z() );
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real Vector3<Real>::length () const
	{
		return (Real) std::sqrt(
			m_arrCoords[0]*m_arrCoords[0] +
			m_arrCoords[1]*m_arrCoords[1] +
			m_arrCoords[2]*m_arrCoords[2]);
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real Vector3<Real>::lengthSquared () const
	{
		return
			m_arrCoords[0]*m_arrCoords[0] +
			m_arrCoords[1]*m_arrCoords[1] +
			m_arrCoords[2]*m_arrCoords[2];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real Vector3<Real>::dot (const Vector3& rkV) const
	{
		return
			m_arrCoords[0]*rkV.m_arrCoords[0] +
			m_arrCoords[1]*rkV.m_arrCoords[1] +
			m_arrCoords[2]*rkV.m_arrCoords[2];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real Vector3<Real>::normalize ()
	{
		Real fLength = length();

		if (fLength > std::numeric_limits< Real >::epsilon() )
		{
			Real fInvLength = ((Real)1.0)/fLength;
			m_arrCoords[0] *= fInvLength;
			m_arrCoords[1] *= fInvLength;
			m_arrCoords[2] *= fInvLength;
		}
		else
		{
			fLength = (Real)0.0;
			m_arrCoords[0] = (Real)0.0;
			m_arrCoords[1] = (Real)0.0;
			m_arrCoords[2] = (Real)0.0;
		}

		return fLength;
	}
	//----------------------------------------------------------------------------
	template <>
	inline int Vector3<int>::normalize ()
	{
		int fLength = length();
		
		if (fLength > 0 )
		{
			m_arrCoords[0] /= fLength;
			m_arrCoords[1] /= fLength;
			m_arrCoords[2] /= fLength;
		}
		else
		{
			fLength = 0;
			m_arrCoords[0] = 0;
			m_arrCoords[1] = 0;
			m_arrCoords[2] = 0;
		}
		
		return fLength;
	}
	//----------------------------------------------------------------------------
	template <>
	inline unsigned int Vector3<unsigned int>::normalize ()
	{
		unsigned int fLength = length();
		
		if (fLength > 0 )
		{
			m_arrCoords[0] /= fLength;
			m_arrCoords[1] /= fLength;
			m_arrCoords[2] /= fLength;
		}
		else
		{
			fLength = 0;
			m_arrCoords[0] = 0;
			m_arrCoords[1] = 0;
			m_arrCoords[2] = 0;
		}
		
		return fLength;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector3<Real> Vector3<Real>::normal() const
	{
		Vector3<Real> result( *this );
		result.normalize();
		return result;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector3<Real> Vector3<Real>::cross (const Vector3& rkV) const
	{
		return Vector3(
			m_arrCoords[1]*rkV.m_arrCoords[2] - m_arrCoords[2]*rkV.m_arrCoords[1],
			m_arrCoords[2]*rkV.m_arrCoords[0] - m_arrCoords[0]*rkV.m_arrCoords[2],
			m_arrCoords[0]*rkV.m_arrCoords[1] - m_arrCoords[1]*rkV.m_arrCoords[0]);
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector3<Real> Vector3<Real>::unitCross (const Vector3& rkV) const
	{
		Vector3 kCross(
			m_arrCoords[1]*rkV.m_arrCoords[2] - m_arrCoords[2]*rkV.m_arrCoords[1],
			m_arrCoords[2]*rkV.m_arrCoords[0] - m_arrCoords[0]*rkV.m_arrCoords[2],
			m_arrCoords[0]*rkV.m_arrCoords[1] - m_arrCoords[1]*rkV.m_arrCoords[0]);
		kCross.normalize();
		return kCross;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	void Vector3<Real>::orthonormalize (Vector3& rkU, Vector3& rkV, Vector3& rkW)
	{
		// If the input vectors are v0, v1, and v2, then the Gram-Schmidt
		// orthonormalization produces vectors u0, u1, and u2 as follows,
		//
		//   u0 = v0/|v0|
		//   u1 = (v1-(u0*v1)u0)/|v1-(u0*v1)u0|
		//   u2 = (v2-(u0*v2)u0-(u1*v2)u1)/|v2-(u0*v2)u0-(u1*v2)u1|
		//
		// where |A| indicates length of vector A and A*B indicates dot
		// product of vectors A and B.

		// compute u0
		rkU.normalize();

		// compute u1
		Real fDot0 = rkU.dot(rkV); 
		rkV -= fDot0*rkU;
		rkV.normalize();

		// compute u2
		Real fDot1 = rkV.dot(rkW);
		fDot0 = rkU.dot(rkW);
		rkW -= fDot0*rkU + fDot1*rkV;
		rkW.normalize();
	}
	//----------------------------------------------------------------------------
	template <class Real>
	void Vector3<Real>::orthonormalize (Vector3* akV)
	{
		orthonormalize(akV[0],akV[1],akV[2]);
	}
	//----------------------------------------------------------------------------
	template <class Real>
	void Vector3<Real>::computeExtremes (int iVQuantity, const Vector3* akPoint,
		Vector3& rkMin, Vector3& rkMax)
	{
		ASSERT(iVQuantity > 0 && akPoint);

		rkMin = akPoint[0];
		rkMax = rkMin;
		for (int i = 1; i < iVQuantity; i++)
		{
			const Vector3<Real>& rkPoint = akPoint[i];
			for (int j = 0; j < 3; j++)
			{
				if (rkPoint[j] < rkMin[j])
				{
					rkMin[j] = rkPoint[j];
				}
				else if (rkPoint[j] > rkMax[j])
				{
					rkMax[j] = rkPoint[j];
				}
			}
		}
	}
	
}

