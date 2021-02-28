//
//  EdTimelineSubjectDisplay.cpp
//  Fresh
//
//  Created by Jeff Wofford on 7/16/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "EdTimelineSubjectDisplay.h"
#include "TextField.h"
#include "Editor.h"

namespace fr
{	
	FRESH_DEFINE_CLASS_UNPLACEABLE( EdTimelineSubjectDisplay )
	DEFINE_VAR( EdTimelineSubjectDisplay, SmartPtr< TextField >, m_subjectNameText );
	DEFINE_VAR( EdTimelineSubjectDisplay, DisplayObjectContainer::ptr, m_timelineProper );
	DEFINE_VAR( EdTimelineSubjectDisplay, ClassInfo::cptr, m_childControlClass );
	DEFINE_VAR( EdTimelineSubjectDisplay, vec2, m_childControlOffset );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( EdTimelineSubjectDisplay )
	
	void EdTimelineSubjectDisplay::subject( DisplayObject::wptr subject_ )
	{
		m_subject = subject_;
		
		if( m_subject )
		{
			if( m_subjectNameText )
			{
				m_subjectNameText->text( m_subject->name() );
			}
			
			if( m_timelineProper )
			{
				m_timelineProper->visible( true );
				
				if( auto subjectAsContainer = subject_->as< DisplayObjectContainer >() )
				{
					setupTimeline( *subjectAsContainer );
				}
			}
		}
		else
		{
			if( m_subjectNameText )
			{
				m_subjectNameText->text( "" );
			}
			
			if( m_timelineProper )
			{
				m_timelineProper->visible( false );
			}
		}
	}
	
	void EdTimelineSubjectDisplay::onObjectsWereSelectedOrDeselected( const std::set< DisplayObject::wptr >& selectedSubjectChildren )
	{
		ASSERT( m_timelineProper );
		ASSERT( m_subject );

		if( auto subjectAsContainer = m_subject->as< DisplayObjectContainer >() )
		{
			if( subjectAsContainer->numChildren() != m_timelineProper->numChildren() )
			{
				setupTimeline( *subjectAsContainer );
			}
			
			for( size_t i = 0; i < subjectAsContainer->numChildren(); ++i )
			{
				auto timelineControl = m_timelineProper->getChildAt( i )->as< DisplayObjectContainer >();
				ASSERT( timelineControl );
				if( auto childName = timelineControl->getDescendantByName< TextField >( "_name", NameSearchPolicy::Substring ))
				{
					const auto& subjectChild = subjectAsContainer->getChildAt( i );
					
					Color nameColor( 0xcccccccc );
					
					if( selectedSubjectChildren.end() != selectedSubjectChildren.find( subjectChild ))
					{
						nameColor = 0xFFFFFFFF;
					}
					
					childName->color( nameColor );
				}
			}
		}
	}
	
	void EdTimelineSubjectDisplay::setupTimeline( const DisplayObjectContainer& subjectAsContainer )
	{
		ASSERT( m_timelineProper );
		
		m_timelineProper->removeChildren();
		
		const auto nChildren = subjectAsContainer.numChildren();
		
		for( size_t child = 0; child < nChildren; ++child )
		{
			addSubjectChildControl( *subjectAsContainer.getChildAt( child ));
		}
		
		repositionAllControls();
	}
	
