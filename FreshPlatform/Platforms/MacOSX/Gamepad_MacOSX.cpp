//
//  Gamepad_MacOSX.cpp
//  Fresh
//
//  Created by Jeff Wofford on 4/14/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//

#include "Gamepad.h"
#include "FreshEssentials.h"
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDUsageTables.h>
using namespace fr;

#if 1
#	define gamepad_trace release_trace
#endif

namespace
{
	// Get HID device property key as a string
	std::string getDeviceString( IOHIDDeviceRef ref, CFStringRef prop )
	{
		CFTypeRef typeRef = IOHIDDeviceGetProperty( ref, prop );
		if( typeRef && ( CFGetTypeID( typeRef ) == CFStringGetTypeID() ))
		{
			CFStringRef str = static_cast< CFStringRef >( typeRef );
			return stringFromCFString( str );
		}

		release_warning( "Gamepad: Unable to read string value for property '" << stringFromCFString(prop) );
		return "Unknown Gamepad";
	}


	// Get HID device property key as an unsigned int
	unsigned int getDeviceUint( IOHIDDeviceRef ref, CFStringRef prop )
	{
		CFTypeRef typeRef = IOHIDDeviceGetProperty( ref, prop );
		if (typeRef && ( CFGetTypeID( typeRef ) == CFNumberGetTypeID() ))
		{
			SInt32 value;
			CFNumberGetValue(static_cast< CFNumberRef>( typeRef ), kCFNumberSInt32Type, &value );
			return value;
		}

		release_warning( "Gamepad: Unable to read uint value for property '" << stringFromCFString( prop ));
		return 0;
	}
	
	void onDeviceValueChanged( void* context, IOReturn result, void * sender, IOHIDValueRef value );
	
	struct GamepadPayload
	{
		struct Control
		{
			IOHIDElementRef element;
			IOHIDElementCookie cookie;
		};
		
		struct Axis : public Control
		{
			CFIndex logicalMin;
			CFIndex logicalMax;
		};
		
		struct Button : public Control
		{};
		
		std::string buttonName( size_t index ) const
		{
			REQUIRES( index < m_buttons.size() );
			
			CFStringRef name = IOHIDElementGetName( m_buttons[ index ].element );
			const char* sz = CFStringGetCStringPtr( name, kCFStringEncodingUTF8 );
			return sz ? std::string{ sz } : std::string{};
		}
		
		std::string axisName( size_t index ) const
		{
			REQUIRES( index < m_axes.size() );
			CFStringRef name = IOHIDElementGetName( m_axes[ index ].element );
			const char* sz = CFStringGetCStringPtr( name, kCFStringEncodingUTF8 );
			return sz ? std::string{ sz } : std::string{};
		}
		
