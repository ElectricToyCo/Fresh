/*
 *  Application_RaspberryPi.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 2/20/14.
 *  Copyright 2014 jeffwofford.com. All rights reserved.
 *
 */

#include "Application.h"
#include "FreshDebug.h"
#include "FreshOpenGL.h"
#include "FreshFile.h"
#include "FreshTime.h"
#include "FreshException.h"
#include "Objects.h"
#include "Assets.h"
#include "FreshXML.h"
#include "CommandProcessor.h"
#include "TelnetServer.h"
#include <algorithm>

// Raspberry Pi Stuff
// https://github.com/benosteen/opengles-book-samples/blob/master/LinuxX11/Common/esUtil.c may be helpful
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

#include "bcm_host.h"

#define EGL_EGLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext_brcm.h>

#if FRESH_TELEMETRY_ENABLED
#	include "UserTelemetry.h"
#endif

using namespace fr;

#if 0
#	define trace_keys( x ) trace( x )
#else
#	define trace_keys( x )
#endif

namespace
{
	bool g_keyCodes[ 256 ] = { 0 };

	unsigned int g_nMouseSequences = 0;		// Counts the number of times that the mouse has been pressed. Roughly corresponds to CocoaTouch's touchId concept.
	vec2 g_lastMousePoint;
	bool g_haveLastMousePoint = false;

	struct MultiTapCounter
	{
		MultiTapCounter( real maxTapDistanceSquared = 4*4, TimeType maxTapDelay = 0.5 ) 
			:	m_maxTapDistanceSquared( maxTapDistanceSquared )
			,	m_maxTapDelay( maxTapDelay )
		{ 
			reset(); 
		}

		size_t tapCount() const { return m_nTaps; }

		void onDown( const vec2& location )
		{
			const auto time = getAbsoluteTimeSeconds();

			if( !isStillTap( location, time ))
			{
				reset();
			}

			recordTap( location, time );
		}

		void onUp( const vec2& location )
		{
			const auto time = getAbsoluteTimeSeconds();

			if( !isStillTap( location, time ))
			{
				reset();
			}
		}

	private:

		bool isStillTap( const vec2& location, TimeType eventTime ) const
		{
			if( m_lastTouchTime < 0 )
			{
				return true;
			}
			else
			{
				return	distanceSquared( m_originalTouchPoint, location ) <= m_maxTapDistanceSquared && 
						eventTime - m_lastTouchTime <= m_maxTapDelay;
			}
		}

		void recordTap( const vec2& location, TimeType eventTime )
		{
			if( m_lastTouchTime < 0 )	// First tap in series.
			{
				m_originalTouchPoint = location;
			}

			m_lastTouchTime = eventTime;

			++m_nTaps;
		}

		void reset()
		{
			m_nTaps = 0;
			m_lastTouchTime = -1;
		}

		size_t m_nTaps;
		vec2 m_originalTouchPoint;
		TimeType m_lastTouchTime;

		real m_maxTapDistanceSquared;
		TimeType m_maxTapDelay;

	} g_multiTapCounter;

	static void createAppTouches( const vec2& touchPoint, const vec2& lastTouchPoint, Application::Touches& outAppTouches, int nClicks )
	{
		outAppTouches.push_back( Application::Touch( 
													touchPoint,
													lastTouchPoint,
													vec2::ZERO,
													0,	// Which touch in the current group?
													1,	// How many touches in the current group? (Always at most 1 for mouse.)
													nClicks,	// Click count (e.g. single click vs. double click. Assume 1.)
													reinterpret_cast< void* >( g_nMouseSequences )));
	}


	bool isKeyDown( Keyboard::Key key )
	{
		if( 0 <= key && key < 256 )
		{
			return g_keyCodes[ key ];
		}
		else
		{
			return false;
		}
	}

	void onKeyChanged( Keyboard::Key key, bool downElseUp )
	{
		if( 0 <= key && key < 256 )
		{
			g_keyCodes[ key ] = downElseUp;
		}
	}

