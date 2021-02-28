

/*
 *  Archive.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 3/1/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 *	Includes classes useful for serialization and de-serialization of Objects and other datatypes.
 *
 */

#ifndef _FRESH_ARCHIVE_H_INCLUDED
#define _FRESH_ARCHIVE_H_INCLUDED

#include "FreshException.h"
#include "ObjectLinker.h"
#include "Object.h"
#include "StringTable.h"
#include "FreshPath.h"
#include <sstream>
#include <iomanip>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <iterator>

namespace fr
{
	
	void readToDelimiter( std::istream& in, std::string& outStr, const std::string& delimiters );
	void readWhile( std::istream& in, std::string& outStr, std::function< bool( int ) >&& predicate );

	// Class Stringifier is an expert at serializing all sorts of data types into formatted strings.
	//
	class Stringifier
	{
	public:
		
		struct SavedObjectObserver
		{
			virtual ~SavedObjectObserver() {}
			virtual void onObjectSerialized( Object::cptr object ) = 0;
		};
		
		Stringifier( std::ostream& stream, SavedObjectObserver* observer = nullptr )
		:	m_stream( stream )
		,	m_observer( observer )
		{
			m_stream << std::setw( 0 );
		}
		
		std::ostream& rawStream() const { return m_stream; }		
		
		SavedObjectObserver* getObjectObserver() const		{ return m_observer; }
		
		template< typename T >
		Stringifier& operator<<( const T& value )
		{
			m_stream << value;
			return *this;
		}

		Stringifier& operator<<( bool value )
		{
			m_stream << std::boolalpha << value << std::resetiosflags( std::ios_base::boolalpha );
			return *this;
		}
			
		Stringifier& operator<<( const std::string& value )
		{
			m_stream << "\"" << value << "\"";
			return *this;
		}
		
		Stringifier& operator<<( ClassInfo::cptr const value )
		{
			if( value )
			{
				m_stream << value->className();
			}
			else
			{
				m_stream << "null";
			}
			return *this;
		}
		
		template< typename T >
		Stringifier& operator<<( const WeakPtr< T >& value )
		{
			return operator<<( static_cast< const Object* >( value.get() ));
		}
		
		template< typename T >
		Stringifier& operator<<( const SmartPtr< T >& value )
		{
			return operator<<( static_cast< const Object* >( value.get() ));
		}
		
		template< typename FirstT, typename SecondT >
		Stringifier& operator<<( const std::pair< FirstT, SecondT >& aPair )
		{
			*this << "{" << aPair.first << "=" << aPair.second << "}";
			return *this;
		}

		
		template< size_t i >
		struct writeTupleElements
		{
			template< typename Tuple >
			static inline void write( Stringifier& stringifier, Tuple& aTuple )
			{
				// Writer earlier elements (front to back).
				//
				writeTupleElements< i - 1 >::write( stringifier, aTuple );
				stringifier << ", " << std::get< i >( aTuple );
			}
		};
		
		template< typename... Args >
		Stringifier& operator<<( const std::tuple< Args... >& aTuple )
		{
			typedef decltype( aTuple ) tuple_t;
			
			*this << "{";
			
			writeTupleElements< std::tuple_size< typename std::decay< tuple_t >::type >::value - 1 >::write( *this, aTuple );
			
			*this << "}";
			return *this;
		}
		
		
		template<typename IteratorT>
		void writeSequence( IteratorT first, IteratorT last )
		{
			m_stream << "[";

			IteratorT cur = first;
			
			while( cur != last )
			{
				if( cur != first )
				{
					m_stream << ",";
				}
				
				*this << *cur;
				
				++cur;
			}
			
			m_stream << "]";
		}
			
		template< class T >
		Stringifier& operator<<( const std::vector< T >& container )
		{
			writeSequence( container.begin(), container.end() );
			return *this;
		}	

		template< typename T >
		Stringifier& operator<<( const std::list< T >& container )
		{
			writeSequence( container.begin(), container.end() );
			return *this;
		}	
		
		template< typename Key, typename Value >
		Stringifier& operator<<( const std::map< Key, Value >& container )
		{
			m_stream << "{";
			
			for( auto iter = container.begin(); iter != container.end(); ++iter )
			{
				if( iter != container.begin() )
				{
					m_stream << ",";
				}
				
				*this << iter->first << "=" << iter->second;
			}
			
			m_stream << "}";
			
			return *this;
		}	
	
	private:
		
