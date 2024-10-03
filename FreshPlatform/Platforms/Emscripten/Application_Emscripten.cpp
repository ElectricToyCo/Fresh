/*
 *  Application_Emscripten.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 2014/09/20.
 *  Copyright 2014 jeffwofford.com. All rights reserved.
 *
 */

#include "Application.h"
#include "FreshDebug.h"
#include "FreshOpenGL.h"
#include "AudioSystem.h"
#include "FreshFile.h"
#include "FreshTime.h"
#include "Objects.h"
#include "Assets.h"
#include "FreshXML.h"
#include "CommandProcessor.h"
#include "TelnetServer.h"
#include <algorithm>

#include <emscripten.h>
#include <emscripten/html5.h>

#if FRESH_TELEMETRY_ENABLED
#	include "UserTelemetry.h"
#endif

#define USE_HARD_FULLSCREEN 1

#if 0
#	define input_trace release_trace
#else
#	define input_trace(x)
#endif

using namespace fr;

namespace
{
	// Could also be "document" or "window"
	const char* const EMSCRIPTEN_CANVAS_ELEMENT_ID = "#canvas";

	ApplicationImplementation* g_applicationImplementation = nullptr;
	void EMSCRIPTEN_KEEPALIVE updateMainLoop();

	EM_BOOL EMSCRIPTEN_KEEPALIVE onMouseDown( int eventType, const EmscriptenMouseEvent* event, void* userData );
	EM_BOOL EMSCRIPTEN_KEEPALIVE onMouseMove( int eventType, const EmscriptenMouseEvent* event, void* userData );
	EM_BOOL EMSCRIPTEN_KEEPALIVE onMouseUp( int eventType, const EmscriptenMouseEvent* event, void* userData );
	EM_BOOL EMSCRIPTEN_KEEPALIVE onTouchDown( int eventType, const EmscriptenTouchEvent* event, void* userData );
	EM_BOOL EMSCRIPTEN_KEEPALIVE onTouchMove( int eventType, const EmscriptenTouchEvent* event, void* userData );
	EM_BOOL EMSCRIPTEN_KEEPALIVE onTouchUp( int eventType, const EmscriptenTouchEvent* event, void* userData );
	EM_BOOL EMSCRIPTEN_KEEPALIVE onWheelEvent( int eventType, const EmscriptenWheelEvent* event, void* userData );
	EM_BOOL EMSCRIPTEN_KEEPALIVE onKeyDown( int eventType, const EmscriptenKeyboardEvent* event, void* userData );
	EM_BOOL EMSCRIPTEN_KEEPALIVE onKeyUp( int eventType, const EmscriptenKeyboardEvent* event, void* userData );
	EM_BOOL EMSCRIPTEN_KEEPALIVE onWindowResized( int eventType, const EmscriptenUiEvent* event, void* userData );
	EM_BOOL EMSCRIPTEN_KEEPALIVE onFullscreenResized( int eventType, const EmscriptenFullscreenChangeEvent* event, void* userData );
}

namespace fr
{
	void EMSCRIPTEN_KEEPALIVE openURLInBrowser( const std::string& url )
	{
		emscripten_run_script( createString( "window.location.href = '" << url << "';" ).c_str() );
	}

	class ApplicationImplementation
	{
	public:

