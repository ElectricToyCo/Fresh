//
//  LuaCppInterop.hpp
//
//  Created by Jeff Wofford on 6/25/18.
//

#ifndef LuaCppInterop_hpp
#define LuaCppInterop_hpp

#include <tuple>
#include <utility>
#include <string>
#include <vector>

namespace luacpp
{
	inline std::vector< std::string > split( const std::string& str, const std::string& delims )
	{
		std::vector< std::string > container;

		std::size_t current, previous = 0;
		current = str.find_first_of( delims );
		while( current != std::string::npos )
		{
			container.push_back( str.substr(previous, current - previous ));
			previous = current + 1;
			current = str.find_first_of(delims, previous);
		}
		container.push_back( str.substr( previous, current - previous ));

		return container;
	}

	template< typename IterT >
	inline std::string join( IterT begin, IterT end, const std::string& connector = ", " )
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
	
	using uint = unsigned int;
	
	template< typename T >
	struct LuaDefault
	{
		static constexpr T value = {};
	};
	
	template<> struct LuaDefault< int > { static constexpr int value  = 0xEFFFFFFF; };
	template<> struct LuaDefault< uint > { static constexpr uint value = 0xEFFFFFFF; };
	template<> struct LuaDefault< float > { static constexpr float value = -1.248e9f; };
	template<> struct LuaDefault< double > { static constexpr double value = -1.248e9; };
	template<> struct LuaDefault< std::string > { static const std::string value; };
	
	template< typename T >
	inline bool isDefault( const T& arg )
	{
		return arg == luacpp::LuaDefault< typename std::decay< T >::type >::value;
	}
	
#	define DEFAULT( arg, defaultValue ) if( luacpp::isDefault( arg )) { arg = (defaultValue); }

	template< typename T > bool luaMayTakeExpectedType( lua_State* lua );
	template<> inline bool luaMayTakeExpectedType< bool >( lua_State* lua ) { return lua_type( lua, -1 ) == LUA_TBOOLEAN || lua_isinteger( lua, -1 ); }
	template<> inline bool luaMayTakeExpectedType< int >( lua_State* lua ) { return lua_isinteger( lua, -1 ); }
	template<> inline bool luaMayTakeExpectedType< float >( lua_State* lua ) { return lua_isinteger( lua, -1 ) || lua_isnumber( lua, -1 ); }
	template<> inline bool luaMayTakeExpectedType< double >( lua_State* lua ) { return lua_isinteger( lua, -1 ) || lua_isnumber( lua, -1 ); }
	template<> inline bool luaMayTakeExpectedType< std::string >( lua_State* lua ) { return lua_isstring( lua, -1 ); }

	template< typename T > T luaTakeResult( lua_State* lua );
	template<> inline bool luaTakeResult( lua_State* lua ) { return lua_toboolean( lua, -1 ) != 0; }
	template<> inline int luaTakeResult( lua_State* lua ) { return static_cast< int >( lua_tointeger( lua, -1 )); }
	template<> inline float luaTakeResult( lua_State* lua ) { return static_cast< float >( lua_tonumber( lua, -1 )); }
	template<> inline double luaTakeResult( lua_State* lua ) { return static_cast< double >( lua_tonumber( lua, -1 )); }
	template<> inline std::string luaTakeResult( lua_State* lua ) { return lua_tostring( lua, -1 ); }

	template< typename T > struct LuaReturnType 
	{ 
		static const int count = 1; 
		static T fetchResult( lua_State* lua )
		{
			const auto result = luaTakeResult< typename std::decay< T >::type >( lua );
			lua_pop( lua, 1 );
			return result;
		}
	};
	template<> struct LuaReturnType< void > 
	{ 
		static const int count = 0;
		static void fetchResult( lua_State* lua )
		{
			return;
		}
	};
    template< typename... Elems > struct LuaReturnType< std::tuple< Elems... >>
    {
        static const int count = 0;
        static std::tuple< Elems... > fetchResult( lua_State* lua )
        {
            return { LuaReturnType< Elems... >::fetchResult( lua ) };
        }
    };

	template< typename T >
	int luaPushValue( lua_State* lua, T value );
	
	template<>
	inline int luaPushValue( lua_State* lua, float value )
	{
		lua_pushnumber( lua, static_cast< lua_Number >( value ));
		return 1;
	}	

	template<>
	inline int luaPushValue( lua_State* lua, double value )
	{
		lua_pushnumber( lua, static_cast< lua_Number >( value ));
		return 1;
	}	
	
	template<>
	inline int luaPushValue( lua_State* lua, int value )
	{
		lua_pushinteger( lua, static_cast< lua_Integer >( value ));
		return 1;
	}	

