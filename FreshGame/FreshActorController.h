//
//  FreshActorController.h
//  Fresh
//
//  Created by Jeff Wofford on 12/3/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_FreshActorController_h
#define Fresh_FreshActorController_h

#include "EventDispatcher.h"
#include "FreshTileGrid.h"

namespace fr
{
	class FreshActor;
	
	class FreshActorController : public EventDispatcher
	{
		FRESH_DECLARE_CLASS( FreshActorController, EventDispatcher );
	public:
		
		SYNTHESIZE_GET( WeakPtr< FreshActor >, host );
		
		virtual void possess( FreshActor& actor );
		virtual void unpossess( FreshActor& actor );
		
		SYNTHESIZE( real, maxTouchNodeDistance );
		virtual bool travelTo( const vec2& pos );
		virtual void stopTravel();
		virtual vec2 travelDestination();
		
		virtual void update();
		
	protected:
		
		virtual void onTravelCompleted() {}
		
	private:
		
		VAR( WeakPtr< FreshActor >, m_host );
		
		DVAR( real, m_maxTouchNodeDistance, 1.0f );		// The maximum distance at which the host can approach a path node and the node is considered "touched" or "visited."
		VAR( FreshTileGrid::WorldSpacePath, m_path );
		DVAR( size_t, m_nextPathDestination, 0 );
		DVAR( bool, m_wantPathSmoothing, false );
	};
	
}

#endif
