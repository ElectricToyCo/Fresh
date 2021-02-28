//
//  FreshException.h
//  Fresh
//
//  Created by Jeff Wofford on 6/24/12.
//  Copyright (c) 2012 jeffwofford.com. All rights reserved.
//

#ifndef Fresh_FreshException_h
#define Fresh_FreshException_h

#include "CoreCppCompatability.h"
#include <exception>
#include <stdexcept>
#include <string>
#include <sstream>

namespace fr
{
	
	class FreshException : public std::exception
	{
	public:

		FreshException( const char* file, int line, const char* function, const std::string& explanation );
		
		virtual const char* what() const noexcept;
		
		const char* file() const noexcept						{ return m_file; }
		int line() const noexcept								{ return m_line; }
		const char* function() const noexcept					{ return m_function; }

	private:

		std::string m_explanation;
		const char* m_file;
		int m_line;
		const char* m_function;
		
		mutable std::string m_formattedMessage;
	};
	
}

#define FRESH_DEFINE_EXCEPTION_TYPE( className ) class className : public fr::FreshException { public: explicit className( const char* file, int line, const char* function, const std::string& explanation ) : FreshException( file, line, function, explanation ) {} };

#define FRESH_THROW( exceptionType, explanation ) \
	{	std::ostringstream formattedExplanation; formattedExplanation << explanation;	\
		throw exceptionType( __FILE__, __LINE__, FRESH_CURRENT_FUNCTION, formattedExplanation.str() );	\
	}

#endif
