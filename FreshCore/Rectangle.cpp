//
//  Rectangle.cpp
//  Fresh
//
//  Created by Jeff Wofford on 2/23/13.
//
//

#include "Rectangle.h"

namespace fr
{
	template<> const Rectangle< int >			Rectangle< int >::INVERSE_INFINITE( Rectangle< int >::infinity(), Rectangle< int >::infinity(), Rectangle< int >::negativeInfinity(), Rectangle< int >::negativeInfinity() );
	template<> const Rectangle< unsigned int >  Rectangle< unsigned int >::INVERSE_INFINITE( Rectangle< unsigned int >::infinity(), Rectangle< unsigned int >::infinity(), Rectangle< unsigned int >::negativeInfinity(), Rectangle< unsigned int >::negativeInfinity() );
	template<> const Rectangle< float >			Rectangle< float >::INVERSE_INFINITE( Rectangle< float >::infinity(), Rectangle< float >::infinity(), Rectangle< float >::negativeInfinity(), Rectangle< float >::negativeInfinity() );
	template<> const Rectangle< double >		Rectangle< double >::INVERSE_INFINITE( Rectangle< double >::infinity(), Rectangle< double >::infinity(), Rectangle< double >::negativeInfinity(), Rectangle< double >::negativeInfinity() );
}
