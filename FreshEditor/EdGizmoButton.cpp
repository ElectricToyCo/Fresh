//
//  EdGizmoButton.cpp
//  Fresh
//
//  Created by Jeff Wofford on 5/26/11.
//  Copyright 2011 jeffwofford.com. All rights reserved.
//

#include "EdGizmoButton.h"

namespace fr
{

	const char* EdGizmoButton::UPDATED =  "Updated";

	FRESH_DEFINE_CLASS_UNPLACEABLE( EdGizmoButton )
	
	DEFINE_VAR( EdGizmoButton, angle, m_defaultRotation );
	DEFINE_VAR( EdGizmoButton, bool, m_allowRadiusChange );
	DEFINE_VAR( EdGizmoButton, bool, m_draggableGizmo );

	EdGizmoButton::EdGizmoButton( CreateInertObject c )
	:	Super( c )
	{
		isDragEnabled( true );
		minStageDistanceForDrag( 4 );
	}
	
	EdGizmoButton::EdGizmoButton( const ClassInfo& assignedClassInfo, NameRef objectName )
	:	Super( assignedClassInfo, objectName )
	{
		isDragEnabled( true );
		minStageDistanceForDrag( 4 );
	}
	
	void EdGizmoButton::orbitRadius( real radius )
	{
		m_orbitRadius = radius;
	}
	
	void EdGizmoButton::orbitCenter( const vec2& pos )
	{
		m_orbitCenter = pos;
	}
	
	void EdGizmoButton::updateForConstraintChange()
	{
		vec2 orbitNormal( m_orbitRadius, 0 );
		orbitNormal.rotate( m_rotation );
		
		vec2 pos( m_orbitCenter + orbitNormal );
		
		position( pos );
	}
	
	void EdGizmoButton::updateForTouchPosition( const vec2& touchPos )
	{
		vec2 pos = localToParent( touchPos );
		
		vec2 orbitNormal = pos - m_orbitCenter;		// orbitCenter is in parent space.

		if( m_allowRadiusChange )
		{
			// Constrain my position to sit at the orbit Radius from the orbit center.
			//
			orbitNormal = m_position - m_orbitCenter;		// orbitCenter is in parent space.
			m_orbitRadius = distance( pos, m_orbitCenter );
		}
		
		if( !orbitNormal.isZero( 5.0f ) )
		{		
			orbitNormal.normalize();
			orbitNormal *= m_orbitRadius;
		
			rotation( orbitNormal.angle() );
		}
		
		updateForConstraintChange();		
	}

	void EdGizmoButton::onDragMove( const EventTouch& event )
	{
		Super::onDragMove( event );

		if( m_draggableGizmo )
		{
			updateForTouchPosition( event.location() );

			Event updatedEvent( UPDATED, this );
			dispatchEvent( &updatedEvent );
		}
	}
		
	void EdGizmoButton::resetOrbitPosition()
	{
		rotation( m_defaultRotation );
		updateForConstraintChange();
	}
}
