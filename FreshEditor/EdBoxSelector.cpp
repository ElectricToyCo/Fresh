//
//  EdBoxSelector.cpp
//  Fresh
//
//  Created by Jeff Wofford on 6/3/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "EdBoxSelector.h"

namespace fr
{
	
	FRESH_DEFINE_CLASS_UNPLACEABLE( EdBoxSelector )
	
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( EdBoxSelector )
	
	EdBoxSelector::EdBoxSelector( const fr::ClassInfo& assignedClassInfo, fr::Object::NameRef objectName )
	:	Super( assignedClassInfo, objectName )
	{
		visible( false );
		isTouchEnabled( false );
	}
	
	void EdBoxSelector::startPoint( const vec2& p )
	{
		m_startPoint = p;
		onPointChanged();
	}
	void EdBoxSelector::endPoint( const vec2& p )
	{
		m_endPoint = p;
		onPointChanged();
	}
	
	rect EdBoxSelector::selectionRectangle() const
	{
		return rect( std::min( m_startPoint.x, m_endPoint.x ),
					std::min( m_startPoint.y, m_endPoint.y ),
					std::max( m_startPoint.x, m_endPoint.x ),
					std::max( m_startPoint.y, m_endPoint.y ));
	}
	
	void EdBoxSelector::onPointChanged()
	{
		auto& myGraphics = graphics();
		
		myGraphics.clear();
		
		myGraphics.lineStyle( 0x77001077 );
		myGraphics.beginFill( 0x55444455 );
		
		myGraphics.drawRect( selectionRectangle() );
		
		myGraphics.endFill();
	}
		
}