		std::ostream& m_stream;
		SavedObjectObserver* m_observer;

		Stringifier& operator<<( const Object* value )
		{
			if( !value )
			{
				m_stream << NULL_PTR_STRING;
			}
			else
			{
				m_stream << value->toString();
			}
			
			if( m_observer )
			{
				m_observer->onObjectSerialized( value );
			}
			
			return *this;
		}
			
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////

	class StringifierObjectGraph : public Stringifier::SavedObjectObserver
	{
	public:
		
		typedef std::set< Object::cptr > ObjectSet;
		
		virtual void onObjectSerialized( Object::cptr object ) override
		{
			if( object )
			{
				m_foundObjects.insert( object );
			}
		}
		
		const ObjectSet& getFoundObjects() const { return m_foundObjects; }
		
		void clear()					{ m_foundObjects.clear(); }

	private:
		
		 ObjectSet m_foundObjects;
		
	};
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	
	class Destringifier
	{
	public:
		
		// Exceptions
		
		FRESH_DEFINE_EXCEPTION_TYPE( UnexpectedFormat )
		
		// Member functions
		
		explicit Destringifier( const std::string& aString = "" )
		:	m_stream( aString )
		{
			m_stream >> std::boolalpha;
		}
		
		SYNTHESIZE( bool, gobbleStrings );
		
		std::istream& stream() { return m_stream; }
		
		std::string str() const
		{
			return m_stream.str();
		}
		
		SYNTHESIZE( void*, propertyOwner );
		
		explicit operator bool() const  { return static_cast< bool >( m_stream ); }
		bool eof() const				{ return m_stream.eof(); }
		int get()						{ return m_stream.get(); }
		int peek()						{ return m_stream.peek(); }
		void putback( char c )			{ m_stream.putback( c ); }
		
		template< typename T >
		Destringifier& operator>>( T& value )
		{
			ASSERT( !m_stream.fail() );
			m_stream >> value;
			if( m_stream.fail() )
			{
				FRESH_THROW( UnexpectedFormat, "Format exception" );
			}
			
			return *this;
		}
		
		Destringifier& operator>>( path& value )
		{
			std::string pathString;
			operator>>( pathString );
			value = pathString;
			return *this;
		}
		
		Destringifier& operator>>( StringTabulated& value )
		{
			std::string str;
			operator>>( str );
			value = std::move( str );
			return *this;
		}
		
		Destringifier& operator>>( std::string& value )
		{
			if( m_gobbleStrings )
			{
				value = m_stream.str();
				if( !value.empty() && value.front() == '!' )
				{
					// String table string. Read the identifier.
					//
					value.erase( value.begin() );
					value = StringTable::string( value );
				}
			}
			else
			{
				value.clear();
				
				m_stream >> std::ws;	// Clear initial whitespace.
				
				if( m_stream.peek() == '!' )
				{
					// String table string. Read the identifier.
					//
					m_stream.get();	// Skipping the string table marker.
					readToDelimiter( m_stream, value, m_activeStringDelimiters + " \t\n" );
					value = StringTable::string( value );
				}
				else if( m_stream.peek() == '\"' )
				{
					// Has opening quote. Read to closing quote.
					
					m_stream.get();	// Skip opening quote.
					
					bool escapingNext = false;
					while( m_stream )
					{
						int c = m_stream.get();
						if( c == std::char_traits< char >::eof() )
						{
							dev_warning( "No closing quote on string " << value );
							break;
						}
						
						if( escapingNext )
						{
							if( c == 'n' )
							{
								c = '\n';
							}
							else if( c == 't' )
							{
								c = '\t';
							}
						}
						else
						{
							if( c == '\\' )		// Backslash.
							{
								escapingNext = true;
								continue;
							}
							else if( c == '\"' )		// Found double-quote character.
							{
								// Found ending quote.
								break;
							}
						}
						
						escapingNext = false;
						value.push_back( c );
					}
				}
				else
				{
					readToDelimiter( m_stream, value, m_activeStringDelimiters + " \t\n" );
				}
			}
			return *this;
		}
		
		Destringifier& operator>>( ClassInfo::cptr& value )
		{
			std::string className;
			m_stream >> std::ws;
			readWhile( m_stream, className, []( int c ) { return c == '_' || ::isalnum( c ); } );
			
			if( className != "null" )
			{
				value = getClass( className );
				if( !value )
				{
					dev_warning( "Unrecognized class name '" << className << "'." );
				}
			}
			return *this;
		}

