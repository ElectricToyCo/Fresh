//
//  UISliderKnob.h
//  Fresh
//
//  Created by Jeff Wofford on 6/5/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_UISliderKnob_h
#define Fresh_UISliderKnob_h

#include "SimpleButton.h"

namespace fr
{
	
	class UISliderKnob : public SimpleButton
	{
	public:
		
		real value() const;
		real proportion() const;
		real percent() const			{ return proportion() * 100.0f; }
		
		void value( real x );
		void proportion( real x );
		void percent( real x )			{ proportion( x / 100.0f ); }
		
		SYNTHESIZE_SET( int, slidingAxis );
		int slidingAxis() const			{ return ( m_slidingAxis < 0 || m_slidingAxis > 1 ) ? 0 : m_slidingAxis; }
		
		SYNTHESIZE( Range< real >, visualRange );
		SYNTHESIZE( Range< real >, valueRange );
		SYNTHESIZE( real, step );

		virtual void onDragMove( const EventTouch& event ) override;

	protected:
		
		virtual void onValueChanged( real newValue );
		
	private:
		
		DVAR( int, m_slidingAxis, 0 );		// The axis along which the knob may slide. 0 => X, 1 => Y. If not 0 or 1, taken to be 0.
		DVAR( Range< real >, m_visualRange, Range< real >( 0.0f, 100.0f ));
		DVAR( Range< real >, m_valueRange, Range< real >( 0.0f, 1.0f ));
		DVAR( real, m_step, 0.0f );	// 0.0f means no stepping.
		VAR( Object::wptr, m_onChangeCallee );
		VAR( std::string, m_onChangeMethodExpression );
		
		FRESH_DECLARE_CLASS( UISliderKnob, SimpleButton );
	};
	
}

#endif
