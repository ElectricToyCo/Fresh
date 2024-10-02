#include "FreshFile.h"
#include "Texture.h"
#include "Objects.h"
#include "Renderer.h"
#include "FreshOpenGL.h"
#include "CommandProcessor.h"
#include "Application.h"
#include "ImageLoader.h"
#include "lodepng.h"

namespace
{
#if DEV_MODE && 0
	void dumpPixels( const unsigned char* pixels, const int width, const int height, const int nChannels = 4, std::ostream& out = std::cout )
	{
		for( int y = 0; y < height; ++y )
		{
			out << "|";
			for( int x = 0; x < width; ++x )
			{
				for( int c = 0; c < nChannels; ++c, ++pixels )
				{
					out << std::hex << std::setw( 2 ) << std::setfill( '0' ) << int( *pixels );
				}
				out << "|";
			}
			out << std::endl;
		}
	}
#endif

}

namespace fr
{

	FRESH_DEFINE_CLASS( Texture )

	DEFINE_VAR( Texture, Vector2ui, m_dimensions );
	DEFINE_VAR( Texture, Vector2ui, m_originalDimensions );
	DEFINE_VAR( Texture, AlphaUsage, m_alphaUsage );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( Texture )

	Texture::~Texture()
	{
		destroy();
	}

	void Texture::destroy()
	{
		if( m_idTexture > 0 )
		{
			// Unbind. This is necessary because of the following possible sequence:
			//		Texture id 4 (e.g.) is currently bound.
			//		Texture id 4 is destroyed
			//		A new texture is created. This re-allocates id #4.
			//		When the new texture is bound using Renderer::bindTextureId(), the renderer thinks #4 is current,
			//		so it doesn't bother binding the new texture. The new texture is new and needs to be bound separately,
			//		but its id is not new, so it is not recognized as new.
			//
			// A less aggressive form of this function would first verify that the currently bound texture is m_idTexture
			// before unbinding it. This would be a performance improvement. But since texture deletion is rare and tends
			// to get lumped up during loads or memory warnings, I'm not worried at this point.
			//
			if( Renderer::doesExist() )
			{
				Renderer::instance().bindTextureId( 0 );
			}

			glDeleteTextures( 1, &m_idTexture );
			m_idTexture = 0;
			HANDLE_GL_ERRORS();
		}
	}

	void Texture::loadFromFile( const path& path, ClampMode clampModeU, ClampMode clampModeV, Color colorKey, bool doBuildMipmaps, unsigned int alphaBorderFlags, bool retainTexelData )
	{
		REQUIRES( !path.empty() );

		destroy();

		bool hasAlpha, hasAlphaPremultiplied;
		uint width, height;
		auto pixels = ImageLoader::loadImageRGBA( path, width, height, hasAlpha, hasAlphaPremultiplied, ImageLoader::WantPremultiplied );

		m_dimensions.x = width;
		m_dimensions.y = height;

		bool foundTransparencyAlpha = setTransparencyInPixelData( pixels.get(), m_dimensions.x * m_dimensions.y, colorKey );
		hasAlpha = hasAlpha || foundTransparencyAlpha;

		const auto alphaUsage = hasAlpha ? ( hasAlphaPremultiplied ? AlphaUsage::AlphaPremultiplied : AlphaUsage::Alpha ) : AlphaUsage::None;

		loadFromPixelData( pixels.get(), m_dimensions, alphaUsage, clampModeU, clampModeV, doBuildMipmaps, alphaBorderFlags, retainTexelData );
	}

	void Texture::loadFromPixelData( const unsigned char* pixelData, const Vector2ui& dimensions, AlphaUsage alphaUsage, ClampMode clampModeU, ClampMode clampModeV, bool doBuildMipmaps, unsigned int alphaBorderFlags, bool retainTexelData )
	{
		REQUIRES( dimensions.x > 0 && dimensions.y > 0 );
		ASSERT( pixelData );

		destroy();

		// Verify that the texture will fit.
		//
#ifdef DEBUG
		const uint maxAllowedTextureSize = Texture::maxAllowedSize();
		ASSERT_MSG( dimensions.x <= maxAllowedTextureSize, this << " had width of " << dimensions.x << " > " << maxAllowedTextureSize );
		ASSERT_MSG( dimensions.y <= maxAllowedTextureSize, this << " had height of " << dimensions.y << " > " << maxAllowedTextureSize );
#endif

		if( doBuildMipmaps && !( isPowerOfTwo( dimensions.x ) && isPowerOfTwo( dimensions.y ) ))
		{
			dev_warning( this << " wants mipmaps but is not of power-of-two dimensions " << dimensions << ". Refusing mipmaps on this texture." );
			doBuildMipmaps = false;
		}

#if DEV_MODE && 0
		dumpPixels( pixelData, dimensions.x, dimensions.y );
#endif

		m_dimensions = dimensions;
		m_alphaUsage = alphaUsage;

		m_originalDimensions = m_dimensions * static_cast< uint >( Application::instance().config().contentScale() );

		// Create a new OpenGL Texture, bind it, and load it with the pixel data.
		//
		glGenTextures( 1, &m_idTexture );

		Renderer::instance().bindTextureId( m_idTexture );

		setClampMode( clampModeU, clampModeV );

		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, m_dimensions.x, m_dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData );

