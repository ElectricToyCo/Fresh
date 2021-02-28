//
//  StringTabulated.h
//  Fresh
//
//  Created by Jeff Wofford on 2/27/13.
//
//

#ifndef Fresh_StringTabulated_h
#define Fresh_StringTabulated_h

#include <utility>
#include <string>
#include <set>			// Slower than unordered_set, but protects iterator validity when it changes, which we require.
#include <iostream>

namespace fr
{
	
	class StringTabulated
	{
	public:
		
		typedef std::set< std::string > StringTable;

		StringTabulated();
		StringTabulated( const char* sz );	// Implicit
		StringTabulated( const std::string& s );	// Implicit
		StringTabulated( std::string&& s );	// Implicit
		
		operator const std::string&() const;
		
		size_t size() const;
		size_t length() const									{ return size(); }
		bool empty() const;
		void clear();
		
		StringTabulated& operator=( const char* sz );
		StringTabulated& operator=( const std::string& s );
		StringTabulated& operator=( std::string&& s );
		
		bool operator==( const StringTabulated& s ) const;
		bool operator!=( const StringTabulated& s ) const		{ return !operator==( s ); }
		bool operator>( const StringTabulated& s ) const;
		bool operator<( const StringTabulated& s ) const;
		
		bool isEqualCaseInsensitive( const StringTabulated& s ) const;
		
		int compare( const StringTabulated& s ) const;
		// Acts like strcmp.
		
	private:
		
		StringTable::const_iterator m_iterMyString;
	
		friend std::ostream& operator<<( std::ostream& out, const StringTabulated& s );
		friend std::istream& operator>>( std::istream& in, StringTabulated& s );
		
	};

	///////////////////////////////////////////////////////////////////////////////////
	
	inline std::ostream& operator<<( std::ostream& out, const StringTabulated& s )
	{
		out << std::string( s );
		return out;
	}
	
	inline std::istream& operator>>( std::istream& in, StringTabulated& s )
	{
		std::string str;
		in >> str;
		s = std::move( str );
		return in;
	}
	
	
}

#endif
