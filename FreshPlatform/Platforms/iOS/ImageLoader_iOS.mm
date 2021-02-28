//
//  ImageLoader_iOS.mm
//  Fresh
//
//  Created by Jeff Wofford on 10/10/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "../ImageLoader_Common.h"
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>

namespace fr
{
	namespace ImageLoader
	{
		void FreeImage::operator()( unsigned char* pixelData ) const
		{
			ASSERT( pixelData );
			delete[] pixelData;
		}
		
		///////////////////////////////////////////////////////////////
		
		ImagePtr loadImageRGBA( const path& filePath,
								uint& outWidth,
								uint& outHeight,
								bool& outHasAlpha,
								bool& outIsPremultipliedAlpha,
								PremultiplyPolicy premultiplyPolicy )
		{
			// Creates a Core Graphics image from an image file
			
			NSString* filename = [NSString stringWithUTF8String: filePath.c_str() ];
			UIImage* image = [UIImage imageWithContentsOfFile: filename];
			
			if( !image )
			{
				FRESH_THROW( FreshException, "FRESH ERROR: Could not load image '" << filePath << "'.")
			}
			
			CGImageRef cgImage = image.CGImage;
			
			if( !cgImage )
			{
				FRESH_THROW( FreshException, "FRESH ERROR: Loaded image '" << filePath << "' but could not get CGImage.")
			}
			
			outWidth = static_cast< uint >( CGImageGetWidth( cgImage ));
			outHeight = static_cast< uint >( CGImageGetHeight( cgImage ));
			
			const CGImageAlphaInfo alphaInfo = CGImageGetAlphaInfo( cgImage );
			switch( alphaInfo )
			{
				case kCGImageAlphaNone:
				case kCGImageAlphaNoneSkipLast:
				case kCGImageAlphaNoneSkipFirst:
					outHasAlpha = false;
					break;
				default:
					outHasAlpha = true;
					break;
			}
						
			const unsigned int nBytesPerPixel = 4;	// Force RGBA
			const size_t nPixelBytes = outWidth * outHeight * nBytesPerPixel;
			
			ImagePtr pixels( new unsigned char[ nPixelBytes ] );
			std::memset( pixels.get(), 0x00000000, nPixelBytes );
			
			//
			// We have the pixel data in cgImage, but we need to get it into pixelData so that we can pass it to GL through glTexImage2D (below).
			// First, make a cgContext to "wrap" the pixelData.
			// Then draw cgImages's contents into the cgContext--which will also draw into pixelData.
			// (After that we can dump cgContext.)
			// Then we'll be ready to send the pixelData into glTexImage2D.
			//
			
			CGBitmapInfo bitmapInfo;
			
			switch( premultiplyPolicy )
			{
				default:
				case DontCare:
					bitmapInfo = CGImageGetBitmapInfo( cgImage );
					outIsPremultipliedAlpha = bitmapInfo == kCGImageAlphaPremultipliedLast;
					break;
				case WantPremultiplied:
					bitmapInfo = kCGImageAlphaPremultipliedLast;
					outIsPremultipliedAlpha = true;
					break;
				case WantNotPremultiplied:
					bitmapInfo = kCGImageAlphaLast;
					outIsPremultipliedAlpha = false;
					break;
			}
			
			// Wrap the (blank) pixelData with a context.
			//
			CGContextRef cgContext = CGBitmapContextCreate( pixels.get(),
															   outWidth,
															   outHeight,
															   8,
															   outWidth * nBytesPerPixel,
															   CGImageGetColorSpace( cgImage ),
															   bitmapInfo );
			
			// Copy the sprite image data to the context and therefore to the pixelData.
			//
			CGContextDrawImage( cgContext, CGRectMake( 0, 0, (CGFloat) outWidth, (CGFloat) outHeight ), cgImage );
			
			// We're done with the context, so release it.
			//
			CGContextRelease( cgContext );
			
			return pixels;
		}
	}
}

