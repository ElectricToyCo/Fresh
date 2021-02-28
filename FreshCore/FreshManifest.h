//
//  FreshManifest.h
//  Fresh
//
//  Created by Jeff Wofford on 8/29/14.
//  Copyright (c) 2014 The Electric Toy Company. All rights reserved.
//

#ifndef Fresh_FreshManifest_h
#define Fresh_FreshManifest_h

#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <memory>
#include <cassert>
#include "FreshXML.h"
#include "FreshPath.h"

#ifdef _MSC_VER
#	pragma warning( push )
#	pragma warning( disable: 4624 )
#endif

namespace fr
{
	class Manifest
	{
	public:
		
		struct Value;
		
		using PropertyAttributes = std::vector< std::string >;
		using Map = std::map< std::string, std::pair< std::shared_ptr< Value >, PropertyAttributes >>;
		using Array = std::vector< std::shared_ptr< Value >>;
		
		struct Object;
		
		struct Value
		{
			enum class Type
			{
				String,
				Map,
				Array,
				Object
			};
			
			explicit Value( std::string&& string );
			explicit Value( const std::string& string );
			explicit Value( std::shared_ptr< Map >&& map );
			explicit Value( const std::shared_ptr< Map >& map );
			explicit Value( std::shared_ptr< Array >&& array );
			explicit Value( const std::shared_ptr< Array >& array );
			explicit Value( std::shared_ptr< Object >&& object );
			explicit Value( const std::shared_ptr< Object >& object );
			
			~Value();
			
			template< typename T >
			bool is() const;
			
			template< typename T >
			const T& get() const;
			
			Type type() const { return m_type; }
			
		private:
			
			Type m_type;
			
			union
			{
				std::string s;
				std::shared_ptr< Map > m;
				std::shared_ptr< Array > a;
				std::shared_ptr< Object > o;
			};
			
			friend std::ostream& operator<<( std::ostream& out, const Value& x );
		};
		
		struct Directive
		{
			enum class Kind
			{
				Const,
				Object,
				Class
			};
			
			Kind kind() const { return m_kind; }
			const std::string& name() const { return m_name; }
			
			template< typename T >
			const T* as() const;
			
		protected:
			explicit Directive( const std::string& n, Kind k ) : m_kind( k ), m_name( n ) {}
			
		private:
			Kind m_kind;
			std::string m_name;
		};
		
		struct Const : public Directive
		{
			static const Kind staticKind = Kind::Const;
			
			std::string type;
			std::string value;
			
			Const( const std::string& n, const std::string& t, const std::string& v )
			: Directive( n, staticKind )
			, type( t )
			, value( v )
			{}
		};
		
		struct Object : public Directive
		{
			static const Kind staticKind = Kind::Object;
			
			std::string className;
			std::shared_ptr< Map > map;
			
			Object( const std::string& n, const std::string& c, std::shared_ptr< Map >&& m )
			: Directive( n, staticKind )
			, className( c )
			, map( std::move( m ))
			{}
		};
		
		struct Class : public Directive
		{
			static const Kind staticKind = Kind::Class;
			
			std::vector< std::string > baseClassNames;
			std::shared_ptr< Map > map;
			
			Class( const std::string& n, std::vector< std::string >&& names, std::shared_ptr< Map >&& m )
			: Directive( n, staticKind )
			, baseClassNames( names )
			, map( std::move( m ))
			{}
		};
		
		Manifest();
		
		void load( const path& path );
		void load( std::istream& in );

		void save( const path& path ) const;
		void save( std::ostream& out ) const;
		
		typedef std::vector< std::shared_ptr< Directive >> Directives;
		
		template< typename FunctionT >
		void eachDirective( FunctionT&& fn ) const
		{
			std::for_each( m_directives.begin(), m_directives.end(), [&]( const Directives::value_type& directive )
						  {
							  fn( *directive );
						  } );
		}
		
		void load( const path& path, Directives& directives );
		void load( std::istream& in, Directives& directives );

		void save( const path& path, const Directives& directives ) const;
		void save( std::ostream& out, const Directives& directives ) const;

		void load( const XmlElement& rootElement );
		void load( const XmlElement& rootElement, Directives& directives );

		// Internal parsing support.
		//
		enum class TokenType
		{
			QuotedString,
			Identifier,
			Include,
			Const,
			Class,
			Object,
			Extends,
			CurlyBraceOpen,
			CurlyBraceClose,
			BracketOpen,
			BracketClose,
			AttributeMarker,
			Newline,
			EndOfFile,
			
