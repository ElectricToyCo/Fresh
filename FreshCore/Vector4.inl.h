#include "FreshDebug.h"
#include "SerializeVector.h"
#include <cmath>

namespace fr
{
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real>::Vector4()
	{
		m_arrCoords[0] = m_arrCoords[1] = m_arrCoords[2] = m_arrCoords[3] = 0;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real>::Vector4 (Real f)
	{
		m_arrCoords[0] = m_arrCoords[1] = m_arrCoords[2] = m_arrCoords[3] = f;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real>::Vector4 (Real fX, Real fY, Real fZ, Real fW)
	{
		m_arrCoords[0] = fX;
		m_arrCoords[1] = fY;
		m_arrCoords[2] = fZ;
		m_arrCoords[3] = fW;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real>::Vector4 (const Real* afTuple)
	{
		m_arrCoords[0] = afTuple[0];
		m_arrCoords[1] = afTuple[1];
		m_arrCoords[2] = afTuple[2];
		m_arrCoords[3] = afTuple[3];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real>::Vector4 (const Vector4& rkV)
	{
		m_arrCoords[0] = rkV.m_arrCoords[0];
		m_arrCoords[1] = rkV.m_arrCoords[1];
		m_arrCoords[2] = rkV.m_arrCoords[2];
		m_arrCoords[3] = rkV.m_arrCoords[3];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real>::Vector4 (const Vector3< Real >& vec3, Real fW )
	{
		m_arrCoords[0] = vec3.X();
		m_arrCoords[1] = vec3.Y();
		m_arrCoords[2] = vec3.Z();
		m_arrCoords[3] = fW;
	}

	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real>::operator const Real* () const
	{
		return m_arrCoords;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real>::operator Real* ()
	{
		return m_arrCoords;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real Vector4<Real>::operator[] (size_t i) const
	{
		ASSERT(0 <= i && i <= 3);
		return m_arrCoords[i];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real& Vector4<Real>::operator[] (size_t i)
	{
		ASSERT(0 <= i && i <= 3);
		return m_arrCoords[i];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real Vector4<Real>::X () const
	{
		return m_arrCoords[0];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real& Vector4<Real>::X ()
	{
		return m_arrCoords[0];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real Vector4<Real>::Y () const
	{
		return m_arrCoords[1];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real& Vector4<Real>::Y ()
	{
		return m_arrCoords[1];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real Vector4<Real>::Z () const
	{
		return m_arrCoords[2];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real& Vector4<Real>::Z ()
	{
		return m_arrCoords[2];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real Vector4<Real>::W () const
	{
		return m_arrCoords[3];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real& Vector4<Real>::W ()
	{
		return m_arrCoords[3];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real>& Vector4<Real>::operator= (const Vector4& rkV)
	{
		m_arrCoords[0] = rkV.m_arrCoords[0];
		m_arrCoords[1] = rkV.m_arrCoords[1];
		m_arrCoords[2] = rkV.m_arrCoords[2];
		m_arrCoords[3] = rkV.m_arrCoords[3];
		return *this;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real>& Vector4<Real>::operator= (const Vector3< Real >& rkV)
	{
		m_arrCoords[0] = rkV.X();
		m_arrCoords[1] = rkV.Y();
		m_arrCoords[2] = rkV.Z();
		m_arrCoords[3] = (Real)1.0;
		return *this;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	int Vector4<Real>::compareArrays (const Vector4& rkV) const
	{
		return memcmp(m_arrCoords,rkV.m_arrCoords,4*sizeof(Real));
	}
	//----------------------------------------------------------------------------
	template <class Real>
	bool Vector4<Real>::operator== (const Vector4& rkV) const
	{
		return compareArrays(rkV) == 0;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	bool Vector4<Real>::operator!= (const Vector4& rkV) const
	{
		return compareArrays(rkV) != 0;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	bool Vector4<Real>::operator< (const Vector4& rkV) const
	{
		return compareArrays(rkV) < 0;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	bool Vector4<Real>::operator<= (const Vector4& rkV) const
	{
		return compareArrays(rkV) <= 0;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	bool Vector4<Real>::operator> (const Vector4& rkV) const
	{
		return compareArrays(rkV) > 0;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	bool Vector4<Real>::operator>= (const Vector4& rkV) const
	{
		return compareArrays(rkV) >= 0;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real> Vector4<Real>::operator+ (const Vector4& rkV) const
	{
		return Vector4(
			m_arrCoords[0]+rkV.m_arrCoords[0],
			m_arrCoords[1]+rkV.m_arrCoords[1],
			m_arrCoords[2]+rkV.m_arrCoords[2],
			m_arrCoords[3]+rkV.m_arrCoords[3]);
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real> Vector4<Real>::operator- (const Vector4& rkV) const
	{
		return Vector4(
			m_arrCoords[0]-rkV.m_arrCoords[0],
			m_arrCoords[1]-rkV.m_arrCoords[1],
			m_arrCoords[2]-rkV.m_arrCoords[2],
			m_arrCoords[3]-rkV.m_arrCoords[3]);
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real> Vector4<Real>::operator* (const Vector4& rkV) const
	{
		return Vector4(
					   m_arrCoords[0] * rkV.m_arrCoords[0],
					   m_arrCoords[1] * rkV.m_arrCoords[1],
					   m_arrCoords[2] * rkV.m_arrCoords[2],
					   m_arrCoords[3] * rkV.m_arrCoords[3]);
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real> Vector4<Real>::operator/ (const Vector4& rkV) const
	{
		return Vector4(
					   m_arrCoords[0] / rkV.m_arrCoords[0],
					   m_arrCoords[1] / rkV.m_arrCoords[1],
					   m_arrCoords[2] / rkV.m_arrCoords[2],
					   m_arrCoords[3] / rkV.m_arrCoords[3]);
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real> Vector4<Real>::operator+ (Real fScalar) const
	{
		return Vector4(
					   fScalar + m_arrCoords[0],
					   fScalar + m_arrCoords[1],
					   fScalar + m_arrCoords[2],
					   fScalar + m_arrCoords[3]);
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real> Vector4<Real>::operator- (Real fScalar) const
	{
		return Vector4(
					   fScalar - m_arrCoords[0],
					   fScalar - m_arrCoords[1],
					   fScalar - m_arrCoords[2],
					   fScalar - m_arrCoords[3]);
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real> Vector4<Real>::operator* (Real fScalar) const
	{
		return Vector4(
			fScalar*m_arrCoords[0],
			fScalar*m_arrCoords[1],
			fScalar*m_arrCoords[2],
			fScalar*m_arrCoords[3]);
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real> Vector4<Real>::operator/ (Real fScalar) const
	{
		Vector4 kQuot;

		if (fScalar != (Real)0.0)
		{
			Real fInvScalar = ((Real)1.0)/fScalar;
			kQuot.m_arrCoords[0] = fInvScalar*m_arrCoords[0];
			kQuot.m_arrCoords[1] = fInvScalar*m_arrCoords[1];
			kQuot.m_arrCoords[2] = fInvScalar*m_arrCoords[2];
			kQuot.m_arrCoords[3] = fInvScalar*m_arrCoords[3];
		}
		else
		{
			kQuot.m_arrCoords[0] = std::numeric_limits< Real >::max();
			kQuot.m_arrCoords[1] = std::numeric_limits< Real >::max();
			kQuot.m_arrCoords[2] = std::numeric_limits< Real >::max();
			kQuot.m_arrCoords[3] = std::numeric_limits< Real >::max();
		}

		return kQuot;
	}
	//----------------------------------------------------------------------------
	template <>
	inline Vector4<int> Vector4<int>::operator/ (int fScalar) const
	{
		Vector4 kQuot;
		
		kQuot.m_arrCoords[0] = m_arrCoords[0] / fScalar;
		kQuot.m_arrCoords[1] = m_arrCoords[1] / fScalar;
		kQuot.m_arrCoords[2] = m_arrCoords[2] / fScalar;
		kQuot.m_arrCoords[3] = m_arrCoords[3] / fScalar;
		
		return kQuot;
	}
	//----------------------------------------------------------------------------
	template <>
	inline Vector4<unsigned int> Vector4<unsigned int>::operator/ (unsigned int fScalar) const
	{
		Vector4 kQuot;
		
		kQuot.m_arrCoords[0] = m_arrCoords[0] / fScalar;
		kQuot.m_arrCoords[1] = m_arrCoords[1] / fScalar;
		kQuot.m_arrCoords[2] = m_arrCoords[2] / fScalar;
		kQuot.m_arrCoords[3] = m_arrCoords[3] / fScalar;
		
		return kQuot;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real> Vector4<Real>::operator- () const
	{
		return Vector4(
			-m_arrCoords[0],
			-m_arrCoords[1],
			-m_arrCoords[2],
			-m_arrCoords[3]);
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real> operator* (Real fScalar, const Vector4<Real>& rkV)
	{
		return Vector4<Real>(
			fScalar*rkV[0],
			fScalar*rkV[1],
			fScalar*rkV[2],
			fScalar*rkV[3]);
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real>& Vector4<Real>::operator+= (const Vector4& rkV)
	{
		m_arrCoords[0] += rkV.m_arrCoords[0];
		m_arrCoords[1] += rkV.m_arrCoords[1];
		m_arrCoords[2] += rkV.m_arrCoords[2];
		m_arrCoords[3] += rkV.m_arrCoords[3];
		return *this;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real>& Vector4<Real>::operator-= (const Vector4& rkV)
	{
		m_arrCoords[0] -= rkV.m_arrCoords[0];
		m_arrCoords[1] -= rkV.m_arrCoords[1];
		m_arrCoords[2] -= rkV.m_arrCoords[2];
		m_arrCoords[3] -= rkV.m_arrCoords[3];
		return *this;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real>& Vector4<Real>::operator*= (const Vector4& rkV)
	{
		m_arrCoords[0] *= rkV.m_arrCoords[0];
		m_arrCoords[1] *= rkV.m_arrCoords[1];
		m_arrCoords[2] *= rkV.m_arrCoords[2];
		m_arrCoords[3] *= rkV.m_arrCoords[3];
		return *this;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real>& Vector4<Real>::operator/= (const Vector4& rkV)
	{
		m_arrCoords[0] /= rkV.m_arrCoords[0];
		m_arrCoords[1] /= rkV.m_arrCoords[1];
		m_arrCoords[2] /= rkV.m_arrCoords[2];
		m_arrCoords[3] /= rkV.m_arrCoords[3];
		return *this;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real>& Vector4<Real>::operator+= (Real fScalar)
	{
		m_arrCoords[0] += fScalar;
		m_arrCoords[1] += fScalar;
		m_arrCoords[2] += fScalar;
		m_arrCoords[3] += fScalar;
		return *this;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real>& Vector4<Real>::operator-= (Real fScalar)
	{
		m_arrCoords[0] -= fScalar;
		m_arrCoords[1] -= fScalar;
		m_arrCoords[2] -= fScalar;
		m_arrCoords[3] -= fScalar;
		return *this;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real>& Vector4<Real>::operator*= (Real fScalar)
	{
		m_arrCoords[0] *= fScalar;
		m_arrCoords[1] *= fScalar;
		m_arrCoords[2] *= fScalar;
		m_arrCoords[3] *= fScalar;
		return *this;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real>& Vector4<Real>::operator/= (Real fScalar)
	{
		if (fScalar != (Real)0.0)
		{
			Real fInvScalar = ((Real)1.0)/fScalar;
			m_arrCoords[0] *= fInvScalar;
			m_arrCoords[1] *= fInvScalar;
			m_arrCoords[2] *= fInvScalar;
			m_arrCoords[3] *= fInvScalar;
		}
		else
		{
			m_arrCoords[0] = std::numeric_limits< Real >::max();
			m_arrCoords[1] = std::numeric_limits< Real >::max();
			m_arrCoords[2] = std::numeric_limits< Real >::max();
			m_arrCoords[3] = std::numeric_limits< Real >::max();
		}

		return *this;
	}
	//----------------------------------------------------------------------------
	template <>
	inline Vector4<int>& Vector4<int>::operator/= (int fScalar)
	{
		m_arrCoords[0] /= fScalar;
		m_arrCoords[1] /= fScalar;
		m_arrCoords[2] /= fScalar;
		m_arrCoords[3] /= fScalar;
		
		return *this;
	}
	//----------------------------------------------------------------------------
	template <>
	inline Vector4<unsigned int>& Vector4<unsigned int>::operator/= (unsigned int fScalar)
	{
		m_arrCoords[0] /= fScalar;
		m_arrCoords[1] /= fScalar;
		m_arrCoords[2] /= fScalar;
		m_arrCoords[3] /= fScalar;
		
		return *this;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real Vector4<Real>::length() const
	{
		return (Real) std::sqrt(
			m_arrCoords[0]*m_arrCoords[0] +
			m_arrCoords[1]*m_arrCoords[1] +
			m_arrCoords[2]*m_arrCoords[2] +
			m_arrCoords[3]*m_arrCoords[3]);
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real Vector4<Real>::lengthSquared () const
	{
		return
			m_arrCoords[0]*m_arrCoords[0] +
			m_arrCoords[1]*m_arrCoords[1] +
			m_arrCoords[2]*m_arrCoords[2] +
			m_arrCoords[3]*m_arrCoords[3];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real Vector4<Real>::dot (const Vector4& rkV) const
	{
		return
			m_arrCoords[0]*rkV.m_arrCoords[0] +
			m_arrCoords[1]*rkV.m_arrCoords[1] +
			m_arrCoords[2]*rkV.m_arrCoords[2] +
			m_arrCoords[3]*rkV.m_arrCoords[3];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real Vector4<Real>::dotAs2 (const Vector4& rkV) const
	{
		return
			m_arrCoords[0]*rkV.m_arrCoords[0] +
			m_arrCoords[1]*rkV.m_arrCoords[1];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real Vector4<Real>::dotAs3 (const Vector4& rkV) const
	{
		return
			m_arrCoords[0]*rkV.m_arrCoords[0] +
			m_arrCoords[1]*rkV.m_arrCoords[1] +
			m_arrCoords[2]*rkV.m_arrCoords[2];
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Real Vector4<Real>::normalize ()
	{
		Real fLength = length();

		if (fLength > std::numeric_limits< Real >::epsilon())
		{
			Real fInvLength = ((Real)1.0)/fLength;
			*this *= fInvLength;
		}
		else
		{
			fLength = (Real)0.0;
			setToZero();
		}

		return fLength;
	}
	//----------------------------------------------------------------------------
	template <>
	inline int Vector4<int>::normalize ()
	{
		int fLength = length();
		
		if (fLength > 0)
		{
			*this /= fLength;
		}
		else
		{
			fLength = 0;
			setToZero();
		}
		
		return fLength;
	}
	//----------------------------------------------------------------------------
	template <>
	inline unsigned int Vector4<unsigned int>::normalize ()
	{
		unsigned int fLength = length();
		
		if (fLength > 0)
		{
			*this /= fLength;
		}
		else
		{
			fLength = 0;
			setToZero();
		}
		
		return fLength;
	}
	//----------------------------------------------------------------------------
	template <class Real>
	Vector4<Real> Vector4<Real>::crossAsVec3(const Vector4& rkV) const
	{
		return Vector4(
			m_arrCoords[1]*rkV.m_arrCoords[2] - m_arrCoords[2]*rkV.m_arrCoords[1],
			m_arrCoords[2]*rkV.m_arrCoords[0] - m_arrCoords[0]*rkV.m_arrCoords[2],
			m_arrCoords[0]*rkV.m_arrCoords[1] - m_arrCoords[1]*rkV.m_arrCoords[0],
			1.0f);
	}
	//----------------------------------------------------------------------------


}
