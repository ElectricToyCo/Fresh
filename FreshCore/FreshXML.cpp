/*
 *  FreshXML.cpp
 *  TestSerialization
 *
 *  Created by Jeff Wofford on 11/4/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */


#include "FreshXML.h"
#include "FreshDebug.h"
#include "FreshEssentials.h"
#include "FreshFile.h"
#include "Object.h"
#include "ObjectLinker.h"
#include "CommandProcessor.h"

namespace fr
{	
	const XmlElement* loadXmlDocument( const path& fullPath, XmlDocument& doc )
	{
		doc.LoadFile( fullPath.string() );
		
		if( doc.Error() )
		{
			dev_warning( "XML Document '" << fullPath << "' gave parse error " << doc.ErrorId() << " (" << doc.ErrorRow() << ":" << doc.ErrorCol() << "): " << doc.ErrorDesc() );
			return nullptr;
		}
		
		return doc.RootElement();		
	}
	
	const XmlElement* getChildElementWithName( const XmlNode& parent, const std::string& childElementName, const std::string& childNameAttribute )
	{
		return getChildElementWithName( const_cast< XmlNode& >( parent ), childElementName, childNameAttribute );
	}
	
	XmlElement* getChildElementWithName( XmlNode& parent, const std::string& childElementName, const std::string& childNameAttribute )
	{
		REQUIRES( !childElementName.empty() );
		REQUIRES( !childNameAttribute.empty() );
		
		for( XmlElement* element = parent.FirstChildElement( childElementName ); element; element = element->NextSiblingElement( childElementName ))
		{
			const char* szElementName = element->Attribute( "name" );
			
			if( stringCaseInsensitiveCompare( childNameAttribute.c_str(), szElementName))
			{
				return element;
			}
		}
		
		return nullptr;
	}
	
	size_t countChildElements( const ElementIterator& iter, const char* value )
	{
		size_t n = 0;
		
		ElementIterator copy( iter );
		
		while( copy != ElementIterator() )
		{
			++n;
			++copy;
		}
		
		return n;
	}

	
	XmlNode* stringToXml( const std::string& strXML )
	{
		XmlDocument doc;
		doc.Parse( strXML.c_str() );
		
		if( doc.Error() )
		{
			FRESH_THROW( FreshException, "XML string gave parse error " << doc.ErrorId() << " (" << doc.ErrorRow() << ":" << doc.ErrorCol() << "): " << doc.ErrorDesc() );
		}
		else
		{
			ASSERT( doc.RootElement() );
			return doc.RootElement()->Clone();
		}
	}
	
	void xmlToString( std::string& outString, const XmlNode* xmlNode )
	{
		REQUIRES( xmlNode );
		outString << *xmlNode;
	}
	
	std::unique_ptr< XmlElement > stringToXmlElement( const std::string& strXML )
	{
		XmlNode* node = stringToXml( strXML );
		
		if( node )
		{
			XmlElement* element = dynamic_cast< XmlElement* >( node );
			
			return std::unique_ptr< XmlElement >( element );
		}
		else
		{
			return nullptr;
		}
	}

	bool isXmlEquivalent( const std::string& a, const std::string& b )
	{
		std::istringstream ssa( a );
		std::istringstream ssb( b );
		
		while( ssa && !ssa.eof() && ssb && !ssb.eof() )
		{
			char ca, cb;
			ssa >> ca;		// Skipping ws.
			ssb >> cb;
			
			// Replace double quotes with single quotes
			if( ca == '\"' ) ca = '\'';
			if( cb == '\"' ) cb = '\'';
			
			if( ca != cb )
			{
				return false;
			}
		}
		
		// Skip any concluding whitespace.
		ssa >> std::ws;
		ssb >> std::ws;
		
		return ssa.eof() && ssb.eof();
	}
	
	
	////////////////////////////////////////////////////////////////////////////////
	
	ElementIterator::ElementIterator()
	{}
	
	ElementIterator::ElementIterator( const XmlNode& parent, const std::string& childName )
	:	m_childName( childName )
	{
		m_currentChild = parent.FirstChildElement();
		if( m_currentChild )
		{
			proceed( false );
		}
	}
	
