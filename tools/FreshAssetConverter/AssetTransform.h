//
//  AssetTransform.h
//  fac
//
//  Created by Jeff Wofford on 2/6/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef fac_AssetTransform_h
#define fac_AssetTransform_h

#include "Asset.h"
#include "Property.h"
#include "FreshPath.h"

namespace fac
{
	using fr::path;
	
	class AssetTransform : public fr::Object
	{
		FRESH_DECLARE_CLASS_ABSTRACT( AssetTransform, Object )
	public:
		
		path fullPathSrc() const;
		path fullPathDest() const;

		void adjustPaths( const path& rootPathSrc, const path& rootPathDest, const path& basePathSrc, const path& basePathDest );
		
		virtual std::string assetClassName() const = 0;
		virtual std::string loaderClassName() const			{ return assetClassName() + "Loader"; }
		virtual std::string loaderExportClassName() const;
		virtual std::string defaultDestExtension() const;
		fr::ClassInfo::cptr loaderClass() const;
		fr::ClassInfo::cptr loaderExportClass() const;
		
		fr::ObjectId assetName() const;
		void assetName( Object::NameRef name )			{ REQUIRES( !name.empty() ); m_assetName = name; }
		fr::ObjectId loaderName() const;
		
		virtual bool apply();
		virtual void writeAssetToDatabase( std::ostream& assetDatabaseXML ) const;
		
		bool isSrcNewer() const;
		std::time_t destModifiedTime() const;
		
		virtual bool isApplicable() const;
		virtual bool isValid() const;
		
	protected:
		
		SYNTHESIZE_GET( const path&, srcPath )
		SYNTHESIZE_GET( const path&, destPath )
		SYNTHESIZE_GET( const std::string&, destExtension )
		
		virtual void writeAssetToDatabaseBegin( std::ostream& assetDatabaseXML ) const;
		virtual void writeAssetToDatabaseEnd( std::ostream& assetDatabaseXML ) const;
		
		bool hasValidAssetName() const;
		
	private:
		
		VAR( AssetTransform, path, m_srcPath );
		VAR( AssetTransform, path, m_destPath );			// If blank, and !m_assetName.empty() use assetName with m_destExtension extension.
															// If blank and no asset name, use srcPath with m_destExtension.
															// Otherwise, m_destExtension is set to this path's extension.
		VAR( AssetTransform, std::string, m_destExtension );
		VAR( AssetTransform, Object::Name, m_assetName );	// If empty, name == dest path.
		DVAR( AssetTransform, bool, m_forceOverrideNewer, false );
		VAR( AssetTransform, fr::ClassName, m_loaderExportClass );
		
		path m_rootPathSrc;			// The root path of all source assets.
		path m_rootPathDest;		// The root path of all dest assets.
		path m_basePathSrc;
		path m_basePathDest;
	};
			
}

#endif
