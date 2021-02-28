/*
 *  CoreCommands.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/29/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#include "CoreCommands.h"
#include "CommandProcessor.h"
#include "ObjectStreamFormatter.h"
#include "Assets.h"

#ifdef FRESH_PROFILER_ENABLED
#	include "Profiler.h"
#	include "FreshFile.h"
#endif


namespace fr
{

	CoreCommands::CoreCommands( CommandProcessor& processor )
	{
		// Create the classes command.
		//
		{
			auto caller = make_caller< void, const std::string& >( std::bind( &CoreCommands::listClasses, this, std::placeholders::_1 ) );
			auto command = processor.registerCommand( this, "classes", "lists classes matching name filter", std::move( caller ) );
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "class-filter", true, "the name, or part of the name, of the desired class(es)" } ));
		}
		// Create the packages command.
		//
		{
			auto caller = make_caller< void, PackageNameRef >( std::bind( &CoreCommands::listPackages, this, std::placeholders::_1 ) );
			auto command = processor.registerCommand( this, "packages", "lists packages matching name filter", std::move( caller ) );
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "package-filter", true, "the name, or a regex of the name, of the desired package(es)" } ));
		}
		
		// Create the searchpackages command.
		//
		{
			auto caller = make_caller< void >( std::bind( &CoreCommands::searchPackages, this ) );
			auto command = processor.registerCommand( this, "searchpackages", "lists all search packages in order", std::move( caller ) );
		}

		// Create the package command.
		//
		{
			auto caller = make_caller< void, PackageNameRef >( std::bind( &CoreCommands::packageContents, this, std::placeholders::_1 ) );
			auto command = processor.registerCommand( this, "package", "lists all objects belonging to packages that match the name filter", std::move( caller ) );
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "package-filter", true, "the name, or a regex of the name, of the desired packag(es)" } ));
		}
		
		// Create the objpkg command.
		//
		{
			auto caller = make_caller< void, const ObjectId& >( std::bind( &CoreCommands::packagesFor, this, std::placeholders::_1 ) );
			auto command = processor.registerCommand( this, "objpkg", "Identifies the package(s) referencing each matching object", std::move( caller ) );
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "object-id", false, "the id, or a regex of the id, of the desired object(s)" } ));
		}
		
		// Create the list command.
		//
		{
			auto caller = make_caller< void, const ObjectId& >( std::bind( &CoreCommands::list, this, std::placeholders::_1 ) );
			auto command = processor.registerCommand( this, "list", "lists matching objects", std::move( caller ) );
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "object-id", false, "the id, or a regex of the id, of the desired object(s)" } ));
		}
		
		// Create the examine command.
		//
		{
			auto caller = make_caller< void, const ObjectId& >( std::bind( &CoreCommands::examine, this, std::placeholders::_1, false ) );
			auto command = processor.registerCommand( this, "examine", "show properties of one or more objects", std::move( caller ));
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "object-id", false, "the id, or part of the id, of the desired object(s)" } ));
		}
		
		// Create the xml command.
		//
		{
			auto caller = make_caller< void, const ObjectId& >( std::bind( &CoreCommands::examineAsXml, this, std::placeholders::_1, false ) );
			auto command = processor.registerCommand( this, "xml", "display objects in full XML format", std::move( caller ) );
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "object-id", false, "the id, or part of the id, of the desired object(s)" } ));
		}
		
		// Create the props command.
		//
		{
			auto caller = make_caller< void, const ObjectId& >( std::bind( &CoreCommands::examine, this, std::placeholders::_1, true ) );
			auto command = processor.registerCommand( this, "props", "display objects showing all properties", std::move( caller ) );
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "object-id", false, "the id, or part of the id, of the desired object(s)" } ));
		}
		
		// Create the get command.
		//
		{
			auto caller = make_caller< void, const ObjectId&, const std::string& >( std::bind( &CoreCommands::get, this, std::placeholders::_1, std::placeholders::_2 ));
			auto command = processor.registerCommand( this, "get", "displays the value of a certain property belonging to one or more objects", std::move( caller ));
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "object-id", false, "the id, or part of the id, of the desired object(s)" } ));
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "prop-name", false, "the name of the property to display" } ));
		}
		
		// Create the set1st command.
		//
		{
			auto caller = stream_function< void( const ObjectId&, const std::string&, const std::string& ) > ( std::bind( &CoreCommands::set, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, true ) );
			auto command = processor.registerCommand( this, "set", "sets the value of a certain property belonging to an object", std::move( caller ) );
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "object-id", false, "the id, or part of the id, of the desired object(s)" } ));
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "prop-name", false, "the name of the property to assign" } ));
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "prop-value", false, "the value to assign to the property" } ));
		}
		
		// Create the setall command.
		//
		{
			auto caller = stream_function< void( const ObjectId&, const std::string&, const std::string& ) >( std::bind( &CoreCommands::set, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, false ) );
			auto command = processor.registerCommand( this, "setall", "sets the value of a certain property belonging to one or more matching objects", std::move( caller ) );
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "object-id", false, "the id, or part of the id, of the desired object(s)" } ));
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "prop-name", false, "the name of the property to assign" } ));
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "prop-value", false, "the value to assign to the property" } ));
		}
		
		// Create the setd command.
		//
		{
			auto caller = stream_function< void( ClassNameRef, const std::string&, const std::string& ) >( std::bind( &CoreCommands::setDefault, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 ) );
			auto command = processor.registerCommand( this, "setd", "sets the default value of a certain property belonging to a class and all instances of that class", std::move( caller ) );
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "class-name", false, "the name of the class to be written" } ));
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "prop-name", false, "the name of the property to assign" } ));
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "prop-value", false, "the value to assign to the property" } ));
		}

		// Create the call command.
		//
		{
			auto caller = stream_function< void( const ObjectId&, const std::string&, const gobbling_string& ) >( std::bind( &CoreCommands::call, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 ) );
			auto command = processor.registerCommand( this, "call", "Calls a method on matching objects", std::move( caller ) );
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "object-id-filter", false, "the object id, or part of the id, of the desired object(s)" } ));
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "mutator-name", false, "the name of the mutator to invoke" } ));
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "additional-args", true, "additional arguments to pass into the invocation" } ));
		}
		
		// Create the methods command.
		//
		{
			auto caller = make_caller< void, ClassInfo::NameRef >( std::bind( &CoreCommands::methods, this, std::placeholders::_1 ) );
			auto command = processor.registerCommand( this, "methods", "lists all methods for the indicated class", std::move( caller ) );
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "class-name", false, "a class to list methods for" } ));
		}
		
		// Create the throw command.
		//
		{
			auto caller = stream_function< void( const gobbling_string& ) >( std::bind( &CoreCommands::simulateThrow, this, std::placeholders::_1 ) );
			auto command = processor.registerCommand( this, "throw", "throws an exception failure (for testing exception handling)", std::move( caller ) );
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "message", true, "an optional failure message" } ));
		}

		// Create the gpf command.
		//
		{
			auto caller = stream_function< void() >( std::bind( &CoreCommands::simulateGeneralProtectionFault, this ) );
			processor.registerCommand( this, "gpf", "simulates a general protection fault", std::move( caller ) );
		}

#if DEV_MODE && FRESH_BREAKPOINTS_ENABLED
		// Create the break command.
		//
		{
			auto caller = stream_function< void( const std::string&, const ObjectId& ) >( std::bind( &CoreCommands::setBreakpoint, this, std::placeholders::_1, std::placeholders::_2 ) );
			auto command = processor.registerCommand( this, "break", "sets an internal breakpoint associated with <tag> that will halt when matching objects.", std::move( caller ) );
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "tag", false, "the breakpoint tag" } ));
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "object-id-filter", false, "the object id, or part of the id, of the desired object(s)" } ));
		}
		// Create the breakclear command.
		//
		{
			auto caller = stream_function< void( const std::string& ) >( std::bind( &CoreCommands::clearBreakpoint, this, std::placeholders::_1 ) );
			auto command = processor.registerCommand( this, "breakclear", "clears the internal breakpoint associated with <tag>.", std::move( caller ) );
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "tag", false, "the breakpoint tag" } ));
		}
		// Create the breaklist command.
		//
		{
			auto caller = stream_function< void( const std::string& ) >( std::bind( &CoreCommands::listBreakpoints, this, std::placeholders::_1 ) );
			auto command = processor.registerCommand( this, "breaklist", "lists internal breakpoints.", std::move( caller ) );
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "tag", true, "the name or substring of the breakpoint tag" } ));
		}
		// Create the breaktags command.
		//
		{
			auto caller = stream_function< void( const std::string& ) >( std::bind( &CoreCommands::listBreakpointTags, this, std::placeholders::_1 ) );
			auto command = processor.registerCommand( this, "breaktags", "lists available breakpoint tags.", std::move( caller ) );
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "tag", true, "the name or substring of the breakpoint tag" } ));
		}
		
#endif
		
#ifdef FRESH_PROFILER_ENABLED
		// Create the dumpprofile command.
		//
		{
			auto caller = stream_function< void() >( std::bind( &CoreCommands::dumpProfile, this ) );
			processor.registerCommand( this, "dumpprofile", "prints profiling information", std::move( caller ) );
		}
		// Create the clearprofile command.
		//
		{
			auto caller = stream_function< void() >( std::bind( &CoreCommands::clearProfile, this ) );
			processor.registerCommand( this, "clearprofile", "restarts profile data gathering", std::move( caller ) );
		}
#endif
	}
	
	CoreCommands::~CoreCommands()
	{
		if( CommandProcessor::doesExist() )
		{
			CommandProcessor::instance().unregisterAllCommandsForHost( this );
		}
	}

	void CoreCommands::listClasses( const std::string& className ) const
	{
		trace( "Classes:" );
		
		std::vector< ClassName > classNames;
		
		forEachClass( [&]( const ClassInfo& classInfo )
					 {
						 if( std::string( classInfo.className() ).find( className ) != std::string::npos )
						 {
							 classNames.push_back( classInfo.className() );
						 }
					 } );

		std::sort( classNames.begin(), classNames.end() );
		
		for( const auto& thisClass : classNames )
		{
			trace( "\t" << thisClass );
		}
	}
	
	void CoreCommands::listPackages( PackageNameRef packageNameSubstring ) const
	{
		const Package& rootPackage = getRootPackage();
		
		trace( "Packages: (contained objects/size; * - denotes retained)" );
		size_t totalObjects = 0;
		rootPackage.forEachMemberOfType< Package >( [&totalObjects, &packageNameSubstring] ( const Package::cptr& package )
								  {
									  if( package->matchesFilters( "", packageNameSubstring ))
									  {
										  const size_t size = package->size();
										  const size_t nulls = package->numNullMembers();
										  const size_t containedObjects = size - nulls;
										  totalObjects += containedObjects;
										  
										  trace( "    " << std::setw( 20 ) << std::left << package->name() << "(" << containedObjects << "/" << size << ( package->areMembersRetained() ? "*" : "") << ")" );
									  }
								  } );
		trace( "END Packages (" << totalObjects << " total objects)" );
	}
	
	void CoreCommands::searchPackages() const
	{
		trace( "Search Packages in order: (contained objects; * - denotes retained)" );
		size_t totalObjects = 0;
		forEachSearchPackage( [&totalObjects] ( const Package& package ) -> bool
							 {
								 const size_t containedObjects = package.size();
								 totalObjects += containedObjects;
								 
								 trace( "    " << std::setw( 20 ) << std::left << package.name() << "(" << containedObjects << ( package.areMembersRetained() ? "*" : "") << ")" );
								 
								 return false;	// keep traversing
							 } );
		trace( "END Search Packages (" << totalObjects << " total objects)" );
	}

	void CoreCommands::packageContents( PackageNameRef packageNameSubstring ) const
	{
		const Package& rootPackage = getRootPackage();

		trace( "Package Contents" );
		size_t totalObjects = 0;
		rootPackage.forEachMemberOfType< Package >( [&totalObjects, &packageNameSubstring] ( const Package::cptr& package )
												   {
													   if( package->matchesFilters( "", packageNameSubstring ))
													   {
														   const size_t containedObjects = package->size();
														   totalObjects += containedObjects;
														   
														   trace( "    " << std::setw( 20 ) << std::left << package->name() << "(" << containedObjects << ( package->areMembersRetained() ? "*" : "") << ")" );
														   
														   package->forEachMember( [] ( const Object::cptr& member )
																				   {
																					   trace( "        " << member );
																				   } );
													   }
												   } );
		trace( "END Package Contents (" << totalObjects << " total objects)" );
	}
	
	void CoreCommands::packagesFor( const ObjectId& objectId ) const
	{
		std::vector< Object::ptr > objects = getFilteredObjects( objectId, true /* list all rooted packages, not just search packages */ );
		
		const Package& rootPackage = getRootPackage();
		
		for( auto object : objects )
		{
			trace( object );
		
			size_t nReferringPackages = 0;
			rootPackage.forEachMemberOfType< Package >( [&] ( const Package::cptr& package )
											{
												if( package->has( object ))
												{
													trace( "    " << package->name() );
													++nReferringPackages;
												}
											} );
			
			if( nReferringPackages == 0 )
			{
				trace( "    (none)" );
			}
		}
	}

	void CoreCommands::list( const ObjectId& objectId ) const
	{
		// If `null` filter specified, grab all objects.
		const ObjectId filter = objectId.isNull() ? ObjectId{ "Object", "" } : objectId;
		
		std::ostringstream stream;
		Stringifier consoleStringifier( stream );
		ObjectStreamFormatter formatter( consoleStringifier );

		trace( "Listing objects matching filter >" << filter << "<." );
		
		listToFormatter( filter, formatter );

		trace( stream.str() );
	}

	void CoreCommands::examine( const ObjectId& objectId, bool showAllProps ) const
	{
		std::ostringstream stream;
		Stringifier consoleStringifier( stream );
		ObjectStreamFormatterManifest formatter( consoleStringifier );
		
		listToFormatter( objectId, formatter, showAllProps );
		
		trace( stream.str() );
	}
	
	void CoreCommands::examineAsXml( const ObjectId& objectId, bool showAllProps ) const
	{
		std::ostringstream stream;
		Stringifier consoleStringifier( stream );
		ObjectStreamFormatterXml formatter( consoleStringifier );
		
		listToFormatter( objectId, formatter, showAllProps );
		
		trace( stream.str() );
	}
	
	void CoreCommands::listToFormatter( const ObjectId& objectId, ObjectStreamFormatter& formatter, bool showAllProps ) const
	{
		std::vector< Object::ptr > objects = getFilteredObjects( objectId, true /* list all rooted packages, not just search packages */ );
		
		for( auto object : objects )
		{
			object->serialize( formatter, showAllProps );
		}
		formatter.getStringifier() << "Count: " << objects.size() << "\n";
	}
	
	void CoreCommands::get( const ObjectId& objectId, const std::string& propName ) const
	{
		std::vector< Object::ptr > objects = getFilteredObjects( objectId, true );

		for( const auto& object : objects )
		{
			const ClassInfo& objectClass = object->classInfo();
			
			const PropertyAbstract* property = objectClass.getPropertyByName( propName );
			
			if( property )
			{
				std::stringstream stream;
				Stringifier stringifier( stream );
				ObjectStreamFormatterManifest stringFormatter( stringifier );
				
				property->saveToFormatter( stringFormatter, object.get() );
				trace( object->toString() << "." << stream.str() );
			}
			else
			{
				trace( object->toString() << "." << propName << " does not exist." );
			}
		}		
	}
	
	void CoreCommands::set( const ObjectId& objectId, const std::string& propName, const std::string& propValue, bool firstObjectOnly ) const
	{
		std::vector< Object::ptr > objects = getFilteredObjects( objectId, true );
		
		trace( objects.size() << " object" << ( objects.size() == 1 ? "" : "s" )<< " found:" );
		
		for( const auto& object : objects )
		{
			bool succeeded = setObjectProperty( object, propName, propValue );
			
			if( succeeded && firstObjectOnly )
			{
				break;
			}
		}		
	}
	
	void CoreCommands::setDefault( ClassNameRef className, const std::string& propName, const std::string& propValue ) const
	{
		auto classInfo = getClass( className );
		if( classInfo )
		{
			// Get the default object for this class.
			//
			if( auto defaultObject = classInfo->defaultObject())
			{
				bool succeeded = defaultObject->setPropertyValue( propName, propValue );
				if( succeeded )
				{
					// Go ahead and set all instances of this class, too.
					//
					set( ObjectId{ className }, propName, propValue, false );
				}
				else
				{
					trace( "Default object for class '" << className << "' has no property " << propName << " or could not assign it to '" << propValue << "'." );
				}
			}
			else
			{
				trace( "Class '" << className << "' has no default object." );
			}
		}
		else
		{
			trace( "Unrecognized class '" << className << "'." );
		}
	}
	
	void CoreCommands::call( const ObjectId& objectId, const std::string& methodName, const gobbling_string& params ) const
	{
		if( objectId.isNull() )
		{
			trace( "Null object ID matches no objects." );
			return;
		}
		
		std::vector< Object::ptr > objects = getFilteredObjects( objectId, true );
		
		// Convert newlines to space in params.
		//
		std::string fixedParams( params );
		std::replace( fixedParams.begin(), fixedParams.end(), '\n', ' ' );
		
		for( auto object : objects )
		{
			trace( "Calling " << object << "." << methodName << "(" << fixedParams << ")" );
			
			std::istringstream paramStream( fixedParams );
			paramStream >> std::boolalpha;
			std::string result = object->call( methodName, paramStream );
			if( result.empty() == false )
			{
				trace( "Result: " << result );
			}
		}
	}
	
	bool CoreCommands::setObjectProperty( Object::ptr object, const std::string& propName, const std::string& propValue ) const
	{
		return object->setPropertyValue( propName, propValue );
	}

	void CoreCommands::methods( ClassInfo::NameRef className ) const
	{
		if( isClass( className ))
		{
			const ClassInfo& classInfo = *getClass( className );
			trace( "Methods for class " << className << ":" );
			classInfo.forEachMethod( []( const std::string& name, const StreamedMethodAbstract& method )
			{
				trace( "\t" << name );
			} );
		}
		else
		{
			trace( "Unrecognized class '" << className << "'." );
		}
	}

	void CoreCommands::setBreakpoint( const std::string& tag, const ObjectId& objectId )
	{
#if DEV_MODE && FRESH_BREAKPOINTS_ENABLED
		trace( "Setting debug breakpoint '" << tag << "' for object id " << objectId );
		Breakpoint::set( tag, objectId.className(), objectId.objectName() );
#endif
	}
	
	void CoreCommands::clearBreakpoint( const std::string& tag )
	{
#if DEV_MODE && FRESH_BREAKPOINTS_ENABLED
		trace( "Clearing debug breakpoint for tag '" << tag << "'." );
		Breakpoint::clear( tag );
#endif
	}
	
	void CoreCommands::listBreakpoints( const std::string& tagFilter )
	{
#if DEV_MODE && FRESH_BREAKPOINTS_ENABLED
		trace( "Breakpoints:" );
		Breakpoint::each( [&]( const std::string& tag, const std::string& classNameFilter, const std::string& objectNameFilter )
							   {
								   if( tag.find( tagFilter ) != std::string::npos )
								   {
									   trace( "\t" << tag << ": " << ObjectId( classNameFilter, objectNameFilter ));
								   }
							   } );
#endif
	}

	void CoreCommands::listBreakpointTags( const std::string& tagFilter )
	{
#if DEV_MODE && FRESH_BREAKPOINTS_ENABLED
		trace( "Available breakpoint tags:" );
		Breakpoint::each( [&]( const std::string& tag, const std::string& classNameFilter, const std::string& objectNameFilter )
							   {
								   if( tag.find( tagFilter ) != std::string::npos )
								   {
									   trace( "\t" << tag );
								   }
							   } );
#endif
	}
	
	void CoreCommands::simulateThrow( const gobbling_string& message )
	{
		m_desiredSimulatedThrowMessage = std::string( "SIMULATED: " ) + message;
	}
	
	std::string CoreCommands::desiredSimulatedThrowMessage()
	{
		auto copy( std::move( m_desiredSimulatedThrowMessage ));
		m_desiredSimulatedThrowMessage.clear();
		return copy;
	}
	
	void CoreCommands::simulateGeneralProtectionFault()
	{
		m_wantsSimulatedGPF = true;
	}
	
	bool CoreCommands::wantsSimulatedGPF()
	{
		auto result = m_wantsSimulatedGPF;
		m_wantsSimulatedGPF = false;
		return result;
	}
	
#ifdef FRESH_PROFILER_ENABLED
	void CoreCommands::dumpProfile() const
	{
		std::ofstream dump( getDocumentPath( "profile.html" ).string() );
		Profiler::instance().dumpAsText( dump );
		trace( "Dumped." );
	}

	void CoreCommands::clearProfile() const
	{
		Profiler::instance().clear();
		trace( "Cleared." );
	}
#endif
}