		template< typename T >
		Destringifier& operator>>( SmartPtr< T >& value )
		{
			ObjectId objectId;
			m_stream >> objectId;

			if( !objectId.isValid() )
			{
				FRESH_THROW( FreshException, "ObjectId parsing encountered incorrect ObjectId format." );
			}
			
			if( !m_suppressPointerFixup && !objectId.isNull() )
			{
				ObjectLinker::instance().getObjectAddressOrAddForFixup( objectId, m_propertyOwner, value );
			}
			else
			{
				value = nullptr;
			}
			
			return *this;
		}
		
		template< typename T >
		Destringifier& operator>>( WeakPtr< T >& value )
		{
			ObjectId objectId;
			m_stream >> objectId;
			if( !objectId.isValid() )
			{
				FRESH_THROW( FreshException, "ObjectId parsing encountered incorrect ObjectId format." );
			}

			if( !m_suppressPointerFixup && !objectId.isNull() )
			{
				ObjectLinker::instance().getObjectAddressOrAddForFixup( objectId, m_propertyOwner, value );
			}
			
			return *this;
		}
		
		template< typename FirstT, typename SecondT >
		Destringifier& operator>>( std::pair< FirstT, SecondT >& aPair )
		{
			m_gobbleStrings = false;
			bool foundOpening = skip( "{", false );
			
			m_activeStringDelimiters = ",|=)]}";
			
			*this >> aPair.first;
			m_stream >> std::ws;
			if( !isDelimiter( m_stream.peek() ))
			{
				FRESH_THROW( UnexpectedFormat, "Missing separator , or = while reading pair." );
			}
			m_stream.get();
			*this >> aPair.second;
			
			m_activeStringDelimiters.clear();
			
			if( foundOpening )
			{
				m_stream >> std::ws;
				skip( "}" );
			}
			
			return *this;
		}
		
		template< size_t i >
		struct readTupleElements
		{
			template< typename Tuple >
			static inline void read( Destringifier& destringifier, Tuple& aTuple )
			{
				// Read earlier elements (front to back).
				//
				readTupleElements< i - 1 >::read( destringifier, aTuple );

				destringifier >> std::get< i >( aTuple );
				
				destringifier.stream() >> std::ws;
				if( destringifier.stream().peek() == ',' )
				{
					destringifier.stream().get();
				}
			}
		};

		template< typename... Args >
		Destringifier& operator>>( std::tuple< Args... >& aTuple )
		{
			typedef decltype( aTuple ) tuple_t;
			
			m_gobbleStrings = false;
			bool foundOpening = skip( "{", false );
			
			m_activeStringDelimiters = ",|=)]}";
			
			readTupleElements< std::tuple_size< typename std::decay< tuple_t >::type >::value - 1 >::read( *this, aTuple );
			
			m_activeStringDelimiters.clear();
			
			if( foundOpening )
			{
				m_stream >> std::ws;
				skip( "}" );
			}
			
			return *this;
		}
		
		template< class T >
		Destringifier& operator>>( std::vector< T >& container )
		{
			const size_t size = readSequence( container, true );	// Get the size only.
			
			container.clear();
			container.reserve( size );
			
			readSequence( container );
			return *this;
		}	
		
		template< typename T >
		Destringifier& operator>>( std::list< T >& container )
		{
			readSequence( container );
			return *this;
		}	
		
		template< typename Key, typename Value >
		Destringifier& operator>>( std::map< Key, Value >& container )
		{
			container.clear();
			
			m_gobbleStrings = false;
			m_stream >> std::ws;
			
			// Must begin with '{'.
			
			int c = m_stream.get();
			if( c != '{' )
			{
				// Maybe an empty map?
				//
				if( c == std::char_traits< char >::eof() )
				{
					return *this;
				}
				else
				{
					FRESH_THROW( UnexpectedFormat, "map should begin with '{'. We read '" << c << "'." );
				}
			}
			
			while( m_stream )
			{
				// Read key.
				//
				m_stream >> std::ws;
				c = m_stream.peek();
				
				if( c == '}' )
				{
					// Done.
					break;
				}
				else if( c == std::char_traits< char >::eof() )
				{
					dev_warning( "map should end with '}'. We read '" << c << "'." );
					break;
				}
				
				m_activeStringDelimiters = ",|=)]}";
				Key key;
				*this >> key;
				
				m_stream >> std::ws;
				c = m_stream.get();

				if( c != '=' && c != ',' )
				{
					FRESH_THROW( UnexpectedFormat, "map key and value must be separated by '='. We read '" << c << "'." );
				}
				
				// Insert a placeholder pair into the map.
				// We need to do this first so that we can extract from a stringifier straight into the value,
				// to support in-place pointer fixup.
				//
				Value& value = ( container[ key ] = Value{} );
				
				*this >> value;

				m_activeStringDelimiters.clear();
				m_stream >> std::ws;
				
				// There might be a comma now.
				//
				if( m_stream.peek() == ',' )
				{
					m_stream.get();
				}
			}
						
			return *this;
		}
		
