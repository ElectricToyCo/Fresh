//
//  FreshVersioning.cpp
//  Fresh
//
//  Created by Jeff Wofford on 7/12/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "FreshVersioning.h"
#include "FreshPath.h"
#include "FreshDebug.h"
#include "FreshTime.h"
#include "CommandProcessor.h"

namespace
{
	fr::path g_versionInfoFilePath;
	
	std::string g_versionString = "<untracked version>";
	unsigned int g_buildNumber = 0;
	std::time_t g_buildDateTime{ 0 };
	std::string g_sourceControlRevision = "<unknown revision>";
}

namespace fr
{	
	namespace version
	{
		void infoFilePath( const std::string& path )
		{
			g_versionInfoFilePath = path;
			
			if( !exists( g_versionInfoFilePath ))
			{
				con_error( "Versioning: No such file '" << g_versionInfoFilePath << "'." );
			}
			else
			{
				// Open and read the version info file.
				//
				std::ifstream infoFile;
				infoFile.open( g_versionInfoFilePath.c_str() );
				
				if( !infoFile )
				{
					con_error( "Versioning: Could not open version file '" << g_versionInfoFilePath << "'." );
				}
				else
				{
					try
					{
						// Version
						std::getline( infoFile, g_versionString );

						// Build number
						std::string line;
						std::getline( infoFile, line );
						{
							std::istringstream extractor( line );
							extractor.exceptions( std::ios::failbit | std::ios::badbit );
							extractor >> g_buildNumber;
						}
						
						// Build date+time
						std::getline( infoFile, line );
						g_buildDateTime = timeFromStandardFormat( line );
						
						// Revision
						std::getline( infoFile, g_sourceControlRevision );
					}
					catch( ... )
					{
						con_error( "Versioning: Error parsing version information." );
					}
				}
			}
		}
		
		const std::string& infoFilePath()
		{
			return g_versionInfoFilePath.string();
		}
		
		std::string info()
		{
			std::ostringstream stringBuilder;
			stringBuilder << versionString() << " (" << std::hex << std::uppercase << buildNumber() << ")";

			stringBuilder << " git-rev: " << sourceControlRevision();
#if DEV_MODE && defined( DEBUG )
			stringBuilder << " built: " << getStandardTimeDisplay( buildDateTime() );
#endif
			return stringBuilder.str();
		}
		
		std::string versionString()
		{
			return g_versionString;
		}
		
		unsigned int buildNumber()
		{
			return g_buildNumber;
		}
		 
		std::time_t buildDateTime()
		{
			return g_buildDateTime;
		}
		
		std::string sourceControlRevision()
		{
			return g_sourceControlRevision;
		}
	}
}

