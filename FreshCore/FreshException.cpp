//
//  FreshException.cpp
//  Fresh
//
//  Created by Jeff Wofford on 6/24/12.
//  Copyright (c) 2012 jeffwofford.com. All rights reserved.
//

#include "FreshException.h"

namespace
{
	std::string simplify( std::string s )
	{
		const auto pos = s.find_last_of( "/\\" );
		if( pos < s.size() )
		{
			s.erase( 0, pos + 1 );
		}
		return s;
	}
}

namespace fr
{
	
	FreshException::FreshException( const char* file, int line, const char* function, const std::string& explanation )
	:	m_explanation( explanation )
	,	m_file( file )
	,	m_line( line )
	,	m_function( function )
	{}
	
	const char* FreshException::what() const noexcept
	{
		if( m_formattedMessage.empty() )
		{
			// Build the formatted message.
			//
			std::stringstream s;
			s	<< "In function " << ( m_function ? m_function : "<unknown function>" )
				<< " (" << ( m_file ? simplify( m_file ) : "<unknown file>" )
				<< ':' << m_line << "): "
				<< m_explanation;
				
			m_formattedMessage = s.str();
		}
		
		return m_formattedMessage.c_str();
	}
}

