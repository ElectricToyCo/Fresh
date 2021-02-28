//
//  DisplayObjectSpawner.h
//  Fresh
//
//  Created by Jeff Wofford on 1/8/13.
//  Copyright (c) 2013 JeffWofford. All rights reserved.
//

#ifndef Fresh_DisplayObjectSpawner_h
#define Fresh_DisplayObjectSpawner_h

#include "DisplayObject.h"

namespace fr
{
	
	class DisplayObjectSpawner : public DisplayObject
	{
	public:
		
		virtual void beginSpawning( bool restart = true );

	protected:

		virtual DisplayObject::ptr spawnObject();
		virtual void onBeginPlay() override;
		
	private:
		
		DVAR( size_t, m_nObjects, 1 );				// Objects to spawn. 0 means none (as opposed to "infinity").
		VAR( ClassInfo::cptr, m_spawnClass );
		DVAR( size_t, m_initialBurst, 0 );			// How many objects (with max of m_nObjects) to spawn before spawning with delay.
		DVAR( TimeType, m_delayBetweenSpawns, 0 );	// 0 means immediate--all spawn at once.
		
		DVAR( real, m_spawnAreaRadius, 0 );
		
		size_t m_nObjectsSpawned = 0;
		
		FRESH_DECLARE_CALLBACK( DisplayObjectSpawner, onDelayFinished, Event );
		
		FRESH_DECLARE_CLASS( DisplayObjectSpawner, DisplayObject );
	};
	
}

#endif