		ApplicationImplementation( Application* owner, CommandProcessor::ptr commandProcessor )
		:	m_owner( owner )
		,	m_commandProcessor( commandProcessor )
		,	m_isInMainLoop( false )
		,	m_desiredFramesPerSecond( owner->config().desiredFramesPerSecond() )
		{
			release_trace( "Emscripten ApplicationImplementation constructor" );

			ASSERT( !g_applicationImplementation );
			g_applicationImplementation = this;

			REQUIRES( owner );
			m_desiredClocksPerFrame = secondsToClocks( 1.0 / m_desiredFramesPerSecond );

			// release_trace( "   emscripten_set_mouse*_callback" );
			emscripten_set_mousedown_callback( EMSCRIPTEN_CANVAS_ELEMENT_ID, 0, 1, onMouseDown );
			emscripten_set_mousemove_callback( EMSCRIPTEN_CANVAS_ELEMENT_ID, 0, 1, onMouseMove );
			emscripten_set_mouseup_callback(   EMSCRIPTEN_CANVAS_ELEMENT_ID, 0, 1, onMouseUp );

			// release_trace( "   emscripten_set_touch*_callback" );
			emscripten_set_touchstart_callback(  EMSCRIPTEN_CANVAS_ELEMENT_ID, 0, 1, onTouchDown );
			emscripten_set_touchmove_callback(   EMSCRIPTEN_CANVAS_ELEMENT_ID, 0, 1, onTouchMove );
			emscripten_set_touchend_callback(    EMSCRIPTEN_CANVAS_ELEMENT_ID, 0, 1, onTouchUp );
			emscripten_set_touchcancel_callback( EMSCRIPTEN_CANVAS_ELEMENT_ID, 0, 1, onTouchUp );

			// release_trace( "   emscripten_set_wheel_callback" );
			emscripten_set_wheel_callback(     EMSCRIPTEN_CANVAS_ELEMENT_ID, 0, 1, onWheelEvent );

			// release_trace( "   emscripten_set_key*_callback" );
			emscripten_set_keydown_callback(   EMSCRIPTEN_EVENT_TARGET_DOCUMENT, 0, 1, onKeyDown );
			emscripten_set_keyup_callback(     EMSCRIPTEN_EVENT_TARGET_DOCUMENT, 0, 1, onKeyUp );

			// release_trace( "   emscripten_set_resize_callback" );
			emscripten_set_resize_callback(	   EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, onWindowResized );

			// release_trace( "   emscripten_set_fullscreenchange_callback" );
			emscripten_set_fullscreenchange_callback( EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, onFullscreenResized );

			release_trace( "Emscripten ApplicationImplementation: running javascript literal code" );

			// Run Emscripten JavaScript directly from here.
			//
			EM_ASM(

				// Create a directory for persistent data
				//
				FS.mkdir( '/Documents' );	// May already have been created.

				// Mount persistent directory as IDBFS
				//
				FS.mount( IDBFS, {}, '/Documents' );

				Module.print( "Starting file sync..." );

				Module.syncdone = 0;

				// Populate Documents directory with existing persistent source data
				// stored with Indexed Db
				// 		first parameter = "true" mean synchronize from Indexed Db to Emscripten file system,
				//  		"false" would mean synchronize from Emscripten file system to Indexed Db
				// 		second parameter = function called when data are synchronized
				FS.syncfs( true, function(err) {
					assert( !err );
					Module.print( "File synchronization finished.");
					Module.syncdone = 1;
				});
			);

			release_trace( "Emscripten ApplicationImplementation constructor complete" );
		}

		// TODO!!! Not convinced this is called from anywhere.
		void savePersistentData()
		{
			EM_ASM(
			   // Persist filesystem changes
			   FS.syncfs( false, function (err) {} )
		   );
		}

		~ApplicationImplementation()
		{
			release_trace( "Fresh Emscripten: Application implementation destructor" );
			savePersistentData();
			shutdownOpenGL();
			g_applicationImplementation = nullptr;
		}

		bool hasMainWindow() const
		{
			return m_webglContext >= 0;
		}

		void createMainWindow()
		{
			// In the web implementation we ignore `m_owner->config().desiredWindowRect()` and take the window rect
			// instead from the actual browser window size.

			const Vector2i dims = Application::instance().getWindowDimensions();

			release_trace( "Using canvas size (" << dims.x << ", " << dims.y << ")" );
			emscripten_set_canvas_element_size( EMSCRIPTEN_CANVAS_ELEMENT_ID, dims.x, dims.y );

			ASSERT( m_webglContext < 0 );

			EmscriptenWebGLContextAttributes attr;
			attr.alpha = false;
			attr.depth = true;
			attr.stencil = true;
			attr.antialias = false;
			attr.preserveDrawingBuffer = false;
			attr.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;
			attr.failIfMajorPerformanceCaveat = false;
			attr.enableExtensionsByDefault = true;
			attr.premultipliedAlpha = true;
			attr.majorVersion = 2;
			attr.minorVersion = 0;
			emscripten_webgl_init_context_attributes( &attr );

			m_webglContext = emscripten_webgl_create_context( EMSCRIPTEN_CANVAS_ELEMENT_ID, &attr );
			ASSERT( m_webglContext >= 0 );

			emscripten_webgl_make_context_current( m_webglContext );
		}

