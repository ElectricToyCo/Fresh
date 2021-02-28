//
//  Gamepad_Unix.cpp
//  Fresh
//
//  Created by Jeff Wofford on 4/14/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//

#include "Gamepad.h"
#include "FreshRange.h"
#include "FreshMath.h"
#include "CommandProcessor.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
using namespace fr;

#if 0 	// TDOO!!
#	define gamepad_manager_trace release_trace
#else
#	define gamepad_manager_trace(x)
#endif

#if 1 	// TDOO!!
#	define gamepad_trace release_trace
#else
#	define gamepad_trace(x)
#endif

namespace
{
	inline bool isBitSet( int bitIndex, int* array )
	{
		static const int bitsPerInt = sizeof( int ) * 8;
		return ( array[ bitIndex / bitsPerInt ] >> ( bitIndex % bitsPerInt )) & 0x1;
	}

	class GamepadPayload
	{
	public:

		GamepadPayload( Gamepad& gamepad,
						int fileDescriptor,
						const std::string& devicePath )
		:	m_gamepad( gamepad )
		,	m_fileDescriptor( fileDescriptor )
		,	m_devicePath( devicePath )
		{
			// Get the description.
			//
			char name[ 128 ];
			if( ::ioctl( fileDescriptor, EVIOCGNAME( sizeof( name )), name ) > 0 )
			{
				m_description = name;
			} 
			else 
			{
				m_description = devicePath;
			}
			
			// Get vendor and product information.
			//
			struct input_id id;
			if( !::ioctl( fileDescriptor, EVIOCGID, &id ))
			{
				m_vendorID = id.vendor;
				m_productID = id.product;
			} 

			int evCapBits[ (  EV_CNT - 1 ) / sizeof( int ) * 8 + 1 ];
			int evKeyBits[ ( KEY_CNT - 1 ) / sizeof( int ) * 8 + 1 ];
			int evAbsBits[ ( ABS_CNT - 1 ) / sizeof( int ) * 8 + 1 ];
			std::memset( evCapBits, 0, sizeof( evCapBits ));
			std::memset( evKeyBits, 0, sizeof( evKeyBits ));
			std::memset( evAbsBits, 0, sizeof( evAbsBits ));

			ioctl( m_fileDescriptor, EVIOCGBIT( EV_KEY, sizeof( evKeyBits )), evKeyBits );
			ioctl( m_fileDescriptor, EVIOCGBIT( EV_ABS, sizeof( evAbsBits )), evAbsBits );
			
			// Record axis information.
			//
			m_numAxes = 0;
			for( int bit = 0; bit < ABS_CNT; ++bit )
			{
				if( isBitSet( bit, evAbsBits ))
				{
					if( ioctl( m_fileDescriptor, EVIOCGABS( bit ), &m_axisInfo[ bit ]) < 0 ||
						  m_axisInfo[ bit ].minimum == m_axisInfo[ bit ].maximum ) 
					{
						continue;
					}

					gamepad_trace( "Axis " << m_numAxes << " has range [" << m_axisInfo[ bit ].minimum << "," << m_axisInfo[ bit ].maximum << "]." );

					m_axisMap[ bit ] = m_numAxes;
					++m_numAxes;
				}
			}

			// Record button information.
			//
			m_numButtons = 0;
			for( int bit = BTN_MISC; bit < KEY_CNT; ++bit ) 
			{
				if( isBitSet( bit, evKeyBits )) 
				{
					m_buttonMap[ bit - BTN_MISC ] = m_numButtons;
					++m_numButtons;
				}
			}

			gamepad_trace( "Created gamepad " << m_devicePath << " with " << m_numAxes << " axes and " << m_numButtons << " buttons." );

			std::map< size_t, Gamepad::Button > buttonMap			// TODO!!! map should be configured per vendor/product rather than statically like this.
			{
				std::make_pair(  0, Gamepad::Button::A ),
				std::make_pair(  1, Gamepad::Button::B ),
				std::make_pair(  3, Gamepad::Button::X ),
				std::make_pair(  4, Gamepad::Button::Y ),
//				std::make_pair( -1, Gamepad::Button::DPadRight ),		// DPad unavailable as buttons.
//				std::make_pair( -1, Gamepad::Button::DPadDown ),
//				std::make_pair( -1, Gamepad::Button::DPadLeft ),
//				std::make_pair( -1, Gamepad::Button::DPadUp ),
				std::make_pair(  9, Gamepad::Button::LStick ),
				std::make_pair(  10, Gamepad::Button::RStick ),
				std::make_pair(  4, Gamepad::Button::LBumper ),
				std::make_pair(  5, Gamepad::Button::RBumper ),
				std::make_pair(  6, Gamepad::Button::Back ),
				std::make_pair(  7, Gamepad::Button::Start ),
			};

			std::map< size_t, Gamepad::Axis > axisMap
			{
				std::make_pair( 0, Gamepad::Axis::LX ),
				std::make_pair( 1, Gamepad::Axis::LY ),
				std::make_pair( 3, Gamepad::Axis::RX ),
				std::make_pair( 4, Gamepad::Axis::RY ),
				std::make_pair( 2, Gamepad::Axis::LTrigger ),
				std::make_pair( 5, Gamepad::Axis::RTrigger ),
			};

			m_gamepad.create( this, std::move( buttonMap ), std::move( axisMap ));

			// Start the async update thread.
			//
			m_hasUpdateThreadDied = false;
			m_updateThread.reset( new std::thread( &GamepadPayload::asyncUpdate, this ));
		}

