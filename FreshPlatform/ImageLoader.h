//
//  ImageLoader.h
//  Fresh
//
//  Created by Jeff Wofford on 10/10/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_ImageLoader_h
#define Fresh_ImageLoader_h

#include "FreshPath.h"
#include "FreshEssentials.h"

namespace fr
{
	namespace ImageLoader
	{
		enum PremultiplyPolicy
		{
			DontCare,
			WantPremultiplied,
			WantNotPremultiplied
		};
		
		struct FreeImage
		{
			void operator()( unsigned char* pixelData ) const;
		};
		typedef std::unique_ptr< unsigned char[], FreeImage > ImagePtr;
		
		
		///////////////////////////////////////////////////////////////////////////////////
		
		ImagePtr loadImageRGBA( const path& filePath,
									  unsigned int& outWidth,
									  unsigned int& outHeight,
									  bool& outHasAlpha,
									  bool& outIsPremultipliedAlpha,
									  PremultiplyPolicy premultiplyPolicy = DontCare );
		// Returns a pointer to the pixel data in 4-byte RGBA format.
		// ImagePtr is a unique_ptr, so the data will die when the ImagePtr dies.
		// std::move() it to pass it around.
		// hasAlpha will be true if the image probably has transparent pixels.
		// This will sometimes have false positives, however: the pixels aren't necessarily scanned
		// one-by-one to determine whether they're all opaque or not.
		// If premultiplyPolicy != DontCare and hasAlpha == true, the pixels will be
		// processed to have the alpha premultiplication state you request.
		// If !hasAlpha, isPremultipliedAlpha is undefined.
		// Throws an exception if it can't open the file or read it or whatever.
	}
	
}

#endif
