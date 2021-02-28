//
//  EdPointList.cpp
//  Fresh
//
//  Created by Jeff Wofford on 6/15/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "EdPointList.h"

namespace
{
	using namespace fr;
	
	vec2 transformPoint( const DisplayObject& from, const DisplayObject& to, const vec2& point )
	{
		if( !&from || !&to || &from == &to )
		{
			return point;
		}
		else
		{
			return to.globalToLocal( from.localToGlobal( point ));
		}
	}
	
	vec2 transformScale( const DisplayObject& from, const DisplayObject& to, const vec2& scale )
	{
		if( !&from || !&to || &from == &to )
		{
			return scale;
		}
		else
		{
			return to.globalToLocalScale( from.localToGlobalScale( scale ));
		}
	}
}

namespace fr
{	
	FRESH_DEFINE_CLASS_UNPLACEABLE( EdPointList )
	DEFINE_VAR( EdPointList, DisplayObject::wptr, m_subject );
	DEFINE_VAR( EdPointList, ClassInfo::cptr, m_classPointProxy );
	DEFINE_VAR( EdPointList, vec2, m_proxyScale );
	DEFINE_VAR( EdPointList, Color, m_lineColor );
	DEFINE_VAR( EdPointList, Color, m_fillColor );
	DEFINE_VAR( EdPointList, bool, m_isLineLoop );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( EdPointList )

	EdPointList::EdPointList( const fr::ClassInfo& assignedClassInfo, fr::Object::NameRef objectName )
	:	Super( assignedClassInfo, objectName )
	,	m_markup( createObject< Sprite >())
	{
		addChild( m_markup );
		m_markup->isTouchEnabled( false );
	}

	EdPointList::~EdPointList()
	{
		clearPoints();
	}
	
	void EdPointList::clearPoints()
	{
		// Clear all children but the markup.
		//
		while( numChildren() >= 2 )
		{
			removeChildAt( 1 );
		}
		
		drawLines();
	}
	
	void EdPointList::addPoint( const vec2& point )
	{
		setupDefaults();

		ASSERT( m_fnTransformPoint );
		
		if( m_classPointProxy )
		{
			auto proxy = createObject< DisplayObject >( *m_classPointProxy );
			
			if( proxy )
			{
				addChild( proxy );
				proxy->position( m_fnTransformPoint( *m_subject, *this, point ));
				
				for( const auto& callback : m_proxyCallbacks )
				{
					proxy->addEventListener( callback.first, callback.second );
				}
				
				drawLines();
			}
			else
			{
				dev_warning( this << " could not create proxy of class '" << m_classPointProxy << "'." );
			}
		}
		else
		{
			dev_warning( this << " had no classPointProxy." );
		}
	}

	void EdPointList::addProxyEventListener( Event::TypeRef type, CallbackFunctionAbstract::ptr fnCallback )
	{
		for( auto proxy : *this )
		{
			proxy->addEventListener( type, fnCallback );
		}
		m_proxyCallbacks.push_back( std::make_pair( type, fnCallback ));
	}
	
	void EdPointList::update()
	{
		setupDefaults();
		
		if( !m_subject )
		{
			return;
		}
		
		// Update the scale for each proxy.
		//
		setupDefaults();
		if( m_fnTransformScale )
		{
			for( size_t i = 1; i < numChildren(); ++i )
			{
				auto child = getChildAt( i );
				
				child->scale( m_fnTransformScale( *this, *m_subject, m_proxyScale ));
			}
		}
		
		drawLines();
		
		Super::update();
	}

	void EdPointList::drawLines()
	{
		setupDefaults();
		
		if( m_markup )
		{
			auto& myGraphics = m_markup->graphics();
			myGraphics.clear();
			
			if( numChildren() > 2 )
			{
				myGraphics.lineStyle( m_lineColor );
				
				if( m_fillColor.getA() > 0 )
				{
					myGraphics.beginFill( m_fillColor );
				}
				
				bool first = true;
				for( size_t i = 1; i < numChildren(); ++i )
				{
					auto proxy = getChildAt( i );
					const auto& pos = proxy->position();
					
					if( first )
					{
						myGraphics.moveTo( pos );
						first = false;
					}
					else
					{
						myGraphics.lineTo( pos );
					}
				}
				
				if( m_isLineLoop && numChildren() > 3 )
				{
					myGraphics.lineTo( getChildAt( 1 )->position() );
				}
				
				if( m_fillColor.getA() > 0 )
				{
					myGraphics.endFill();
				}
			}
		}
	}
	
	std::vector< vec2 > EdPointList::points() const
	{
		std::vector< vec2 > thePoints;
		forEachPoint( [&thePoints] ( const vec2& point ) { thePoints.push_back( point ); } );
		return thePoints;
	}
	
	void EdPointList::setupDefaults()
	{
		if( !m_fnTransformPoint )
		{
			m_fnTransformPoint = std::bind( transformPoint, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 );
		}
		if( !m_fnTransformScale )
		{
			m_fnTransformScale = std::bind( transformScale, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 );
		}
	}
}

