//
//  VirtualTrackball.h
//  Fresh
//
//  Created by Jeff Wofford on 4/26/20.
//

#ifndef VirtualTrackball_h
#define VirtualTrackball_h

#include "Objects.h"
#include "FreshMath.h"
#include "EventTouch.h"

namespace fr
{
	class VirtualTrackball : public Object
	{
		FRESH_DECLARE_CLASS( VirtualTrackball, Object );
	public:
		
		SYNTHESIZE( real, frictionTouched );
		SYNTHESIZE( Range< real >, frictionLoose );
		SYNTHESIZE( real, speedToFrictionPower );
		SYNTHESIZE( real, speedForMinimalFriction );
		SYNTHESIZE( real, speedMax );
		SYNTHESIZE( real, dragScalar );
		
		virtual void update();

		void reset() { m_position = m_velocity = m_acceleration = vec2::ZERO; }
		SYNTHESIZE_GET( vec2, velocity );
		SYNTHESIZE_GET( vec2, position );
		real speed() const { return m_velocity.length(); }
		
		void onDragBegin( const fr::EventTouch& event );
		void onDragMove( const fr::EventTouch& event );
		void onDragEnd( const fr::EventTouch& event );
		
	protected:
		
		real friction( const Range< real >& frictionRange ) const;
		
	private:
		
		DVAR( real, m_frictionTouched, 0.9f );
		DVAR( Range< real >, m_frictionLoose, Range< real >( 0.012f, 0.05f ));
		DVAR( real, m_speedToFrictionPower, 3.f );
		DVAR( real, m_speedForMinimalFriction, 3.5f );
		DVAR( real, m_speedMax, 4.0f );
		DVAR( real, m_dragScalar, 1.5f );

		bool m_isDragging = false;

		vec2 m_position;
		vec2 m_velocity;
		vec2 m_acceleration;

		vec2 m_lastDragVelocity;
	};
}

#endif /* VirtualTrackball_h */
