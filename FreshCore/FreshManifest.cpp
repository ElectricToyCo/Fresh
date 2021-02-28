//
//  FreshManifest.cpp
//  Fresh
//
//  Created by Jeff Wofford on 8/29/14.
//  Copyright (c) 2014 The Electric Toy Company. All rights reserved.
//

#include "FreshManifest.h"
#include "IndentingStream.h"
#include "FreshException.h"
#include "FreshFile.h"
#include "FreshDebug.h"
#include <cassert>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <functional>

#define createString( expr ) ([&](){ std::ostringstream s; s << expr; return s.str(); }())
#define FATAL_ERROR( expr ) { fatalError( createString( expr )); }

namespace
{
	using namespace fr;
	
	inline bool eof( int c )
	{
		return c == std::char_traits< char >::eof();
	}
	
	bool needsHereDocument( const std::string& s )
	{
		return std::any_of( s.begin(), s.end(), []( char c )
						   {
							   return c == '\"' || c == '\r' || c == '\n';
						   } );
	}
	
	std::string findHereDocumentTag( const std::string& s )
	{
		std::string tag = "END";
		while( s.find( tag ) != std::string::npos )
		{
			tag += "_END";
		}
		return tag;
	}
	
	IndentingOStreambuf& identingStreamBuf( std::ostream& stream )
	{
		return *static_cast< IndentingOStreambuf* >( stream.rdbuf() );
	}
	
	const std::unordered_map< std::string, Manifest::TokenType > KEYWORDS =
	{
		{ "include", Manifest::TokenType::Include },
		{ "const", Manifest::TokenType::Const },
		{ "class", Manifest::TokenType::Class },
		{ "object", Manifest::TokenType::Object },
		{ "extends", Manifest::TokenType::Extends },
	};
	
//	bool isKeyword( const std::string& identifier )
//	{
//		return KEYWORDS.end() != KEYWORDS.find( identifier );
//	}
	
	inline std::string stringFromChars( const std::initializer_list< char >& chars )
	{
		return std::string{ chars.begin(), chars.end() };
	}
	
	inline std::string stringFromChars( char c )
	{
		return stringFromChars( { c } );
	}
	
#define CHECK_PENDING_IDENTIFIER	\
	if( !contents.empty() )	\
	{	\
		in.putback( c );	\
		return Manifest::Token{ Manifest::TokenType::Identifier, std::move( contents ) };	\
	}
		
}

namespace fr
{
	Manifest::Value::Value( std::string&& string )
	:	m_type( Type::String )
	,	s( std::move( string ))
	{}
	
	Manifest::Value::Value( const std::string& string )
	:	m_type( Type::String )
	,	s( string )
	{}
	
	Manifest::Value::Value( std::shared_ptr< Map >&& map )
	:	m_type( Type::Map )
	,	m( std::move( map ))
	{}
	
	Manifest::Value::Value( const std::shared_ptr< Map >& map )
	:	m_type( Type::Map )
	,	m( map )
	{}
	
	Manifest::Value::Value( std::shared_ptr< Array >&& array )
	:	m_type( Type::Array )
	,	a( std::move( array ))
	{}
	
	Manifest::Value::Value( const std::shared_ptr< Array >& array )
	:	m_type( Type::Array )
	,	a( array )
	{}
	
	Manifest::Value::Value( std::shared_ptr< Object >&& object )
	:	m_type( Type::Object )
	,	o( std::move( object ))
	{}
	
	Manifest::Value::Value( const std::shared_ptr< Object >& object )
	:	m_type( Type::Object )
	,	o( object )
	{}
	
	Manifest::Value::~Value()
	{}
	
	///////////////////////////////////////////////////////////////////////////////
	
	Manifest::Manifest()
	:	whitespace( *this )
	{}
	
	void Manifest::load( const path& path )
	{
		load( path, m_directives );
	}
	
	void Manifest::load( std::istream& in )
	{
		load( in, m_directives );
	}
	
	void Manifest::load( const XmlElement& rootElement )
	{
		return load( rootElement, m_directives );
	}
	
