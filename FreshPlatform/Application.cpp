//
//  Application.cpp
//  Fresh
//
//  Created by Jeff Wofford on 7/5/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "Application.h"
#include "CommandProcessor.h"
#include "FreshFile.h"
#include "FreshTime.h"
#include "FreshThread.h"
#include "ObjectLinker.h"
#include "AudioSystem.h"
#include "Renderer.h"
#include "FreshVersioning.h"
#include "FreshGameCenter.h"
#include "FreshSocial.h"
#include "FreshLicensing.h"

#if FRESH_TELEMETRY_ENABLED
#	include "UserTelemetry.h"
#endif

namespace
{
	const char* const FULLSCREEN_PREFERENCE = "startupFullscreen";
}

namespace fr
{

#define APP_CONFIG_VAR( type, name )	\
	DEFINE_VAR( AppConfig, type, m_##name );

#define APP_CONFIG_DVAR( type, name, default )	\
	DEFINE_VAR( AppConfig, type, m_##name );

	FRESH_DEFINE_CLASS( AppConfig )
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
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( AppConfig )

	/////////////////////////////////////////////////////////////////////////////

	Application::Application( const path& configFilePath )
	:	Singleton< Application >( this )
	{
		std::atexit( &Application::staticAtExit );

		release_trace( "Starting Fresh application." );
		dev_trace( "dev_trace enabled." );
#ifdef DEBUG
		release_trace( "DEBUG defined." );
#else
		release_trace( "DEBUG undefined." );
#endif

#ifdef DEV_MODE
		release_trace( "DEV_MODE=" << DEV_MODE << "." );
#else
		release_trace( "DEV_MODE undefined." );
#endif

#if FRESH_ALLOW_THREADING
		release_trace( "FRESH_ALLOW_THREADING is enabled" );
#else
		release_trace( "FRESH_ALLOW_THREADING is disabled" );
#endif

#ifdef FRESH_PROFILER_ENABLED
		release_trace( "Profiling enabled." );
#else
		release_trace( "Profiling disabled." );
#endif

		// Update the working directory if requested.
		//
		processWorkingDirectoryRedirection();

		try
		{
			release_trace( "Seeking version file...")
			version::infoFilePath( getResourcePath( "assets/ver/version.txt" ).string() );
		}
		catch( ... )
		{
			dev_warning( "Could not load version.txt file." );
		}

		// Create Fresh system facilities.
		//
		release_trace( "Initiating reflection system.")
		fr::initReflection();

		ObjectLinker::create();

		m_systemPackage = createPackage( "~system" );
		addSearchPackage( m_systemPackage );

		// Load configuration.
		//
		path actualConfigPath( configFilePath );
		REQUIRES( !actualConfigPath.empty() );

		try
		{
			actualConfigPath = amendConfigFilePath( actualConfigPath );
			release_trace( "The amended config package is " << actualConfigPath );
		}
		catch( ... )
		{
			release_warning( "Application problem amending config path. Trying default (" << actualConfigPath << ")." );
		}

		try
		{
			release_trace( "Loading config package at " << actualConfigPath );
			Package::ptr configPackage = loadPackage( actualConfigPath );
			m_config = configPackage->find< AppConfig >( "config" );

			if( !m_config )
			{
				release_error( "Application config package '" << actualConfigPath << "' had no AppConfig'config' member. Using defaults." );
			}
		}
		catch( const std::exception& e )
		{
			release_error( "Application config package '" << actualConfigPath << "' could not be loaded. " << e.what() );
			throw;
		}

		if( !m_config )
		{
			m_config = createObject< AppConfig >( "config" );
		}
		ASSERT( m_config );

		release_trace( m_config->desiredTitle() << " " << version::info() );
		release_trace( "Running on platform: " << getPlatform() );
		release_trace( "Device ID: " << getDeviceId() );

		// Apply the document subfolder
		if( !m_config->documentsSubfolder().empty() )
		{
			release_trace( "Using document subfolder " << m_config->documentsSubfolder() );
			documentSubfolderPath( m_config->documentsSubfolder() );
		}

		loadStringTable();

		// Apply configuration.
		//
#if FRESH_TELEMETRY_ENABLED
		if( !m_config->telemetryURL().empty() )
		{
			release_trace( "Initiating telemetry with " << m_config->telemetryURL() << "." );
			m_telemetry.reset( new UserTelemetry( m_config->telemetryURL() ));
			m_telemetry->startSession( "~unknown", getPlatform(), getOSVersion(), getDeviceId(), version::info() );
		}
#endif

		if( !m_config->logFileName().empty() )
		{
			release_trace( "Creating logfile " << m_config->logFileName() << "." );

			try
			{
				std::unique_ptr< std::ofstream > logFile( new std::ofstream( getDocumentPath( m_config->logFileName() ).c_str() ));
				DevLog::setLogStream( std::move( logFile ));
			}
			catch( ... )
			{
				dev_warning( "Unable to create log file " << m_config->logFileName() );
			}
		}

		// Load asset database.
		//
		release_trace( "Loading asset database " << m_config->assetDatabasePath() << "." );
		m_assetPackage = createPackage< AssetPackage >( "~asset_manager" );
		try
		{
			m_assetPackage->loadDatabase( m_config->assetDatabasePath() );
		}
		catch( const std::exception& e )
		{
			release_error( "Error opening asset database " << m_config->assetDatabasePath() << ": " << e.what() );
		}

		// Construct command processor.
		//
		release_trace( "Creating command processor." );
		m_commandProcessor = createObject< CommandProcessor >( "command processor" );

		// Construct application implementation.
		//
		release_trace( "Constructing app implementation." );
		constructImplementation();

		// Prepare GameCenter
		//
		release_trace( "Creating Game Center." );
		m_gameCenter.reset( new GameCenter() );
		if( m_config->gameCenterEnabled() )
		{
			m_gameCenter->authenticate();
		}

		// Prepare Social interface (Twitter and Facebook)
		//
		release_trace( "Initiating social interface." );
		m_social.reset( new Social() );

		// Prepare gamepad support.
		//
		release_trace( "Initiating gamepad." );
		m_gamepadManager = createObject< GamepadManager >( systemPackage(), "gamepad_manager" );

		//
		// Create console commands.
		//

		release_trace( "Adding console commands." );

		// Create the telemetry command.
		//
		{
			auto caller = make_caller< void, bool  >( std::bind( &Application::enableTelemetry, this, std::placeholders::_1 ) );
			auto command = m_commandProcessor->registerCommand( this, "telemetry", "enables or disables user telemetry events. Session and context reports are unaffected.", std::move( caller ));
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "<bool>", true, "specifies whether to enable or disable telemetry" } ));
		}

