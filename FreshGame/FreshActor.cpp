//
//  FreshActor.cpp
//  Fresh
//
//  Created by Jeff Wofford on 12/3/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//

#include "FreshActor.h"
#include "Stage.h"
#include "FreshWorld.h"
#include "Camera.h"
#include "Lighting.h"
#include "FreshTileGrid.h"
#include "FreshActorController.h"

namespace
{
	using namespace fr;
	
	const float SEPARATION_BUFFER = 0.1f;
	const float MIN_FLOOR_Y = -0.1f;				// cos of least-angled floor that is considered a floor.
	
	template< typename T >
	struct stepper
	{
		stepper( T min, T max, T step = T( 1 ))
		:	m_begin( min )
		,	m_end( max )
		,	m_step( step )
		{
			if( step < 0 )
			{
				std::swap( m_begin, m_end );
			}
			m_end += step;
			m_current = m_begin;
		}
		
		stepper& operator++()		// ++Prefix
		{
			m_current += m_step;
			return *this;
		}
		
		stepper operator++(int)		// Postfix++
		{
			stepper copy( *this );
			m_current += m_step;
			return copy;
		}
		
		bool more() const
		{
			return m_current != m_end;
		}
		
		operator T() const
		{
			return m_current;
		}
		
	private:
		
		T m_begin;
		T m_end;
		T m_step;
		T m_current;
	};
}

namespace fr
{
	FRESH_DEFINE_CLASS( FreshActor )
	
	DEFINE_VAR( FreshActor, vec2, m_lastPosition );
	DEFINE_VAR( FreshActor, vec2, m_velocity );
	DEFINE_VAR( FreshActor, vec2, m_acceleration );
	DEFINE_VAR( FreshActor, vec2, m_dimensions );
	DEFINE_VAR( FreshActor, real, m_mass );
	DEFINE_VAR( FreshActor, real, m_groundedDrag );
	DEFINE_VAR( FreshActor, real, m_airDrag );
	DEFINE_VAR( FreshActor, vec2, m_controllerImpulseScale );
	DEFINE_VAR( FreshActor, vec2, m_jumpImpulseScale );
	DEFINE_VAR( FreshActor, vec2, m_airControl );
	DEFINE_VAR( FreshActor, bool, m_grounded );
	DEFINE_VAR( FreshActor, bool, m_mayLand );
	DEFINE_VAR( FreshActor, bool, m_doesUpdatePhysics );
	DEFINE_VAR( FreshActor, uint, m_collisionMask );
	DEFINE_VAR( FreshActor, uint, m_collisionRefusalMask );
	DEFINE_VAR( FreshActor, Segments, m_dynamicLightBlockers );
	DEFINE_VAR( FreshActor, SmartPtr< FreshActorController >, m_controller );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( FreshActor )
	
	SmartPtr< FreshActorController > FreshActor::controller( SmartPtr< FreshActorController > controller_ )
	{
		if( m_controller )
		{
			m_controller->unpossess( *this );
		}
		
		m_controller = controller_;
		
		if( m_controller )
		{
			m_controller->possess( *this );
		}
		
		return m_controller;
	}
	
	rect FreshActor::collisionBounds() const
	{
		return rect{ position() - m_dimensions * 0.5f, position() + m_dimensions * 0.5f };
	}
	
	FreshWorld& FreshActor::world() const
	{
		auto world = firstAncestorOfType< FreshWorld >( *this );
		ASSERT( world );
		return *world;
	}
	
	WeakPtr< FreshTileGrid > FreshActor::navigationTileGrid() const
	{
		// May return nullptr.
		return world().getDescendantByName< FreshTileGrid >( "" );
	}
	
	real FreshActor::pathfindingCollisionRadius() const
	{
		const auto result = m_dimensions.majorAxisValue() * 0.5f;
		PROMISES( result >= 0 );
		return result;
	}
	
	void FreshActor::postLoad()
	{
		Super::postLoad();
		
		m_lastPosition = position();
	}
	
	void FreshActor::onAddedToStage()
	{
		Super::onAddedToStage();
		
		if( m_controller && m_controller->host() != this )
		{
			ASSERT( !m_controller->host() );
			m_controller->possess( *this );
		}
	}
	
	void FreshActor::applyControllerImpulse( const vec2& i )
	{
		vec2 impulse = i;
		if( !m_grounded )
		{
			impulse *= m_airControl;
		}
		applyImpulse( impulse * m_controllerImpulseScale );
	}
	
	void FreshActor::jump( real jumpPower )
	{
		if( m_grounded )
		{
			applyImpulse( m_jumpImpulseScale * jumpPower );
			beginFalling();
		}
	}
	