	void Manifest::save( const path& path ) const
	{
		save( path, m_directives );
	}
	
	void Manifest::save( std::ostream& out ) const
	{
		save( out, m_directives );
	}
	
	void Manifest::load( const path& path, Directives& directives )
	{
		std::ifstream in( getResourcePath( path ).c_str() );
		
		if( !in )
		{
			FATAL_ERROR( "Could not open " << path << "." );
		}
		
		m_path = path;
		m_lineNumber = m_columnNumber = 0;
		load( in, directives );
		
		m_path.clear();
	}
	
	void Manifest::load( std::istream& in, Directives& directives )
	{
		while( in )
		{
			auto token = nextToken( in );
			
			switch( token.type )
			{
				case TokenType::Include:
					includeDirective( in, directives );
					break;
				case TokenType::Const:
					directives.push_back( constDirective( in ));
					break;
				case TokenType::Class:
					directives.push_back( classDirective( in ));
					break;
				case TokenType::Object:
					directives.push_back( objectDirective( in ));
					break;
				case TokenType::EndOfFile:
					break;
				default:
					FATAL_ERROR( "Unexpected token "
								<< token.type
								<< " ("
								<< token.content
								<< ") while looking for directive." );
					break;
			}
		}
		
		m_peekedToken.reset();
	}
	
	void Manifest::save( const path& path, const Directives& directives ) const
	{
		std::ofstream out( path.c_str() );
		
		if( !out )
		{
			FATAL_ERROR( "Could not open " << path << "." );
		}
		
		save( out, directives );
	}
	
	void Manifest::save( std::ostream& out, const Directives& directives ) const
	{
		// Amend the ostream to use an indenting buffer.
		//
		IndentingOStreambuf streamBuf( out );
		
		for( const auto& directive : directives )
		{
			out << *directive;
		}
	}
	
	void Manifest::updateLineAndColumnNumber( int c )
	{
		if( c == '\n' )
		{
			++m_lineNumber;
			m_columnNumber = 0;
		}
		else if( !eof( c ))
		{
			++m_columnNumber;
		}
	}
	
	void Manifest::readToDelimiter( std::istream& in, std::string& out, const std::string& delimiter )
	{
		size_t firstMatchingChar = ~0UL;
		std::string::const_iterator delimiterCharToMatch = delimiter.begin();
		
		while( in )
		{
			const auto c = in.get();
			
			if( eof( c ))
			{
				break;
			}
			
			updateLineAndColumnNumber( c );
			
			out += c;
			
			if( c == *delimiterCharToMatch )
			{
				// Just started matching?
				//
				if( firstMatchingChar >= out.size() )
				{
					assert( out.size() > 0 );
					firstMatchingChar = out.size() - 1;
				}
				
				++delimiterCharToMatch;
				
				if( delimiterCharToMatch == delimiter.end() )
				{
					// Matched the whole delimiter.
					
					// Remove the delimiter text from the out text.
					//
					out.erase( out.begin() + firstMatchingChar, out.end() );
					break;
				}
			}
			else
			{
				delimiterCharToMatch = delimiter.begin();
				firstMatchingChar = -1;
			}
		}
	}
	
	void Manifest::readQuotedString( std::istream& in, std::string& out )
	{
		bool escaped = false;
		
		while( in )
		{
			auto c = in.get();
			
			updateLineAndColumnNumber( c );
			
			if( eof( c ))
			{
				FRESH_THROW( FreshException, "Quoted string '" << out << "' never terminated." );
			}
			else if( escaped )
			{
				escaped = false;
				
				switch( c )
				{
					case 'n': c = '\n'; break;
					case 't': c = '\t'; break;
				}
			}
			else if( c == '\\' )
			{
				escaped = true;
				continue;
			}
			else if( c == '\"' )
			{
				break;
			}
			
			out += c;
		}
	}
	
