#include "Vector4.h"

namespace fr
{
	
	template<> const Vector4<int> Vector4<int>::ZERO(0,0,0,0);
	template<> const Vector4<int> Vector4<int>::UNIT_X(1,0,0,0);
	template<> const Vector4<int> Vector4<int>::UNIT_Y(0,1,0,0);
	template<> const Vector4<int> Vector4<int>::UNIT_Z(0,0,1,0);
	template<> const Vector4<int> Vector4<int>::UNIT_W(0,0,0,1);

	template<> const Vector4<unsigned int> Vector4<unsigned int>::ZERO(0,0,0,0);
	template<> const Vector4<unsigned int> Vector4<unsigned int>::UNIT_X(1,0,0,0);
	template<> const Vector4<unsigned int> Vector4<unsigned int>::UNIT_Y(0,1,0,0);
	template<> const Vector4<unsigned int> Vector4<unsigned int>::UNIT_Z(0,0,1,0);
	template<> const Vector4<unsigned int> Vector4<unsigned int>::UNIT_W(0,0,0,1);

	template<> const Vector4<float> Vector4<float>::ZERO(0.0f,0.0f,0.0f,0.0f);
	template<> const Vector4<float> Vector4<float>::UNIT_X(1.0f,0.0f,0.0f,0.0f);
	template<> const Vector4<float> Vector4<float>::UNIT_Y(0.0f,1.0f,0.0f,0.0f);
	template<> const Vector4<float> Vector4<float>::UNIT_Z(0.0f,0.0f,1.0f,0.0f);
	template<> const Vector4<float> Vector4<float>::UNIT_W(0.0f,0.0f,0.0f,1.0f);

	template<> const Vector4<double> Vector4<double>::ZERO(0.0,0.0,0.0,0.0);
	template<> const Vector4<double> Vector4<double>::UNIT_X(1.0,0.0,0.0,0.0);
	template<> const Vector4<double> Vector4<double>::UNIT_Y(0.0,1.0,0.0,0.0);
	template<> const Vector4<double> Vector4<double>::UNIT_Z(0.0,0.0,1.0,0.0);
	template<> const Vector4<double> Vector4<double>::UNIT_W(0.0,0.0,0.0,1.0);
	
}
