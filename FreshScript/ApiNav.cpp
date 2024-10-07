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
    LUA_FUNCTION( ssolid, 1 )
    void FantasyConsole::ssolid( int sprite, bool solid )
    {
        DEFAULT( solid, true );
        
        if( !m_tileGrid || static_cast< size_t >( sprite ) >= m_tileGrid->numTileTemplates() )
        {
            return;
        }
        
        m_tileGrid->tileTemplate( sprite )->isSolid( solid );
    }

	LUA_FUNCTION( navdest, 5 )
	std::tuple< bool, real, real > FantasyConsole::navdest( real desiredDestinationX, real desiredDestinationY, real radius, real originX, real originY )
	{
        if( !m_tileGrid ) return { false, 0, 0 };
        
        vec2 destination;
        bool found = m_tileGrid->findValidDestination( destination, vec2( desiredDestinationX, desiredDestinationY), radius, vec2( originX, originY ));
        
        return { found, destination.x, destination.y };
	}

	LUA_FUNCTION( nav, 4 )
	std::vector< std::tuple< real, real >> FantasyConsole::nav( real fromX, real fromY, real toX, real toY, real actorRadius, bool smooth )
	{
        if( !m_tileGrid ) return {};
        
        DEFAULT( actorRadius, 1.0f );
        DEFAULT( smooth, false );
        
        const auto from = vec2( fromX, fromY );
        
        FreshTileGrid::WorldSpacePath path;
        bool found = m_tileGrid->findClosestPath( from, vec2( toX, toY ), path, actorRadius );
        if( found )
        {
            if( smooth )
            {
                m_tileGrid->smoothPath( path, from, actorRadius );
                path.insert( path.begin(), from );
            }
            
            return fr::map( path, []( const vec2& position )
            {
                return std::make_tuple( position.x, position.y );
            });
        }
        else
        {
            return {};
        }
	}
}
