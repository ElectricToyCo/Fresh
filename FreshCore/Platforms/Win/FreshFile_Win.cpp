#include "../../FreshFile.h"
#include "../../FreshEssentials.h"
#include "../../FreshDebug.h"
#include "../../FreshException.h"
#include <Shlobj.h>					// For ::SHGetFolderPath()

#include <fstream>

namespace
{
	::FILETIME stdTimeToFileTime( std::time_t t )
	{
		LONGLONG ll = Int32x32To64( t, 10000000 ) + 116444736000000000;

		::FILETIME fileTime;
		
		fileTime.dwLowDateTime = (DWORD)ll;
		fileTime.dwHighDateTime = ll >> 32;

		return fileTime;
	}

	std::time_t fileTimeToStdTime( const FILETIME& ft )
	{
		// the reverse of http://support.microsoft.com/kb/167296/en-us
		ULONGLONG ull = reinterpret_cast< const ULONGLONG& >( ft );
		ull -= 116444736000000000;
		ull /= 10000000;
		ASSERT( ull < ULONG_MAX );
		return static_cast< time_t >( ull );
	}
}

namespace fr
{
	void openURLInBrowser( const std::string& url )
	{
		ASSERT( false );
		// TODO
	}
	
	void processWorkingDirectoryRedirection()
	{
		std::ifstream redirectionFile( "redirection.txt" );
		if( redirectionFile.is_open() )
		{
			char redirectionPath[ MAX_PATH ];
			redirectionFile.getline( redirectionPath, MAX_PATH );

			ASSERT( strlen( redirectionPath ) > 0 );

			::SetCurrentDirectory( redirectionPath );

			::GetCurrentDirectory( MAX_PATH, redirectionPath );

			dev_trace( "Set working directory to " << redirectionPath );
		}
	}

	path getDocumentBasePath()
	{
		char szPath[ MAX_PATH ];
		VERIFY( ::SHGetFolderPath( NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, szPath ) == S_OK );

		path basePath( szPath );

		basePath /= documentSubfolderPath();

		if( !exists( basePath ))
		{
			create_directory( basePath );
		}

		return basePath;
	}

	path getResourcePath( const path& resourcePath )
	{
		if( exists( resourcePath ))
		{
			return resourcePath;
		}
		else
		{
			FRESH_THROW( FreshException, "Failed to find resource '" << resourcePath << "'." );
		}
	}
	
	void explorePath( const path& path )
    {
        // TODO
    }

	void copyToPasteboard( const std::string& string )
	{
		VERIFY( ::OpenClipboard( NULL ));
		VERIFY( ::EmptyClipboard());

		size_t nChars = string.size();
		// God I hate Windows.

		HGLOBAL globalMem = ::GlobalAlloc( GMEM_MOVEABLE, ( nChars + 1 ) * sizeof( string[0] ));

		LPTSTR globalBuffer = static_cast< LPTSTR >( ::GlobalLock( globalMem ));

        std::memcpy( globalBuffer, string.data(), nChars * sizeof( string[0] )); 
        globalBuffer[ nChars ] = '\0';
        
		::GlobalUnlock( globalMem );

		::SetClipboardData( CF_TEXT, globalMem );

		VERIFY( ::CloseClipboard());
	}

	std::string pasteFromPasteboard()
	{
		std::string contents;
		if( ::IsClipboardFormatAvailable( CF_TEXT ))
		{
			VERIFY( ::OpenClipboard( NULL ));

		HGLOBAL globalMem = ::GetClipboardData( CF_TEXT ); 
		if( globalMem ) 
		{ 
			LPTSTR globalBuffer = static_cast< LPTSTR >( ::GlobalLock( globalMem ));
			if( globalBuffer ) 
			{ 
				contents.assign( globalBuffer );
				::GlobalUnlock( globalMem ); 
			} 
		} 
			VERIFY( ::CloseClipboard());

		}

		return contents;
 	}

	path getTempDirectoryPath()
	{
		// TODO
		ASSERT( false );
		return "";
	}
	
	path createTempFile( std::ofstream& outTempFile )
	{
		// TODO
		ASSERT( false );
		return "";
	}

	std::time_t getFileLastModifiedTime( const path& filePath )
	{
		// Get a handle for the file.
		//
		HANDLE hFile = ::CreateFileA( filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		ASSERT( hFile != INVALID_HANDLE_VALUE );

		// Retrieve the last write time.
		//
		::FILETIME creationTime, lastAccessTime, lastWriteTime;
		::GetFileTime( hFile, &creationTime, &lastAccessTime, &lastWriteTime );

		::CloseHandle( hFile );

		// Convert to std::time_t
		//
		return fileTimeToStdTime( lastWriteTime );
	}
	
	void setFileLastModifiedTime( const path& filePath, std::time_t time )
	{
		// Get a handle for the file.
		//
		HANDLE hFile = ::CreateFileA( filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		ASSERT( hFile != INVALID_HANDLE_VALUE );

		::FILETIME lastWriteTime = stdTimeToFileTime( time );
		::SetFileTime( hFile, NULL, NULL, &lastWriteTime );

		::CloseHandle( hFile );
	}
}