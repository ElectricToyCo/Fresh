#include "ObjectManager.h"
#include "ObjectStreamFormatter.h"
#include "ObjectStore.h"
#include "FreshFile.h"
#include "FreshTest.h"
using namespace Fresh;
using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Test functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool createSimpleObjects()
{
	ObjectManager& objectManager = ObjectManager::instance();
	
	// Constructor with default arguments.
	{
		Object::ptr object = new Object();
		
		// Verify object is managed.
		VERIFY_BOOL( objectManager.hasObject( object ));
		
		// Verify object can be found by its own name.
		VERIFY_BOOL( object->getUnqualifiedName() == object->getName() );
		VERIFY_BOOL( objectManager.getObject( object->getClassName(), object->getUnqualifiedName() ) == object );
		VERIFY_BOOL( objectManager.getObject( object->getClassName(), object->getQualifiedName() ) == object );
	}
	
	// Unmanaged object.
	{
		Object::ptr object = new Object( Object::DEFAULT_OBJECT_NAME, false /* unmanaged */ );
		
		// Verify object is unmanaged.
		VERIFY_BOOL( !objectManager.hasObject( object ));
	}
	
	// Named object with default namespace.
	{
		const char* const NAME = "TestObject1";
		Object::ptr object = new Object( NAME );

		// Verify object name is as requested.
		VERIFY_BOOL( object->hasName( NAME ));
		VERIFY_BOOL( object->getName() == NAME );
		VERIFY_BOOL( object->getUnqualifiedName() == object->getName() );

		// Verify object can be found as in the current namespace.
		VERIFY_BOOL( objectManager.hasObject( object ));
		VERIFY_BOOL( objectManager.getObject( object->getClassName(), object->getUnqualifiedName() ) == object );
		VERIFY_BOOL( objectManager.getObject( object->getClassName(), object->getQualifiedName() ) == object );
		
		// Verify object can be found from interior namespace.
		const char* const NAMESPACE_NAME = "namespace1";
		objectManager.beginNamespace( NAMESPACE_NAME );
		const Namespace& interiorNamespace = objectManager.currentNamespace();
		VERIFY_BOOL( interiorNamespace.name() == NAMESPACE_NAME );

		VERIFY_BOOL( interiorNamespace.getObject( object->getClassName(), object->getUnqualifiedName() ));	// Unqualified name in "ancestor" namespace can be found.
		VERIFY_BOOL( interiorNamespace.getObject( object->getClassName(), object->getQualifiedName() ));		// Qualified name can.		
		
		objectManager.endNamespace();
		VERIFY_BOOL( objectManager.currentNamespace().name().empty() );
	}
	
	// Named object in interior namespace indicated in given name.
	{
		// Create the target namespace.
		const char* const NAMESPACE_NAME = "interior1";
		objectManager.beginNamespace( NAMESPACE_NAME );

		const char* const UNQUALIFIED_NAME = "TestObject1";
		Object::ptr object = new Object( UNQUALIFIED_NAME );

		objectManager.endNamespace();

		// Verify object name is as requested.
		std::string PARTIALLY_QUALIFIED_NAME = std::string( NAMESPACE_NAME ) + "::" + UNQUALIFIED_NAME;
		std::string FULLY_QUALIFIED_NAME = std::string( "::" ) + PARTIALLY_QUALIFIED_NAME;
		VERIFY_BOOL( object->hasName( UNQUALIFIED_NAME ));
		VERIFY_BOOL( object->getName() == UNQUALIFIED_NAME );
		VERIFY_BOOL( object->getQualifiedName() == FULLY_QUALIFIED_NAME );
		
		// Test namechain generation.
		//
		Namechain namechain( object->getQualifiedName() );
		VERIFY_BOOL( namechain.isValid() );
		VERIFY_BOOL( static_cast< std::string >( namechain ) == FULLY_QUALIFIED_NAME );

		// Verify object can be found in the root namespace.
		VERIFY_BOOL( objectManager.currentNamespace().name().empty() );
		VERIFY_BOOL( objectManager.hasObject( object ));
		VERIFY_BOOL( objectManager.getObject( object->getClassName(), object->getUnqualifiedName() ) == nullptr );	// Can't find because the object is in a child namespace.
		VERIFY_BOOL( objectManager.getObject( object->getClassName(), object->getQualifiedName() ) == object );		// Can find with qualification.
		VERIFY_BOOL( objectManager.getObject( object->getClassName(), PARTIALLY_QUALIFIED_NAME ) == object );		// Can find with or partial qualification.
		
		// Verify object can be found from the same interior namespace.
		{
			objectManager.beginNamespace( NAMESPACE_NAME );
			
			const Namespace& interiorNamespace = objectManager.currentNamespace();
			VERIFY_BOOL( interiorNamespace.name() == NAMESPACE_NAME );
			
			VERIFY_BOOL( interiorNamespace.getObject( object->getClassName(), object->getUnqualifiedName() ) == object );	// Unqualified name for object in this namespace can be found.
			VERIFY_BOOL( interiorNamespace.getObject( object->getClassName(), object->getQualifiedName() ) == object );		// Fully qualified name can too.
			VERIFY_BOOL( interiorNamespace.getObject( object->getClassName(), PARTIALLY_QUALIFIED_NAME ) == object );			// And partially qualified name as well.
			
			objectManager.endNamespace();
		}
		
		// Verify object finding from a peer namespace.
		{
			objectManager.beginNamespace( "interior2" );
			
			const Namespace& interiorNamespace = objectManager.currentNamespace();
			VERIFY_BOOL( interiorNamespace.name() == "interior2" );
			
			VERIFY_BOOL( interiorNamespace.getObject( object->getClassName(), object->getUnqualifiedName() ) == nullptr );	// Can't find "TestObject1" from namespace "interior2".
			VERIFY_BOOL( interiorNamespace.getObject( object->getClassName(), object->getQualifiedName() ) == object );		// Fully qualified name works, though.
			VERIFY_BOOL( interiorNamespace.getObject( object->getClassName(), PARTIALLY_QUALIFIED_NAME ) == object );			// And partially qualified name as well.
			
			objectManager.endNamespace();
		}
	}

	// Named object created while interior namespace is current, but forced into other namespace.
	{
		// Create a namespace.
		objectManager.beginNamespace( "interior1" );
		objectManager.endNamespace();

		// Create another namespace.
		objectManager.beginNamespace( "interior2" );

		// Create an object in the interior1 (not the current) namespace, forcing it by name.
		//
		Object::ptr object = new Object( "::interior1::innerObject" );

		// Verify object name is as requested.
		VERIFY_BOOL( object->hasName( "innerObject" ));
		VERIFY_BOOL( object->getName() == "innerObject" );
		VERIFY_BOOL( object->getQualifiedName() == "::interior1::innerObject" );

		// Find object from "interior2" namespace?
		VERIFY_BOOL( objectManager.hasObject( object ));
		VERIFY_BOOL( objectManager.getObject( object->getClassName(), object->getUnqualifiedName() ) == nullptr );	// Can't find because the object is in a different child namespace.
		VERIFY_BOOL( objectManager.getObject( object->getClassName(), object->getQualifiedName() ) == object );		// Can find with qualification.
		VERIFY_BOOL( objectManager.getObject( object->getClassName(), "interior1::innerObject" ) == object );			// Can find with partial qualification.

		objectManager.endNamespace();

		// Find object from root namespace?
		VERIFY_BOOL( objectManager.getObject( object->getClassName(), object->getUnqualifiedName() ) == nullptr );	// Can't find because the object is in a child namespace.
		VERIFY_BOOL( objectManager.getObject( object->getClassName(), object->getQualifiedName() ) == object );		// Can find with qualification.
		VERIFY_BOOL( objectManager.getObject( object->getClassName(), "interior1::innerObject" ) == object );			// Can find with partial qualification.
	}
	
	// Expected failure for attempt to create an object in an unrecognized namespace.
	{
		try
		{
			Object::ptr object = new Object( "nowhere::anObject" );
			VERIFY_BOOL_MSG( false, "Object creation in unrecognized namespace should not have succeeded." );
		}
		catch( const FreshException& e )
		{
			// Exception caught as expected. No problem.
		}
		catch( ... )
		{
			VERIFY_BOOL_MSG( false, "Object creation in unrecognized namespace threw unexpected exception." );
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main( int argc, char * const argv[] )
{
	{
		try
		{
			// Initial setup.
			//
			Object::useTimeCodedDefaultNames( false );		// Use predictable object names so that test results are
															// consistent from run to run.
			ObjectManager objectManager;
			
			// Run tests.
			//
			VERIFY_TEST( createSimpleObjects() );
		}
		catch( ... )
		{
			cerr << "Caught unknown exception.\n";
			return 1;
		}
	}

	cout << "All tests passed.\n";

	return 0;
}
