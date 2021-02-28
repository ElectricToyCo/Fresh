//
//  UIRadioButtons.h
//  Fresh
//
//  Created by Jeff Wofford on 1/25/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_UIRadioButtons_h
#define Fresh_UIRadioButtons_h

#include "UICheckbox.h"

namespace fr
{
	
	class UIRadioButtons : public DisplayObjectContainer
	{
	public:
		
		static const char* SELECTION_CHANGED;
		
		size_t numChildButtons() const;
		
		size_t selectedButtonIndex() const;					// May return size_t( -1 ), meaning nothing is selected.
		void selectChildButton( size_t iChildButton );		// If out of range, deselects all.

		UICheckbox::ptr selectedChild() const;				// May return nullptr.
		void selectChildButton( UICheckbox::ptr child );	// If !child, deselects all.

		virtual void onAllLoaded() override;

	protected:
		
		virtual void onAddedChild( DisplayObject::ptr child ) override;
		
		size_t getIndexOfButton( UICheckbox::ptr button ) const;
		UICheckbox::ptr getButtonForIndex( size_t index ) const;
		
	private:
		
		DVAR( size_t, m_creationSelection, 0 );		// Use -1 to mean nothing is selected.
		UICheckbox::ptr m_selectedChild;
		
		FRESH_DECLARE_CALLBACK( UIRadioButtons, onChildTapped, EventTouch );
		
		FRESH_DECLARE_CLASS( UIRadioButtons, DisplayObjectContainer );
	};
	
}

#endif