	void FreshActor::update()
	{
		if( m_controller )
		{
			ASSERT( m_controller->host() == this );
			m_controller->update();
		}
		
		const vec2 savedPosition = position();
		
		if( doUpdatePhysics() )
		{
			const real deltaTime = stage().secondsPerFrame();
			
			const auto drag = m_grounded ? m_groundedDrag : m_airDrag;
			
			m_acceleration += m_velocity * -drag;
			
			m_velocity += m_acceleration * deltaTime;
			position( position() + m_velocity * deltaTime );
		}
		
		m_acceleration.setToZero();
		m_lastPosition = savedPosition;
		
		Super::update();
		
		if( doUpdate() )
		{
			// If we moved this frame and we have light blockers, dirty any nearby lights.
			//
			const DisplayObjectState& priorState = getPreviousState();	// Note that the previous state is only recorded before the first step of the Stage update. Thus this could be inaccurate sometimes.
			
			if( auto lighting = world().lighting() )
			{
				if( !m_dynamicLightBlockers.empty() && ( m_areDynamicLightBlockersDirty ||
														( priorState.position() != m_position ||
														 priorState.scale() != m_scale ||
														 angle( priorState.rotation()) != m_rotation ||
														 priorState.pivot() != m_pivot
														 ))
					  )
				{
					lighting->dirtyAffectedLights( world().globalToLocal( parent()->localToGlobal( m_position )), getRadius( m_dynamicLightBlockers ));
					m_areDynamicLightBlockersDirty = false;
				}
			}
		}
	}
	
	void FreshActor::resolveTileCollisions( FreshTileGrid& tileGrid )
	{
		if( !mayCollideWith( tileGrid ))
		{
			return;
		}
		
		// Defeat Wile-E-Coyoteing by checking if anything's below me.
		//
		if( m_grounded )
		{
			// Look at the tile(s) just below my "feet".
			//
			const vec2 checkPointA = position() + vec2( m_dimensions.x * -0.5f, m_dimensions.y * 0.5f + SEPARATION_BUFFER * 2.0f );
			const vec2 checkPointB = position() + vec2( m_dimensions.x *  0.5f, m_dimensions.y * 0.5f + SEPARATION_BUFFER * 2.0f );
			if( !tileGrid.getTile( checkPointA ).isSolid() && !tileGrid.getTile( checkPointB ).isSolid() )
			{
				beginFalling();
			}
		}
		
		const vec2 halfDimensions( m_dimensions * 0.5f );
		
		const Vector2i bl( tileGrid.worldToTileSpace( position() - halfDimensions ));
		const Vector2i ur( tileGrid.worldToTileSpace( position() + halfDimensions ));
		
		for( stepper< int > y( bl.y, ur.y, static_cast< int >( signNoZero( m_velocity.y ))); y.more(); ++y )
		{
			for( stepper< int > x( bl.x, ur.x, static_cast< int >( signNoZero( m_velocity.x ))); x.more(); ++x )
			{
				if( isMarkedForDeletion() )
				{
					return;
				}
				
				// Resolve collisions.
				//
				if( tileGrid.getTile( Vector2i( x, y ) ).isSolid() )
				{
					const auto halfTileSize = tileGrid.tileSize() * 0.5f;
					const auto tileCenter = tileGrid.tileCenter( Vector2i( x, y ) );
					resolveCollision( rect( tileCenter - halfTileSize, tileCenter + halfTileSize ));
					
					// TODO Should actually restart this function (basically)
					// because position() and m_velocity have changed and bl and ur, which control this loop,
					// may therefore have changed, along with the stepping direction.
					//
					// As it stands, we'll sometimes collide with things we shouldn't,
					// and sometimes pass through things.
					//
					// Generally these errors will be rare and hard to see: they occur most
					// often when we're moving fast, and that's also when they're hardest to see.
				}
			}
		}
	}
	
