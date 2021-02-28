//
//  EdObjectBrowser.h
//  Fresh
//
//  Created by Jeff Wofford on 6/8/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_EdObjectBrowser_h
#define Fresh_EdObjectBrowser_h

#include "UIDisplayGrid.h"
#include "SimpleButton.h"
#include "TextField.h"

namespace fr
{
	class EdObjectPreview : public SimpleButton
	{
		FRESH_DECLARE_CLASS( EdObjectPreview, SimpleButton );
	public:

		SYNTHESIZE( Object::wptr, forObject );

	private:
		
		Object::wptr m_forObject;
	};
	
	/////////////////////////////////////////////////////////////////////////////
	
	class EdObjectBrowser : public DisplayObjectContainer
	{
		FRESH_DECLARE_CLASS( EdObjectBrowser, DisplayObjectContainer );
	public:
		
		static const char* BROWSER_OBJECT_TAPPED;
		
		virtual void refresh();
		virtual void populate( const ObjectId& filters );
		void clear();
		
		void selectedObject( Object::ptr object );	// May be null.
		
		SYNTHESIZE_GET( EventDispatcher::wptr, delegate )
		
	protected:
		
		virtual EdObjectPreview::ptr createPreview( Object& forObject ) const;
		virtual DisplayObject::ptr createStandIn( Object& forObject ) const;
		
	private:
		
		VAR( TextField::ptr, m_caption );
		VAR( UIDisplayGrid::ptr, m_displayGrid );
		DVAR( ClassInfo::cptr, m_classPreview, &EdObjectPreview::StaticGetClassInfo() );
		DVAR( ClassInfo::cptr, m_classStandIn, &Sprite::StaticGetClassInfo() );
		VAR( ObjectId, m_currentFilter );
		EventDispatcher::wptr m_delegate;
		
		std::vector< Object::wptr > m_referencedObjects;	// One-to-one correspondance with children of m_displayGrid.

		FRESH_DECLARE_CALLBACK( EdObjectBrowser, onPreviewTapped, EventTouch );
	};
	
}

#endif