	template<>
	inline int luaPushValue( lua_State* lua, unsigned long value )
	{
		lua_pushinteger( lua, static_cast< lua_Integer >( value ));
		return 1;
	}

	template<>
	inline int luaPushValue( lua_State* lua, std::string value )
	{
		lua_pushstring( lua, value.c_str() );
		return 1;
	}	
	
	template<>
	inline int luaPushValue( lua_State* lua, const char* value )
	{
		lua_pushstring( lua, value );
		return 1;
	}	
	
	template<>
	inline int luaPushValue( lua_State* lua, bool value )
	{
		lua_pushboolean( lua, value );
		return 1;
	}

    template< typename... Args >
    int marshallArguments( lua_State* lua, Args... args );
    
    template<>
    inline int marshallArguments<>( lua_State* lua )
    {
        return 0;
    }
    
    template< typename FirstArg, typename... Args >
    inline int marshallArguments( lua_State* lua, const FirstArg& arg, Args... args )
    {
        luaPushValue( lua, arg );
        return 1 + marshallArguments( lua, args... );
    }
    
    // luaPushValue< std::tuple > support

    template< typename... Elements >
    int marshallTableElement( lua_State* lua, size_t index, Elements... element );
    
    template<>
    inline int marshallTableElement<>( lua_State* lua, size_t index )
    {
        return 0;
    }
    
    template< typename FirstElement, typename... Elements >
    inline int marshallTableElement( lua_State* lua, size_t index, const FirstElement& element, Elements... elements )
    {
		lua_pushinteger( lua, index );
        luaPushValue( lua, element );
		lua_settable( lua, -3 );
        return 1 + marshallTableElement( lua, index + 1, elements... );
    }

    template< typename... Elems, size_t... Is >
    inline int luaPushTupleAsTable( lua_State* lua, const std::tuple< Elems... >& value, std::index_sequence<Is...> )
    {
        lua_newtable( lua );
        marshallTableElement( lua, 1, std::get< Is >( value )... );
		return 1;
    }
	
    template< typename... Elems, size_t... Is >
    inline int luaPushTupleAsSequence( lua_State* lua, const std::tuple< Elems... >& value, std::index_sequence<Is...> )
    {
        return marshallArguments( lua, std::get< Is >( value )... );
    }
	
    template< typename... Elems >
    inline int luaPushValue( lua_State* lua, const std::tuple< Elems... >& value )
    {
		// Push the tuple as a Lua table. We could alternatively push it as a "sequence".
		//
		// `luaPushTupleAsSequence()` works well for C++ functions returning multiple values as a tuple:
		// they sit directly on the stack, unindexed, and Lua can decompose the results pleasantly (`var x, y, z = fn()`).
		// But this doesn't work for when tuples are members of vectors, for example.
		// Here, since we push tuples as tables, in Lua we must use `var x, y, z = table.unpack( fn() )` for decomposition.
		//
        return luaPushTupleAsTable( lua, value, std::make_index_sequence< std::tuple_size< std::tuple< Elems... > >::value >{} );
    }
    
    // luaPushValue< std::map > support
    
    template< typename Key, typename Value >
    inline int luaPushValue( lua_State* lua, const std::map< Key, Value >& map )
    {
        lua_newtable( lua );
        for( const auto& pair : map )
        {
            luaPushValue( lua, pair.first );
            luaPushValue( lua, pair.second );
            lua_settable( lua, -3 );
        }
        return 1;
    }
    
    // luaPushValue< std::vector > support
    
    template< typename Value >
    inline int luaPushValue( lua_State* lua, const std::vector< Value >& vector )
    {
        lua_newtable( lua );
        for( size_t i = 0; i < vector.size(); ++i )
        {
            lua_pushinteger( lua, i + 1 );
            luaPushValue( lua, vector[ i ] );
            lua_settable( lua, -3 );
        }
        return 1;
    }
    
    // luaGetRequiredArgument
    
	template< typename T >
	T luaGetRequiredArgument( lua_State* lua, size_t argIndex );
	
	template<>
	inline int luaGetRequiredArgument( lua_State* lua, size_t argIndex )
	{
		return static_cast< int >( luaL_checkinteger( lua, (int) argIndex ));
	}	
	
	template<>
	inline uint luaGetRequiredArgument( lua_State* lua, size_t argIndex )
	{
		return static_cast< uint >( luaL_checkinteger( lua, (int) argIndex ));
	}	
	
	template<>
	inline float luaGetRequiredArgument( lua_State* lua, size_t argIndex )
	{
		return static_cast< float >( luaL_checknumber( lua, (int) argIndex ));
	}	
	
