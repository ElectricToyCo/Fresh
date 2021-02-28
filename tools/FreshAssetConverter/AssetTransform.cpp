//
//  AssetTransform.cpp
//  fac
//
//  Created by Jeff Wofford on 2/6/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "AssetTransform.h"
#include "Objects.h"
#include "FreshFile.h"
#include "Asset.h"
using namespace fr;

namespace fac
{
	
	DEFINE_VAR( AssetTransform, path, m_srcPath );
	DEFINE_VAR( AssetTransform, path, m_destPath );
	DEFINE_VAR( AssetTransform, std::string, m_destExtension );
	DEFINE_VAR( AssetTransform, Object::Name, m_assetName );
	DEFINE_DVAR( AssetTransform, bool, m_forceOverrideNewer );
	DEFINE_VAR( AssetTransform, fr::ClassName, m_loaderExportClass );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( AssetTransform )
	
	fr::ObjectId AssetTransform::assetName() const
	{
		return ObjectId( assetClassName(), m_assetName );
	}
	
	std::string AssetTransform::loaderExportClassName() const
	{
		if( m_loaderExportClass.empty() )
		{
			return loaderClassName();
		}
		else
		{
			return m_loaderExportClass;
		}
	}
	
	fr::ObjectId AssetTransform::loaderName() const
	{
		return ObjectId( loaderClassName(), m_assetName );
	}
	
	fr::ClassInfo::cptr AssetTransform::loaderClass() const
	{
		return getClass( loaderClassName() );
	}

	fr::ClassInfo::cptr AssetTransform::loaderExportClass() const
	{
		return getClass( loaderExportClassName() );
	}

	std::string AssetTransform::defaultDestExtension() const
	{
		auto loader = loaderExportClass();
		std::string extension = loader->defaultObject()->getPropertyValue( "extension" );
		
		if( !extension.empty() && extension.front() != '.' )
		{
			extension.insert( extension.begin(), '.' );
		}
		
		return extension;
	}
	
	bool AssetTransform::apply()
	{		
		// Ensure path for dest file is present.
		//
		return create_directories( fullPathDest().parent_path() );
	}
	
	void AssetTransform::writeAssetToDatabase( std::ostream& assetDatabaseXML ) const
	{
		writeAssetToDatabaseBegin( assetDatabaseXML );
		writeAssetToDatabaseEnd( assetDatabaseXML );
	}
	
	void AssetTransform::writeAssetToDatabaseBegin( std::ostream& assetDatabaseXML ) const
	{
		assetDatabaseXML << "\t<object class=\"" << loaderExportClassName() << "\" name=\"" << m_assetName << "\">\n";
		
		// Figure out the base path for the loader.
		//
		auto loader = loaderExportClass();
		std::istringstream stream( loader->defaultObject()->getPropertyValue( "basePath" ));
		path loaderBasePath;
		stream >> loaderBasePath;
		
		path assetPath( m_basePathDest / m_destPath );		// Relative to root.

		//
		// Strip the loaderBasePath from the left side of the relative asset path.
		//
		
		// Convert the paths into containers for easier manipulation.
		
		std::list< std::string > loaderParentPaths;
		loaderParentPaths.assign( loaderBasePath.begin(), loaderBasePath.end() );
		
		std::list< std::string > assetParentPaths;
		assetParentPaths.assign( assetPath.begin(), assetPath.end() );
		
		while( !loaderParentPaths.empty() && loaderParentPaths.front() == assetParentPaths.front() )
		{
			loaderParentPaths.pop_front();
			assetParentPaths.pop_front();
			
			ASSERT( !assetParentPaths.empty() );
		}
		
		// Convert the asset path container back into a path.
		//
		assetPath.clear();
		for( const auto& subpath : assetParentPaths )
		{
			assetPath /= subpath;
		}
		
		assetPath = change_extension( assetPath, "" );
		
		if( assetPath.string() != m_assetName )
		{
			assetDatabaseXML << "\t\t<filePath>" << assetPath << "</filePath>\n";
		}
		
		if( m_destExtension != defaultDestExtension() )
		{
			// Remove leading dot.
			//
			std::string extension = m_destExtension;
			if( !extension.empty() && extension.front() == '.' )
			{
				extension.erase( extension.begin() );
			}
			
			assetDatabaseXML << "\t\t<extension>" << extension << "</extension>\n";
		}
	}
	
	void AssetTransform::writeAssetToDatabaseEnd( std::ostream& assetDatabaseXML ) const
	{
		assetDatabaseXML << "\t</object>\n";
	}

	bool AssetTransform::isSrcNewer() const
	{
		return m_forceOverrideNewer || ( getFileLastModifiedTime( fullPathSrc().string() ) > destModifiedTime() );
	}

	std::time_t AssetTransform::destModifiedTime() const
	{
		return getFileLastModifiedTime( fullPathDest().string() );
	}
	
	bool AssetTransform::isApplicable() const
	{
		// Returns true iff this transform can in fact be applied.
		//
		return isValid() && exists( fullPathSrc() );
	}
	
	bool AssetTransform::isValid() const
	{
		return hasValidAssetName() && m_srcPath.empty() == false;
	}
	
	path AssetTransform::fullPathSrc() const
	{
		return m_rootPathSrc / m_basePathSrc / m_srcPath;
	}
	
	path AssetTransform::fullPathDest() const
	{
		return m_rootPathDest / m_basePathDest / m_destPath;
	}

	void AssetTransform::adjustPaths( const path& rootPathSrc, const path& rootPathDest, const path& basePathSrc, const path& basePathDest )
	{
		// Store the root and base paths.
		//
		m_rootPathSrc = rootPathSrc;
		m_rootPathDest = rootPathDest;
		m_basePathSrc = basePathSrc;
		m_basePathDest = basePathDest;
		
		// Fixup the destination file extension.
		//
		if( m_destExtension.empty() )
		{
			// None provided. Ask the transform class for the default.
			//
			m_destExtension = defaultDestExtension();
		}
		
		// Ensure we have a dot at the front of the extension.
		//
		if( !m_destExtension.empty() && m_destExtension.front() != '.' )
		{
			m_destExtension.insert( m_destExtension.begin(), '.' );		// Prepend the dot on the extension.
		}
		
		// Ensure the destPath is either specified legally or maps to the srcPath.
		//
		if( m_destPath.empty() )
		{
			// Is assetName available? If so, use that.
			//
			if( !m_assetName.empty() )
			{
				m_destPath = m_assetName;
			}
			else // Else use source path.
			{
				m_destPath = m_srcPath;
			}
			m_destPath = change_extension( m_destPath, m_destExtension );
		}
		else
		{
			std::string pathBasedDestExtension = m_destPath.extension();
			
			if( !m_destExtension.empty() && m_destExtension != pathBasedDestExtension )
			{
				std::cerr << "WARNING: Asset with explicit destPath '" << m_destPath << "' indicated a contradictory destExtension '" << m_destExtension << "'.\n";
			}
			
			m_destExtension = pathBasedDestExtension;
		}
		
		// Determine the asset name if it isn't otherwise indicated.
		//
		if( m_assetName.empty() )
		{
			m_assetName = change_extension( m_destPath, "" ).string();		// Removing extension.
		}
	}
	
	bool AssetTransform::hasValidAssetName() const
	{
		return m_assetName.empty() == false || m_destPath.empty() == false || m_srcPath.empty() == false;
	}
	
}

