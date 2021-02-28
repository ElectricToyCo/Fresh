//
//  FreshVersioning.h
//  Fresh
//
//  Created by Jeff Wofford on 7/12/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_FreshVersioning_h
#define Fresh_FreshVersioning_h

#include <string>
#include <ctime>

namespace fr
{
	namespace version
	{
		///////////////////////////////////////////////////////////////////////////////////////////////
		//
		// VERSION FILE FORMAT
		// ===================
		//
		// A text file, one datum per line:
		//
		//		version string	(format totally flexible)
		//		build number	(unsigned int)
		//		build date+time	(YYY-MM-DD HH:MM:SS)
		//		SCM revision	(format totally flexible)
		//
		// Example file:
		//		----------------------------------------
		//		MacOSX 2.0
		//		1
		//		2013-07-12 15:59:00
		//		046b5d5e04169da180752879742a24ed25a7c6b5
		//		----------------------------------------
		//
		// You should generally .gitignore (or ignore by whatever mechanism in whatever SCM you use)
		// the version text file and automatically update it either after each commit or
		// before each build.
		//
		///////////////////////////////////////////////////////////////////////////////////////////////
		
		// Configuration to indicate where the system should find the version info file.
		//
		void infoFilePath( const std::string& path );
		const std::string& infoFilePath();
		
		std::string info();			// A single string representing information about the version in a standard format.
		
		// Individual version data:
		//
		std::string versionString();			// E.g. "1.4.1 for Mac"
		unsigned int buildNumber();				// Corresponds to the number of times the product has been built, at least for public consumption.
		std::time_t buildDateTime();			// The GMT time at which this particular version was created.
		std::string sourceControlRevision();	// The revision with the project SCM program (e.g. git or Subversion) that was current when this build was made.
												// That is not to say that the build code was identical to this revision, since the programmer may have changed something while
												// creating the build. But this was the HEAD revision at the time of the build.
	}
}

#endif
