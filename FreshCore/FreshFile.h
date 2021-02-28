/*
 *  FreshFile.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/12/08.
 *  Copyright 2008 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_FILE_H_INCLUDED_
#define FRESH_FILE_H_INCLUDED_

#include <string>
#include <vector>
#include <ctime>
#include "FreshPath.h"

namespace fr
{
	void openURLInBrowser( const std::string& url );

	void processWorkingDirectoryRedirection();

	path getDocumentBasePath();
	path getDocumentPath( const path& documentPath );
	void explorePath( const path& path );	// Opens path in macOS Finder, Windows Explorer, etc.

	// Determines the name of the subfolder within the system Documents path that are used for this application.
	// This is not supported on all systems (particularly not sandboxed OSes).
	void documentSubfolderPath( const path& subfolder );
	const path& documentSubfolderPath();
	
	bool hasResource( const path& filenameIncludingExtension );
	path getResourcePath( const path& filenameIncludingExtension );	
	
	enum class GetFilePathPolicy
	{
		PreferDocument,
		PreferResource
	};
	
	path getFilePath( const path& relativePath, GetFilePathPolicy policy = GetFilePathPolicy::PreferDocument );
	
	std::string getFileText( const path& resourcePath );
	
	void copyFile( const path& from, const path& to );
	
	void copyToPasteboard( const std::string& string );
	std::string pasteFromPasteboard();
	
	path getTempDirectoryPath();
	path createTempFile( std::ofstream& outTempFile );
	
	std::time_t getFileLastModifiedTime( const path& filePath );
	void setFileLastModifiedTime( const path& filePath, std::time_t time );
}

#endif
