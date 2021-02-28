/*
 *  ObjectId.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 10/30/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#include "ObjectId.h"
#include "FreshDebug.h"
#include "FreshEssentials.h"
#include "CommandProcessor.h"
#include "Object.h"

namespace
{
	using namespace fr;
	
	const char PACKAGE_DELIMITER[] = "::";
	
	void divideObjectNameToObjectAndPackageName( std::string& inOutObjectName, std::string& outPackageName )
	{
		size_t iPackageDelimiter = inOutObjectName.find( PACKAGE_DELIMITER );
		if( iPackageDelimiter != std::string::npos )
		{
			outPackageName = inOutObjectName.substr( 0, iPackageDelimiter );
			inOutObjectName.erase( inOutObjectName.begin(), inOutObjectName.begin() + ( iPackageDelimiter + std::strlen( PACKAGE_DELIMITER ) ));
		}
		else
		{
			outPackageName.clear();
		}
	}	
}

namespace fr
{
	const ObjectId ObjectId::NULL_OBJECT( "", "", "null" );

	ObjectId::ObjectId( const std::string& objectIdStringForm )
	{
		const size_t iFirstTick = objectIdStringForm.find( '\'' );
		if( iFirstTick != std::string::npos )
		{
			const size_t iNextTick = objectIdStringForm.find( '\'', iFirstTick + 1 );
			
			if( iNextTick != std::string::npos )
			{
				m_className = objectIdStringForm.substr( 0, iFirstTick );
				m_objectName = objectIdStringForm.substr( iFirstTick + 1, iNextTick - ( iFirstTick + 1 ));

				divideObjectNameToObjectAndPackageName( m_objectName, m_packageName );
				
				m_objectName = parseObjectName( m_objectName );
			}
			else
			{
				FRESH_THROW( FreshException, "ObjectId string \"" << objectIdStringForm << "\" lacks a closing tick '." );
			}
		}
		else
		{
			// Treat the string as a class name only, with blank object name.
			//
			m_className = objectIdStringForm;
		}
	}

	ObjectId::ObjectId( const StringTabulated& className, const std::string& objectName )
	:	m_className( className )
	,	m_objectName( parseObjectName( objectName ))
	{}
	
	ObjectId::ObjectId( const std::string& packageName, const StringTabulated& className, const std::string& objectName )
	:	m_packageName( packageName )
	,	m_className( className )
	,	m_objectName( parseObjectName( objectName ))
	{}

	ObjectId::ObjectId( const Manifest::Object& element )
	{
		m_className = element.className;
		
		// Assign the object name.
		//
		if( !element.name().empty() )
		{
			m_objectName = element.name();
			
			divideObjectNameToObjectAndPackageName( m_objectName, m_packageName );
			m_objectName = parseObjectName( m_objectName );				
		}
		else
		{
			m_objectName = DEFAULT_OBJECT_NAME;
		}
	}

	ObjectId::operator std::string() const
	{
		return std::string( m_className ) + "'" + ( m_packageName.empty() ? "" : ( std::string( m_packageName ) + PACKAGE_DELIMITER ) ) + m_objectName + "'";
	}

	bool ObjectId::isNull() const
	{
		return m_className.empty() && ( m_objectName.empty() || m_objectName == NULL_PTR_STRING );
	}
	
	bool ObjectId::isValid() const
	{
		return isNull() || ( !m_className.empty() && !m_objectName.empty() );
	}

	const std::string& ObjectId::packageName() const
	{
		return m_packageName;
	}
	
	const StringTabulated& ObjectId::className() const
	{
		return m_className;
	}

	const std::string& ObjectId::objectName() const
	{
		return m_objectName;
	}

	bool ObjectId::operator< ( const ObjectId& other ) const
	{
		return m_packageName < other.m_packageName || ( m_packageName == other.m_packageName && ( m_className < other.m_className || ( m_className == other.m_className && m_objectName < other.m_objectName )));
	}
	
	bool ObjectId::operator==( const ObjectId& other ) const
	{
		return ( isNull() && other.isNull() ) || ( m_className == other.m_className && m_objectName == other.m_objectName );
	}

	bool ObjectId::operator!=( const ObjectId& other ) const
	{
		return !operator==( other );
	}

	std::istream& operator>>( std::istream& in, ObjectId& outId )
	{
		// Read the stream until we reach something that is not involved in type or variable names.
		
		try
		{
			outId.m_objectName.clear();
			
			in >> std::ws;
			
			std::string strClassName;
			fr::getline( in, strClassName, " \'],}" );

			trim( strClassName );

			if( strClassName == NULL_PTR_STRING || strClassName.empty() )
			{
				// Found null object id.
				outId.m_objectName = NULL_PTR_STRING;
				strClassName.clear();
			}
			else
			{
				in >> std::ws;
				const char c = in.peek();
				if( c != '\'' )
				{
					dev_warning( "ObjectId had class name " << strClassName << " but next character was '" << c << "' rather than the required '\''." );
				}
				else
				{
					in.get();
				}
				fr::getline( in, outId.m_objectName, '\'' );
				
				divideObjectNameToObjectAndPackageName( outId.m_objectName, outId.m_packageName );
				outId.m_objectName = parseObjectName( outId.m_objectName );
			}
			
			outId.m_className = std::move( strClassName );
		}
		catch( ... )
		{
			outId.m_className.clear();
			outId.m_objectName.clear();
		}
		
		return in;
	}
	
	std::ostream& operator<<( std::ostream& out, const ObjectId& id )
	{
		out << std::string( id );
		return out;
	}

}