		// Create the quit/exit command.
		//
		{
			auto caller = make_caller< void >( std::bind( &Application::quit, this, 0 ) );
			m_commandProcessor->registerCommand( this, "quit", "quits the application. (alias 'exit'.)", caller );
			m_commandProcessor->registerCommand( this, "exit", "quits the application. (alias 'quit'.)", caller );
		}

		// Create the dumpassets command.
		//
		{
			auto caller = stream_function< void() >( std::bind( &Application::dumpAssets, this ) );
			m_commandProcessor->registerCommand( this, "dumpassets", "displays information about available Assets", std::move( caller ) );
		}

		// Create the purgeassets command.
		//
		{
			auto caller = stream_function< void() >( std::bind( &Application::purgeAssets, this ) );
			m_commandProcessor->registerCommand( this, "purgeassets", "releases currently unused Assets", std::move( caller ) );
		}

		// Create the clearachievements command.
		//
		{
			auto caller = stream_function< void() >( std::bind( &Application::clearAchievements, this ) );
			m_commandProcessor->registerCommand( this, "clearachievements", "Clears the current player's Game Center achievement record", std::move( caller ) );
		}

		release_trace( "Application done constructing." );
	}

	Application::~Application()
	{
		atExit();
		m_commandProcessor->unregisterAllCommandsForHost( this );
		destroyImplementation();
	}

	const AppConfig& Application::config() const
	{
		ASSERT( m_config );
		return *m_config;
	}

	GameCenter& Application::gameCenter() const
	{
		ASSERT( m_gameCenter );
		return *m_gameCenter;
	}

	Social& Application::social() const
	{
		ASSERT( m_social );
		return *m_social;
	}

	GamepadManager& Application::gamepadManager() const
	{
		ASSERT( m_gamepadManager );
		return *m_gamepadManager;
	}

	CommandProcessor& Application::commandProcessor()
	{
		ASSERT( m_commandProcessor );
		return *m_commandProcessor;
	}

	Package& Application::systemPackage() const
	{
		ASSERT( m_systemPackage );
		return *m_systemPackage ;
	}

	AssetPackage& Application::assetPackage() const
	{
		ASSERT( m_assetPackage );
		return *m_assetPackage;
	}

	void Application::loadStringTable()
	{
		// What language does the user work in?
		//
		const auto languageCode = userLanguageCode();

		release_trace( "Language: " << languageCode );

		auto loadStringTable = [&]( const path& basePath, const std::string& languageCode ) -> StringTable::ptr
		{
			auto stringTablePath = []( const path& basePath, const std::string& languageCode )
			{
				if( languageCode.empty() )
				{
					return basePath;
				}

				// Inject the language code as an expected folder before the filename.
				//
				return basePath.parent_path() / languageCode / basePath.filename();
			};

			const auto stringsPath = stringTablePath( basePath, languageCode );

			release_trace( "Trying string table path: " << stringsPath );

			try
			{
				auto stringTablePackage = loadPackage( stringsPath );
				return stringTablePackage->find< StringTable >( "strings" );
			}
			catch( const std::exception& e )
			{
				release_warning( "Could not load string table: " << e.what() );
				return nullptr;
			}
		};

		auto stringTablePath = m_config->stringTablePath();
		if( !stringTablePath.empty() )
		{
			// Try the user's locale first.
			//
			m_stringTable = loadStringTable( stringTablePath, languageCode );

			// If failed, try the base locale.
			//
			if( !m_stringTable )
			{
				m_stringTable = loadStringTable( stringTablePath.string(), "" );
			}
		}

		// Did we load a string table?
		//
		if( !m_stringTable )
		{
			// No. Use a blank one as the default.
			//
			release_trace( "Using blank string table." );
			m_stringTable = createObject< StringTable >( getTransientPackage() );
		}
	}

	void Application::createAudioSystem()
	{
		try
		{
			trace( "Creating AudioSystem." );
			m_audioSystem = createObject< AudioSystem >( &systemPackage(), AudioSystem::StaticGetClassInfo(), "theAudioSystem" );
		}
		catch( const std::exception& e )
		{
			release_error( "Exception while creating audio system: '" << e.what() << "'" );
		}
		catch( ... )
		{
			release_error( "Unknown exception while creating audio system." );
		}
	}

	bool Application::wantsFullScreenStartup() const
	{
		bool wants = false;
		if( m_config )
		{
			wants = m_config->startFullScreen();
		}
		std::string result;
		if( loadPreference( FULLSCREEN_PREFERENCE, result ))
		{
			std::istringstream stream( result );
			stream >> std::boolalpha >> wants;
		}

		return wants;
	}

	void Application::wantsFullScreenStartup( bool fullscreen )
	{
		std::ostringstream stream;
		stream << std::boolalpha << fullscreen;

		savePreference( FULLSCREEN_PREFERENCE, stream.str().c_str() );
	}

	void Application::updateFrame()
	{
		TIMER_AUTO_FUNC

		if( m_nUpdates == 0 )
		{
			// Create the render and audio system.
			//
			createObject< Renderer >( &systemPackage(), Renderer::StaticGetClassInfo(), "renderer" );

			createAudioSystem();

			// Load classes file, if any.
			// Must be last thing before first update because all prior systems (e.g. renderer) need to have been constructed.
			//
			if( !m_config->classPath().empty() )
			{
				try
				{
					auto classPath = m_config->classPath();
					dev_trace( "Loading classes from '" << classPath << "'." );
					loadPackage( classPath );
					dev_trace( "Done." );
				}
				catch( const std::exception& e )
				{
					release_error( e.what() );
				}
			}
		}

		//
		// Perform the update.
		//

		// Update Telnet server.
		//
		ASSERT( m_commandProcessor );
		m_commandProcessor->telnetServer().updateConnections();

		// The viewport based on the window dimensions.
		//
		const Vector2i& dimensions = getWindowDimensions();
		Renderer::instance().setViewport( Rectanglei( 0, 0, dimensions.x, dimensions.y ));

		// The gamepads.
		//
		ASSERT( m_gamepadManager );
		m_gamepadManager->update();

		// The main dispatch queue.
		//
		{
			TIMER_AUTO( Poll main dispatch queue )
			dispatch::mainQueue().poll();
		}

		// Do the preFirstUpdate call for the stage if it's the first update.
		//
		if( m_nUpdates == 0 )
		{
			m_didStartupWithAlternativeKeyDown = isApplicationAlternativeStartupKeyDown();

			onPreFirstUpdate();
		}

		// Do the update.
		//
		update();

#if DEV_MODE
		// Simulate failures.
		//
		{
			const auto& message = m_commandProcessor->coreCommands().desiredSimulatedThrowMessage();
			if( !message.empty() )
			{
				FRESH_THROW( FreshException, message );
			}

			if( m_commandProcessor->coreCommands().wantsSimulatedGPF() )
			{
				int* p = nullptr;
				*p = 0xFFFFFF;
			}
		}
#endif

		++m_nUpdates;
	}

	void Application::update()
	{
		TIMER_AUTO_FUNC
		swapBuffers();
	}

	void Application::onMemoryWarning()
	{
		onTerminationThreat();
		assetPackage().releaseRetainedZombies();
	}

	void Application::onWaking()
	{}

	void Application::onSleeping()
	{
		onTerminationThreat();
	}

	void Application::setWindowTitleToDefault()
	{
		windowTitle( m_config->desiredTitle() );
	}

	TimeType Application::desiredFramesPerSecond() const
	{
		return m_config->desiredFramesPerSecond();
	}

	void Application::desiredFramesPerSecond( TimeType framesPerSecond )
	{
		m_config->desiredFramesPerSecond( framesPerSecond );
		desiredFramesPerSecondDetail( framesPerSecond );
	}

	path Application::amendConfigFilePath( const path& defaultPath ) const
	{
		const auto tryPathWithSuffix = []( const path& basePath, const std::string& suffix )
		{
			auto extension = basePath.extension();
			ASSERT( extension.front() == '.' );
			extension.erase( extension.begin() );

			path variant = change_extension( change_extension( basePath, "" ) + suffix, extension );

			try
			{
				if( exists( getResourcePath( variant )))
				{
					return variant;
				}
			}
			catch( ... )
			{}

			return path{};
		};

		const auto platformSuffixes = getPlatformConfigFileSuffixes();

		for( const auto& platformSuffix : platformSuffixes )
		{
			auto platformVariants = getVariantConfigFileSuffixes( platformSuffix );
			platformVariants.push_back( "" );

			for( const auto& platformVariant : platformVariants )
			{
				auto amendedConfigPath = tryPathWithSuffix( defaultPath, platformSuffix + platformVariant );

				if( !amendedConfigPath.empty() )
				{
					return amendedConfigPath;
				}
			}
		}

		return defaultPath;
	}

	void Application::onTerminationThreat()
	{}

	void Application::onTerminating()
	{
		m_terminating = true;
		onTerminationThreat();
	}

	bool Application::terminating() const
	{
		return m_terminating;
	}

	void Application::atExit()
	{
#if FRESH_TELEMETRY_ENABLED
		if( m_telemetry )
		{
			m_telemetry->endSession();
		}
#endif

		onTerminating();
	}

	void Application::staticAtExit()
	{
		if( Application::doesExist() )
		{
			Application::instance().atExit();
		}

		DevLog::flushLogStream();
	}

	void Application::toggleFullscreen()
	{
		goFullscreen( !isFullscreen() );
	}

	void Application::enableTelemetry( bool doEnable )
	{
#if FRESH_TELEMETRY_ENABLED
		if( m_telemetry )
		{
			m_telemetry->eventsEnabled( doEnable );
		}
#else
		trace( "FreshTelemetry disabled at compile time." );
#endif
	}

	void Application::dumpAssets()
	{
		std::ostringstream assetTrace;
		assetPackage().dumpAssetReport( assetTrace );
		trace( assetTrace.str() );
	}

	void Application::purgeAssets()
	{
		assetPackage().releaseRetainedZombies();
	}

	void Application::clearAchievements()
	{
		gameCenter().clearAllAchievementProgress();
	}
}
