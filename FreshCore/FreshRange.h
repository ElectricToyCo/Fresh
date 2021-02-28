//
//  FreshRange.h
//  Fresh
//
//  Created by Jeff Wofford on 7/8/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_FreshRange_h
#define Fresh_FreshRange_h

#include "SerializeVector.h"

namespace fr
{

	/////////////////////////////////////////////////////////////////////////////////
	// RANGE: Inclusive range. min may or may not be <= max.

	template< typename T >
	class Range
	{
	public:
		typedef T element_type;
		const static size_t nComponents = 2;
		
		T min, max;
		
		Range() : min( 0 ), max( 0 ) {}
		Range( const T& a, const T& b ) : min( a ), max( b ) {}
		
		void set( const T& a, const T& b ) { min = a; max = b; }
		T delta() const { return max - min; }
		T midpoint() const { return ( min + max ) * 0.5f; }
		
		bool wellFormed() const { return min <= max; }
		bool encompasses( const T& x ) const { return min <= x && x <= max; }
		
		const T& operator[]( size_t i ) const { return i == 0 ? min : max; }
		T& operator[]( size_t i ) { return i == 0 ? min : max; }
		
		bool operator==( const Range& other ) const { return min == other.min && max == other.max; }
	};

	template< typename T >
	Range< T > makeRange( T min, T max )
	{
		ASSERT( min <= max );
		return Range< T >( min, max );
	}

	template< typename Real >
	std::ostream& operator<<( std::ostream& out, const Range< Real >& range )
	{
		SerializeVector::write( out, range );
		return out;
	}
	
	template< typename Real >
	std::istream& operator>>( std::istream& in, Range< Real >& range )
	{
		return SerializeVector::read( in, range );
	}
}

#endif