	const XmlElement& ElementIterator::operator*() const
	{
		if( m_inclusionStack.empty() )
		{
			ASSERT( m_currentChild );
			return *m_currentChild;
		}
		else
		{
			ASSERT( m_inclusionStack.back().innerChild );
			return *( m_inclusionStack.back().innerChild );
		}
	}
	
	const XmlElement* ElementIterator::operator->() const
	{
		if( m_inclusionStack.empty() )
		{
			ASSERT( m_currentChild );
			return m_currentChild;
		}
		else
		{
			ASSERT( m_inclusionStack.back().innerChild );
			return m_inclusionStack.back().innerChild;
		}
	}
	
	ElementIterator& ElementIterator::operator++()
	{
		proceed( true );
		return *this;
	}
	
	void ElementIterator::proceed( bool goRegardless )
	{
		if( m_inclusionStack.empty() == false )
		{
			bool moreElementsRemain = proceed( m_inclusionStack.back().innerChild, goRegardless );
			if( !moreElementsRemain )
			{
				m_inclusionStack.pop_back();
				proceed( true );
			}
		}
		else
		{
			proceed( m_currentChild, goRegardless );
		}
	}
	
	bool ElementIterator::proceed( const XmlElement*& child, bool goRegardless )
	{
		ASSERT( child );
		
		// Move forward so long as:
		//		1) there are more children to traverse (i.e. we're not at the end)
		//		2) the current child is uninteresting, that is:
		//			a) we have a child name and this child doesn't match it OR
		//			b) this is an include child
		//
		for(;;)
		{
			if( goRegardless )
			{				
				// Move ahead.
				child = child->NextSiblingElement();				
			}
			
			goRegardless = true;

			// Stop if we're at the end.
			if( !child )	
			{
				return false;
			}
			
			// If we've found an <include> element, process it.
			//
			if( m_childName != "include" && child->ValueStr() == "include" )
			{
				processIncludeElement( *child );
				break;
			}
			
			// Stop if we've reached a child with the proper name.
			if( m_childName.empty() || child->ValueStr() == m_childName )
			{
				break;
			}
		}
		
		return true;
	}
	
	void ElementIterator::processIncludeElement( const XmlElement& element )
	{
		// Get the file path to include.
		//
		const char* szInclusionPath = element.Attribute( "url" );
		
		if( !szInclusionPath )
		{
			dev_warning( "ElementIterator::processIncludeElement() found no 'url' attribute. Ignoring this <include> element." );
		}
		else
		{
			// Does this element want to subsume the inner root element?
			//
			const char* szSubsumeRoot = element.Attribute( "subsume-root" );
			bool subsumeRoot = szSubsumeRoot;
			
			if( szSubsumeRoot )
			{
				std::istringstream ss( szSubsumeRoot );
				ss >> std::boolalpha >> subsumeRoot;
			}
			
			// Make the path into a resource path.
			//
			path resourcePath = getResourcePath( szInclusionPath );
			
			m_inclusionStack.push_back( Inclusion() );
			
			Inclusion& inclusion = m_inclusionStack.back();
			
			inclusion.rootElement = loadXmlDocument( resourcePath, inclusion.document );
			if( !inclusion.rootElement )
			{
				dev_warning( "ElementIterator::processIncludeElement() could not load included XML file ''" << resourcePath << "'." );
				m_inclusionStack.pop_back();
			}
			else
			{
				inclusion.innerChild = inclusion.rootElement;
				
				if( subsumeRoot )
				{
					inclusion.innerChild = inclusion.rootElement->FirstChildElement();
				}
				
				proceed( false );
			}
		}
	}
	
	bool ElementIterator::operator==( const ElementIterator& other ) const
	{
		if( m_inclusionStack.size() != other.m_inclusionStack.size() )
		{
			return false;
		}
		else if( m_inclusionStack.empty() == false )
		{
			ASSERT( other.m_inclusionStack.empty() );	// Not bothering to handle both iterators being somewhere in the middle of an inclusion stack. Comparison is expensive.
		}
		
		return m_currentChild == other.m_currentChild &&
			( !m_currentChild || !other.m_currentChild || ( m_childName == other.m_childName ));
	}
	
}


