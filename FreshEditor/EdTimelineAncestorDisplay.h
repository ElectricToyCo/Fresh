//
//  EdTimelineAncestorDisplay.h
//  Fresh
//
//  Created by Jeff Wofford on 7/16/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_EdTimelineAncestorDisplay_h
#define Fresh_EdTimelineAncestorDisplay_h

#include "SimpleButton.h"

namespace fr
{
	
	class EdTimelineAncestorDisplay : public DisplayObjectContainer
	{
		FRESH_DECLARE_CLASS( EdTimelineAncestorDisplay, DisplayObjectContainer );
	public:
		
		virtual void populate( const DisplayObject& subject, DisplayObjectContainer::wptr baseAncestor = nullptr );
		// Populates the display with the ancestors of subject. subject itself will not appear.
		// if baseAncestor != null, no ancestor "deeper" than baseAncestor will be shown.
		
		void clear();
		
	protected:
		
		void addControlForAncestor( DisplayObjectContainer& ancestor );
		
	private:
		
		VAR( ClassInfo::cptr, m_buttonClass );
		VAR( ClassInfo::cptr, m_textClass );
		VAR( vec2, m_buttonOffset );

		std::vector< DisplayObjectContainer::wptr > m_ancestors;
		
		FRESH_DECLARE_CALLBACK( EdTimelineAncestorDisplay, onAncestorButtonTapped, EventTouch );
	};
	
}

#endif