	Manifest::Token Manifest::nextToken( std::istream& in, bool skipNewlines )
	{
		if( m_peekedToken )
		{
			Token result = *m_peekedToken;
			m_peekedToken.reset();
			return result;
		}
		
		std::string contents;
		
		while( in )
		{
			const char c = in.get();
			const char lookahead = in.peek();
			
			if( eof( c ))
			{
				CHECK_PENDING_IDENTIFIER
				
				assert( contents.empty() );
				return Token{ TokenType::EndOfFile };
			}
			else if( c == '{' )
			{
				CHECK_PENDING_IDENTIFIER
				
				assert( contents.empty() );
				return Token{ TokenType::CurlyBraceOpen, stringFromChars( c ) };
			}
			else if( c == '}' )
			{
				CHECK_PENDING_IDENTIFIER
				
				assert( contents.empty() );
				return Token{ TokenType::CurlyBraceClose, stringFromChars( c ) };
			}
			else if( c == '[' )
			{
				CHECK_PENDING_IDENTIFIER
				
				assert( contents.empty() );
				return Token{ TokenType::BracketOpen, stringFromChars( c ) };
			}
			else if( c == ']' )
			{
				CHECK_PENDING_IDENTIFIER
				
				assert( contents.empty() );
				return Token{ TokenType::BracketClose, stringFromChars( c ) };
			}
			else if( c == '@' )
			{
				return Token{ TokenType::AttributeMarker, stringFromChars( c ) };
			}
			else if( c == '/' && ( lookahead == '/' || lookahead == '*' ))
			{
				if( lookahead == '/' )
				{
					CHECK_PENDING_IDENTIFIER
					
					// C++-style comment
					//
					in.get();
					
					// Read to the end of the line.
					//
					std::getline( in, contents );
					contents.clear();
					
					++m_lineNumber;
					m_columnNumber = 0;
					
					if( !skipNewlines )
					{
						return Token{ TokenType::Newline };
					}
					else
					{
						continue;
					}
				}
				else
				{
					ASSERT( lookahead == '*' );
					
					CHECK_PENDING_IDENTIFIER
					
					/* C-style comment */
					//
					in.get();
					
					readToDelimiter( in, contents, "*/" );
					contents.clear();
					continue;
				}
			}
			else if( c == '\"' )
			{
				CHECK_PENDING_IDENTIFIER
				
				// Explicit start of string.
				//
				assert( contents.empty() );
				readQuotedString( in, contents );
				return Token{ TokenType::QuotedString, std::move( contents ) };
			}
			else if( c == '<' && lookahead == '<' )
			{
				CHECK_PENDING_IDENTIFIER
				
				// Here document (multiline string).
				
				assert( contents.empty() );
				
				in.get();
				
				std::string concluder;
				in >> whitespace >> concluder >> whitespace;
				
				if( in.eof() || concluder.empty() )
				{
					FRESH_THROW( FreshException, "Here document (<< ...) lacked terminator." );
				}
				else
				{
					readToDelimiter( in, contents, concluder );
					
					if( in.eof() )
					{
						FRESH_THROW( FreshException, "Here document (<< " << concluder << " ...) was not terminated with '" << concluder << "'." );
					}
					
					return Token{ TokenType::QuotedString, std::move( contents ) };
				}
			}
			else if( ::isspace( c ))
			{
				// Got any content yet? If so, return it as an identifier or keyword. Else skip it.
				
				if( !contents.empty() )
				{
					// Check for keywords.
					//
					auto iter = KEYWORDS.find( contents );
					if( iter != KEYWORDS.end() )
					{
						in.putback( c );
						return Token{ iter->second, std::move( contents ) };
					}
					
					CHECK_PENDING_IDENTIFIER
				}
				
				updateLineAndColumnNumber( c );
				
				// Newline?
				if( !skipNewlines && ( c == '\n' || c == '\r' ))
				{
					// Read through to the next non-space character.
					in >> whitespace;
					return Token{ TokenType::Newline };
				}
				else
				{
					continue;
				}
			}
			
			updateLineAndColumnNumber( c );
			contents += c;
		}

		assert( false );
		return Token{ TokenType::EndOfFile };
	}
	
