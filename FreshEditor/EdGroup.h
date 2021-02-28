//
//  EdGroup.h
//  Fresh
//
//  Created by Jeff Wofford on 6/10/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_EdGroup_h
#define Fresh_EdGroup_h

#include "DisplayObjectContainer.h"

namespace fr
{
	
	class EdGroup : public DisplayObjectContainer
	{
		FRESH_DECLARE_CLASS( EdGroup, DisplayObjectContainer );
	public:
		
		void take( DisplayObject::ptr newChild );
		// Moves newChild from its existing parent (if any) into this object.
		// REQUIRES( newChild );
		// REQUIRES( this != newChild );
		// REQUIRES( !hasDescendant( newChild ));
		// PROMISES( has( newChild ));
		
		virtual void onBeginPlay() override;
	};
	
}

#endif
