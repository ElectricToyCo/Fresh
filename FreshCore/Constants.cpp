//
//  Constants.cpp
//  Fresh
//
//  Created by Jeff Wofford on 6/29/15.
//  Copyright (c) 2015 Jeff Wofford. All rights reserved.
//

#include "Constants.h"

namespace
{
	using namespace fr::constant;
	std::map< Type, std::map< Name, std::string >> g_constants;
	
	inline std::string normalizeCase( const std::string& text )
	{
		return fr::toLower( text );
	}
}

namespace fr
{
	namespace constant
	{
		void setValueString( TypeRef type, NameRef name, const std::string& value )
		{
			g_constants[ normalizeCase( type ) ][ normalizeCase( name ) ] = value;
		}
		
		bool exists( TypeRef type, NameRef name )
		{
			const auto iter = g_constants.find( normalizeCase( type ));
			if( iter != g_constants.end() )
			{
				return iter->second.find( name ) != iter->second.end();
			}
			else
			{
				return false;
			}
		}
		
		bool isRecognizedType( TypeRef type )
		{
			return g_constants.find( normalizeCase( type )) != g_constants.end();
		}
		
		const std::string& getValueString( TypeRef type, NameRef name )
		{
			static const std::string defaultValue{};
			
			const auto iter = g_constants.find( normalizeCase( type ));
			if( iter != g_constants.end() )
			{
				const auto& typeMap = iter->second;
				const auto innerIter = typeMap.find( normalizeCase( name ));
				if( innerIter != typeMap.end() )
				{
					return innerIter->second;
				}
				else
				{
					return defaultValue;
				}
			}
			else
			{
				return defaultValue;
			}
		}
		
		NameRef getNameForValueString( TypeRef type, const std::string& value )
		{
			const auto iter = g_constants.find( normalizeCase( type ));
			if( iter != g_constants.end() )
			{
				const auto& typeMap = iter->second;
				
				for( const auto& pair : typeMap )
				{
					if( pair.second == value )
					{
						return pair.first;
					}
				}
			}
			
			static const std::string defaultValue{};
			return defaultValue;
		}
	}
}

