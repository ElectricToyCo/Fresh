/*
 *  Application.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 12/2/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_APPLICATION_H_INCLUDED
#define FRESH_APPLICATION_H_INCLUDED

#include "Singleton.h"
#include "Assets.h"
#include "Objects.h"
#include "StringTable.h"
#include "FreshVector.h"
#include "EventKeyboard.h"
#include "Gamepad.h"

#ifdef FRESH_PROFILER_ENABLED
#	include "Profiler.h"
#endif

namespace fr
{
	class CommandProcessor;
	class GameCenter;
	class Social;
	class AudioSystem;
	
#define APP_CONFIG_VAR( type, name )	\
	public: SYNTHESIZE( type, name )	\
	private: VAR( type, m_##name )
	
#define APP_CONFIG_DVAR( type, name, default )	\
	public: SYNTHESIZE( type, name )	\
	private: DVAR( type, m_##name, default )
	
	class AppConfig : public fr::Object
	{
		FRESH_DECLARE_CLASS( AppConfig, Object )
	public:
		
		APP_CONFIG_DVAR( TimeType, desiredFramesPerSecond, 60 );
		APP_CONFIG_DVAR( std::string, desiredTitle, "Fresh Application" );
		APP_CONFIG_VAR( Rectanglei, desiredWindowRect );
		APP_CONFIG_VAR( path, classPath );
		APP_CONFIG_VAR( path, stringTablePath );
		APP_CONFIG_DVAR( real, contentScale, 1.0f );
		APP_CONFIG_DVAR( path, assetDatabasePath, "assets/asset-database.fresh" );
		APP_CONFIG_VAR( path, documentsSubfolder );
		APP_CONFIG_VAR( path, logFileName );
		APP_CONFIG_VAR( std::string, telemetryURL );
		APP_CONFIG_DVAR( bool, gameCenterEnabled, true );
		APP_CONFIG_DVAR( bool, startFullScreen, false );
	};
	
#undef APP_CONFIG_VAR
#undef APP_CONFIG_DVAR
	
	// Class Application is responsible for creating, maintaining, and destroying the OpenGL-renderable
	// OS window while presenting a platform-independent interface to the user.
	// During initialization (prior to the first call to OnPreUpdate()), it creates the Renderer.
	// It is also responsible for maintaining the main game loop at a given maximum frame rate.
	
	class Application : public Singleton< Application >
	{
	public:
		
		typedef int ExitCode;
		
		typedef std::vector< ClassInfo::Name > ClassSequence;
		
		explicit Application( const path& configFilePath );
		// REQUIRES( !configFilePath.empty() );

		virtual ~Application();
		
		virtual void onMemoryWarning();
		virtual void onTerminationThreat();
		virtual void onTerminating();
		virtual void onWaking();
		virtual void onSleeping();
		
		bool terminating() const;
		
		CommandProcessor& commandProcessor();
		Package& systemPackage() const;
		AssetPackage& assetPackage() const;
		
		// File dialog prompting.
		std::string getPromptedFilePath( bool forSaveElseOpen, const char* semicolonSeparatedFileExtensions );
		
		void quit( ExitCode exitCode = 0 );
		
		const AppConfig& config() const;
		
		GameCenter& gameCenter() const;
		Social& social() const;
		GamepadManager& gamepadManager() const;
		
		std::string userLanguageCode() const;
		
		bool isMultitouch() const;		// True if the app may receive multiple simultaneous touches.
		
		Vector2i getScreenDimensions() const;
		Vector2i getWindowDimensions() const;
		float getWindowAspectRatio() const
		{
			Vector2i dimensions( getWindowDimensions() );			
			return dimensions.x / static_cast< float >( dimensions.y );
		}
		real pixelsPerScreenInch() const;
		TimeType desiredFramesPerSecond() const;
		void desiredFramesPerSecond( TimeType framesPerSecond );
		
		std::string windowTitle() const;
		void windowTitle( const std::string& value );
		void setWindowTitleToDefault();

		bool isFullscreen() const;
		void goFullscreen( bool fullscreenElseWindowed );
		void toggleFullscreen();
		
		rect safeAreaInsets() const;
		
		bool isMainLoopRunning() const;
		ExitCode runMainLoop( int argc, const char* argv[] );
		// REQUIRES( !IsMainLoopRunning() );
		
		void updateFrame();

		void swapBuffers();

		SYNTHESIZE_GET( bool, didStartupWithAlternativeKeyDown )
		
		// Functions to override in subclasses.
		//
		virtual void onPreFirstUpdate() {}
		// Called after initialization but before the first update is executed.
		virtual void update();
		// Called every frame.
		virtual void onResize( int newWidth, int newHeight ) {}
		// Called when the window size changes.
		
		// Touch-related stuff.
		//
		struct Touch
		{
			typedef void* Id;
			
			vec2 position{0};
			vec2 lastPosition{0};
			int iTouch{0};
			int nTouches{0};
			int nTaps{0};
			Id touchId{0};
			vec2 wheelDelta{0};
			
			Touch() {}
			
			Touch( const vec2& position_,
				const vec2& lastPosition_,
				const vec2& wheelDelta_,
				int iTouch_,
				int nTouches_,
				int nTaps_,
				void* touchId_ )
			:	position( position_ )
			,	lastPosition( lastPosition_ )
			,	iTouch( iTouch_ )
			,	nTouches( nTouches_ )
			,	nTaps( nTaps_ )
			,	touchId( touchId_ )
			,	wheelDelta( wheelDelta_ )
			{}
			
		};
		typedef std::vector< Touch > Touches;
		typedef std::vector< Touch >::const_iterator TouchIter;
		
		virtual void onTouchesBegin( TouchIter begin, TouchIter end ) {}
		virtual void onTouchesMove( TouchIter begin, TouchIter end ) {}
		virtual void onTouchesEnd( TouchIter begin, TouchIter end ) {}
		virtual void onTouchesCancelled( TouchIter begin, TouchIter end )			{ onTouchesEnd( begin, end ); }
		virtual void onWheelMove( TouchIter begin, TouchIter end ) {}
		
		virtual void onKeyUp( const EventKeyboard& event ) {}
		virtual void onKeyDown( const EventKeyboard& event ) {}
		
		virtual void onGainedFocus() {}
		virtual void onLostFocus() {}
		
		virtual void onWindowReshape() {}
		
		void enableTelemetry( bool doEnable );
		void dumpAssets();
		void purgeAssets();
		
		void clearAchievements();
		
	protected:
		
		static void staticAtExit();
		void atExit();
		
		void constructImplementation();
		void destroyImplementation();
		
		path amendConfigFilePath( const path& defaultConfigPath ) const;
		std::vector< std::string > getPlatformConfigFileSuffixes() const;
		std::vector< std::string > getVariantConfigFileSuffixes( const std::string& platformSuffix ) const;
		
		bool isApplicationAlternativeStartupKeyDown() const;
		
		void loadStringTable();
		
		void createAudioSystem();
		
		bool wantsFullScreenStartup() const;
		void wantsFullScreenStartup( bool fullscreen );
		
	private:
		
		class ApplicationImplementation* m_impl;
		
		AppConfig::ptr m_config;
		
		StringTable::ptr m_stringTable;
				
		SmartPtr< CommandProcessor > m_commandProcessor;
		Package::ptr m_systemPackage;
		AssetPackage::ptr m_assetPackage;
		
		GamepadManager::ptr m_gamepadManager;

		bool m_didStartupWithAlternativeKeyDown = false;
		bool m_terminating = false;
		size_t m_nUpdates = 0;

		std::unique_ptr< GameCenter > m_gameCenter;
		std::unique_ptr< Social > m_social;
		
		SmartPtr< AudioSystem > m_audioSystem;

#if FRESH_TELEMETRY_ENABLED
		std::unique_ptr< class UserTelemetry > m_telemetry;
#endif

#ifdef FRESH_PROFILER_ENABLED
		Profiler m_profiler;
#endif

		void desiredFramesPerSecondDetail( TimeType framesPerSecond );		// Per platform.
		
		FRESH_PREVENT_COPYING( Application )
	};
	
}


#endif
