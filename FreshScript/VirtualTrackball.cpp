//
//  VirtualTrackball.cpp
//  Fresh
//
//  Created by Jeff Wofford on 4/26/20.
//

#include "VirtualTrackball.h"
#include "Property.h"

namespace fr
{
	FRESH_DEFINE_CLASS( VirtualTrackball )
	DEFINE_VAR( VirtualTrackball, real, m_frictionTouched );
	DEFINE_VAR( VirtualTrackball, Range< real >, m_frictionLoose );
	DEFINE_VAR( VirtualTrackball, real, m_speedToFrictionPower );
	DEFINE_VAR( VirtualTrackball, real, m_speedForMinimalFriction );
	DEFINE_VAR( VirtualTrackball, real, m_speedMax );
	DEFINE_VAR( VirtualTrackball, real, m_dragScalar );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( VirtualTrackball )

	void VirtualTrackball::update()
	{
		if( m_isDragging )
		{
			auto relativeVelocity = m_lastDragVelocity - m_velocity;
			m_acceleration += relativeVelocity * ( 1.0f - m_frictionTouched );
			
			m_lastDragVelocity.setToZero();
		}
		
		m_velocity += m_acceleration;
		m_velocity -= m_velocity * friction( m_frictionLoose );
		
		// Limit trackball speed.
		if( speed() > m_speedMax )
		{
			m_velocity.normalize();
			m_velocity *= m_speedMax;
		}
		
		m_position += m_velocity;
		
		m_acceleration.setToZero();
	}
	
	void VirtualTrackball::onDragBegin( const fr::EventTouch& event )
	{
		m_isDragging = true;
	}

	void VirtualTrackball::onDragEnd( const fr::EventTouch& event )
	{
		m_isDragging = false;
	}

	void VirtualTrackball::onDragMove( const fr::EventTouch& event )
	{
		m_lastDragVelocity = ( event.location() - event.previousLocation() ) * m_dragScalar;
	}
	
	real VirtualTrackball::friction( const Range< real >& frictionRange ) const
	{
		auto friction = std::pow( clamp( 1.0f - speed() / m_speedForMinimalFriction, 0.0f, 1.0f ), m_speedToFrictionPower );
		return lerp( frictionRange, friction );
	}
}