		Application::ExitCode runMainLoop( int argc, const char* argv[] )
		{
			REQUIRES( !m_isInMainLoop );

#if defined( DEV_MODE ) && 0
			m_commandProcessor->startListenServer();
#endif

			createMainWindow();

			m_isInMainLoop = true;
			m_nTicks = 0;

			try
			{
				emscripten_set_main_loop( &updateMainLoop, 0 /* Use browser's requestAnimationFrame() */, true /* infinite loop */ );
				return 0;
			}
			catch( const std::exception& e )
			{
				m_isInMainLoop = false;
				release_trace( "Exception while running main loop: " << e.what() );
				return -1;
			}
			catch( ... )
			{
				m_isInMainLoop = false;
				release_trace( "Unknown exception while running main loop." );
				return -2;
			}
		}

		void swapBuffers()
		{
			ASSERT( hasMainWindow() );
		}

		std::string getPromptedFilePath( bool forSaveElseOpen, const char* semicolonSeparatedFileExtensions )
		{
			return "";
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
			Vector2d dims;
			emscripten_get_element_css_size( EMSCRIPTEN_CANVAS_ELEMENT_ID, &dims.x, &dims.y );

			return vector_cast< int >( dims );
		}

		Vector2i getWindowDimensions() const
		{
			return getScreenDimensions();
		}

		real pixelsPerScreenInch() const
		{
			// Assume a 17-in (diagonal) desktop. Could easily be larger or smaller.
			//
			const real monitorWidthInches = 14.57f;
			return getScreenDimensions().x / monitorWidthInches;
		}

		void goFullscreen( bool fullscreen )
		{
			if( fullscreen )
			{
				EmscriptenFullscreenStrategy strategy;
				memset( &strategy, 0, sizeof( strategy ));
				strategy.scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_ASPECT;
				strategy.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_STDDEF;
				strategy.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_DEFAULT;
				strategy.canvasResizedCallback = reinterpret_cast< decltype( strategy.canvasResizedCallback ) >( onFullscreenResized );

				release_trace( "moving to fullscreen" );

#if USE_HARD_FULLSCREEN
				emscripten_request_fullscreen_strategy( EMSCRIPTEN_EVENT_TARGET_WINDOW, true, &strategy );
#else
				emscripten_enter_soft_fullscreen( EMSCRIPTEN_EVENT_TARGET_WINDOW, &strategy );
#endif
				release_trace( "moved to fullscreen, we think." );
			}
			else
			{
#if USE_HARD_FULLSCREEN
				emscripten_exit_fullscreen();
#else
				emscripten_exit_soft_fullscreen();
#endif
			}
		}

		bool isFullscreen() const
		{
			EmscriptenFullscreenChangeEvent status;
			emscripten_get_fullscreen_status( &status );
			return status.isFullscreen;
		}

		std::string windowTitle() const
		{
			return m_owner->config().desiredTitle();
		}

		void windowTitle( const std::string& value )
		{
			// Ignore.
		}

		bool isMainLoopRunning() const
		{
			return m_isInMainLoop;
		}

		void updateFrame()
		{
			++m_nTicks;

			m_owner->updateFrame();

			if( m_nTicks == 1 )
			{
				EM_ASM( onGameStarting() );
			}
		}

		void onSystemAudioFocusGained()
		{
			if( AudioSystem::doesExist() )
			{
				AudioSystem::instance().unsuspend();
			}
		}

		void onSystemAudioFocusLost()
		{
			if( AudioSystem::doesExist() )
			{
				AudioSystem::instance().suspend();
			}
		}

		SYNTHESIZE( bool, isUpdating )

	protected:

		void shutdownOpenGL()
		{
			ASSERT( m_webglContext >= 0 );
			emscripten_webgl_destroy_context( m_webglContext );
			m_webglContext = -1;
		}

	private:

		Application* m_owner;
		SmartPtr< CommandProcessor > m_commandProcessor;
		bool m_isInMainLoop;
		TimeType m_desiredFramesPerSecond;

		SystemClock m_desiredClocksPerFrame;

		EMSCRIPTEN_WEBGL_CONTEXT_HANDLE m_webglContext = -1;

		bool m_isUpdating = true;
		size_t m_nTicks = 0;
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
		std::exit( exitCode );
	}

	bool Application::isMultitouch() const
	{
		return true;		// At least some web browsers support multitouch.
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

	void Application::desiredFramesPerSecondDetail( TimeType fps )
	{
		m_impl->desiredFramesPerSecond( fps );
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

	Application::ExitCode Application::runMainLoop( int argc, const char* argv[] )
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
		return { "-Web" };
	}

	std::vector< std::string > Application::getVariantConfigFileSuffixes( const std::string& platformSuffix ) const
	{
		return {};
	}

	bool Application::isApplicationAlternativeStartupKeyDown() const
	{
		return false;
	}
}

namespace
{
	void updateMainLoop()
	{
		ASSERT( g_applicationImplementation );
		g_applicationImplementation->updateFrame();
	}

