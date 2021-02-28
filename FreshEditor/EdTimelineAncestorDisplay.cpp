//
//  EdTimelineAncestorDisplay.cpp
//  Fresh
//
//  Created by Jeff Wofford on 7/16/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "EdTimelineAncestorDisplay.h"
#include "TextField.h"
#include "Editor.h"

namespace fr
{	
	FRESH_DEFINE_CLASS_UNPLACEABLE( EdTimelineAncestorDisplay )
	DEFINE_VAR( EdTimelineAncestorDisplay, ClassInfo::cptr, m_buttonClass );
	DEFINE_VAR( EdTimelineAncestorDisplay, ClassInfo::cptr, m_textClass );
	DEFINE_VAR( EdTimelineAncestorDisplay, vec2, m_buttonOffset );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( EdTimelineAncestorDisplay )
	
	void EdTimelineAncestorDisplay::populate( const DisplayObject& subject, DisplayObjectContainer::wptr baseAncestor )
	{
		REQUIRES( !baseAncestor || &subject == baseAncestor || subject.hasAncestor( baseAncestor ));

		clear();
		
		if( &subject == baseAncestor )
		{
			// Nothing to do.
			return;
		}
		
		// Gather ancestors.
		//
		std::vector< DisplayObjectContainer::wptr > ancestors;
		
		DisplayObjectContainer::ptr currentAncestor = subject.parent();
		while( currentAncestor )
		{
			ancestors.push_back( currentAncestor );
			
			// If we've now reached the baseAncestor, we're done.
			//
			if( baseAncestor == currentAncestor )
			{
				break;
			}
			
			currentAncestor = currentAncestor->parent();
		}
		
		// Reverse the order.
		//
		std::reverse( ancestors.begin(), ancestors.end() );
		
		// Create the controls.
		//
		std::for_each( ancestors.begin(), ancestors.end(), [&]( DisplayObjectContainer::wptr ancestor ) { addControlForAncestor( *ancestor ); } );
	}
	
	void EdTimelineAncestorDisplay::clear()
	{
		removeChildren();
		m_ancestors.clear();
	}
	
	void EdTimelineAncestorDisplay::addControlForAncestor( DisplayObjectContainer& ancestor )
	{
		// Add a button if we can.
		//
		if( m_buttonClass )
		{
			auto button = createObject< SimpleButton >( *m_buttonClass );
			if( !button )
			{
				dev_warning( this << " could not create SimpleButton of class " << m_buttonClass );
			}
			else
			{
				if( m_textClass )
				{
					auto text = createObject< TextField >( *m_textClass );
					if( !text )
					{
						dev_warning( this << " could not create TextField of class " << m_textClass );
					}
					else
					{
						// Everything is in order, so actually add and setup the button.
						//
						addChild( button );
						m_ancestors.push_back( &ancestor );		// Keeping a 1-to-1 relationship between this vector and the added buttons.
						
						button->addEventListener( SimpleButton::TAPPED, FRESH_CALLBACK( onAncestorButtonTapped ));
						button->position( m_buttonOffset * numChildren() );
						
						// Setup the text.
						//
						button->addChild( text );
						text->text( ancestor.name() );
					}
				}
			}
		}
	}
	
	FRESH_DEFINE_CALLBACK( EdTimelineAncestorDisplay, onAncestorButtonTapped, EventTouch )
	{
//		if( event.tapCount() == 2 )
		{
			// Move to editing this ancestor.
			
			auto displayObjectTarget = event.currentTarget()->as< DisplayObject >();
			
			auto ancestor = m_ancestors.at( getChildIndex( displayObjectTarget ));
			
			auto editor = stage().as< Editor >();
			ASSERT( editor );
			
			editor->editAncestor( ancestor );
		}
	}
}

