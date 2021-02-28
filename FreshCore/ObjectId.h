/*
 *  ObjectId.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 10/30/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_OBJECT_ID_H_INCLUDED
#define FRESH_OBJECT_ID_H_INCLUDED

#include "StringTabulated.h"
#include "FreshManifest.h"
#include <string>
#include <iostream>

namespace fr
{

	const char NULL_PTR_STRING[] = "null";
	
	class ObjectId
	{
	public:
		
		static const ObjectId NULL_OBJECT;

		// When converted to and from strings, object ids are of the form:
		// className'packageName::objectName'

		ObjectId() {}
		explicit ObjectId( const std::string& objectIdStringForm );
		ObjectId( const StringTabulated& className, const std::string& objectName );	// Blank package name.
		ObjectId( const std::string& packageName, const StringTabulated& className, const std::string& objectName );
		explicit ObjectId( const Manifest::Object& element );

		explicit operator std::string() const;
		
		std::string toString() const { return operator std::string(); }
		
		void reset()									{ *this = NULL_OBJECT; }

		bool isNull() const;
		bool isValid() const;
		
		const std::string& packageName() const;
		const StringTabulated& className() const;
		const std::string& objectName() const;
		
		bool operator==( const ObjectId& other ) const;
		bool operator!=( const ObjectId& other ) const;
		bool operator< ( const ObjectId& other ) const;
		
		explicit operator bool() const			{ return !isNull(); }
			
	private:
		
		std::string m_packageName;
		StringTabulated m_className;
		std::string m_objectName;
			
		friend std::istream& operator>>( std::istream& in, ObjectId& outId );
		friend std::ostream& operator<<( std::ostream& out, const ObjectId& id );
	};
}

#endif
