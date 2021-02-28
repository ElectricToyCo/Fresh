//
//  FreshTest.h
//  Fresh
//
//  Created by Jeff Wofford on 11/2/12.
//
//

#ifndef Fresh_FreshTest_h
#define Fresh_FreshTest_h

#include <iostream>

#define VERIFY_BOOL_MSG( expr, message ) if( !( expr )) { dev_trace( "BOOL TEST FAILED: " << message ); return false; }
#define VERIFY_BOOL( expr ) VERIFY_BOOL_MSG( (expr), #expr )

#define VERIFY_TEST_MSG( expr, message ) if( !(expr) ) { dev_trace( "TEST FAILED: " << message ); return 1; }
#define VERIFY_TEST( expr ) VERIFY_TEST_MSG( (expr), #expr )


#endif
