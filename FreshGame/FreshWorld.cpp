//
//  FreshWorld.cpp
//  Fresh
//
//  Created by Jeff Wofford on 12/3/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//

#include "FreshWorld.h"
#include "FreshActor.h"
#include "FreshActorController.h"
#include "FreshTileGrid.h"
#include "Camera.h"
#include "Lighting.h"
#include "Stage.h"

namespace fr
{

	FRESH_DEFINE_CLASS( Attachment )
	
	DEFINE_VAR( Attachment, WeakPtr< DisplayObject >, m_attached );
	DEFINE_VAR( Attachment, WeakPtr< DisplayObject >, m_to );
	DEFINE_VAR( Attachment, vec2, m_offset );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( Attachment )

	SYNTHESIZE_SET_IMPL( Attachment, WeakPtr< DisplayObject >, attached );
	SYNTHESIZE_SET_IMPL( Attachment, WeakPtr< DisplayObject >, to );
	
	bool Attachment::valid() const
	{
		return m_attached && m_to;
	}
	
	void Attachment::update()
	{
		ASSERT( valid() );
		m_attached->position( m_to->position() + m_offset );
	}
	
	////////////////////////////////////////////////////////////////////////////////
	
	FRESH_DEFINE_CLASS( FreshWorld )
	
	DEFINE_VAR( FreshWorld, vec2, m_gravity );
	DEFINE_VAR( FreshWorld, SmartPtr< Camera >, m_camera );
	DEFINE_VAR( FreshWorld, std::vector< Attachment::ptr >, m_attachments );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( FreshWorld )
	
	FRESH_CUSTOM_STANDARD_CONSTRUCTOR_NAMING( FreshWorld )
	,	TimeServer( createObject< CallbackScheduler >( &getTransientPackage(), CallbackScheduler::StaticGetClassInfo() ) )
	{}
	
	SYNTHESIZE_SET_IMPL( FreshWorld, SmartPtr< Camera >, camera );
	
	void FreshWorld::update()
	{
		Super::update();
		
		updateScheduledCallbacks();
		
		updateAttachments();
		
		// Build a list of actor descendants.
		//
		std::vector< FreshActor::ptr > actors;
		forEachDescendant< FreshActor >( [&]( FreshActor& actor )
										{
											actors.push_back( &actor );
										} );
		
		updateActors( actors );

		// Update the camera.
		//
		updateCamera();
	}
	
	void FreshWorld::updateActors( const std::vector< SmartPtr< FreshActor > >& actors )
	{
		// Apply gravity to actors.
		//
		if( !m_gravity.isZero() )
		{
			for( auto actor : actors )
			{
				actor->applyGravity( m_gravity );
			}
		}
		
		resolveCollisions( actors );
	}
	
	void FreshWorld::resolveCollisions( const std::vector< SmartPtr< FreshActor >>& actors )
	{
		// Resolve actor-tilegrid collisions.
		//
		forEachChild< FreshTileGrid >( [&]( FreshTileGrid& tileGrid )
									  {
										  for( auto actor : actors )
										  {
											  actor->resolveTileCollisions( tileGrid );
										  }
									  } );
		
		// Resolve actor-actor collisions.
		//
		for( auto outer = actors.begin(); outer != actors.end(); ++outer )
		{
			for( auto inner = outer + 1; inner != actors.end(); ++inner )
			{
				const auto outerActor = *outer;
				const auto innerActor = *inner;
				
				ASSERT( outerActor != innerActor );
				
				outerActor->resolveActorCollision( *innerActor );
			}
		}
	}
	
	void FreshWorld::updateCamera()
	{
		if( m_camera )
		{
			m_camera->setScreenSize( stage().stageDimensions() );
			m_camera->update();
			
			auto pos = m_camera->viewTranslation();			
			position( pos );
			scale( m_camera->viewScale() );
			
			rotation( m_camera->viewRotation() );
		}
	}
	
	void FreshWorld::updateAttachments()
	{
		// Detect and report cycles.
		// TODO
		
		removeElements( m_attachments, []( const Attachment::ptr& attachment )
										   {
											   return !attachment->valid();
										   } );
		
		for( const auto& attachment : m_attachments )
		{
			attachment->update();
		}
	}

	SmartPtr< Lighting > FreshWorld::lighting() const
	{
		return getChildByName< Lighting >( "" );
	}
	
	TimeType FreshWorld::time() const
	{
		return nUpdates() * stage().secondsPerFrame();
	}
	
	void FreshWorld::attach( DisplayObject& attached, DisplayObject& to, const vec2& offset )
	{
		const auto attachment = createObject< Attachment >();
		attachment->attached( &attached );
		attachment->to( &to );
		attachment->offset( offset );
		
		m_attachments.push_back( attachment );
	}
	
	vec2 FreshWorld::detach( DisplayObject& attached, DisplayObject& to )
	{
		const auto iter = std::find_if( m_attachments.begin(), m_attachments.end(), [&]( Attachment::ptr attachment )
									{
										return attachment->attached() == &attached && attachment->to() == &to;
									} );
		
		ASSERT( iter != m_attachments.end() );
		
		const Attachment::ptr attachment = *iter;
		
		m_attachments.erase( iter );
		
		return attachment->offset();
	}	
}

