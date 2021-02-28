//
//  EdTimeline.h
//  Fresh
//
//  Created by Jeff Wofford on 7/16/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_EdTimeline_h
#define Fresh_EdTimeline_h

#include "MovieClip.h"

namespace fr
{
	class EdTimelineAncestorDisplay;
	class EdTimelineSubjectDisplay;
	
	class EdTimeline : public MovieClip
	{
		FRESH_DECLARE_CLASS( EdTimeline, MovieClip );
	public:
		
		void setup( DisplayObject::wptr subject, DisplayObjectContainer::wptr base );

		void onObjectsWereSelectedOrDeselected( const std::set< DisplayObject::wptr >& selectedSubjectChildren );

	private:
		
		VAR( SmartPtr< EdTimelineAncestorDisplay >, m_ancestorDisplay );
		VAR( SmartPtr< EdTimelineSubjectDisplay >, m_subjectDisplay );
		
	};
	
}

#endif
