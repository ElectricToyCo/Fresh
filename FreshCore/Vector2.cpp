#include "Vector2.h"

namespace fr
{
	template<> const Vector2<int> Vector2<int>::ZERO(   0, 0 );
	template<> const Vector2<int> Vector2<int>::UNIT_X( 1, 0 );
	template<> const Vector2<int> Vector2<int>::UNIT_Y( 0, 1 );

	template<> const Vector2<unsigned int> Vector2<unsigned int>::ZERO(   0, 0 );
	template<> const Vector2<unsigned int> Vector2<unsigned int>::UNIT_X( 1, 0 );
	template<> const Vector2<unsigned int> Vector2<unsigned int>::UNIT_Y( 0, 1 );

	template<> const Vector2<float> Vector2<float>::ZERO(   0.0f, 0.0f );
	template<> const Vector2<float> Vector2<float>::UNIT_X( 1.0f, 0.0f );
	template<> const Vector2<float> Vector2<float>::UNIT_Y( 0.0f, 1.0f );

	template<> const Vector2<double> Vector2<double>::ZERO(   0.0, 0.0 );
	template<> const Vector2<double> Vector2<double>::UNIT_X( 1.0, 0.0 );
	template<> const Vector2<double> Vector2<double>::UNIT_Y( 0.0, 1.0 );

}