	Manifest::Token Manifest::peekToken( std::istream& in, bool skipNewlines )
	{
		if( m_peekedToken )
		{
			return *m_peekedToken;
		}
		else
		{
			auto token = nextToken( in, skipNewlines );
			
			m_peekedToken.reset( new Token( token ) );
			return *m_peekedToken;
		}
	}
	
	Manifest::Token Manifest::requireToken( std::istream& in, const std::initializer_list< TokenType >& types, bool skipNewlines )
	{
		skipNewlines = skipNewlines && types.end() == std::find( types.begin(), types.end(), TokenType::Newline );
		
		auto token = nextToken( in, skipNewlines );
		
		if( std::find( types.begin(), types.end(), token.type ) == types.end() )
		{
			FATAL_ERROR( "You had \"" << token.content << "\" (" << token.type << ") where " << types << " was expected." );
		}
		
		return token;
	}
	
	Manifest::Token Manifest::requireToken( std::istream& in, TokenType type, bool skipNewlines )
	{
		return requireToken( in, { type }, skipNewlines );
	}
	
	Manifest::Token Manifest::string( std::istream& in, bool greedyWhenUnquoted )
	{
		auto token = nextToken( in );
		
		switch( token.type )
		{
			case TokenType::Identifier:
				if( greedyWhenUnquoted )
				{
					readToDelimiter( in, token.content, "\n" );
					in.putback( '\n' );
				}
				return Token{ TokenType::String, std::move( token.content ) };
				
			case TokenType::QuotedString:
				return Token{ TokenType::String, std::move( token.content ) };
				
			default:
				FATAL_ERROR( "Unexpected token " << token.type << ". Expected Identifier or QuotedString." );
				return token;
		}
	}
	
	Manifest::Token Manifest::identifier( std::istream& in )
	{
		auto token = nextToken( in );
		
		switch( token.type )
		{
			case TokenType::Identifier:
				return token;
				
			default:
				FATAL_ERROR( "Unexpected token " << token.type << ". Expected Identifier." );
				return token;
		}
	}
	
	std::shared_ptr< Manifest::Map > Manifest::map( std::istream& in )
	{
		auto map = std::make_shared< Manifest::Map >();
		
		auto token = peekToken( in );
		
		std::vector< std::string > pendingPropertyAttributes;
		
		while( token.type != TokenType::CurlyBraceClose )
		{
			if( token.type == TokenType::AttributeMarker )
			{
				token = nextToken( in );

				auto propertyAttribute = string( in );
				pendingPropertyAttributes.push_back( propertyAttribute.content );
				
				continue;
			}
			
			auto key = string( in );

			(*map)[ key.content ] = std::make_pair( value( in, true /* property value */ ), std::move( pendingPropertyAttributes ));
			
			token = peekToken( in, false );
			if( token.type == TokenType::CurlyBraceClose )
			{
				break;
			}
			else if( token.type == TokenType::Newline )
			{
				// Skip the newline.
				//
				token = nextToken( in );
				token = peekToken( in );
			}
			else
			{
				FATAL_ERROR( "Map value should be followed by a newline. Was followed by '" << token.content << "'." );
			}
		}
		
		requireToken( in, TokenType::CurlyBraceClose );
		
		return map;
	}
	
	std::shared_ptr< Manifest::Array > Manifest::array( std::istream& in )
	{
		auto array = std::make_shared< Manifest::Array >();
		
		auto token = peekToken( in );
		
		while( token.type != TokenType::BracketClose )
		{
			array->push_back( value( in ));
			
			token = peekToken( in );
		}
		
		requireToken( in, TokenType::BracketClose );
		
		return array;
	}
	