	template<>
	inline double luaGetRequiredArgument( lua_State* lua, size_t argIndex )
	{
		return static_cast< double >( luaL_checknumber( lua, (int) argIndex ));
	}	
	
	template<>
	inline std::string luaGetRequiredArgument( lua_State* lua, size_t argIndex )
	{
		return luaL_checkstring( lua, (int) argIndex );
	}	
	
	template<>
	inline bool luaGetRequiredArgument( lua_State* lua, size_t argIndex )
	{
		return lua_toboolean( lua, (int) argIndex );
	}	
	
	template< typename T >
	inline T luaGetArgument( lua_State* lua, size_t& argIndex, bool required )
	{
		++argIndex;
		
		return ( !required && lua_isnoneornil( lua, (int) argIndex ) ) 
		? LuaDefault< T >::value 
		: luaGetRequiredArgument< T >( lua, argIndex );
	}
	
	struct LuaHostRegistry
	{
		static LUACPP_HOST_TYPE_PTR hostForState( lua_State* state )
		{
			return luaHosts()[ state ];
		}
		
		static void addHost( lua_State* state, LUACPP_HOST_TYPE_PTR host )
		{
			ASSERT( state );
			ASSERT( host );
			luaHosts()[ state ] = host;
		}
		
		static void removeHost( lua_State* lua )
		{
			const auto iter = luaHosts().find( lua );
			if( iter != luaHosts().end() )
			{
				luaHosts().erase( lua );
			}
		}
		
	private:
		using LuaHostMap = std::unordered_map< lua_State*, LUACPP_HOST_TYPE_PTR >;
		static LuaHostMap& luaHosts()
		{
			static LuaHostMap luaHostMap;
			return luaHostMap;
		}
	};
	
	template< typename ReturnT, typename... Args >
	struct LuaCallerBase
	{
		typedef ReturnT (LUACPP_HOST_TYPE::*func_type)( Args... );
		
		template< typename F >
		LuaCallerBase( F f, size_t numRequiredArguments )
		:	m_function( f )
		,	m_numRequiredArguments( numRequiredArguments )
		{}
		
		static void removeHost( lua_State* state )
		{
			ASSERT( state );
		}
		
	protected:
		
		ReturnT call( lua_State* lua ) const
		{
			auto host = LuaHostRegistry::hostForState( lua );
			ASSERT( host );
			size_t argIndex = 0;
			return ((*host).*m_function)( static_cast< Args >( luaGetArgument< typename std::decay< Args >::type >( lua, argIndex, argIndex < m_numRequiredArguments ))... );
		}
		
		func_type m_function;
		size_t m_numRequiredArguments;
	};
	
	template< typename ReturnT, typename... Args >
	struct LuaCaller : public LuaCallerBase< ReturnT, Args... >
	{
		using Super = LuaCallerBase< ReturnT, Args... >;
		using Super::Super;
		
		int operator()( lua_State* lua ) const
		{			
			return luaPushValue( lua, Super::call( lua ) );
		}				
	};
	
	template< typename... Args >
	struct LuaCaller< void, Args... > : public LuaCallerBase< void, Args... >
	{
		using Super = LuaCallerBase< void, Args... >;
		using Super::Super;
		
		int operator()( lua_State* lua ) const
		{
			Super::call( lua );
			return 0;
		}
	};
	
	template< typename ReturnT, typename... Args>
	int callCppFromLua( lua_State* lua, ReturnT (LUACPP_HOST_TYPE::*fn)( Args... ), size_t numRequiredArguments )
	{
		ASSERT( lua );
		LuaCaller< ReturnT, Args... > caller{ fn, numRequiredArguments };
		return caller( lua );
	}
	
	////////////// Function registration
	
	struct LuaFunctionRegisterer
	{
		LuaFunctionRegisterer( const std::string& name, lua_CFunction function )
		{
			functions()[ name ] = function;
		}
		
		static void registerAllFunctionsWithState( lua_State* lua )
		{
			for( const auto& pair : functions() )
			{
				lua_register( lua, pair.first.c_str(), pair.second );
			}
		}
		
	private:
		
		using FunctionMap = std::unordered_map< std::string, lua_CFunction >;
		static FunctionMap& functions()
		{
			static FunctionMap functions;
			return functions;
		}
	};

	inline bool doesLuaFunctionExist( lua_State* lua, const std::string& fnName )
	{
		lua_getglobal( lua, fnName.c_str() );
		return lua_isfunction( lua, lua_gettop( lua ));
	}

