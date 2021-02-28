/*
 *  Application.mm
 *  Fresh
 *
 *  Created by Jeff Wofford on 12/2/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#include "Application.h"
#include "FreshDebug.h"
#include "FreshOpenGL.h"
#include "FreshFile.h"
#include "Package.h"
#include "Assets.h"
#include "CommandProcessor.h"
#include "TelnetServer.h"
#import <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>

#if FRESH_TELEMETRY_ENABLED
#	include "UserTelemetry.h"
#endif

namespace
{
    // Thread-synchronized input handling.
    //
    enum class TouchType
    {
        Begin,
        Move,
        End,
        Wheel
    };
    fr::Application::Touches g_appTouches[ 4 ];
    
    std::vector< fr::EventKeyboard > g_appKeyboardEvents;
    
    std::mutex g_inputMutex;
    
    void addKeyEvent( const fr::EventKeyboard& event )
    {
        std::lock_guard< std::mutex > guard( g_inputMutex );
        g_appKeyboardEvents.push_back( event );
    }

    template< typename FnT >
    void withTouches( TouchType type, FnT&& fn )
    {
        std::lock_guard< std::mutex > guard( g_inputMutex );
        fn( g_appTouches[ static_cast< int >( type ) ] );
    }
    
    void sendInputEvents()
    {
        auto& app = fr::Application::instance();
        
        std::lock_guard< std::mutex > guard( g_inputMutex );
        
        // Send touches
        //
        {
            auto& appTouches = g_appTouches[ static_cast< int >( TouchType::Begin )];

            app.onTouchesBegin( appTouches.begin(), appTouches.end() );
            appTouches.clear();
        }
        {
            auto& appTouches = g_appTouches[ static_cast< int >( TouchType::Move )];
            
            app.onTouchesMove( appTouches.begin(), appTouches.end() );
            appTouches.clear();
        }
        {
            auto& appTouches = g_appTouches[ static_cast< int >( TouchType::End )];
            app.onTouchesEnd( appTouches.begin(), appTouches.end() );
            appTouches.clear();
        }
        {
            auto& appTouches = g_appTouches[ static_cast< int >( TouchType::Wheel )];
            app.onWheelMove( appTouches.begin(), appTouches.end() );
            appTouches.clear();
        }
        
        // Send keyboard events
        //
        for( const auto& event : g_appKeyboardEvents )
        {
            if( event.type() == fr::EventKeyboard::KEY_DOWN )
            {
                app.onKeyDown( event );
            }
            else
            {
                app.onKeyUp( event );
            }
        }
        g_appKeyboardEvents.clear();
    }
    
    inline std::string castString( NSString* string )
	{
		return string != nil ? [string UTF8String] : std::string{};
	}

	inline NSString* castString( const std::string& string )
	{
		return [NSString stringWithUTF8String: string.c_str() ];
	}
}

/* NSApplication override ************************************************************************************************************/

@interface FreshApplication : NSApplication< NSApplicationDelegate >
@end

@implementation FreshApplication

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication
{
	return YES;
}

- (void)sendEvent:(NSEvent *)event
{
    if ([event type] == NSKeyUp && ([event modifierFlags] & NSCommandKeyMask))
	{
		if( [self keyWindow] != nil )
		{
			trace( "hasKeyWindow" );
		}
        [[self keyWindow] sendEvent:event];
	}
    else
	{
        [super sendEvent:event];
	}
}

- (void)terminate:(id)sender
{
	fr::Application::instance().onTerminating();
	[super terminate: sender];
}

- (void)deactivate
{
	fr::Application::instance().onTerminationThreat();	
	[super deactivate];
}

@end

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* OpenGL View Class *****************************************************************************************************************/

@interface FreshGLView : NSOpenGLView <NSWindowDelegate>
{
	fr::Application* application;
}

- (void)setApplication:( fr::Application*) app frameRate:(int)fps;
- (void) setFPS: (int) fps;
- (IBAction) toggleFullScreen: (id) sender;