	// MARK: MOUSE AND TOUCH SUPPORT //////////////////////////////////////////////////////////////////////////////////

	std::unordered_map< long, EmscriptenTouchPoint > g_touchPoints;

	// Unique touch ID support for mouse and touch.
	//
	size_t g_touchId = 0;
	size_t g_currentActiveMouseId = ~0;

	template< typename EmscriptenEventType >
	void* uniqueTouchIdentifier( const EmscriptenEventType& event );

	template<>
	void* uniqueTouchIdentifier( const EmscriptenMouseEvent& event )
	{
		return reinterpret_cast< void* >( g_touchId );
	}

	template<>
	void* uniqueTouchIdentifier( const EmscriptenTouchPoint& event )
	{
		return reinterpret_cast< void* >( event.identifier );
	}

	template< typename EmscriptenEventType >
	bool isActiveTouch( const EmscriptenEventType& event );

	template<>
	bool isActiveTouch( const EmscriptenMouseEvent& event )
	{
		return g_touchId == g_currentActiveMouseId;
	}

	template<>
	bool isActiveTouch( const EmscriptenTouchPoint& event )
	{
		return g_touchPoints.find( event.identifier ) != g_touchPoints.end();
	}

	// Touch movement
	//
	template< typename EmscriptenEventType >
	vec2 movement( const EmscriptenEventType& event );

	template<>
	vec2 movement( const EmscriptenMouseEvent& event )
	{
		return { (real) event.movementX, (real) event.movementY };
	}

	template<>
	vec2 movement( const EmscriptenTouchPoint& event )
	{
		const auto priorEventIter = g_touchPoints.find( event.identifier );
		if( priorEventIter != g_touchPoints.end() )
		{
			const auto& priorEvent = priorEventIter->second;
			return vec2{ (real) event.targetX, (real) event.targetY } -
			vec2{ (real) priorEvent.targetX, (real) priorEvent.targetY };
		}
		else
		{
			return {};
		}
	}

	// Creating a single touch point from mouse or touch.
	//
	template< typename EmscriptenPointType >
	std::pair< bool, Application::Touch > createTouch( const EmscriptenPointType& eventPoint )
	{
		// Emscripten screws up mouse coordinates badly. You can ask for the targetX and Y position
		// of the point, but actually this point is in window space, relative to the canvas's
		// original position prior to transformation.

		Vector2d windowSize;

		emscripten_get_element_css_size( EMSCRIPTEN_CANVAS_ELEMENT_ID, &windowSize.x, &windowSize.y );

		const vec2 canvasSize = vector_cast< real >( g_applicationImplementation->getWindowDimensions() );

		// If the touch is new, verify whether the point is within the window.
		//
		const bool includeOutOfBounds = isActiveTouch( eventPoint );

		if( !includeOutOfBounds && (eventPoint.targetX < 0 || eventPoint.targetX >= windowSize.x ||
									eventPoint.targetY < 0 || eventPoint.targetY >= windowSize.y ))
		{
			input_trace( "Touch rejected because " << eventPoint.targetX << "," << eventPoint.targetY << " was outside "
						<< windowSize.x << "," << windowSize.y << " (and maybe " << canvasSize.x << "," << canvasSize.y << ")" );
			return std::make_pair( false, Application::Touch{} );
		}

		const vec2 dims = vector_cast< real >( windowSize );
		const vec2 scale = canvasSize / dims;

		const vec2 change = movement( eventPoint );

		return std::make_pair( true, Application::Touch{
			vec2( eventPoint.targetX * scale.x, ( dims.y - eventPoint.targetY ) * scale.y ),
			vec2(( eventPoint.targetX - change.x ) * scale.x,
				 ( dims.y - ( eventPoint.targetY - change.y )) * scale.y),
			vec2::ZERO,	// wheel
			0,
			1,
			1,			// tap count.
			uniqueTouchIdentifier( eventPoint ) });
	}

	// Creating a set of touches for a mouse or touch event.
	//
	template< typename EmscriptenEventType >
	Application::Touches createTouches( const EmscriptenEventType& event );

	template<>
	Application::Touches createTouches( const EmscriptenMouseEvent& event )
	{
		Application::Touches touches;
		const auto touchPair = createTouch( event );
		if( touchPair.first )
		{
			touches.push_back( touchPair.second );
		}
		return touches;
	}

