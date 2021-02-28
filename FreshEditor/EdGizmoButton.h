//
//  EdGizmoButton.h
//  Fresh
//
//  Created by Jeff Wofford on 5/26/11.
//  Copyright 2011 jeffwofford.com. All rights reserved.
//
#pragma once

#include "SimpleButton.h"

namespace fr
{

	class EdGizmoButton : public SimpleButton
	{
	public:

		static const char* UPDATED;
		
		SYNTHESIZE_GET( real, orbitRadius );
		void orbitRadius( real radius );
		
		SYNTHESIZE_GET( vec2, orbitCenter );
		void orbitCenter( const vec2& pos );
		
		SYNTHESIZE_GET( angle, defaultRotation );

		virtual void updateForTouchPosition( const vec2& touchPos );
		virtual void updateForConstraintChange();
		
		virtual void resetOrbitPosition();

	protected:
		
		DVAR( angle, m_defaultRotation, 45.0f );
		DVAR( bool, m_allowRadiusChange, false );
		DVAR( bool, m_draggableGizmo, true );

		real m_orbitRadius = 0;
		vec2 m_orbitCenter;

		virtual void onDragMove( const EventTouch& event ) override;
		
		FRESH_DECLARE_CLASS( EdGizmoButton, SimpleButton )
		
	};
	
}
