//
//  DisplayPackage.cpp
//  Fresh
//
//  Created by Jeff Wofford on 3/4/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "DisplayPackage.h"
#include "DisplayObject.h"
#include "CommandProcessor.h"

namespace fr
{
	
	FRESH_DEFINE_CLASS( DisplayPackage )
	
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( DisplayPackage )
	
	void DisplayPackage::root( SmartPtr< DisplayObject > root )
	{
		m_root = root;
		
		if( m_root && !has( m_root ))
		{
			add( m_root );
		}
	}

	void DisplayPackage::writeRootElement( std::ostream& out ) const
	{
		out << "<objects root=\"";
		if( m_root )
		{
			out << m_root;
		}
		else
		{
			out << "'null'";
		}
		out << "\">\n";
	}
	
	std::vector< Object::ptr > DisplayPackage::loadFromManifest( const Manifest& manifest )
	{
		auto objects = Super::loadFromManifest( manifest );
		
		DisplayObject::ptr frontAsDisplayObject = objects.empty() ? nullptr : objects.front()->as< DisplayObject >();
		
		if( frontAsDisplayObject )
		{
			root( frontAsDisplayObject );
		}
		else
		{
			con_error( "DisplayPackage " << name() << " contained no display objects!" );
		}
		
		return objects;
	}
}

