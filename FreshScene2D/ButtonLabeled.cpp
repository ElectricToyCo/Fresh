/*
 *  ButtonLabeled.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 7/9/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#include "ButtonLabeled.h"
#include "Objects.h"

namespace fr
{
	FRESH_DEFINE_CLASS( ButtonLabeled )

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( ButtonLabeled )

	void ButtonLabeled::setLabelDisabled( DisplayObject* label )
	{
		setLabel( "disabled", label );
	}

	void ButtonLabeled::setLabelOut( DisplayObject* label )
	{
		setLabel( "out", label );
	}

	void ButtonLabeled::setLabelDown( DisplayObject* label )
	{
		setLabel( "down", label );
	}

	void ButtonLabeled::setLabelUp( DisplayObject* label )
	{
		setLabel( "up", label );
	}

	void ButtonLabeled::setLabel( const std::string& keyframeName, DisplayObject* label )
	{
		ASSERT( !getChildByName( label->name(), NameSearchPolicy::ExactMatch ) );		// Label object must have unique id.
		ASSERT( hasKeyframe( keyframeName ));							// Call this function after setting up keyframes.
		
		addChild( label );	
		
		Keyframe& keyframe = getKeyframe( keyframeName );
		keyframe.setChildState( *label );		// Just make sure this child is shown in the way it is now situated.
		
		label->setKeyframeVisible( false );
	}
}
