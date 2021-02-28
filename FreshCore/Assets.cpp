//
//  Assets.cpp
//  Fresh
//
//  Created by Jeff Wofford on 3/2/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "Assets.h"
#include "Property.h"
#include "Packages.h"

namespace fr
{
	
	class AssetClassMap : public Object
	{
		FRESH_DECLARE_CLASS( AssetClassMap, Object )
		
	public:

		typedef std::map< ClassName, ClassName > Mapping;
		
		SYNTHESIZE_GET( Mapping, map );
		
	private:
		
		VAR( Mapping, m_map );
	};
	
	FRESH_DEFINE_CLASS( AssetClassMap )
	DEFINE_VAR( AssetClassMap, Mapping, m_map );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( AssetClassMap )
	
	//////////////////////////////////////////
	
	class AssetLoaderPackage : public Package
	{
		FRESH_DECLARE_CLASS( AssetLoaderPackage, Package )
		
	public:
		
		virtual std::vector< Object::ptr > loadFromManifest( const Manifest& manifest ) override
		{
			auto objects = Super::loadFromManifest( manifest );
			if( !objects.empty() )
			{
				if( auto classMap = objects.front()->as< AssetClassMap >() )
				{
					m_classNameMap = classMap->map();
				}
			}
			return objects;
		}
		
		ClassInfo::cptr getAssetLoaderClassForAssetClass( const ClassInfo& assetClass ) const
		{
			std::string assetLoaderClassName( std::string( assetClass.className() ) + "Loader" );

			auto iter = m_classNameMap.find( assetClass.className() );
			if( iter != m_classNameMap.end() )
			{
				assetLoaderClassName = iter->second;
			}
		
			if( !isClass( assetLoaderClassName ))
			{
				// Use the default asset loader.
				//
				assetLoaderClassName = "AssetLoader_Default";
				ASSERT( isClass( assetLoaderClassName ));
			}
			
			return getClass( assetLoaderClassName );
		}
		
	protected:
		
		virtual Object::ptr requestGeneric( const ClassInfo& classInfo, NameRef objectName ) override
		{
			REQUIRES( objectName.empty() == false );
			
			Object::ptr object = Super::requestGeneric( classInfo, objectName );
			
			if( !object && classInfo.isKindOf( AssetLoader::StaticGetClassInfo() ))
			{
				// If an asset loader isn't specifically listed in the asset database
				// (typically in assets.fresh), we can create a default one with the obvious parameters.
				//
				AssetLoader::ptr loader = createObject< AssetLoader >( this, classInfo, objectName );
				ASSERT( loader );
				ASSERT( has( loader ));
				
				loader->filePath( objectName );
				
				return loader;
			}
			else
			{
				return object;
			}
		}


	private:
		
		std::map< ClassName, ClassName > m_classNameMap;	// Maps asset class name to loader class name
	};
	
	FRESH_DEFINE_CLASS( AssetLoaderPackage )
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( AssetLoaderPackage )
	
	//////////////////////////////////////////
	
	FRESH_DEFINE_CLASS( AssetPackage )
	
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( AssetPackage )
	
	AssetPackage::AssetPackage( const ClassInfo& assignedClassInfo, NameRef name )
	:	Super( assignedClassInfo, name )
	,	m_loaderPackage( createPackage< AssetLoaderPackage >( "~asset-database" ))
	{
		// Add to default object search packages.
		//
		addSearchPackage( this );
		
		// Set to "retain" (strong) mode.
		//
		retainMembers();
	}
	
	void AssetPackage::loadDatabase( const path& databasePath )
	{
		ASSERT( m_loaderPackage );
		m_loaderPackage->retainMembers();
		m_loaderPackage->loadFile( databasePath );
	}
	