	Keyboard::Key getKeyFromKeySym( int keysym )
	{
		if(( keysym >= '0' && keysym <= 'Z' ) ||
		   (  keysym >= 'a' && keysym <= 'z'))
		{
			return Keyboard::Key( std::toupper( keysym ));	
		}

		switch( keysym )
		{
		case XK_BackSpace: return Keyboard::Backspace;
		case XK_Tab: return Keyboard::Tab;
		case XK_Clear: return Keyboard::NumpadClear;
		case XK_Return: return Keyboard::Enter;
		case XK_Pause: return Keyboard::PauseBreak;
		case XK_Caps_Lock: return Keyboard::CapsLock;
		case XK_Escape: return Keyboard::Escape;
		case XK_space: return Keyboard::Space;
		case XK_Page_Up: return Keyboard::PageUp;
		case XK_Page_Down: return Keyboard::PageDown;
		case XK_End: return Keyboard::End;
		case XK_Home: return Keyboard::Home;
		case XK_Left: return Keyboard::LeftArrow;
		case XK_Up: return Keyboard::UpArrow;
		case XK_Right: return Keyboard::RightArrow;
		case XK_Down: return Keyboard::DownArrow;
		case XK_KP_Insert: return Keyboard::Insert;
		case XK_KP_Delete: return Keyboard::Delete;
		case XK_KP_0: return Keyboard::Numpad0;
		case XK_KP_1: return Keyboard::Numpad1;
		case XK_KP_2: return Keyboard::Numpad2;
		case XK_KP_3: return Keyboard::Numpad3;
		case XK_KP_4: return Keyboard::Numpad4;
		case XK_KP_5: return Keyboard::Numpad5;
		case XK_KP_6: return Keyboard::Numpad6;
		case XK_KP_7: return Keyboard::Numpad7;
		case XK_KP_8: return Keyboard::Numpad8;
		case XK_KP_9: return Keyboard::Numpad9;
		case XK_KP_Multiply: return Keyboard::NumpadMultiply;
		case XK_KP_Add: return Keyboard::NumpadAdd;
		case XK_KP_Subtract: return Keyboard::NumpadSubtract;
		case XK_KP_Decimal: return Keyboard::NumpadDecimal;
		case XK_KP_Divide: return Keyboard::NumpadDivide;
		case XK_F1: return Keyboard::F1;
		case XK_F2: return Keyboard::F2;
		case XK_F3: return Keyboard::F3;
		case XK_F4: return Keyboard::F4;
		case XK_F5: return Keyboard::F5;
		case XK_F6: return Keyboard::F6;
		case XK_F7: return Keyboard::F7;
		case XK_F8: return Keyboard::F8;
		case XK_F9: return Keyboard::F9;
		case XK_F10: return Keyboard::F10;
		case XK_F11: return Keyboard::F11;
		case XK_F12: return Keyboard::F12;
		case XK_F13: return Keyboard::F13;
		case XK_F14: return Keyboard::F14;
		case XK_F15: return Keyboard::F15;
		case XK_Scroll_Lock: return Keyboard::ScrollLock;
		case XK_Shift_L: return Keyboard::Shift;
		case XK_Shift_R: return Keyboard::Shift;
		case XK_Control_L: return Keyboard::CtrlCommand;
		case XK_Control_R: return Keyboard::CtrlCommand;
		case XK_Alt_L: return Keyboard::AltOption;
		case XK_Alt_R: return Keyboard::AltOption;
		case XK_semicolon: return Keyboard::Semicolon;      
		case XK_equal: return Keyboard::Equal;
		case XK_comma: return Keyboard::Comma;
		case XK_minus: return Keyboard::Minus;
		case XK_period: return Keyboard::Period;
		case XK_slash: return Keyboard::Slash;
		case XK_grave: return Keyboard::Backtick;
		case XK_bracketright: return Keyboard::RightBracket;
		case XK_backslash: return Keyboard::Backslash;
		case XK_bracketleft: return Keyboard::LeftBracket;
		case XK_quotedbl: return Keyboard::Quote;
		default:
			return Keyboard::Unsupported;
		}
	}

	fr::EventKeyboard getFreshKeyboardEvent( const XEvent& event, const std::string& eventType )
	{
		XComposeStatus composeStatus;
		char szChars[ 16 ];
		KeySym keySym;
		int nChars = XLookupString( const_cast< XKeyEvent* >( &event.xkey ), szChars, 16, &keySym, &composeStatus );

		Keyboard::Key key = getKeyFromKeySym( keySym );

		trace_keys( eventType << ": " << key << " keysym: " << keySym );

		// The printable character.
		//
		unsigned int c = 0;
		if( nChars > 0 )
		{
			c = szChars[ 0 ];
		}

		const bool shift = isKeyDown( Keyboard::Shift );
		const bool isAHeldRepeat = false;	// TODO

		return EventKeyboard(
			eventType, 
			nullptr, 
			c, 
			key, 
			isKeyDown( Keyboard::AltOption ), 
			isKeyDown( Keyboard::CtrlCommand ), 
			shift, 
			isAHeldRepeat );
	}

#if 0
	void respondToEvent( const XEvent& event )
	{
		// TODO
			if( Application::doesExist() )
			{
				Application::instance().onGainedFocus();
			}
			break;

		// TODO
			if( Application::doesExist() )
			{
				Application::instance().onLostFocus();
			}
			break;
	}
#endif
}