	template<>
	Application::Touches createTouches( const EmscriptenTouchEvent& event )
	{
		Application::Touches touches;
		for( int i = 0; i < event.numTouches; ++i )
		{
			const auto touchPair = createTouch( event.touches[ i ] );
			if( touchPair.first )
			{
				touches.push_back( touchPair.second );
			}
		}
		return touches;
	}

	// Mouse handling.
	//
	EM_BOOL onMouseDown( int eventType, const EmscriptenMouseEvent* event, void* userData )
	{
		input_trace( "onMouseDown( " << event->targetX << "," << event->targetY << ")" );
		++g_touchId;

		const auto touches = createTouches( *event );
		if( touches.empty() == false && Application::doesExist() )
		{
			Application::instance().onTouchesBegin( touches.begin(), touches.end() );
			g_currentActiveMouseId = g_touchId;
			return true;
		}
		return false;
	}

	EM_BOOL onMouseMove( int eventType, const EmscriptenMouseEvent* event, void* userData )
	{
		input_trace( "onMouseMove( " << event->targetX << "," << event->targetY << ")" );
		input_trace(    "screen: " << event->screenX << "," << event->screenY
					<< " client: " << event->clientX << "," << event->clientY
					<< " canvas: " << event->canvasX << "," << event->canvasY );


		// Only handle move if mouse is actually down.
		if( event->buttons == 0 ) return true;

		const auto touches = createTouches( *event );
		if( touches.empty() == false && Application::doesExist() )
		{
			Application::instance().onTouchesMove( touches.begin(), touches.end() );
			return true;
		}
		return false;
	}

	EM_BOOL onMouseUp( int eventType, const EmscriptenMouseEvent* event, void* userData )
	{
		input_trace( "onMouseUp( " << event->targetX << "," << event->targetY << ")" );

		const auto touches = createTouches( *event );
		if( touches.empty() == false && Application::doesExist() )
		{
			Application::instance().onTouchesEnd( touches.begin(), touches.end() );
			g_currentActiveMouseId = ~0;
			return true;
		}
		return false;
	}

	EM_BOOL onWheelEvent( int eventType, const EmscriptenWheelEvent* event, void* userData )
	{
		input_trace( "onWheelEvent( " << event->deltaX << "," << event->deltaY << ")" );
		++g_touchId;

		auto touches = createTouches( event->mouse );
		if( touches.empty() == false && Application::doesExist() )
		{
			touches.front().wheelDelta = vec2( event->deltaX, event->deltaY );
			touches.front().nTaps = 0;
			Application::instance().onWheelMove( std::begin( touches ), std::end( touches ) );
			return true;
		}
		return false;
	}

	// Touch handling.
	//
	void startTouch( const EmscriptenTouchPoint& touch )
	{
		g_touchPoints[ touch.identifier ] = touch;
	}
	void updateTouch( const EmscriptenTouchPoint& touch )
	{
		g_touchPoints[ touch.identifier ] = touch;
	}
	void endTouch( const EmscriptenTouchPoint& touch )
	{
		const auto iter = g_touchPoints.find( touch.identifier );
		if( iter != g_touchPoints.end() )
		{
			g_touchPoints.erase( iter );
		}
	}

	EM_BOOL onTouchDown( int eventType, const EmscriptenTouchEvent* event, void* userData )
	{
		input_trace( "onTouchDown( " << eventType << ")" );

		const auto touches = createTouches( *event );
		if( touches.empty() == false && Application::doesExist() )
		{
			// Record touch objects so we can track movement.
			//
			for( int i = 0; i < event->numTouches; ++i )
			{
				startTouch( event->touches[ i ] );
			}

			Application::instance().onTouchesBegin( touches.begin(), touches.end() );
			return true;
		}
		return false;
	}

	EM_BOOL onTouchMove( int eventType, const EmscriptenTouchEvent* event, void* userData )
	{
		input_trace( "onTouchMove( " << eventType << ")" );

		const auto touches = createTouches( *event );
		if( touches.empty() == false && Application::doesExist() )
		{
			Application::instance().onTouchesMove( touches.begin(), touches.end() );
			for( int i = 0; i < event->numTouches; ++i )
			{
				updateTouch( event->touches[ i ] );
			}

			return true;
		}

		return false;
	}

