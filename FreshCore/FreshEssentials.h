/*
 *  FreshEssentials.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 7/2/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 *	Defines general macros and functions related to this particular platform/compiler.
 *
 */

#ifndef FRESH_ESSENTIALS_H_INCLUDED
#define FRESH_ESSENTIALS_H_INCLUDED

#include <cstddef>
#include <locale>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>
#include <memory>
#include <functional>
#include <type_traits>
#include "StringTabulated.h"
#include "FreshOptional.h"
#include "FreshDebug.h"

#if defined(__GNUC__) && ( defined(__APPLE_CPP__) || defined(__APPLE_CC__) || defined(__MACOS_CLASSIC__) )
#	include <TargetConditionals.h>
#elif defined( _WIN32 )

#	define _WIN32_LEAN_AND_MEAN
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif
#	include <windows.h>
#	include <ShlObj.h>
#	undef min
#	undef max
#	undef abs
#	undef near
#	undef far
#	undef GetClassName
#	undef DrawText
#	undef LoadImage
#	undef GetCurrentTime
#	undef DELETE
#	undef NO_PRIORITY
#	undef MIN_PRIORITY
#	undef NUM_POINTS

#endif

#if TARGET_OS_MAC
#	include <CoreFoundation/CoreFoundation.h>
#	define FRESH_ALLOW_THREADING 1
#elif ANDROID
#	define FRESH_ALLOW_THREADING 1
#endif


#define STRINGIFY( x ) #x

// Helpful macro for preventing calls to an Object's copy constructor and operator=().
//
#define FRESH_PREVENT_COPYING( className )	\
private:	\
className( const className& );	\
void operator=( const className& );	\
className( const className&& );	\
void operator=( const className&& );

namespace fr
{
	template< typename T >
	using optional = std::experimental::optional< T >;
	
	
	// fr::passing< T >::type gives you the best type for passing a variable of type T:
	// i.e. pass by value for simple-and-small types, pass by reference for larger types or types with specialized copy construction.
	// Analogous to boost::call_traits<>.

#if 0
	// See http://stackoverflow.com/questions/12752220/how-to-create-a-c-template-to-pass-a-value-in-the-best-way	
	// This technique seems to work well but is not supported even as late as GCC 4.7.1
	//
	template< typename T >
	struct passing : std::conditional< std::< T >::value && sizeof( T ) <= sizeof( void* ), T, const T& > {};
#else
	template< typename T >
	struct passing : std::conditional< 
		std::is_pointer< T >::value ||
		( std::is_arithmetic< T >::value && sizeof( T ) <= sizeof( void* )) ||
		std::is_enum< T >::value,
		T, const T& > 
	{};
	template< typename T >
	struct passing< T& >
	{
		typedef const T& type;
	};
#endif
}

// Helpful macros for synthesizing default setters and getters.
// So for variable "string m_foo" you write: SYNTHESIZE( string, foo ). The macro produces:
//		foo( const string& foo_ );
//		const string& foo() const;
// Note that large and non-trivially-passable types are automatically passed by const reference rather than by value.
// The philosophy here is to always avoid public data by using getters and/or setters.
// Then later, if a setter or getter needs to become more complex than the default provided here,
// just implement your own replacement using the same function signature.
//
#ifndef _MSC_VER
	
	// Honest, decent, Christian implementation for non-Visual Studio compilers.
	//
