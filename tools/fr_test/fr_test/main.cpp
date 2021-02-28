//
//  main.cpp
//  fr_test
//
//  Created by Jeff Wofford on 8/29/14.
//  Copyright (c) 2014 The Electric Toy Company. All rights reserved.
//

#include "FreshManifest.h"
#include <iostream>
#include <fstream>
#include <cassert>

using namespace std;

int main(int argc, const char * argv[])
{
	std::istream* in = &cin;
	
	std::ifstream file;
	if( argc > 1 )
	{
		file.open( argv[ 1 ] );
		if( !file )
		{
			cerr << "Unable to open file '" << argv[ 1 ] << "'.\n";
			exit( 1 );
		}
		
		in = &file;
	}
	
	assert( in );
	
	fr::Manifest manifest;
	
	try
	{
		manifest.load( *in );
	}
	catch( const std::exception& e )
	{
		cerr << "Exception while loading manifest: '" << e.what() << "'.\n";
		exit( 2 );
	}
	
	manifest.eachDirective( [&]( const fr::Manifest::Directive& directive )
					  {
						  switch( directive.kind() )
						  {
							  case fr::Manifest::Directive::Kind::Const:
							  {
								  std::cout << "const " << directive.name() << " ";
								  const auto& thing = *directive.as< fr::Manifest::Const >();
								  std::cout << thing.type << " = " << thing.value;
								  break;
							  }
							  
							  case fr::Manifest::Directive::Kind::Object:
							  {
								  std::cout << "object " << directive.name() << " is a ";
								  const auto& thing = *directive.as< fr::Manifest::Object >();
								  std::cout << thing.className << " with properties:\n";
								  
								  for( const auto& property : *thing.map )
								  {
									  std::cout << "    " << property.first << " = (a value)\n";
								  }
								  
								  break;
							  }
							  
							  case fr::Manifest::Directive::Kind::Class:
							  {
								  std::cout << "class " << directive.name() << " extends ";
								  const auto& thing = *directive.as< fr::Manifest::Class >();
								  std::cout << thing.baseClassNames.front() << " (possibly among others) with properties:\n";
								  
								  for( const auto& property : *thing.map )
								  {
									  std::cout << "    " << property.first << " = (a value)\n";
								  }
								  
								  break;
							  }
						  }
						  
						  std::cout << std::endl;
					  } );

	std::cout << "Saved manifest: ---------------------------- " << std::endl;
	manifest.save( std::cout );
	std::cout << "Saved manifest. ---------------------------- " << std::endl;

    return 0;
}