	void EdTimelineSubjectDisplay::addSubjectChildControl( const DisplayObject& subjectChild )
	{
		ASSERT( m_childControlClass );
		ASSERT( m_timelineProper );
		ASSERT( m_subjectNameText );
		
		auto childControl = createObject< MovieClip >( *m_childControlClass );
		ASSERT( childControl );

		// Setup dragging.
		//
		childControl->addEventListener( EventTouch::TOUCH_BEGIN, FRESH_CALLBACK( onControlGripTouchBegin ));
		
		// Setup the name.
		//
		if( auto childName = childControl->getDescendantByName< TextField >( "_name", NameSearchPolicy::Substring ))
		{
			childName->text( subjectChild.name() );
			
			if( auto childNameButton = childName->parent()->as< SimpleButton >() )
			{
				childNameButton->addEventListener( TAPPED, FRESH_CALLBACK( onControlNameTapped ));
			}
		}

		// Setup the lock checkbox.
		//
		if( auto lock = childControl->getDescendantByName< UICheckbox >( "_lock", NameSearchPolicy::Substring ))
		{
			lock->checked( subjectChild.editorLocked() );
			lock->addEventListener( TAPPED, FRESH_CALLBACK( onControlLockTapped ));
		}
		
		// Setup the lock checkbox.
		//
		if( auto visibility = childControl->getDescendantByName< UICheckbox >( "_visibility", NameSearchPolicy::Substring ))
		{
			visibility->checked( subjectChild.visible() );
			visibility->addEventListener( TAPPED, FRESH_CALLBACK( onControlVisibilityTapped ));
		}
		
		m_timelineProper->addChild( childControl );
	}
	
	vec2 EdTimelineSubjectDisplay::controlPosition( size_t iChild ) const
	{
		return m_subjectNameText->position() + vec2( m_subjectNameText->bounds().width() + 4.0f, 0 ) +
			   m_childControlOffset * iChild;
	}
	
	void EdTimelineSubjectDisplay::repositionAllControls()
	{
		for( size_t i = 0; i < m_timelineProper->numChildren(); ++i )
		{
			m_timelineProper->getChildAt( i )->position( controlPosition( i ));
		}
	}
	
	FRESH_DEFINE_CALLBACK( EdTimelineSubjectDisplay, onControlGripTouchBegin, EventTouch )
	{
		auto control = event.currentTarget()->as< MovieClip >();
		ASSERT( control );
		
		dev_trace( *event.target() );

		if( event.target()->matchesFilters( "", ".+_grip" ))
		{
			control->doMoveWithDrag( true );
			control->addEventListener( DRAG_BEGIN, FRESH_CALLBACK( onControlGripDragBegin ));
			control->addEventListener( DRAG_MOVE , FRESH_CALLBACK( onControlGripDragMove ));
			control->addEventListener( DRAG_END  , FRESH_CALLBACK( onControlGripDragEnd ));
		}
		else
		{
			// Refuse dragging.
			//
			control->doMoveWithDrag( false );
		}
	}
	
	FRESH_DEFINE_CALLBACK( EdTimelineSubjectDisplay, onControlGripDragBegin, EventTouch )
	{
		auto control = event.currentTarget()->as< MovieClip >();
		ASSERT( control );
		
		dev_trace( *event.target() );
		
		m_controlDragStartPosition = control->position();
		control->color( 0x80808080 );
	}
	
	FRESH_DEFINE_CALLBACK( EdTimelineSubjectDisplay, onControlGripDragMove, EventTouch )
	{
		auto control = event.currentTarget()->as< MovieClip >();
		ASSERT( control );
		
		dev_trace( *control );

		const real lockedY = clamp( control->position().y, controlPosition( 0 ).y, controlPosition( m_timelineProper->numChildren() ).y );
		
		control->position( m_controlDragStartPosition.x, lockedY );
	}
	
