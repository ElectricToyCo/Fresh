/*
 *  Camera.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/9/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_CAMERA_H_INCLUDED_
#define FRESH_CAMERA_H_INCLUDED_

#include "FreshVector.h"
#include "Object.h"
#include "DisplayObject.h"

namespace fr
{
	// A Camera knows how to track certain objects or locations, smoothly interpolating from target to target if desired.
	// You should call the update() function each frame after updating object positions but before rendering.
	// Then call viewTranslation() and viewScale() in order to modify your game world's translation and scale prior to rendering.
	//
	class Camera : public Object
	{
	public:
		
		// The Camera can focus either on an object or a location.
		// Calling either of these two functions "trumps" the other one.
		//
		void setTarget( DisplayObject::cptr target, bool useEasing = true );
		void setTarget( const vec2& targetLoc, bool useEasing = true, bool clampTargetPosition = false );
		
		bool hasTarget( DisplayObject::cptr target ) const 				{ return m_target == target; }
		DisplayObject::cwptr getTarget() const								{ return m_target; }
			// May return 0.
		
		vec2 getTargetLocation() const;
			// Returns the current target location regardless of whether it is based on an object's location or a specified vector.
		
		// The focal point is where the camera is actually looking (as opposed to the target which is where it's trying to look but might not be due to tweening.
		//
		void setFocalPoint( const fr::vec2& focalPoint );
		void setFocalPointToCurrentTarget();
		const vec2& getFocalPoint() const 								{ return m_focalPoint; }
		SYNTHESIZE( vec2, lerpAlpha );
		
		void setZoomRatio( real zoomRatio, bool useEasing = true );
			// REQUIRES( zoomRatio > 0 );
		real getZoomRatio() const											{ return m_zoomRatio; }
		void setZoomLerp( real zoomLerp )									{ m_zoomLerpAlpha = zoomLerp; }
		real getZoomLerp() const											{ return m_zoomLerpAlpha; }
		
		SYNTHESIZE( real, minZoomRatio );
		SYNTHESIZE( real, maxZoomRatio );
		
		SYNTHESIZE( real, rotation );
		SYNTHESIZE( real, targetRotation );
		SYNTHESIZE( real, rotationLerp );
		
		void setClampingBounds( const rect& rectBounds )				{ m_rectBounds = rectBounds; }
			// If rectBounds has a width > 0, the camera will clamp itself such that the screen area will never
			// view outside of the rectangle's left and right values. Otherwise the camera is unbounded in X.
			// Likewise for rectBounds's height and the camera's Y movement.
		const rect& getClampingBounds() const							{ return m_rectBounds; }
		
		void setScreenSize( const vec2& screenDimensions )				{ m_screenHalfDimensions = screenDimensions * 0.5f; }
		
		//
		// Shake
		//
		
		SYNTHESIZE( real, shakeStiffness );
		SYNTHESIZE( real, shakeDamping );
		
		void shake( real initialSpeed );		// Uses random angle
		void shake( real initialSpeed, real initialAngle );
		void shake( const vec2& initialForce );
		
		//
		// Call these function every frame, after physics updates but before rendering.
		//
		
		virtual void update();
		
		vec2 viewTranslation() const;
		vec2 viewScale() const;
		angle viewRotation() const;
		
	protected:
		
		virtual void updateZoom();
		virtual void updatePosition();
		virtual void updateRotation();
		virtual void updateShake();
		
		vec2 clampTargetLocation( const vec2& location ) const;

	private:

		VAR( vec2, m_focalPoint );
		
		VAR( vec2, m_shakeOffset );
		VAR( vec2, m_shakeVelocity );
		DVAR( real, m_shakeDamping, 0.2f );
		DVAR( real, m_shakeStiffness, 0.95f );
		
		DVAR( rect, m_rectBounds, rect( 1, 1, 0, 0 ));		// mins are bigger than maxes, so bounds ignored.
		
		VAR( DisplayObject::cwptr, m_target );
		VAR( vec2, m_locTarget );		// Only used if m_target == nullptr
		
		DVAR( real, m_targetZoomRatio, 1.0f );
		DVAR( real, m_zoomRatio, 1.0f );
		DVAR( real, m_minZoomRatio, 0.25f );
		DVAR( real, m_maxZoomRatio, 4.0f );

		DVAR( bool, m_isEasing, false );
		
		DVAR( vec2, m_lerpAlpha, vec2( 0.1f ));
		DVAR( real, m_zoomLerpAlpha, 0.1f );
		VAR( vec2, m_screenHalfDimensions );

		// Degrees.
		DVAR( real, m_rotation, 0 );
		DVAR( real, m_targetRotation, 0 );
		DVAR( real, m_rotationLerp, 0.1f );
		
		FRESH_DECLARE_CLASS( Camera, Object )
	};

}

#endif
