//
//  Archive.cpp
//  Fresh
//
//  Created by Jeff Wofford on 11/2/12.
//
//

#include "Archive.h"

namespace fr
{

	void readToDelimiter( std::istream& in, std::string& outStr, const std::string& delimiters )
	{
		readWhile( in, outStr, [&]( int c )
				  {
					  return delimiters.find( c ) == std::string::npos;
				  } );
	}
	
	void readWhile( std::istream& in, std::string& outStr, std::function< bool( int ) >&& predicate )
	{
		while( in )
		{
			int c = in.peek();
			
			if( c != std::char_traits< char >::eof() && predicate( c ))
			{
				outStr.push_back( in.get() );
			}
			else
			{
				break;
			}
		}
	}
	
	bool Destringifier::skip( const char* sz, bool throwOnMismatch )
	{
		const char* originalSz = sz;
		
		while( *sz )
		{
			if( m_stream.fail() || m_stream.eof() )
			{
				if( throwOnMismatch )
				{
					FRESH_THROW( UnexpectedFormat, "While skipping \"" << originalSz << "\": stream " << ( m_stream.eof() ? " reached EOF." : " had error." ) );
				}
				return false;
			}
			
			const char c = m_stream.get();
			
			if( c != *sz )
			{
				if( throwOnMismatch )
				{
					FRESH_THROW( UnexpectedFormat, "While skipping \"" << originalSz << "\": stream '" << m_stream.str() << "' @ byte " << m_stream.tellg() << " mismatched skip string ('" << *sz << "')." );
				}
				else
				{
					m_stream.putback( c );
				}
				return false;
			}
			++sz;
		}
		
		return *sz == '\0';	// Safely reached the end of the input string.
	}

}
