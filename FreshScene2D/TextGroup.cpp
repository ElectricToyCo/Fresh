//
//  TextGroup.cpp
//  Fresh
//
//  Created by Jeff Wofford on 8/16/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "TextGroup.h"
#include "TextField.h"

namespace fr
{	
	FRESH_DEFINE_CLASS( TextGroup )
	DEFINE_VAR( TextGroup, std::string, m_text );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( TextGroup )
	
	void TextGroup::text( const std::string& text_ )
	{
		m_text = text_;
		propagateText();
	}
	
	void TextGroup::propagateText()
	{
		// Update to all text descendants.
		//
		forEachDescendant< TextField >( [&]( TextField& field )
									   {
										   field.text( m_text );
									   } );
	}
	
	void TextGroup::update()
	{
		propagateText();
		Super::update();
	}
}