		GamepadPayload( Gamepad::wptr gamepad, IOHIDDeviceRef device )
		:	m_gamepad( gamepad )
		,	m_device( device )
		{
			ASSERT( gamepad );
			
			// Get the device description.
			//
			m_deviceName = getDeviceString( m_device, CFSTR( kIOHIDProductKey ) );
			m_vendorId   = getDeviceUint( 	m_device, CFSTR( kIOHIDVendorIDKey ) );
			m_productId  = getDeviceUint( 	m_device, CFSTR( kIOHIDProductIDKey ) );
			
			release_trace( "Found gamepad '" << m_deviceName << "' vendor: " << m_vendorId << " product: " << m_productId );
			
			// Read button and axis information.
			//
			CFArrayRef elements = IOHIDDeviceCopyMatchingElements( m_device, NULL, kIOHIDOptionsTypeNone );
			const CFIndex nElements = CFArrayGetCount( elements );
			
			for( CFIndex i = 0; i < nElements; ++i )
			{
				IOHIDElementRef element = static_cast< IOHIDElementRef >( const_cast< void* >( CFArrayGetValueAtIndex( elements, i )));
				const IOHIDElementType type = IOHIDElementGetType( element );
				
				if( type == kIOHIDElementTypeInput_Misc || type == kIOHIDElementTypeInput_Axis )
				{
					const auto usagePage = IOHIDElementGetUsagePage( element );
					if( usagePage != kHIDPage_GenericDesktop )
					{
						continue;
					}
					
					const auto axisUsage = IOHIDElementGetUsage( element );
					
					// Ignore irrelevant axes.
					if( axisUsage < kHIDUsage_GD_X || axisUsage > kHIDUsage_GD_Rz )
					{
						continue;
					}

					Axis axis;
					
					axis.element = element;
					axis.cookie = IOHIDElementGetCookie( element );
					axis.logicalMin = IOHIDElementGetLogicalMin( element );
					axis.logicalMax = IOHIDElementGetLogicalMax( element );

					size_t index = axisUsage - kHIDUsage_GD_X;
					ASSERT( index <= ( kHIDUsage_GD_Rz - kHIDUsage_GD_X ));
					
					m_axes.resize( std::max( m_axes.size(), index + 1 ));
					m_axes[ index ] = axis;
				}
				else if( type == kIOHIDElementTypeInput_Button )
				{
					m_buttons.push_back( Button{} );		// Later these will be sorted to usage order.
					Button& button = m_buttons.back();
					button.element = element;
					button.cookie = IOHIDElementGetCookie( element );
				}
			}
			CFRelease( elements );
			
			// Sort buttons to have the same order as their OS-defined usage.
			std::sort( m_buttons.begin(), m_buttons.end(), []( const Button& a, const Button& b )
					  {
							return IOHIDElementGetUsage( a.element ) < IOHIDElementGetUsage( b.element );
					  });

			std::map< size_t, Gamepad::Button > buttonMap			// TODO map should be configured per vendor/product rather than statically like this.
			{
				std::make_pair(  1, Gamepad::Button::A ),
				std::make_pair(  2, Gamepad::Button::B ),
				std::make_pair(  0, Gamepad::Button::X ),
				std::make_pair(  3, Gamepad::Button::Y ),
//				std::make_pair(  8, Gamepad::Button::DPadRight ),
//				std::make_pair(  6, Gamepad::Button::DPadDown ),
//				std::make_pair(  7, Gamepad::Button::DPadLeft ),
//				std::make_pair(  5, Gamepad::Button::DPadUp ),
				std::make_pair( 10, Gamepad::Button::LStick ),
				std::make_pair( 11, Gamepad::Button::RStick ),
				std::make_pair(  4, Gamepad::Button::LBumper ),
				std::make_pair(  5, Gamepad::Button::RBumper ),
				std::make_pair(  8, Gamepad::Button::Back ),
				std::make_pair(  9, Gamepad::Button::Start ),
			};

			std::map< size_t, Gamepad::Axis > axisMap
			{													// TODO These mappings are based on the 8BitDo DualShock 4 Wireless
				std::make_pair( 0, Gamepad::Axis::LX ),			// kHIDUsage_GD_X
				std::make_pair( 1, Gamepad::Axis::LY ),			// kHIDUsage_GD_Y
				std::make_pair( 2, Gamepad::Axis::RX ),			// kHIDUsage_GD_Z
				std::make_pair( 5, Gamepad::Axis::RY ),			// kHIDUsage_GD_Rx
				std::make_pair( 3, Gamepad::Axis::LTrigger ),	// kHIDUsage_GD_Ry
				std::make_pair( 4, Gamepad::Axis::RTrigger ),	// kHIDUsage_GD_Rz
			};

			m_gamepad->create( this, std::move( buttonMap ), std::move( axisMap ));

			IOHIDDeviceRegisterInputValueCallback( m_device, onDeviceValueChanged, this );
		}
		
		bool hasDevice( IOHIDDeviceRef device ) const { return m_device == device; }
		SYNTHESIZE_GET( Gamepad::wptr, gamepad )
		
		void onValueChanged( IOHIDValueRef value )
		{
			if( IOHIDValueGetLength( value ) > 4 )
			{
				// Per Alex Diener's implementation: "Workaround for a strange crash that occurs with PS3 controller; was getting lengths of 39 (!)."
				return;
			}
			
			IOHIDElementRef element = IOHIDValueGetElement( value );
			IOHIDElementCookie cookie = IOHIDElementGetCookie( element );
			
			CFIndex integerValue = IOHIDValueGetIntegerValue( value );
			
			// Is this an axis control?
			//
			auto iter = std::find_if( m_axes.begin(), m_axes.end(), [&]( const Axis& axis )
									 {
										 return axis.cookie == cookie;
									 } );
			
			if( iter != m_axes.end() )
			{
				const auto axisIndex = iter - m_axes.begin();
				Axis& axis = *iter;
				
				axis.logicalMin = std::min( axis.logicalMin, integerValue );
				axis.logicalMax = std::max( axis.logicalMax, integerValue );
				
				const float floatValue = ( integerValue - axis.logicalMin ) /
					static_cast< float >( axis.logicalMax - axis.logicalMin) * 2.0f - 1.0f;
				
				m_gamepad->setAxisValue( axisIndex, floatValue );
				
				gamepad_trace( "axis " << axisIndex << " moved to " << floatValue );
			}
			else
			{
				// Is it a button control?
				//
				auto iterFoundButton = std::find_if( m_buttons.begin(), m_buttons.end(), [&]( const Button& button )
										 {
											 return button.cookie == cookie;
										 } );
				
				if( iterFoundButton != m_buttons.end() )
				{
					const size_t index = iterFoundButton - m_buttons.begin();
					m_gamepad->setButtonValue( index, integerValue );

					gamepad_trace( "button " << index << " changed to " << integerValue );
				}
			}
		}
		
	private:
		
		Gamepad::wptr m_gamepad;
		IOHIDDeviceRef m_device;
		std::vector< Axis > m_axes;
		std::vector< Button > m_buttons;
		
		std::string m_deviceName;
		uint m_vendorId;
		uint m_productId;
	};
	
	IOHIDManagerRef g_hidManager = nullptr;
	std::vector< GamepadPayload* > g_payloads;
	
