//
//  FreshSocial_iOS.mm
//  Fresh
//
//  Created by Jeff Wofford on 10/29/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "FreshSocial.h"
#include "FreshDebug.h"
#include "Application.h"
#import <Social/Social.h>

extern UIWindow* g_rootWindow;

namespace
{
	using namespace fr;
	
	NSString* cocoaService( Social::Service service )
	{
		switch( service )
		{
			case Social::Facebook:
				return SLServiceTypeFacebook;
			case Social::Twitter:
				return SLServiceTypeTwitter;
			default:
				return nil;
		}
	}
}

namespace fr
{
	
	class SocialImpl
	{
	public:
		
		bool isAvailable( Social::Service service ) const
		{
			NSString* cocoa = cocoaService( service );
			
			if( cocoa )
			{
				return [SLComposeViewController isAvailableForServiceType: cocoa];
			}
			else
			{
				return false;
			}
		}
		
		void proposePost( Social::Service service, const std::string& proposedText,
						 const std::vector< unsigned int >& pixels,
						 unsigned int width,
						 unsigned int height )
		{
			if( isAvailable( service ))
			{
				NSString* cocoa = cocoaService( service );
				SLComposeViewController* controller = [SLComposeViewController composeViewControllerForServiceType: cocoa];
				ASSERT( controller );
				[controller setInitialText:[NSString stringWithUTF8String: proposedText.c_str()]];
				
				// Image?
				//
				if( !pixels.empty() && width > 0 && height > 0 )
				{
					CGDataProviderRef provider = CGDataProviderCreateWithData( NULL, pixels.data(), width * height, NULL );
					
					const int bitsPerComponent = 8;
					const int bitsPerPixel = 32;
					const int bytesPerRow = 4 * width;
					
					CGColorSpaceRef colorSpaceRef = CGColorSpaceCreateDeviceRGB();
					CGBitmapInfo bitmapInfo = kCGBitmapByteOrderDefault;
					CGColorRenderingIntent renderingIntent = kCGRenderingIntentDefault;

					
					CGImageRef imageRef = CGImageCreate( width,
														height,
														bitsPerComponent,
														bitsPerPixel,
														bytesPerRow,
														colorSpaceRef,
														bitmapInfo,
														provider,
														NULL,
														NO,
														renderingIntent );
					
					UIImage* myImage = [ UIImage imageWithCGImage:imageRef scale:1 orientation:UIImageOrientationUp ];

					[controller addImage: myImage];
					
					CGImageRelease( imageRef );
					CGDataProviderRelease( provider );
					CGColorSpaceRelease( colorSpaceRef );
				}
				
				UIViewController* hostController = [g_rootWindow rootViewController];
				ASSERT( hostController );

				[hostController presentViewController: controller animated:YES completion:nil];
			}
		}
	};
	
	/////////////////////////////////////////////////////////////

	Social::Social()
	:	m_impl( new SocialImpl() )
	{}
	
	Social::~Social()
	{}						// Defined in the cpp file to avoid early instantiation of std::unique_ptr< (implementation) >
	
	bool Social::isAvailable( Social::Service service ) const
	{
		return m_impl->isAvailable( service );
	}
	
	void Social::proposePost( Social::Service service, const std::string& proposedText )
	{
		m_impl->proposePost( service, proposedText, std::vector< unsigned int >(), 0, 0 );
	}

	void Social::proposePost( Service service, const std::string& proposedText,
					 const std::vector< unsigned int >& imagePixels,
					 unsigned int imageWidth,
					 unsigned int imageHeight )
	{
		m_impl->proposePost( service, proposedText, imagePixels, imageWidth, imageHeight );
	}
}

