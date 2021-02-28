#include "Matrix4.h"


namespace fr
{
	
	template<> const Matrix4<float> Matrix4<float>::ZERO(
		0.0f,0.0f,0.0f,0.0f,
		0.0f,0.0f,0.0f,0.0f,
		0.0f,0.0f,0.0f,0.0f,
		0.0f,0.0f,0.0f,0.0f);
	template<> const Matrix4<float> Matrix4<float>::IDENTITY(
		1.0f,0.0f,0.0f,0.0f,
		0.0f,1.0f,0.0f,0.0f,
		0.0f,0.0f,1.0f,0.0f,
		0.0f,0.0f,0.0f,1.0f);

	template<> const Matrix4<double> Matrix4<double>::ZERO(
		0.0,0.0,0.0,0.0,
		0.0,0.0,0.0,0.0,
		0.0,0.0,0.0,0.0,
		0.0,0.0,0.0,0.0);
	template<> const Matrix4<double> Matrix4<double>::IDENTITY(
		1.0,0.0,0.0,0.0,
		0.0,1.0,0.0,0.0,
		0.0,0.0,1.0,0.0,
		0.0,0.0,0.0,1.0);
	
}
