/*
 *  ObjectStreamFormatter.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/29/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#include "ObjectStreamFormatter.h"
#include "FreshDebug.h"
using namespace fr;

ObjectStreamFormatter::ObjectStreamFormatter( Stringifier& stringifier, unsigned int initialIndentLevel /* = 0 */ )
:	m_indentLevel( initialIndentLevel )
,	m_stringifier( stringifier )
,	m_propertyStringifier( m_propertyStream, stringifier.getObjectObserver() )
{}

void ObjectStreamFormatter::beginObject( const Object* object )
{
	writeIndents();
	getStringifier().rawStream() << object->toString();
}

void ObjectStreamFormatter::beginProperty( const std::string& propertyName )
{
	// Do nothing.
}

void ObjectStreamFormatter::endProperty( const std::string& propertyName )
{
	if( supportsProperties() )
	{
		getStringifier().rawStream() << m_propertyStream.str();	// Copy properties back into "real" stringifier.
		m_propertyStream.str( "" );
	}
}

void ObjectStreamFormatter::endObject( const Object* object )
{
	writeIndents();
	getStringifier().rawStream() << '\n';
}

void ObjectStreamFormatter::writeIndents()
{
	unsigned int nIndents = m_indentLevel;
	while( nIndents-- > 0 )
	{
		getStringifier().rawStream() << '\t';
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

ObjectStreamFormatterManifest::ObjectStreamFormatterManifest( Stringifier& stringifier, unsigned int initialIndentLevel /* = 0 */ )
:	ObjectStreamFormatter( stringifier, initialIndentLevel )
{}

void ObjectStreamFormatterManifest::beginObject( const Object* object )
{
	writeIndents();
	getStringifier().rawStream() << "object " << object->className() << " \"" << object->name() << "\" {\n";
	indent( 1 );
}

void ObjectStreamFormatterManifest::beginProperty( const std::string& propertyName )
{
	writeIndents();
	getStringifier().rawStream() << propertyName << " ";
}

void ObjectStreamFormatterManifest::endProperty( const std::string& propertyName )
{
	ObjectStreamFormatter::endProperty( propertyName );
	getStringifier().rawStream() << '\n';
}

void ObjectStreamFormatterManifest::endObject( const Object* object )
{
	indent( -1 );
	writeIndents();
	getStringifier().rawStream() << "}\n";
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

ObjectStreamFormatterXml::ObjectStreamFormatterXml( Stringifier& stringifier, unsigned int initialIndentLevel /* = 0 */ )
:	ObjectStreamFormatterManifest( stringifier, initialIndentLevel )
{}

void ObjectStreamFormatterXml::beginObject( const Object* object )
{
	writeIndents();
	getStringifier().rawStream() << "<object class='" << object->className() << "' name='" << object->name() << "'>\n";
	indent( 1 );
}

void ObjectStreamFormatterXml::beginProperty( const std::string& propertyName )
{
	writeIndents();
	getStringifier().rawStream() << "<" << propertyName << ">";
}

void ObjectStreamFormatterXml::endProperty( const std::string& propertyName )
{
	ObjectStreamFormatter::endProperty( propertyName );
	getStringifier().rawStream() << "</" << propertyName << ">\n";
}

void ObjectStreamFormatterXml::endObject( const Object* object )
{
	indent( -1 );
	writeIndents();
	getStringifier().rawStream() << "</object>\n";
}