	EM_BOOL onTouchUp( int eventType, const EmscriptenTouchEvent* event, void* userData )
	{
		input_trace( "onTouchUp( " << eventType << ")" );

		const auto touches = createTouches( *event );
		if( touches.empty() == false && Application::doesExist() )
		{
			Application::instance().onTouchesEnd( touches.begin(), touches.end() );
			return true;
		}

		for( int i = 0; i < event->numTouches; ++i )
		{
			endTouch( event->touches[ i ] );
		}

		return false;
	}

	// Keyboard handling.
	//
	inline void postKeyboardEventsWithKey( std::function< void (Application&, const EventKeyboard& )>&& fn,
		Event::TypeRef type,
		Keyboard::Key key,
		unsigned int character,
		bool isAltDown,
		bool isControlDown,
		bool isShiftDown,
		bool isRepeat )
	{
		fn( Application::instance(), EventKeyboard( type, nullptr, character, key, isAltDown, isControlDown, isShiftDown, isRepeat ));
	}

	Keyboard::Key getKeyForKeyCode( unsigned long keycode );

	inline void postKeyboardEvents( std::function< void (Application&, const EventKeyboard& )>&& fn, Event::TypeRef type, const EmscriptenKeyboardEvent& event )
	{
		Keyboard::Key key = getKeyForKeyCode( event.keyCode );

//		release_trace( "Keycode " << event.keyCode << " gives us key " << key );

		if( key != Keyboard::Unsupported )
		{
			// Find the normal UTF8 representation for this key.
			//
			std::ostringstream characters;
			characters << event.key;

			unsigned int character = static_cast< unsigned int >( key );

			// We don't want keys that have big names for their UTF-8 representation (like "Enter" and "Meta").
			// In that case, use their actual key code as their ASCII representation.
			// Otherwise, use the system-proposed key string character.
			//
			if( characters.str().size() == 1 )
			{
				character = characters.str()[ 0 ];
			}

			postKeyboardEventsWithKey( std::move( fn ), type, key, character, event.altKey, event.ctrlKey, event.shiftKey, event.repeat );
		}
	}

	EM_BOOL onKeyDown( int eventType, const EmscriptenKeyboardEvent* event, void* userData )
	{
		Keyboard::Key key = getKeyForKeyCode( event->keyCode );
		if( key != Keyboard::Unsupported )
		{
			Keyboard::onKeyStateChanged( key, true );
		}

		postKeyboardEvents( std::mem_fn( &Application::onKeyDown ), EventKeyboard::KEY_DOWN, *event );
		return true;
	}

	EM_BOOL onKeyUp( int eventType, const EmscriptenKeyboardEvent* event, void* userData )
	{
		Keyboard::Key key = getKeyForKeyCode( event->keyCode );
		if( key != Keyboard::Unsupported )
		{
			Keyboard::onKeyStateChanged( key, false );
		}

		postKeyboardEvents( std::mem_fn( &Application::onKeyUp ), EventKeyboard::KEY_UP, *event );
		return true;
	}

	Keyboard::Key getKeyForKeyCode( unsigned long keycode )
	{
		if( keycode >= Keyboard::Key::MAX_KEYS )
		{
			return Keyboard::Key::Unsupported;
		}
		else
		{
			return static_cast< Keyboard::Key >( keycode );
		}
	}

	EM_BOOL onWindowResized( int eventType, const EmscriptenUiEvent *event, void *userData )
	{
		if( Application::doesExist() )
		{
			const Vector2i dims = Application::instance().getWindowDimensions();
			release_trace( "window resized: " << dims );

			emscripten_set_canvas_element_size( EMSCRIPTEN_CANVAS_ELEMENT_ID, dims.x, dims.y );

			Application::instance().onWindowReshape();
		}
		return true;
	}

	EM_BOOL onFullscreenResized( int eventType, const EmscriptenFullscreenChangeEvent* event, void* userData )
	{
		release_trace( "fullscreen resized" );
		if( Application::doesExist() )
		{
			Application::instance().onWindowReshape();
		}
		return true;
	}

	// Exported functions, callable from Javascript.
	extern "C"
	{
		EMSCRIPTEN_KEEPALIVE void report( const char* message )
		{
			release_trace( message );
		}

		EMSCRIPTEN_KEEPALIVE void goFullscreen()
		{
			g_applicationImplementation->goFullscreen( true );
		}

		EMSCRIPTEN_KEEPALIVE void goWindowed()
		{
			g_applicationImplementation->goFullscreen( false );
		}
	}
}
