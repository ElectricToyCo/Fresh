//
//  VirtualKeys.h
//  Fresh
//
//  Created by Jeff Wofford on 5/31/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_VirtualKeys_h
#define Fresh_VirtualKeys_h

#include "EventKeyboard.h"
#include "EventDispatcher.h"

namespace fr
{
	class EventVirtualKey : public Event
	{
	public:
		
		static const char* VIRTUAL_KEY_DOWN;
		static const char* VIRTUAL_KEY_UP;
		
		typedef std::string KeyName;
		typedef const std::string& KeyNameRef;

		EventVirtualKey( TypeRef type_,
					  Object::ptr target_,
					  KeyNameRef virtualKeyName,
					  Object::ptr currentTarget = nullptr
					  )
		:	Event( type_, target_, currentTarget )
		,	m_keyName( virtualKeyName )
		{}
		
		EventVirtualKey( const EventVirtualKey& event, Phase newPhase )
		:	Event( event, newPhase )
		,	m_keyName( event.m_keyName )
		{}
		
		SYNTHESIZE_GET( KeyNameRef, keyName )
		
	private:
		
		KeyName m_keyName;
	};
	
	//////////////////////////////////////////////////////
	
	class VirtualKeys : public EventDispatcher
	{
		FRESH_DECLARE_CLASS( VirtualKeys, EventDispatcher );
	public:
		
		typedef std::vector< Keyboard::Key > PhysicalKeyCombo;
		
		void bindVirtualKeyToKeyCombo( EventVirtualKey::KeyNameRef virtualKeyName, const PhysicalKeyCombo& combo, bool replaceElseAdd = true );
		void unbindVirtualKey( EventVirtualKey::KeyNameRef virtualKeyName );
		
		bool isVirtualKeyDown( EventVirtualKey::KeyNameRef virtualKeyName ) const;
		
		// Input notifications from the user.
		//
		virtual void clearKeysDown();		// Marks all keys as up.
		virtual void onKeyUp( const EventKeyboard& event );
		virtual void onKeyDown( const EventKeyboard& event );

		// Serialization.
		//
		virtual void load( const Manifest::Map& properties ) override;

		void loadBindings( const std::string& bindingScript );
		void bindByScript( const std::string& bindCommand );

	protected:
		
		void onKeyChanged( Keyboard::Key key, bool goingDownElseUp );
		bool isComboAllDown( const VirtualKeys::PhysicalKeyCombo& combo ) const;
		
	private:
		
		static const size_t MAX_PHYSICAL_KEYS = 256;
		
		typedef std::vector< PhysicalKeyCombo > PhysicalKeyCombos;
		
		typedef std::map< EventVirtualKey::KeyName, PhysicalKeyCombos > Bindings;
		typedef std::map< EventVirtualKey::KeyName, bool > States;
		
		VAR( Bindings, m_bindings );
		
		bool m_keysDown[ MAX_PHYSICAL_KEYS ] = { false };
		States m_virtualKeyStates;
	};
	
}

#endif