namespace fr
{

	class ApplicationImplementation
	{
	public:

		ApplicationImplementation( Application* owner, CommandProcessor::ptr commandProcessor )
		:	m_owner( owner )
		,	m_commandProcessor( commandProcessor )
		,	m_isInMainLoop( false )
		,	m_desiredFramesPerSecond( owner->config().desiredFramesPerSecond() )
		,	m_nextUpdateTime( 0 )
		,	m_display( nullptr )
		,	m_window( 0 )
		,	m_eglDisplay( 0 )
		,	m_eglContext( 0 )
		,	m_eglSurface( 0 )
		{
			REQUIRES( owner );
			m_desiredClocksPerFrame = secondsToClocks( 1.0 / m_desiredFramesPerSecond );
		}

		~ApplicationImplementation()
		{
			shutdownOpenGL();
		}

		bool hasMainWindow() const
		{
			return m_display && m_window && m_eglDisplay && m_eglContext && m_eglSurface;
		}

		void createMainWindow()
		{
			//
			// Create the window itself.
			//

			int ulCornerX = m_owner->config().desiredWindowRect().left();
			int ulCornerY = m_owner->config().desiredWindowRect().top();
			int width = m_owner->config().desiredWindowRect().right() - m_owner->config().desiredWindowRect().left();
			int height = m_owner->config().desiredWindowRect().bottom() - m_owner->config().desiredWindowRect().top();

			// If ulCornerX or ulCornerY are < 0, that coordinate is set arbitrarily by the operating system.
			//
			if( ulCornerX < 0 )
			{
				ulCornerX = 0;		// TODO center.
			}
			if( ulCornerY < 0 )
			{
				ulCornerY = 0;		// TODO center.
			}
			if( width <= 0 )
			{
				width = 1280;
			}
			if( height <= 0 )
			{
				height = 800;
			}

			// Raspberry Pi Broadcom GPU initialization. See http://www.raspberrypi.org/phpBB3/viewtopic.php?f=63&t=6488
			//
			bcm_host_init();

			
			//
			// Initialize X Server.
			//
			
			ASSERT( !m_display );
			m_display = XOpenDisplay( nullptr );
			if( !m_display )
			{
				FRESH_THROW( FreshException, "Could not create X display." );
			}

			Window root = DefaultRootWindow( m_display );

			XSetWindowAttributes windowAttributes;
			windowAttributes.event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;

			m_window = XCreateWindow( m_display,
									root,
									ulCornerX, ulCornerY,
									width, height,
									0, 				// Border width
									CopyFromParent,
									InputOutput,
									CopyFromParent,
									CWEventMask,	// Properties that were set above in windowAttributes.
									&windowAttributes );

			windowAttributes.override_redirect = false;
			XChangeWindowAttributes( m_display, m_window, CWOverrideRedirect, &windowAttributes );

			XMapWindow( m_display, m_window );

			XStoreName( m_display, m_window, m_owner->config().desiredTitle().c_str() );

			//
			// Initialize EGL stuff (Raspberry Pi-specific)
			//
			
			// Initialize the display.
			//
			m_eglDisplay = eglGetDisplay( EGL_DEFAULT_DISPLAY );
			if( m_eglDisplay == EGL_NO_DISPLAY )
			{
				FRESH_THROW( FreshException, "Got no EGL display." );
			}
			
			if( !eglInitialize( m_eglDisplay, NULL, NULL ))
			{
				FRESH_THROW( FreshException, "Unable to initialize EGL." );
			}
			
			// Find a display configuration.
			//
			EGLint configurationAttributes[] =
			{
				EGL_RED_SIZE, 8,
				EGL_GREEN_SIZE, 8,
				EGL_BLUE_SIZE, 8,
				EGL_ALPHA_SIZE, 8,
				EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_OPENGL_ES2_BIT,
				EGL_NONE
			};
			
			EGLConfig configuration;
			EGLint nConfigurations;
			bool found = eglChooseConfig( m_eglDisplay, configurationAttributes, &configuration, 1, &nConfigurations );
			
			if( !found )
			{
				FRESH_THROW( FreshException, "Failed to choose config (eglError: " << eglGetError() << ")" );
			}
			if( nConfigurations != 1 )
			{
				FRESH_THROW( FreshException, "Didn't get exactly one config, but " << nConfigurations );
			}
			
			// Create the GL rendering context.
			//
			EGLint contextAttributes[] = {
				EGL_CONTEXT_CLIENT_VERSION, 2,
				EGL_NONE
			};
			m_eglContext = eglCreateContext( m_eglDisplay, configuration, EGL_NO_CONTEXT, contextAttributes );
			
			if( m_eglContext == EGL_NO_CONTEXT )
			{
				FRESH_THROW( FreshException, "Unable to create EGL context (eglError: " << eglGetError() << ")" );
			}
			
			// ...And make it current.
			//
			eglMakeCurrent( m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext );
		}

