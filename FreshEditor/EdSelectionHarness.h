//
//  EdSelectionHarness.h
//  Fresh
//
//  Created by Jeff Wofford on 6/17/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_EdSelectionHarness_h
#define Fresh_EdSelectionHarness_h

#include "DisplayObjectContainer.h"

namespace fr
{
	class Manipulator;
	
	class EdSelectionHarness : public DisplayObjectContainer
	{
		FRESH_DECLARE_CLASS( EdSelectionHarness, DisplayObjectContainer );
	public:
		
		SYNTHESIZE( WeakPtr< Manipulator >, manipulator );
		
		SYNTHESIZE_GET( DisplayObject::wptr, selection );
		virtual void selection( DisplayObject::wptr s );
		
		virtual void updateToSelection() {}
		virtual void updateFromSelection() {}

		
	private:
		
		DisplayObject::wptr m_selection;
		WeakPtr< Manipulator > m_manipulator;
		
	};
	
}

#endif