		~GamepadPayload()
		{
			m_updateThread.reset();
			close( m_fileDescriptor );
		}

		bool hasPath( const std::string& path ) const
		{
			return m_devicePath == path;
		}
		
		bool updateStates()
		{
			// Process queued messages.
			//
			readMessageQueue();

			// Still attached if the update thread is still running.
			//
			return !m_hasUpdateThreadDied;
		}

	protected:

		struct Event
		{
			double timestamp;

			enum class ControlType
			{
				None,		// Not a control. Maybe some device-wide event.
				Axis,
				Button
			} controlType;

			int controlID;

			union
			{
				float f;
				bool b;
			} value;

			explicit Event( double t, int control, float axisValue )
			:	timestamp( t )
			,	controlType( ControlType::Axis )
			,	controlID( control )
			{
				value.f = axisValue;
			}
			explicit Event( double t, int control, bool buttonDown )
			:	timestamp( t )
			,	controlType( ControlType::Button )
			,	controlID( control )
			{
				value.b = buttonDown;
			}
		};


		void queueEvent( Event&& event )
		{
			m_mutex.lock();
			m_events.emplace_back( event );
			m_mutex.unlock();
		}

		void queueAxisEvent( double time, int axisID, float value )
		{
			queueEvent( Event{ time, axisID, value } );
		}

		void queueButtonEvent( double time, int buttonID, bool value )
		{
			queueEvent( Event{ time, buttonID, value } );
		}

		void readMessageQueue()
		{
			m_mutex.lock();

			for( const auto& event : m_events )
			{
				switch( event.controlType )
				{
					case Event::ControlType::Axis:
						m_gamepad.setAxisValue( event.controlID, event.value.f );
					break;
					case Event::ControlType::Button:
						m_gamepad.setButtonValue( event.controlID, event.value.b );
					break;

					default:
						ASSERT( false );
						break;
				}
			}

			m_events.clear();

			m_mutex.unlock();
		}

		void asyncUpdate()
		{
			try
			{
				struct input_event event;
				while( read( m_fileDescriptor, &event, sizeof( struct input_event )) > 0 ) 
				{
					if( event.type == EV_ABS ) 
					{
						if( event.code > ABS_MAX || m_axisMap[ event.code ] == -1 ) 
						{
							continue;
						}
						
						float value = lerp( -1.0f, 1.0f, proportion< float >( event.value, m_axisInfo[ event.code ].minimum, m_axisInfo[ event.code ].maximum ));
						queueAxisEvent( event.time.tv_sec + event.time.tv_usec * 0.000001,
						                m_axisMap[ event.code ],
						                value );
					} 
					else if( event.type == EV_KEY ) 
					{
						if( event.code < BTN_MISC || event.code > KEY_MAX || m_buttonMap[ event.code - BTN_MISC ] == -1 ) 
						{
							continue;
						}
						
						queueButtonEvent( event.time.tv_sec + event.time.tv_usec * 0.000001,
						                  m_buttonMap[ event.code - BTN_MISC ],
						                  !!event.value );
					}
				}
			}
			catch( ... )
			{
				release_trace( "Unknown exception" );
			}
			m_hasUpdateThreadDied = true;
		}

	private:

		Gamepad& m_gamepad;
		int m_fileDescriptor = 0;
		std::string m_devicePath;
		std::string m_description;
		int m_vendorID = 0;
		int m_productID = 0;
		int m_buttonMap[ KEY_CNT - BTN_MISC ];
		int m_axisMap[ ABS_CNT ];
		struct input_absinfo m_axisInfo[ ABS_CNT ];

		int m_numButtons = 0;
		int m_numAxes = 0;

		std::unique_ptr< std::thread > m_updateThread;
		std::recursive_mutex m_mutex;
		std::atomic< bool > m_hasUpdateThreadDied;

		std::vector< Event > m_events;
	};

