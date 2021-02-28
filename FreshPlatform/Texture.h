#ifndef TEXTURE_H_INCLUDED_
#define TEXTURE_H_INCLUDED_

#include "Asset.h"
#include "Property.h"
#include "Color.h"
#include "Vector2.h"
#include "FreshOpenGL.h"

namespace fr
{

	class Texture : public Asset
	{
	public:
		
		enum class ClampMode
		{
			Wrap,
			Clamp
		};
		
		enum class FilterMode
		{
			Nearest,
			Bilinear,
			Trilinear,
		};
		
		enum class AlphaUsage
		{
			None,
			Alpha,
			AlphaPremultiplied
		};
		
		virtual ~Texture();
		
		void destroy();

		virtual void loadFromPixelData( const unsigned char* pixelData, const Vector2ui& dimensions, AlphaUsage alphaUsage = AlphaUsage::None, ClampMode clampModeU = ClampMode::Clamp, ClampMode clampModeV = ClampMode::Clamp, bool doBuildMipmaps = false, unsigned int alphaBorderFlags = 0, bool retainTexelData = false );
		virtual void loadFromColors( const std::vector< Color >& colors, const Vector2ui& dimensions, ClampMode clampModeU = ClampMode::Clamp, ClampMode clampModeV = ClampMode::Clamp, bool doBuildMipmaps = false, unsigned int alphaBorderFlags = 0, bool retainTexelData = false );
		
		virtual void loadFromFile( const path& path, ClampMode clampModeU = ClampMode::Clamp, ClampMode clampModeV = ClampMode::Clamp, Color colorKey = 0, bool doBuildMipmaps = false, unsigned int alphaBorderFlags = 0, bool retainTexelData = false );
			// REQUIRES( !strFilename.empty() );
		
		void savePNG( const fr::path& toPath ) const;
		
		virtual void enableMipmapping();
			// REQUIRES( getTextureId() );
			// REQUIRES( isPowerOfTwo( dimensions().x ) && isPowerOfTwo( dimensions().y ));
		
		virtual void assumeId( unsigned int idTexture, const Vector2ui& dimensions );
		// REQUIRES( idTexture > 0 );
		uint getTextureId() const						{ return m_idTexture; }
		
		const Vector2ui& dimensions() const				{ return m_dimensions; }
		const Vector2ui& getOriginalDimensions() const	{ return m_originalDimensions; }
		
		virtual size_t getMemorySize() const override;
			// Gets the size in bytes of the texture memory, assuming 4-byte RGBA pixels.
			// Does not include the size of the texture object itself--only the pixel data.

		// Best (but not required) to do this through loading rather than after loading.
		void setClampMode( ClampMode clampModeU, ClampMode clampModeV );
		void filterMode( FilterMode filterMode_ );
		
		SYNTHESIZE_GET( AlphaUsage, alphaUsage )
		
		const std::vector< Color >& getLoadedTexels() const;
		
#if !GL_ES_VERSION_2_0
		std::vector< unsigned char > readTexels() const;
		
		typedef std::function< bool( int iLevel, int width, int height, std::vector< unsigned char>& pixelBytes ) > MipmapFunction;
		void forEachMipmapLevel( MipmapFunction&& fn ) const;
		// The MipmapFunction should return true if it modified the pixelBytes and thus requires them to be re-uploaded to the GPU texture proper.
#endif
		
		static int maxAllowedSize();
		
		static bool setTransparencyInPixelData( unsigned char* pixelData, size_t nTexels, Color colorKey = 0 );
		// Returns true iff found and set at least one alpha pixel
		
		static void paintBorder( unsigned int* abgrPixels, int width, int height, unsigned int borderFlags, int borderSize = 1, Color borderColor = Color::Invisible );
		
	private:
		
		uint m_idTexture = 0;

		VAR( Vector2ui, m_dimensions );
		VAR( Vector2ui, m_originalDimensions );	// The dimensions of the image prior to resizing for power-of-two conformance.
		DVAR( AlphaUsage, m_alphaUsage, AlphaUsage::None );
		
