//
//  EdTimeline.cpp
//  Fresh
//
//  Created by Jeff Wofford on 7/16/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "EdTimeline.h"
#include "EdTimelineAncestorDisplay.h"
#include "EdTimelineSubjectDisplay.h"

namespace fr
{	
	FRESH_DEFINE_CLASS_UNPLACEABLE( EdTimeline )
	DEFINE_VAR( EdTimeline, SmartPtr< EdTimelineAncestorDisplay >, m_ancestorDisplay );
	DEFINE_VAR( EdTimeline, SmartPtr< EdTimelineSubjectDisplay >, m_subjectDisplay );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( EdTimeline )
	
	void EdTimeline::setup( DisplayObject::wptr subject, DisplayObjectContainer::wptr base )
	{
		if( m_ancestorDisplay )
		{
			if( subject )
			{
				m_ancestorDisplay->populate( *subject, base );
			}
			else
			{
				m_ancestorDisplay->clear();
			}
		}
		
		if( m_subjectDisplay )
		{
			vec2 subjectDisplayPosition;
			
			rect subjectFrame = frame();
			
			if( m_ancestorDisplay )
			{
				real ancestorRight = m_ancestorDisplay->bounds().right();
				if( isInfinite( ancestorRight ))
				{
					ancestorRight = subjectFrame.left() + 4;		// TODO Magic
				}
				
				subjectDisplayPosition = vec2( ancestorRight + 2, 0 );
			}
			
			m_subjectDisplay->position( subjectDisplayPosition );
			
			// Set the subject frame to the available space.
			//
			// TODO
//			subjectFrame.right( subjectFrame.right() - ( subjectFrame.left() - m_subjectDisplay->position().x ));
			m_subjectDisplay->frame( subjectFrame );
			
			m_subjectDisplay->subject( subject );
		}
	}

	void EdTimeline::onObjectsWereSelectedOrDeselected( const std::set< DisplayObject::wptr >& selectedSubjectChildren )
	{
		ASSERT( m_subjectDisplay );
		m_subjectDisplay->onObjectsWereSelectedOrDeselected( selectedSubjectChildren );
	}
}

