//
//  FreshAnalytics.h
//  Fresh
//
//  Created by Jeff Wofford on 8/11/17.
//  Copyright (c) 2017 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_FreshAnalytics_h
#define Fresh_FreshAnalytics_h

#include <string>

namespace fr
{
	namespace analytics
	{
		void enterContext( const std::string& name );
		void leaveContext( const std::string& name, int score, bool succeeded );
		void registerEvent( const std::string& type, const std::string& subjectType = "", const std::string& subjectName = "", const std::string& comment = "" );
		void registerEvent( const std::string& type, float x, float y, float z, const std::string& subjectType = "", const std::string& subjectName = "", const std::string& comment = "" );
		void registerPurchase( float price, const std::string& currency, bool succeeded, const std::string& itemName, const std::string& itemType, const std::string& itemID );
	}
}

#endif