		bool m_haveMipMaps = false;
		
		std::vector< Color > m_retainedTexelData;
		
		friend struct TextureLoader;

		FRESH_DECLARE_CLASS( Texture, Asset )
	};
	
	//////////////////////////////////////////////////////////////////////////////////////

	struct TextureLoader : public AssetLoader
	{
		virtual void loadAsset( Asset::ptr asset ) override;
		// REQUIRES( asset );
		
	protected:
		
		DVAR( Texture::ClampMode, m_clampModeU, Texture::ClampMode::Clamp );
		DVAR( Texture::ClampMode, m_clampModeV, Texture::ClampMode::Clamp );
		DVAR( Texture::FilterMode, m_filterMode, Texture::FilterMode::Bilinear );
		DVAR( bool, m_doMipMap, true );
		DVAR( bool, m_retainTexelData, false );
		DVAR( Color, m_colorKey, 0 );
		DVAR( unsigned int, m_alphaBorderFlags, 0 );	// Each bit indicates whether a border is wanted or not: bit 1, 2, 4, 8 => left, top, right, bottom
		VAR( Vector2ui, m_originalDimensions );
		
		FRESH_DECLARE_CLASS( TextureLoader, AssetLoader )	
	};
		
	//////////////////////////////////////////////////////////////////

	FRESH_ENUM_STREAM_IN_BEGIN( Texture, ClampMode )
	FRESH_ENUM_STREAM_IN_CASE( Texture::ClampMode, Wrap )
	FRESH_ENUM_STREAM_IN_CASE( Texture::ClampMode, Clamp )
	FRESH_ENUM_STREAM_IN_END()
	
	FRESH_ENUM_STREAM_OUT_BEGIN( Texture, ClampMode )
	FRESH_ENUM_STREAM_OUT_CASE( Texture::ClampMode, Wrap )
	FRESH_ENUM_STREAM_OUT_CASE( Texture::ClampMode, Clamp )
	FRESH_ENUM_STREAM_OUT_END()

	//////////////////////////////////////////////////////////////////

	FRESH_ENUM_STREAM_IN_BEGIN( Texture, FilterMode )
	FRESH_ENUM_STREAM_IN_CASE( Texture::FilterMode, Nearest )
	FRESH_ENUM_STREAM_IN_CASE( Texture::FilterMode, Bilinear )
	FRESH_ENUM_STREAM_IN_CASE( Texture::FilterMode, Trilinear )
	FRESH_ENUM_STREAM_IN_END()
	
	FRESH_ENUM_STREAM_OUT_BEGIN( Texture, FilterMode )
	FRESH_ENUM_STREAM_OUT_CASE( Texture::FilterMode, Nearest )
	FRESH_ENUM_STREAM_OUT_CASE( Texture::FilterMode, Bilinear )
	FRESH_ENUM_STREAM_OUT_CASE( Texture::FilterMode, Trilinear )
	FRESH_ENUM_STREAM_OUT_END()
	
	//////////////////////////////////////////////////////////////////
	
	FRESH_ENUM_STREAM_IN_BEGIN( Texture, AlphaUsage )
	FRESH_ENUM_STREAM_IN_CASE( Texture::AlphaUsage, None )
	FRESH_ENUM_STREAM_IN_CASE( Texture::AlphaUsage, Alpha )
	FRESH_ENUM_STREAM_IN_CASE( Texture::AlphaUsage, AlphaPremultiplied )
	FRESH_ENUM_STREAM_IN_END()
	
	FRESH_ENUM_STREAM_OUT_BEGIN( Texture, AlphaUsage )
	FRESH_ENUM_STREAM_OUT_CASE( Texture::AlphaUsage, None )
	FRESH_ENUM_STREAM_OUT_CASE( Texture::AlphaUsage, Alpha )
	FRESH_ENUM_STREAM_OUT_CASE( Texture::AlphaUsage, AlphaPremultiplied )
	FRESH_ENUM_STREAM_OUT_END()

}		// END namespace fr

#endif