			// Non-terminal tokens
			String,
			Map,
			Array,
			Value,
			
			ValueList,
			
			KeyValuePair,
			KeyValuePairList,
			
			Directive,
			Directives,
			IncludeDirective,
			ConstDirective,
			ClassDirective,
			ObjectDirective,
		};

		struct Token
		{
			TokenType type;			
			std::string content;
		};
		
		struct Whitespace
		{
			Manifest& manifest;
			
			explicit Whitespace( Manifest& m ) : manifest( m ) {}
		};
		
	private:
		
		Directives m_directives;

		// Parsing Internals.
		//
		path m_path;
		Whitespace whitespace;
		size_t m_lineNumber = 0;
		size_t m_columnNumber = 0;
		std::unique_ptr< Token > m_peekedToken;
		
		void updateLineAndColumnNumber( int c );
		void readToDelimiter( std::istream& in, std::string& out, const std::string& delimiter );
		void readQuotedString( std::istream& in, std::string& out );
		Token nextToken( std::istream& in, bool skipNewlines = true );
		Token peekToken( std::istream& in, bool skipNewlines = true );
		Token requireToken( std::istream& in, const std::initializer_list< TokenType >& types, bool skipNewlines = true );
		Token requireToken( std::istream& in, TokenType type, bool skipNewlines = true );
		Token string( std::istream& in, bool greedyWhenUnquoted = false );
		Token identifier( std::istream& in );
		std::shared_ptr< Manifest::Map > map( std::istream& in );
		std::shared_ptr< Manifest::Array > array( std::istream& in );
		std::shared_ptr< Manifest::Value > value( std::istream& in, bool propertyValue = false );
		void includeDirective( std::istream& in, Manifest::Directives& directives );
		std::shared_ptr< Manifest::Const > constDirective( std::istream& in );
		std::shared_ptr< Manifest::Class > classDirective( std::istream& in );
		std::shared_ptr< Manifest::Object > objectDirective( std::istream& in );
		
		void fatalError( const std::string& message ) const;
		
		friend std::istream& operator>>( std::istream& in, Whitespace& );
	};
	
	std::ostream& operator<<( std::ostream& out, const Manifest::Directive& x );
	std::ostream& operator<<( std::ostream& out, const Manifest::Const& x );
	std::ostream& operator<<( std::ostream& out, const Manifest::Object& x );
	std::ostream& operator<<( std::ostream& out, const Manifest::Class& x );
	std::ostream& operator<<( std::ostream& out, const Manifest::Array& x );
	std::ostream& operator<<( std::ostream& out, const Manifest::Map& x );
	std::ostream& operator<<( std::ostream& out, Manifest::TokenType type );
	std::ostream& operator<<( std::ostream& out, const std::initializer_list< Manifest::TokenType >& types );

	//////////////////////////////////////
	
	template< typename T >
	const T* Manifest::Directive::as() const
	{
		if( kind() == T::staticKind )
		{
			return static_cast< const T* >( this );
		}
		else
		{
			return nullptr;
		}
	}

	template< typename T >
	Manifest::Value::Type typeForType();
	
	template<>
	inline Manifest::Value::Type typeForType< std::string >()
	{
		return Manifest::Value::Type::String;
	}

	template<>
	inline Manifest::Value::Type typeForType< Manifest::Map >()
	{
		return Manifest::Value::Type::Map;
	}

	template<>
	inline Manifest::Value::Type typeForType< Manifest::Array >()
	{
		return Manifest::Value::Type::Array;
	}

	template<>
	inline Manifest::Value::Type typeForType< Manifest::Object >()
	{
		return Manifest::Value::Type::Object;
	}
	
	template< typename T >
	bool Manifest::Value::is() const
	{
		return m_type == typeForType< T >();
	}
	
	template<>
	inline const std::string& Manifest::Value::get() const
	{
		assert( is< std::string >() );
		return s;
	}
	
	template<>
	inline const Manifest::Map& Manifest::Value::get() const
	{
		assert( is< Manifest::Map >() );
		return *m;
	}
	
	template<>
	inline const Manifest::Array& Manifest::Value::get() const
	{
		assert( is< Manifest::Array >() );
		return *a;
	}
	
	template<>
	inline const Manifest::Object& Manifest::Value::get() const
	{
		assert( is< Manifest::Object >() );
		return *o;
	}
}

#ifdef _MSC_VER
#	pragma warning( pop )
#endif

#endif
