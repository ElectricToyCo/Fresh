//
//  FantasyConsole.inl.h
//  Fresh
//
//  Created by Jeff Wofford on 6/24/18.
//

#include "ApiImplementation.h"

namespace fr
{
	template< typename ReturnT, typename... Args >
	inline ReturnT FantasyConsole::callScriptFunction( const std::string& fnName, Args... args ) const
	{
		ASSERT( m_lua );
		return luacpp::callLua< ReturnT >( m_lua, fnName, args... );
	}
}
