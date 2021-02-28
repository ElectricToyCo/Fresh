//
//  PropertyAbstract.cpp
//  Fresh
//
//  Created by Jeff Wofford on 2/23/13.
//
//

#include "PropertyAbstract.h"
#include "Object.h"
#include "ClassInfo.h"

namespace fr
{
	Object* PropertyAbstract::defaultObject() const
	{
		return m_originatingClass.defaultObject();
	}
	
	void PropertyAbstract::setDefaultValueByString( const std::string& strValue )
	{
		setValueByString( defaultObject(), strValue );
	}	

	std::string PropertyAbstract::getDefaultValueByString() const
	{
		const Object* theDefaultObject = defaultObject();
		ASSERT( theDefaultObject );
		return getValueByString( theDefaultObject );
	}
}
