//
//  ApiMath.cpp
//  Fresh
//
//  Created by Jeff Wofford on 8/26/18.
//  Copyright (c) 2018 Jeff Wofford. All rights reserved.
//

#include "FantasyConsole.h"
#include "FreshVector.h"

namespace fr
{
    LUA_FUNCTION( vec, 0 )
    std::map< std::string, real > FantasyConsole::vec( real x, real y )
    {
        DEFAULT( x, 0 )
        DEFAULT( y, x )
        
        // TODO: metatable
        
        return {{ "x", x }, { "y", y }};
    }
    
    void FantasyConsole::setupMath( lua_State* lua )
    {
        REQUIRES( lua );
        
    }
}

