/*
 *  CommandProcessor.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/29/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#include "CommandProcessor.h"
#include "Objects.h"
#include "TelnetServer.h"

namespace fr
{
	CommandProcessor::CommandAbstract::CommandAbstract( void* host, const std::string& name, const std::string& description )
	:	m_host( host )
	,	m_name( name )
	,	m_description( description )
	{
		REQUIRES( host );
		REQUIRES( !name.empty() );
	}

	void CommandProcessor::CommandAbstract::addArgument( const Argument& argument )
	{
		m_arguments.push_back( argument );
	}

	
	void CommandProcessor::CommandAbstract::getUsageString( std::ostream& stream, bool doVerbose ) const
	{
		stream << m_name;
		
		// Print one-line version.
		//
		for( const auto& argument : m_arguments )
		{
			stream << " ";
			
			if( argument.optional )
			{
				stream << "[";
			}
			else
			{
				stream << "<";
			}

			stream << argument.name;
			
			if( argument.optional )
			{
				stream << "]";
			}
			else
			{
				stream << ">";
			}
		}
		
		stream << "\t-\t" << m_description;

		// Print verbose version.
		//
		if( doVerbose && !m_arguments.empty() )
		{
			for( const auto& argument : m_arguments )
			{
				stream << "\n\t" << argument.name << "\t-\t";
				if( argument.optional )
				{
					stream << "(opt) ";
				}
				stream << argument.description;
			}
		}
	}
	
	/////////////////////////////////////////////////////////////////////////////////////
	
	FRESH_DEFINE_CLASS( CommandProcessor )

	CommandProcessor::CommandProcessor( CreateInertObject c )
	:	Super( c )
	,	ObjectSingleton< CommandProcessor >( nullptr )
	{}
	
	CommandProcessor::CommandProcessor( const ClassInfo& assignedClassInfo, NameRef name )
	:	Super( assignedClassInfo, name )
	,	ObjectSingleton< CommandProcessor >( this )
	,	m_server( TelnetServer::create() )
	,	m_coreCommands( new CoreCommands( *this ))
	{
		// Create the help command.
		//
		{
			auto caller = stream_function< void() >( std::bind( &CommandProcessor::printHelp, this ) );
			registerCommand( this, "help", "prints this information text", std::move( caller ));
		}
		
		// Create the usage command.
		//
		{
			auto caller = stream_function< void( const std::string& ) >( std::bind( &CommandProcessor::printUsage, this, std::placeholders::_1 ) );
			auto command = registerCommand( this, "usage", "prints information about a specific command", std::move( caller ));
			
			command->addArgument( CommandAbstract::Argument( { "command", false, "the name of the command to examine" } ));
		}
	}
	
	std::shared_ptr< CommandProcessor::CommandAbstract > CommandProcessor::registerCommand( std::shared_ptr< CommandAbstract > command )
	{
		REQUIRES( command );
		
		m_commands[ command->name() ] = command;
		
		PROMISES( hasCommand( command->name() ));
		
		return command;
	}
	
	void CommandProcessor::startListenServer()
	{
		ASSERT( m_server );
		m_server->beginListening( std::bind( &CommandProcessor::onDataReceived, this, std::placeholders::_1 ), 47125, "Hello. Use 'help' for help.\n" );
		
#if DEV_MODE
		unsigned int address = getDeviceIPAddress();
		release_trace( "Telnet server started. Use address " << getStringIPAddress( address ) << ":" << m_server->portNumber() );
#endif
	}
	
	bool CommandProcessor::isListenServerStarted() const
	{
		ASSERT( m_server );
		return m_server->isListening();
	}
	
	void CommandProcessor::traceToConsole( const std::string& message )
	{
		std::string doctored;
		doctored = "\033[31m" + message + "\033[0m";
		
		ASSERT( m_server );
		m_server->broadcast( doctored );
	}
	
	bool CommandProcessor::executeCommandLine( const std::string& commandLine )
	{
		std::istringstream stream( commandLine );
		
		// Find the command in question.
		//
		std::string commandName;
		stream >> commandName;
		
		auto iter = m_commands.find( commandName );
		if( iter == m_commands.end() )
		{
			traceToConsole( createString( "Unrecognized command '" << commandName << "'.\n" ));
			return false;
		}
		
		CommandAbstract& command = *( iter->second );
		
		try
		{
			command.execute( commandLine );
		}
		catch( const std::exception& e )
		{
			trace( "Exception while executing command '" << commandLine << "': " << e.what() );
			return false;
		}
		catch( ... )
		{
			trace( "Unknown exception while executing command '" << commandLine << "'." );
			return false;
		}
		
		return true;
	}
	
	void CommandProcessor::findCompletingCommandNames( const std::string& commandNameSubstring, std::vector< std::string >& outMatchingCommands ) const
	{
		for( const auto& command : m_commands )
		{
			if( command.second->name().find( commandNameSubstring ) != std::string::npos )
			{
				outMatchingCommands.push_back( command.second->name() );
			}
		}
	}
	
	void CommandProcessor::unregisterAllCommandsForHost( void* host )
	{
		for( auto iter = m_commands.begin(); iter != m_commands.end(); )
		{
			if( iter->second->hasHost( host ))
			{
				iter = m_commands.erase( iter );
			}
			else
			{
				++iter;
			}
		}
	}
	
	bool CommandProcessor::hasCommand( const std::string& name ) const
	{
		return m_commands.find( name ) != m_commands.end();
	}
	
	void CommandProcessor::getUsageString( std::ostream& stream, const std::string& name, bool verbose ) const
	{
		auto iter = m_commands.find( name );
		REQUIRES( iter != m_commands.end() );
		
		const CommandAbstract& command = *( iter->second );
		
		command.getUsageString( stream, verbose );
	}
	
	void CommandProcessor::onDataReceived( const std::string& text )
	{
		bool didExecute = executeCommandLine( text );
		
		if( !didExecute )
		{
			// Was the text just a single word?
			//
			std::istringstream stream( text );
			std::string commandName;
			stream >> commandName;
			
			std::string additionalText;
			stream >> additionalText;
			
			if( !commandName.empty() && additionalText.empty() )
			{
				// Yes. Print options for the command.
				//
				std::vector< std::string > matchingCommands;
				findCompletingCommandNames( commandName, matchingCommands );
				
				for( const auto& possibleCommandName : matchingCommands )
				{
					traceToConsole( createString( "\t" << possibleCommandName << "\n" ));
				}
			}
		}
	}

	void CommandProcessor::printHelp()
	{
		traceToConsole( "Commands:\n" );
		
		for( auto iter : m_commands )
		{
			std::ostringstream stream;
			stream << "\t" << iter.second->name();
			
			traceToConsole( stream.str() );
			
			traceToConsole( "\n" );
		}		
	}

	void CommandProcessor::printUsage( const std::string& commandName )
	{
		auto iter = m_commands.find( commandName );
		if( iter == m_commands.end() )
		{
			traceToConsole( createString( "Command '" << commandName << "' not found.\n" ));
		}
		else
		{
			traceToConsole( "USAGE: " );
			
			std::stringstream stream;
			iter->second->getUsageString( stream, true );
			
			traceToConsole( stream.str() );
			traceToConsole( "\n" );
		}
	}
}
