//
//  FreshActor.h
//  Fresh
//
//  Created by Jeff Wofford on 12/3/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_FreshActor_h
#define Fresh_FreshActor_h

#include "MovieClip.h"
#include "FreshWorld.h"
#include "Segment.h"

namespace fr
{
	class FreshActorController;
	class FreshTileGrid;
	
	class FreshActor : public MovieClip
	{
		FRESH_DECLARE_CLASS( FreshActor, MovieClip );
	public:
		
		SYNTHESIZE( real, mass );
		SYNTHESIZE_GET( vec2, velocity );
		
		SYNTHESIZE_GET( bool, grounded )
		SYNTHESIZE_GET( SmartPtr< FreshActorController >, controller )
		SmartPtr< FreshActorController > controller( SmartPtr< FreshActorController > controller_ );
		
		SYNTHESIZE_GET( vec2, dimensions );
		rect collisionBounds() const;
		
		virtual WeakPtr< FreshTileGrid > navigationTileGrid() const;
		virtual real pathfindingCollisionRadius() const;
		// PROMISES( result >= 0 );

		vec2 effectiveVelocity() const { return m_position - m_lastPosition; }
		
		FreshWorld& world() const;

		virtual void resolveTileCollisions( FreshTileGrid& tileGrid );
		virtual void resolveActorCollision( FreshActor& otherActor );
		virtual void resolveCollision( const rect& rect );
		
		virtual void applyImpulse( const vec2& i )
		{
			if( m_mass > 0 )
			{
				m_acceleration += i / m_mass;
			}
		}
		
		virtual void applyGravity( const vec2& g )
		{
			if( m_mass > 0 && !m_grounded )
			{
				m_acceleration += g;
			}
		}
		
		virtual void applyControllerImpulse( const vec2& i );
		virtual void jump( real jumpPower );
		
		virtual void update() override;
		
		virtual void postLoad() override;
		virtual void onAddedToStage() override;
		
	protected:

		VAR( vec2, m_velocity );

		// To be overridden in derived classes
		//
		virtual void beginFalling();
		virtual void onLanded( const vec2& hitNormal );
		virtual void onBumpedWall( const vec2& hitNormal );
		virtual void onBumpedActor( FreshActor& otherActor, const vec2& hitNormal, real adjustmentDistance );

		virtual bool mayCollideWith( const FreshActor& otherActor ) const;
		virtual bool shouldResolveCollisionWith( const FreshActor& otherActor, const vec2& hitNormal, real adjustmentDistance ) const;
		
		virtual bool mayCollideWith( const FreshTileGrid& tileGrid ) const;
		
		bool findCollisionNormal( const vec2& rectCenter, const vec2& rectSize, const vec2& thatVelocity, vec2& outHitNormal, int& outNormalAxis, float& outAdjustmentDistance ) const;
		
		void findCollisionNormalWithVelocity( const vec2& rectCenter, const vec2& rectSize, const vec2& overlaps, const vec2& delta, const vec2& thatVelocity, vec2& outHitNormal, int& outNormalAxis ) const;
		
		virtual bool doUpdatePhysics() const;

	private:
		
		VAR( vec2, m_lastPosition );
		VAR( vec2, m_acceleration );
		VAR( vec2, m_dimensions );

		DVAR( real, m_mass, 1 );
		DVAR( real, m_groundedDrag, 0.2f );
		DVAR( real, m_airDrag, 0 );

		DVAR( vec2, m_controllerImpulseScale, vec2( 10 ));
		DVAR( vec2, m_jumpImpulseScale, vec2( 0, 1000 ));
		DVAR( vec2, m_airControl, vec2( 0.5 ));
		
		DVAR( bool, m_grounded, false );
		DVAR( bool, m_mayLand, true );				// Else never becomes grounded.
		DVAR( bool, m_doesUpdatePhysics, true );

		DVAR( uint, m_collisionMask, ~0 );
		DVAR( uint, m_collisionRefusalMask, 0 );
		
		VAR( Segments, m_dynamicLightBlockers );

		bool m_areDynamicLightBlockersDirty = true;
		
		VAR( SmartPtr< FreshActorController >, m_controller );
	};
	
}

#endif
