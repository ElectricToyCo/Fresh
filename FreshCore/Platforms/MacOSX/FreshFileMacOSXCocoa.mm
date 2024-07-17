
/*
 *  FreshFileMacOSXCocoa.mm
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/12/08.
 *  Copyright 2008 jeffwofford.com. All rights reserved.
 *
 */

#include "FreshFile.h"
#include "FreshPath.h"
#include "FreshDebug.h"
#include "FreshException.h"
#include "CommandProcessor.h"
#include <Cocoa/Cocoa.h>
#include <fstream>
#include <cstdlib>
#include <utime.h>
#include <sys/stat.h>

namespace fr
{
	inline std::string castString( NSString* string )
	{
		return string != nil ? [string UTF8String] : std::string{};
	}
	
	__unused inline NSString* castString( const std::string& string )
	{
		return [NSString stringWithUTF8String: string.c_str() ];
	}

	void processWorkingDirectoryRedirection()
	{
		@autoreleasepool
		{
			chdir([[[NSBundle mainBundle] resourcePath] cStringUsingEncoding:NSUTF8StringEncoding]);
		}
	}

	void openURLInBrowser( const std::string& url )
	{
		NSString* nsString = [NSString stringWithUTF8String: url.c_str()];
		
		[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:nsString]];
	}

	path getDocumentBasePath()
	{
		NSString *documentsDirectory = nil;
		NSArray *paths = NSSearchPathForDirectoriesInDomains( NSApplicationSupportDirectory, NSUserDomainMask, YES );
		if ([paths count] > 0)  
		{
			documentsDirectory = [paths objectAtIndex:0];
		}
		
		std::string result = [documentsDirectory UTF8String];

		path basePath( result );
		
		basePath /= documentSubfolderPath();

		if( !exists( basePath ))
		{
            if( !create_directories( basePath ))
            {
                release_trace( "Unable to create document base path " << basePath );
            }
		}
		
		return basePath;
	}
	
	void explorePath( const path& path )
	{
        if( !exists( path ))
        {
            if( !create_directories( path ))
            {
                release_trace( "Unable to create explored path " << path );
            }
        }
		[[NSWorkspace sharedWorkspace] openFile:castString( path.string() )];
	}

	path getResourcePath( const char* szFilename, const char* szExtension )
	{
		std::string result;
		
		@autoreleasepool
		{
			REQUIRES( szFilename != nullptr && std::strlen( szFilename ) > 0 );
			REQUIRES( szExtension != nullptr );

			std::string path( szFilename );
			auto pos = path.rfind( '/' );
			
			NSString* dirString = nil;
			NSString* fileNameString = nil;
			
			if( pos != std::string::npos )
			{
				dirString = [NSString stringWithUTF8String: path.substr( 0, pos ).c_str()];		
				fileNameString = [NSString stringWithUTF8String: path.substr( pos + 1 ).c_str() ];
			}
			else
			{
				fileNameString = [NSString stringWithUTF8String: szFilename ];
			}
			
			// Find the actual file name from the bundle.
			//
			NSString* extensionString = [NSString stringWithUTF8String: szExtension ];
			
			NSString* fileString = [[NSBundle mainBundle] pathForResource:fileNameString ofType:extensionString inDirectory: dirString];	
			
			if( fileString == nil )
			{
				FRESH_THROW( FreshException, "Failed to find resource '" << szFilename << ( szExtension ? "." : "" ) << szExtension << "'." );
			}
			
			result = [fileString UTF8String];
		}
		
		return result;
	}

	path getResourcePath( const path& filenameIncludingExtension )
	{
		return getResourcePath( (filenameIncludingExtension.parent_path() / filenameIncludingExtension.stem() ).c_str(),
								strip_leading_dot( filenameIncludingExtension.extension() ).c_str() );
	}

	void copyToPasteboard( const std::string& string )
	{
		NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];

		// Mac OS X 10.5 version.
		//
		NSArray* types = [NSArray arrayWithObjects: NSStringPboardType, nil]; 
		[pasteboard declareTypes:types owner:nil];
		
		// Mac OS X 10.6 version.
		//
	//	[pasteboard clearContents];
		
		[pasteboard setString:[NSString stringWithUTF8String: string.c_str()] forType:NSStringPboardType];
	}

	std::string pasteFromPasteboard()
	{
		NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
		
		// Mac OS X 10.5 version.
		//
		NSArray* pasteTypes = [NSArray arrayWithObjects: NSStringPboardType, nil];
		NSString* bestType = [pasteboard availableTypeFromArray:pasteTypes]; 
		if( bestType != nil )
		{
			NSString* strPasted = [pasteboard stringForType:NSStringPboardType];
			
			if( strPasted != nil && [strPasted length] > 0 )
			{
				return std::string( [strPasted UTF8String] );
			}
		}
		return "";
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