	void FreshActor::resolveActorCollision( FreshActor& otherActor )
	{
		if( !mayCollideWith( otherActor ))
		{
			return;
		}
		
		const auto rect = otherActor.collisionBounds();
		
		vec2 hitNormal;
		int normalAxis;
		float adjustmentDistance;
		
		bool collided = findCollisionNormal( rect.midpoint(), rect.dimensions(), otherActor.effectiveVelocity(), hitNormal, normalAxis, adjustmentDistance );
		if( !collided )
		{
			return;
		}

		const auto totalMass = m_mass + otherActor.m_mass;
		
		if( !shouldResolveCollisionWith( otherActor, hitNormal, adjustmentDistance ) || totalMass <= 0 )
		{
			// Immoveable object meets irresistible force.
			//
			return;
		}
		else
		{
			real proportionA = otherActor.m_mass / totalMass;
			real proportionB = m_mass / totalMass;
			
			if( m_mass <= 0 )
			{
				proportionA = 0;
				proportionB = 1.0f;
			}
			else if( otherActor.m_mass <= 0 )
			{
				proportionA = 1.0f;
				proportionB = 0;
			}
			
			position( position() + hitNormal * adjustmentDistance * proportionA );
			otherActor.position( otherActor.position() - hitNormal * adjustmentDistance * proportionB );

			{
				vec2 velocityAlongNormal;
				velocityAlongNormal[ normalAxis ] = m_velocity[ normalAxis ];
				if( hitNormal.dot( velocityAlongNormal ) <= 0 )
				{
					// Zero velocity in this axis.
					//
					m_velocity[ normalAxis ] = 0;
				}
			}
			{
				vec2 velocityAlongNormal;
				velocityAlongNormal[ normalAxis ] = otherActor.m_velocity[ normalAxis ];
				if( hitNormal.dot( velocityAlongNormal ) <= 0 )
				{
					// Zero velocity in this axis.
					//
					otherActor.m_velocity[ normalAxis ] = 0;
				}
			}
		}
		
		onBumpedActor( otherActor, hitNormal, adjustmentDistance );
	}
	
	void FreshActor::resolveCollision( const rect& rect )
	{
		vec2 hitNormal;
		int normalAxis;
		float adjustmentDistance;
		
		bool collided = findCollisionNormal( rect.midpoint(), rect.dimensions(), vec2( 0 ), hitNormal, normalAxis, adjustmentDistance );
		if( !collided )
		{
			return;
		}
		
		position( position() + hitNormal * adjustmentDistance );
		
		// Don't zero velocity if the FreshActor is already moving away from the other object.
		//
		vec2 velocityAlongNormal;
		velocityAlongNormal[ normalAxis ] = m_velocity[ normalAxis ];
		if( hitNormal.dot( velocityAlongNormal ) <= 0 )
		{
			// Zero velocity in this axis.
			//
			m_velocity[ normalAxis ] = 0;
		}
		
		//
		// Update physics states to reflect this collision and notify handlers.
		//
		
		// Is this a floor? (And do we care?)
		//
		if( m_mayLand && hitNormal.y <= MIN_FLOOR_Y )
		{
			if( !m_grounded )
			{
				onLanded( hitNormal );
			}
		}
		else
		{
			onBumpedWall( hitNormal );
		}
	}
	
	void FreshActor::beginFalling()
	{
		m_grounded = false;
	}
	
	void FreshActor::onLanded( const vec2& hitNormal )
	{
		ASSERT( m_mayLand );
		m_grounded = true;
	}
	
	void FreshActor::onBumpedWall( const vec2& hitNormal )
	{}
	
	void FreshActor::onBumpedActor( FreshActor& otherActor, const vec2& hitNormal, real adjustmentDistance )
	{}
	
	bool FreshActor::mayCollideWith( const FreshActor& otherActor ) const
	{
		return	  ( m_collisionMask & otherActor.m_collisionMask ) != 0 &&
				  ( m_collisionRefusalMask & otherActor.m_collisionRefusalMask ) == 0;
	}
	
	bool FreshActor::mayCollideWith( const FreshTileGrid& tileGrid ) const
	{
		return	( m_collisionMask & tileGrid.collisionMask() ) != 0 &&
				( m_collisionRefusalMask & tileGrid.collisionRefusalMask() ) == 0;
	}

	bool FreshActor::shouldResolveCollisionWith( const FreshActor& otherActor, const vec2& hitNormal, real adjustmentDistance ) const
	{
		return true;
	}
	
	bool FreshActor::findCollisionNormal( const vec2& rectCenter, const vec2& rectSize, const vec2& thatVelocity, vec2& outHitNormal, int& outNormalAxis, float& outAdjustmentDistance ) const
	{
        bool colliding = fr::findCollisionNormal( position(), m_dimensions, position() - m_lastPosition, rectCenter, rectSize, thatVelocity, outHitNormal, outNormalAxis, outAdjustmentDistance );
        
        if( colliding )
        {
            outAdjustmentDistance += SEPARATION_BUFFER;
        }
        
        return colliding;
	}
	
	bool FreshActor::doUpdatePhysics() const
	{
		return m_doesUpdatePhysics;
	}
}
