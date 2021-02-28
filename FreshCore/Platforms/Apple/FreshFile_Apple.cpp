/*
 *  FreshTime_Apple.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 7/17/08.
 *  Copyright 2008 jeffwofford.com. All rights reserved.
 *
 */

#include "FreshFile.h"
#include "FreshDebug.h"
#include <fstream>
#include <cstdlib>
#include <utime.h>
#include <sys/stat.h>

namespace fr
{
	path createTempFile( std::ofstream& outTempFile )
	{
		// Find where to put it.
		//
		path tempPath = getTempDirectoryPath();		// Includes trailing /
		ASSERT( tempPath.empty() == false );
		tempPath /= "freshXXXXXX";
		
		// Make tempBasePath writable so that mkstemp can jack with it.
		//
		const char* szPath = tempPath.c_str();
		const size_t lenPath = tempPath.string().length();
		
		std::vector< char > tempBasePathBytes( szPath, szPath + lenPath );
		tempBasePathBytes.push_back( '\0' );		// Need null terminator though.
		
		const int fileDescriptor = ::mkstemp( tempBasePathBytes.data() );
		ASSERT( fileDescriptor != -1 );
		
		// Convert the modified path back to a string.
		//
		std::string tempBasePathString( tempBasePathBytes.begin(), tempBasePathBytes.end() - 1 );
		
		tempPath = tempBasePathString;
		
		outTempFile.open( tempPath.c_str(), std::ios_base::trunc | std::ios_base::out );
		close( fileDescriptor );
		
		return tempPath;
	}
	
	std::time_t getFileLastModifiedTime( const path& filePath )
	{
		struct stat attrib;			// create a file attribute structure
		int result = ::stat( filePath.c_str(), &attrib );		// get the attributes of afile.txt
		if( result != -1 )
		{
			return attrib.st_mtime;
		}
		else
		{
			return time_t( 0 );
		}
	}
	
	void setFileLastModifiedTime( const path& filePath, std::time_t time )
	{
		utimbuf modificationAndAccessTimes;
		modificationAndAccessTimes.actime = time;
		modificationAndAccessTimes.modtime = time;
		::utime( filePath.c_str(), &modificationAndAccessTimes );
	}
}