	GamepadPayload& payloadForGamepad( Gamepad::ptr gamepad )
	{
		REQUIRES( gamepad );
		auto payload = reinterpret_cast< GamepadPayload* >( gamepad->payload() );
		ASSERT( payload );
		return *payload;
	}

}

namespace fr
{
	Gamepad::~Gamepad()
	{
		{
			auto payload = reinterpret_cast< GamepadPayload* >( m_payload );
			delete payload;
		}
		m_payload = nullptr;
	}
	

	void Gamepad::updateStates()
	{
		const auto payload = reinterpret_cast< GamepadPayload* >( m_payload );
		m_attached = payload->updateStates();
	}
	
	///////////////////////////////////////////

	void GamepadManager::construct()
	{
		gamepad_manager_trace( *this << " constructing." );
		updateSystem();
	}
	
	void GamepadManager::updateSystem()
	{
		// Watch for connections/disconnections.
		//

		static time_t lastInputStatTime = 0;
		time_t currentTime = time( nullptr );
		
		DIR* const inputDirectory = opendir( "/dev/input" );
		if( inputDirectory ) 
		{
			gamepad_manager_trace( *this << " scanning /dev/input." );

			const struct dirent* entity = nullptr;

			while(( entity = readdir( inputDirectory )) ) 
			{
				unsigned int charsConsumed = 0;

				int num = -1;
				if( std::sscanf( entity->d_name, "event%d%n", &num, &charsConsumed ) && 
					charsConsumed == std::strlen( entity->d_name )) 
				{
					gamepad_manager_trace( *this << " considering entity " << entity->d_name );

					char fileName[ PATH_MAX ];
					std::snprintf( fileName, PATH_MAX, "/dev/input/%s", entity->d_name );
					struct stat statBuf;
					if( ::stat( fileName, &statBuf ) || statBuf.st_mtime < lastInputStatTime ) 
					{
						continue;
					}

					gamepad_manager_trace( *this << " " << entity->d_name << " was modified since last check." );
					
					bool isDuplicate = std::any_of( m_gamepads.begin(), m_gamepads.end(), [&]( Gamepad::ptr gamepad )
					{
						auto& payload = payloadForGamepad( gamepad );
						return payload.hasPath( fileName );
					} );

					if( isDuplicate ) 
					{
						continue;
					}

					gamepad_manager_trace( *this << " " << entity->d_name << " is not a duplicate." );
					
					const int fileDescriptor = open( fileName, O_RDONLY, 0 );

					int evCapBits[ (  EV_CNT - 1 ) / sizeof( int ) * 8 + 1 ];
					int evKeyBits[ ( KEY_CNT - 1 ) / sizeof( int ) * 8 + 1 ];
					int evAbsBits[ ( ABS_CNT - 1 ) / sizeof( int ) * 8 + 1 ];
					std::memset( evCapBits, 0, sizeof( evCapBits ));
					std::memset( evKeyBits, 0, sizeof( evKeyBits ));
					std::memset( evAbsBits, 0, sizeof( evAbsBits ));
					if( ioctl( fileDescriptor, EVIOCGBIT( 0,      sizeof( evCapBits )), evCapBits ) < 0 ||
					    ioctl( fileDescriptor, EVIOCGBIT( EV_KEY, sizeof( evKeyBits )), evKeyBits ) < 0 ||
					    ioctl( fileDescriptor, EVIOCGBIT( EV_ABS, sizeof( evAbsBits )), evAbsBits ) < 0 ) 
					{
						gamepad_manager_trace( *this << " " << entity->d_name << " had no caps, keys, or abs's." );

						close( fileDescriptor);
						continue;
					}

					if( !isBitSet( EV_KEY, evCapBits ) || !isBitSet( EV_ABS, evCapBits ) ||
					    !isBitSet( ABS_X,  evAbsBits ) || !isBitSet( ABS_Y,  evAbsBits ) ||
					    ( !isBitSet( BTN_TRIGGER, evKeyBits ) && !isBitSet( BTN_A, evKeyBits ) && !isBitSet( BTN_1, evKeyBits ))) 
					{
						gamepad_manager_trace( *this << " " << entity->d_name << " had no interesting bits set." );

						close( fileDescriptor );
						continue;
					}

					gamepad_manager_trace( *this << " creating gamepad." );
			
					//
					// A new gamepad has been discovered or attached.
					//

					// Create the gamepad and payload.
					//
					auto gamepad = createGamepad();
					
					// Pointer stored internally:
					new GamepadPayload( 
						*gamepad,
						fileDescriptor,
						fileName );

					onGamepadAttached( gamepad );
				}
			}
		}

		closedir( inputDirectory );

		lastInputStatTime = currentTime;
	}
}

