//
//  VersionNumberDisplay.h
//  Fresh
//
//  Created by Jeff Wofford on 1/24/15.
//  Copyright (c) 2015 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_VersionNumberDisplay_h
#define Fresh_VersionNumberDisplay_h

#include "DisplayObjectContainer.h"
#include "TextField.h"

namespace fr
{
	
	class VersionNumberDisplay : public DisplayObjectContainer
	{
		FRESH_DECLARE_CLASS( VersionNumberDisplay, DisplayObjectContainer );
	public:
		
		virtual void onTapped( const EventTouch& event ) override;
		
	private:
		
		VAR( TextField::ptr, m_text );
		
	};
	
}

#endif
