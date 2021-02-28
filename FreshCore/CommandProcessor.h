/*
 *  CommandProcessor.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/29/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_COMMAND_PROCESSOR_H_INCLUDED
#define FRESH_COMMAND_PROCESSOR_H_INCLUDED

#include "ObjectSingleton.h"
#include "Object.h"
#include "CoreCommands.h"
#include "TelnetServer.h"
#include <map>

namespace fr
{
	class CommandProcessor : public Object, public ObjectSingleton< CommandProcessor >
	{
	public:
		
		class CommandAbstract
		{
		public:
			
			struct Argument
			{
				std::string name;
				bool optional;
				std::string description;
			};
			
			CommandAbstract( void* host, const std::string& name, const std::string& description );
			// REQUIRES( !name.empty() );
			
			virtual ~CommandAbstract() {}

			bool hasHost( void* aHost ) const									{ return m_host == aHost; }
			SYNTHESIZE( std::string, name );
			SYNTHESIZE( std::string, description );
			void addArgument( const Argument& argument );
			
			virtual void execute( const std::string& commandLine ) = 0;
			// REQUIRES( hasCallback() );
			
			void getUsageString( std::ostream& stream, bool doVerbose ) const;
			
		private:
			
			void* m_host;
			std::string m_name;
			std::vector< Argument > m_arguments;
			std::string m_description;
		};
		
		template< typename StreamedFunctionCallerT >
		class Command : public CommandAbstract
		{
		public:
			Command( void* host, const std::string& name, const std::string& description, StreamedFunctionCallerT&& caller )
			:	CommandAbstract( host, name, description )
			,	m_caller( std::forward< StreamedFunctionCallerT >( caller ))
			{}
			
			Command( const Command& ) = delete;
			void operator=( const Command& ) = delete;
			
			virtual void execute( const std::string& commandLine ) override
			{
				std::istringstream stream( commandLine );
				stream.setf( std::ios_base::boolalpha );
				std::string commandName;
				stream >> commandName;		// Skip the command name.
				
				m_caller( stream );
			}
			
		private:
			
			StreamedFunctionCallerT m_caller;
		};
		

		template< typename StreamedFunctionCallerT >
		std::shared_ptr< CommandAbstract > registerCommand( void* host, const std::string& name, const std::string& description, StreamedFunctionCallerT&& caller )
		{
			typedef typename std::remove_reference< StreamedFunctionCallerT >::type caller_t;
			typedef Command< caller_t > CommandT;
			std::shared_ptr< CommandT > command( std::make_shared< CommandT >( host, name, description, std::forward< caller_t >( caller )));
			
			registerCommand( command );
			return command;
		}
		
		std::shared_ptr< CommandAbstract > registerCommand( std::shared_ptr< CommandAbstract > command );
		// REQUIRES( host );
		// REQUIRES( command );
		// PROMISES( hasCommand( command->name() ));
		
		void unregisterAllCommandsForHost( void* host );
		
		TelnetServer& telnetServer() const	{ ASSERT( m_server ); return *m_server; }

		void startListenServer();
		bool isListenServerStarted() const;
		
		void traceToConsole( const std::string& message );
		
		bool executeCommandLine( const std::string& commandLine );
		// Returns true iff the command line was well form and was actually executed.
		
		void findCompletingCommandNames( const std::string& commandNameSubstring, std::vector< std::string >& outMatchingCommands ) const;
		bool hasCommand( const std::string& name ) const;
		void getUsageString( std::ostream& stream, const std::string& name, bool verbose = false ) const;
		// REQUIRES( hasCommand( name ));
		
		void printHelp();
		void printUsage( const std::string& commandName );
		
		CoreCommands& coreCommands()		{ ASSERT( m_coreCommands ); return *m_coreCommands; }
		
	private:
		
		std::map< std::string, std::shared_ptr< CommandAbstract >> m_commands;
		
		SmartPtr< TelnetServer > m_server;
		
		std::unique_ptr< CoreCommands > m_coreCommands;
		
		void onDataReceived( const std::string& text );
		
		FRESH_DECLARE_CLASS( CommandProcessor, Object )
	};

}

#	define trace( msg ) { dev_trace( msg ); if( fr::CommandProcessor::doesExist() ) { std::stringstream ss_; ss_ << msg << std::endl; fr::CommandProcessor::instance().traceToConsole( ss_.str() ); } }
#	define con_warning( msg ) { dev_warning( msg ); if( fr::CommandProcessor::doesExist() ) { std::stringstream ss_; ss_ << msg << std::endl; fr::CommandProcessor::instance().traceToConsole( ss_.str() ); } }
#	define con_error( msg ) { dev_error( msg ); if( fr::CommandProcessor::doesExist() ) { std::stringstream ss_; ss_ << msg << std::endl; fr::CommandProcessor::instance().traceToConsole( ss_.str() ); } }

#endif