#	define SYNTHESIZE_GET( propRefType, propName )	\
		inline typename fr::passing< propRefType >::type propName() const { return m_##propName; }

#	define SYNTHESIZE_SET( propRefType, propName )	\
		inline typename fr::passing< propRefType >::type propName( typename fr::passing< propRefType >::type propName##_ ) { m_##propName = propName##_; return m_##propName; }

// Pointers to forward-declared can't deal with assignment in the header, so we split SYNTHESIZE_SET into _DECL and _IMPL parts.
#	define SYNTHESIZE_SET_DECL( propRefType, propName )	\
		typename fr::passing< propRefType >::type propName( typename fr::passing< propRefType >::type propName##_ )

#	define SYNTHESIZE_SET_IMPL( className, propRefType, propName )	\
		typename fr::passing< propRefType >::type className::propName( typename fr::passing< propRefType >::type propName##_ ) { m_##propName = propName##_; return m_##propName; }

#	define SYNTHESIZE_SET_DEFINE( className, propRefType, propName )	\
		typename fr::passing< propRefType >::type className::propName( typename fr::passing< propRefType >::type newValue )
#else
	
	// Evil, wicked, morally degenerate implementation for Visual Studio compilers.
	//
#	define SYNTHESIZE_GET( propRefType, propName )	\
	inline propRefType propName() const { return m_##propName; }
	
#	define SYNTHESIZE_SET( propRefType, propName )	\
	inline propRefType propName( propRefType propName##_ ) { m_##propName = propName##_; return m_##propName; }
	
	// Pointers to forward-declared can't deal with assignment in the header, so we split SYNTHESIZE_SET into _DECL and _IMPL parts.
#	define SYNTHESIZE_SET_DECL( propRefType, propName )	\
	propRefType propName( propRefType propName##_ );
	
#	define SYNTHESIZE_SET_IMPL( className, propRefType, propName )	\
	propRefType className::propName( propRefType propName##_ ) { m_##propName = propName##_; return m_##propName; }

#	define SYNTHESIZE_SET_DEFINE( className, propRefType, propName )	\
	propRefType className::propName( propRefType newValue )
#endif
	
#define SYNTHESIZE( propRefType, propName )	\
	SYNTHESIZE_GET( propRefType, propName )	\
	SYNTHESIZE_SET( propRefType, propName )

// For forcing inline functions.
//
#undef ALWAYS_INLINE
#if !defined( DEBUG ) && defined( __GNUC__ )
	#define ALWAYS_INLINE inline __attribute__((always_inline))
#else
	#define ALWAYS_INLINE inline
#endif


#define GET_NUM_STATIC_ARRAY_ELEMENTS( arr ) (sizeof( arr ) / sizeof( (arr)[0]))

namespace fr
{
	
	typedef std::string PackageName;
	typedef const std::string& PackageNameRef;
	typedef std::string ClassName;
	typedef const std::string& ClassNameRef;
	typedef std::string ObjectName;
	typedef const std::string& ObjectNameRef;
	typedef std::string PropertyName;
	typedef const std::string& PropertyNameRef;
	
	
	const char* const DEFAULT_OBJECT_NAME = "";
	
	// Signals Objects to create themselves as "inert". Used for default objects.
	struct CreateInertObject {};

	std::locale& getGeneralLocale();
	
	std::string getPlatform();
	std::string getPlatformModel();
	std::string getPlatformSubmodel();
	
	std::string getOSVersion();
	
	std::string getDeviceId();
	
	unsigned int getDeviceIPAddress();
	std::string getStringIPAddress( unsigned int address );
	
	std::string getByteCountString( size_t nBytes );
	
	std::string urlEncode( const std::string& str );
	
	void savePreference( const std::string& key, const std::string& value );
	bool loadPreference( const std::string& key, std::string& outValue );
	// Returns true if found.
	void syncPreferences();
	
	bool isSystemMusicPlaying();
	
	inline bool doesStringHaveWhitespace( const std::string& s )
	{
		for( size_t i = 0; i < s.size(); ++i )
		{
			if( isspace( s[ i ] ))
			{
				return true;
			}
		}
		return false;
	}
	
	inline void stripWhitespaceFromString( std::string& inOutString )
	{
		// Strip whitespace from type name.
		for( size_t i = 0; i < inOutString.size(); ++i )
		{
			if( isspace( inOutString[ i ] ))
			{
				inOutString.erase( inOutString.begin() + i );
				--i;
			}
		}
	}
	
	inline std::string getWhitespaceStrippedString( const std::string& s )
	{
		std::string copy( s );
		stripWhitespaceFromString( copy );
		return copy;
	}
	
	inline void trimLeft( std::string& s )
	{
		s.erase( s.begin(), std::find_if( s.begin(), s.end(), [] ( int i ) { return ::isgraph( i ); } ) );
	}
	
	inline void trimRight( std::string& s )
	{
		s.erase( std::find_if( s.rbegin(), s.rend(), [] ( int i ) { return ::isgraph( i ); } ).base(), s.end() );
	}
	
	inline void trim( std::string& s )
	{
        trimLeft( s );
		trimRight( s );
	}
	
	template< typename ContainerT, typename FnT >
	auto map( const ContainerT& container, FnT&& fn )
	{
		using ResultT = decltype( fn( {} ) );
		std::vector< ResultT > result;
		std::transform( std::begin( container ), std::end( container ), std::back_inserter( result ), std::move( fn ));
		return result;
	}

	std::string toLower( const std::string& s );
	std::string toUpper( const std::string& s );
	
	void replaceAll( std::string& within, const std::string& replaced, const std::string& with );
	
	bool stringCaseInsensitiveCompare( const char* str1, const char* str2 );
	bool stringCaseInsensitiveCompare( const std::string& str1, const std::string& str2 );
	size_t stringCaseInsensitiveFind( const std::string& str1, const std::string& str2 );
	
	void getline( std::istream& stream, std::string& out, const std::string& delimiters );
	inline void getline( std::istream& stream, std::string& out, char delimiter ) { std::getline( stream, out, delimiter ); }
	
	bool matchesFilter( const std::string& str, const std::string& filter );
	// filter is regex string.
	
	std::vector< std::string > split( const std::string& str, const std::string& delims = " " );
	
	template< typename IterT >
	std::string join( IterT begin, IterT end, const std::string& connector = ", " )
	{
		std::ostringstream str;
		for( ; begin != end; ++begin )
		{
			if( str.str().empty() == false )
			{
				str << connector;
			}
			str << *begin;
		}
		return str.str();
	}

	template< typename T >
	std::string join( const T& v, const std::string& connector = ", " )
	{
		return join( std::begin( v ), std::end( v ), connector );
	}

	std::string toTraditionalLetters( int index );		// 0 becomes A, 25 becomes Z, 26 becomes AA, 27 becomes AB, etc.
	
	// Case insensitive comparison functor for use with std::map and such.
	//
	struct CompareStringsCaseInsensitive : public std::binary_function< std::string, std::string, bool >
	{
		struct CompareCharsCaseInsensitive
		{ 
			const std::ctype< char >& m_ctype;
			
			CompareCharsCaseInsensitive( const std::ctype< char >& c ) : m_ctype( c ) 
			{} 
			
			bool operator()( char x, char y ) const 
			{
				return m_ctype.toupper(x) < m_ctype.toupper(y);
			} 
		};
		
		CompareStringsCaseInsensitive()
		: m_ctype( std::use_facet< std::ctype< char > >( getGeneralLocale() ) ) 
		{}
		
		bool operator()( const std::string& x, const std::string& y ) const 
		{
			return std::lexicographical_compare( x.begin(), x.end(), y.begin(), y.end(), CompareCharsCaseInsensitive( m_ctype ) );
		}
		
	private:
		
		const std::ctype<char>& m_ctype;		
	};

	template< typename map_t >
	inline typename map_t::iterator find_value( const map_t& theMap, const typename map_t::mapped_type& value )
	{
		return std::find_if( theMap.begin(), theMap.end(), [&value] ( const typename map_t::value_type& pair )
					 {
						 return pair.second == value;
					 } );
	}

	template< typename map_t >
	inline typename map_t::const_iterator find_cvalue( const map_t& theMap, const typename map_t::mapped_type& value )
	{
		return std::find_if( theMap.begin(), theMap.end(), [&value] ( const typename map_t::value_type& pair )
					 {
						 return pair.second == value;
					 } );
	}

	template< typename map_t >
	inline const typename map_t::mapped_type& find_else( const map_t& theMap, const typename map_t::key_type& key, const typename map_t::mapped_type& theDefault = {})
	{
		const auto iter = theMap.find( key );
		return iter != theMap.end() ? iter->second : theDefault;
	}
	
	template< typename container_t, typename FunctionT >
	inline void removeElements( container_t& container, FunctionT&& filter )
	{
		container.erase( std::remove_if( container.begin(), container.end(), std::move( filter ) ), container.end() );
	}
	
	template< typename PtrTA, typename PtrTB >
	PtrTA unwrap( PtrTA a, PtrTB b )
	{
		ASSERT( b );
		return a ? a : PtrTA{ b };
	}
	
	struct CallsWhenDeleted
	{
		explicit CallsWhenDeleted( std::function< void() >&& fn ) : m_fn( std::move( fn )) {}
		~CallsWhenDeleted() { m_fn(); }
		
	private:
		
		std::function< void() > m_fn;
	};

    // Use `Global< T > g_globalX;` and `g_globalX.with( ... );` in order to avoid problems with globals being used
    // after deletion during the shutdown of systems.
    //
    template< typename T >
    class Global
    {
    public:
        Global() { s_exists = true; }
        ~Global() { s_exists = false; }
        
        template< typename FunctionT >
        void with( FunctionT&& fn )
        {
            if( s_exists )
            {
                fn( m_var );
            }
        }
        
    private:
        static bool s_exists;
        T m_var;
    };
    
    template< typename T > bool Global< T >::s_exists;

#define DEFER( fn ) auto deferred_ = CallsWhenDeleted( fn );

#if TARGET_OS_MAC
	std::string stringFromCFString( CFStringRef cfStr );
#endif
}


#endif
