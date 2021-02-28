/*
 *  Asset.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 1/22/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 *	An Asset is an Object that knows how to load itself from a file.
 *	Examples include bitmaps, animations, meshes, audio, and the like.
 *	Assets extend Object from a memory management perspective in that:
 *		Each asset is always unique (one asset per file name per asset name).
 *		Assets can persist even when unreferenced by any particular object
 *		(to avoid the waste of frequently getting rid of slow-to-load assets).
 *	Assets extend Object from a loading perspective in that:
 *		Every asset can load itself from a file, given a file name referencing a valid file for that asset type.
 *	Assets work hand-in-hand with AssetPackage, which provides part or all of these features.
 */

#ifndef _FRESH_ASSET_H_INCLUDED_
#define _FRESH_ASSET_H_INCLUDED_

#include "Packages.h"
#include "ObjectSingleton.h"

namespace fr
{
	
	class Asset : public Object
	{
	public:
		
		virtual size_t getMemorySize() const = 0;
		virtual bool isLoaded() const { return true; }
		
	private:
		
		FRESH_DECLARE_CLASS_ABSTRACT( Asset, Object )
	};
	
	///////////////////////////////////////////////////////////////////////////////////////////////
	
	class AssetLoader : public Object
	{
	public:
		
		AssetLoader( const ClassInfo& assignedClassInfo, NameRef name, const std::string& basePath, const std::string& extension );
		
		SYNTHESIZE( std::string, filePath )
		SYNTHESIZE( std::string, basePath )
		SYNTHESIZE( std::string, extension )
		
		virtual const ClassInfo& assetClass() const						{ ASSERT( m_assetClass ); return *m_assetClass; }
		
		virtual void loadAsset( Asset::ptr asset )
		{
			REQUIRES( asset );
			++m_nTimesLoaded;
		}
		
		size_t timesLoaded() const							{ return m_nTimesLoaded; }

	protected:
			
		VAR( std::string, m_basePath );
		VAR( std::string, m_filePath );
		VAR( std::string, m_extension );
		VAR( ClassInfo::cptr, m_assetClass );
		size_t m_nTimesLoaded = 0;
		
		std::string getCompletePath() const
		{
			return decoratePath( m_basePath, m_filePath, m_extension );
		}
		
		static std::string decoratePath( const std::string& basePath, const std::string& filePath, const std::string& extension );
		
		FRESH_DECLARE_CLASS_ABSTRACT( AssetLoader, Object )
	};

	inline FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( AssetLoader )

	inline AssetLoader::AssetLoader( const ClassInfo& assignedClassInfo, NameRef name )
	:	Super( assignedClassInfo, name )
	{
		m_filePath = this->name();
	}
	
	inline AssetLoader::AssetLoader( const ClassInfo& assignedClassInfo, NameRef name, const std::string& basePath, const std::string& extension )
	:	Super( assignedClassInfo, name )
	,	m_basePath( basePath )
	,	m_extension( extension )
	{
		m_filePath = this->name();
	}
	
	///////////////////////////////////////////////////////////////////////////////////////////////

	class AssetLoader_Default : public AssetLoader
	{
	public:
		
		AssetLoader_Default( const ClassInfo& assignedClassInfo, NameRef name, const std::string& basePath, const std::string& extension );

		virtual void load( const Manifest::Map& properties ) override
		{
			Super::load( properties );
			if( isInert() ) return;
			
			// Get the passthrough sub-property, if any.
			//
			const auto passthroughProperty = properties.find( "passthrough" );
			
			if( passthroughProperty != properties.end() )
			{
				m_passthroughMap = passthroughProperty->second.first->get< Manifest::Map >();
			}
		}
		
		virtual void loadAsset( Asset::ptr asset ) override
		{
			REQUIRES( asset );
			
			Super::loadAsset( asset );
			
			if( !m_passthroughMap.empty() )
			{
				asset->load( m_passthroughMap );
			}
		}

	private:
		
		Manifest::Map m_passthroughMap;

		FRESH_DECLARE_CLASS( AssetLoader_Default, AssetLoader )
	};
	
	inline AssetLoader_Default::AssetLoader_Default( const ClassInfo& assignedClassInfo, NameRef name, const std::string& basePath, const std::string& extension )
	:	Super( assignedClassInfo, name, basePath, extension )
	{}
	
}

#endif