	void onDeviceAttached( void* context, IOReturn result, void* sender, IOHIDDeviceRef device )
	{
		GamepadManager* const gamepadManager = reinterpret_cast< GamepadManager* >( context );
		ASSERT( gamepadManager );
		
		// Create the gamepad.
		//
		auto gamepad = gamepadManager->createGamepad();
		
		// Setup the payload.
		//
		GamepadPayload* const payload = new GamepadPayload( gamepad.get(), device );
		ASSERT( payload );
		g_payloads.push_back( payload );
		
		gamepadManager->onGamepadAttached( gamepad );
	}
	
	void onDeviceDetached( void* context, IOReturn result, void* sender, IOHIDDeviceRef device )
	{
		GamepadManager* const gamepadManager = reinterpret_cast< GamepadManager* >( context );
		ASSERT( gamepadManager );
		
		// Find the payload associated with this device.
		//
		auto iter = std::find_if( g_payloads.begin(), g_payloads.end(), [&]( GamepadPayload* payload )
					 {
						 ASSERT( payload );
						 return payload->hasDevice( device );
					 } );
		
		if( iter != g_payloads.end() )
		{
			// Remove the gamepad.
			//
			gamepadManager->onGamepadDetached( (*iter)->gamepad() );
			
			// Delete the payload.
			//
			delete *iter;
			g_payloads.erase( iter );
		}
	}
	
	void onDeviceValueChanged( void* context, IOReturn result, void* sender, IOHIDValueRef value )
	{
		GamepadPayload* const payload = reinterpret_cast< GamepadPayload* >( context );
		ASSERT( payload );
		
		payload->onValueChanged( value );
	}
}

namespace fr
{
	Gamepad::~Gamepad()
	{
	}
	
	void Gamepad::updateStates()
	{
		// Do nothing.
	}
	
	///////////////////////////////////////////

	void GamepadManager::construct()
	{
		ASSERT( !g_hidManager );
		g_hidManager = IOHIDManagerCreate( kCFAllocatorDefault, kIOHIDOptionsTypeNone );
		IOHIDManagerOpen( g_hidManager, kIOHIDOptionsTypeNone );
		
		IOHIDManagerScheduleWithRunLoop( g_hidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode );

		CFStringRef keys[2];
		keys[0] = CFSTR( kIOHIDDeviceUsagePageKey );
		keys[1] = CFSTR( kIOHIDDeviceUsageKey );
		
		CFNumberRef values[2];
		int value;
		
		CFDictionaryRef dictionaries[3];
		
		value = kHIDPage_GenericDesktop;
		values[0] = CFNumberCreate( kCFAllocatorDefault, kCFNumberSInt32Type, &value );
		value = kHIDUsage_GD_Joystick;
		values[1] = CFNumberCreate( kCFAllocatorDefault, kCFNumberSInt32Type, &value );
		dictionaries[0] = CFDictionaryCreate(kCFAllocatorDefault, (const void **) keys, (const void **) values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
		CFRelease( values[0] );
		CFRelease( values[1] );
		
		value = kHIDPage_GenericDesktop;
		values[0] = CFNumberCreate( kCFAllocatorDefault, kCFNumberSInt32Type, &value );
		value = kHIDUsage_GD_GamePad;
		values[1] = CFNumberCreate( kCFAllocatorDefault, kCFNumberSInt32Type, &value );
		dictionaries[1] = CFDictionaryCreate(kCFAllocatorDefault, (const void **) keys, (const void **) values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
		CFRelease(values[0]);
		CFRelease(values[1]);
		
		value = kHIDPage_GenericDesktop;
		values[0] = CFNumberCreate( kCFAllocatorDefault, kCFNumberSInt32Type, &value );
		value = kHIDUsage_GD_MultiAxisController;
		values[1] = CFNumberCreate( kCFAllocatorDefault, kCFNumberSInt32Type, &value );
		dictionaries[2] = CFDictionaryCreate(kCFAllocatorDefault, (const void **) keys, (const void **) values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
		CFRelease(values[0]);
		CFRelease(values[1]);
		
		CFArrayRef array = CFArrayCreate( kCFAllocatorDefault, (const void **) dictionaries, 3, &kCFTypeArrayCallBacks );
		CFRelease( dictionaries[0] );
		CFRelease( dictionaries[1] );
		CFRelease( dictionaries[2] );
		IOHIDManagerSetDeviceMatchingMultiple( g_hidManager, array );
		CFRelease( array );
		
		IOHIDManagerRegisterDeviceMatchingCallback( g_hidManager, onDeviceAttached, this );
		IOHIDManagerRegisterDeviceRemovalCallback( g_hidManager, onDeviceDetached, this );
	}
	
	void GamepadManager::updateSystem()
	{
		// Do nothing.
	}
	
	void GamepadManager::destroy()
	{
		if( g_hidManager )
		{
			IOHIDManagerUnscheduleFromRunLoop( g_hidManager, CFRunLoopGetCurrent(), kCFRunLoopCommonModes );
			IOHIDManagerClose( g_hidManager, kIOHIDOptionsTypeNone );
			CFRelease( g_hidManager );
			g_hidManager = nullptr;
		}
	}
}

