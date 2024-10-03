//
//  ApiNav.cpp
//  Fresh
//
//  Created by Jeff Wofford on 10/11/19.
//  Copyright (c) 2019 Jeff Wofford. All rights reserved.
//

#include "FantasyConsole.h"
#include "FreshVector.h"

namespace fr
{	
	LUA_FUNCTION( navdest, 5 )
	std::tuple< bool, real, real > FantasyConsole::navdest( real desiredDestinationX, real desiredDestinationY, real radius, real originX, real originY, int layer )
	{
        DEFAULT( layer, 0 );
        
		return { false, 0, 0 };
	}

	LUA_FUNCTION( nav, 3 )
	std::vector< std::tuple< real, real >> FantasyConsole::nav( real fromX, real fromY, real toX, real toY, real actorRadius, bool smooth, int layer )
	{
        DEFAULT( smooth, false );
        DEFAULT( layer, 0 );
        
		return {};
	}
}