		template< typename ContainerT >
		size_t readSequence( ContainerT& container, bool countOnly = false )
		{
			typedef typename ContainerT::value_type ElementT;
		
			// Remember the current position.
			std::streamoff curPosition = m_stream.tellg();

			size_t nElements = 0;
			
			m_stream >> std::ws;
			
			const bool foundOpening = m_stream.peek() == '[';
			
			if( foundOpening )
			{
				m_stream.get();
			}
	
			m_gobbleStrings = false;
			m_activeStringDelimiters = ",|])}";
			
			while( m_stream )
			{
				// Make sure there's something here.
				//
				m_stream >> std::ws;
				const int c = m_stream.peek();
				if( c == std::char_traits< char >::eof() || ( foundOpening && c == ']' ))
				{
					break;
				}
				
				++nElements;
				if( !countOnly )
				{
					// Create and read into the element.
					//
					container.resize( std::max( nElements, container.size() ));
					
					auto iter = container.begin();
					std::advance( iter, nElements - 1 );
					
					ElementT& element = *iter;
					
					try
					{
						*this >> element;
					}
					catch( ... )
					{
						// Abort this sequence read.
						//
						throw;
					}
				}
				else
				{
					ElementT element;
					m_suppressPointerFixup = true;
					*this >> element;
					m_suppressPointerFixup = false;
				}
				
				// Check for and skip delimiter.
				m_stream >> std::ws;

				if( isDelimiter( m_stream.peek() ))
				{
					m_stream.get();
				}
			}

			m_activeStringDelimiters.clear();
			
			if( foundOpening )
			{
				while( true )
				{
					m_stream >> std::ws;
					
					auto c = m_stream.peek();
					
					if( c == std::char_traits< char >::eof() )
					{
						dev_warning( "While reading sequence, the expected closing brace ']' was missing." );
						break;
					}
					else if( c == ']' )
					{
						m_stream.get();
						break;
					}
					else if( isDelimiter( c ))
					{
						m_stream.get();
					}
					else
					{
						dev_warning( "While reading sequence, found '" << (char) c << "' rather than the expected closing brace ']'." );
						break;
					}
				}
			}

			if( countOnly )
			{
				// Restore the position.
				m_stream.clear();					// Clear fail bit, if any.
				m_stream.seekg( curPosition );
			}
			else
			{
				// Ensure the container is no larger than the required size.
				//
				ASSERT( container.size() >= nElements );
				container.resize( nElements );
			}
			
			return nElements;
		}

		bool skip( const char* sz, bool throwOnMismatch = true );
		
		static bool isDelimiter( char c )    { return c == ',' || c == '|' || c == '='; }
				
	private:
		
		std::istringstream m_stream;
		bool m_gobbleStrings = true;		// If true, unquoted strings read until the end of the stream; otherwise they read until whitespace.
		bool m_suppressPointerFixup = false;
		std::string m_activeStringDelimiters;
		void* m_propertyOwner = nullptr;
		
		template< typename T >
		Destringifier& operator>>( T*& value );			// Deprecated
		Destringifier& operator>>( Object*& value );	// Deprecated
	};

	////////////////////////////////////////////////////////////////////////
	
	template<>
	struct Destringifier::readTupleElements< 0 >
	{
		template< typename Tuple >
		static inline void read( Destringifier& destringifier, Tuple& aTuple )
		{
			destringifier >> std::get< 0 >( aTuple );
			
			destringifier.stream() >> std::ws;
			if( destringifier.stream().peek() == ',' )
			{
				destringifier.stream().get();
			}
			
			// Done.
		}
	};