		if( doBuildMipmaps )
		{
			glGenerateMipmap( GL_TEXTURE_2D );
			m_haveMipMaps = true;
		}

#if 0		// Support for alphaBorderFlags

		// For each mipmap level, carve out an alpha border where desired.
		//
		if( alphaBorderFlags != 0 )
		{
#if !GL_ES_VERSION_2_0
			forEachMipmapLevel( [&]( int /* iLevel */, int width, int height, unsigned char* pixelBytes ) -> bool
							   {
								   paintBorder( reinterpret_cast< unsigned int* >( pixelBytes ), width, height, alphaBorderFlags, 1, 0x00FFFFFF );
								   return true;
							   } );
#else
			dev_warning( this << " requested an alpha border, but this cannot be provided with OpenGL ES 2.0." );
#endif
		}
#endif

		// Set the texture parameters for filtering.
		//
		filterMode( FilterMode::Bilinear );

		m_retainedTexelData.clear();
		if( retainTexelData )
		{
			const uint* colorsBegin = reinterpret_cast< const uint* >( pixelData );
			size_t nTexels = m_dimensions.x * m_dimensions.y;
			m_retainedTexelData.resize( nTexels );
			std::transform( colorsBegin, colorsBegin + nTexels, m_retainedTexelData.begin(), []( uint colorARGB )
						   {
							   return Color{ colorARGB };
						   } );
		}

