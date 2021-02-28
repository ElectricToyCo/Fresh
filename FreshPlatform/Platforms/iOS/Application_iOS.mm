/*
 *  Application_iOS.mm
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
#include "AudioSystem.h"
#include "Renderer.h"
#include "FreshXML.h"
#include "CommandProcessor.h"
#include "TelnetServer.h"

#if FRESH_TELEMETRY_ENABLED
#	include "UserTelemetry.h"
#endif

#import <UIKit/UIKit.h>
#import "GameView_iOS.h"

#import "OpenGLES2ContextHandler.h"

using namespace fr;

// Globals
UIWindow*	g_rootWindow = nil;

namespace
{
	// Helpful: http://en.wikipedia.org/wiki/List_of_iOS_devices
	
	inline bool isLowEndDevice( const std::string& deviceModel )
	{
		return
		deviceModel == "iPhone 1G" ||
		deviceModel == "iPhone 3G" ||
		deviceModel == "iPhone 3GS" ||
		deviceModel == "iPhone 4"	||
		deviceModel == "iPad 1" ||
		deviceModel == "iPod Touch (1st gen)" ||
		deviceModel == "iPod Touch (2nd gen)";
		
		// TODO what about the following?:
		//		deviceModel == "iPod Touch (3rd gen)"
		//		deviceModel == "iPod Touch (4th gen)"		
	}
	
	inline std::string castString( NSString* string )
	{
		return string != nil ? [string UTF8String] : std::string{};
	}
	
	__unused inline NSString* castString( const std::string& string )
	{
		return [NSString stringWithUTF8String: string.c_str() ];
	}
}

///////////////////////////////////////////////

@interface GameViewController : UIViewController
@end

@implementation GameViewController

- (BOOL)prefersStatusBarHidden
{
    return YES;
}

- (BOOL)prefersHomeIndicatorAutoHidden
{
	return YES;
}

@end

///////////////////////////////////////////////

@interface FreshAppDelegate : NSObject <UIApplicationDelegate> 
{
@private
    UIWindow* gameWindow;
    GameView* gameView;
	
	GameViewController* gameViewController;
}

@property (nonatomic, retain) UIWindow* gameWindow;
@property (nonatomic, retain) GameView* gameView;
@property (nonatomic, retain) GameViewController* gameViewController;

@end

@implementation FreshAppDelegate

@synthesize gameWindow;
@synthesize gameView;
@synthesize gameViewController;

- (void) createWindowAndViewWithRect: (CGRect) viewRect frameRate: (NSTimeInterval) framesPerSecond
{
	ASSERT( gameWindow == nil );
	ASSERT( gameView == nil );
	ASSERT( gameViewController == nil );
	
	// Determine screen size in pixel space. This supports iPhone 4 resolutions.
	UIScreen* mainScreen = [UIScreen mainScreen];
	CGFloat scale = 1.0f;
	
	// Conditional support of scale for 4.0+ devices.
	if( [mainScreen respondsToSelector: @selector(scale)] )
	{
		scale = [mainScreen scale];
	}
		
	CGRect logicalBounds = [mainScreen bounds];
	CGRect pixelBounds = logicalBounds;
	
	if( scale > 0 ) 
	{
		pixelBounds.origin.x *= scale;
		pixelBounds.origin.y *= scale;
		pixelBounds.size.width *= scale;
		pixelBounds.size.height *= scale;
	}
	
	// Create the window
	//
    gameWindow = g_rootWindow = [[UIWindow alloc] initWithFrame:logicalBounds];
	gameWindow.multipleTouchEnabled = YES;
	
	//
	// Create the view
	//
	
	if( viewRect.origin.x == 0 )
	{
		viewRect.origin.x = logicalBounds.origin.x;
	}
	if( viewRect.origin.y == 0 )
	{
		viewRect.origin.y = logicalBounds.origin.y;
	}
	if( viewRect.size.width == 0 )
	{
		viewRect.size.width = logicalBounds.size.width;
	}
	if( viewRect.size.height == 0 )
	{
		viewRect.size.height = logicalBounds.size.height;
	}
	
	// Setup the rendering context handler.
	//
	OpenGLES2ContextHandler* contextHandler = [[OpenGLES2ContextHandler alloc] init];

	// Allocate and initialize the view.
	//
    gameView = [[GameView alloc] initWithFrame:viewRect frameRate: framesPerSecond contextHandler:contextHandler];
	gameView.multipleTouchEnabled = YES;

	[gameView setContentScaleFactor: scale];
	
	gameViewController = [[GameViewController alloc] init];
	[gameViewController setView: gameView];	
	
    [gameWindow addSubview: gameView];
	
	gameWindow.rootViewController = gameViewController;
	
	
    // Show the window
	//
    [gameWindow makeKeyAndVisible];
	
	// Start updating
	//
    [gameView startAnimation];
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions   
{
	Rectanglei desiredViewRect = Application::instance().config().desiredWindowRect();
	NSTimeInterval desiredFramesPerSecond = Application::instance().desiredFramesPerSecond();
	
	CGRect viewRect;
	viewRect.origin.x = desiredViewRect.left();
	viewRect.origin.y = desiredViewRect.top();
	viewRect.size.width = desiredViewRect.right() - desiredViewRect.left();
	viewRect.size.height = desiredViewRect.bottom() - desiredViewRect.top();			
	
	[self createWindowAndViewWithRect: viewRect frameRate: desiredFramesPerSecond];
	
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
	trace( "applicationWillResignActive" );
    [gameView stopAnimation];
	Application::instance().onMemoryWarning();		// A termination threat, yes, but also free up memory in order to motivate iOS to keep us around while suspended.
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
	trace( "applicationDidBecomeActive" );

    [gameView startAnimation];
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
	trace( "applicationDidEnterBackground" );

#if FRESH_TELEMETRY_ENABLED
	if( UserTelemetry::doesExist() )
	{
		UserTelemetry::instance().pauseSession();
	}
#endif
	
	Application::instance().onSleeping();
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
	trace( "applicationWillEnterForeground" );
	
#if FRESH_TELEMETRY_ENABLED
	if( UserTelemetry::doesExist() )
	{
		UserTelemetry::instance().resumeSession();
	}
#endif
	
	Application::instance().onWaking();
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    [gameView stopAnimation];

#if FRESH_TELEMETRY_ENABLED
	if( UserTelemetry::doesExist() )
	{
		UserTelemetry::instance().endSession();
	}
#endif
	
	Application::instance().onTerminating();
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
	trace( "applicationDidReceiveMemoryWarning" );
	
	Application::instance().onMemoryWarning();	
}

@end


namespace fr
{
	class ApplicationImplementation
	{
	public:
		
		ApplicationImplementation()
		:	m_isInMainLoop( false )
		{}
		
		~ApplicationImplementation()
		{}
		
		Application::ExitCode runMainLoop( int argc, const char* argv[] )
		{
			m_isInMainLoop = true;
			
			Application::ExitCode retVal = 0;
			
			try
			{
				@autoreleasepool
				{
					@try
					{
						retVal = UIApplicationMain( argc, const_cast< char** >( argv ), nil, @"FreshAppDelegate" );
					}
					@catch( NSException* ex )
					{
						trace( "FATAL ERROR: Objective-C exception:" );
						trace( "\tname: " << [[ex name] UTF8String ] );
						trace( "\treason: " << [[ex reason] UTF8String ] );
					}
				}
			}
			catch( const std::exception& e )
			{
				trace( "FATAL ERROR: std::exception with message '" << e.what() << "'." );				
			}
			catch( ... )
			{
				trace( "FATAL ERROR: Unknown exception." );
			}
				
			m_isInMainLoop = false;
			
			return retVal;
		}
		
		static Vector2i getScreenDimensions()
		{
			@autoreleasepool
			{
				CGSize screenSize = [UIScreen mainScreen].bounds.size;
				float screenScale = [UIScreen mainScreen].scale;
				return Vector2i( screenSize.width * screenScale, screenSize.height * screenScale );
			}
		}
		
		static Vector2i getWindowDimensions()
		{
			@autoreleasepool
			{
				FreshAppDelegate* appDelegate = (FreshAppDelegate*) ([UIApplication sharedApplication].delegate);
				ASSERT( appDelegate != nil );
				ASSERT( appDelegate.gameView != nil );
				CGSize viewDimensions = appDelegate.gameView.bounds.size;
				return Vector2i( viewDimensions.width * appDelegate.gameView.contentScaleFactor, viewDimensions.height * appDelegate.gameView.contentScaleFactor );
			}
		}

		void desiredFramesPerSecond( TimeType fps )
		{
			FreshAppDelegate* appDelegate = (FreshAppDelegate*) ([UIApplication sharedApplication].delegate);
			[appDelegate.gameView setFramesPerSecond:fps];
		}

		rect safeAreaInsets() const
		{
			FreshAppDelegate* appDelegate = (FreshAppDelegate*) ([UIApplication sharedApplication].delegate);
			ASSERT( appDelegate != nil );
			ASSERT( appDelegate.gameView != nil );
			
			if( @available( iOS 11.0, * ))
			{
				const auto contentScale = [appDelegate.gameView contentScaleFactor];
				
				const auto insets = [appDelegate.gameView safeAreaInsets];
				return {
					static_cast< real >( insets.left * contentScale ),
					static_cast< real >( insets.top * contentScale ),
					static_cast< real >( insets.right * contentScale ),
					static_cast< real >( insets.bottom * contentScale )
				};
 			}
			else
			{
				return {};
			}
		}
		
		real pixelsPerScreenInch() const
		{
			// TODO Base on iOS device model.
			
			// Assume a 17-in (diagonal) desktop. Could easily be larger or smaller.
			//
			const real monitorWidthInches = 14.57f;
			return getScreenDimensions().x / monitorWidthInches;
		}		

		bool isMainLoopRunning() const
		{
			return m_isInMainLoop;
		}

		void swapBuffers()
		{
			// Not explicitly done on iOS
		}
		
	private:

		bool m_isInMainLoop;
	};

	///////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////
	
	void Application::constructImplementation()
	{
		m_impl = new ApplicationImplementation();
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
	
	bool Application::isMultitouch() const
	{
		return true;
	}
	
	std::string Application::getPromptedFilePath( bool forSaveElseOpen, const char* fileFilters )
	{
		return "";
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
		return ApplicationImplementation::getScreenDimensions();
	}
	
	Vector2i Application::getWindowDimensions() const
	{
		return ApplicationImplementation::getWindowDimensions();
	}

	void Application::desiredFramesPerSecondDetail( TimeType fps )
	{
		m_impl->desiredFramesPerSecond( fps );
	}
	
	std::string Application::windowTitle() const
	{
		// iOS apps have no window title.
		return "";
	}
	
	void Application::windowTitle( const std::string& value )
	{
		// Ignore this. iOS apps have no window title.
	}
	
	void Application::goFullscreen( bool fullscreen )
	{
		// Ignored on this platform.
	}
	
	rect Application::safeAreaInsets() const
	{
		return m_impl->safeAreaInsets();
	}

	bool Application::isFullscreen() const
	{
		return true;		// iOS is always full screen.
	}
	
	Application::ExitCode Application::runMainLoop( int argc, const char* argv[] )
	{
		REQUIRES( !isMainLoopRunning() );
		
#if DEV_MODE && 0
		// Create the `memwarning` command.
		//
#	pragma clang diagnostic push
#	pragma clang diagnostic ignored "-Wundeclared-selector"
		auto caller = make_caller< void >( []() { 
			[[UIApplication sharedApplication] performSelector:@selector(_performMemoryWarning)];
		} );
		m_commandProcessor->registerCommand( this, "memwarning", "initiates a memory warning", std::move( caller ) );
#	pragma clang diagnostic pop		
#endif
		
		// Start the listen server.
		//
		@autoreleasepool
		{
#if DEV_MODE && 1			
			m_commandProcessor->startListenServer();
#endif
			return m_impl->runMainLoop( argc, argv );
		}
	}

	bool Application::isMainLoopRunning() const
	{
		return m_impl->isMainLoopRunning();
	}
	
	std::vector< std::string > Application::getPlatformConfigFileSuffixes() const
	{
		return { "-iOS", "-mobile" };
	}
	
	std::vector< std::string > Application::getVariantConfigFileSuffixes( const std::string& platformSuffix ) const
	{
		std::vector< std::string > variants;

		const auto deviceModel = getPlatformModel();
		
		if( isLowEndDevice( deviceModel ))
		{
			variants.push_back( "-low-end" );
		}
		else if( std::max( getScreenDimensions().x, getScreenDimensions().y ) < 1536 )
		{
			variants.push_back( "-normal-res" );
		}
		
		return variants;
	}

	bool Application::isApplicationAlternativeStartupKeyDown() const
	{
		return false;
	}

	std::string Application::userLanguageCode() const
	{
		NSLocale* locale = [NSLocale currentLocale];
		if( [locale respondsToSelector: @selector(languageCode)] )
		{
			return castString( [locale languageCode] );
		}
		else
		{
			NSString* languageCode = [NSLocale preferredLanguages][0];
			return castString( languageCode ).substr( 0, 2 );	// Just the language code itself.
		}
				
	}
}
