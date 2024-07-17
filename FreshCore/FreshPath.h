//
//  FreshPath.h
//  Fresh
//
//  Created by Jeff Wofford on 2/6/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_FreshPath_h
#define Fresh_FreshPath_h


#include <fstream>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <string>

#ifdef _WIN32
#	define _WIN32_LEAN_AND_MEAN
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif
#	include <windows.h>
#	undef abs
#else
#	include <sys/stat.h>		// For mkdir();
#	include <dirent.h>
#endif

namespace fr
{
	
	class path
	{
	public:
		
#ifdef _WIN32
		static const char preferred_separator = '\\';
#else 
		static const char preferred_separator = '/';
#endif

		path( const char* p = "" ) : m_path( p ) { normalize(); }
		path( const std::string& p ) : m_path( p ) { normalize(); }
		path( const path& p ) : path( p.string() ) {}
		
		path& operator=( const char* p )
		{
			m_path = p;
			normalize();
			return *this;
		}
		
		path& operator=( const std::string& p )
		{
			return operator=( p.c_str() );
		}
		
		path& operator=( const path& p )
		{
			return operator=( p.c_str() );
		}
		
		path& operator /=( const char* other )
		{
			if( other && std::strlen( other ) > 0 )
			{
				if(( m_path.empty() == false && m_path.back() != '/' ) && other[ 0 ] != '/' && other[ 0 ] != '\\' )
				{
					m_path.push_back( '/' );
				}
				m_path += other;
				normalize();	// TODO could make this cheaper by only normalizing the contents of other.
			}
			return *this;
		}
		
		path& operator/=( const std::string& other ) { return operator /=( other.c_str() ); }
		path& operator/=( const path& other ) { return operator /=( other.c_str() ); }
		
		path operator/( const char* other ) const { path result( *this ); result /= other; return result; }
		path operator/( const std::string& other ) const { return operator /( other.c_str() ); }
		path operator/( const path& other ) const { return operator /( other.c_str() ); }
		
		path& operator+=( const char* other )
		{
			if( other && std::strlen( other ) > 0 )
			{
				m_path += other;
			}
			return *this;
		}
		
		path& operator+=( const std::string& other ) { return operator+=( other.c_str() ); }
		path& operator+=( const path& other ) { return operator+=( other.c_str() ); }

		path operator+( const char* other ) { path result( *this ); result += other; return result; }
		path operator+( const std::string& other ) { return operator+( other.c_str() ); }
		path operator+( const path& other ) { return operator+( other.c_str() ); }

		void clear() { m_path.clear(); }
		bool empty() const { return m_path.empty(); }
		const std::string& string() const { return m_path; }
		const char* c_str() const { return m_path.c_str(); }

		path&  make_preferred()
		{
#ifdef _WIN32
			std::replace( m_path.begin(), m_path.end(), '/', '\\' );
#endif
			return *this;
		}

		bool operator ==( const path& other ) const { return m_path == other.m_path; }
		
		class iterator
		{
		public:
			
			typedef std::allocator< std::string > allocator_type;
			typedef allocator_type::difference_type difference_type;
			typedef allocator_type::value_type value_type;

			explicit iterator( std::string::const_iterator mine, std::string::const_iterator begin, std::string::const_iterator end ) : m_mine( mine ), m_begin( begin ), m_end( end ) {}
			
			iterator& operator++()
			{
				while( m_mine != m_end )
				{
					if( *m_mine == '/' )
					{
						++m_mine;
						break;
					}
					
					++m_mine;
				}
				return *this;
			}
			iterator operator++( int ) { return operator++(); }
			
			iterator& operator--()
			{
				bool skippedFirstSlash = m_mine == m_end;
				while( m_mine != m_begin )
				{
					if( m_mine != m_end && *m_mine == '/' )
					{
						if( skippedFirstSlash )
						{
							++m_mine;
							break;
						}
						else
						{
							skippedFirstSlash = true;
						}
					}
					
					--m_mine;
				}
				return *this;
			}
			iterator operator--( int ) { return operator--(); }
			
			std::string operator*() const
			{
				if( *m_mine == '/' )	// Should only be true for the root node (leading slash).
				{
					return "/";
				}
				
				auto slash = std::find( m_mine, m_end, '/' );
				std::string token;
				for( auto here = m_mine; here != slash; ++here )
				{
					token += *here;
				}
				return token;
			}
			
			bool operator ==( const iterator& other ) const
			{
				return m_mine == other.m_mine;
			}
			
			bool operator !=( const iterator& other ) const { return !operator==( other ); }
			
		private:

			std::string::const_iterator m_mine;
			std::string::const_iterator m_begin;
			std::string::const_iterator m_end;
			
		};
		
		
		path parent_path() const
		{
			auto prevToEnd = --end();
			
			if( empty() || begin() == prevToEnd ) return path();
			else
			{
				path pp;
				for( auto iter = begin(); iter != prevToEnd; ++iter )
				{
					pp /= *iter;
				}
				return pp;
			}
		}
		
