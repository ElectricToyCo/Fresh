#ifndef FRESH_MATRIX4_H_INCLUDED
#define FRESH_MATRIX4_H_INCLUDED

// Adapted from WildMagic3

//#include "Vector4.h"
#include <string>

namespace fr
{

	template< class Real >
	class Vector3;

	template< class Real >
	class Vector4;
	
	template <class Real>
	class Matrix4
	{
	public:
		
		struct ConstructZero {};
		
		Matrix4();	// Constructs identity matrix.
		explicit Matrix4( ConstructZero );	// Constructs zero matrix.

		// copy constructor
		Matrix4 (const Matrix4& rkM);

		// input Mrc is in row r, column c.
		Matrix4 (Real fM00, Real fM01, Real fM02, Real fM03,
				 Real fM10, Real fM11, Real fM12, Real fM13,
				 Real fM20, Real fM21, Real fM22, Real fM23,
				 Real fM30, Real fM31, Real fM32, Real fM33);

		// Create a matrix from an array of numbers.  The input array is
		// interpreted based on the Boolean input as
		//   true:  entry[0..15]={m00,m01,m02,m03,m10,m11,m12,m13,m20,m21,m22,
		//                        m23,m30,m31,m32,m33} [row major]
		//   false: entry[0..15]={m00,m10,m20,m30,m01,m11,m21,m31,m02,m12,m22,
		//                        m32,m03,m13,m23,m33} [col major]
		Matrix4 (const Real afEntry[16], bool bRowMajor);

		void makeZero();
		void makeIdentity();

		void set(Real fM00, Real fM01, Real fM02, Real fM03,
				 Real fM10, Real fM11, Real fM12, Real fM13,
				 Real fM20, Real fM21, Real fM22, Real fM23,
				 Real fM30, Real fM31, Real fM32, Real fM33);

		void set(const Real afEntry[16], bool bRowMajor);

		// member access
		operator const Real* () const;
		operator Real* ();
		const Real* operator[] (int iRow) const;
		Real* operator[] (int iRow);
		Real operator() (int iRow, int iCol) const;
		Real& operator() (int iRow, int iCol);
		void setRow (int iRow, const Vector4<Real>& rkV);
		Vector4<Real> getRow (int iRow) const;
		void setColumn (int iCol, const Vector4<Real>& rkV);
		Vector4<Real> getColumn (int iCol) const;
		void getColumnMajor (Real* afCMajor) const;

		// assignment
		Matrix4& operator= (const Matrix4& rkM);

		// comparison
		bool operator== (const Matrix4& rkM) const;
		bool operator!= (const Matrix4& rkM) const;
		bool operator<  (const Matrix4& rkM) const;
		bool operator<= (const Matrix4& rkM) const;
		bool operator>  (const Matrix4& rkM) const;
		bool operator>= (const Matrix4& rkM) const;

		// arithmetic operations
		Matrix4 operator+ (const Matrix4& rkM) const;
		Matrix4 operator- (const Matrix4& rkM) const;
		Matrix4 operator* (const Matrix4& rkM) const;
		Matrix4 operator* (Real fScalar) const;
		Matrix4 operator/ (Real fScalar) const;
		Matrix4 operator- () const;

		
		// arithmetic updates
		Matrix4& operator+= (const Matrix4& rkM);
		Matrix4& operator-= (const Matrix4& rkM);
		Matrix4& operator*= (const Matrix4& rkM);
		Matrix4& operator*= (Real fScalar);
		Matrix4& operator/= (Real fScalar);

		// matrix times vector
		Vector4<Real> operator* (const Vector4<Real>& rkV) const;  // M * v
		Vector3<Real> operator* (const Vector3<Real>& rkV) const;

		// other operations
		Matrix4 getTranspose () const;  // M^T
		Matrix4 transposeTimes (const Matrix4& rkM) const;  // this^T * M
		Matrix4 timesTranspose (const Matrix4& rkM) const;  // this * M^T
		Matrix4 getInverse () const;
		Matrix4 adjoint () const;
		Real determinant () const;
		Real qForm (const Vector4<Real>& rkU,
			const Vector4<Real>& rkV) const;  // u^T*M*v

		// projection matrices onto a specified plane
		void makeObliqueProjection (const Vector3<Real>& rkNormal,
			const Vector3<Real>& rkPoint, const Vector3<Real>& rkDirection);
		void makePerspectiveProjection (const Vector3<Real>& rkNormal,
			const Vector3<Real>& rkPoint, const Vector3<Real>& rkEye);

		// reflection matrix through a specified plane
		void makeReflection (const Vector3<Real>& rkNormal,
			const Vector3<Real>& rkPoint);

		void makeOrthoProjection( Real left, Real right, Real bottom, Real top, Real near, Real far );
		void makePerspectiveProjection( Real radianFOV, Real aspectRatio, Real nearPlaneDist, Real farPlaneDist );
			// REQUIRES( 0 < radianFOV && radianFOV < PI );
			// REQUIRES( 0 < aspectRatio );
			// REQUIRES( 0 < nearPlaneDist );
			// REQUIRES( 0 < farPlaneDist );
			// REQUIRES( nearPlaneDist < farPlaneDist );
		void makeTranslation( const Vector3<Real>& translation );
		void makeScale( const Vector3<Real>& scale );
		void makeRotation( Real fYAngle, const Vector3<Real>& axis );
		void makeShearX( Real angle );
		void makeShearY( Real angle );
		Matrix4& fromEulerAnglesXYZ (Real fYAngle, Real fPAngle, Real fRAngle);
		void makeLookAt( const Vector3<Real>& cameraPosition, const Vector3<Real>& focalPoint, const Vector3<Real>& up );

		std::string toString() const;

		// special matrices
		static const Matrix4 ZERO;
		static const Matrix4 IDENTITY;

	private:
		// for indexing into the 1D array of the matrix, iCol+N*iRow
		constexpr static int I ( int iRow, int iCol );

		// support for comparisons
		int compareArrays (const Matrix4& rkM) const;

		Real m_afEntry[16];
	};

	// c * M
	template <class Real>
	Matrix4<Real> operator* (Real fScalar, const Matrix4<Real>& rkM);

	// v^T * M
	template <class Real>
	Vector4<Real> operator* (const Vector4<Real>& rkV, const Matrix4<Real>& rkM);

	
	template <class Real>
	std::ostream& operator<< (std::ostream& rkOStr, const Matrix4<Real>& rkV);
	
	template <class Real>
	std::istream& operator>> (std::istream& rkOStr, Matrix4<Real>& rkV);
	

	
	typedef Matrix4<float> Matrix4f;
	typedef Matrix4<double> Matrix4d;

}

#include "Matrix4.inl.h"

#endif
