/*
 *  Application_Android.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 2014/04/21.
 *  Copyright 2014 jeffwofford.com. All rights reserved.
 *
 */

#include <android_native_app_glue.h>
#include <android/native_window.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <jni.h>

#include <unistd.h>

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

#if FRESH_TELEMETRY_ENABLED
#	include "UserTelemetry.h"
#endif

using namespace fr;

struct android_app* g_androidApp = nullptr;

namespace
{
	extern fr::ApplicationImplementation* g_appImplementation;

	void onAppCmd( struct android_app* app, int32_t cmd );
	int32_t onAppInput( struct android_app* app, AInputEvent* event );

	bool isLowEndDevice( const std::string& deviceModel )
	{
		return true;		// Consider all Android devices to be low-end. Sheesh.
	}
}

extern int main( int argc, char* argv[] );

// Entry point for Android applications.
//
void android_main( struct android_app* androidApp )
{
	release_trace( "Android: Starting application." );

#if DEBUG
	release_trace( "Android: In debug mode" );
#endif

#if DEV_MODE
	release_trace( "Android: In dev mode" );
#endif

	g_androidApp = androidApp;
	ASSERT( g_androidApp );

	// Setup callbacks.
	//
	g_androidApp->onAppCmd = &onAppCmd;
	g_androidApp->onInputEvent = &onAppInput;

	// Run the normal C++-based "main()" function (normally in the project's main.cpp file).
	//
	std::string str( "TODO" );
	std::vector< char > writable( str.begin(), str.end() );
	writable.push_back('\0');
	char* argv[] = { writable.data() };

	::main( 1, argv );
}

// Other Java native callables (called from Java).
//

bool g_androidAuthorized = true;

