//
//  Classes.h
//  Fresh
//
//  Created by Jeff Wofford on 2/27/13.
//
//

#ifndef __Fresh__Classes__
#define __Fresh__Classes__

#include "ClassInfo.h"
#include "ObjectFactory.h"

namespace fr
{
	
	// Call this at the beginning of main().
	// It initializes (actually concludes initialization of)
	// the Fresh reflection system.
	//
	void initReflection();
	
	bool isClass( ClassInfo::NameRef className );
	
	ClassInfo::ptr getClass( ClassInfo::NameRef className );
	// Returns null if no such class.
	
	void forEachClass( std::function< void( ClassInfo& ) >&& fnPerClass, ClassInfo::cptr baseClass = nullptr );
	
	ClassInfo::ptr createClass( ClassInfo::NameRef pseudoclassName, ClassInfo::NameRef baseClassName, const Manifest::Class& configuration, ClassInfo::Placeable isPlaceable = ClassInfo::Placeable::Inherit );

	bool isClassKindOf( const ClassInfo& maybeDerived, const ClassInfo& maybeBase );
	



	////////////////////////////////////////////////////////////////////////////////////
	// Implementation

	ClassInfo::ptr registerClass_implementation( ClassInfo::ptr classInfo );
	
	template< typename class_t >
	ClassInfo::ptr createNativeClass( ClassInfo::ptr base, ClassInfo::NameRef className, ClassInfo::Placeable isPlaceable )
	{
		REQUIRES( className.empty() == false );
		
		typedef ObjectFactory< class_t > factory_t;
		std::unique_ptr< factory_t > factory( new factory_t() );
		
		return registerClass_implementation( new ClassInfo( base, className, std::move( factory ), static_cast< Manifest::Map* >( nullptr ) /* configuration element */, true /* isNative */, isPlaceable ));
	}

	template< typename class_t >
	ClassInfo::ptr createNativeClassAbstract( ClassInfo::ptr base, ClassInfo::NameRef className )
	{
		REQUIRES( className.empty() == false );
		return registerClass_implementation( new ClassInfo( base, className, nullptr /* factory */, static_cast< Manifest::Map* >( nullptr ) /* configuration element */, true /* isNative */ ));
	}
}

#endif