		HANDLE_GL_ERRORS();
	}

	void Texture::loadFromColors( const std::vector< Color >& colors, const Vector2ui& dimensions, ClampMode clampModeU, ClampMode clampModeV, bool doBuildMipmaps, unsigned int alphaBorderFlags, bool retainTexelData )
	{
		ASSERT( colors.size() >= dimensions.x * dimensions.y );
		std::vector< unsigned int > pixelData( colors.size() );

		std::transform( colors.begin(), colors.end(), pixelData.begin(), []( const Color& color ) { return color.getABGR(); } );

		loadFromPixelData( reinterpret_cast< unsigned char* >( pixelData.data() ), dimensions, AlphaUsage::AlphaPremultiplied, clampModeU, clampModeV, doBuildMipmaps, alphaBorderFlags, false );

		if( retainTexelData )
		{
			m_retainedTexelData = colors;
		}
	}

	void Texture::savePNG( const fr::path& toPath ) const
	{
		if( !m_retainedTexelData.empty() && m_dimensions.x > 0 && m_dimensions.y > 0 )
		{
			create_directories( toPath.parent_path() );

			const auto errCode = lodepng::encode( toPath.string(), reinterpret_cast< const unsigned char* >( m_retainedTexelData.data() ), m_dimensions.x, m_dimensions.y );

			if( errCode )
			{
				dev_error( "PNG Encoding error " << errCode << ": " << lodepng_error_text( errCode ));
			}
		}
	}

	void Texture::enableMipmapping()
	{
		REQUIRES( getTextureId() );
		REQUIRES( isPowerOfTwo( dimensions().x ) && isPowerOfTwo( dimensions().y ));
		Renderer::instance().bindTextureId( m_idTexture );
		glGenerateMipmap( GL_TEXTURE_2D );
		HANDLE_GL_ERRORS();
		m_haveMipMaps = true;
	}

	int Texture::maxAllowedSize()
	{
		GLint maxAllowedTextureSize = 0;
		glGetIntegerv( GL_MAX_TEXTURE_SIZE, &maxAllowedTextureSize );
        ASSERT( maxAllowedTextureSize > 0 );

		return maxAllowedTextureSize;
	}

	bool Texture::setTransparencyInPixelData( unsigned char* pixelData, size_t nTexels, Color colorKey /* = 0 */ )
	{
		ASSERT( pixelData );

		bool foundTransparentPixel = false;

		//
		// Convert colorkey'd texels to black with 0 alpha.
		//
		if( colorKey != 0 )
		{
			unsigned char* byteRGBA = pixelData;
			unsigned char* lastByteRGBA = byteRGBA + ( nTexels * 4 );

			unsigned char colorKeyR = static_cast< unsigned char >( colorKey.getR() );
			unsigned char colorKeyG = static_cast< unsigned char >( colorKey.getG() );
			unsigned char colorKeyB = static_cast< unsigned char >( colorKey.getB() );

			while( byteRGBA < lastByteRGBA )
			{
				// Does color key match?
				if( byteRGBA[ 0 ] == colorKeyR &&
				   byteRGBA[ 1 ] == colorKeyG &&
				   byteRGBA[ 2 ] == colorKeyB )
				{
					*( (unsigned int*) byteRGBA ) = 0;		// Assign the whole RGBA color to black with 0 alpha.
					foundTransparentPixel = true;
				}

				byteRGBA += 4;
			}
		}

		return foundTransparentPixel;
	}

	void Texture::paintBorder( unsigned int* abgrPixels, int width, int height, unsigned int borderFlags, int borderSize, Color borderColor )
	{
		ASSERT( abgrPixels );
		ASSERT( width && height && borderSize );

		if( borderFlags == 0 )
		{
			// No painting actually wanted.
			//
			return;
		}

		const unsigned int color = borderColor.getABGR();

		// Horizontal paint.
		//
		if( borderFlags & ( 2 | 8 ))	// Top or bottom
		{
			for( int y = 0; y < borderSize && y < height; ++y )
			{
				for( int x = 0; x < width; ++x )
				{
					// Top
					if( borderFlags & 2 ) abgrPixels[ x + y * width ] = color;

					// Bottom
					if( borderFlags & 8 ) abgrPixels[ x + ( height - 1 - y ) * width ] = color;
				}
			}
		}

		// Vertical paint.
		//
		if( borderFlags & ( 1 | 4 ))	// Left or Right
		{
			for( int x = 0; x < borderSize && x < width; ++x )
			{
				for( int y = 0; y < height; ++y )
				{
					// Left
					if( borderFlags & 1 ) abgrPixels[ x + y * width ] = color;

					// Right
					if( borderFlags & 4 ) abgrPixels[ ( width - 1 - x ) + y * width ] = color;
				}
			}
		}
	}

	void Texture::assumeId( unsigned int idTexture, const Vector2ui& dimensions )
	{
		REQUIRES( idTexture > 0 );

		destroy();

		m_idTexture = idTexture;
		m_dimensions = dimensions;
		m_originalDimensions = m_dimensions * static_cast< uint >( Application::instance().config().contentScale());
	}

	size_t Texture::getMemorySize() const
	{
		return m_dimensions.x * m_dimensions.y * 4;		// 4 bytes per pixel.
	}

	void Texture::setClampMode( ClampMode clampModeU, ClampMode clampModeV )
	{
		if(( clampModeU != ClampMode::Clamp && !isPowerOfTwo( m_dimensions.x )) || ( clampModeV != ClampMode::Clamp && !isPowerOfTwo( m_dimensions.y )))
		{
			dev_warning( this << " wants non-clamped wrapping in one or more dimensions, but is not of power-of-two dimensions in the required axes. Attempting to apply the requested wrap mode anyway." );
		}

		Renderer::instance().bindTextureId( m_idTexture );

		GLenum clampEnumU = ( clampModeU == ClampMode::Wrap ) ? GL_REPEAT : GL_CLAMP_TO_EDGE;
		GLenum clampEnumV = ( clampModeV == ClampMode::Wrap ) ? GL_REPEAT : GL_CLAMP_TO_EDGE;

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clampEnumU );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clampEnumV );
		HANDLE_GL_ERRORS();
	}

	inline GLenum getGLEnumFromFilterMode( Texture::FilterMode filterMode, bool areMipMapsEnabled )
	{
		switch( filterMode )
		{
			default:
				ASSERT( false );
			case Texture::FilterMode::Nearest:
				return GL_NEAREST;
			case Texture::FilterMode::Bilinear:
				if( areMipMapsEnabled )
				{
					return GL_LINEAR_MIPMAP_NEAREST;
				}
				else
				{
					return GL_LINEAR;
				}
			case Texture::FilterMode::Trilinear:
				if( areMipMapsEnabled )
				{
					return GL_LINEAR_MIPMAP_LINEAR;
				}
				else
				{
					return GL_LINEAR;
				}
		}
	}

	void Texture::filterMode( FilterMode filterMode_ )
	{
        Renderer::instance().bindTextureId( m_idTexture );

		GLenum enumFilter = getGLEnumFromFilterMode( filterMode_, m_haveMipMaps );

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, enumFilter );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterMode_ == FilterMode::Nearest ? GL_NEAREST : GL_LINEAR );
		HANDLE_GL_ERRORS();
	}

	const std::vector< Color >& Texture::getLoadedTexels() const
	{
		// Will be empty if not retained.
		return m_retainedTexelData;
	}

