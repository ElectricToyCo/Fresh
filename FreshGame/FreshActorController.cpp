//
//  FreshActorController.cpp
//  Fresh
//
//  Created by Jeff Wofford on 12/3/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//

#include "FreshActorController.h"
#include "FreshActor.h"
#include "Camera.h"

#if 0
#	define travel_trace( x ) dev_trace( x )
#else
#	define travel_trace( x )
#endif

namespace fr
{	
	FRESH_DEFINE_CLASS( FreshActorController )
	DEFINE_VAR( FreshActorController, WeakPtr< FreshActor >, m_host );
	DEFINE_VAR( FreshActorController, real, m_maxTouchNodeDistance );
	DEFINE_VAR( FreshActorController, FreshTileGrid::WorldSpacePath, m_path );
	DEFINE_VAR( FreshActorController, size_t, m_nextPathDestination );
	DEFINE_VAR( FreshActorController, bool, m_wantPathSmoothing );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( FreshActorController )

	void FreshActorController::possess( FreshActor& actor )
	{
		ASSERT( !m_host );
		m_host = &actor;
	}
	
	void FreshActorController::unpossess( FreshActor& actor )
	{
		ASSERT( m_host == &actor );
		m_host = nullptr;
	}
	
	bool FreshActorController::travelTo( const vec2& pos )
	{
		REQUIRES( m_host );
		
		auto tileGrid = m_host->navigationTileGrid();
		if( !tileGrid )
		{
			dev_warning( this << " had no navigation tile grid for travelling." )
			return false;
		}
		
		const auto collisionRadius = m_host->pathfindingCollisionRadius();

		const auto amendedPos = tileGrid->findClearLocationNearby( pos, collisionRadius, tileGrid->tileSize() * 3, [&]( const vec2& p )
																  {
																	  const auto dist = distance( p, pos );
																	  if( dist == 0 ) return std::numeric_limits< real >::infinity();
																	  else return 1.0f / dist;
																  } );
		m_nextPathDestination = 0;

		// Successful?
		if( amendedPos.x < 0 )
		{
			travel_trace( "Found no clear location near: " << pos );
			m_path.clear();
			return false;
		}
		
		travel_trace( "Found amended clear position " << amendedPos << " based on target " << pos );
		
		const auto hostPosition = m_host->position();
		const auto didFind = tileGrid->findClosestPath( hostPosition, amendedPos, m_path, collisionRadius );
		
		if( didFind )
		{
			travel_trace( "Found path to: " << amendedPos << " (near " << pos << ")" );
			
			// Smooth the path.
			//
			if( m_wantPathSmoothing )
			{
				tileGrid->smoothPath( m_path, hostPosition, collisionRadius, m_maxTouchNodeDistance * m_maxTouchNodeDistance );
			}
		}
		else
		{
			travel_trace( "Found no path to: " << amendedPos << " (near " << pos << ")" );
		}
		
		return didFind;
	}
	
	void FreshActorController::stopTravel()
	{
		m_path.clear();
		m_nextPathDestination = 0;
	}
	
	vec2 FreshActorController::travelDestination()
	{
		return m_path.empty() ? vec2{ -std::numeric_limits< real >::infinity() } : m_path.back();
	}
	
	void FreshActorController::update()
	{
		if( !m_host ) { return; }
		
		while( m_nextPathDestination < m_path.size() )
		{
			const auto currentDestination = m_path[ m_nextPathDestination ];
			
			// Move to the current destination.
			//
			const auto delta = currentDestination - m_host->position();
			
			// Reached it?
			//
			const auto distSquared = delta.lengthSquared();
			
			if( distSquared <= m_maxTouchNodeDistance * m_maxTouchNodeDistance )
			{
				// Yes.
				//
				travel_trace( "Reached: " << currentDestination );
				++m_nextPathDestination;
				
				if( m_nextPathDestination >= m_path.size() )
				{
					// Let the controller know we're done.
					//
					onTravelCompleted();
					break;
				}
				
				continue;
			}
			else
			{
				travel_trace( "Moving toward: " << currentDestination );
				const auto normal = delta / std::sqrt( distSquared );
				m_host->applyControllerImpulse( normal );
				
				break;
			}
		}
	}

}