extern "C"
{
	JNIEXPORT void JNICALL Java_co_electrictoy_fr_FreshNativeActivity_onUnauthorizedPlayDetected( JNIEnv*, jclass )
	{
		release_trace( "aborting unauthorized app" );
		g_androidAuthorized = false;
	}

	JNIEXPORT void JNICALL Java_co_electrictoy_fr_FreshNativeActivity_onAudioFocusGained( JNIEnv*, jclass );
	JNIEXPORT void JNICALL Java_co_electrictoy_fr_FreshNativeActivity_onAudioFocusLost( JNIEnv*, jclass );
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
		{
			g_appImplementation = this;

			REQUIRES( owner );
			m_desiredClocksPerFrame = secondsToClocks( 1.0 / m_desiredFramesPerSecond );
		}

		~ApplicationImplementation()
		{
			shutdownOpenGL();
		}

		bool hasMainWindow() const
		{
			return m_display;
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

			m_window = g_androidApp->window;
			ASSERT( m_window );

			ASSERT( !m_display );

			m_display = eglGetDisplay( EGL_DEFAULT_DISPLAY );

			eglInitialize( m_display, 0, 0 );

			const EGLint attribs[] = {
				EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
				EGL_BLUE_SIZE, 8,
				EGL_GREEN_SIZE, 8,
				EGL_RED_SIZE, 8,
				EGL_DEPTH_SIZE, 16,
				EGL_STENCIL_SIZE, 4,
				EGL_NONE
			};

			EGLint numConfigs;
			EGLBoolean result = eglChooseConfig( m_display, attribs, &m_config, 1, &numConfigs );
			if( !result )
			{
				release_trace( "eglChooseConfig() returned failure." )
			}

			EGLint format;
			eglGetConfigAttrib( m_display, m_config, EGL_NATIVE_VISUAL_ID, &format );

			ANativeWindow_setBuffersGeometry( m_window, 0, 0, format );

			EGLint contextAttribs[] =
			{
				EGL_CONTEXT_CLIENT_VERSION, 2,
				EGL_NONE
			};
			m_context = eglCreateContext( m_display, m_config, 0, contextAttribs );
			ASSERT( m_context != EGL_NO_CONTEXT );

			setupEGLSurface( m_window );
		}

		void setupEGLSurface( ANativeWindow* window )
		{
			m_window = window;

			ASSERT( m_window );
			ASSERT( m_display );
			ASSERT( m_context );

			dev_trace( "Creating EGL surface." );
			m_surface = eglCreateWindowSurface( m_display, m_config, m_window, 0 );

			eglMakeCurrent( m_display, m_surface, m_surface, m_context );
			HANDLE_GL_ERRORS();
		}

		void teardownEGLSurface()
		{
			dev_trace( "Releasing EGL surface." );
			eglDestroySurface( m_display, m_surface );
			m_surface = EGL_NO_SURFACE;
			HANDLE_GL_ERRORS();
		}

		Application::ExitCode runMainLoop( int argc, char* argv[] )
		{
			REQUIRES( !m_isInMainLoop );

#if defined( DEV_MODE ) && 1
			if( !m_commandProcessor->isListenServerStarted() )
			{
				m_commandProcessor->startListenServer();
			}
#endif

			m_isInMainLoop = true;

			int exitCode = 0;

			try
			{
				while( true )
				{
					// Read pending events.
					//
					int events;
					struct android_poll_source* source = nullptr;

					const bool wantsFreshUpdate = m_isUpdating && g_androidAuthorized && m_window;

					const int pollTimeout = wantsFreshUpdate
						? 0 	// no timeout
						: -1; 	// infinite timeout until new event

					if( int ident = ALooper_pollAll( pollTimeout, NULL, &events, reinterpret_cast< void** >( &source )) >= 0 )
					{
						if( source )
						{
							ASSERT( g_androidApp );
							source->process( g_androidApp, source );
						}

						// Exit requested?
						//
						if( g_androidApp->destroyRequested != 0 )
						{
							m_isInMainLoop = false;
							break;
						}
					}
					else if( wantsFreshUpdate )
					{
						// Is it time for another update?
						// TODO could reduce battery drain by sleeping here if enough wait time is ahead of us.
						if( getAbsoluteTimeClocks() >= m_nextUpdateTime )
						{
							m_nextUpdateTime = getAbsoluteTimeClocks() + m_desiredClocksPerFrame;

							updateFrame();

#ifdef DEBUG
							GLint err = glGetError();
							if( err != GL_NO_ERROR )
							{
								dev_error( "OpenGL Error during update." );
							}
#endif
						}
					}
				}

				exitCode = 0;
			}
			catch( const std::exception& e )
			{
				release_trace( "Exception while running main loop: " << e.what() );
				exitCode = -1;
			}
			catch( ... )
			{
				release_trace( "Unknown exception while running main loop." );
				exitCode = -2;
			}

			m_isInMainLoop = false;
			return exitCode;
		}

		void swapBuffers()
		{
			ASSERT( hasMainWindow() );
			eglSwapBuffers( m_display, m_surface );
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
			Vector2i dims;
			eglQuerySurface( m_display, m_surface, EGL_WIDTH, &dims.x );
			eglQuerySurface( m_display, m_surface, EGL_HEIGHT, &dims.y );
			return dims;
		}

		Vector2i getWindowDimensions() const
		{
			return getScreenDimensions();
		}

		void desiredFramesPerSecond( TimeType fps )
		{
			m_desiredFramesPerSecond = fps;
			m_desiredClocksPerFrame = secondsToClocks( 1.0 / m_desiredFramesPerSecond );
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
			m_owner->updateFrame();
		}

		void onSystemAudioFocusGained()
		{
			release_trace( "onSystemAudioFocusGained" );
			if( AudioSystem::doesExist() )
			{
				AudioSystem::instance().unsuspend();
			}
		}

		void onSystemAudioFocusLost()
		{
			release_trace( "onSystemAudioFocusLost." );
			if( AudioSystem::doesExist() )
			{
				release_trace( "onSystemAudioFocusLost - suspending" );
				AudioSystem::instance().suspend();
			}
		}

		void resumeMainLoop()
		{
			m_isUpdating = true;
		}

		void pauseMainLoop()
		{
			m_isUpdating = false;
		}

	protected:

		void shutdownOpenGL()
		{
			eglMakeCurrent( m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
			eglDestroySurface( m_display, m_surface );
			eglDestroyContext( m_display, m_context );
			eglTerminate( m_display );

			m_display = EGL_NO_DISPLAY;
			m_surface = EGL_NO_SURFACE;
			m_context = EGL_NO_CONTEXT;
		}

	private:

		Application* m_owner;
		SmartPtr< CommandProcessor > m_commandProcessor;
		bool m_isInMainLoop;
		TimeType m_desiredFramesPerSecond;

		SystemClock m_nextUpdateTime;
		SystemClock m_desiredClocksPerFrame;

		ANativeWindow* m_window = nullptr;
		EGLDisplay m_display = EGL_NO_DISPLAY;
		EGLConfig m_config;
		EGLSurface m_surface = EGL_NO_SURFACE;
		EGLContext m_context = EGL_NO_CONTEXT;

		bool m_isUpdating = true;
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
		// Ignored on this platform.
	}

	bool Application::isFullscreen() const
	{
		return true;		// Always full screen on this platform.
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
		return { "-Android", "-mobile" };
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
		// TODO: Locale getCurrentLocale(Context context){
//		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N){
//			return context.getResources().getConfiguration().getLocales().get(0);
//		} else{
//			//noinspection deprecation
//			return context.getResources().getConfiguration().locale;
//		}
		return "";
	}
}

