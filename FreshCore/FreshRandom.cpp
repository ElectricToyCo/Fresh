//
//  FreshRandom.cpp
//  Fresh
//
//  Created by Jeff Wofford on 12/11/13.
//
//

#include "FreshRandom.h"
#include <chrono>

namespace
{
	std::vector< fr::Random > g_engines = { fr::Random( 1 ) };
	
	inline unsigned int now()
	{
		return (unsigned int) std::chrono::system_clock::now().time_since_epoch().count();
	}
}

namespace fr
{
	
	Random createRandomGenerator( unsigned int seed )
	{
		return Random{ seed };
	}
	
	Random createRandomGenerator()
	{
		return createRandomGenerator( now() );
	}
	
	Random& currentRandomGenerator()
	{
		ASSERT( !g_engines.empty() );
		return g_engines.back();
	}
	
	void pushRandomGeneratorRandomized()
	{
		g_engines.emplace_back( now() );
	}
	
	void pushRandomGenerator( unsigned int seed )
	{
		g_engines.emplace_back( seed );
	}

	void pushRandomGenerator( const Random& generator )
	{
		g_engines.push_back( generator );
	}
	
	Random popRandomGenerator()
	{
		ASSERT( !g_engines.empty() );
		Random back = g_engines.back();
		g_engines.pop_back();
		return back;
		ASSERT( !g_engines.empty() );
	}
	
}
