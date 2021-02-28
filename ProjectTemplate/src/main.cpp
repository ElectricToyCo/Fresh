//
//  main.cpp
//
//  Created by Jeff Wofford on 2014/03/04.
//  Copyright 2014 jeffwofford.com. All rights reserved.
//

#include "ApplicationStaged.h"
#include "FreshFile.h"
using namespace fr;

int main( int argc, char* argv[] )
{
	ApplicationStaged app( "assets/config.fresh" );
	int retVal = app.runMainLoop( argc, argv );
	
    return retVal;
}
