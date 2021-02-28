//
//  Objects.h
//  Fresh
//
//  Created by Jeff Wofford on 2/27/13.
//
//

#ifndef Fresh_Objects_h
#define Fresh_Objects_h

#include "Object.h"
#include <vector>

namespace fr
{
	
	class Package;
	
	//////////////////////////////////////////////////////////////
	// createObject<> with variants.
	
	// Creates an object in the given package (optional) with the given base class, object name, and initializing element.
	template< typename return_t >
	SmartPtr< return_t > createObject( SmartPtr< Package > package, const ClassInfo& desiredBase, ObjectNameRef objectName = DEFAULT_OBJECT_NAME, const Manifest::Map* properties = nullptr );
	// REQUIRES( classInfo.isKindOf( return_t::StaticGetClassInfo() ));

	// Creates an object of the class return_t in the given package with the given (optional) object name and initializing element.
	template< typename return_t >
	SmartPtr< return_t > createObject( Package& package, ObjectNameRef objectName = DEFAULT_OBJECT_NAME, const Manifest::Map* properties = nullptr );

	// Creates an object of the given class and name, with a further specialization of the class.
	template< typename return_t = Object >
	SmartPtr< return_t > createObject( const ClassInfo& classInfo, Object::NameRef name = DEFAULT_OBJECT_NAME, const Manifest::Map* properties = nullptr );
	// REQUIRES( classInfo.isKindOf( return_t::StaticGetClassInfo() ));
	
	// Creates an object of the given class and name.
	template< typename return_t = Object >
	SmartPtr< return_t > createObject( Object::NameRef name = DEFAULT_OBJECT_NAME, const Manifest::Map* properties = nullptr );

	// Creates the object implied by the initialization object.
	template< typename return_t = Object >
	SmartPtr< return_t > createObject( const Manifest::Object& initialization );
	
	// Creates an object of the given id..
	template< typename return_t = Object >
	SmartPtr< return_t > createObject( const ObjectId& objectId, const Manifest::Map* properties = nullptr );
	
	//////////////////////////////////////////////////////////////
	// Accessing objects
	
	// ObjectId specified, possibly including the package.
	template< typename return_t = Object >
	SmartPtr< return_t > getObject( const ObjectId& objectId );

	template< typename return_t = Object >
	SmartPtr< return_t > getObject( Object::NameRef name );

	// Returns all registered objects that match the given objectFilter.
	// The package name must match by regex.
	// The class name must match precisely the name of a base class of each object returned.
	// The object name must match as a regex.
	// Returned objects are all of class return_t or more derived.
	// If searchAllRootedPackages and the objectFilter.packageName() is empty, then
	// all packages owned by the root package will be searched.
	// Else only the search packages will be searched.
	template< typename return_t = Object >
	std::vector< SmartPtr< return_t > > getFilteredObjects( const ObjectId& objectFilter, bool searchAllRootedPackages = false );
	
	//////////////////////////////////////////////////////////////
	// createOrGetObject<>
	// Gets the object iff it exists; else creates it.

	// If the object exists and applyElementIfGet, the object is re-loaded with the supplied initialization (ifAny).
	// ObjectId specified, possibly including package.
	template< typename return_t = Object >
	SmartPtr< return_t > createOrGetObject( const ObjectId& objectId, const Manifest::Map* properties = nullptr, bool applyElementIfGet = true );

	template< typename return_t = Object >
	SmartPtr< return_t > createOrGetObject( Object::NameRef objectName, const Manifest::Map* properties = nullptr, bool applyElementIfGet = true );

	// If the object exists and applyElementIfGet, the object is re-loaded with the supplied initialization.
	// ObjectId specified via Manifest::Object, possibly including package.
	template< typename return_t = Object >
	SmartPtr< return_t > createOrGetObject( const Manifest::Object& initialization, bool applyElementIfGet = true );
	
	//////////////////////////////////////////////////////////////
	// isObject()
	// Returns true iff the given pointer points to a real object
	//
	bool isObject( const Object* object );
}

#include "Objects.inl.h"

#endif
