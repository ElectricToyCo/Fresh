//
//  EdSelectionHarness.cpp
//  Fresh
//
//  Created by Jeff Wofford on 6/17/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "EdSelectionHarness.h"
#include "EdManipulator.h"

namespace fr
{	
	FRESH_DEFINE_CLASS_UNPLACEABLE( EdSelectionHarness )
	
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( EdSelectionHarness )
	
	void EdSelectionHarness::selection( DisplayObject::wptr s )
	{
		m_selection = s;
	}
}