#if !GL_ES_VERSION_2_0
	std::vector< unsigned char > Texture::readTexels() const
	{
		ASSERT( m_idTexture != 0 );
		Renderer::instance().bindTextureId( m_idTexture );

		GLuint fbo;
		glGenFramebuffers( 1, &fbo );
		glBindFramebuffer( GL_FRAMEBUFFER, fbo );
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_idTexture, 0 );

		std::vector< unsigned char > texels( 4 * m_dimensions.x * m_dimensions.y );
		glReadPixels( 0, 0, m_dimensions.x, m_dimensions.y, GL_RGBA, GL_UNSIGNED_BYTE, texels.data() );

		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
		glDeleteFramebuffers( 1, &fbo );

		return texels;
	}

	void Texture::forEachMipmapLevel( MipmapFunction&& fn ) const
	{
		GLint nMipmapLevels = 0;
		glGetTexParameteriv( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &nMipmapLevels );
		ASSERT( nMipmapLevels > 0 );

		for( GLint iLevel = 0; iLevel < nMipmapLevels; ++iLevel )
		{
			GLint levelWidth = 0, levelHeight = 0;
			glGetTexLevelParameteriv( GL_TEXTURE_2D, iLevel, GL_TEXTURE_WIDTH, &levelWidth );
			glGetTexLevelParameteriv( GL_TEXTURE_2D, iLevel, GL_TEXTURE_HEIGHT, &levelHeight );

			if( levelWidth == 0 || levelWidth == GL_INVALID_VALUE ||
			   levelHeight == 0 || levelHeight == GL_INVALID_VALUE )
			{
				// We've gone past the valid mipmap level range for this texture.
				break;
			}

			// Get the pixels for this level.
			//
			std::vector< unsigned char > pixelBytes( levelWidth * levelHeight * 4 );
			glGetTexImage( GL_TEXTURE_2D, iLevel, GL_RGBA, GL_UNSIGNED_BYTE, pixelBytes.data() );

			bool changed = fn( iLevel, levelWidth, levelHeight, pixelBytes );

			if( changed )
			{
				glTexImage2D( GL_TEXTURE_2D, iLevel, GL_RGBA, levelWidth, levelHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelBytes.data() );
			}
		}
	}
#endif

	////////////////////////////////////////////////////////////////////

	FRESH_DEFINE_CLASS( TextureLoader )
	DEFINE_VAR( TextureLoader, Texture::ClampMode, m_clampModeU );
	DEFINE_VAR( TextureLoader, Texture::ClampMode, m_clampModeV );
	DEFINE_VAR( TextureLoader, Texture::FilterMode, m_filterMode );
	DEFINE_VAR( TextureLoader, bool, m_doMipMap );
	DEFINE_VAR( TextureLoader, bool, m_retainTexelData );
	DEFINE_VAR( TextureLoader, Color, m_colorKey );
	DEFINE_VAR( TextureLoader, Vector2ui, m_originalDimensions );
	DEFINE_VAR( TextureLoader, unsigned int, m_alphaBorderFlags );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( TextureLoader )

	TextureLoader::TextureLoader( const fr::ClassInfo& assignedClassInfo, fr::Object::NameRef objectName )
	:	Super( assignedClassInfo, objectName )
	{
		m_assetClass = getClass( "Texture" );
		doctorClass< TextureLoader >( [&]( ClassInfo& classInfo, TextureLoader& defaultObject )
										{
											DOCTOR_PROPERTY_ASSIGN( assetClass )
										} );
	}

	void TextureLoader::loadAsset( Asset::ptr asset )
	{
		REQUIRES( asset );

		Super::loadAsset( asset );

		Texture::ptr texture = dynamic_freshptr_cast< Texture::ptr >( asset );
		ASSERT( texture );

		texture->loadFromFile( getResourcePath( getCompletePath() ), m_clampModeU, m_clampModeV, m_colorKey, m_doMipMap, m_alphaBorderFlags, m_retainTexelData );

		if( !m_originalDimensions.isZero() )
		{
			texture->m_originalDimensions = m_originalDimensions;		// Legal due to friendship.
		}

		texture->filterMode( m_filterMode );
	}

}

