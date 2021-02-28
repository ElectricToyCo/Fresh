//
//  VersionNumberDisplay.cpp
//  Fresh
//
//  Created by Jeff Wofford on 1/24/15.
//  Copyright (c) 2015 Jeff Wofford. All rights reserved.
//

#include "VersionNumberDisplay.h"
#include "FreshVersioning.h"

namespace fr
{	
	FRESH_DEFINE_CLASS( VersionNumberDisplay )
	DEFINE_VAR( VersionNumberDisplay, TextField::ptr, m_text );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( VersionNumberDisplay )
	
	void VersionNumberDisplay::onTapped( const EventTouch& event )
	{
		if( m_text )
		{
			m_text->text( version::info() );
		}
		
		color( Color::White );
		Super::onTapped( event );
	}
	
}

