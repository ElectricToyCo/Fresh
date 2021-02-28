//
//  EdGroup.cpp
//  Fresh
//
//  Created by Jeff Wofford on 6/10/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "EdGroup.h"

namespace fr
{
	FRESH_DEFINE_CLASS_UNPLACEABLE( EdGroup )
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( EdGroup )
	
	void EdGroup::take( DisplayObject::ptr newChild )
	{
		// Moves newChild from its existing parent (if any) into this object.
		REQUIRES( newChild );
		REQUIRES( this != newChild );
		REQUIRES( !hasDescendant( newChild ));
		
		if( auto childsParent = newChild->parent() )
		{
			childsParent->removeChild( newChild );
		}
		addChild( newChild );
		
		PROMISES( hasChild( newChild ));
	}
	
	void EdGroup::onBeginPlay()
	{
		if( doUpdate() )
		{
			disgorge();
		}
		Super::onBeginPlay();
	}
	
}

