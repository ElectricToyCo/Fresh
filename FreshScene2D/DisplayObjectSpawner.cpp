//
//  DisplayObjectSpawner.cpp
//  Fresh
//
//  Created by Jeff Wofford on 1/8/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "DisplayObjectSpawner.h"
#include "Stage.h"
#include "CommandProcessor.h"

namespace fr
{
	
	FRESH_DEFINE_CLASS( DisplayObjectSpawner )
	
	DEFINE_VAR( DisplayObjectSpawner, size_t, m_nObjects );	// 0 means none.
	DEFINE_VAR( DisplayObjectSpawner, ClassInfo::cptr, m_spawnClass );
	DEFINE_VAR( DisplayObjectSpawner, size_t, m_initialBurst );
	DEFINE_VAR( DisplayObjectSpawner, TimeType, m_delayBetweenSpawns );	// 0 means immediate--all spawn at once.
	DEFINE_VAR( DisplayObjectSpawner, real, m_spawnAreaRadius );
	
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( DisplayObjectSpawner )
	
	void DisplayObjectSpawner::beginSpawning( bool restart )
	{
		if( restart )
		{
			m_nObjectsSpawned = 0;
		}
		
		if( m_delayBetweenSpawns > 0 )
		{
			// Spawn "initial burst", then delay for the rest.
			//
			while( m_nObjectsSpawned < m_nObjects && m_nObjectsSpawned < m_initialBurst )
			{
				spawnObject();
			}
			
			if( m_nObjectsSpawned < m_nObjects )
			{
				stage().scheduleCallback( FRESH_CALLBACK( onDelayFinished ), m_delayBetweenSpawns );
			}
		}
		else
		{
			// Spawn them all immediately.
			//
			while( m_nObjectsSpawned < m_nObjects )
			{
				spawnObject();
			}
		}
	}
	
	DisplayObject::ptr DisplayObjectSpawner::spawnObject()
	{
		ASSERT( parent() );
		
		DisplayObject::ptr object;
		if( m_spawnClass )
		{
			object = createObject< DisplayObject >( *m_spawnClass );
			if( object )
			{
				++m_nObjectsSpawned;

				vec2 delta( randInRange( 0.0f, m_spawnAreaRadius ), 0 );
				delta.rotate( angle( randInRange( 0.0f, 360.0f )));
				
				const vec2 spawnPosition( position() + delta );
				
				object->position( spawnPosition );
				parent()->addChild( object );
			}
			else
			{
				con_error( this << " unable to spawn DisplayObject of class '" << m_spawnClass->className() << "'." );
			}
		}
		
		return object;
	}

	void DisplayObjectSpawner::onBeginPlay()
	{
		Super::onBeginPlay();
		
		if( doUpdate() && m_nObjects > 0 )
		{
			beginSpawning();
		}
	}
	
	FRESH_DEFINE_CALLBACK( DisplayObjectSpawner, onDelayFinished, Event )
	{
		if( m_nObjectsSpawned < m_nObjects )
		{
			spawnObject();
			
			// Still want more?
			//
			if( m_nObjectsSpawned < m_nObjects )
			{
				stage().scheduleCallback( FRESH_CALLBACK( onDelayFinished ), m_delayBetweenSpawns );
			}
		}
	}
}