@end

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace fr
{
	class ApplicationImplementation
	{
	public:
		
		ApplicationImplementation( Application* owner )
		:	m_owner( owner )
		,	m_window( nil )
		,	m_glView( nil )
		,	m_isInMainLoop( false )
		{
			REQUIRES( owner );
			
			FreshApplication* nsApp = (FreshApplication*) [FreshApplication sharedApplication];
			[nsApp setDelegate: nsApp];
		}
		
		~ApplicationImplementation()
		{
			shutdownOpenGL();
			
			m_window = nil;
		}
		
		bool hasMainWindow() const
		{
			return m_window != nil && m_glView != nil;
		}
		
		void createMainWindow( const std::string& title, bool fullscreen, int width = -1, int height = -1, int ulCornerX = -1, int ulCornerY = -1 )
		{
			REQUIRES( !hasMainWindow() );
			
			NSRect screenRect = [[NSScreen mainScreen] frame];

			// Set arbitrary defaults
			//
			if( width <= 0 )
			{
				width = 960;	
			}

			if( height <= 0 )
			{
				height = 640;
			}
			
			if( ulCornerX < 0 )
			{
				ulCornerX = screenRect.size.width * 0.5f - width / 2;
			}
			if( ulCornerY < 0 )
			{
				ulCornerY = screenRect.size.height * 0.5f - height / 2;
			}
			
			NSRect contentRect = NSMakeRect( ulCornerX, ulCornerY, width, height );
			
			const NSUInteger style = NSTitledWindowMask | NSClosableWindowMask | NSResizableWindowMask;
			
			// Create the window.
			//
			m_window = [[NSWindow alloc] 
						initWithContentRect:contentRect
						styleMask:style
						backing:NSBackingStoreBuffered
						defer:NO ];
			ASSERT( m_window != nil );
			
			// Control window sizing.
			//
			[m_window setContentAspectRatio: NSMakeSize( width, height ) ];
			
			const float MIN_WINDOW_WIDTH = 320;
			[m_window setContentMinSize: NSMakeSize( MIN_WINDOW_WIDTH, MIN_WINDOW_WIDTH * ((float)height/width))];
			
			[m_window setAcceptsMouseMovedEvents: NO];	// Mouse move events disabled. Touch interfaces don't really deal with move events--only drags.
			
			initializeOpenGL();
			
			GLint err = glGetError(); 
			if (err != GL_NO_ERROR) 
			{
				con_error( "Failed to Initialize OpenGL" );
			}
			
			windowTitle( title );

			[m_window makeKeyAndOrderFront:nil];
			
			goFullscreen( fullscreen );
			
			PROMISES( hasMainWindow() );		
		}
		
		Application::ExitCode runMainLoop( int argc, const char* argv[], bool fullscreen )
		{
			REQUIRES( !m_isInMainLoop );
			
			const auto& windowRect = m_owner->config().desiredWindowRect();
			createMainWindow( m_owner->config().desiredTitle(), fullscreen, windowRect.width(), windowRect.height() );
			
			m_isInMainLoop = true;
			
			int frameRate = static_cast< int >( m_owner->config().desiredFramesPerSecond() );
			[m_glView setApplication: m_owner frameRate: frameRate];
			
			const int exitCode = NSApplicationMain( argc, argv );
			
			m_isInMainLoop = false;
			
			return exitCode;		
		}
		
		Vector2i getScreenDimensions() const
		{
			NSRect screenRect = [[NSScreen mainScreen] frame];
			return Vector2i( screenRect.size.width, screenRect.size.height );
		}
		
		Vector2i getWindowDimensions() const
		{
			ASSERT( hasMainWindow() );
			@autoreleasepool
			{
				const NSRect contentRect = [m_glView frame];
				const NSRect backingRect = [m_glView convertRectToBacking:contentRect];

				return Vector2i( backingRect.size.width, backingRect.size.height );

			} // autoreleasepool
		}

		void desiredFramesPerSecond( TimeType fps )
		{
			[m_glView setFPS: static_cast< int >( fps )];
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
				return [[m_window title] UTF8String];
			}
			else
			{
				return m_owner->config().desiredTitle();
			}
		}
		
		void windowTitle( const std::string& value )
		{
			ASSERT( hasMainWindow() );
			
			[m_window setTitle: castString( value.c_str() )];
		}
		
		void goFullscreen( bool fullscreen )
		{
			if( isFullscreen() != fullscreen )
			{
				if( fullscreen )
				{
					[m_glView enterFullScreenMode: m_glView.window.screen withOptions: nil];
				}
				else
				{
					[m_glView exitFullScreenModeWithOptions: nil];
				}
			}
		}
		
		bool isFullscreen() const
		{
			return [m_glView isInFullScreenMode];
		}
		
		bool isMainLoopRunning() const
		{
			return m_isInMainLoop;
		}
		
		void swapBuffers()
		{
			CGLFlushDrawable( CGLGetCurrentContext() );
		}
		
	protected:
		
		void initializeOpenGL()
		{
			NSOpenGLPixelFormatAttribute attributes[] =
			{
				NSOpenGLPFADoubleBuffer,
				NSOpenGLPFAAccelerated,
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
				NSOpenGLPFAAllowOfflineRenderers,
#endif
				NSOpenGLPFADepthSize, 24,
				NSOpenGLPFAStencilSize, 8,
				0
			};
			
			NSOpenGLPixelFormat *pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
			
			NSRect contentRect = [[NSScreen mainScreen] visibleFrame];
			
			m_glView = [[FreshGLView alloc] initWithFrame:contentRect
												  pixelFormat:pixelFormat];
			ASSERT( m_glView != nil );
			
			ASSERT( m_window != nil );
			[m_window setContentView: m_glView];
			[m_window setDelegate: m_glView];
		}
		
		void shutdownOpenGL()
		{
			m_glView = nil;
		}
		
	private:

		Application* m_owner;
		NSWindow* m_window;
		FreshGLView* m_glView;
		bool m_isInMainLoop;
	};

	///////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////

	void Application::constructImplementation()
	{
		m_impl = new ApplicationImplementation( this );
	}

	void Application::destroyImplementation()
	{
		delete m_impl;
		m_impl = nullptr;
	}
	
	real Application::pixelsPerScreenInch() const
	{
		return m_impl->pixelsPerScreenInch();
	}
	
	std::string Application::getPromptedFilePath( bool forSaveElseOpen, const char* fileFilters )
	{
		NSMutableArray* filters = nil;
		
		if( fileFilters )
		{
			filters = [NSMutableArray array];
			
			std::istringstream strFilters( fileFilters );
			while( strFilters )
			{
				std::string filter;
				std::getline( strFilters, filter, ';' );
				if( filter.empty() == false )
				{
					if( filter.front() == '.' )	// Remove leading extensions.
					{
						filter.erase( filter.begin() );
					}
					
					[filters addObject: castString( filter )];
				}
			}
		}
		
		NSSavePanel* panel = nil;
		if( forSaveElseOpen )
		{
			panel = [NSSavePanel savePanel];
		}
		else
		{
			NSOpenPanel* openPanel = [NSOpenPanel openPanel];;
			panel = openPanel;
			[openPanel setAllowsMultipleSelection: NO];
			[openPanel setCanChooseFiles: YES];
			[openPanel setCanChooseDirectories: NO];
		}

		[panel setAllowsOtherFileTypes: YES];
		[panel setAllowedFileTypes: filters];
		NSInteger result = [panel runModal];
		
		if( result == NSFileHandlingPanelOKButton )
		{
			NSURL* fileURL = [panel URL];
			if( fileURL && [fileURL isFileURL] )
			{
				return std::string( [[fileURL path] UTF8String]);
			}
			else
			{
				return "";
			}
		}
		else
		{
			return "";
		}
	}
		
	void Application::quit( ExitCode exitCode /*= 0*/ )
	{
		exit( exitCode );
	}
	
	bool Application::isMultitouch() const
	{
		return false;	// No multitouch on Mac.
	}
	
	rect Application::safeAreaInsets() const
	{
		return {};
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
	
	void Application::desiredFramesPerSecondDetail( TimeType fps )
	{
		m_impl->desiredFramesPerSecond( fps );
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
		// Record our preference along with going full screen.
		// We record a preference of going fullscreen after it's successful,
		// and a preference of going windowed before,
		// in order to avoid problems with fullscreen locking out the OS
		// on launch.
		//
		if( !fullscreen ) wantsFullScreenStartup( fullscreen );
		m_impl->goFullscreen( fullscreen );
		if(  fullscreen ) wantsFullScreenStartup( fullscreen );
	}
	
	bool Application::isFullscreen() const
	{
		return m_impl->isFullscreen();
	}
	
	Application::ExitCode Application::runMainLoop( int argc, const char* argv[] )
	{
		REQUIRES( !isMainLoopRunning() );

		try
		{
			@autoreleasepool
			{
#if DEV_MODE && 1
				m_commandProcessor->startListenServer();
#endif
				return m_impl->runMainLoop( argc, argv, wantsFullScreenStartup() );
			}
		}
		catch( const std::exception& e )
		{
			release_error( "std::exception with message '" << e.what() << "'." );
			return -1;
		}
		catch( ... )
		{
			release_error( "Unknown exception." );
			return -2;
		}
	}
	
	bool Application::isMainLoopRunning() const
	{
		return m_impl->isMainLoopRunning();
	}

	std::vector< std::string > Application::getPlatformConfigFileSuffixes() const
	{
		return { "-Mac" };
	}
	
	std::vector< std::string > Application::getVariantConfigFileSuffixes( const std::string& platformSuffix ) const
	{
		return {};
	}
	
	bool Application::isApplicationAlternativeStartupKeyDown() const
	{
		return GetCurrentKeyModifiers() & ::optionKey;
	}
	
	std::string Application::userLanguageCode() const
	{
		return castString( [[NSLocale currentLocale] languageCode] );
	}
}

