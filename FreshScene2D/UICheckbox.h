//
//  UICheckbox.h
//  Fresh
//
//  Created by Jeff Wofford on 1/25/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_UICheckbox_h
#define Fresh_UICheckbox_h

#include "SimpleButton.h"

namespace fr
{
	
	class UICheckbox : public SimpleButton
	{
	public:
		
		SYNTHESIZE_GET( bool, checked );
		void checked( bool c );
		
		virtual void onAllLoaded() override;
		
	protected:
		
		virtual void onTapped( const EventTouch& event ) override;
		
	private:
		
		DVAR( bool, m_checked, false );
		VAR( MovieClip::ptr, m_checkHost );
		
		FRESH_DECLARE_CLASS( UICheckbox, SimpleButton );
	};
	
}

#endif
