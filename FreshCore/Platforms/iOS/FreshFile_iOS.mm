/*
 *  FreshFile_iOS.mm
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/12/08.
 *  Copyright 2008 jeffwofford.com. All rights reserved.
 *
 */

#include "FreshFile.h"
#include "FreshDebug.h"
#include "FreshException.h"
#include <map>
#include <fstream>
#import <UIKit/UIKit.h>

namespace fr
{

	void processWorkingDirectoryRedirection()
	{
		@autoreleasepool
		{
			::chdir([[[NSBundle mainBundle] resourcePath] cStringUsingEncoding:NSUTF8StringEncoding]);
		}
	}

	void openURLInBrowser( const std::string& url )
	{
		NSString* nsString = [NSString stringWithUTF8String: url.c_str()];
		
		[[UIApplication sharedApplication] openURL:[NSURL URLWithString: nsString] options:@{} completionHandler:nil];
	}

	path getDocumentBasePath()
	{
		NSString *documentsDirectory = nil;
		NSArray *paths = NSSearchPathForDirectoriesInDomains( NSDocumentDirectory, NSUserDomainMask, YES );
		if ([paths count] > 0)  
		{
			documentsDirectory = [paths objectAtIndex:0];
		}
		
		std::string result = [documentsDirectory UTF8String];
		
		return result;
	}
	
	void explorePath( const path& path )
	{
//		debug_trace( "Platform ignoring `explorePath( " << path << " )`." );
		// Ignored.
	}

	path getResourcePath( const char* szFilename, const char* szExtension )
	{
		REQUIRES( szFilename != 0 && strlen( szFilename ) > 0 );
		REQUIRES( szExtension != 0 );
		
		@autoreleasepool
		{
			// Find the actual file name from the bundle.
			//
			NSString* fileNameString = [NSString stringWithUTF8String: szFilename ];
			NSString* extensionString = [NSString stringWithUTF8String: szExtension ];
			
			NSString* fileString = [[NSBundle mainBundle] pathForResource: fileNameString ofType: extensionString];
			
			if( fileString == nil )
			{
				FRESH_THROW( FreshException, "Failed to find resource '" << szFilename << "." << szExtension << "'." );
			}
			
			std::string result( [fileString UTF8String] );
			
			return result;
		}
	}

	path getResourcePath( const path& filenameIncludingExtension )
	{
		return getResourcePath( (filenameIncludingExtension.parent_path() / filenameIncludingExtension.stem() ).c_str(),
							   strip_leading_dot( filenameIncludingExtension.extension() ).c_str() );
	}
	
	void copyToPasteboard( const std::string& string )
	{	
		[UIPasteboard generalPasteboard].string = [NSString stringWithUTF8String: string.c_str()];
	}

	std::string pasteFromPasteboard()
	{
		return std::string( [[UIPasteboard generalPasteboard].string UTF8String] );
	}
		
	path getTempDirectoryPath()
	{
		@autoreleasepool
		{
			std::string result = [NSTemporaryDirectory() UTF8String];
			return result;
		}
	}
}
