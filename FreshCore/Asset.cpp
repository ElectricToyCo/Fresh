/*
 *  Asset.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 1/22/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#include "Asset.h"
#include "Property.h"

namespace fr
{
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( AssetLoader_Default )
	
	
	FRESH_DEFINE_CLASS( AssetLoader_Default );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( Asset )
	
	DEFINE_VAR( AssetLoader, std::string, m_basePath );
	DEFINE_VAR( AssetLoader, std::string, m_filePath );
	DEFINE_VAR( AssetLoader, std::string, m_extension );
	DEFINE_VAR( AssetLoader, ClassInfo::cptr, m_assetClass );

	///////////////////////////////////////////////////////

	std::string AssetLoader::decoratePath( const std::string& basePath, const std::string& filePath, const std::string& extension )
	{
		if( filePath.empty() )
		{
			return filePath;
		}
		
		std::string completePath( basePath );
		if( !completePath.empty() )
		{
			// Add / if necessary.
			//
			if( completePath.back() != '/' )
			{
				completePath += '/';
			}
		}
		
		completePath += filePath;
		
		// Add the extension, if any.
		if( !extension.empty())
		{
			// Add the dot if needed.
			//
			if( extension.front() != '.' )
			{
				completePath += '.';
			}
			completePath += extension;
		}
		
		return completePath;
	}
}
