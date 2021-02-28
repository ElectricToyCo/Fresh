//
//  Constants.h
//  Fresh
//
//  Created by Jeff Wofford on 6/29/15.
//  Copyright (c) 2015 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_Constants_h
#define Fresh_Constants_h

#include <string>
#include "Archive.h"
#include "FreshException.h"

namespace fr
{
	namespace constant
	{
		using Name = std::string;
		using NameRef = const std::string&;

		using Type = std::string;
		using TypeRef = const std::string&;
		
		void setValueString( TypeRef type, NameRef name, const std::string& value );
		bool exists( TypeRef type, NameRef name );
		bool isRecognizedType( TypeRef type );
		
		const std::string& getValueString( TypeRef type, NameRef name );

		NameRef getNameForValueString( TypeRef type, const std::string& value );

		template< typename T >
		void set( TypeRef type, NameRef name, const T& value )
		{
			std::ostringstream stream;
			Stringifier stringifier( stream );
			stringifier << value;
			
			setValueString( type, name, stream.str() );
		}		
		
		template< typename T >
		T get( TypeRef type, NameRef name )
		{
			const auto valueString = getValueString( type, name );
			Destringifier destringifier( valueString );
			T value = T{};
			destringifier >> value;
			return value;
		}
	}
		
}

#endif
