//
//  escaped_string.h
//  Fresh
//
//  Created by Jeff Wofford on 3/8/12.
//  Copyright (c) 2012 jeffwofford.com. All rights reserved.
//

#ifndef Fresh_escaped_string_h
#define Fresh_escaped_string_h

#include <string>
#include <iostream>

namespace fr
{

	class gobbling_string : public std::string
	{};
	
	inline std::istream& operator>>( std::istream& in, gobbling_string& str )
	{
		while( in.good() )
		{
			int c = in.get();
			if( c == std::char_traits< char >::eof() )
			{
				break;
			}
			
			str += c;
		}
		
		return in;
	}

	////////////////////////////////////////////////////

	class escaped_string : public std::string
	{};
	
	inline std::istream& operator>>( std::istream& in, escaped_string& str )
	{
		bool skipLeadingSpaces = true;
		
		while( in.good() )
		{
			int c = in.get();
			
			if( isspace( c ) )
			{
				if( skipLeadingSpaces )
				{
					continue;
				}
				else if( !str.empty() && str.back() == '\\' )
				{
					str.pop_back();
				}
				else
				{
					break;
				}
			}
			else
			{
				skipLeadingSpaces = false;
			}
			
			str += c;
		}
		
		return in;
	}
	
	////////////////////////////////////////////////////
	
	class explicitly_terminated_string : public std::string
	{};
	
	inline std::istream& operator>>( std::istream& in, explicitly_terminated_string& str )
	{
		bool skipLeadingSpaces = true;
		
		const char terminator[] = "<EOF />";
		const size_t lenTerminator = strlen( terminator );
		const size_t LEN_READAHEAD = 8;
		
		assert( LEN_READAHEAD >= lenTerminator );
		
		char readAhead[ LEN_READAHEAD ];
		std::memset( readAhead, 0, LEN_READAHEAD );
		char* endReadAhead = readAhead + lenTerminator;
		
		while( in.good() )
		{
			const int c = in.get();

			if( isspace( c ) )
			{
				if( skipLeadingSpaces )
				{
					continue;
				}
			}
			else
			{
				skipLeadingSpaces = false;
			}
			
			str += c;
			
			// Append c to the back of readAhead.
			//
			std::copy( readAhead + 1, endReadAhead, readAhead );
			*( endReadAhead - 1 ) = c;
			
			// Look for terminator.
			//
			if( std::memcmp( readAhead, terminator, lenTerminator ) == 0 )
			{
				// Remove the terminator from the string.
				//
				str.erase( str.begin() + ( str.size() - lenTerminator ), str.end() );
				break;
			}
		}
		
		return in;
	}

}

#endif
