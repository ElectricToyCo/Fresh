#include "FreshFile.h"
#include "FreshEssentials.h"
#include "FreshDebug.h"
#include "FreshException.h"

#include <fstream>

namespace
{
	fr::path g_documentSubfolderPath = "Fresh";
}

namespace fr
{

	void copyFile( const path& from, const path& to )
	{
		std::ifstream  src( from.string(), std::ios::binary );
		std::ofstream  dst( to.string(),   std::ios::binary );
		
		dst << src.rdbuf();
	}
	
	bool hasResource( const path& filenameIncludingExtension )
	{
		return getResourcePath( filenameIncludingExtension ).empty() == false;
	}

	std::string getFileText( const path& fullPath )
	{
		if( !exists( fullPath ))
		{
			FRESH_THROW( FreshException, "File '" << fullPath << "' did not exist." );
		}
		
		std::ifstream sourceFile( fullPath.c_str() );
		ASSERT( !sourceFile.fail() );

		std::string outText;

		sourceFile.seekg( 0, std::ios::end );
		outText.reserve( static_cast< size_t >( sourceFile.tellg() ));
		sourceFile.seekg( 0, std::ios::beg );
					
		outText.assign(( std::istreambuf_iterator<char>( sourceFile )), std::istreambuf_iterator<char>());
	
		sourceFile.close();

		return outText;
	}
	
	path getFilePath( const path& relativePath, GetFilePathPolicy policy )
	{
		path path;
		
		const int nFileLocations = 2;	// Document or Resource.
		
		int fileLocationToTry = ( policy == GetFilePathPolicy::PreferDocument ) ? 0 : 1;
		
		for( int nFileLocationsTried = 0; nFileLocationsTried < nFileLocations; ++nFileLocationsTried )
		{
			if( fileLocationToTry == 0 )	// Document
			{
				path = getDocumentPath( relativePath );
				if( !exists( path ))
				{
					path.clear();
				}
			}
			else							// Resource
			{
				try
				{
					path = getResourcePath( relativePath );
				}
				catch( ... )
				{}	// Nevermind.
			}
			
			if( !path.empty() )
			{
				break;
			}
			
			fileLocationToTry = ( fileLocationToTry + 1 ) % nFileLocations;
		}
		
		return path;
	}

	void documentSubfolderPath( const path& subfolder )
	{
		g_documentSubfolderPath = subfolder;
	}

	const path& documentSubfolderPath()
	{
		return g_documentSubfolderPath;
	}

	path getDocumentPath( const path& documentPath )
	{
		return getDocumentBasePath() / documentPath;
	}
}
