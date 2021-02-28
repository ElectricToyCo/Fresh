//
//  Assets.h
//  Fresh
//
//  Created by Jeff Wofford on 3/2/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_Assets_h
#define Fresh_Assets_h

#include "Asset.h"
#include "Package.h"

namespace fr
{
	
	class AssetLoaderPackage;
	
	class AssetPackage : public Package
	{
	public:
		
		void loadDatabase( const path& databasePath );
		
		AssetLoader::ptr getLoader( const ObjectId& loaderName ) const;
		// May return null.
		
		void loadAssets( std::function< bool( const ClassInfo&, ObjectNameRef ) >&& predicateShouldLoadAsset );
		// Preloads and retains all assets for which predicateShouldLoadAsset( asset's id ) is true.
		
		// For development
		//
		void dumpAssetReport( std::ostream& out ) const;
		
	protected:

		virtual Object::ptr requestGeneric( const ClassInfo& classInfo, NameRef objectName ) override;
		
		virtual void sortZombiesForReleasePriority( std::vector< Object::ptr >::iterator begin, std::vector< Object::ptr >::iterator end ) override;
		virtual size_t getZombieSizeBytes( const Object& object ) const  override;

	private:
		
		SmartPtr< AssetLoaderPackage > m_loaderPackage;
		
		FRESH_DECLARE_CLASS( AssetPackage, Package )
	};
	
}

#endif
