/*
 *  FreshXML.h
 *  TestSerialization
 *
 *  Created by Jeff Wofford on 11/4/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_FRESH_XML_H_INCLUDED
#define FRESH_FRESH_XML_H_INCLUDED

#include "tinyxml.h"
#include "StringTabulated.h"
#include "FreshPath.h"
#include <list>
#include <memory>

namespace fr
{
	
	// Define Xml types in the Fresh namespace for some modicum of future
	// independence from TinyXML.
	
	typedef TiXmlDocument XmlDocument;
	typedef TiXmlNode XmlNode;
	typedef TiXmlElement XmlElement;
	typedef TiXmlAttribute XmlAttribute;
	typedef TiXmlText XmlText;

	const XmlElement* loadXmlDocument( const path& fullPath, XmlDocument& doc );
	
	const XmlElement* getChildElementWithName( const XmlNode& parent, const std::string& childElementName, const std::string& childNameAttribute );
		// REQUIRES( parent );
		// REQUIRES( !childElementName.empty() );
		// REQUIRES( !childNameAttribute.empty() );
		// PROMISES NOTHING
	
	XmlElement* getChildElementWithName( XmlNode& parent, const std::string& childElementName, const std::string& childNameAttribute );
		// REQUIRES( parent );
		// REQUIRES( !childElementName.empty() );
		// REQUIRES( !childNameAttribute.empty() );
		// PROMISES NOTHING

	XmlNode* stringToXml( const std::string& strXML );
		// Returns nullptr if the string cannot be converted to valid XML.
		// Else returns a pointer to an XMLNode of whatever type.
		// THE CALLER IS RESPONSIBLE FOR DELETING THE RETURNED POINTER
		// WHEN DONE USING IT.
		// PROMISES NOTHING
	
	void xmlToString( std::string& outString, const XmlNode* xmlNode );
		// REQUIRES( xmlNode );
	
	std::unique_ptr< XmlElement > stringToXmlElement( const std::string& strXML );
		// Returns nullptr if the string cannot be converted to a valid XmlElement.

	bool isXmlEquivalent( const std::string& a, const std::string& b );
	
	//////////////////////////////////////////////////////////////////////////////////////////////////
	
	class ElementIterator
	{
	public:
		
		ElementIterator();		// Produces an "end" iterator.
		ElementIterator( const ElementIterator& other ) : m_childName( other.m_childName ), m_currentChild( other.m_currentChild ) 
		{
			assert( other.m_inclusionStack.empty() );	// Doesn't preserve inclusions.
		}
		
		ElementIterator& operator=( const ElementIterator& other )
		{
			if( this != &other )
			{
				assert( other.m_inclusionStack.empty() );	// Doesn't preserve inclusions.
				m_childName = other.m_childName;
				m_currentChild = other.m_currentChild;
			}
			return *this;
		}
		explicit ElementIterator( const XmlNode& parent, const std::string& childName  = "" );
		
		const XmlElement& operator*() const;
		const XmlElement* operator->() const;
		
		ElementIterator& operator++();
		
		bool operator==( const ElementIterator& other ) const;
		bool operator!=( const ElementIterator& other ) const
		{
			return !operator==( other );
		}
		
	protected:
		
		void proceed( bool goRegardless );
		bool proceed( const XmlElement*& child, bool goRegardless );
		void processIncludeElement( const XmlElement& element );
		
	private:
		
		struct Inclusion
		{
			XmlDocument document;
			const XmlElement* rootElement;
			const XmlElement* innerChild;
		};
		
		std::string m_childName;
		const XmlElement* m_currentChild = nullptr;
		
		std::list< Inclusion > m_inclusionStack;
		
	};

	size_t countChildElements( const ElementIterator& iter, const char* value = "" );
	
}

#endif