#define event_trace release_trace

namespace
{
	ApplicationImplementation* g_appImplementation = nullptr;

	void onAppCmd( struct android_app* app, int32_t cmd )
	{
		// See http://docs.nvidia.com/tegra/Content/AN_LC_Basics_Practice.html
		switch( cmd )
		{
			case APP_CMD_START:
				event_trace( "APP_CMD_START" );
				break;

			case APP_CMD_STOP:
				event_trace( "APP_CMD_STOP" );
				break;

			case APP_CMD_DESTROY:
				event_trace( "APP_CMD_DESTROY" );
				ANativeActivity_finish( app->activity );
				exit( 0 );
				break;

			case APP_CMD_GAINED_FOCUS:
				event_trace( "APP_CMD_GAINED_FOCUS" );
				break;

			case APP_CMD_LOST_FOCUS:
				event_trace( "APP_CMD_LOST_FOCUS" );
				break;

			case APP_CMD_INIT_WINDOW:
				// The window is being shown, get it ready.
				//
				event_trace( "APP_CMD_INIT_WINDOW" );
				ASSERT( app == g_androidApp );
				ASSERT( g_appImplementation );
				if( !g_appImplementation->hasMainWindow() )
				{
					g_appImplementation->createMainWindow();
				}
				else
				{
					g_appImplementation->setupEGLSurface( g_androidApp->window );
				}

				g_appImplementation->resumeMainLoop();
				g_appImplementation->onSystemAudioFocusGained();
				break;

			case APP_CMD_TERM_WINDOW:
				event_trace( "APP_CMD_TERM_WINDOW" );

				if( Application::doesExist() )
				{
					Application::instance().onTerminationThreat();
				}

				// Stop updating.
				//
				g_appImplementation->onSystemAudioFocusLost();
				g_appImplementation->pauseMainLoop();
				g_appImplementation->teardownEGLSurface();
				break;

			case APP_CMD_RESUME:
				// Resume updating.
				event_trace( "APP_CMD_RESUME" );
				g_appImplementation->resumeMainLoop();
				if( Application::doesExist() )
				{
					Application::instance().onWaking();
				}
				break;

			case APP_CMD_PAUSE:
				// Stop updating.
				event_trace( "APP_CMD_PAUSE" );
				g_appImplementation->pauseMainLoop();
				if( Application::doesExist() )
				{
					Application::instance().onSleeping();
				}
				break;

			case APP_CMD_LOW_MEMORY:
				event_trace( "APP_CMD_LOW_MEMORY" );
				if( Application::doesExist() )
				{
					Application::instance().onMemoryWarning();
				}
				break;
		}
	}

	struct NativeTouch
	{
		int id;
		vec2 pos;
		vec2 lastPos;
	};

	std::vector< NativeTouch > g_nativeTouches;

	size_t findNativeTouch( int id )
	{
		auto iter = std::find_if( g_nativeTouches.begin(), g_nativeTouches.end(), [&id]( const NativeTouch& touch ) { return id == touch.id; } );
		if( iter != g_nativeTouches.end() )
		{
			return iter - g_nativeTouches.begin();
		}
		else
		{
			return size_t( -1 );
		}
	}

	Application::Touches createTouches( const NativeTouch& touch )
	{
		const auto& dims = Application::instance().getWindowDimensions();

		Application::Touches touches;

		touches.emplace_back( Application::Touch{
			vec2( touch.pos.x, dims.y - touch.pos.y ),
			vec2( touch.lastPos.x, dims.y - touch.lastPos.y ),
			vec2::ZERO,	// wheel
			touch.id,
			static_cast< int >( g_nativeTouches.size() ),
			1,
			reinterpret_cast< void* >( touch.id ) } );

		return touches;
	}

