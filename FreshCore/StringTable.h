//
//  StringTable.h
//  Fresh
//
//  Created by Jeff Wofford on 4/17/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_StringTable_h
#define Fresh_StringTable_h

#include "Object.h"
#include "PropertyAbstract.h"
#include "ObjectSingleton.h"
#include "StringTabulated.h"
#include <map>

namespace fr
{
	
	class StringTable : public Object, public ObjectSingleton< StringTable >
	{
		FRESH_DECLARE_CLASS( StringTable, Object );
	public:

		static bool hasString( const StringTabulated& key );
		static const std::string& string( const StringTabulated& key );

	protected:
		
		bool localHasString( const StringTabulated& key ) const;
		const std::string& getString( const StringTabulated& key );
		
	private:
		
		typedef std::map< StringTabulated, std::string > Table;
		
		// Doing manually what VAR usually does, because we can't include Property.h due to cyclical dependencies.
		//
		VAR( Table, m_table );
	};
	
}

#endif
