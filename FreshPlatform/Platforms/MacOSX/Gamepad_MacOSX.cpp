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

/*

 Here's the way this maps out.

 On the user (code) side, we're going to ask a `Gamepad` for what the latest value of a given button or axis is.
 We'll use the Fresh-specific symbols to say what we mean: the `Button` and `Axis` enums found in `Gamepad.h`.
 So we can say, `gamepad->axis( Axis::RX )` to get the value [-1,1] of the right-hand stick's X axis.

 Within the `Gamepad` class these values are collected and stored in `m_gatheringAxes`, which is an array
 allowing you to find the axis or button value from an index. The index is simply the raw int cast of the enums,
 that is, for the above example, `m_gatheringAxes[ (int) Axis::RX ]`.

 But the hardware may have a very different ordering of these controls. What is '0' for the Xbox's left-hand X axis
 might be '5' for the Playstation controller's left-hand X axis. Class `Gamepad` therefore stores a separate mapping called
 `m_hardwareAxisMap` and `m_hardwareButtonMap`. This mapping is set by the hardware implementation
 (one per each platform) and specifies which hardware-specified index corresponds to the symbolic `Button` or `Axis`.
 In other words, it establishes that after initialization when the hardware says "the value of axis '3' is 0.7",
 whether hardware-axis '3' corresponds to Axis::RX or Axis::LY or whatever.
 We could simplify the `Gamepad` implementation somewhat by requiring the hardware to speak in terms of the symbolic
 enums, but the hardware code is complicated enough as it is. This way the hardware code tells `Gamepad`
 what it intends to call the various controls, then can later deal with those controls as the hardware itself wants
 to deal with them.

 This brings us in turn to the hardware code itself, which is indeed quite hairy by its nature, thanks to the OS
 and general HID programming customs.

 When a gamepad is plugged in it appears as a `IOHIDDeviceRef`. We create a Fresh `Gamepad` object and a
 local-to-this-file `GamepadPayload` in order to marry the two systems. We read the device for all its elements,
 storing the "cookie" (id) of each element along with the element itself and any other useful and unchanging information.
 We ask the HID system for a callback whenever any control on the device changes. And we tell the `Gamepad` what the hardware
 mapping will be--what indexes we will later use to indicate the various symbolic names.

 */

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

    // TODO doesn't work. 2024-10-02
    //
    std::string getElementUsageString( IOHIDElementRef element )
    {
        CFTypeRef usageString = IOHIDElementGetProperty( element, CFSTR(kIOHIDElementNameKey));
        if( usageString && CFGetTypeID( usageString ) == CFStringGetTypeID() )
        {
            return CFStringGetCStringPtr( reinterpret_cast< CFStringRef >( usageString ), kCFStringEncodingUTF8 );
        }
        else
        {
            return {};
        }
    }

	void onDeviceValueChanged( void* context, IOReturn result, void* sender, IOHIDValueRef value );

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

            std::map< size_t, Gamepad::Axis > hidUsagesToAxes
            {
                std::make_pair( kHIDUsage_GD_X, Gamepad::Axis::LX ),
                std::make_pair( kHIDUsage_GD_Y, Gamepad::Axis::LY ),
                std::make_pair( kHIDUsage_GD_Z, Gamepad::Axis::LTrigger ),		// TODO Left for sure?
                std::make_pair( kHIDUsage_GD_Rx, Gamepad::Axis::RX ),
                std::make_pair( kHIDUsage_GD_Ry, Gamepad::Axis::RY ),
                std::make_pair( kHIDUsage_GD_Rz, Gamepad::Axis::RTrigger ),		// TODO right for sure?
            };

            std::map< size_t, Gamepad::Button > hidUsagesToButtons
            {
                // TODO 2024-10-02 tested with Mac and Xbox 360 controller.
				std::make_pair( kHIDUsage_Button_1, Gamepad::Button::A ),
				std::make_pair( kHIDUsage_Button_2, Gamepad::Button::B ),
				std::make_pair( kHIDUsage_Button_3, Gamepad::Button::X ),
				std::make_pair( kHIDUsage_Button_4, Gamepad::Button::Y ),
                std::make_pair( kHIDUsage_GD_DPadRight, Gamepad::Button::DPadRight ),
				std::make_pair( kHIDUsage_GD_DPadDown, Gamepad::Button::DPadDown ),
				std::make_pair( kHIDUsage_GD_DPadLeft, Gamepad::Button::DPadLeft ),
				std::make_pair( kHIDUsage_GD_DPadUp, Gamepad::Button::DPadUp ),
				std::make_pair( kHIDUsage_Button_13, Gamepad::Button::LStick ),
				std::make_pair( kHIDUsage_Button_14, Gamepad::Button::RStick ),
				std::make_pair( kHIDUsage_Button_5, Gamepad::Button::LBumper ),
				std::make_pair( kHIDUsage_Button_6, Gamepad::Button::RBumper ),
				std::make_pair( kHIDUsage_GD_Select, Gamepad::Button::Back ),
				std::make_pair( kHIDUsage_GD_Start, Gamepad::Button::Start ),
			};

            std::map< size_t, Gamepad::Button > buttonMap;
            std::map< size_t, Gamepad::Axis > axisMap;

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

					const auto usage = IOHIDElementGetUsage( element );

                    release_trace( "Device misc or axis element with usage " << usage );

					// Ignore irrelevant axes.
					if( usage < kHIDUsage_GD_X || usage > kHIDUsage_GD_Rz )
					{
						continue;
					}

					Axis axis;

					axis.element = element;
					axis.cookie = IOHIDElementGetCookie( element );
					axis.logicalMin = IOHIDElementGetLogicalMin( element );
					axis.logicalMax = IOHIDElementGetLogicalMax( element );

					size_t index = usage - kHIDUsage_GD_X;
					ASSERT( index <= ( kHIDUsage_GD_Rz - kHIDUsage_GD_X ));

					m_axes.resize( std::max( m_axes.size(), index + 1 ));
					m_axes[ index ] = axis;

                    Gamepad::Axis freshAxisSymbol = hidUsagesToAxes[ usage ];

                    axisMap[ index ] = freshAxisSymbol;
				}
				else if( type == kIOHIDElementTypeInput_Button )
				{
                    const auto usage = IOHIDElementGetUsage( element );

                    release_trace( "Device button element with usage " << usage );

                    const auto iter = hidUsagesToButtons.find( usage );
                    if( iter != hidUsagesToButtons.end() )
                    {
                        const size_t index = m_buttons.size();

                        m_buttons.push_back( Button{} );
                        Button& button = m_buttons.back();
                        button.element = element;
                        button.cookie = IOHIDElementGetCookie( element );

                        buttonMap[ index ] = iter->second;
                    }
				}
			}
            
			CFRelease( elements );

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

				gamepad_trace( "hardware axis " << axisIndex << " moved to " << floatValue );
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
                    if(index < size_t( Gamepad::Button::NUM ))
                    {
                        m_gamepad->setButtonValue( index, integerValue );

                        gamepad_trace( "hardware button " << index << " changed to " << integerValue );
                    }
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