	int32_t onAppInput( struct android_app* app, AInputEvent* event )
	{
		if( Application::doesExist() )
		{
			auto& application = Application::instance();

			const auto eventType = AInputEvent_getType( event );

			switch( eventType )
			{
				// case AINPUT_EVENT_TYPE_KEY:
				// {
				// 	auto keycode = AKeyEvent_getKeyCode( event );
				// 	switch( keycode )
				// 	{
				//		...
				// 	}
				// 	break;
				// }

				case AINPUT_EVENT_TYPE_MOTION:
				{
					const auto now = AMotionEvent_getEventTime( event );

					const auto rawActionId = AMotionEvent_getAction( event );

					const auto pointerIndex = ( rawActionId & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK ) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
					const auto actionId = rawActionId & AMOTION_EVENT_ACTION_MASK;

					const auto pointerId = AMotionEvent_getPointerId( event, pointerIndex );
					const vec2 pos( AMotionEvent_getX( event, pointerIndex ), AMotionEvent_getY( event, pointerIndex ));

					switch( actionId )
					{
						case AMOTION_EVENT_ACTION_DOWN:
						case AMOTION_EVENT_ACTION_POINTER_DOWN:
						{
							NativeTouch touch{ pointerId, pos, pos };
							g_nativeTouches.push_back( touch );

							auto touches = createTouches( touch );
							application.onTouchesBegin( touches.begin(), touches.end() );
							break;
						}

						case AMOTION_EVENT_ACTION_UP:
						case AMOTION_EVENT_ACTION_POINTER_UP:
						{
							// Find this touch.
							//
							const auto iTouch = findNativeTouch( pointerId );
							if( iTouch < g_nativeTouches.size() )
							{
								auto& touch = g_nativeTouches[ iTouch ];

								touch.lastPos = touch.pos;
								touch.pos = pos;
								auto touches = createTouches( touch );
								application.onTouchesEnd( touches.begin(), touches.end() );

								// Remove the touch.
								//
								g_nativeTouches.erase( g_nativeTouches.begin() + iTouch );
							}
							else
							{
								dev_warning( "ACTION_POINTER_UP: Unrecognized touch id: " << pointerId );
							}

							break;
						}

						case AMOTION_EVENT_ACTION_MOVE:
						{
							// Find this touch.
							//
							const auto iTouch = findNativeTouch( pointerId );
							if( iTouch < g_nativeTouches.size() )
							{
								auto& touch = g_nativeTouches[ iTouch ];

								touch.lastPos = touch.pos;
								touch.pos = pos;
								auto touches = createTouches( touch );
								application.onTouchesMove( touches.begin(), touches.end() );
							}
							else
							{
								dev_warning( "ACTION_POINTER_MOVE: Unrecognized touch id: " << pointerId );
							}

							break;
						}

						case AMOTION_EVENT_ACTION_CANCEL:
						{
							// Find this touch.
							//
							const auto iTouch = findNativeTouch( pointerId );
							if( iTouch < g_nativeTouches.size() )
							{
								auto& touch = g_nativeTouches[ iTouch ];

								touch.lastPos = touch.pos;
								touch.pos = pos;
								auto touches = createTouches( touch );
								application.onTouchesCancelled( touches.begin(), touches.end() );

								// Remove the touch.
								//
								g_nativeTouches.erase( g_nativeTouches.begin() + iTouch );
							}
							else
							{
								dev_warning( "ACTION_POINTER_UP: Unrecognized touch id: " << pointerId );
							}

							break;
						}

						default:
							// Don't care.
							break;
					}
					return 1;
				}

				default:
					// Ignore.
					break;
			}
		}
		return 0;		// Not handling.
	}
}

JNIEXPORT void JNICALL Java_co_electrictoy_fr_FreshNativeActivity_onAudioFocusGained( JNIEnv*, jclass )
{
	if( g_appImplementation )
	{
		g_appImplementation->onSystemAudioFocusGained();
	}
}

JNIEXPORT void JNICALL Java_co_electrictoy_fr_FreshNativeActivity_onAudioFocusLost( JNIEnv*, jclass )
{
	if( g_appImplementation )
	{
		g_appImplementation->onSystemAudioFocusLost();
	}
}