	FRESH_DEFINE_CALLBACK( EdTimelineSubjectDisplay, onControlGripDragEnd, EventTouch )
	{
		auto subjectAsContainer = m_subject->as< DisplayObjectContainer >();
		ASSERT( subjectAsContainer );
		
		const auto nSubjectChildren = subjectAsContainer->numChildren();
		ASSERT( m_timelineProper->numChildren() == nSubjectChildren );
		
		auto control = event.currentTarget()->as< MovieClip >();
		ASSERT( control );

		dev_trace( *control );

		control->removeEventListener( DRAG_BEGIN, FRESH_CALLBACK( onControlGripDragBegin ));
		control->removeEventListener( DRAG_MOVE , FRESH_CALLBACK( onControlGripDragMove ));
		control->removeEventListener( DRAG_END  , FRESH_CALLBACK( onControlGripDragEnd ));
		
		auto childIndex = m_timelineProper->getChildIndex( control );
		ASSERT( childIndex < nSubjectChildren );
		
		dev_trace( this << " control " << control << " at index " << childIndex );
		
		control->color( Color::White );
		control->doMoveWithDrag( false );

		//
		// Place the control and child at the appropriate index.
		//
		
		// What index is wanted?
		//
		const real baseY = controlPosition( 0 ).y;
		const int indexDelta = int(( control->position().y - baseY ) / m_childControlOffset.y - real( childIndex ));
		
		const size_t desiredIndex = size_t( clamp( int( childIndex ) + indexDelta,
									   0,
									   int( nSubjectChildren ) - 1 ));
		
		ASSERT( desiredIndex <= subjectAsContainer->numChildren() );
		
		if( desiredIndex != childIndex )
		{
			auto subjectChild = subjectAsContainer->getChildAt( childIndex );
			subjectAsContainer->removeChildAt( childIndex );
		
			m_timelineProper->removeChildAt( childIndex );
			
			subjectAsContainer->addChildAt( subjectChild, desiredIndex );
			m_timelineProper->addChildAt( control, desiredIndex );
			
			// Let the editor know about this change.
			//
			auto editor = stage().as< Editor >();
			editor->saveHistoryState();
		}
		
		repositionAllControls();
	}
	
	FRESH_DEFINE_CALLBACK( EdTimelineSubjectDisplay, onControlNameTapped, EventTouch )
	{
		auto control = event.currentTarget()->as< DisplayObject >()->parent()->as< MovieClip >();
		ASSERT( control );
		
		auto childIndex = m_timelineProper->getChildIndex( control );
		ASSERT( childIndex < m_timelineProper->numChildren() );
		
		if( auto subjectAsContainer = m_subject->as< DisplayObjectContainer >() )
		{
			auto subjectChild = subjectAsContainer->getChildAt( childIndex );
			ASSERT( subjectChild );
			
			auto editor = stage().as< Editor >();
			ASSERT( editor );
			if( event.tapCount() == 1 )
			{
				editor->currentManipulator().toggleSelection( subjectChild );
			}
			else if( event.tapCount() == 2 )
			{
				editor->editChild( subjectChild );
			}
		}
	}

	FRESH_DEFINE_CALLBACK( EdTimelineSubjectDisplay, onControlLockTapped, EventTouch )
	{
		auto checkbox = event.currentTarget()->as< UICheckbox >();
		ASSERT( checkbox );
		
		auto control = checkbox->parent()->as< MovieClip >();
		ASSERT( control );
		
		auto childIndex = m_timelineProper->getChildIndex( control );
		ASSERT( childIndex < m_timelineProper->numChildren() );
		
		if( auto subjectAsContainer = m_subject->as< DisplayObjectContainer >() )
		{
			auto subjectChild = subjectAsContainer->getChildAt( childIndex );
			ASSERT( subjectChild );
		
			subjectChild->editorLocked( checkbox->checked() );
			
			stage().as< Editor >()->currentManipulator().deselect( subjectChild );
		}
	}
	
	FRESH_DEFINE_CALLBACK( EdTimelineSubjectDisplay, onControlVisibilityTapped, EventTouch )
	{
		auto checkbox = event.currentTarget()->as< UICheckbox >();
		ASSERT( checkbox );
		
		auto control = checkbox->parent()->as< MovieClip >();
		ASSERT( control );
		
		auto childIndex = m_timelineProper->getChildIndex( control );
		ASSERT( childIndex < m_timelineProper->numChildren() );
		
		if( auto subjectAsContainer = m_subject->as< DisplayObjectContainer >() )
		{
			auto subjectChild = subjectAsContainer->getChildAt( childIndex );
			ASSERT( subjectChild );
			
			subjectChild->visible( checkbox->checked() );
		}
	}
}

