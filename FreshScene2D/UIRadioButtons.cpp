//
//  UIRadioButtons.cpp
//  Fresh
//
//  Created by Jeff Wofford on 1/25/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "UIRadioButtons.h"

namespace fr
{
	
	FRESH_DEFINE_CLASS( UIRadioButtons )
	DEFINE_VAR( UIRadioButtons, size_t, m_creationSelection );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( UIRadioButtons )
	
	const char* UIRadioButtons::SELECTION_CHANGED = "Selection Changed";
	
	void UIRadioButtons::onAllLoaded()
	{
		Super::onAllLoaded();

		// Deal with creation settings.
		//
		selectChildButton( m_creationSelection );
	}
	
	size_t UIRadioButtons::numChildButtons() const
	{
		return std::count_if( begin(), end(), []( DisplayObject::ptr child )
				   {
					   return dynamic_freshptr_cast< UICheckbox::ptr >( child );
				   } );
	}
	
	size_t UIRadioButtons::selectedButtonIndex() const
	{
		return getIndexOfButton( m_selectedChild );
	}
	
	void UIRadioButtons::selectChildButton( size_t iChildButton )
	{
		selectChildButton( getButtonForIndex( iChildButton ));
	}
	
	UICheckbox::ptr UIRadioButtons::selectedChild() const
	{
		return m_selectedChild;
	}
	
	void UIRadioButtons::selectChildButton( UICheckbox::ptr selectedChild )
	{
		const size_t priorSelection = selectedButtonIndex();
		
		m_selectedChild = selectedChild;
		
		for( size_t i = 0; i < numChildren(); ++i )
		{
			UICheckbox::ptr childButton = dynamic_freshptr_cast< UICheckbox::ptr >( getChildAt( i ) );
			
			if( childButton )
			{
				childButton->checked( childButton == m_selectedChild );
			}
		}
		
		if( priorSelection != selectedButtonIndex() )
		{
			Event event( SELECTION_CHANGED, this );
			dispatchEvent( &event );
		}
	}

	size_t UIRadioButtons::getIndexOfButton( UICheckbox::ptr button ) const
	{
		size_t index = -1;
		
		if( button )
		{
			for( size_t i = 0; i < numChildren(); ++i )
			{
				UICheckbox::ptr childButton = dynamic_freshptr_cast< UICheckbox::ptr >( getChildAt( i ) );
				
				if( childButton )
				{
					++index;
					
					if( button == childButton )
					{
						break;
					}
				}
			}
		}
		return index;
	}
	
	UICheckbox::ptr UIRadioButtons::getButtonForIndex( size_t index ) const
	{
		size_t seekingIndex = -1;
		for( size_t i = 0; i < numChildren(); ++i )
		{
			UICheckbox::ptr childButton = dynamic_freshptr_cast< UICheckbox::ptr >( getChildAt( i ) );
			
			if( childButton )
			{
				++seekingIndex;
				
				if( index == seekingIndex )
				{
					return childButton;
				}
			}
		}
		return nullptr;
	}
	
	void UIRadioButtons::onAddedChild( DisplayObject::ptr child )
	{
		ASSERT( child );
		UICheckbox::ptr checkbox = dynamic_freshptr_cast< UICheckbox::ptr >( child );
		
		if( checkbox )
		{
			checkbox->addEventListener( SimpleButton::TAPPED, FRESH_CALLBACK( onChildTapped ));
		}
	}
	
	FRESH_DEFINE_CALLBACK( UIRadioButtons, onChildTapped, EventTouch )
	{
		UICheckbox::ptr checkbox = dynamic_freshptr_cast< UICheckbox::ptr >( event.currentTarget() );
		ASSERT( checkbox );
		
		selectChildButton( checkbox );
	}
}

