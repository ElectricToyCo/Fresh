//
//  ImageLoader.cpp
//  Fresh
//
//  Created by Jeff Wofford on 10/10/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "ImageLoader_Common.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_HDR

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#pragma GCC diagnostic ignored "-Wstringop-overflow="
#include "../stb_image.h"
#pragma GCC diagnostic pop

namespace
{
	using namespace fr;

	// Returns true iff some non-255 alpha component was found.
	bool premultiplyAlphaRGBA( unsigned char* pixels, size_t width, size_t height )
	{
		bool foundTransparency = false;

		unsigned char* const end = pixels + width * height * 4;

		for( ; pixels < end; pixels += 4 )
		{
			real alpha = pixels[ 3 ] / 255.0f;

			if( alpha < 1.0f )
			{
				foundTransparency = true;
				pixels[ 0 ] = static_cast< unsigned char >( pixels[ 0 ] * alpha );
				pixels[ 1 ] = static_cast< unsigned char >( pixels[ 1 ] * alpha );
				pixels[ 2 ] = static_cast< unsigned char >( pixels[ 2 ] * alpha );
			}
		}

		return foundTransparency;
	}

	bool unPremultiplyAlphaRGBA( unsigned char* pixels, size_t width, size_t height )
	{
		bool foundTransparency = false;

		unsigned char* const end = pixels + width * height * 4;

		for( ; pixels < end; pixels += 4 )
		{
			const real alpha = pixels[ 3 ] / 255.0f;

			if( 0 < alpha && alpha < 1.0f )
			{
				foundTransparency = true;
				pixels[ 0 ] = static_cast< unsigned char >( pixels[ 0 ] / alpha );
				pixels[ 1 ] = static_cast< unsigned char >( pixels[ 1 ] / alpha );
				pixels[ 2 ] = static_cast< unsigned char >( pixels[ 2 ] / alpha );
			}
		}

		return foundTransparency;
	}
}

namespace fr
{
	namespace ImageLoader
	{
		void FreeImage::operator()( unsigned char* pixelData ) const
		{
			ASSERT( pixelData );
			stbi_image_free( pixelData );
		}

		///////////////////////////////////////////////////////////////

		ImagePtr loadImageRGBA( const path& filePath,
								unsigned int& outWidth,
								unsigned int& outHeight,
								bool& hasAlpha,
								bool& isPremultipliedAlpha,
								PremultiplyPolicy premultiplyPolicy )
		{
			int width, height, nChannels;
			ImagePtr pixels( stbi_load( filePath.c_str(), &width, &height, &nChannels, 4 /* force to 4 bytes per pixel */ ));

			if( !pixels )
			{
				auto failureReason = stbi_failure_reason();
				FRESH_THROW( FreshException, "FRESH ERROR: Could not load image '" << filePath <<
							"'. Loader reported reason: '" << ( failureReason ? failureReason : "(None given.)") << "'" );
			}

			hasAlpha = nChannels == 4;
			isPremultipliedAlpha = false;		// stb_image always returns un-premultiplied pixels.
			outWidth = width;
			outHeight = height;

			// Premultiply or de-premultiply alpha if requested.

			const bool wantPremultiplied = ( premultiplyPolicy == WantPremultiplied );
			if( hasAlpha && premultiplyPolicy != DontCare && ( isPremultipliedAlpha != wantPremultiplied ))
			{
				if( wantPremultiplied )
				{
					hasAlpha = premultiplyAlphaRGBA( pixels.get(), width, height );
				}
				else
				{
					hasAlpha = unPremultiplyAlphaRGBA( pixels.get(), width, height );
				}

				isPremultipliedAlpha = wantPremultiplied;
			}

			return pixels;
		}
	}
}

