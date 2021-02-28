//
//  Packages.inl.h
//  Fresh
//
//  Created by Jeff Wofford on 3/3/13.
//
//

#ifndef Fresh_Packages_inl_h
#define Fresh_Packages_inl_h

#include "Objects.h"

namespace fr
{
	template< typename package_t >
	SmartPtr< package_t > getPackage( Package::NameRef name, typename std::enable_if< std::is_convertible< package_t*, Package* >::value, int >::type )
	{
		REQUIRES( name.empty() == false );
		return getRootPackage().find< package_t >( name );
	}

	template< typename package_t >
	SmartPtr< package_t > createPackage( Package::NameRef name, typename std::enable_if< std::is_convertible< package_t*, Package* >::value, int >::type )
	{
		SUPPRESS_FIXUP_CAPTURE()
		return createObject< package_t >( &getRootPackage(), package_t::StaticGetClassInfo(), name );
	}

	template< typename package_t >
	SmartPtr< package_t > loadPackage( const path& pathToPackage, Package::NameRef name, typename std::enable_if< std::is_convertible< package_t*, Package* >::value, int >::type )
	{
		auto package = createPackage< package_t >( name );
		package->retainMembers();
		package->loadFile( pathToPackage );
		return package;
	}
}

#endif
