//
//  FreshRandom.h
//  Fresh
//
//  Created by Jeff Wofford on 12/11/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_FreshRandom_h
#define Fresh_FreshRandom_h

#include "FreshRange.h"
#include <random>
#include <algorithm>    // For std::random_shuffle()

namespace fr
{
	typedef std::default_random_engine Random;
	
	// Creating random generators.
	//
	Random createRandomGenerator( unsigned int seed );
	Random createRandomGenerator();		// Randomized to time.
	
	// The random generator stack.
	// All randInRange()-style functions (below) use the top generator on this stack.
	//
	Random& currentRandomGenerator();
	void pushRandomGeneratorRandomized();					// Randomized to time.
	void pushRandomGenerator( unsigned int seed );			// Adds a new generator.
	void pushRandomGenerator( const Random& generator );	// Adds a copy of an existing generator.
	Random popRandomGenerator();
	
	inline bool pctChance( float percentChanceOfSuccess )
	{
		static std::uniform_real_distribution< float > distribution( 0.0, 100.0f );
		return distribution( currentRandomGenerator() ) < percentChanceOfSuccess;
	}
	
	template< typename T >
	inline T randInRange( T min, T max )		// max is inclusive for integers, exclusive for reals.
	{
		std::uniform_int_distribution< T > distribution( min, max );
		return distribution( currentRandomGenerator() );
	}
	
	template<>
	inline float randInRange( float min, float maxExclusive )
	{
		std::uniform_real_distribution< float > distribution( min, maxExclusive );
		return distribution( currentRandomGenerator() );
	}
	
	template<>
	inline double randInRange( double min, double maxExclusive )
	{
		std::uniform_real_distribution< double > distribution( min, maxExclusive );
		return distribution( currentRandomGenerator() );
	}
	
	template< typename T >
	inline T randInRange( const fr::Range< T >& range )
	{
		return randInRange( range.min, range.max );
	}
	
	inline bool randomBool()
	{
		return randInRange( 0, 1 ) != 0;
	}
	
	inline int randomSign()
	{
		return randomBool() ? 1 : -1;
	}
	
	template< typename T >
	inline T randomInt( T maxExclusive )
	{
		ASSERT( maxExclusive > T(0) );
		return randInRange( T(0), maxExclusive - T(1) );
	}
	
	template< typename IterT >
	inline void randomShuffle( IterT begin, IterT end )
	{
		std::shuffle( begin, end, currentRandomGenerator() );
	}
	
	template< typename T, typename TPower >
	inline T curvedRandInSet( T maxInclusive, TPower power )
	{
		static_assert( std::is_floating_point< TPower >::value, "Exponent should be float or double." );
		const T result = static_cast< T >( std::floor( std::pow( randInRange( TPower( 0 ), TPower( 1 )), power ) * ( maxInclusive + T( 1 ))));
		
		ASSERT( 0 <= result && result <= maxInclusive );
		return result;
	}
	
	template< typename RangeT >
	const typename RangeT::value_type& randomElement( const RangeT& range )
	{
		auto first = std::begin( range );
		const size_t nElements = std::distance( first, std::end( range ) );
		
		if( nElements == 0 )
		{
			FRESH_THROW( fr::FreshException, "randomElement() cannot reference empty container." );
		}
		else
		{
			const auto which = randInRange( size_t( 0 ), nElements - 1 );
			std::advance( first, which );
			return *first;
		}
	}
}

#endif
