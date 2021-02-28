//
//  UISliderKnob.cpp
//  Fresh
//
//  Created by Jeff Wofford on 6/5/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "UISliderKnob.h"
#include "Property.h"

namespace fr
{
	
	FRESH_DEFINE_CLASS( UISliderKnob )	
	DEFINE_VAR( UISliderKnob, int, m_slidingAxis );
	DEFINE_VAR( UISliderKnob, Range< real >, m_visualRange);
	DEFINE_VAR( UISliderKnob, Range< real >, m_valueRange );
	DEFINE_VAR( UISliderKnob, real, m_step );
	DEFINE_VAR( UISliderKnob, Object::wptr, m_onChangeCallee );
	DEFINE_VAR( UISliderKnob, std::string, m_onChangeMethodExpression );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( UISliderKnob )
	
	UISliderKnob::UISliderKnob( const fr::ClassInfo& assignedClassInfo, fr::Object::NameRef objectName )
	:	Super( assignedClassInfo, objectName )
	{
		isDragEnabled( true );
		doMoveWithDrag( true );
		minStageDistanceForDrag( 1.0f );
		minRealDurationForDrag( 0.25 );
	}
	
	real UISliderKnob::value() const
	{
		real result = lerp( m_valueRange, proportion() );

		if( m_step > 0 )
		{
			result = roundToNearest( result, m_step );
		}

		return clamp( result, std::min( m_valueRange.min, m_valueRange.max ), std::max( m_valueRange.min, m_valueRange.max ) );
	}
	
	real UISliderKnob::proportion() const
	{
		return fr::proportion( m_position[ slidingAxis() ], m_visualRange );
	}
	
	void UISliderKnob::value( real x )
	{
		proportion( fr::proportion( x, m_valueRange ));
	}
	
	void UISliderKnob::proportion( real x )
	{
		if( x < 0 || x > 1.0f )
		{
			dev_warning( this << " proportion " << x << " out of range." );
		}
		x = clamp( x, 0.0f, 1.0f );
		
		const int axis = slidingAxis();
		m_position[ axis ] = lerp( m_visualRange, x );
	}

	void UISliderKnob::onDragMove( const EventTouch& event )
	{
		// Superceding super class implementation.
		const auto oldValue = value();
		
		if( doMoveWithDrag() )
		{
			position( localToParent( event.location() - dragLocalStartLocation() ));
		}
		
		const int axis = slidingAxis();
		const int otherAxis = ( axis + 1 ) & 1;
		
		// Lock to sliding axis.
		//
		m_position[ otherAxis ] = 0;
		
		// Lock to visual limits.
		//
		m_position[ axis ] = clamp( m_position[ axis ], m_visualRange.min, m_visualRange.max );
		
		const auto newValue = value();
		if( oldValue != newValue )
		{
			onValueChanged( newValue );
		}
		
		EventTouch dragEvent( DRAG_MOVE, event.touchId(), event.touchPhase(), event.tapCount(), event.location(), event.previousLocation(), event.nTouches(), event.iThisTouch(), event.wheelDelta(), event.target(), this );
		dispatchEvent( &dragEvent );
	}
	
	void UISliderKnob::onValueChanged( real newValue )
	{
		if( m_onChangeCallee )
		{
			std::istringstream methodExpression( createString( m_onChangeMethodExpression << " " << newValue ));
			std::string methodName;
			methodExpression >> methodName;
			trim( methodName );
			if( !methodName.empty() )
			{
				m_onChangeCallee->call( methodName, methodExpression );
			}
			else
			{
				dev_warning( this << " has on change callee " << m_onChangeCallee << " but no method expression." );
			}
		}
	}
}

