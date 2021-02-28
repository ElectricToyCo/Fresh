//
//  Objects.inl.h
//  Fresh
//
//  Created by Jeff Wofford on 2/27/13.
//
//

#ifndef Fresh_Objects_inl_h
#define Fresh_Objects_inl_h

#include "Packages.h"
#include "Classes.h"
#include "FreshException.h"
#include "Profiler.h"

namespace fr
{	
	template< typename return_t >
	SmartPtr< return_t > createObject( Package::ptr package, const ClassInfo& desiredBase, ObjectNameRef objectName, const Manifest::Map* properties )
	{
		TIMER_AUTO( createObject( Package::ptr, const ClassInfo&, ObjectNameRef, const Manifest::Object* ))

		ASSERT( &desiredBase );
		
		typedef SmartPtr< return_t > ptr_t;

		ptr_t result;
		
		ObjectId id( desiredBase.className(), objectName );
		
		if( !package )
		{
			// Use the capturing package, if any.
			//
			package = activePackage();	// Might still be null. That's okay.
		}
		
		if( package )
		{
			pushActivePackage( package );	// Push the package. Might be a redundant push of already-active package, but that's okay.
		}
			
		// Load the object in a "fixup capture" so that pointer fixup will be handled afterward, if needed.
		{
			CAPTURE_FIXUP_FOR_OBJECT( "While creating object " << id << ": " );	// This will also establish a temporary package if there is no active package.
			
			// Verify that the classInfo is legal.
			//
			const ClassInfo& requiredBase = return_t::StaticGetClassInfo();
			
			if( !desiredBase.isKindOf( requiredBase ))
			{
				FRESH_THROW( FreshException, "Error creating object " << id << ": The indicated class " << desiredBase.className() << " is not a kind of " << requiredBase.className() << "." );
			}
			
			if( desiredBase.isAbstract() )
			{
				FRESH_THROW( FreshException, "Error creating object " << id << ": The indicated class " << desiredBase.className() << " is abstract." );
			}
		
			//
			// Create the object.
			//			
			auto& factory = desiredBase.factory();
			result = dynamic_freshptr_cast< ptr_t >( factory.createObject( desiredBase, id.objectName() ));
			ASSERT( result );
			
			// Add the object to its package.
			//
			if( package )
			{
				package->add( result );
			}

			// Allow classes with "configuring elements" to apply them to the object.
			//
			desiredBase.applyConfiguration( *result, properties );
			
			// If we have an element, apply it.
			//
			if( properties )
			{
				result->load( *properties );
			}
		}				// APPLY OBJECT FIXUP (on death of capturer)

		// AFTER FIXUP
		//
		// If the package isn't loading as a whole, immediately let the object know
		// that it's done loading.
		//
		if( !package || !package->isLoading() )
		{
			result->postLoad();
			result->onAllLoaded();
		}

		if( package )
		{
			popActivePackage();
		}
		
		return result;
	}

	template< typename return_t >
	SmartPtr< return_t > createObject( Package& package, ObjectNameRef objectName, const Manifest::Map* properties )
	{
		return createObject< return_t >( &package, return_t::StaticGetClassInfo(), objectName, properties );
	}
	
	template< typename return_t >
	SmartPtr< return_t > createObject( const ObjectId& id, const Manifest::Map* properties )
	{
		TIMER_AUTO( createObject( const ObjectId&, const Manifest::Object* ))
		
		Package::ptr package;
		if( id.packageName().empty() == false )
		{
			package = getPackage( id.packageName() );
			if( !package )
			{
				dev_warning( "When creating object " << id << ": the indicated package " << id.packageName() << " could not be found." );
			}
		}
		
		if( id.className().empty() )
		{
			FRESH_THROW( FreshException, "Error creating object " << id << ": No class name indicated." );
		}
		
		ClassInfo::ptr desiredBase = getClass( id.className() );
		
		if( !desiredBase )
		{
			FRESH_THROW( FreshException, "Error creating object " << id << ": Unrecognized class " << id.className() << "." );
		}
		
		return createObject< return_t >( package, *desiredBase, id.objectName(), properties );
	}
	
	template< typename return_t >
	SmartPtr< return_t > createObject( ObjectNameRef name, const Manifest::Map* properties )
	{
		TIMER_AUTO( createObject( ObjectNameRef, const Manifest::Object* ))
		return createObject< return_t >( nullptr /* unknown or no package */, return_t::StaticGetClassInfo(), name, properties );
	}
	
	template< typename return_t >
	SmartPtr< return_t > createObject( const ClassInfo& classInfo, ObjectNameRef objectName, const Manifest::Map* properties )
	{
		ASSERT( &classInfo );
		TIMER_AUTO( createObject( const ClassInfo&, ObjectNameRef ))
		return createObject< return_t >( nullptr /* unknown or no package */, classInfo, objectName, properties );
	}
		
