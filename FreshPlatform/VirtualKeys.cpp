//
//  VirtualKeys.cpp
//  Fresh
//
//  Created by Jeff Wofford on 5/31/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "VirtualKeys.h"
#include "Property.h"

namespace fr
{

	const char* EventVirtualKey::VIRTUAL_KEY_DOWN = "VirtualKeyDown";
	const char* EventVirtualKey::VIRTUAL_KEY_UP = "VirtualKeyUp";

	/////////////////////////////////////
	
	FRESH_DEFINE_CLASS( VirtualKeys )

	DEFINE_VAR( VirtualKeys, Bindings, m_bindings );
	
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( VirtualKeys )
	
	void VirtualKeys::bindVirtualKeyToKeyCombo( EventVirtualKey::KeyNameRef virtualKeyName, const PhysicalKeyCombo& combo, bool replaceElseAdd )
	{
		auto iter = m_bindings.find( virtualKeyName );
		if( iter != m_bindings.end() )
		{
			if( replaceElseAdd )
			{
				iter->second.clear();
			}
			iter->second.push_back( combo );
		}
		else
		{
			PhysicalKeyCombos combos;
			combos.push_back( combo );
			m_bindings[ virtualKeyName ] = std::move( combos );
		}
	}
	
	void VirtualKeys::unbindVirtualKey( EventVirtualKey::KeyNameRef virtualKeyName )
	{
		auto iter = m_bindings.find( virtualKeyName );
		if( iter != m_bindings.end() )
		{
			m_bindings.erase( iter );
		}
	}
	
	bool VirtualKeys::isVirtualKeyDown( EventVirtualKey::KeyNameRef virtualKeyName ) const
	{
#if _WIN32 && 1			// TODO debugging

		auto fn = [&]( int vk, Keyboard::Key key, const std::string& keyname )
		{
			bool actuallyDown = ( ::GetAsyncKeyState( vk ) & 0x8000 ) == 0x8000;
			bool consideredDown = m_keysDown[ key ];
			if( actuallyDown != consideredDown )
			{
				dev_trace( keyname << " key thought of as " << consideredDown << " when it is actually " << actuallyDown );
			}
		};

		fn( VK_MENU, Keyboard::AltOption, "AltOption" );
		fn( VK_SHIFT, Keyboard::Shift, "Shift" );
		fn( VK_CONTROL, Keyboard::CtrlCommand, "CtrlCommand" );

#endif

		auto iter = m_virtualKeyStates.find( virtualKeyName );
		if( iter != m_virtualKeyStates.end() )
		{
			return iter->second;
		}
		
		// No such binding. Therefore the key isn't down.
		return false;
	}
	
	void VirtualKeys::clearKeysDown()
	{
		for( size_t i = 0; i < MAX_PHYSICAL_KEYS; ++i )
		{
			if( m_keysDown[ i ] )
			{
				onKeyChanged( Keyboard::Key( i ), false );
			}
		}
	}

	void VirtualKeys::onKeyUp( const EventKeyboard& event )
	{
		onKeyChanged( event.key(), false );
	}
	
	void VirtualKeys::onKeyDown( const EventKeyboard& event )
	{
		onKeyChanged( event.key(), true );
	}

	bool VirtualKeys::isComboAllDown( const VirtualKeys::PhysicalKeyCombo& combo ) const
	{
		for( size_t i = 0; i < MAX_PHYSICAL_KEYS; ++i )
		{
			const bool isKeyDown = m_keysDown[ i ];
			const bool isKeyPartOfCombo = std::find( combo.begin(), combo.end(), Keyboard::Key( i )) != combo.end();
			
			if( isKeyDown != isKeyPartOfCombo )
			{
				return false;
			}
		}
		
		return true;
	}
	
	void VirtualKeys::onKeyChanged( Keyboard::Key key, bool goingDownElseUp )
	{
//		dev_trace( "Key " << key << " going " << ( goingDownElseUp ? "down" : "up" ) );
		
		m_keysDown[ key ] = goingDownElseUp;
		
		// For every binding...
		//
		for( const auto& binding : m_bindings )
		{
			const auto& virtualKeyName = binding.first;
			
			// For every combo associated with this virtual key...
			//
			for( const auto& combo : binding.second )
			{
				// Is this key the last key in this combo?
				//
				if( combo.back() == key )
				{
					// Yes it is.
					
					// Does this key event *complete* or *break* the combo (i.e. are all involved keys now down? or is one of them up?
					//
					const bool comboDown = isComboAllDown( combo );
					
					if( comboDown != m_virtualKeyStates[ virtualKeyName ] )
					{
						m_virtualKeyStates[ virtualKeyName ] = comboDown;
						
//						dev_trace( "Virtual key '" << virtualKeyName << "' is now " << ( comboDown ? "down" : "up" ) );
						
						// Notify the universe.
						//
						EventVirtualKey event( goingDownElseUp ? EventVirtualKey::VIRTUAL_KEY_DOWN : EventVirtualKey::VIRTUAL_KEY_UP, this, virtualKeyName, this );
						dispatchEvent( &event );
					}
				}
			}
		}
	}
	
