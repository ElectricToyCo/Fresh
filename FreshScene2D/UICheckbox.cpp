//
//  UICheckbox.cpp
//  Fresh
//
//  Created by Jeff Wofford on 1/25/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "UICheckbox.h"

namespace fr
{
	
	FRESH_DEFINE_CLASS( UICheckbox )
	DEFINE_VAR( UICheckbox, bool, m_checked );
	DEFINE_VAR( UICheckbox, MovieClip::ptr, m_checkHost );
	
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( UICheckbox )
	
	void UICheckbox::checked( bool c )
	{
		m_checked = c;
		
		std::string desiredStateLabel = m_checked ? "checked" : "unchecked";
		
		if( m_checkHost && m_checkHost->hasKeyframe( desiredStateLabel ))
		{
			m_checkHost->gotoAndStop( desiredStateLabel );
		}
	}
	
	void UICheckbox::onTapped( const EventTouch& event )
	{
		if( event.phase() == Event::Bubbling || event.phase() == Event::AtTarget )
		{
			if( enabled() )
			{
				// Toggle check mark.
				//
				checked( !m_checked );
			}
			Super::onTapped( event );
		}
	}
	
	void UICheckbox::onAllLoaded()
	{
		Super::onAllLoaded();
		
		checked( m_checked );
		
		if( m_checkHost && !hasChild( m_checkHost ))
		{
			addChild( m_checkHost );
		}
	}
	
}

