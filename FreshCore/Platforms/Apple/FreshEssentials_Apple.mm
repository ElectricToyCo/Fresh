/*
 *  FreshEssentials_Apple.mm
 *  Fresh
 *
 *  Created by Jeff Wofford on 3/19/14.
 *  Copyright 2014 jeffwofford.com. All rights reserved.
 *
 */

#include "FreshEssentials.h"
#include "FreshDebug.h"

#if TARGET_OS_IPHONE
#	include <UIKit/UIKit.h>
#else
#	include <Cocoa/Cocoa.h>
#endif

namespace fr
{
	std::string getOSVersion()
	{
#if TARGET_OS_IPHONE
		return createString( [[[UIDevice currentDevice] systemName] UTF8String] << " " << [[[UIDevice currentDevice] systemVersion] UTF8String] );
#else
		id processInfo = [NSProcessInfo processInfo];
		NSString* versionString = [processInfo operatingSystemVersionString];
		return createString( "MacOSX " << [versionString UTF8String] );
#endif
	}
}