		void handleEvent( const XEvent& event )
		{
			switch( event.type )
			{
				case KeyPress:
				{
					EventKeyboard freshEvent = getFreshKeyboardEvent( event, EventKeyboard::KEY_DOWN );
					Application::instance().onKeyDown( freshEvent );
					onKeyChanged( freshEvent.key(), true );
				}
				break;

				case KeyRelease:
				{
					EventKeyboard freshEvent = getFreshKeyboardEvent( event, EventKeyboard::KEY_UP );
					Application::instance().onKeyUp( freshEvent );
					onKeyChanged( freshEvent.key(), false );
				}
				break;

				case ButtonPress:
				{
					if( event.xbutton.button == Button1 )
					{
						vec2 touchPoint( getTouchPoint( event.xbutton ));
						g_multiTapCounter.onDown( touchPoint );
						Application::Touches appTouches;			
						createAppTouches( touchPoint, touchPoint, appTouches, g_multiTapCounter.tapCount() );
						Application::instance().onTouchesBegin( appTouches.begin(), appTouches.end() );

						g_lastMousePoint = touchPoint;
						g_haveLastMousePoint = true;
					}
				}
				break;

				case ButtonRelease:
				{
					if( event.xbutton.button == Button1 )
					{
						vec2 touchPoint( getTouchPoint( event.xbutton ));
						g_multiTapCounter.onUp( touchPoint );
						Application::Touches appTouches;
						createAppTouches( touchPoint, g_lastMousePoint, appTouches, g_multiTapCounter.tapCount() );			
						Application::instance().onTouchesEnd( appTouches.begin(), appTouches.end() );
						g_haveLastMousePoint = false;
					}
				}
				break;

				case MotionNotify:
				{
					if( event.xmotion.window == m_window && g_haveLastMousePoint )
					{
						vec2 touchPoint( getTouchPoint( event.xmotion ));
						Application::Touches appTouches;			
						createAppTouches( touchPoint, g_lastMousePoint, appTouches, 0 );
						Application::instance().onTouchesMove( appTouches.begin(), appTouches.end() );
						g_lastMousePoint = touchPoint;
						g_haveLastMousePoint = true;
					}
				}
				break;

				case ResizeRequest:
					// TODO
				break;
			}
		}

		Application::ExitCode runMainLoop( int argc, char* argv[] )
		{
			REQUIRES( !m_isInMainLoop );

#if defined( DEV_MODE ) && 1
			m_commandProcessor->startListenServer();
#endif

			createMainWindow();

			m_isInMainLoop = true;

			try
			{
				while( true )
				{
					if( XPending( m_display ))
					{
						XEvent event;
						XNextEvent( m_display, &event );
						handleEvent( event );
					}
					else
					{
						// Is it time for another update?
						if( getAbsoluteTimeClocks() >= m_nextUpdateTime )
						{
							m_nextUpdateTime = getAbsoluteTimeClocks() + m_desiredClocksPerFrame;

							updateFrame();

#ifdef DEBUG
							GLint err = glGetError();
							if( err != GL_NO_ERROR )
							{
								dev_error( "OpenGL Error during update" );
							}
#endif
						}
					}
				}
				m_isInMainLoop = false;
				return 0;
			}
			catch( const std::exception& e )
			{
				m_isInMainLoop = false;
				trace( "Exception while running main loop: " << e.what() );
				return -1;
			}
			catch( ... )
			{
				m_isInMainLoop = false;
				trace( "Unknown exception while running main loop." );
				return -2;
			}
		}

		void swapBuffers()
		{
			ASSERT( hasMainWindow() );
			eglSwapBuffers( m_eglDisplay, m_eglSurface );
		}

		std::string getPromptedFilePath( bool forSaveElseOpen, const char* semicolonSeparatedFileExtensions )		
		{
			// TODO
			return "";
		}

		TimeType getDesiredFramesPerSecond() const
		{
			return m_desiredFramesPerSecond;
		}

		Vector2i getScreenDimensions() const
		{
			return Vector2i( XDisplayWidth( m_display, 0 ), XDisplayHeight( m_display, 0 ));
		}