	template< typename return_t >
	SmartPtr< return_t > createObject( const Manifest::Object& initialization )
	{
		ASSERT( &initialization );
		TIMER_AUTO( createObject( const Manifest::Object& ))
		return createObject< return_t >( ObjectId( initialization ), &initialization );
	}

	///////////////////////////////////////////////////////////////////////////
	
	template< typename return_t >
	SmartPtr< return_t > getObject( const ObjectId& id )
	{
		// Identify and find the package.

		// Prefer the package actually requested by the object.
		//
		if( !id.packageName().empty() )
		{ 
			Package::ptr package = getPackage( id.packageName() );
			
			if( !package )
			{
				dev_warning( "When seeking object " << id << ": the indicated package " << id.packageName() << " could not be found." );
				return nullptr;
			}
			else
			{
				// request(), unlike find(), might actually create the object.
				return package->request< return_t >( id );
			}
		}
		else
		{
			// No package indicated by the objectId.
			// Search for the correct package, starting with the active package.
			//
			SmartPtr< return_t > result;
			
			// Try the active package, if any.
			//
			if( activePackage() )
			{
				result = activePackage()->request< return_t >( id );
			}

			// If no success so far, try the search packages. This can be expensive: O(n^2).
			//
			if( !result )
			{
				forEachSearchPackage( [&id, &result] ( Package& package ) -> bool
									 {
										 result = package.request< return_t >( id );
										 return !result.isNull();
									 } );
			}
			
			return result;			
		}		
	}
	
	template< typename return_t >
	SmartPtr< return_t > getObject( Object::NameRef name )
	{
		return getObject< return_t >( ObjectId( return_t::StaticGetClassInfo().className(), name ));
	}
	
	template< typename return_t >
	std::vector< SmartPtr< return_t >> getFilteredObjects( const ObjectId& objectFilter, bool searchAllRootedPackages )
	{
		std::vector< SmartPtr< return_t >> results;
		
		// Package name specifies a known package?
		//
		if( objectFilter.packageName().empty() == false )
		{
			auto package = getPackage( objectFilter.packageName() );
			if( package )
			{
				return package->findFiltered< return_t >( objectFilter );
			}
		}
		
		auto gatherFromPackage = [&]( const Package& package ) -> bool
		{
			if( package.matchesFilters( "Package", objectFilter.packageName() ))
			{
				std::vector< SmartPtr< return_t >> packageFiltered = package.findFiltered< return_t >( objectFilter );
				results.insert( results.end(), packageFiltered.begin(), packageFiltered.end() );
			}
			return false;	// Don't stop traversing.
		};
		
		if( searchAllRootedPackages )
		{
			getRootPackage().forEachMemberOfType< Package >( [&]( const Package::cptr& package )
															{
																gatherFromPackage( *package );
															} );
		}
		else
		{
			// Go through search packages (including the active package) accumulating objects that match
			// the given filter.
			//
			if( auto theActivePackage = activePackage() )
			{
				gatherFromPackage( *theActivePackage );
			}
			forEachSearchPackage( gatherFromPackage );
		}
		return results;
	}
	
	template< typename return_t >
	SmartPtr< return_t > createOrGetObject( const ObjectId& objectId, const Manifest::Map* properties, bool applyElementIfGet )
	{
		typedef typename std::remove_const< return_t >::type nonconst_return_t;
		typedef SmartPtr< nonconst_return_t > ptr_t;
		
		ptr_t result;
		
		if( objectId.objectName().empty() == false )	// Don't try to find anonymous objects.
		{
			result = getObject< nonconst_return_t >( objectId );
		}
		
		if( result )
		{
			if( properties && applyElementIfGet )
			{
				result->load( *properties );
			}
		}
		else
		{
			result = createObject< nonconst_return_t >( objectId, properties );
		}
		
		return result;
	}
	
	template< typename return_t >
	SmartPtr< return_t > createOrGetObject( ObjectNameRef objectName, const Manifest::Map* properties, bool applyElementIfGet )
	{
		return createOrGetObject< return_t >( ObjectId( return_t::StaticGetClassInfo().className(), objectName ),
											  properties,
											  applyElementIfGet );
	}

	template< typename return_t >
	SmartPtr< return_t > createOrGetObject( const Manifest::Object& initialization, bool applyElementIfGet )
	{
		ASSERT( &initialization );
		return createOrGetObject< return_t >( ObjectId{ initialization }, initialization.map.get(), applyElementIfGet );
	}

	inline bool isObject( const Object* object )
	{
		// Go through all the packages known to the root package (which should be everything),
		// and ask if any owns an object with this address.
		//
		bool found = false;
		
		const auto& rootPackage = getRootPackage();
		
		rootPackage.forEachMemberOfType< Package >( [ &found, object ] ( const Package::cptr& package )
							 {
								 if( package->has( object ))
								 {
									 found = true;
								 }
							 } );
		
		return found;
	}
}

#endif