		path filename() const
		{
			return empty() ? path() : *--end();
		}
		
		path stem() const
		{
			path myFilename = filename();
			if( myFilename.m_path == "." || myFilename.m_path == ".." )
			{
				return myFilename;
			}
			
			auto lastDotPos = myFilename.m_path.find_last_of( '.' );
			
			return myFilename.m_path.substr( 0, lastDotPos );
		}
		
		std::string extension() const
		{
			auto lastDot = m_path.find_last_of( '.' );
			if( lastDot != std::string::npos )
			{
				return m_path.substr( lastDot );
			}
			else
			{
				return "";
			}
		}
		
		typedef iterator const_iterator;
		
		iterator begin() const	{ return iterator( m_path.begin(), m_path.begin(), m_path.end() ); }
		iterator end() const	{ return iterator( m_path.end(),   m_path.begin(), m_path.end() ); }
		
	private:
		
		std::string m_path;
		
		void normalize()
		{
			// Make sure all path dividers are forward slashes.
			std::replace( m_path.begin(), m_path.end(), '\\', '/' );
		}
			
		friend std::istream& operator>>( std::istream& in, path& writeme );
		friend std::ostream& operator<<( std::ostream& out, const path& readme );
	};
	
	inline std::istream& operator>>( std::istream& in, path& writeme )
	{
		in >> writeme.m_path;
		return in;
	}
	
	inline std::ostream& operator<<( std::ostream& out, const path& readme )
	{
		out << readme.m_path;
		return out;
	}
	
	inline bool exists( const path& p )
	{
#ifdef _WIN32

		const DWORD attributes = ::GetFileAttributesA( p.string().c_str() );

		if( attributes != INVALID_FILE_ATTRIBUTES )
		{
			return true;
		}
		else
		{
			const auto errval = ::GetLastError();

			// Adapted from boost.
			bool notFound = errval == ERROR_FILE_NOT_FOUND
				|| errval == ERROR_PATH_NOT_FOUND
				|| errval == ERROR_INVALID_NAME  // "tools/jam/src/:sys:stat.h", "//foo"
				|| errval == ERROR_INVALID_DRIVE  // USB card reader with no card inserted
				|| errval == ERROR_NOT_READY  // CD/DVD drive with no disc inserted
				|| errval == ERROR_INVALID_PARAMETER  // ":sys:stat.h"
				|| errval == ERROR_BAD_PATHNAME  // "//nosuch" on Win64
				|| errval == ERROR_BAD_NETPATH;  // "//nosuch" on Win32

			// TODO not correctly handling ERROR_SHARING_VIOLATION

			return !notFound;
		}
#else
		struct stat status;
		const int result = ::stat( p.string().c_str(), &status );
		return result == 0;
#endif
	}
	
	inline path change_extension( const path& p, const std::string& newExt )
	{
		return p.parent_path() / ( std::string( p.stem().string() ) + (( newExt.empty() || newExt.front() == '.' ) ? "" : "." ) + newExt );
	}
	
	inline path strip_leading_dot( const path& p )
	{
		const auto str = p.string();
		if( str.empty() == false && str[ 0 ] == '.' )
		{
			return path{ str.substr( 1 ) };
		}
		else
		{
			return p;
		}
	}
	
	inline bool create_directory( const path& p )
	{
#ifdef _WIN32
		return ::CreateDirectoryA( p.string().c_str(), 0 ) != 0;
#else
		return ::mkdir( p.string().c_str(), S_IRWXU|S_IRWXG|S_IRWXO ) == 0;
#endif
	}
	
	inline bool create_directories( const path& p )
	{
		if( !exists( p ))
		{
			const path parent = p.parent_path();
			if( !parent.empty() )
			{
				if( !exists( parent ))
				{
					bool result = create_directories( parent );
					if( !result )
					{
						return false;
					}
				}
			}
			
			return create_directory( p );
		}
		else
		{
			return false;
		}
	}
	
	template< typename FnT >
	inline bool each_child_file( const path& folder, FnT&& fn )
	{
#if _WIN32
		const char* const szDir = ( folder / "*" ).c_str();
		
		WIN32_FIND_DATA ffd;
		HANDLE hFind = FindFirstFile( szDir, &ffd );
		
		if( hFind == INVALID_HANDLE_VALUE )
		{
			return false;
		}
		else
		{
			do
			{
				const std::string name = ffd.cFileName;
				const bool isRegularFile = ( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) == 0;
				fn( name, isRegularFile );
			}
			while( FindNextFile( hFind, &ffd ) != 0 );

			return true;
		}
#else
		DIR* directory = ::opendir( folder.c_str() );
		if( directory ) 
		{
            while( const dirent* const entry = ::readdir( directory ))
			{
                const bool isRegularFile = entry->d_type == DT_REG;
                if( isRegularFile || entry->d_type == DT_DIR )
                {
                    fn( std::string{ entry->d_name }, isRegularFile );
				}
			}
			::closedir( directory );
			return true;
		} 
		else 
		{
			return false;
		}
#endif
	}
}

#endif
