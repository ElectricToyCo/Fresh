//
//  Packages.h
//  Fresh
//
//  Created by Jeff Wofford on 2/28/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_Packages_h
#define Fresh_Packages_h

#include "Package.h"
#include <type_traits>

namespace fr
{

	template< typename package_t = Package >
	SmartPtr< package_t > getPackage( Package::NameRef name, typename std::enable_if< std::is_convertible< package_t*, Package* >::value, int >::type = int() );
		// REQUIRES( name.empty() == false );

	template< typename package_t = Package >
	SmartPtr< package_t > createPackage( Package::NameRef name = DEFAULT_OBJECT_NAME, typename std::enable_if< std::is_convertible< package_t*, Package* >::value, int >::type = int() );
	
	template< typename package_t = Package >
	SmartPtr< package_t > loadPackage( const path& pathToPackage, Package::NameRef name = DEFAULT_OBJECT_NAME, typename std::enable_if< std::is_convertible< package_t*, Package* >::value, int >::type = int() );
	// Package will be retained.
	
	Package& getRootPackage();
	// The root package contains all other packages. You would not normally need to deal directly with it.
	
	Package& getTransientPackage();
	// A convenience package for objects that want to avoid membership in any searchable package.

	///////////////////////////////////////////////////////////////
	// SEARCH PACKAGES
	// The set of search packages defines which objects are automatically
	// findable using functions like getObject() and getFilteredObjects().
	//
	void addSearchPackage( Package::wptr package );
	void removeSearchPackage( Package::wptr package );
	
	// If fnPerPackage returns true, stops the traversal.
	void forEachSearchPackage( std::function< bool ( Package& ) >&& fnPerPackage );
}

#include "Packages.inl.h"

#endif
