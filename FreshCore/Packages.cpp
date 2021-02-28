//
//  Packages.cpp
//  Fresh
//
//  Created by Jeff Wofford on 2/28/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "Packages.h"
#include <deque>

namespace
{
	typedef std::deque< fr::Package::wptr > SearchPackages;
	SearchPackages& getSearchPackages()
	{
		static SearchPackages packages;
		return packages;
	}
	
	void cleanupSearchPackages()
	{
		// Deletes null package pointers resulting from self-deleting packages.
		//
		SearchPackages& packages = getSearchPackages();
		
		removeElements( packages, std::mem_fn( &fr::Package::wptr::isNull ));
	}
}

namespace fr
{
	Package& getRootPackage()
	{
		static Package::ptr rootPackage = createObject< Package >( "~" );
		return *rootPackage;
	}

	Package& getTransientPackage()
	{
		static Package::ptr transientPackage = createPackage( "~transient" );
		return *transientPackage;
	}
	
	//////////////////////////////////////////////
	
	void addSearchPackage( Package::wptr package )
	{
		REQUIRES( package );
		if( package->name()  == "~transient" )
		{
			dev_warning( "Adding the ~transient package to search packages. Is that really what you intend?" );
		}
		
		cleanupSearchPackages();
		
		SearchPackages& packages = getSearchPackages();
		if( packages.end() == std::find( packages.begin(), packages.end(), package ))
		{
			packages.push_front( package );
		}
	}
	
	void removeSearchPackage( Package::wptr package )
	{
		REQUIRES( package );
		cleanupSearchPackages();
		
		SearchPackages& packages = getSearchPackages();
		
		auto iter = std::find( packages.begin(), packages.end(), package );
		if( packages.end() != iter )
		{
			packages.erase( iter );
		}
	}
	
	void forEachSearchPackage( std::function< bool ( Package& ) >&& fnPerPackage )
	{
		cleanupSearchPackages();
		
		// Copy so that internal calls to add and remove don't jack with iterators.
		
		SearchPackages packagesCopy = getSearchPackages();
		
		for( const auto& package : packagesCopy )
		{
			if( package )	// Packages might be dying within fnPerPackage calls.
			{
				bool done = fnPerPackage( *package );
				
				if( done )
				{
					break;
				}
			}
		}
	}
}
