//
//  StringTable.cpp
//  Fresh
//
//  Created by Jeff Wofford on 4/17/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//

#include "StringTable.h"
#include "Property.h"
#include "FreshDebug.h"

namespace
{
	const std::string DEFAULT_STRING;
}

namespace fr
{	
	FRESH_DEFINE_CLASS( StringTable )
	DEFINE_VAR( StringTable, Table, m_table );

	StringTable::StringTable( fr::CreateInertObject c )
	:	Super( c )
	,	ObjectSingleton< StringTable >( nullptr )
	{}
	
	StringTable::StringTable( const ClassInfo& assignedClassInfo, NameRef objectName /* = DEFAULT_OBJECT_NAME */ )
	:	Object( assignedClassInfo, objectName )
	,	ObjectSingleton< StringTable >( this )
	{}
	
	bool StringTable::hasString( const StringTabulated& key )
	{
		ASSERT( doesExist() );
		return instance().localHasString( key );
	}
	
	const std::string& StringTable::string( const StringTabulated& key )
	{
		ASSERT( doesExist() );
		return instance().getString( key );
	}
	
	bool StringTable::localHasString( const StringTabulated& key ) const
	{
		return m_table.find( key ) != m_table.end();
	}
	
	const std::string& StringTable::getString( const StringTabulated& key )
	{
		auto iter = m_table.find( key );
		if( iter != m_table.end() )
		{
			return iter->second;
		}
		else
		{
			dev_warning( "Could not find string table string with key '" << key << "'." );
			return DEFAULT_STRING;
		}
	}
	
}

