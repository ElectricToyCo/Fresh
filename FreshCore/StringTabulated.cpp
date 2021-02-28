//
//  StringTabulated.cpp
//  Fresh
//
//  Created by Jeff Wofford on 2/27/13.
//
//

#include "StringTabulated.h"
#include "FreshDebug.h"
#include "FreshEssentials.h"
#include "FreshException.h"

namespace
{
	using StringTable = fr::StringTabulated::StringTable;
	
	StringTable& getTable()
	{
		static StringTable table;
		return table;
	}
	
	StringTable::const_iterator findInTable( const char* s )
	{
		return getTable().find( s );
	}
	StringTable::const_iterator findInTable( const std::string& s )
	{
		return getTable().find( s );
	}
}

namespace fr
{
	
	StringTabulated::StringTabulated()
	{
		*this = std::string{};
	}
	
	StringTabulated::StringTabulated( const char* sz )
	{
		*this = sz;
	}
	
	StringTabulated::StringTabulated( const std::string& s )
	{
		*this = s;
	}
	
	StringTabulated::StringTabulated( std::string&& s )
	{
		*this = std::forward< std::string >( s );
	}
	
	StringTabulated::operator const std::string&() const
	{
		return *m_iterMyString;
	}
	
	size_t StringTabulated::size() const
	{
		return m_iterMyString->size();
	}
	
	bool StringTabulated::empty() const
	{
		return m_iterMyString->empty();
	}
	
	void StringTabulated::clear()
	{
		m_iterMyString = findInTable( "" );
	}
	
	StringTabulated& StringTabulated::operator=( const char* sz )
	{
		if( !sz )
		{
			FRESH_THROW( FreshException, "StringTabulated assigned to null string." );
		}

		return *this = std::string( sz );
	}
	
	StringTabulated& StringTabulated::operator=( const std::string& s )
	{
		m_iterMyString = findInTable( s );
		if( m_iterMyString == getTable().end() )
		{
			// Add it.
			//
			const auto& pair = getTable().insert( s );
			ASSERT( pair.second );
			m_iterMyString = pair.first;
		}
		ASSERT( m_iterMyString != getTable().end() );
		ASSERT( *m_iterMyString == s );
		
		return *this;
	}
	
	StringTabulated& StringTabulated::operator=( std::string&& s )
	{
		m_iterMyString = findInTable( s );
		if( m_iterMyString == getTable().end() )
		{
			// Add it.
			//
			const auto& pair = getTable().insert( std::move( s ));
			ASSERT( pair.second );
			m_iterMyString = pair.first;
		}
		ASSERT( m_iterMyString != getTable().end() );
		// ASSERT( *m_iterMyString == s );	// This is the spirit of the law, but I can't actually check it here because s probably got moved at the emplace.
		
		return *this;
	}
	
	bool StringTabulated::operator==( const StringTabulated& s ) const
	{
		return m_iterMyString == s.m_iterMyString;
	}
	
	bool StringTabulated::operator>( const StringTabulated& s ) const
	{
		return *m_iterMyString > *s.m_iterMyString;
	}
	
	bool StringTabulated::operator<( const StringTabulated& s ) const
	{
		return *m_iterMyString < *s.m_iterMyString;
	}

	bool StringTabulated::isEqualCaseInsensitive( const StringTabulated& s ) const
	{
		return stringCaseInsensitiveCompare( static_cast< const std::string& >( *this ), static_cast< const std::string& >( s ));
	}	

	int StringTabulated::compare( const StringTabulated& s ) const
	{
		return std::strcmp( m_iterMyString->c_str(), s.m_iterMyString->c_str() );
	}
	

}
