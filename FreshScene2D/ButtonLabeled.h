/*
 *  ButtonLabeled.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 7/9/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_BUTTON_LABELED_H_INCLUDED
#define FRESH_BUTTON_LABELED_H_INCLUDED

#include "SimpleButton.h"

namespace fr 
{
	
	// A ButtonLabeled is a button with an "inner" DisplayObject rendered as a child in front of the button itself. Typically this would be useful for
	// label text or an icon image. Multiple, distinct, DisplayObjects may be specified for different button states so that the label will automatically change appearance
	// as the user interacts with the button.
	//
	class ButtonLabeled : public SimpleButton
	{
	public:
		
		// In the following SetLabel*() functions, label must be a unique object with a unique name relative to the other labels and children of this button.
		//
		virtual void setLabelDisabled( DisplayObject* label );
		virtual void setLabelOut( DisplayObject* label );		
		virtual void setLabelDown( DisplayObject* label );		
		virtual void setLabelUp( DisplayObject* label );		
		
	protected:
		
		virtual void setLabel( const std::string& keyframeName, DisplayObject* label );
		
	private:
		
		FRESH_DECLARE_CLASS( ButtonLabeled, SimpleButton )
	};
}

#endif
