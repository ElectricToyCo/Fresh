//
//  SerializeVector.h
//  Fresh
//
//  Created by Jeff Wofford on 6/19/12.
//  Copyright (c) 2012 jeffwofford.com. All rights reserved.
//

#ifndef Fresh_SerializeVector_h
#define Fresh_SerializeVector_h

#include <iomanip>
#include "FreshDebug.h"
#include "FreshException.h"

namespace fr
{
	
	namespace SerializeVector
	{

		// Exception classes.
		class IncorrectDelimiter {};
		class MissingOpeningParenthesis {};
		class MissingClosingParenthesis {};
		class AdditionalTextAfterVector {};

		///////////////////////////////////////////////////////////////////
		
		template< class OStream, class Vector >
		OStream& write( OStream& out, const Vector& vec )
		{
			out << '(';
			for( size_t i = 0; i < Vector::nComponents; ++i )
			{
				out << vec[ i ];
				
				// If all the rest of the components have the same value, stop here.
				//
				if( i == 0 )
				{
					bool allTheSame = true;
					for( size_t j = 1; j < Vector::nComponents; ++j )
					{
						if( vec[ i ] != vec[ j ] )
						{
							allTheSame = false;
							break;
						}
					}
					
					if( allTheSame )
					{
						break;
					}
				}
				
				if( i + 1 < Vector::nComponents )
				{
					out << ',';
				}
			}
			out << ')';
			
			return out;
		}

		///////////////////////////////////////////////////////////////////
		
		template< class IStream, class Vector >
		IStream& read( IStream& in, Vector& vec )
		{
			// Use a temporary so that errors won't corrupt the *vec* parameter.
			Vector writtenVec;
			
			typedef typename Vector::element_type Real;
			
			bool openedParenthesis = false;
			
			// Skip any leading whitespace.
			in >> std::ws;
			
			// Check for opening parenthesis.	
			char c = in.peek();
			if( c == '(' )
			{
				// Move past the parenthesis.
				in.get();
				openedParenthesis = true;
			}
			
			Real firstValue = Real( 0 );
			size_t nExplicitValues = 0;
			for( size_t element = 0; element < Vector::nComponents; ++element )
			{
				// Skip whitespace.
				in >> std::ws;
				
				// Check for early finish.
				//
				if( in.eof() || ( openedParenthesis && in.peek() == ')' ))
				{
					// If only one value was given, use it for everything.
					// If more than one value was given, use zero for everything else.
					writtenVec[ element ] = nExplicitValues == 1 ? firstValue : Real( 0 );
					continue;
				}
				
				if( element > 0 )
				{
					// Read delimiter.
					
					char a = in.get();
					if( a != ',' )
					{
						FRESH_THROW( FreshException, "Incorrect delimiter '" << a << "'. Expected comma. (,)" );
					}
				}
				
				// Read the value.
				//
				in >> writtenVec[ element ];

				++nExplicitValues;
				
				if( element == 0 )
				{
					firstValue = writtenVec[ element ];
				}
			}
			
			if( openedParenthesis )
			{
				// Check for closing parenthesis.
				
				if( in.eof() )
				{
					FRESH_THROW( FreshException, "Found ( without )." );
				}
				
				in >> std::ws;
				c = in.peek();
				if( c == ')' )
				{
					// Move past the parenthesis.
					in.get();
				}
				else
				{
					FRESH_THROW( FreshException, "Found ( without )." );
				}
			}
			else if( !in.eof() )
			{
				in >> std::ws;
				c = in.peek();
				
				if( c == ')' )
				{
					FRESH_THROW( FreshException, "Found ) without (." );
				}
				else
				{
					FRESH_THROW( FreshException, "Additional text after vector, starting with '" << c << "'." );
				}
			}
			
			in.clear();
			
			vec = writtenVec;

			return in;
		}
	}
}

#endif
