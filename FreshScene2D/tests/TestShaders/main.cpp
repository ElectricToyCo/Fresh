//
//  main.cpp
//  TestShaders
//
//  Created by Jeff Wofford on 12/2/10.
//  Copyright 2010 jeffwofford.com. All rights reserved.
//

#include "ApplicationStaged.h"
#include "Package.h"
#include "Asset.h"
#include "Renderer.h"
#include "Stage.h"
#include "Texture.h"
#include "ParticleEmitter.h"
#include "Renderer.h"
#include "ShaderProgram.h"
#include "DevStatsDisplay.h"
#include "TextField.h"
#include "FreshFile.h"

using namespace fr;

int main( int argc, char* argv[] )
{
	ApplicationStaged app( "appConfig.xml" );	
	int retVal = app.runMainLoop( argc, argv );
	
    return retVal;
}
