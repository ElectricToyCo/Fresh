///
//  StreamedFunctionCaller.h
//  Fresh
//
//	Classes and templates that enable calling an arbitrary function for which the argument values are read from an istream.
//
//  Created by Jeff Wofford on 2/8/12.
//  Copyright (c) 2012 jeffwofford.com. All rights reserved.
//

#ifndef Fresh_StreamedFunctionCaller_h
#define Fresh_StreamedFunctionCaller_h

#include "FreshDebug.h"
#include <functional>
#include <stdexcept>
#include "escaped_string.h"

namespace fr
{
	
	// Based on http://stackoverflow.com/questions/8476975/call-function-with-parameters-extracted-from-string
	
	template< typename T >
	struct storage_type
	{
		typedef typename std::decay< T >::type type;
	};
	
	template< typename T >
	struct storage_type< T* >
	{
		typedef typename storage_type< T >::type* type;
	};

	template<>
	struct storage_type< std::string >
	{
		typedef escaped_string type;
	};
	
	template< typename F >
	struct stream_function;
	
	template< typename return_t, typename... Args >
	struct stream_function< return_t( Args... ) >
	{
		typedef return_t func_type( Args... );
		typedef std::function< func_type > function;
		static const unsigned int arity = sizeof...( Args );
		
		template< typename F >
		stream_function( F&& f )
		:	m_function( std::forward< F >( f ))
		{}
		
		std::string operator()( std::istream& args ) const
		{
			return callWithStringReturn( args, typename std::is_void< return_t >::type() );
		}
		
		return_t callWithTypedReturn( std::istream& args ) const
		{
			return call( args );
		}
		
	private:
		
		template< typename T >
		static T get( std::istream& args )
		{
			T t = T();
			args >> t;
			return t;
		}
		
		return_t call( std::istream& args ) const
		{
			return return_t( m_function( get< typename storage_type< Args >::type >( args )... ));
		}
		
		std::string callWithStringReturn( std::istream& args, std::false_type ) const		// Non-void return type
		{
			return_t result( call( args ));
			std::ostringstream s;
			s << result;
			return s.str();
		}
		
		std::string callWithStringReturn( std::istream& args, std::true_type ) const		// void return type
		{
			call( args );
			return std::string();
		}
		
		function m_function;
	};
	
	template< typename return_t, typename... Args >
	stream_function< return_t( Args... ) > make_caller( return_t (fn)( Args... ) )
	{
		return stream_function< return_t( Args... ) >( *fn );
	}
	
	template< typename result_t, typename... Args, typename fn_t >
	stream_function< result_t( Args... ) > make_caller( fn_t&& fn )
	{
		return stream_function< result_t( Args... ) >( std::forward< fn_t >( fn ));
	}
	
	//////////////////////////////////////////////////////////////////////////
	
	template< typename object_t, typename F >
	struct stream_method;
	
	template< typename object_t, typename return_t, typename... Args >
	struct stream_method< object_t, return_t( Args... ) >
	{
		typedef return_t func_type( object_t*, Args... );
		typedef std::function< func_type > function;
		static const unsigned int arity = sizeof...( Args );
		
		template< typename F >
		stream_method( F&& f )
		:	m_function( std::forward< F >( f ))
		{}
		
		std::string operator()( object_t* host, std::istream& args ) const
		{
			ASSERT( host );
			return callWithStringReturn( host, args, typename std::is_void< return_t >::type() );
		}
		
		return_t callWithTypedReturn( object_t* host, std::istream& args ) const
		{
			ASSERT( host );
			return call( host, args );
		}
		
	private:
		
		template< typename T >
		static T get( std::istream& args )
		{
			T t = T();
			args >> t;
			return t;
		}
		
		return_t call( object_t* host, std::istream& args ) const
		{
			return return_t( m_function( host, get< typename storage_type< Args >::type >( args )... ));
		}
		
		std::string callWithStringReturn( object_t* host, std::istream& args, std::false_type ) const		// Non-void return type
		{
			return_t result( call( host, args ));
			std::ostringstream s;
			s << result;
			return s.str();
		}
		
		std::string callWithStringReturn( object_t* host, std::istream& args, std::true_type ) const		// void return type
		{
			call( host, args );
			return std::string();
		}
		
		function m_function;
	};
	
	template< typename object_t, typename return_t, typename... Args >
	stream_method< object_t, return_t( Args... ) > make_method_caller( return_t (object_t::*fn)( Args... ) )
	{
		return stream_method< object_t, return_t( Args... ) >( std::mem_fn( fn ));
	}
	
	template< typename object_t, typename return_t, typename... Args, typename fn_t >
	stream_method< object_t, return_t( Args... ) > make_method_caller( fn_t&& fn )
	{
		return stream_method< object_t, return_t( Args... ) >( std::forward< fn_t >( fn ));
	}
}


#endif

