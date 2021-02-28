//
//  FreshAnalytics_Apple.mm
//  Fresh
//
//  Created by Jeff Wofford on 08/11/17.
//
//

#include "FreshAnalytics.h"
#include "FreshDebug.h"
#include "FreshEssentials.h"

#if FRESH_USE_CRASHLYTICS
#	import "Crashlytics/Crashlytics.h"

namespace
{
	__unused inline std::string convert( NSString* string )
	{
		if( string )
		{
			return [string UTF8String];
		}
		else
		{
			return {};
		}
	}
	
	__unused inline NSString* convert( const std::string& string )
	{
		return [NSString stringWithUTF8String: string.c_str() ];
	}
}
#endif

namespace fr
{
	namespace analytics
	{
		void enterContext( const std::string& name )
		{
#if FRESH_USE_CRASHLYTICS
			[Answers logLevelStart: convert( name ) customAttributes: nil ];
#endif
		}
		
		void leaveContext( const std::string& name, int score, bool succeeded )
		{
#if FRESH_USE_CRASHLYTICS
			[Answers logLevelEnd: convert( name ) score: [NSNumber numberWithInt:score] success: [NSNumber numberWithBool:succeeded] customAttributes: nil];
#endif
		}
		
		void registerEvent( const std::string& type, const std::string& subjectType, const std::string& subjectName, const std::string& comment )
		{
#if FRESH_USE_CRASHLYTICS
			[Answers logCustomEventWithName:convert( type )
						   customAttributes:@{
											  @"subjectType" : convert( subjectType ),
											  @"subjectName" : convert( subjectName ),
											  @"comment" : convert( comment )
											  }];
#endif
		}
		
		void registerEvent( const std::string& type, float x, float y, float z, const std::string& subjectType, const std::string& subjectName, const std::string& comment )
		{
#if FRESH_USE_CRASHLYTICS
			[Answers logCustomEventWithName:convert( type )
						   customAttributes:@{
											  @"subjectType" : convert( subjectType ),
											  @"subjectName" : convert( subjectName ),
											  @"comment" : convert( comment ),
											  @"x" : [NSNumber numberWithFloat:x],
											  @"y" : [NSNumber numberWithFloat:y],
											  @"z" : [NSNumber numberWithFloat:z]
											  }];
#endif
		}
		
		void registerPurchase( float price, const std::string& currency, bool succeeded, const std::string& itemName, const std::string& itemType, const std::string& itemID )
		{
#if FRESH_USE_CRASHLYTICS
			[Answers logPurchaseWithPrice: [[NSDecimalNumber alloc] initWithFloat: price]
								 currency: convert( currency )
								  success: [NSNumber numberWithBool:succeeded]
								 itemName: convert( itemName )
								 itemType: convert( itemType )
								   itemId: convert( itemID )
						 customAttributes: nil];
#endif
		}
	}
}
