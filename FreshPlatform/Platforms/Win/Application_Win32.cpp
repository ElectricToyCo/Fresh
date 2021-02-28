/*
 *  Application_Win32.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 10/29/12.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#include "../../Application.h"
#include "../../FreshCore/FreshDebug.h"
#include "../../FreshOpenGL.h"
#include "../../FreshCore/FreshFile.h"
#include "../../FreshCore/FreshTime.h"
#include "../../FreshCore/Objects.h"
#include "../../FreshCore/Assets.h"
#include "../../FreshCore/FreshXML.h"
#include "../../FreshCore/CommandProcessor.h"
#include "../../FreshCore/TelnetServer.h"
#include <algorithm>

#if FRESH_TELEMETRY_ENABLED
#	include "UserTelemetry.h"
#endif

#if 0
#	define trace_keys( x ) trace( x )
#else
#	define trace_keys( x )
#endif

namespace
{
	using namespace fr;

	const DWORD WINDOWED_STYLE = WS_OVERLAPPED | WS_BORDER | WS_THICKFRAME | WS_CAPTION | WS_VISIBLE | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
	const DWORD WINDOWED_STYLE_EX = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;

	const real WHEEL_SCALAR = 0.25f;

	unsigned int g_nMouseSequences = 0;		// Counts the number of times that the mouse has been pressed. Roughly corresponds to CocoaTouch's touchId concept.
	vec2 g_lastMousePoint;
	bool g_haveLastMousePoint = false;

	inline bool isWinKeyDown( int vk )
	{
		return ( ::GetAsyncKeyState( vk ) & 0x8000 ) != 0;
	}

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

	// Adapted from <windowsx.h>
	int getXFromLPARAM( LPARAM lParam )
	{
		return ((int)(short)LOWORD( lParam ));
	}
	int getYFromLPARAM( LPARAM lParam )
	{
		return ((int)(short)HIWORD( lParam ));
	}
	int getWheelDeltaFromWPARAM( WPARAM wParam )
	{
		return ((int)(short)HIWORD( wParam ));
	}

	Keyboard::Key getKeyFromWPARAM( WPARAM wParam )
	{
		if( wParam >= '0' && wParam <= 'Z' ) return Keyboard::Key( wParam );		

		switch( wParam )
		{
		case VK_BACK: return Keyboard::Backspace;
		case VK_TAB: return Keyboard::Tab;
		case VK_CLEAR: return Keyboard::NumpadClear;
		case VK_RETURN: return Keyboard::Enter;
		case VK_SHIFT: return Keyboard::Shift;
		case VK_CONTROL: return Keyboard::CtrlCommand;
		case VK_MENU: return Keyboard::AltOption;
		case VK_PAUSE: return Keyboard::PauseBreak;
		case VK_CAPITAL: return Keyboard::CapsLock;
		case VK_ESCAPE: return Keyboard::Escape;
		case VK_SPACE: return Keyboard::Space;
		case VK_PRIOR: return Keyboard::PageUp;
		case VK_NEXT: return Keyboard::PageDown;
		case VK_END: return Keyboard::End;
		case VK_HOME: return Keyboard::Home;
		case VK_LEFT: return Keyboard::LeftArrow;
		case VK_UP: return Keyboard::UpArrow;
		case VK_RIGHT: return Keyboard::RightArrow;
		case VK_DOWN: return Keyboard::DownArrow;
		case VK_INSERT: return Keyboard::Insert;
		case VK_DELETE: return Keyboard::Delete;
		case VK_NUMPAD0: return Keyboard::Numpad0;
		case VK_NUMPAD1: return Keyboard::Numpad1;
		case VK_NUMPAD2: return Keyboard::Numpad2;
		case VK_NUMPAD3: return Keyboard::Numpad3;
		case VK_NUMPAD4: return Keyboard::Numpad4;
		case VK_NUMPAD5: return Keyboard::Numpad5;
		case VK_NUMPAD6: return Keyboard::Numpad6;
		case VK_NUMPAD7: return Keyboard::Numpad7;
		case VK_NUMPAD8: return Keyboard::Numpad8;
		case VK_NUMPAD9: return Keyboard::Numpad9;
		case VK_MULTIPLY: return Keyboard::NumpadMultiply;
		case VK_ADD: return Keyboard::NumpadAdd;
		case VK_SUBTRACT: return Keyboard::NumpadSubtract;
		case VK_DECIMAL: return Keyboard::NumpadDecimal;
		case VK_DIVIDE: return Keyboard::NumpadDivide;
		case VK_F1: return Keyboard::F1;
		case VK_F2: return Keyboard::F2;
		case VK_F3: return Keyboard::F3;
		case VK_F4: return Keyboard::F4;
		case VK_F5: return Keyboard::F5;
		case VK_F6: return Keyboard::F6;
		case VK_F7: return Keyboard::F7;
		case VK_F8: return Keyboard::F8;
		case VK_F9: return Keyboard::F9;
		case VK_F10: return Keyboard::F10;
		case VK_F11: return Keyboard::F11;
		case VK_F12: return Keyboard::F12;
		case VK_F13: return Keyboard::F13;
		case VK_F14: return Keyboard::F14;
		case VK_F15: return Keyboard::F15;
		case VK_SCROLL: return Keyboard::ScrollLock;
		case VK_LSHIFT: return Keyboard::LeftShift;
		case VK_RSHIFT: return Keyboard::RightShift;
		case VK_LCONTROL: return Keyboard::LeftCtrlCommand;
		case VK_RCONTROL: return Keyboard::RightCtrlCommand;
		case VK_LMENU: return Keyboard::LeftAltOption;
		case VK_RMENU: return Keyboard::RightAltOption;
		case VK_OEM_1: return Keyboard::Semicolon;      
		case VK_OEM_PLUS: return Keyboard::Equal;
		case VK_OEM_COMMA: return Keyboard::Comma;
		case VK_OEM_MINUS: return Keyboard::Minus;
		case VK_OEM_PERIOD: return Keyboard::Period;
		case VK_OEM_2: return Keyboard::Slash;
		case VK_OEM_3: return Keyboard::Backtick;
		case VK_OEM_4: return Keyboard::RightBracket;
		case VK_OEM_5: return Keyboard::Backslash;
		case VK_OEM_6: return Keyboard::LeftBracket;
		case VK_OEM_7: return Keyboard::Quote;
		default:
			return Keyboard::Unsupported;
		}
	}

	unsigned int getCharFromWPARAM( WPARAM wParam, bool shift )
	{
		if( wParam >= '0' && wParam <= '9' )
		{
			return wParam;
		}
		if( wParam >= VK_NUMPAD0 && wParam <= VK_NUMPAD9 )
		{
			return '0' + ( wParam - VK_NUMPAD0 );
		}
		if( wParam >= 'A' && wParam <= 'Z' ) 
		{
			return shift ? wParam : ( wParam + ( 'a' - 'A' ));
		}

		switch( wParam )
		{
		case VK_SPACE: return ' ';
		case VK_MULTIPLY: return '*';
		case VK_ADD: return '+';
		case VK_SUBTRACT: return '-';
		case VK_DECIMAL: return '.';
		case VK_DIVIDE: return '/';
		case VK_OEM_1: return shift ? ':' : ';';      
		case VK_OEM_PLUS: return shift ? '+' : '=';
		case VK_OEM_COMMA: return shift ? '<' : ',';
		case VK_OEM_MINUS: return shift ? '_' : '-';
		case VK_OEM_PERIOD: return shift ? '>' : '.';
		case VK_OEM_2: return shift ? '?' : '/';
		case VK_OEM_3: return shift ? '~' : '`';
		case VK_OEM_4: return shift ? '}' : ']';
		case VK_OEM_5: return shift ? '|' : '\\';
		case VK_OEM_6: return shift ? '{' : '[';
		case VK_OEM_7: return shift ? '\"' : '\'';
		default:
			return 0;
		}
	}

	vec2 getTouchPoint( HWND hWnd, const vec2& point )
	{
		// Flip in Y.
		RECT rect;
		::GetClientRect( hWnd, &rect );
		return vec2( point.x, rect.bottom - point.y );
	}

	vec2 getTouchPoint( HWND hWnd, LPARAM lParam )
	{
		return getTouchPoint( hWnd, vec2( (real) getXFromLPARAM( lParam ), (real) getYFromLPARAM( lParam )));
	}
	
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

	LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
	{
		if( !Application::doesExist() )
		{
			return ::DefWindowProc(hWnd, message, wParam, lParam);
		}
	
		switch( message )
		{
		case WM_SETFOCUS:
			if( Application::doesExist() )
			{
				Application::instance().onGainedFocus();
			}
			break;
		case WM_KILLFOCUS:
			if( Application::doesExist() )
			{
				Application::instance().onLostFocus();
			}
			break;
		case WM_LBUTTONDOWN:
			{
				++g_nMouseSequences;

				vec2 touchPoint( getTouchPoint( hWnd, lParam ));

				g_multiTapCounter.onDown( touchPoint );

				Application::Touches appTouches;
		
				createAppTouches( touchPoint, touchPoint, appTouches, g_multiTapCounter.tapCount() );
		
				Application::instance().onTouchesBegin( appTouches.begin(), appTouches.end() );

				g_lastMousePoint = touchPoint;
				g_haveLastMousePoint = true;
				return 0;
			}
		case WM_LBUTTONUP:
			{
				vec2 touchPoint( getTouchPoint( hWnd, lParam ));

				g_multiTapCounter.onUp( touchPoint );

				Application::Touches appTouches;

				createAppTouches( touchPoint, g_lastMousePoint, appTouches, g_multiTapCounter.tapCount() );
		
				Application::instance().onTouchesEnd( appTouches.begin(), appTouches.end() );

				g_haveLastMousePoint = false;
				return 0;
			}
		case WM_MOUSEMOVE:
			{
				vec2 touchPoint( getTouchPoint( hWnd, lParam ));

				Application::Touches appTouches;
		
				createAppTouches( touchPoint, g_lastMousePoint, appTouches, 0 );
		
				Application::instance().onTouchesMove( appTouches.begin(), appTouches.end() );

				g_lastMousePoint = touchPoint;
				g_haveLastMousePoint = true;
				return 0;
			}
		case WM_MOUSELEAVE:
			{
				Application::Touches appTouches;

				createAppTouches( g_lastMousePoint, g_lastMousePoint, appTouches, 0 );

				Application::instance().onTouchesEnd( appTouches.begin(), appTouches.end() );

				return 0;
			}
		case WM_MOUSEWHEEL:
			{
				Application::Touches appTouches;

				// lParam mouse position is incorrect for multi-monitor setups (as Microsoft's help docs admit.)
				// Use GetCursorPos().

				POINT mouse;
				::GetCursorPos( &mouse );
				::ScreenToClient( hWnd, &mouse );

				vec2 touchPoint( getTouchPoint( hWnd, vec2( (real) mouse.x, (real) mouse.y )));

				vec2 wheelDelta( 0, -WHEEL_SCALAR * getWheelDeltaFromWPARAM( wParam ));

				appTouches.push_back( Application::Touch( 
															touchPoint,
															g_lastMousePoint,
															wheelDelta,
															0,	// Which touch in the current group?
															1,	// How many touches in the current group? (Always at most 1 for mouse.)
															0,	// Click count (e.g. single click vs. double click. Assume 1.)
															reinterpret_cast< void* >( g_nMouseSequences )));

				Application::instance().onWheelMove( appTouches.begin(), appTouches.end() );
				g_lastMousePoint = touchPoint;
				return 0;
			}
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			{
				Keyboard::Key key = getKeyFromWPARAM( wParam );

				trace_keys( "KEYDOWN: " << key << " wParam: " << wParam );

				const bool shift = isWinKeyDown( VK_SHIFT );
				unsigned int c = getCharFromWPARAM( wParam, shift );
				const bool isAHeldRepeat = ( lParam & ( 1 << 30 )) != 0;

  				if( key != Keyboard::Unsupported )
				{
					Keyboard::onKeyStateChanged( key, true );
				}
				
				EventKeyboard event( EventKeyboard::KEY_DOWN, nullptr, c, key, isWinKeyDown( VK_MENU ), isWinKeyDown( VK_CONTROL ), shift, isAHeldRepeat );
				Application::instance().onKeyDown( event );
				return 0;
			}
		case WM_KEYUP:
		case WM_SYSKEYUP:
			{
				Keyboard::Key key = getKeyFromWPARAM( wParam );

				trace_keys( "KEYUP: " << key << " wParam: " << wParam );

				// Quit if using the standard system shortcut.
				//
				if( key == Keyboard::F4 && isWinKeyDown( VK_MENU ))
				{
					::PostQuitMessage( 0 );
				}
				else
				{			
					if( key != Keyboard::Unsupported )
					{
						Keyboard::onKeyStateChanged( key, false );
					}

					const bool shift = isWinKeyDown( VK_SHIFT );
					unsigned int c = getCharFromWPARAM( wParam, shift );
					EventKeyboard event( EventKeyboard::KEY_UP, nullptr, c, key, isWinKeyDown( VK_MENU ), isWinKeyDown( VK_CONTROL ), shift, false /* up is never a repeat */ );
					Application::instance().onKeyUp( event );
				}
				return 0;
			}
		case WM_SIZE:
			{
				fr::Application::instance().onResize( getXFromLPARAM( lParam ), getYFromLPARAM( lParam ));
				return 0;
			}
		case WM_DESTROY:
			::PostQuitMessage( 0 );
			return 0;
		default:
			return ::DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}
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
		,	m_hWnd( 0 )
		,	m_hDC( 0 )
		,	m_hGLRenderingContext( 0 )
		{
			REQUIRES( owner );

			m_desiredClocksPerFrame = secondsToClocks( 1.0 / m_desiredFramesPerSecond );
		}

		~ApplicationImplementation()
		{
//			shutdownOpenGL();		// Don't bother shutting down GL. The OS will handle this, and doing this here causes ordering problems with global objects that are
									// destroyed after the application is.
			::ReleaseDC( m_hWnd, m_hDC );
		}

		bool hasMainWindow() const
		{
			return m_hWnd && m_hDC;
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
				ulCornerX = CW_USEDEFAULT;
			}
			if( ulCornerY < 0 )
			{
				ulCornerY = CW_USEDEFAULT;
			}
			if( width <= 0 )
			{
				width = 1280;
			}
			if( height <= 0 )
			{
				height = 800;
			}

			RECT windowRect = { 0, 0, width, height };

			// Determin the application instance.
			//
			HINSTANCE hInstance = ::GetModuleHandle( NULL );

			// Register the window class.
			//
			const char WINDOW_CLASS_NAME[] = "WindowClass";

			WNDCLASSEX wcx;
			::ZeroMemory( &wcx, sizeof( WNDCLASSEX ) );

			wcx.cbSize = sizeof( WNDCLASSEX );
			wcx.lpszClassName = WINDOW_CLASS_NAME;
			wcx.hInstance = hInstance;
			wcx.lpfnWndProc = WndProc;
			wcx.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
			wcx.cbClsExtra = 0;
			wcx.cbWndExtra = 0;
			wcx.hbrBackground = NULL;
			wcx.lpszMenuName = NULL;
			wcx.hCursor = ::LoadCursor( NULL, IDC_ARROW );
			wcx.hIcon = ::LoadIcon( NULL, IDI_APPLICATION );
			wcx.hIconSm = ::LoadIcon( NULL, IDI_APPLICATION );

			::RegisterClassEx( &wcx );

			// Create and position the window.
			//
			::AdjustWindowRectEx( &windowRect, WINDOWED_STYLE, FALSE, WINDOWED_STYLE_EX );

			// We don't intend the UL corner to go into negative space, so push the rect
			// out into positive space.
			if( static_cast< int >( windowRect.right ) != CW_USEDEFAULT )
			{
				windowRect.right += windowRect.left;
			}
			if( static_cast< int >( windowRect.bottom ) != CW_USEDEFAULT )
			{
				windowRect.bottom += windowRect.top;
			}

			m_hWnd = ::CreateWindowEx(
				WINDOWED_STYLE_EX,
				wcx.lpszClassName,
				m_owner->config().desiredTitle().c_str(),
				WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WINDOWED_STYLE,
				ulCornerX,
				ulCornerY,
				windowRect.right - windowRect.left,
				windowRect.bottom - windowRect.top,
				NULL, NULL, wcx.hInstance, NULL );

			ASSERT( m_hWnd != NULL );

			m_hDC = ::GetDC( m_hWnd );
			ASSERT( m_hDC != NULL );

			initializeOpenGL();
			ASSERT( m_hGLRenderingContext != 0 );

			GLint err = glGetError();
			if( err != GL_NO_ERROR )
			{
				dev_error( "Failed to Initialize OpenGL" );
			}

			::ShowWindow( m_hWnd, true );
			::UpdateWindow( m_hWnd );
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
				MSG msg;
				while( true )
				{
					if( ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ))
					{
						if( msg.message == WM_QUIT )
						{
							break;
						}

						::TranslateMessage( &msg );
						::DispatchMessage( &msg );
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
				return (int) msg.wParam;
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
			::SwapBuffers( m_hDC );
		}

		std::string getPromptedFilePath( bool forSaveElseOpen, const char* semicolonSeparatedFileExtensions )		
		{
			char initialFilter[] = "All Files\0.*\0Fresh Files";

			std::vector< char > filter( initialFilter, initialFilter + sizeof( initialFilter ));	// Include the null terminator.

			if( semicolonSeparatedFileExtensions )
			{
				filter.insert( filter.end(), semicolonSeparatedFileExtensions, semicolonSeparatedFileExtensions + strlen( semicolonSeparatedFileExtensions ) + 1 );	// Including terminator

				// Expand dots to be preceded with a wildcard * symbol.
				//
				for( size_t i = 0; i < filter.size(); ++i )
				{
					const char c = filter[ i ];
					if( c == '.' )
					{
						filter.insert( filter.begin() + i, '*' );
						++i;
					}
				}
			}

			filter.push_back( '\0' );

			::OPENFILENAMEA openFileName = {0};
			char szFilePath[ MAX_PATH ] = { '\0' };

			openFileName.lStructSize = sizeof( openFileName );
			openFileName.hwndOwner = m_hWnd;
			openFileName.lpstrFile = szFilePath;
			openFileName.nMaxFile = MAX_PATH;
			openFileName.lpstrFilter = filter.data();
			openFileName.nFilterIndex = 2;
			openFileName.lpstrFileTitle = NULL;
			openFileName.nMaxFileTitle = 0;
			openFileName.lpstrInitialDir = NULL;
			openFileName.Flags = ( OFN_HIDEREADONLY | OFN_PATHMUSTEXIST ) | ( forSaveElseOpen ? 0 : ( OFN_FILEMUSTEXIST ));

			bool okPressed = false;

			if( forSaveElseOpen )
			{
				okPressed = ::GetSaveFileNameA( &openFileName ) == TRUE;
			}
			else
			{
				okPressed = ::GetOpenFileNameA( &openFileName ) == TRUE;
			}

			if( okPressed )
			{
				return openFileName.lpstrFile;
			}
			else
			{
				return std::string();
			}
		}

		void desiredFramesPerSecond( TimeType fps )
		{
			m_desiredFramesPerSecond = fps;
			m_desiredClocksPerFrame = secondsToClocks( 1.0 / m_desiredFramesPerSecond );
		}

		TimeType getDesiredFramesPerSecond() const
		{
			return m_desiredFramesPerSecond;
		}

		Vector2i getScreenDimensions() const
		{
			RECT rect;
			::GetWindowRect( ::GetDesktopWindow(), &rect );
			return Vector2i( rect.right, rect.bottom );
		}

		Vector2i getWindowDimensions() const
		{
			RECT rect;
			::GetClientRect( m_hWnd, &rect );
			return Vector2i( rect.right, rect.bottom );
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
				const size_t len = 2048;
				char text[ len ];
				::GetWindowTextA( m_hWnd, text, len );
				return text;
			}
			else
			{
				return m_owner->config().desiredTitle();
			}
		}
		
		void windowTitle( const std::string& value )
		{
			ASSERT( hasMainWindow() );
			::SetWindowTextA( m_hWnd, value.c_str() );
		}
		
		bool isMainLoopRunning() const
		{
			return m_isInMainLoop;
		}

		bool isFullscreen() const
		{
			return m_isFullscreen;
		}

		void goFullscreen( bool fullscreenElseWindowed )
		{
			static const DWORD WINDOWED_STYLE = WS_OVERLAPPEDWINDOW | WS_BORDER | WS_VISIBLE;
			static const DWORD WINDOWED_STYLE_EX = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;

			::ShowWindow( m_hWnd, SW_HIDE );

			int resolutionX = m_owner->config().desiredWindowRect().right() - m_owner->config().desiredWindowRect().left();
			int resolutionY = m_owner->config().desiredWindowRect().bottom() - m_owner->config().desiredWindowRect().top();

			if( fullscreenElseWindowed )
			{
				::DEVMODE dmScreenSettings = {{0}};
				dmScreenSettings.dmSize			= sizeof( dmScreenSettings );   
				dmScreenSettings.dmPelsWidth    = resolutionX;          
				dmScreenSettings.dmPelsHeight   = resolutionY;          
				dmScreenSettings.dmBitsPerPel   = 32;                   
				dmScreenSettings.dmFields	= DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

				BOOL success = ::ChangeDisplaySettings( &dmScreenSettings, CDS_FULLSCREEN );

				if( success != DISP_CHANGE_SUCCESSFUL )
				{
					::ShowWindow( m_hWnd, SW_SHOW );
					release_error( "Could not switch to the requested full screen mode." );
				}
				else
				{
					// Successful. Set window styles.
					//
					::SetWindowLong( m_hWnd, GWL_STYLE, WS_POPUP );
					::SetWindowLong( m_hWnd, GWL_EXSTYLE, WS_EX_APPWINDOW );

					// Hide the mouse cursor (optional).
					//
					::ShowCursor( FALSE );

					// Set window size and position.
					// This will cause a WM_SIZE message to come in, which should
					// update the OpenGL viewport and perhaps the projection.
					//
					::SetWindowPos( m_hWnd, HWND_TOP, 0, 0, resolutionX, resolutionY, 
						SWP_SHOWWINDOW | SWP_FRAMECHANGED );

					m_isFullscreen = true;
				}
			}
			else
			{
				::ChangeDisplaySettings( NULL, 0 );

				// Set window styles.
				//
				::SetWindowLong( m_hWnd, GWL_STYLE, WINDOWED_STYLE );
				::SetWindowLong( m_hWnd, GWL_EXSTYLE, WINDOWED_STYLE_EX );

				// Set window size and position.
				//
				RECT windowRect;
				windowRect.top = 0;
				windowRect.left = 0;
				windowRect.right = resolutionX;
				windowRect.bottom = resolutionY;
			    
				// Adjust the overall window size to support a client of the desired
				// size.
				::AdjustWindowRectEx( &windowRect, 
					WINDOWED_STYLE, FALSE, WINDOWED_STYLE_EX );

				::SetWindowPos( m_hWnd, HWND_TOP, 
					0, 0,		// UL corner of the window. Could change this.
					windowRect.right - windowRect.left, 
					windowRect.bottom - windowRect.top, 
					SWP_SHOWWINDOW | SWP_FRAMECHANGED );

				// Show the mouse cursor (optional).
				//
				::ShowCursor( TRUE );

				// Make the window receive keyboard events.
				//
				::SetFocus( m_hWnd );

				m_isFullscreen = false;
			}
		}

		void updateFrame()
		{
			m_owner->updateFrame();
		}

	protected:

		void initializeOpenGL()
		{
			ASSERT( m_hDC );

			// Set the window pixel format
			//
			PIXELFORMATDESCRIPTOR pixelFormatDescriptor = {0};

			pixelFormatDescriptor.nSize = sizeof( pixelFormatDescriptor );
			pixelFormatDescriptor.nVersion = 1;

			pixelFormatDescriptor.dwFlags =
				PFD_DRAW_TO_WINDOW |
				PFD_SUPPORT_OPENGL |
				PFD_DOUBLEBUFFER;
			pixelFormatDescriptor.dwLayerMask = PFD_MAIN_PLANE;
			pixelFormatDescriptor.iPixelType = PFD_TYPE_RGBA;
			pixelFormatDescriptor.cColorBits = 32;
			pixelFormatDescriptor.cDepthBits = 0;	// No depth buffer for now.
			pixelFormatDescriptor.cStencilBits = 8;

			int pixelFormat = ::ChoosePixelFormat( m_hDC, &pixelFormatDescriptor );
			ASSERT( pixelFormat != 0 );
			::SetPixelFormat( m_hDC, pixelFormat, &pixelFormatDescriptor );

			// Create the OpenGL render context
			//
			m_hGLRenderingContext = wglCreateContext( m_hDC );
			wglMakeCurrent ( m_hDC, m_hGLRenderingContext );
		}

		void shutdownOpenGL()
		{
			wglMakeCurrent( NULL, NULL );
			wglDeleteContext( m_hGLRenderingContext );
		}

	private:

		Application* m_owner = nullptr;
		SmartPtr< CommandProcessor > m_commandProcessor;
		bool m_isInMainLoop = false;
		bool m_isFullscreen = false;
		TimeType m_desiredFramesPerSecond = 0;

		SystemClock m_nextUpdateTime = 0;
		SystemClock m_desiredClocksPerFrame = 0;
		HWND m_hWnd = NULL;
		HDC m_hDC = NULL;
		HGLRC m_hGLRenderingContext = NULL;
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
		::PostQuitMessage( exitCode );
	}

	bool Application::isMultitouch() const
	{
		return false;	// No multitouch on PC.
	}
	
	void Application::swapBuffers()
	{
		m_impl->swapBuffers();
	}

	rect Application::safeAreaInsets() const
	{
		return {};
	}
		
	std::string Application::userLanguageCode() const
	{
		// TODO
		return "";
	}

	Vector2i Application::getScreenDimensions() const
	{
		return m_impl->getScreenDimensions();
	}

	Vector2i Application::getWindowDimensions() const
	{
		return m_impl->getWindowDimensions();
	}

	void Application::desiredFramesPerSecondDetail( TimeType fps )
	{
		m_impl->desiredFramesPerSecond( fps );
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

	Application::ExitCode Application::runMainLoop( int argc, char* argv[] )
	{
		REQUIRES( !isMainLoopRunning() );
		return m_impl->runMainLoop( argc, argv );
	}

	bool Application::isMainLoopRunning() const
	{
		return m_impl->isMainLoopRunning();
	}

	bool Application::isFullscreen() const
	{
		return m_impl->isFullscreen();
	}

	void Application::goFullscreen( bool fullscreenElseWindowed )
	{
		m_impl->goFullscreen( fullscreenElseWindowed );
	}

	std::vector< std::string > Application::getPlatformConfigFileSuffixes() const
	{
		return { "-Win" };
	}

	std::vector< std::string > Application::getVariantConfigFileSuffixes( const std::string& platformSuffix ) const
	{
		return {};
	}

	bool Application::isApplicationAlternativeStartupKeyDown() const
	{
		return isWinKeyDown( VK_LMENU );
	}
}

