//
//  ApiMap.cpp
//  Fresh
//
//  Created by Jeff Wofford on 8/05/18.
//  Copyright (c) 2018 Jeff Wofford. All rights reserved.
//

#include "FantasyConsole.h"
#include "FreshVector.h"

namespace fr
{		
	LUA_FUNCTION( map, 4 )
	void FantasyConsole::map( int cel_x, int cel_y, int sx, int sy, int cel_w, int cel_h, int layer )
	{
		mapdraw( cel_x, cel_y, sx, sy, cel_w, cel_h, layer );
	}

	LUA_FUNCTION( mapdraw, 4 )
	void FantasyConsole::mapdraw( int cel_x, int cel_y, int sx, int sy, int cel_w, int cel_h, int layer )
	{
        DEFAULT( cel_w, 1 );
        DEFAULT( cel_h, cel_w );
        DEFAULT( layer, 0 );
		
		if( layer >= static_cast< int >( m_mapLayerSizes.size() ))
		{
			return;
		}
		
		ASSERT( layer < static_cast< int >( m_mapSpriteLayers.size() ));
		
		auto& mapSize = m_mapLayerSizes[ layer ];

        const auto spriteSize = m_baseSpriteSizeInTexels;
        
        const int left = clamp( cel_x, 0, mapSize.x );
        const int right = clamp( cel_x + cel_w, 0, mapSize.x );
        const int top = clamp( cel_y, 0, mapSize.y );
        const int bottom = clamp( cel_y + cel_h, 0, mapSize.y );
        for( int y = top; y < bottom; ++y )
		{
            const real screenY = sy + ( y - cel_y ) * spriteSize.y;
			for( int x = left; x < right; ++x )
			{
                const real screenX = sx + ( x - cel_x ) * spriteSize.x;
                const int sprite = mget( x, y, layer );
                
                // Only draw if the `sprite` is non-zero and either `layer` is 0 or the sprite's flags are set for every bit set in `layer`.
                if( sprite > 0 && ( 0 == layer || layer == fget( sprite, layer )))
                {
                    spr( sprite, screenX, screenY, 1, 1, false, false, Color::White, 0 );
                }
			}
		}
	}

	LUA_FUNCTION( mget, 2 )
	int FantasyConsole::mget( real x, real y, int layer )
	{
        DEFAULT( layer, 0 );
		
		if( layer >= static_cast< int >( m_mapLayerSizes.size() ))
		{
			return 0;
		}
		
		ASSERT( layer < static_cast< int >( m_mapSpriteLayers.size() ));

		const auto& mapSize = m_mapLayerSizes[ layer ];
		const auto& mapSprites = m_mapSpriteLayers[ layer ];

        const int ix = clamp( static_cast< int >( std::floor( x )), 0, mapSize.x - 1 );
        const int iy = clamp( static_cast< int >( std::floor( y )), 0, mapSize.y - 1 );
        const int index = ix + iy * mapSize.x;
        
        return index < static_cast< int >( mapSprites.size() ) ? mapSprites[ index ] : 0;
	}

	LUA_FUNCTION( mset, 3 )
	void FantasyConsole::mset( real x, real y, int v, int layer )
	{
        DEFAULT( layer, 0 );
		
		if( layer >= static_cast< int >( m_mapLayerSizes.size() ))
		{
			return;
		}

		ASSERT( layer < static_cast< int >( m_mapSpriteLayers.size() ));

		auto& mapSize = m_mapLayerSizes[ layer ];
		auto& mapSprites = m_mapSpriteLayers[ layer ];

		const int ix = clamp( static_cast< int >( std::floor( x )), 0, mapSize.x - 1 );
        const int iy = clamp( static_cast< int >( std::floor( y )), 0, mapSize.y - 1 );
        const int index = ix + iy * mapSize.x;
        
        if( index < static_cast< int >( mapSprites.size() ))
        {
            mapSprites[ index ] = v;
        }
	}
}

