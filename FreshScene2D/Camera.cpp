/*
 *  Camera.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/9/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#include "Camera.h"
#include "Objects.h"
#include "Stage.h"

namespace
{
	inline fr::real specialClamp( fr::real a, fr::real min, fr::real max )
	{
		if( max - min <= 0 )
		{
			return ( min + max ) * 0.5f;
		}
		else
		{
			return fr::clamp( a, min, max );
		}
	}
}

namespace fr
{
	FRESH_DEFINE_CLASS( Camera )

	DEFINE_VAR( Camera, vec2, m_focalPoint );
	DEFINE_VAR( Camera, vec2, m_shakeOffset );
	DEFINE_VAR( Camera, vec2, m_shakeVelocity );
	DEFINE_VAR( Camera, real, 	 m_shakeDamping );
	DEFINE_VAR( Camera, real, 	 m_shakeStiffness );
	DEFINE_VAR( Camera, rect, 	m_rectBounds );
	DEFINE_VAR( Camera, DisplayObject::cwptr, m_target );
	DEFINE_VAR( Camera, vec2, m_locTarget );
	DEFINE_VAR( Camera, real, m_targetZoomRatio );
	DEFINE_VAR( Camera, real, m_zoomRatio );
	DEFINE_VAR( Camera, real, m_minZoomRatio );
	DEFINE_VAR( Camera, real, m_maxZoomRatio );
	DEFINE_VAR( Camera, bool, m_isEasing );
	DEFINE_VAR( Camera, vec2, m_lerpAlpha );
	DEFINE_VAR( Camera, real, m_zoomLerpAlpha );
	DEFINE_VAR( Camera, real, m_rotation );
	DEFINE_VAR( Camera, real, m_targetRotation );
	DEFINE_VAR( Camera, real, m_rotationLerp );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( Camera )
	
	void Camera::setTarget( DisplayObject::cptr target, bool useEasing /* = true */ )
	{
		if( !target )
		{
			if( m_target )
			{
				setTarget( m_target->position(), useEasing );
			}
			else
			{
				setTarget( m_focalPoint, useEasing );
			}
			return;
		}
		
		m_target = target;	
		m_isEasing = useEasing;

		if( !m_isEasing )
		{
			setFocalPointToCurrentTarget();
		}
	}

	void Camera::setTarget( const vec2& targetLoc, bool useEasing /* = true */, bool clampTargetPosition /* = false */ )
	{
		m_target = nullptr;
		m_locTarget = targetLoc;
		
		if( clampTargetPosition )
		{
			m_locTarget = clampTargetLocation( m_locTarget );
		}
			
		m_isEasing = useEasing;
		
		if( !m_isEasing )
		{
			setFocalPointToCurrentTarget();
		}
	}

	vec2 Camera::getTargetLocation() const
	{
		if( m_target )
		{
			return m_target->position();
		}
		else
		{
			return m_locTarget;
		}
	}

	void Camera::setFocalPoint( const vec2& focalPoint )
	{
		m_focalPoint = focalPoint;
	}

	void Camera::setFocalPointToCurrentTarget()
	{
		if( m_target )
		{
			setFocalPoint( m_target->position() );
		}
		else
		{
			setFocalPoint( m_locTarget );
		}
	}

	void Camera::setZoomRatio( real zoomRatio, bool useEasing /* = true */ )
	{
		REQUIRES( zoomRatio > 0 );
		
		m_targetZoomRatio = zoomRatio;
		
		if( !useEasing )
		{
			m_zoomRatio = clamp( m_targetZoomRatio, m_minZoomRatio, m_maxZoomRatio );
			
			m_focalPoint = clampTargetLocation( m_focalPoint );
			
			if( !m_target )
			{
				m_locTarget = clampTargetLocation( m_locTarget );
			}
		}
	}

	vec2 Camera::viewTranslation() const
	{
		vec2 adjustedFocalPoint( clampTargetLocation( m_focalPoint + m_shakeOffset ) );
		
		return ( -adjustedFocalPoint * viewScale() ).getRotated( viewRotation() );
	}

	vec2 Camera::viewScale() const
	{
		ASSERT( m_zoomRatio > 0 );
		
		return vec2( m_zoomRatio, m_zoomRatio );
	}

	angle Camera::viewRotation() const
	{
		return -m_rotation;
	}
	
	void Camera::shake( real initialSpeed )
	{
		shake( initialSpeed, randInRange( 0.0f, 360.0f ) );
	}

	void Camera::shake( real initialSpeed, real initialAngle )
	{
		shake( vec2( cos( initialAngle ), sin( initialAngle )) * initialSpeed );
	}

	void Camera::shake( const vec2& initialForce )
	{
		m_shakeVelocity += initialForce;
	}

	void Camera::update()
	{
		updateZoom();
		updatePosition();
		updateRotation();
		updateShake();
		
		m_focalPoint = clampTargetLocation( m_focalPoint );
	}

	void Camera::updateZoom()
	{
		// Update the zoom (scale).
		//
		m_zoomRatio = lerp( m_zoomRatio, m_targetZoomRatio, m_zoomLerpAlpha );
		m_zoomRatio = clamp( m_zoomRatio, m_minZoomRatio, m_maxZoomRatio );
	}
	
	void Camera::updatePosition()
	{
		// Update the target position.
		//
		vec2 targetLoc = m_locTarget;
		if( m_target )
		{
			targetLoc = m_target->position();
		}
		
		// Move to or toward the target position.
		//
		if( m_isEasing )
		{
			// Easing.
			//
			setFocalPoint( lerp( m_focalPoint, targetLoc, m_lerpAlpha ));
		}
		else
		{
			// Not easing. Go straight there.
			setFocalPointToCurrentTarget();
		}		
	}

	void Camera::updateShake()
	{
		// Update view shake.
		//
		m_shakeVelocity -= m_shakeOffset * m_shakeStiffness;
		m_shakeOffset += m_shakeVelocity;
		m_shakeOffset -= m_shakeOffset * m_shakeDamping;
	}
	
	void Camera::updateRotation()
	{
		m_rotation = lerp( angle{ m_rotation }, angle{ m_targetRotation }, m_rotationLerp ).toDegrees< real >();
	}
	
	vec2 Camera::clampTargetLocation( const vec2& location ) const
	{
		const vec2 worldSpaceHalfScreenDimensions( m_screenHalfDimensions / m_zoomRatio );
		
		// Force focal point to stay within bounds.
		//
		vec2 result( location );
		if( m_rectBounds.width() >= 0 )
		{
			result.x = specialClamp( result.x, m_rectBounds.left() + worldSpaceHalfScreenDimensions.x, m_rectBounds.right() - worldSpaceHalfScreenDimensions.x );		
		}
		if( m_rectBounds.height() >= 0 )
		{
			result.y = specialClamp( result.y, m_rectBounds.top() + worldSpaceHalfScreenDimensions.y, m_rectBounds.bottom() - worldSpaceHalfScreenDimensions.y );
		}
		
		return result;
	}
}
