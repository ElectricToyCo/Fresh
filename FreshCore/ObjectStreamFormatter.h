/*
 *  ObjectStreamFormatter.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/29/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_OBJECT_STREAM_FORMATTER_H_INCLUDED
#define FRESH_OBJECT_STREAM_FORMATTER_H_INCLUDED

#include "Object.h"
#include "Archive.h"

#include <iostream>
#include <set>

namespace fr
{
	
	class ObjectStreamFormatter
	{
	public:
		ObjectStreamFormatter( Stringifier& stringifier, unsigned int initialIndentLevel = 0 );
		virtual ~ObjectStreamFormatter() {}
		
		virtual void beginObject( const Object* object );
		virtual void beginProperty( const std::string& propertyName );
		virtual void endProperty( const std::string& propertyName );
		virtual void endObject( const Object* object );
		
		virtual void indent( int indentChange )		{ m_indentLevel += indentChange; }
		void writeIndents();
		
		Stringifier& getStringifier()				{ return m_stringifier; }
		Stringifier& getPropertyStringifier()		{ return m_propertyStringifier; }
		
	protected:
		
		unsigned int getIndentLevel() const { return m_indentLevel; }
		
		virtual bool supportsProperties() const { return false; }
		
	private:
		
		unsigned int m_indentLevel;		
		Stringifier& m_stringifier;
		std::ostringstream m_propertyStream;
		Stringifier m_propertyStringifier;
		
	};
	
	class ObjectStreamFormatterManifest : public ObjectStreamFormatter
	{
	public:
		ObjectStreamFormatterManifest( Stringifier& stringifier, unsigned int initialIndentLevel = 0 );
		
		virtual void beginObject( const Object* object );
		virtual void beginProperty( const std::string& propertyName );
		virtual void endProperty( const std::string& propertyName );
		virtual void endObject( const Object* object );

	protected:
		virtual bool supportsProperties() const { return true; }
		
	};
	
	class ObjectStreamFormatterXml : public ObjectStreamFormatterManifest
	{
	public:
		ObjectStreamFormatterXml( Stringifier& stringifier, unsigned int initialIndentLevel = 0 );
		
		virtual void beginObject( const Object* object );
		virtual void beginProperty( const std::string& propertyName );
		virtual void endProperty( const std::string& propertyName );
		virtual void endObject( const Object* object );
		
	protected:
		virtual bool supportsProperties() const { return true; }
		
	};
	
}

#endif