	void VirtualKeys::load( const Manifest::Map& properties )
	{
		Super::load( properties );
		
		// Load bindings.
		//
		auto passthroughProperty = properties.find( "passthrough" );
		
		if( passthroughProperty != properties.end() )
		{
			auto& passthroughMap = passthroughProperty->second.first->get< Manifest::Map >();
			auto bindElement = passthroughMap.find( "bind" );
			
			if( bindElement != passthroughMap.end() )
			{
				loadBindings( bindElement->second.first->get< std::string >() );
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////////
//
// BIND LOADING

/* COMMAND SYNTAX:
 
 bindCommand => VIRTUAL_KEY_NAME '=' boundToExpression
 
 boundToExpression =>		physicalKeyExpression '|' boundToExpression
 |							physicalKeyExpression
 
 physicalKeyExpression =>	KEY_NAME '+' physicalKeyExpression
 |							KEY_NAME
 
 */


namespace
{
	using namespace fr;
	
	enum Token
	{
		END,
		EQUALS,
		PIPE,
		CONCAT,
		IDENTIFIER,
		NONE,
	};
	
	std::string g_putBackValue;
	Token g_putBackToken = NONE;
	
	void putBack( Token token, const std::string& value )
	{
		ASSERT( g_putBackToken == NONE );
		ASSERT( token != NONE );
		g_putBackToken = token;
		g_putBackValue = value;
	}
	
	Token getToken( std::istream& in, std::string& value )
	{
		value.clear();
		
		if( g_putBackToken != NONE )
		{
			value = g_putBackValue;
			g_putBackValue.clear();
			
			Token savedToken = g_putBackToken;
			g_putBackToken = NONE;
			
			return savedToken;
		}
		
		bool skippingInitialSpace = true;
		while( true )
		{
			const int c = in.get();
			
			if( c == std::char_traits< char >::eof() )
			{
				if( skippingInitialSpace )
				{
					return END;
				}
				else
				{
					return IDENTIFIER;
				}
			}
			
			if( ::isspace( c ))
			{
				if( skippingInitialSpace )
				{
					continue;
				}
				else
				{
					return IDENTIFIER;
				}
			}
			else
			{
				skippingInitialSpace = false;
			}
			
			value += c;
			
			switch( c )
			{
				case '=': return EQUALS;
				case '|': return PIPE;
				case '+': return CONCAT;
				default: break;		// Don't respond to any other single character.
			}
		}
	}
	
	bool parsePhysicalKeyExpression( std::istream& in, VirtualKeys::PhysicalKeyCombo& outCombo )
	{
		std::string tokenValue;
		Token token = getToken( in, tokenValue );
		
		if( token == IDENTIFIER )
		{
			// Handle the identifier.
			//
			Keyboard::Key key;
			std::istringstream keyName( tokenValue );
			keyName >> key;
			
			if( key == Keyboard::Unsupported )
			{
				dev_warning( "Invalid key name in physicalKeyExpression '" << tokenValue << "'." );
				return false;
			}
			
			outCombo.push_back( key );
			
			// Lookahead.
			//
			std::string nextTokenValue;
			auto nextToken = getToken( in, nextTokenValue );
			
			if( nextToken == CONCAT )
			{
				return parsePhysicalKeyExpression( in, outCombo );
			}
			else
			{
				// Put the token back.
				//
				putBack( nextToken, nextTokenValue );
				return true;
			}
		}
		else
		{
			dev_warning( "Expected IDENTIFIER in physicalKeyExpression. Instead got " << tokenValue << "."  );
			return false;
		}
	}
	
	bool parseBoundToExpression( std::istream& in, std::vector< VirtualKeys::PhysicalKeyCombo >& outCombos )
	{
		while( true )
		{
			VirtualKeys::PhysicalKeyCombo combo;
			
			bool success = parsePhysicalKeyExpression( in, combo );
			if( !success )
			{
				dev_warning( "Expected physicalKeyExpression in boundToExpression." );
				return false;
			}
			else
			{
				ASSERT( !combo.empty() );
				outCombos.push_back( combo );
				
				std::string tokenValue;
				Token token = getToken( in, tokenValue );
				
				if( token == PIPE )
				{
					continue;
				}
				else if( token == END )
				{
					return true;
				}
				else
				{
					dev_warning( "Got unexpected token " << tokenValue << " after boundToExpression." );
				}
			}
		}
	}
}

namespace fr
{
	void VirtualKeys::loadBindings( const std::string& bindingScript )
	{
		size_t start = 0;
		
		// Process each line that ends with a semicolon or newline.
		//
		while( start < bindingScript.size() )
		{
			auto endOfCommand = bindingScript.find_first_of( ";\n", start );
			
			if( endOfCommand >= bindingScript.size() )
			{
				endOfCommand = bindingScript.size();
			}
			
			if( endOfCommand > start )
			{
				bindByScript( bindingScript.substr( start, endOfCommand - start ));
			}
			
			start = endOfCommand + 1;
		}
	}
	
	void VirtualKeys::bindByScript( const std::string& bindCommand )
	{
		// Separate the command around the =.
		//
		auto iDivider = bindCommand.find( '=' );
		
		if( iDivider >= bindCommand.size() )
		{
			// No divider.
			//
			dev_warning( this << " received binding command '" << bindCommand << "' with no divider (=)." );
			return;
		}
		
		std::string virtualKeyName = bindCommand.substr( 0, iDivider );
		trim( virtualKeyName );
		
		// Parse the bound-to part.
		//
		std::istringstream boundToExpression( bindCommand.substr( iDivider + 1 ));
		
		std::vector< VirtualKeys::PhysicalKeyCombo > combos;
		bool success = parseBoundToExpression( boundToExpression, combos );
		
		if( success )
		{
			m_bindings[ virtualKeyName ] = std::move( combos );
		}
	}

}