	template<>
	struct Stringifier::writeTupleElements< 0 >
	{
		template< typename Tuple >
		static inline void write( Stringifier& stringifier, Tuple& aTuple )
		{
			stringifier << std::get< 0 >( aTuple );
			// Done.
		}
	};

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ENUM SERIALIZATION SUPPORT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if DEV_MODE
#	define FRESH_ENUM_DEBUG const char* szEnumType = STRINGIFY( EnumNamespace##_##EnumType );
#else
#	define FRESH_ENUM_DEBUG
#endif

#define FRESH_ENUM_STREAM_IN_BEGIN( EnumNamespace, EnumType )	\
	inline std::istream& operator>>( std::istream& in, EnumNamespace::EnumType& enumValue )	\
	{	\
		in >> std::ws;	\
		FRESH_ENUM_DEBUG	\
		std::string enumAsString;	\
		while( in ) {	\
			const int c = in.get();	\
			if( c != '_' && !::isalnum( c ))	\
			{	\
				in.putback( c );	\
				break;	\
			}	\
			enumAsString.push_back( c );	\
		}	\
		in.clear();

#define FRESH_ENUM_STREAM_IN_CASE( EnumNamespace, EnumCase )	\
		if( enumAsString == #EnumCase )	\
		{	\
			enumValue = EnumNamespace::EnumCase;	\
		}	\
		else

#define FRESH_ENUM_STREAM_IN_END()	\
		{	\
			in.setstate( std::ios::failbit );	\
			dev_warning( "While loading enum " << szEnumType << " from stream: Unrecognized Enumerant '" << enumAsString << "'." );	\
		}	\
		return in;	\
	}

#define FRESH_ENUM_STREAM_OUT_BEGIN( EnumNamespace, EnumType )	\
	inline std::ostream& operator<<( std::ostream& out, EnumNamespace::EnumType enumValue )	\
	{	\
		const char* szEnumType = STRINGIFY( EnumNamespace##_##EnumType );	\
		std::string enumAsString;	\
		switch( enumValue )	\
		{	

#define FRESH_ENUM_STREAM_OUT_CASE( EnumNamespace, EnumCase )	\
		case EnumNamespace::EnumCase:	\
			enumAsString = #EnumCase;	\
			break;

#define FRESH_ENUM_STREAM_OUT_END()	\
		default:	\
			fr::reportInvalidEnumerant( szEnumType, int( enumValue ));\
			break;	\
		}	\
		out << enumAsString;	\
		return out;	\
	}

namespace fr
{
	// Helpful for debugging FRESH_ENUM... macros below.
	inline void reportInvalidEnumerant( const std::string& enumType, int enumValue )
	{
		dev_warning( "While saving enum " << enumType << " to stream: Invalid enumerant value '" << enumValue << "'." );
	}
	
	FRESH_ENUM_STREAM_IN_BEGIN( PropertyAbstract, ControlType )
	FRESH_ENUM_STREAM_IN_CASE( PropertyAbstract::ControlType, String )
	FRESH_ENUM_STREAM_IN_CASE( PropertyAbstract::ControlType, Bool )
	FRESH_ENUM_STREAM_IN_CASE( PropertyAbstract::ControlType, Integer )
	FRESH_ENUM_STREAM_IN_CASE( PropertyAbstract::ControlType, Float )
	FRESH_ENUM_STREAM_IN_CASE( PropertyAbstract::ControlType, Color )
	FRESH_ENUM_STREAM_IN_CASE( PropertyAbstract::ControlType, Angle )
	FRESH_ENUM_STREAM_IN_CASE( PropertyAbstract::ControlType, ObjectRef )
	FRESH_ENUM_STREAM_IN_END()

	FRESH_ENUM_STREAM_OUT_BEGIN( PropertyAbstract, ControlType )
	FRESH_ENUM_STREAM_OUT_CASE( PropertyAbstract::ControlType, String )
	FRESH_ENUM_STREAM_OUT_CASE( PropertyAbstract::ControlType, Bool )
	FRESH_ENUM_STREAM_OUT_CASE( PropertyAbstract::ControlType, Integer )
	FRESH_ENUM_STREAM_OUT_CASE( PropertyAbstract::ControlType, Float )
	FRESH_ENUM_STREAM_OUT_CASE( PropertyAbstract::ControlType, Color )
	FRESH_ENUM_STREAM_OUT_CASE( PropertyAbstract::ControlType, Angle )
	FRESH_ENUM_STREAM_OUT_CASE( PropertyAbstract::ControlType, ObjectRef )
	FRESH_ENUM_STREAM_OUT_END()
}

#endif
