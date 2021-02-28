/*
 *  CoreCommands.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/29/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_CORE_COMMANDS_H_INCLUDED
#define FRESH_CORE_COMMANDS_H_INCLUDED

#include "Objects.h"

namespace fr
{
	
	class CoreCommands
	{
	public:
		
		CoreCommands( class CommandProcessor& processor );
		~CoreCommands();

		std::string desiredSimulatedThrowMessage();
		bool wantsSimulatedGPF();
		
		void listClasses( const std::string& className ) const;
		void listPackages( PackageNameRef packageNameSubstring ) const;
		void searchPackages() const;
		void packageContents( PackageNameRef packageNameSubstring ) const;
		void packagesFor( const ObjectId& objectId ) const;
		void list( const ObjectId& objectId ) const;
		void examine( const ObjectId& objectId, bool showAllProps ) const;
		void examineAsXml( const ObjectId& objectId, bool showAllProps = false ) const;
		void get( const ObjectId& objectId, const std::string& propName ) const;
		void set( const ObjectId& objectId, const std::string& propName, const std::string& propValue, bool firstObjectOnly = true ) const;
		void setDefault( ClassNameRef className, const std::string& propName, const std::string& propValue ) const;
		void call( const ObjectId& objectId, const std::string& methodName, const gobbling_string& params ) const;
		void methods( ClassInfo::NameRef className ) const;
		void setBreakpoint( const std::string& tag, const ObjectId& objectId );
		void clearBreakpoint( const std::string& tag );
		void listBreakpoints( const std::string& tagFilter );
		void listBreakpointTags( const std::string& tagFilter );
		void simulateThrow( const gobbling_string& message );
		void simulateGeneralProtectionFault();
		
#ifdef FRESH_PROFILER_ENABLED
		void dumpProfile() const;
		void clearProfile() const;
#endif

	protected:
		
		void listToFormatter( const ObjectId& objectId, ObjectStreamFormatter& formatter, bool showAllProps = false ) const;
		
		bool setObjectProperty( Object::ptr object, const std::string& propName, const std::string& propValue ) const;

		std::string m_desiredSimulatedThrowMessage;
		bool m_wantsSimulatedGPF = false;
	};
}

#endif
