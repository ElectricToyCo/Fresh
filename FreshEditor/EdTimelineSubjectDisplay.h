//
//  EdTimelineSubjectDisplay.h
//  Fresh
//
//  Created by Jeff Wofford on 7/16/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_EdTimelineSubjectDisplay_h
#define Fresh_EdTimelineSubjectDisplay_h

#include "MovieClip.h"

namespace fr
{
	class TextField;
	
	class EdTimelineSubjectDisplay : public MovieClip
	{
		FRESH_DECLARE_CLASS( EdTimelineSubjectDisplay, MovieClip );
	public:
		
		SYNTHESIZE_GET( DisplayObject::wptr, subject )
		void subject( DisplayObject::wptr subject_ );
		
		void onObjectsWereSelectedOrDeselected( const std::set< DisplayObject::wptr >& selectedSubjectChildren );

	protected:
		
		virtual void setupTimeline( const DisplayObjectContainer& subjectAsContainer );
		virtual void addSubjectChildControl( const DisplayObject& subjectChild );

		vec2 controlPosition( size_t iChild ) const;
		
		void repositionAllControls();

	private:

		VAR( SmartPtr< TextField >, m_subjectNameText );
		VAR( DisplayObjectContainer::ptr, m_timelineProper );
		VAR( ClassInfo::cptr, m_childControlClass );
		VAR( vec2, m_childControlOffset );
		
		DisplayObject::wptr m_subject;

		DisplayObject::wptr m_touchStartTarget;
		vec2 m_controlDragStartPosition;
		
		FRESH_DECLARE_CALLBACK( EdTimelineSubjectDisplay, onControlGripTouchBegin, EventTouch );
		FRESH_DECLARE_CALLBACK( EdTimelineSubjectDisplay, onControlGripDragBegin, EventTouch );
		FRESH_DECLARE_CALLBACK( EdTimelineSubjectDisplay, onControlGripDragMove, EventTouch );
		FRESH_DECLARE_CALLBACK( EdTimelineSubjectDisplay, onControlGripDragEnd, EventTouch );
		FRESH_DECLARE_CALLBACK( EdTimelineSubjectDisplay, onControlNameTapped, EventTouch );
		FRESH_DECLARE_CALLBACK( EdTimelineSubjectDisplay, onControlLockTapped, EventTouch );
		FRESH_DECLARE_CALLBACK( EdTimelineSubjectDisplay, onControlVisibilityTapped, EventTouch );		
	};
	
}

#endif