	Object::ptr AssetPackage::requestGeneric( const ClassInfo& classInfo, NameRef objectName )
	{
		// Modifies the base class requestGeneric() pretty substantially.
		// In the case of Assets, will actually create the referenced asset object using a loader
		// if the asset doesn't already exist.
		
		tidy();
		
		Object::ptr object = Super::requestGeneric( classInfo, objectName );
		
		// If the caller is requesting an asset and we didn't find the object,
		// construct it from a loader.
		//
		if( !object && classInfo.isKindOf( Asset::StaticGetClassInfo() ))
		{
			// Seek the loader for this asset.
			//
			ClassInfo::cptr loaderClass = m_loaderPackage->getAssetLoaderClassForAssetClass( classInfo );
			if( !loaderClass )
			{
				FRESH_THROW( FreshException, "Could not find loader for apparent asset class " << classInfo.className() << "." );
			}
			
			if( auto loader = m_loaderPackage->request< AssetLoader >( *loaderClass, objectName /* doubling as the loader's name, too */ ) )
			{
				// Create the asset.
				//
				Asset::ptr asset = createObject< Asset >( this, classInfo, objectName );
				object = asset;
				
				// Let the loader load it.
				//
				loader->loadAsset( asset );
				
#if DEV_MODE && 0			// List loaded assets and their sizes.
				trace( "Asset " << asset << " created with " << asset->getMemorySize() << " bytes." );
#endif
			}
			else
			{
				// Couldn't find loader.
				return nullptr;
			}
		}
		
		return object;
	}

	AssetLoader::ptr AssetPackage::getLoader( const ObjectId& loaderName ) const
	{
		return m_loaderPackage->find< AssetLoader >( loaderName );
	}

	void AssetPackage::loadAssets( std::function< bool( const ClassInfo&, ObjectNameRef ) >&& predicateShouldLoadAsset )
	{
		m_loaderPackage->forEachMemberOfType< AssetLoader >( [&] ( const AssetLoader::cptr& assetLoader )
		{
			if( predicateShouldLoadAsset( assetLoader->assetClass(), assetLoader->name() ))
			{
				auto asset = requestGeneric( assetLoader->assetClass(), assetLoader->name() );
				if( !has( asset ))
				{
					add( asset );
				}
			}
		} );
	}

	void AssetPackage::sortZombiesForReleasePriority( std::vector< Object::ptr >::iterator begin, std::vector< Object::ptr >::iterator end )
	{
		std::sort( begin, end, [&]( const Object::ptr& a, const Object::ptr& b )
				  {
					  return static_cast< Asset* >( a.get() )->getMemorySize() > static_cast< Asset* >( b.get() )->getMemorySize();
				  } );
	}
	
	size_t AssetPackage::getZombieSizeBytes( const Object& object ) const
	{
		if( auto asset = object.as< const Asset >() )
		{
			return asset->getMemorySize();
		}
		else
		{
			return Super::getZombieSizeBytes( object );
		}
	}

	////////////////////////////////////////////////////////////////////////////////////

	void AssetPackage::dumpAssetReport( std::ostream& out ) const
	{
		size_t nBytesTotal = 0;
		
		out << "============================ MANAGED ASSETS ==============================\n";
		out << "Asset                                             Loaded Used?  Refs Bytes\n";
		
		m_loaderPackage->forEachMemberOfType< AssetLoader >( [&out, &nBytesTotal] ( const AssetLoader::cptr& assetLoader )
		{
			const auto timesLoaded = assetLoader->timesLoaded();
			out << std::left << std::setw( 50 ) << assetLoader->name();
			out << std::setw( 7 ) << timesLoaded;
			
			std::vector< Object::ptr > objects = getFilteredObjects( ObjectId( "Asset", assetLoader->name() ));
			
			out << std::setw( 7 );
			if( timesLoaded == 0 )
			{
				out << "UNUSED";
				
				if( !objects.empty() )
				{
					out << "WARNING: Supposedly unused asset actually has " << objects.size() << " instances.\t";
				}
			}
			else
			{
				out << "Y";		// Standing in for "UNUSED"
				
				if( objects.empty() )
				{
					out << "(none)";
				}
				else
				{
					// We're relying on this upcast being valid.
					const Asset& asset = *static_cast< const Asset* >( static_cast< Object* >( objects.front() ));
					
					nBytesTotal += asset.getMemorySize();
					out << std::setw( 5 ) << asset.getReferenceCount();
					out << std::setw( 15 ) << getByteCountString( asset.getMemorySize() );
					
					if( objects.size() > 1 )
					{
						out << " WARNING: More than one (" << objects.size() << ") asset instance created.\t";
					}
				}
			}
			
			out << std::endl;
		} );
		
		out << "---------------------------------------------------------------------------\n";
		out << "Total instantiated Asset bytes: ";
		out << getByteCountString( nBytesTotal ) << std::endl;
		out << "===========================================================================\n";
	}
}
