//
//  main.cpp
//  test_FreshCore
//
//  Created by Jeff Wofford on 3/1/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "Objects.h"
#include "Property.h"
#include "FreshFile.h"

#include <iostream>
#include <sstream>
using namespace Fresh;

class AbstractNode : public Object
{
	FRESH_DECLARE_CLASS_ABSTRACT( AbstractNode, Object )
public:
	
	virtual void add( AbstractNode::ptr other ) = 0;
	
};
FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( AbstractNode )

class Node : public AbstractNode
{
	FRESH_DECLARE_CLASS( Node, AbstractNode )
	
public:
		
	virtual void add( AbstractNode::ptr otherAbstract ) override
	{
		Node::ptr other = dynamic_freshptr_cast< Node::ptr >( otherAbstract );
		REQUIRES( other );
		if( other->hasStrongPathTo( this ))
		{
			if( std::find( m_weakLinks.begin(), m_weakLinks.end(), other ) == m_weakLinks.end() )
			{
				m_weakLinks.push_back( other );
			}
		}
		else
		{
			if( std::find( m_strongLinks.begin(), m_strongLinks.end(), other ) == m_strongLinks.end() )
			{
				m_strongLinks.push_back( other );
			}
		}
	}
	
	bool hasStrongPathTo( Node::ptr to ) const
	{
		REQUIRES( to );
		return hasStrongPathTo( to, ++s_idTraversal );
	}

protected:
	
	bool hasStrongPathTo( Node::ptr to, size_t idTraversal ) const
	{
		if( m_idLastVisited == idTraversal )
		{
			return false;
		}
		else
		{
			m_idLastVisited = idTraversal;
			
			if( this == to )
			{
				return true;
			}
			else
			{
				for( auto strongNeighbor : m_strongLinks )
				{
					if( strongNeighbor->hasStrongPathTo( to, idTraversal ))
					{
						return true;
					}
				}
				return false;
			}
		}
	}

private:
	
	VAR( Node, std::list< Node::ptr >, m_strongLinks );
	VAR( Node, std::list< Node::wptr >, m_weakLinks );
	
	mutable size_t m_idLastVisited = 0;
	
	static size_t s_nNodes;
	static size_t s_idTraversal;
	
};

size_t Node::s_nNodes = 0;
size_t Node::s_idTraversal = 0;

FRESH_DEFINE_CLASS( Node )
DEFINE_VAR( Node, std::list< Node::ptr >, m_strongLinks );
DEFINE_VAR( Node, std::list< Node::wptr >, m_weakLinks );
FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( Node )

Node::Node( const ClassInfo& assignedClassInfo, NameRef name )
:	Super( assignedClassInfo, name.empty() ? (( std::ostringstream( name ) << s_nNodes++ ).str() ) : name )
{}

template< typename iter_t >
void stringifyObjects( std::ostream& out, iter_t begin, iter_t end )
{
	// Serialize the results.
	//
	Stringifier stringifier( out );
	ObjectStreamFormatterXml formatter( stringifier, 1 );
	std::for_each( begin, end, [&formatter] ( Object::ptr p )
				  {
					  p->serialize( formatter );
				  } );	
}

void reportFailure( const char* expression )
{
	std::cerr << "TEST FAILED: '" << expression << "'.\n";
	throw 1;
}
#define TEST_VERIFY( expr ) if( !(expr) ) reportFailure( #expr );

/////////////////////

int main( int argc, const char * argv[] )
{
	try
	{
		path tempPath;
		std::ostringstream nodeManifest;
		{
			srand( (unsigned int) time(0));
			
			const int NUM_NODES = 100;
			const int MAX_LINKS_PER_NODE = 100;

			// Create a set of nodes.
			//
			std::vector< AbstractNode::ptr > nodes( NUM_NODES );
			std::generate( nodes.begin(), nodes.end(), [] () { return createObject< AbstractNode >( Node::StaticGetClassInfo() ); } );
			
			Package::ptr package = createPackage( "nodes" );
			package->add( nodes.begin(), nodes.end() );
			
			// Connect them randomly.
			//
			std::for_each( nodes.begin(), nodes.end(), [&] ( AbstractNode::ptr p )
						  {
							  const int nOutboundLinks = rand() % MAX_LINKS_PER_NODE;
							  for( int i = 0; i < nOutboundLinks; ++i )
							  {
								  const size_t which = rand() % nodes.size();
								  p->add( nodes.at( which ));
							  }
						  } );
			
			// Serialize the package contents.
			//
			stringifyObjects( nodeManifest, nodes.begin(), nodes.end() );
			
			// Save them to a file.
			//
			std::ofstream temp;
			tempPath = createTempFile( temp );
			ASSERT( tempPath.empty() == false );
			
			package->save( temp );
			
			temp.close();

	//		std::cout << "NODES (1) -----------------------------------\n";
	//		std::cout << nodeManifest.str();
		}
		
		// Load the results
		//
		{
			ASSERT( tempPath.empty() == false );
			
			new ObjectLinker();
			
			// Load the previously saved file.
			//
			Package::ptr package = createPackage( "nodes2" );
			std::vector< Object::ptr > packageContents = package->load( tempPath );
			
			// Serialize the package contents as before.
			//
			std::ostringstream nodeManifest2;
			stringifyObjects( nodeManifest2, packageContents.begin(), packageContents.end() );

	//		std::cout << "NODES (2) -----------------------------------\n";
	//		std::cout << nodeManifest2.str();

			// Confirm that the serialization strings are identical.
			//
			TEST_VERIFY( nodeManifest.str() == nodeManifest2.str() );
		}
		
		ASSERT( tempPath.empty() == false );
		std::remove( tempPath.c_str() );
		
		std::cout << "ALL PASSED.\n";
	}
	catch( int i )
	{
		return i;
	}
	
    return 0;
}