	std::shared_ptr< Manifest::Value > Manifest::value( std::istream& in, bool propertyValue )
	{
		auto token = peekToken( in );
		
		switch( token.type )
		{
			case TokenType::CurlyBraceOpen:
			{
				nextToken( in );
				return std::shared_ptr< Manifest::Value >{ new Manifest::Value{ map( in ) }};
			}
				
			case TokenType::Identifier:
			case TokenType::QuotedString:
				return std::shared_ptr< Manifest::Value >{ new Manifest::Value{ string( in, propertyValue /* greedy */ ).content }};
				
			case TokenType::BracketOpen:
			{
				nextToken( in );
				return std::shared_ptr< Manifest::Value >{ new Manifest::Value{ array( in ) }};
			}
				
			case TokenType::Object:
			{
				nextToken( in );
				return std::shared_ptr< Manifest::Value >{ new Manifest::Value{ objectDirective( in ) }};
			}
				
			default:
				FATAL_ERROR( "Unexpected token " << token.type << " while seeking value." );
				return nullptr;
		}
	}
	
	void Manifest::includeDirective( std::istream& in, Manifest::Directives& directives )
	{
		auto path = string( in );
		
		requireToken( in, { TokenType::Newline, TokenType::EndOfFile } );
		
		fr::Manifest subManifest;
		subManifest.load( path.content, directives );
	}
	
	std::shared_ptr< Manifest::Const > Manifest::constDirective( std::istream& in )
	{
		auto constType = requireToken( in, TokenType::Identifier );
		
		auto constName = string( in );
		
		auto constValue = string( in );
		
		requireToken( in, { TokenType::Newline, TokenType::EndOfFile } );
		
		return std::shared_ptr< Manifest::Const >( new Manifest::Const{ constName.content,
			constType.content,
			constValue.content } );
	}
	
	std::shared_ptr< Manifest::Class > Manifest::classDirective( std::istream& in )
	{
		auto className = requireToken( in, TokenType::Identifier );
		
		requireToken( in, TokenType::Extends );
		
		std::vector< std::string > baseClasses;
		
		Token token;
		while( true )
		{
			token = nextToken( in );
			
			if( token.type == TokenType::Identifier )
			{
				baseClasses.push_back( std::move( token.content ));
			}
			else
			{
				if( baseClasses.empty() )
				{
					FATAL_ERROR( "Unexpected token " << token.type << " while looking for base class identifier(s)." );
				}
				break;
			}
		}
		
		if( token.type == TokenType::CurlyBraceOpen )
		{
			auto classMap = map( in );
			
			requireToken( in, { TokenType::Newline, TokenType::EndOfFile } );
			
			return std::shared_ptr< Manifest::Class >( new Manifest::Class{ className.content,
				std::move( baseClasses ),
				std::move( classMap ) } );
		}
		else
		{
			FATAL_ERROR( "Unexpected token " << token.type << " while looking for '{'." );
			return nullptr;
		}
	}
	
	std::shared_ptr< Manifest::Object > Manifest::objectDirective( std::istream& in )
	{
		auto className = requireToken( in, TokenType::Identifier );
		
		// Optional object name.
		//
		Token objectName{ TokenType::String };
		
		auto token = peekToken( in );
		if( token.type != TokenType::CurlyBraceOpen )
		{
			objectName = string( in );
		}
		
		requireToken( in, TokenType::CurlyBraceOpen );
		
		auto objectDirective = std::make_shared< Manifest::Object >( objectName.content,
			className.content,
			map( in ));
		
		return objectDirective;
	}
	
	void Manifest::fatalError( const std::string& message ) const
	{
		std::ostringstream collector;
		collector << "(" << m_path.filename()
			<< ( m_path.empty() ? "" : ":" )
			<< ( m_lineNumber + 1 ) << ">"
			<< ( m_columnNumber + 1 ) << ") "
			<< message;
		FRESH_THROW( FreshException, collector.str() );
	}
	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	std::istream& operator>>( std::istream& in, Manifest::Whitespace& whitespace )
	{
		while( in )
		{
			const char c = in.peek();
			if( ::isspace( c ))
			{
				in.get();
				whitespace.manifest.updateLineAndColumnNumber( c );
			}
			else
			{
				break;
			}
		}
		return in;
	}
	
	std::ostream& operator<<( std::ostream& out, const Manifest::Directive& x )
	{
		switch( x.kind() )
		{
			case Manifest::Directive::Kind::Const:
				return out << static_cast< const Manifest::Const& >( x );
			case Manifest::Directive::Kind::Object:
				return out << static_cast< const Manifest::Object& >( x );
			case Manifest::Directive::Kind::Class:
				return out << static_cast< const Manifest::Class& >( x );
		}
		return out;
	}
	