		Vector2i getWindowDimensions() const
		{
			XWindowAttributes attributes;
			XGetWindowAttributes( m_display, m_window, &attributes );
			return Vector2i( attributes.width, attributes.height );
		}

		real pixelsPerScreenInch() const
		{
			// Assume a 17-in (diagonal) desktop. Could easily be larger or smaller.
			//
			const real monitorWidthInches = 14.57f;			
			return getScreenDimensions().x / monitorWidthInches;
		}

		std::string windowTitle() const
		{
			if( hasMainWindow() )
			{
				char* szName;
				XFetchName( m_display, m_window, &szName );
				std::string name( szName );
				XFree( szName );
				return name;
			}
			else
			{
				return m_owner->config().desiredTitle();
			}
		}
		
		void windowTitle( const std::string& value )
		{
			ASSERT( hasMainWindow() );
			XStoreName( m_display, m_window, value.c_str() );
		}
		
		void goFullscreen( bool fullscreen )
		{
			// TODO
		}
		
		bool isFullscreen() const
		{
			// TODO
			return false;
		}

		bool isMainLoopRunning() const
		{
			return m_isInMainLoop;
		}

		void updateFrame()
		{
			m_owner->updateFrame();
		}

	protected:

		vec2 getTouchPoint( const XButtonEvent& event ) const
		{
			return vec2( event.x, getWindowDimensions().y - event.y );
		}
		
		vec2 getTouchPoint( const XMotionEvent& event ) const
		{
			return vec2( event.x, getWindowDimensions().y - event.y );
		}
		
		void shutdownOpenGL()
		{
			eglDestroyContext( m_eglDisplay, m_eglContext );
			m_eglContext = 0;
			
			eglDestroySurface( m_eglDisplay, m_eglSurface );
			m_eglSurface = 0;
			
			eglTerminate( m_eglDisplay );
			
			XDestroyWindow( m_display, m_window );
			m_window = 0;

			XCloseDisplay( m_display );
			m_display = nullptr;
		}

	private:

		Application* m_owner;
		SmartPtr< CommandProcessor > m_commandProcessor;
		bool m_isInMainLoop;
		TimeType m_desiredFramesPerSecond;

		SystemClock m_nextUpdateTime;
		SystemClock m_desiredClocksPerFrame;
		
		Display* m_display;
		Window m_window;
		EGLDisplay m_eglDisplay;
		EGLContext m_eglContext;
		EGLSurface m_eglSurface;
	};

	std::string Application::getPromptedFilePath( bool forSaveElseOpen, const char* semicolonSeparatedFileExtensions )
	{
		return m_impl->getPromptedFilePath( forSaveElseOpen, semicolonSeparatedFileExtensions );
	}

	void Application::constructImplementation()
	{
		m_impl = new ApplicationImplementation( this, m_commandProcessor );
	}

	void Application::destroyImplementation()
	{
		delete m_impl;
		m_impl = nullptr;
	}

	void Application::quit( ExitCode exitCode /*= 0*/ )
	{
		exit( exitCode );
	}

	void Application::swapBuffers()
	{
		m_impl->swapBuffers();
	}

	Vector2i Application::getScreenDimensions() const
	{
		return m_impl->getScreenDimensions();
	}

	Vector2i Application::getWindowDimensions() const
	{
		return m_impl->getWindowDimensions();
	}

	real Application::pixelsPerScreenInch() const
	{
		return m_impl->pixelsPerScreenInch();
	}

	std::string Application::windowTitle() const
	{
		return m_impl->windowTitle();
	}
	
	void Application::windowTitle( const std::string& value )
	{
		m_impl->windowTitle( value );
	}

	void Application::goFullscreen( bool fullscreen )
	{
		m_impl->goFullscreen( fullscreen );
	}
	
	bool Application::isFullscreen() const
	{
		return m_impl->isFullscreen();
	}

	Application::ExitCode Application::runMainLoop( int argc, char* argv[] )
	{
		REQUIRES( !isMainLoopRunning() );
		return m_impl->runMainLoop( argc, argv );
	}

	bool Application::isMainLoopRunning() const
	{
		return m_impl->isMainLoopRunning();
	}

	std::vector< std::string > Application::getPlatformConfigFileSuffixes() const
	{
		return { "-RPi" };
	}
	
	std::vector< std::string > Application::getVariantConfigFileSuffixes( const std::string& platformSuffix ) const
	{
		return {};
	}
	
	bool Application::isApplicationAlternativeStartupKeyDown() const
	{
		return false;		// TODO Alt key down during startup.
	}
}