/////////////////////////////////////////////////////////////////////////////////

@implementation FreshGLView
{
	NSTimer* timer;
}

- (void)setApplication:(fr::Application*) app frameRate:(int)fps
{
	ASSERT( app != 0 );
	ASSERT( fps > 0 );
	application = app;
	
	timer = nil;
	
	[self setFPS:fps];
}

- (void) setFPS: (int) fps
{
	if( timer != nil )
	{
		[timer invalidate];
	}
	
	// Ask for regular timed callback.
	//
	timer = [NSTimer scheduledTimerWithTimeInterval:(1.0 / fps)
									 target:self
								   selector:@selector(idle)
								   userInfo:nil
									repeats:YES];
}

- (void) windowDidResize:(NSNotification *)notification
{
	if( application )
	{
		application->onWindowReshape();
	}
}

- (IBAction)toggleFullScreen: (id) sender
{
	if( application )
	{
		application->toggleFullscreen();
		application->onWindowReshape();
	}
}

- (void)prepareOpenGL
{
	// Request vertical sync.
	//
    GLint vsync = 1;
    [[self openGLContext] setValues:&vsync forParameter:NSOpenGLCPSwapInterval];
    
    [super prepareOpenGL];
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)isOpaque
{
    return YES;
}

- (void)idle
{
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)rect
{
	if( application != nullptr )
	{
        // Update input.
        //
        sendInputEvents();
        
        // Draw.
        //
		application->updateFrame();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INPUT STUFF
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace fr;

unsigned int g_nMouseSequences = 0;		// Counts the number of times that the mouse has been pressed. Roughly corresponds to CocoaTouch's touchId concept.

static void createAppTouches( NSView* view, CGSize viewSize, NSEvent* theEvent, Application::Touches& outAppTouches )
{
	NSPoint touchPoint = [theEvent locationInWindow];
	NSPoint lastTouchPoint = NSMakePoint( touchPoint.x - [theEvent deltaX], touchPoint.y + [theEvent deltaY] );
	
	// Convert from window space to backing store space.
	touchPoint = [view convertPointToBacking:touchPoint];
	lastTouchPoint = [view convertPointToBacking:lastTouchPoint];

	outAppTouches.push_back( Application::Touch( 
												vec2( touchPoint.x, touchPoint.y ),
												vec2( lastTouchPoint.x, lastTouchPoint.y ),
												vec2::ZERO,
												0,
												1,
												static_cast< int >( [theEvent clickCount] ),
												reinterpret_cast< void* >( g_nMouseSequences )));
}

- (void)mouseDown:(NSEvent *)theEvent
{
	if( !Application::doesExist() )
	{
		return;
	}
	
	++g_nMouseSequences;

	NSPoint touchPoint = [theEvent locationInWindow];
	touchPoint = [self convertPoint:touchPoint fromView:nil];
	
	BOOL isInside = [self mouse:touchPoint inRect:[self bounds]];
	
	if( isInside )
	{
        withTouches( TouchType::Begin, [&]( Application::Touches& appTouches )
                    {
                        createAppTouches( self, self.bounds.size, theEvent, appTouches );
                    });
	}
}

- (void)mouseDragged:(NSEvent *)theEvent
{
	[self mouseMoved: theEvent];	// Redirect, so that both are handled in the same way.
}

- (void)mouseMoved:(NSEvent *)theEvent
{	
	if( !Application::doesExist() )
	{
		return;
	}
	
	NSPoint touchPoint = [theEvent locationInWindow];
	touchPoint = [self convertPoint:touchPoint fromView:nil];

	BOOL isInside = [self mouse:touchPoint inRect:[self bounds]];
	
	NSPoint lastTouchPoint = NSMakePoint( touchPoint.x - [theEvent deltaX], touchPoint.y + [theEvent deltaY] );
	
	if( isInside )
	{
        withTouches( TouchType::Move, [&]( Application::Touches& appTouches )
                    {
                        createAppTouches( self, self.bounds.size, theEvent, appTouches );
                    });
	}
	else
	{
		// Moved outside. Send the event, modified to be inside, to the mouseUp handler.
		//
		NSEvent* cloneEvent = [NSEvent mouseEventWithType: [theEvent type]
												 location: lastTouchPoint			// Lie and say the up position is the last position, which was presumably in-window.
											modifierFlags: [theEvent modifierFlags]
												timestamp: [theEvent timestamp]
											 windowNumber: [theEvent windowNumber]
												  context: [theEvent context]
											  eventNumber: [theEvent eventNumber]
											   clickCount: [theEvent clickCount]
												 pressure: [theEvent pressure]];
		[self mouseUp: cloneEvent];
	}
}


- (void)mouseUp:(NSEvent *)theEvent
{
	if( !Application::doesExist() )
	{
		return;
	}
		
	NSPoint touchPoint = [theEvent locationInWindow];
	touchPoint = [self convertPoint:touchPoint fromView:nil];
	
	BOOL isInside = [self mouse:touchPoint inRect:[self bounds]];
	
	if( isInside )
	{
        withTouches( TouchType::End, [&]( Application::Touches& appTouches )
                    {
                        createAppTouches( self, self.bounds.size, theEvent, appTouches );
                    });
	}
}

- (void)scrollWheel:(NSEvent *)theEvent
{
	if( !Application::doesExist() )
	{
		return;
	}
	
	NSPoint touchPoint = [theEvent locationInWindow];
	touchPoint = [self convertPoint:touchPoint fromView:nil];
	
	BOOL isInside = [self mouse:touchPoint inRect:[self bounds]];
	
	if( isInside )
	{
		Application::Touches appTouches;

		NSPoint lastTouchPoint = NSMakePoint( touchPoint.x - [theEvent deltaX], touchPoint.y + [theEvent deltaY] );
		
		appTouches.push_back( Application::Touch(
												 vec2( touchPoint.x, touchPoint.y ),
												 vec2( lastTouchPoint.x, lastTouchPoint.y ),
												 vec2( -[theEvent scrollingDeltaX], -[theEvent scrollingDeltaY] ),
												 0,
												 1,
												 0,
												 reinterpret_cast< void* >( g_nMouseSequences )));
		
		Application::instance().onWheelMove( appTouches.begin(), appTouches.end() );
	}
}

/////////////////////////////////////////////////////////////////////////////
// KEYBOARD SUPPORT
//

	inline Keyboard::Key getKeyForMacKeyCode( unsigned short keyCode )
{
	switch( keyCode )
	{
		case kVK_ANSI_A: return Keyboard::A;
		case kVK_ANSI_S: return Keyboard::S;
		case kVK_ANSI_D: return Keyboard::D;
		case kVK_ANSI_F: return Keyboard::F;
		case kVK_ANSI_H: return Keyboard::H;
		case kVK_ANSI_G: return Keyboard::G;
		case kVK_ANSI_Z: return Keyboard::Z;
		case kVK_ANSI_X: return Keyboard::X;
		case kVK_ANSI_C: return Keyboard::C;
		case kVK_ANSI_V: return Keyboard::V;
		case kVK_ANSI_B: return Keyboard::B;
		case kVK_ANSI_Q: return Keyboard::Q;
		case kVK_ANSI_W: return Keyboard::W;
		case kVK_ANSI_E: return Keyboard::E;
		case kVK_ANSI_R: return Keyboard::R;
		case kVK_ANSI_Y: return Keyboard::Y;
		case kVK_ANSI_T: return Keyboard::T;
		case kVK_ANSI_1: return Keyboard::Alpha1;
		case kVK_ANSI_2: return Keyboard::Alpha2;
		case kVK_ANSI_3: return Keyboard::Alpha3;
		case kVK_ANSI_4: return Keyboard::Alpha4;
		case kVK_ANSI_6: return Keyboard::Alpha6;
		case kVK_ANSI_5: return Keyboard::Alpha5;
		case kVK_ANSI_Equal: return Keyboard::Equal;
		case kVK_ANSI_9: return Keyboard::Alpha9;
		case kVK_ANSI_7: return Keyboard::Alpha7;
		case kVK_ANSI_Minus: return Keyboard::Minus;
		case kVK_ANSI_8: return Keyboard::Alpha8;
		case kVK_ANSI_0: return Keyboard::Alpha0;
		case kVK_ANSI_RightBracket: return Keyboard::RightBracket;
		case kVK_ANSI_O: return Keyboard::O;
		case kVK_ANSI_U: return Keyboard::U;
		case kVK_ANSI_LeftBracket: return Keyboard::LeftBracket;
		case kVK_ANSI_I: return Keyboard::I;
		case kVK_ANSI_P: return Keyboard::P;
		case kVK_ANSI_L: return Keyboard::L;
		case kVK_ANSI_J: return Keyboard::J;
		case kVK_ANSI_Quote: return Keyboard::Quote;
		case kVK_ANSI_K: return Keyboard::K;
		case kVK_ANSI_Semicolon: return Keyboard::Semicolon;
		case kVK_ANSI_Backslash: return Keyboard::Backslash;
		case kVK_ANSI_Comma: return Keyboard::Comma;
		case kVK_ANSI_Slash: return Keyboard::Slash;
		case kVK_ANSI_N: return Keyboard::N;
		case kVK_ANSI_M: return Keyboard::M;
		case kVK_ANSI_Period: return Keyboard::Period;
		case kVK_ANSI_Grave: return Keyboard::Backtick;
		case kVK_ANSI_KeypadDecimal: return Keyboard::NumpadDecimal;
		case kVK_ANSI_KeypadMultiply: return Keyboard::NumpadMultiply;
		case kVK_ANSI_KeypadPlus: return Keyboard::NumpadAdd;
		case kVK_ANSI_KeypadClear: return Keyboard::NumpadClear;
		case kVK_ANSI_KeypadDivide: return Keyboard::NumpadDivide;
		case kVK_ANSI_KeypadEnter: return Keyboard::NumpadEnter;
		case kVK_ANSI_KeypadMinus: return Keyboard::NumpadSubtract;
		case kVK_ANSI_KeypadEquals: return Keyboard::NumpadEqual;
		case kVK_ANSI_Keypad0: return Keyboard::Numpad0;
		case kVK_ANSI_Keypad1: return Keyboard::Numpad1;
		case kVK_ANSI_Keypad2: return Keyboard::Numpad2;
		case kVK_ANSI_Keypad3: return Keyboard::Numpad3;
		case kVK_ANSI_Keypad4: return Keyboard::Numpad4;
		case kVK_ANSI_Keypad5: return Keyboard::Numpad5;
		case kVK_ANSI_Keypad6: return Keyboard::Numpad6;
		case kVK_ANSI_Keypad7: return Keyboard::Numpad7;
		case kVK_ANSI_Keypad8: return Keyboard::Numpad8;
		case kVK_ANSI_Keypad9: return Keyboard::Numpad9;
		case kVK_Return: return Keyboard::Enter;
		case kVK_Tab: return Keyboard::Tab;
		case kVK_Space: return Keyboard::Space;
		case kVK_Delete: return Keyboard::Backspace;
		case kVK_Escape: return Keyboard::Escape;
		case kVK_Command: return Keyboard::LeftCtrlCommand;
		case kVK_Shift: return Keyboard::LeftShift;
		case kVK_CapsLock: return Keyboard::CapsLock;
		case kVK_Option: return Keyboard::LeftAltOption;
		case kVK_Control: return Keyboard::LeftCtrlCommand;
		case kVK_RightShift: return Keyboard::RightShift;
		case kVK_RightOption: return Keyboard::RightAltOption;
		case kVK_RightControl: return Keyboard::RightCtrlCommand;
		case kVK_F5: return Keyboard::F5;
		case kVK_F6: return Keyboard::F6;
		case kVK_F7: return Keyboard::F7;
		case kVK_F3: return Keyboard::F3;
		case kVK_F8: return Keyboard::F8;
		case kVK_F9: return Keyboard::F9;
		case kVK_F11: return Keyboard::F11;
		case kVK_F13: return Keyboard::F13;
		case kVK_F14: return Keyboard::F14;
		case kVK_F10: return Keyboard::F10;
		case kVK_F12: return Keyboard::F12;
		case kVK_F15: return Keyboard::F15;
		case kVK_Home: return Keyboard::Home;
		case kVK_PageUp: return Keyboard::PageUp;
		case kVK_ForwardDelete: return Keyboard::Delete;
		case kVK_F4: return Keyboard::F4;
		case kVK_End: return Keyboard::End;
		case kVK_F2: return Keyboard::F2;
		case kVK_PageDown: return Keyboard::PageDown;
		case kVK_F1: return Keyboard::F1;
		case kVK_LeftArrow: return Keyboard::LeftArrow;
		case kVK_RightArrow: return Keyboard::RightArrow;
		case kVK_DownArrow: return Keyboard::DownArrow;
		case kVK_UpArrow: return Keyboard::UpArrow;
		
		default:
			return Keyboard::Unsupported;
	}
}

inline void postKeyboardEventsWithKey( bool downElseUp, Keyboard::Key key, unsigned int character, NSUInteger modifierFlags, bool isRepeat )
{
	bool isAltDown = modifierFlags & NSAlternateKeyMask;
	bool isControlDown = modifierFlags & ( NSCommandKeyMask | NSControlKeyMask );
	bool isShiftDown = modifierFlags & NSShiftKeyMask;
    
    Event::TypeRef type = downElseUp ? EventKeyboard::KEY_DOWN : EventKeyboard::KEY_UP;
    
    const EventKeyboard event{ type, nullptr, character, key, isAltDown, isControlDown, isShiftDown, isRepeat };

    addKeyEvent( event );
    
}

inline void postKeyboardEvents( bool downElseUp, NSEvent* event )
{
    const Keyboard::Key key = getKeyForMacKeyCode( [event keyCode] );
	
	if( key != Keyboard::Unsupported )
	{
		std::string characters = [[event characters] UTF8String];
		unsigned int character = 0;

		if( characters.size() == 1 )
		{
			character = characters.front();
		}
	
		postKeyboardEventsWithKey( downElseUp, key, character, [event modifierFlags], [event isARepeat] );
	}
}

#define FLAGS_CHANGED_CASE( key, mask )	\
if(( newFlagsDown & mask ) != ( lastFlagsDown & mask ))	\
{	\
	Keyboard::onKeyStateChanged( key, ( newFlagsDown & mask ) );	\
	postKeyboardEventsWithKey( newFlagsDown & mask,	\
							  key,	\
							  0,	\
							  newFlagsDown,	\
							  false );	\
}

- (void)flagsChanged:(NSEvent *)theEvent
{
	static NSUInteger lastFlagsDown = 0;
	NSUInteger newFlagsDown = [theEvent modifierFlags];

	if( Application::doesExist() )
	{
		FLAGS_CHANGED_CASE( Keyboard::Shift, NSShiftKeyMask )
		FLAGS_CHANGED_CASE( Keyboard::AltOption, NSAlternateKeyMask )
		FLAGS_CHANGED_CASE( Keyboard::CtrlCommand, ( NSCommandKeyMask | NSControlKeyMask ))
	}
	
	lastFlagsDown = newFlagsDown;
}

- (void)keyDown:(NSEvent *)theEvent
{
	Keyboard::Key key = getKeyForMacKeyCode( [theEvent keyCode] );
	if( key != Keyboard::Unsupported )
	{
		Keyboard::onKeyStateChanged( key, true );
	}
	
	postKeyboardEvents( true /* down */, theEvent );
}

- (void)keyUp:(NSEvent *)theEvent
{
	Keyboard::Key key = getKeyForMacKeyCode( [theEvent keyCode] );
	if( key != Keyboard::Unsupported )
	{
		Keyboard::onKeyStateChanged( key, false );
	}
	
	postKeyboardEvents( false /* up */, theEvent );
}

- (BOOL) applicationShouldTerminateAfterLastWindowClosed: (NSApplication *) theApplication
{
	return YES;
}

@end