	std::ostream& operator<<( std::ostream& out, const Manifest::Const& x )
	{
		return out << x.type << " " << x.name() << " " << x.value << std::endl;
	}
	
	std::ostream& operator<<( std::ostream& out, const Manifest::Object& x )
	{
		return out << "object " << x.className << " " << ( x.name().empty() ? "" : x.name() + " " ) << *x.map
			<< std::endl;
	}
	
	std::ostream& operator<<( std::ostream& out, const Manifest::Class& x )
	{
		out << "class " << x.name() << " extends";
		
		for( const auto& super : x.baseClassNames )
		{
			out << " " << super;
		}
		
		out << "\n" << *x.map << std::endl;
		
		return out;
	}
	
	std::ostream& operator<<( std::ostream& out, const Manifest::Value& x )
	{
		switch( x.type() )
		{
			case Manifest::Value::Type::String:
				
				// If the string involves newlines or double quotes, use a "here document".
				if( needsHereDocument( x.s ))
				{
					const auto tag = findHereDocumentTag( x.s );
					out << "<< " << tag;
					auto& streamBuf = identingStreamBuf( out );
					const auto indents = streamBuf.indents();
					streamBuf.setIndent( 0 );
					out << "\n" << x.s << tag;
					streamBuf.setIndent( indents );
				}
				else
				{
					out << "\"" << x.s << "\"";
				}
				break;
			case Manifest::Value::Type::Map:
				out << *x.m;
				break;
			case Manifest::Value::Type::Array:
				out << *x.a;
				break;
			case Manifest::Value::Type::Object:
				out << *x.o;
				break;
		}
		return out;
	}
	
	std::ostream& operator<<( std::ostream& out, const Manifest::Array& arr )
	{
		out << "[";
		
		if( !arr.empty() )
		{
			auto& streamBuf = identingStreamBuf( out );
			streamBuf.indent();
			for( const auto& val : arr )
			{
				out << "\n" << *val;
			}
			streamBuf.unindent();
		}
		out << "\n]";
		return out;
	}
	
	std::ostream& operator<<( std::ostream& out, const Manifest::Map& map )
	{
		out << "{";
		if( !map.empty() )
		{
			auto& streamBuf = identingStreamBuf( out );
			streamBuf.indent();
			for( const auto& pair : map )
			{
				out << "\n" << pair.first << " ";
				for( const auto& attribute : pair.second.second )
				{
					out << "@" << attribute << " ";
				}
				out << *pair.second.first;
			}
			streamBuf.unindent();
		}
		out << "\n}";
		return out;
	}
	
#define CASE( type ) case Manifest::TokenType::type : out << #type; break;
	std::ostream& operator<<( std::ostream& out, Manifest::TokenType type )
	{
		switch( type )
		{
				CASE( QuotedString )
				CASE( Identifier )
				CASE( Include )
				CASE( Const )
				CASE( Class )
				CASE( Object )
				CASE( Extends )
				CASE( CurlyBraceOpen )
				CASE( CurlyBraceClose )
				CASE( BracketOpen )
				CASE( BracketClose )
				CASE( Newline )
				CASE( EndOfFile )
				CASE( String )
				CASE( Map )
				CASE( Array )
				CASE( Value )
				CASE( ValueList )
				CASE( KeyValuePair )
				CASE( KeyValuePairList )
				CASE( Directive )
				CASE( Directives )
				CASE( IncludeDirective )
				CASE( ConstDirective )
				CASE( ClassDirective )
				CASE( ObjectDirective )
			default: assert( false ); break;
		}
		
		return out;
	}

	std::ostream& operator<<( std::ostream& out, const std::initializer_list< Manifest::TokenType >& types )
	{
		bool first = true;
		for( const auto& type : types )
		{
			if( !first ) out << "|";
			out << type;
			first = false;
		}
		
		return out;
	}
}
