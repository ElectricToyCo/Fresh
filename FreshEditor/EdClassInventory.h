//
//  EdClassInventory.h
//  Fresh
//
//  Created by Jeff Wofford on 1/20/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_EdClassInventory_h
#define Fresh_EdClassInventory_h

#include "Sprite.h"
#include "ClassFilter.h"

namespace fr
{

	class EdClassSelector;
	class EdClassInventory : public Sprite
	{
	public:
		
		virtual void refresh();
		
		ClassInfo::NameRef getSelectedClassName() const;
		void setSelectedClassName( ClassInfo::NameRef classInfo );
		
	protected:
	
		SmartPtr< EdClassSelector > getRootSelector();
		void addClass( const ClassInfo& classInfo );
		
		virtual void onAddedToStage() override;
		
		virtual bool isAllowed( ClassInfo::NameRef className ) const;
		
	private:
		
		ClassInfo::Name m_selectedClassName;

		DVAR( ClassInfo::cptr, m_rootClass, &DisplayObject::StaticGetClassInfo() );
		VAR( ClassFilter::ptr, m_classFilter );
		
		FRESH_DECLARE_CALLBACK( EdClassInventory, onSelectorTapped, Event );

		FRESH_DECLARE_CLASS( EdClassInventory, Sprite );
	};
	
}

#endif