	template< typename ReturnT, typename... Args >
	inline ReturnT callLua( lua_State* lua, const std::string& fnName, Args... args )
	{
		ASSERT( lua );
		
		lua_getglobal( lua, fnName.c_str() );
		
		// Marshall arguments to be passed to lua.
		//
		const int argCount = marshallArguments( lua, args... );
		
		// Call the function.
		//
		const auto result = lua_pcall( lua, argCount, LuaReturnType< ReturnT >::count, 0 );
		if( result != LUA_OK )
		{			
			const auto errorMessage = LuaReturnType< std::string >::fetchResult( lua );
			throw std::runtime_error( errorMessage );
		}
		
		// Fetch the result, if any.
		return LuaReturnType< ReturnT >::fetchResult( lua );
	}

	inline std::string luaTypeName( int type )
	{
		switch( type )
		{
			case LUA_TNIL: return "NIL";
			case LUA_TBOOLEAN: return "BOOLEAN";
			case LUA_TLIGHTUSERDATA: return "LIGHTUSERDATA";
			case LUA_TNUMBER: return "NUMBER";
			case LUA_TSTRING: return "STRING";
			case LUA_TTABLE: return "TABLE";
			case LUA_TFUNCTION: return "FUNCTION";
			case LUA_TUSERDATA: return "USERDATA";
			case LUA_TTHREAD: return "THREAD";
			default: return "(Unknown)";
		}
	}
	
	template< typename IterT >
	inline void traversePath( lua_State* L, IterT begin, IterT end )
	{
		ASSERT( begin != end );
		
		for( auto iter = begin; iter != end; ++iter )
		{
			if( iter == begin )
			{
				lua_getglobal( L, (*iter).c_str() );
			}
			else
			{
				const auto type = lua_type( L, -1 );
				if( type == LUA_TTABLE )
				{
					lua_pushstring( L, (*iter).c_str() );
					lua_gettable( L, -2 );
				}
				else
				{
					// Not a table data type. Therefore we should be at the end of our path.
					if( iter + 1 != end )
					{
						throw std::runtime_error( createString( join( begin, iter, "." ) << " is not a table, but a " << luaTypeName( type ) << "." ));
					}
				}
			}
		}
	}
	
	template< typename T >
	T getValue( lua_State* L, const std::vector< std::string >& path )
	{
		ASSERT( path.empty() == false );
		
		traversePath( L, path.begin(), path.end() );
		
		if( luaMayTakeExpectedType< T >( L ))
		{
			return luaTakeResult< T >( L );
		}
		else
		{
			const auto typeName = luaTypeName( lua_type( L, -1 ));
			throw std::runtime_error( createString( "'" << join( path.begin(), path.end(), "." ) << "' was of incorrect type " << typeName << "." ));
		}
	}
	
	template< typename T >
	T getValue( lua_State* L, const std::string& path )
	{
		return getValue< T >( L, split( path, "." ));
	}

	inline int luaType( lua_State* L, const std::vector< std::string >& path )
	{
		ASSERT( path.empty() == false );
		
		traversePath( L, path.begin(), path.end() );
		
		return lua_type( L, -1 );
	}

	inline int luaType( lua_State* L, const std::string& path )
	{
		return luaType( L, split( path, "." ));
	}
	
	template< typename T >
	void setValue( lua_State* L, const std::vector< std::string >& path, const T& value )
	{
		ASSERT( path.empty() == false );
		
		if( path.size() > 1 )
		{
			traversePath( L, path.begin(), path.end() - 1 );

			const auto type = lua_type( L, -1 );
			if( type == LUA_TTABLE )
			{
				lua_pushstring( L, path.back().c_str() );
				luacpp::luaPushValue( L, value );
				lua_settable( L, -3 );
			}
			else
			{
				throw std::runtime_error( createString( join( path.begin(), path.end() - 1, "." ) << " is not a table, but a " << luaTypeName( type ) << "." ));
			}
		}
		else
		{
			luacpp::luaPushValue( L, value );
			lua_setglobal( L, path.front().c_str() );
		}
	}
	
	template< typename T >
	void setValue( lua_State* L, const std::string& path, const T& value )
	{
		setValue< T >( L, split( path, "." ), value );
	}

#	define LUA_FUNCTION( function, numRequiredArguments ) \
	int luacpp_wrap_##function( lua_State* lua )	{ return luacpp::callCppFromLua( lua, &LUACPP_HOST_TYPE::function, numRequiredArguments ); }	\
	luacpp::LuaFunctionRegisterer g_luacpp_registerer_##function( STRINGIFY( function ), &luacpp_wrap_##function );
}

#endif /* LuaCppInterop_hpp */
