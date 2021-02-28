//
//  AssetTransformTexture.h
//  fac
//
//  Created by Jeff Wofford on 2/6/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef fac_AssetTransformTexture_h
#define fac_AssetTransformTexture_h

#include "AssetTransform.h"
#include "FreshVector.h"
#include "Texture.h"

namespace fac
{
	using fr::vec2;
	
	class AssetTransformTexture : public AssetTransform
	{
		FRESH_DECLARE_CLASS( AssetTransformTexture, AssetTransform )
	public:
		
		virtual bool apply() override;
		
		virtual std::string assetClassName() const override { return "Texture"; }
		
	protected:
		
		virtual void writeAssetToDatabaseEnd( std::ostream& assetDatabaseXML ) const override;
		void processResultText( const std::string& resultText );
		
	private:
		
		DVAR( AssetTransformTexture, bool, m_trimTransparency, false );
		DVAR( AssetTransformTexture, vec2, m_postScale, vec2( 1.0f, 1.0f ));
		DVAR( AssetTransformTexture, bool, m_resizeToPowerOf2, false );
		DVAR( AssetTransformTexture, bool, m_reshapeToPowerOf2, false );		// If true, trumps m_resizeToPowerOf2.
		DVAR( AssetTransformTexture, bool, m_layersInitiallyAllHide, false );
		DVAR( AssetTransformTexture, bool, m_layersInitiallyAllShow, false );	// If true, trumps m_layersInitiallyAllHide.
		DVAR( AssetTransformTexture, int, m_jpegQuality, 10 );
		VAR( AssetTransformTexture, std::vector< std::string >, m_layersToShow );
		VAR( AssetTransformTexture, std::vector< std::string >, m_layersToHide );
		DVAR( AssetTransformTexture, bool, m_forceReopenSrcFile, true );
		DVAR( AssetTransformTexture, bool, m_closeSrcWhenDone, true );
		DVAR( AssetTransformTexture, bool, m_doMipMap, true );
		DVAR( AssetTransformTexture, unsigned int, m_alphaBorderFlags, 0 );	// Flags: { 1, 2, 4, 8 } => { left, top, right, bottom }
		DVAR( AssetTransformTexture, fr::Texture::ClampMode, m_clampModeU, fr::Texture::ClampMode::Clamp );
		DVAR( AssetTransformTexture, fr::Texture::ClampMode, m_clampModeV, fr::Texture::ClampMode::Clamp );
		
		fr::Vector2ui m_dimensions, m_originalDimensions;
		
		void configurePhotoshopScript( std::ofstream& out ) const;
		
	};
	
}

#endif
