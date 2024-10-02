//
//  FantasyConsole.cpp
//  Fresh
//
//  Created by Jeff Wofford on 6/22/18.
//  Copyright (c) 2018 Jeff Wofford. All rights reserved.
//

#include "FantasyConsole.h"

#include "ApiImplementation.h"

#include "AudioSystem.h"
#include "ImageLoader.h"
#include "VirtualTrackball.h"
#include "FreshFile.h"
#include "Stage.h"
#include "Application.h"
#include "Dispatch.h"
#include "CommandProcessor.h"
#include "FreshXML.h"
using namespace luacpp;

namespace
{
	const fr::path MAIN_SCRIPT_FILENAME = "main.lua";
	const fr::path MAIN_SPRITESHEET_FILENAME = "sprites.png";
	const fr::path MAIN_MAP_FILENAME = "map.tmx";
	const fr::path AUDIO_FOLDER = "audio";

	const double SECONDS_PER_FRAME = 1.0 / 60.0;

    const fr::ScreenEffects::ShaderState DEFAULT_SHADER_STATE;
}

namespace luacpp
{
	// TODO move to optionals.
	const std::string LuaDefault< std::string >::value = "~<~DEFAULT~>~";

    // TODO Move to LuaCppInterop.hpp header

	std::string printStackValue( lua_State* L, int ndx = -1 )
	{
		std::ostringstream str;

		const int type = lua_type( L, ndx );

		str << luaTypeName( type ) << ": ";

		switch( type )
		{
			case LUA_TNIL:
			case LUA_TFUNCTION:
			default:
			{
				str << "..";
				break;
			}

			case LUA_TTABLE:
			case LUA_TLIGHTUSERDATA:
			case LUA_TUSERDATA:
			case LUA_TTHREAD:
			{
				str << std::hex << std::showbase << reinterpret_cast< unsigned long long >( lua_topointer( L, ndx ));
				break;
			}
			case LUA_TBOOLEAN:
			{
				str << std::boolalpha << lua_toboolean( L, ndx );
				break;
			}
			case LUA_TNUMBER:
			{
				str << lua_tonumber( L,ndx );
				break;
			}
			case LUA_TSTRING:
			{
				str << "'" << lua_tostring( L, ndx ) << "'";
				break;
			}
		}

		return str.str();
	}

	std::string printStack( lua_State* lua )
	{
		std::ostringstream str;

		ASSERT( lua );
		const auto nStackElements = lua_gettop( lua );
		for( int i = -1; i >= -nStackElements; --i )
		{
			str << "\n  #" << i << ": " << printStackValue( lua, i );
		}

		return str.str();
	}

	std::string printTable( lua_State* L, std::vector< const void* >& history, int maxDepth = 0 )
	{
		ASSERT( lua_type( L, -1 ) == LUA_TTABLE );

		if( maxDepth > 0 && static_cast< int >( history.size() ) > maxDepth )
		{
			return {};
		}

		const auto indents = []( size_t n ) {
			return std::string( n * 2, ' ' );
		};

		std::ostringstream str;

		lua_pushnil(L);

//		trace( "stack before first table next():\n" << printStack(L) );

		auto next = [&]() {
//			trace( "next: " << printStackValue( L, -1 ));
			return lua_next( L, -2 );
		};

		while( next() )
		{
//			trace( "stack after next():\n" << printStack(L) );

			const std::string key = lua_tostring( L, -2 );
			str << indents( history.size() ) << key << ": ";

//			trace( "next key is '" << key << "'" );

			str << printStackValue( L, -1 );

			if( lua_type( L, -1 ) == LUA_TTABLE )
			{
				// Recurse

				// Only recurse if we haven't seen this table before.
				const auto tableAddress = lua_topointer( L, -1 );

				if( history.end() == std::find( history.begin(), history.end(), tableAddress ) )
				{
					history.push_back( tableAddress );
					str << printTable( L, history, maxDepth );
					ASSERT( history.back() == tableAddress );
					history.pop_back();
				}
				else
				{
					str << "(cycle)";
					lua_pop(L, 1);
				}
			}
			else
			{
				lua_pop(L, 1);
			}
//			trace( "stack after element pop():\n" << printStack(L) );

			str << "\n";
		}
//		trace( "stack before table pop():\n" << printStack(L) );
		lua_pop(L, 1);
//		trace( "stack after table pop():\n" << printStack(L) );

		return str.str();
	}

	std::string printGlobals( lua_State* L, int maxDepth = 0 )
	{
		lua_pushglobaltable( L );
		std::vector< const void* > history;
		return printTable( L, history, maxDepth );
	}
}

namespace fr
{
	const char* FantasyConsole::COMMAND = "Command";
	const char* FantasyConsole::MESSAGE = "Message";
	const char* FantasyConsole::VIRTUAL_CONTROLS_SHOW = "VIRTUAL_CONTROLS_SHOW";
	const char* FantasyConsole::VIRTUAL_CONTROLS_HIDE = "VIRTUAL_CONTROLS_HIDE";
	const char* FantasyConsole::VIRTUAL_TRACKBALL_USED = "VIRTUAL_TRACKBALL_USED";


	FRESH_DEFINE_CLASS( FantasyConsole )
	DEFINE_VAR( FantasyConsole, path, m_gamesFolder );
	DEFINE_VAR( FantasyConsole, path, m_gamePath );
	DEFINE_VAR( FantasyConsole, vec2i, m_baseSpriteSizeInTexels );
	DEFINE_VAR( FantasyConsole, Texture::FilterMode, m_filterMode );
	DEFINE_VAR( FantasyConsole, real, m_linePadding );
	DEFINE_VAR( FantasyConsole, vec2i, m_defaultScreenSize );
    DEFINE_VAR( FantasyConsole, ScreenEffects::ShaderState, m_consoleShaderState );
    DEFINE_VAR( FantasyConsole, real, m_globalTextScale );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( FantasyConsole )

	FantasyConsole::~FantasyConsole()
	{
		// This insanity is to resolve problems with essential self-deletion during destruction.
		if( !Application::instance().terminating() )
		{
			destroyLua();
		}
	}

	void FantasyConsole::setDitherPattern( uint bits )
	{
		int i = 1;
		for( int y = 3; y > -1; --y )
		{
			for( int x = 3; x > -1; --x )
			{
				m_ditherPattern[x][y] = ( bits & i ) ? 0.0f : 1.0f;
				i <<= 1;
			}
		}
	}

	bool FantasyConsole::supportsVirtualControls() const
	{
		return m_virtualTrackball != nullptr;
	}

	std::string FantasyConsole::saveMemento() const
	{
		try
		{
			if( luacpp::doesLuaFunctionExist( m_lua, "save" ))
			{
				return callScriptFunction< std::string >( "save" );
			}
			else
			{
				release_trace( "Game script has no 'save' function. Game state will not persist. This may be the intended game behavior." );
				return {};
			}
		}
		catch( const std::exception& e )
		{
			release_trace( "LUA error (calling `save()`): " << e.what() );
			return {};
		}
	}

	void FantasyConsole::loadMemento( const std::string& memento )
	{
		try
		{
			if( luacpp::doesLuaFunctionExist( m_lua, "load" ))
			{
				callScriptFunction( "load", memento );
			}
			else
			{
				release_warning( "Game script has no 'load' function. The previously stored persistence memento will not be restored." );
			}
		}
		catch( const std::exception& e )
		{
			release_trace( "LUA error (calling `load()`): " << e.what() );
		}
	}

	void FantasyConsole::clear()
	{
		m_nextDefaultPrintPosition.setToZero();

		setDitherPattern( 0 );

		for( const auto& pair : m_audioCues )
		{
			pair.second->eachSound( []( const Sound::ptr& sound )
								   {
									   sound->stop();
								   } );
		}
		m_audioCues.clear();

		if( m_musicSound )
		{
			m_musicSound->stop();
		}
		m_musicSound = nullptr;

		spriteSheet( nullptr );
		m_spriteFlags.clear();

		m_mapLayerSizes.clear();
		m_mapSpriteLayers.clear();

		m_buttonsDownPrev.clear();
		m_buttonsDown.clear();
		m_joystickStates.clear();

		m_globalTextScale = 1.0;

		m_screen = nullptr;

        m_consoleShaderState = DEFAULT_SHADER_STATE;

		m_virtualTrackball = nullptr;
	}

	void FantasyConsole::loadGame( const path& path )
	{
		// Clear prior state.
		//
		clear();

		createLua();

		isTouchEnabled( true );
		isDragEnabled( true );

        if( !m_screen )
        {
            m_screen = createObject< FantasyConsoleScreen >();
        }
		screen_size( m_defaultScreenSize.x, m_defaultScreenSize.y );

		m_gamePath = path;

		// Find the game folder.
		//
		auto absolutePath = m_gamePath;

		if( !exists( absolutePath ))
		{
			absolutePath = gamesBasePath() / m_gamePath;
		}

		if( !exists( absolutePath ))
		{
			FRESH_THROW( FreshException,
						 "Unable to load '" << path << "': could not find this folder.\n"  <<
						 "    > Also not found at '" << absolutePath << "'." );
		}

		// Load the spritesheet.
		//
		loadSpritesheet( absolutePath / MAIN_SPRITESHEET_FILENAME );

		// Load the map.
		const auto mapPath = absolutePath / MAIN_MAP_FILENAME;
		if( exists( mapPath ))
		{
			loadMap( mapPath );
		}

		// Preload the audio files.
		//
		auto audioBasePath = absolutePath / AUDIO_FOLDER;
		if( !exists( audioBasePath ))
		{
			console_trace( "Found no audio folder to preload at '" << audioBasePath << "'." );
		}
		else
		{
			loadAudioFolder( audioBasePath );
		}

		// Load and run the script.
		//
		auto scriptPath = absolutePath / MAIN_SCRIPT_FILENAME;
		if( !exists( scriptPath ))
		{
			FRESH_THROW( FreshException,
						 "Unable to load game main script '" << scriptPath << "': could not find this file." );
		}

		m_tickCount = 0;

		loadScript( scriptPath );
	}

	std::vector< uint > FantasyConsole::amendSpritesheetTexels( const unsigned char* texelBytesRGBA, vec2ui& inOutDimensions )
	{
		// Copy the spritesheet texels.
		//
		const size_t nSpritesheetTexels = inOutDimensions.x * inOutDimensions.y;
		const auto firstTexel = reinterpret_cast< const uint* >( texelBytesRGBA );

		ASSERT( firstTexel || nSpritesheetTexels == 0 );

		std::vector< uint > texels( firstTexel, firstTexel + nSpritesheetTexels );

		// Append the "blank" sprite area used for apparently untextured things like rectangles and circles.
		//
		texels.insert( texels.end(), inOutDimensions.x * m_baseSpriteSizeInTexels.y, Color::White );
		inOutDimensions.y += m_baseSpriteSizeInTexels.y;

		//
		// Append fonts.
		//

		m_fontsBaseTexelPos = vec2i( 0, inOutDimensions.y );

		// Load the font texels.
		bool hasAlpha, premultiplied;
		const auto fontTexels = ImageLoader::loadImageRGBA( fr::getResourcePath( "assets/art/fresh_fonts.png" ), m_fontTexelDimensions.x, m_fontTexelDimensions.y, hasAlpha, premultiplied, ImageLoader::PremultiplyPolicy::WantPremultiplied );

		// TODO: Support smaller spritesheets than the font texture size.
		ASSERT( m_fontTexelDimensions.x <= inOutDimensions.x );

		// Copy the font texels into the texture.
		const size_t excessWidth = inOutDimensions.x - m_fontTexelDimensions.x;

		for( uint fontTextureRow = 0; fontTextureRow < m_fontTexelDimensions.y; ++fontTextureRow )
		{
			const auto rowStart = reinterpret_cast< const uint* >( fontTexels.get() ) + fontTextureRow * m_fontTexelDimensions.x;

			// Copy the row of font texels.
			texels.insert( texels.end(), rowStart, rowStart + m_fontTexelDimensions.x );

			// Add extra space to the right end of the row to pad it out in compatability with the base spritesheet width.
			texels.insert( texels.end(), excessWidth, Color::Invisible );
		}

		inOutDimensions.y += m_fontTexelDimensions.y;

		return texels;
	}

	void FantasyConsole::console( const std::string& message ) const
	{
		const_cast< FantasyConsole* >( this )->console( message );
	}

	void FantasyConsole::console( const std::string& message )
	{
		if( hasScriptFunction( "console" ))
		{
			callScriptFunction( "console", message );
		}
		else
		{
			trace( message );

			ConsoleEvent event( MESSAGE, this );
			event.message( message );
			dispatchEvent( &event );
		}
	}

	vec2i FantasyConsole::textureDimensions() const
	{
		return spriteSheet() ? vector_cast< int >( spriteSheet()->dimensions() ) : vec2i();
	}

	vec2i FantasyConsole::userSpriteAreaDimensions() const
	{
		return m_userSpriteAreaDimensions;
	}

	vec2i FantasyConsole::spritesDimensions() const
	{
		return userSpriteAreaDimensions() / baseSpriteSizeInTexels();
	}

	int FantasyConsole::maxSpriteIndex() const
	{
		const auto dims = spritesDimensions();
		return dims.x * dims.y;
	}

	vec2i FantasyConsole::spriteIndexToSpritePos( int spriteIndex ) const
	{
		const auto dims = spritesDimensions();
		ASSERT( dims.x > 0 && dims.y > 0 );

		return vec2i( spriteIndex % dims.x, ( spriteIndex / dims.x ) % dims.y );
	}

	vec2i FantasyConsole::spriteSheetPosToTexel( const vec2i& spritePos ) const
	{
		return baseSpriteSizeInTexels() * spritePos;
	}

	vec2 FantasyConsole::texelsToTexCoords( const vec2i& texels ) const
	{
		const auto dims = vector_cast< real >( textureDimensions());
		ASSERT( dims.x > 0 && dims.y > 0 );

		return vector_cast< real >( texels ) / dims;
	}

	fr::rect FantasyConsole::blankTexCoords() const
	{
		const auto texelInTexCoords = 1.0f / vector_cast< real >( textureDimensions() );

		const auto ul = vec2( 0, userSpriteAreaDimensions().y / static_cast< real >( textureDimensions().y ));
		return fr::rect{ ul, ul + texelInTexCoords };
	}

	vec2i FantasyConsole::fontGridSize() const
	{
		return vec2i( 16 );
	}

	vec2i FantasyConsole::fontGlyphTexelSize() const
	{
		return vec2i( 8 );
	}

	int FantasyConsole::numFonts() const
	{
		return 4;	// TODO: Magic
	}

	fr::rect FantasyConsole::glyphTexCoords( int font, char character ) const
	{
		ASSERT( 0 <= font && font < numFonts() );

        const auto grid = fontGridSize();
		const auto glyphSize = fontGlyphTexelSize();

		const auto fontBaseTexelPos = m_fontsBaseTexelPos + vec2i( 0, font * grid.y * glyphSize.y );

		const auto characterGridPos = vec2i( character % grid.x, character / grid.x );
		const auto characterTexelPos = fontBaseTexelPos + characterGridPos * glyphSize;

		return fr::rect( texelsToTexCoords( characterTexelPos ), texelsToTexCoords( characterTexelPos + glyphSize ));
	}

	void FantasyConsole::loadScript( const path& path )
	{
		console_trace( "Loading LUA script at " << path );

		// Load and run the code.
		//
		m_compiled = tryLua(   "loading", [&]() { return luaL_loadfile( m_lua, path.c_str() ); } )
				  && tryLua( "first run", [&]() { return lua_pcall( m_lua, 0, LUA_MULTRET, 0 ); } );
	}

	void FantasyConsole::loadSpritesheet( const path& path )
	{
		console_trace( "Loading spritesheet at " << path );

		vec2ui dimensions;

		m_spriteSheetTexels.clear();
		spriteSheet( nullptr );

		{
			// Load the raw pixel images from the spritesheet file.
			//
			ImageLoader::ImagePtr texels;

			bool hasAlpha, hasAlphaPremultiplied;
			texels = ImageLoader::loadImageRGBA( path, dimensions.x, dimensions.y, hasAlpha, hasAlphaPremultiplied, ImageLoader::WantPremultiplied );

			m_userSpriteAreaDimensions = vector_cast< int >( dimensions );

			// Edit the texels to add necessary additional elements: a "blank" sprite, fonts....
			m_spriteSheetTexels = amendSpritesheetTexels( texels.get(), dimensions );
		}

		// Create the texture object if needed.
		//
		if( !spriteSheet() )
		{
			spriteSheet( createObject< Texture >() );
		}

		// Load the texture object with the texels.
		//
		spriteSheet()->loadFromPixelData( reinterpret_cast< const unsigned char* >( m_spriteSheetTexels.data() ), dimensions, Texture::AlphaUsage::AlphaPremultiplied );
	}

	void FantasyConsole::loadAudioFolder( const path& path )
	{
		fr::each_child_file( path, [&]( const fr::path& filePath, bool regularFile )
		{
            if( regularFile && ( filePath.extension() == ".wav" || filePath.extension() == ".mp3" ))
            {
                loadAudio( path / filePath );
            }
		} );
	}

	void FantasyConsole::loadAudio( const path& path )
	{
		auto cue = createObject< AudioCue >();

		try
		{
			cue->allowAsyncLoad( false );
			cue->loadFile( path, [self = WeakPtr< FantasyConsole >( this )]( const std::string& errorMessage )
						  {
							  ASSERT( dispatch::onMainQueue() );
							  if( self )
							  {
								  self->console( errorMessage );
							  }
						  });
			cue->maxSimultaneousSounds( 2 );
		}
		catch( const std::exception& e )
		{
			console_trace( e.what() );
			return;
		}

		const auto& fileStem = path.stem();

		m_audioCues[ fileStem.string() ] = cue;

		release_trace( "Loaded cue " << path );
	}

	void FantasyConsole::reload()
	{
		if( m_gamePath.empty() == false )
		{
			loadGame( m_gamePath );
		}
	}

	void FantasyConsole::activated( bool activate )
	{
		if( activate )
		{
			if( m_musicSound )
			{
				m_musicSound->play();
			}

			if( m_virtualTrackball )
			{
				Event event( VIRTUAL_CONTROLS_SHOW, this );
				dispatchEvent( &event );
			}
		}
		else
		{
			if( m_musicSound )
			{
				m_musicSound->pause();
			}
		}
	}

	bool FantasyConsole::hasScriptFunction( const std::string& fnName ) const
	{
		return m_lua && luacpp::doesLuaFunctionExist( m_lua, fnName );
	}

	void FantasyConsole::exploreFolder() const
	{
		fr::explorePath( gamesBasePath() );
	}

	void FantasyConsole::onAddedToStage()
	{
		Super::onAddedToStage();

		m_primitivesShaderProgram = Renderer::instance().createOrGetShaderProgram( "SpriteBatch" );
		m_drawVertexStructure = Renderer::instance().createOrGetVertexStructure( "VS_SpriteBatch" );

		stage().addEventListener( EventKeyboard::KEY_DOWN, FRESH_CALLBACK( onKeyDown ));
		stage().addEventListener( EventKeyboard::KEY_UP, FRESH_CALLBACK( onKeyUp ));

		stage().addEventListener( EventTouch::TOUCH_BEGIN, FRESH_CALLBACK( onStageTouchBegin ));
		stage().addEventListener( EventTouch::TOUCH_END, FRESH_CALLBACK( onStageTouchEnd ));
		stage().addEventListener( EventTouch::TOUCH_CANCELLED, FRESH_CALLBACK( onStageTouchCancelled ));

		auto& gamepadManager = Application::instance().gamepadManager();

		gamepadManager.addEventListener( GamepadManager::GAMEPAD_ATTACHED, FRESH_CALLBACK( onGamepadAttached ));
		gamepadManager.addEventListener( GamepadManager::GAMEPAD_DETACHED, FRESH_CALLBACK( onGamepadDetached ));
	}

	void FantasyConsole::onRemovingFromStage()
	{
		stage().removeEventListener( EventKeyboard::KEY_DOWN, FRESH_CALLBACK( onKeyDown ));
		stage().removeEventListener( EventKeyboard::KEY_UP, FRESH_CALLBACK( onKeyUp ));

		stage().removeEventListener( EventTouch::TOUCH_BEGIN, FRESH_CALLBACK( onStageTouchBegin ));
		stage().removeEventListener( EventTouch::TOUCH_END, FRESH_CALLBACK( onStageTouchEnd ));
		stage().removeEventListener( EventTouch::TOUCH_CANCELLED, FRESH_CALLBACK( onStageTouchCancelled ));

		auto& gamepadManager = Application::instance().gamepadManager();

		gamepadManager.removeEventListener( GamepadManager::GAMEPAD_ATTACHED, FRESH_CALLBACK( onGamepadAttached ));
		gamepadManager.removeEventListener( GamepadManager::GAMEPAD_DETACHED, FRESH_CALLBACK( onGamepadDetached ));

		Super::onRemovingFromStage();
	}

	void FantasyConsole::update()
	{
		Super::update();

		if( compiled() )
		{
			try
			{
				callScriptFunction( "update" );
				++m_tickCount;
			}
			catch( const std::exception& e )
			{
				console_trace( "LUA error (calling `update()`): " << e.what() );
			}

			updateInput();
            updateScreenEffects();
		}
	}

    void FantasyConsole::updateScreenEffects() const
    {
        if( auto effects = firstAncestorOfType< ScreenEffects >( *this ))
        {
            effects->transitionToState( m_consoleShaderState, 0 );

            effects->resizeVirtualScreen( screenDims() );
        }
    }

    vec2 FantasyConsole::hostScreenDimensions() const
    {
        if( auto effects = firstAncestorOfType< ScreenEffects >( *this ))
        {
            return vector_cast< real >( effects->virtualScreenDimensions() );
        }
        else if( hasStage() )
        {
            return stage().stageDimensions();
        }
        else
        {
            return vector_cast< real >( m_defaultScreenSize );
        }
    }

	void FantasyConsole::render( TimeType relativeFrameTime, RenderInjector* injector )
	{
		if( !compiled() ) { return; }

		m_drawVertices.clear();
		m_screen->beginFrame();

		try
		{
			callScriptFunction( "draw" );
		}
		catch( const std::exception& e )
		{
			console_trace( "LUA error (calling `draw()`): " << e.what() );
		}

		// release_trace( "- FantasyConsole::render() calling drawPrimitives() with " << m_drawVertices.size() << " verts." );
		drawPrimitives( m_drawVertices );
		m_drawVertices.clear();

		m_screen->endFrame();

		texture( m_screen->texture() );
		texture()->filterMode( m_filterMode );

		resizeForStage();

		Super::render( relativeFrameTime, injector );
	}

	void FantasyConsole::resizeForStage()
	{
		// Scale myself up to fit the stage.
		//
		const real fitScale = getFitRatio( vector_cast< real >( m_screen->size() ), hostScreenDimensions() );
		scale( fitScale );

		// TODO adjust window aspect ratio.
	}

	void FantasyConsole::drawPrimitives( const std::vector< Vertex >& drawVertices ) const
	{
		if( drawVertices.empty() ) { return; }

		// release_trace( "FantasyConsole::drawPrimitives(...)" );

		Renderer& renderer = Renderer::instance();

		renderer.useShaderProgram( m_primitivesShaderProgram );
		renderer.setBlendMode( Renderer::getBlendModeForTextureAlphaUsage( spriteSheet()->alphaUsage() ));
		renderer.applyTexture( spriteSheet() );	// Might be null. That's okay.
		renderer.updateUniformsForCurrentShaderProgram( this );

		// release_trace( "- FantasyConsole::drawPrimitives(...) creating mesh with " << drawVertices.size() << " verts." );

		const auto mesh = createObject< SimpleMesh >();
		mesh->create( Renderer::PrimitiveType::Triangles, drawVertices.begin(), drawVertices.end(), m_drawVertexStructure );

		// If we're going to be drawing, let's make sure the filter mode is set properly.
		if( spriteSheet() )
		{
			spriteSheet()->filterMode( m_filterMode );
		}

		mesh->draw();

		// release_trace( "FantasyConsole::drawPrimitives(...) DONE" );
	}

	path FantasyConsole::gamesBasePath() const
	{
		auto path = getDocumentBasePath() / m_gamesFolder;
		create_directory( path );
		return path;
	}

	void FantasyConsole::createLua()
	{
		destroyLua();

		m_lua = luaL_newstate();

		LuaHostRegistry::addHost( m_lua, this );

		// Load default built-in libraries.
		//
		static const luaL_Reg loadedlibs[] = {
			{"_G", luaopen_base},
			//			{LUA_LOADLIBNAME, luaopen_package},
			{LUA_COLIBNAME, luaopen_coroutine},
			{LUA_TABLIBNAME, luaopen_table},
			//			{LUA_IOLIBNAME, luaopen_io},
			//			{LUA_OSLIBNAME, luaopen_os},
			{LUA_STRLIBNAME, luaopen_string},
			{LUA_MATHLIBNAME, luaopen_math},
			//			{LUA_UTF8LIBNAME, luaopen_utf8},
			{LUA_DBLIBNAME, luaopen_debug},
			{NULL, NULL}
		};

		for( const luaL_Reg* lib = loadedlibs; lib->func; ++lib )
		{
			luaL_requiref( m_lua, lib->name, lib->func, 1 );
			lua_pop( m_lua, 1 );
		}

        setupMath( m_lua );

		LuaFunctionRegisterer::registerAllFunctionsWithState( m_lua );
	}

	void FantasyConsole::destroyLua()
	{
		if( m_lua )
		{
			LuaHostRegistry::removeHost( m_lua );
			lua_close( m_lua );
			m_lua = nullptr;
		}
	}

	FantasyConsole::CoroutineId FantasyConsole::addCoroutineState( lua_State* state )
	{
		ASSERT( state );
		ASSERT( fr::find_cvalue( m_coroutineStates, state ) == m_coroutineStates.cend() );
		m_coroutineStates[ m_nextAvailableCoroutineStateId ] = state;

		luacpp::LuaHostRegistry::addHost( state, this );

		return m_nextAvailableCoroutineStateId++;
	}

	lua_State* FantasyConsole::findCoroutineState( CoroutineId id ) const
	{
		ASSERT( id > 0 );

		const auto iter = m_coroutineStates.find( id );
		ASSERT( iter != m_coroutineStates.end() );
		ASSERT( iter->second );
		return iter->second;
	}

	void FantasyConsole::removeCoroutineState( CoroutineId id )
	{
		ASSERT( id > 0 );

		const auto iter = m_coroutineStates.find( id );
		ASSERT( iter != m_coroutineStates.end() );
		ASSERT( iter->second );

		luacpp::LuaHostRegistry::removeHost( iter->second );

		m_coroutineStates.erase( iter );
	}

	bool FantasyConsole::tryLua( const std::string& context, std::function< int() >&& luaCall ) const
	{
		ASSERT( m_lua );
		int result = luaCall();
		if( result != LUA_OK )
		{
			// Failed.
			console_trace( "LUA error (" << context << "): " << lua_tostring( m_lua, -1 ) );

			// Print the LUA call stack.
			luaL_traceback( m_lua, m_lua, nullptr, 1 );
			console_trace( lua_tostring( m_lua, -1 ));
		}
		return result == LUA_OK;
	}

	vec2 FantasyConsole::transform( const vec2& p ) const
	{
		return m_cameraTranslation + p;
	}

	vec2i FantasyConsole::eventToTouchPos( const vec2& pos )
	{
		return vector_cast< int >(( pos / stage().stageDimensions() + 0.5f ) * vector_cast< real >( screenDims() ));
	}

	void FantasyConsole::vertex( const vec2& position, const vec2& texCoords, const Color& color, const Color& additiveColor )
	{
		m_drawVertices.emplace_back( transform( position ), texCoords, static_cast< const vec4& >( color ), static_cast< const vec4& >( additiveColor ), m_ditherPattern );
	}

	void FantasyConsole::quad( const fr::rect& positions, const fr::rect& texCoords, const Color& color, const Color& additiveColor )
	{
		vertex( positions.ulCorner(), texCoords.ulCorner(), color, additiveColor );
		vertex( positions.urCorner(), texCoords.urCorner(), color, additiveColor );
		vertex( positions.blCorner(), texCoords.blCorner(), color, additiveColor );

		vertex( positions.blCorner(), texCoords.blCorner(), color, additiveColor );
		vertex( positions.urCorner(), texCoords.urCorner(), color, additiveColor );
		vertex( positions.brCorner(), texCoords.brCorner(), color, additiveColor );
	}

	void FantasyConsole::buttonState( int button, int player, bool value )
	{
		m_buttonsDown.resize( std::max( static_cast< int >( m_buttonsDown.size() ), player + 1 ));
		auto& buttons = m_buttonsDown[ player ];
		buttons.resize( std::max( static_cast< int >( buttons.size() ), button + 1 ));
		buttons[ button ] = value;
	}

	void FantasyConsole::updateVirtualTrackball()
	{
		if( m_virtualTrackball )
		{
			m_virtualTrackball->update();

			vec2 trackballVel = m_virtualTrackball->velocity();
			if( trackballVel.lengthSquared() > 1.0f )
			{
				trackballVel.normalize();
			}

			setJoystickState( 0, 0, trackballVel.x );
			setJoystickState( 0, 1, trackballVel.y );

//			release_trace( trackballVel );
		}
	}

	fr::EventTouch FantasyConsole::scaleTrackballEvent( const fr::EventTouch& event ) const
	{
		real scalar = 1;
		if( hasStage() )
		{
			scalar = m_screen->size().majorAxisValue() / stage().stageDimensions().majorAxisValue();
		}
		return EventTouch( event, event.target(), event.location() * scalar, event.previousLocation() * scalar );
	}

	bool FantasyConsole::inTrackpadRegion( const vec2& touchPos ) const
	{
		return m_supportVirtualButtons > 0 ? touchPos.x > 0 : true;
	}

	void FantasyConsole::updateInput()
	{
		m_buttonsDownPrev = m_buttonsDown;
		m_keysDownPrev = m_keysDown;

		if( !m_touchDown )
		{
			m_touchPos.set( -1, -1 );
		}

		updateVirtualTrackball();
	}

	void FantasyConsole::loadMap( const path& mapSource )
	{
		// Load .tmx (Tiled editor) XML file.
		//
		XmlDocument doc;
		const XmlElement* const rootElement = loadXmlDocument( mapSource.c_str(), doc );

		if( !rootElement )
		{
			console_trace( "Unable to open map source file at '" << mapSource << "'." );
		}
		if( rootElement->ValueStr() != "map" )
		{
			console_trace( "Map source file lacked <map> root element." );
		}

		size_t layer = 0;
		for( auto layerElement = rootElement->FirstChildElement( "layer" );
			 layerElement;
			 layerElement = layerElement->NextSiblingElement( "layer" ))
		{
			vec2i mapSize;
			layerElement->Attribute( "width", &mapSize.x );
			layerElement->Attribute( "height", &mapSize.y );

			const auto dataElement = layerElement->FirstChildElement( "data" );
			if( dataElement )
			{
				const auto encoding = dataElement->Attribute( "encoding" );
				if( encoding && std::string( encoding ) == "csv" )
				{
					const auto dataText =  dataElement->GetText();
					if( dataText )
					{
						std::istringstream dataStream( dataText );
						loadMapValues( mapSize, dataStream, layer++ );
						break;
					}
				}
			}
		}

		if( layer == 0 )
		{
			console_trace( "map.tmx found no layer data to load." );
		}
	}

	vec2i FantasyConsole::maxMapLayerSize() const
	{
		return std::accumulate( m_mapLayerSizes.begin(), m_mapLayerSizes.end(), vec2i{}, []( const auto& a, const auto& b ) {
			return vec2i{ std::max( a.x, b.x ), std::max( a.y, b.y ) };
		});
	}

	void FantasyConsole::loadMapValues( const vec2i& size, std::istream& spriteIndexText, size_t layer )
	{
		m_mapLayerSizes.resize( std::max( m_mapLayerSizes.size(), layer + 1 ));
		m_mapSpriteLayers.resize( std::max( m_mapSpriteLayers.size(), layer + 1 ));

		auto& mapSize = m_mapLayerSizes[ layer ];
		auto& mapSprites = m_mapSpriteLayers[ layer ];

		mapSize = size;
		mapSprites.clear();
		mapSprites.resize( mapSize.x * mapSize.y, 0 );

		for( auto& tile : mapSprites )
		{
			spriteIndexText >> std::ws >> tile >> std::ws;
            --tile;     // The "Tiled" editor stores each sprite as 1-indexed (such that the "blank" or default tile is 1). We reckon sprites to be 0-indexed.

			const char separator = spriteIndexText.get();

			if( separator == std::char_traits< char >::eof() )
			{
				break;
			}

			if( separator != ',' )
			{
				console_trace( "Loading map, expected separator ',', but got '" << separator << "'. Ignoring." );
			}
		}
	}

    vec2i FantasyConsole::screenDims() const
	{
		return m_screen->size();
	}

	LUA_FUNCTION( sprite_size, 1 );
	void FantasyConsole::sprite_size( int wid, int hgt )
	{
		SANITIZE( wid, 8, 1, 4096 );
		SANITIZE( hgt, wid, 1, 4096 );
		m_baseSpriteSizeInTexels = vec2i( wid, hgt );
	}

    LUA_FUNCTION( text_scale, 0 );
    void FantasyConsole::text_scale( real scale )
    {
        DEFAULT( scale, 1.0f );
        m_globalTextScale = scale;
    }

	LUA_FUNCTION( text_line_hgt, 0 );
	int FantasyConsole::text_line_hgt( real scale )
	{
		DEFAULT( scale, 1.0f );
		return ( fontGlyphTexelSize().y + linePadding()) * scale * m_globalTextScale;
	}

	LUA_FUNCTION( time, 0 );
	double FantasyConsole::time()
	{
		return m_tickCount * SECONDS_PER_FRAME;
	}

    LUA_FUNCTION( _debug_trace, 0 );
    void FantasyConsole::_debug_trace( std::string message )
    {
        release_trace( "FROM CONSOLE: " << message );
    }

    LUA_FUNCTION( support_virtual_trackball, 0 );
	void FantasyConsole::support_virtual_trackball( bool support )
	{
		DEFAULT( support, true );

		if( !Application::instance().isMultitouch() ) return;

		if( support )
		{
			if( !m_virtualTrackball )
			{
				m_virtualTrackball = createObject< VirtualTrackball >();

				Event event( VIRTUAL_CONTROLS_SHOW, this );
				dispatchEvent( &event );
			}
		}
		else
		{
			if( m_virtualTrackball )
			{
				m_virtualTrackball = nullptr;
				Event event( VIRTUAL_CONTROLS_HIDE, this );
				dispatchEvent( &event );
			}
		}
	}

    LUA_FUNCTION( support_virtual_buttons, 0 );
	void FantasyConsole::support_virtual_buttons( int count )
	{
		SANITIZE( count, 0, 0, 2 );

		m_supportVirtualButtons = count;
	}

    LUA_FUNCTION( show_controls_guides, 0 );
	void FantasyConsole::show_controls_guides( bool show )
	{
		if( show )
		{
			Event event( VIRTUAL_CONTROLS_SHOW, this );
			dispatchEvent( &event );
		}
		else
		{
			Event event( VIRTUAL_CONTROLS_HIDE, this );
			dispatchEvent( &event );
		}
	}

    LUA_FUNCTION( rect_collision_adjustment, 8 );
    std::tuple< bool, real, real, int, real > FantasyConsole::rect_collision_adjustment( real leftA, real topA, real rightA, real botA, real leftB, real topB, real rightB, real botB, real relVelX, real relVelY )
    {
        DEFAULT( relVelX, 0 )
        DEFAULT( relVelY, 1 )

        std::tuple< bool, real, real, int, real > result{};

        fr::rect rectA( leftA, topA, rightA, botA );
        fr::rect rectB( leftB, topB, rightB, botB );

        vec2 hitNormal;
        int hitAxis;
        real adjustmentDistance;
        bool colliding = findCollisionNormal( rectA.midpoint(), rectA.dimensions(), vec2( relVelX, relVelY ),
											  rectB.midpoint(), rectB.dimensions(), vec2{}, hitNormal, hitAxis, adjustmentDistance );
        if( colliding )
        {
            std::get< 0 >( result ) = colliding;
            std::get< 1 >( result ) = hitNormal.x;
            std::get< 2 >( result ) = hitNormal.y;
            std::get< 3 >( result ) = hitAxis;
            std::get< 4 >( result ) = adjustmentDistance;
        }

        return result;
    }

	LUA_FUNCTION( execute, 1 );
	void FantasyConsole::execute( std::string command )
	{
		ConsoleEvent event( COMMAND, this );
		event.message( command );
		dispatchEvent( &event );
	}

	LUA_FUNCTION( cocreate, 0 );
	FantasyConsole::CoroutineId FantasyConsole::cocreate()
	{
		luaL_checktype( m_lua, 1, LUA_TFUNCTION );				// Verify there's a function on the top of the argument stack.
		lua_State* coroutineState = lua_newthread( m_lua );

		const FantasyConsole::CoroutineId stateId = addCoroutineState( coroutineState );

		lua_pushvalue( m_lua, 1 );
		lua_xmove( m_lua, coroutineState, 1 );	// Move the function at the top of the main lua stack to the coroutine's stack.

		return stateId;
	}

	LUA_FUNCTION( costatus, 1 );
	std::string FantasyConsole::costatus( CoroutineId id )
	{
		lua_State* const coroutineState = findCoroutineState( id );
		if( m_lua == coroutineState)
		{
			return "running";
		}
		else
		{
			switch( lua_status( coroutineState ))
			{
				case LUA_YIELD:
					return "suspended";
				case LUA_OK:
				{
					lua_Debug ar;
					if( lua_getstack( coroutineState, 0, &ar ) > 0)  /* does it have frames? */
						return "normal";  /* it is running */
					else if( lua_gettop( coroutineState ) == 0 )
						return "dead";
					else
						return "suspended";  /* initial state */
				}
				default:  /* some error occurred */
					return "dead";
			}
		}

		return luacpp::LuaReturnType< std::string >::fetchResult( m_lua );
	}

	LUA_FUNCTION( coresume, 1 );
	void FantasyConsole::coresume( CoroutineId id )
	{
		lua_State* const coroutineState = findCoroutineState( id );
		lua_resume( coroutineState, m_lua, 0 );
	}

	LUA_FUNCTION( yield, 0 );
	void FantasyConsole::yield()
	{
		lua_yield( m_lua, lua_gettop( m_lua ));
	}


	int FantasyConsole::getLuaType( const std::string& path ) const
	{
		return luacpp::luaType( m_lua, path );
	}

	bool FantasyConsole::getValueBool( const std::string& path ) const
	{
		return luacpp::getValue< bool >( m_lua, path );
	}

	int FantasyConsole::getValueInt( const std::string& path ) const
	{
		return luacpp::getValue< int >( m_lua, path );
	}

	real FantasyConsole::getValueNumber( const std::string& path ) const
	{
		return luacpp::getValue< float >( m_lua, path );
	}

	std::string FantasyConsole::getValueString( const std::string& path ) const
	{
		return luacpp::getValue< std::string >( m_lua, path );
	}

	void FantasyConsole::setValueBool( const std::string& path, bool value )
	{
		luacpp::setValue( m_lua, path, value );
	}

	void FantasyConsole::setValueInt( const std::string& path, int value )
	{
		luacpp::setValue( m_lua, path, value );
	}

	void FantasyConsole::setValueNumber( const std::string& path, real value )
	{
		luacpp::setValue( m_lua, path, value );
	}

	void FantasyConsole::setValueString( const std::string& path, const std::string& value )
	{
		luacpp::setValue( m_lua, path, value );
	}

	int FantasyConsole::buttonForTouchLocation( const vec2& location ) const
	{
		ASSERT( m_supportVirtualButtons <= 2 );

		if( m_supportVirtualButtons <= 0 || inTrackpadRegion( location ))
		{
			return -1;
		}

		const real yProportion = clamp( location.y / ( stage().stageDimensions().y * 0.5f ) + 0.5f, 0.0f, 0.99f );
		return 4 + std::floor( yProportion * m_supportVirtualButtons );
	}

	void FantasyConsole::onTouchBegin( const EventTouch& event )
	{
		Super::onTouchBegin( event );

		if( event.iThisTouch() == 0 )
		{
			m_touchDown = true;
			m_touchPos = eventToTouchPos( event.location() );
		}
	}

	void FantasyConsole::onTouchMove( const EventTouch& event )
	{
		Super::onTouchMove( event );

		if( event.iThisTouch() == 0 )
		{
			m_touchDown = true;
			m_touchPos = eventToTouchPos( event.location() );
		}
	}

	void FantasyConsole::onTouchEnd( const EventTouch& event )
	{
		Super::onTouchEnd( event );

		if( event.iThisTouch() == 0 )
		{
			m_touchDown = false;
			m_touchPos = eventToTouchPos( event.location() );
		}
	}

	void FantasyConsole::onTouchCancelled( const EventTouch& event )
	{
		onTouchEnd( event );
		Super::onTouchCancelled( event );
	}

	void FantasyConsole::onWheelMove( const EventTouch& event )
	{
		Super::onWheelMove( event );
		// TODO
	}

	void FantasyConsole::onDragBegin( const EventTouch& event )
	{
		if( m_virtualTrackball && inTrackpadRegion( event.location() ))
		{
			m_virtualTrackball->onDragBegin( scaleTrackballEvent( event ));
		}
	}

	void FantasyConsole::onDragMove( const EventTouch& event )
	{
		if( m_virtualTrackball )
		{
			Event eventToDispatch( VIRTUAL_TRACKBALL_USED, this );
			dispatchEvent( &eventToDispatch );

			m_virtualTrackball->onDragMove( scaleTrackballEvent( event ));
		}
	}

	void FantasyConsole::onDragEnd( const EventTouch& event )
	{
		if( m_virtualTrackball )
		{
			m_virtualTrackball->onDragEnd( scaleTrackballEvent( event ));
		}
	}


	void FantasyConsole::keyState( Keyboard::Key key, int charCode, bool value )
	{
		static const std::unordered_map< Keyboard::Key, int > keyToCharCode =
		{
			{ Keyboard::Key::Home, 2 },
			{ Keyboard::Key::End, 3 },
			{ Keyboard::Key::Backspace, 8 },
			{ Keyboard::Key::PageUp, 11 },
			{ Keyboard::Key::PageDown, 12 },
			{ Keyboard::Key::Shift, 14 },
			{ Keyboard::Key::LeftArrow, 17 },
			{ Keyboard::Key::RightArrow, 18 },
			{ Keyboard::Key::UpArrow, 19 },
			{ Keyboard::Key::DownArrow, 20 },
			{ Keyboard::Key::CtrlCommand, 26 },
			{ Keyboard::Key::Delete, 127 },
		};

		// Ignore Escape.
		if( key == Keyboard::Key::Escape )
		{
			return;
		}

		const auto iter = keyToCharCode.find( key );
		if( iter != keyToCharCode.end() )
		{
			charCode = iter->second;
		}

		m_keysDown.resize( std::max( static_cast< int >( m_keysDown.size() ), charCode + 1 ));
		m_keysDown[ charCode ] = value;

		// If a new "down" message, consider it just now "up".
		if( value )
		{
			m_keysDownPrev.resize( std::max( static_cast< int >( m_keysDownPrev.size() ), charCode + 1 ));
			m_keysDownPrev[ charCode ] = false;
		}
	}

	void FantasyConsole::onKeyChange( const EventKeyboard& event, bool down )
	{
		keyState( event.key(), static_cast< int >( event.charCode() ), down );

		switch( event.key() )
		{
			case Keyboard::LeftArrow:
				buttonState( 0, 0, down );
				break;
			case Keyboard::RightArrow:
				buttonState( 1, 0, down );
				break;
			case Keyboard::UpArrow:
				buttonState( 2, 0, down );
				break;
			case Keyboard::DownArrow:
				buttonState( 3, 0, down );
				break;
			case Keyboard::Z:
			case Keyboard::C:
			case Keyboard::N:
				buttonState( 4, 0, down );
				break;
			case Keyboard::X:
			case Keyboard::V:
			case Keyboard::M:
				buttonState( 5, 0, down );
				break;

			case Keyboard::S:
				buttonState( 0, 1, down );
				break;
			case Keyboard::F:
				buttonState( 1, 1, down );
				break;
			case Keyboard::E:
				buttonState( 2, 1, down );
				break;
			case Keyboard::D:
				buttonState( 3, 1, down );
				break;
			case Keyboard::LeftShift:
			case Keyboard::Tab:
				buttonState( 4, 1, down );
				break;
			case Keyboard::A:
			case Keyboard::Q:
				buttonState( 5, 1, down );
				break;

			default:
				break;
		}
	}

	FRESH_DEFINE_CALLBACK( FantasyConsole, onKeyDown, EventKeyboard )
	{
		onKeyChange( event, true );
	}

	FRESH_DEFINE_CALLBACK( FantasyConsole, onKeyUp, EventKeyboard )
	{
		onKeyChange( event, false );
	}

	FRESH_DEFINE_CALLBACK( FantasyConsole, onGamepadAttached, EventGamepad )
	{
		const auto gamepad = event.gamepadTarget();
		gamepad->addEventListener( Gamepad::BUTTON_DOWN, FRESH_CALLBACK( onButtonDown ));
		gamepad->addEventListener( Gamepad::BUTTON_UP, FRESH_CALLBACK( onButtonUp ));
		gamepad->addEventListener( Gamepad::AXIS_MOVED, FRESH_CALLBACK( onAxisMoved ));
	}

	FRESH_DEFINE_CALLBACK( FantasyConsole, onGamepadDetached, EventGamepad )
	{
		const auto gamepad = event.gamepadTarget();
		gamepad->removeEventListener( Gamepad::BUTTON_DOWN, FRESH_CALLBACK( onButtonDown ));
		gamepad->removeEventListener( Gamepad::BUTTON_UP, FRESH_CALLBACK( onButtonUp ));
		gamepad->removeEventListener( Gamepad::AXIS_MOVED, FRESH_CALLBACK( onAxisMoved ));
	}

	void FantasyConsole::onButtonChange( const EventGamepadButton& event, bool down )
	{
		const auto& gamepadManager = Application::instance().gamepadManager();

		const int player = static_cast< int >( gamepadManager.gamepadIndex( event.gamepadTarget() ));

		switch( event.button() )
		{
			case Gamepad::Button::DPadLeft:
				buttonState( 0, player, down );
				break;
			case Gamepad::Button::DPadRight:
				buttonState( 1, player, down );
				break;
			case Gamepad::Button::DPadUp:
				buttonState( 2, player, down );
				break;
			case Gamepad::Button::DPadDown:
				buttonState( 3, player, down );
				break;
			case Gamepad::Button::A:
				buttonState( 4, player, down );
				break;
			case Gamepad::Button::B:
				buttonState( 5, player, down );
				break;

			default:
				break;
		}
	}

	FRESH_DEFINE_CALLBACK( FantasyConsole, onButtonDown, EventGamepadButton )
	{
		onButtonChange( event, true );
	}

	FRESH_DEFINE_CALLBACK( FantasyConsole, onButtonUp, EventGamepadButton )
	{
		onButtonChange( event, false );
	}

	FRESH_DEFINE_CALLBACK( FantasyConsole, onAxisMoved, EventGamepadAxis )
	{
        const size_t axisIndex = static_cast< size_t >( event.axis() );
        
        const size_t player = 0;        // TODO: Determine player from event.gamepadTarget();
		setJoystickState( player, axisIndex, event.newValue() );
	}


	FRESH_DEFINE_CALLBACK( FantasyConsole, onStageTouchBegin, EventTouch )
	{
		const auto button = buttonForTouchLocation( event.location() );
		if( button >= 0 )
		{
			m_touchForVirtualButton[ event.touchId() ] = button;
			buttonState( button, 0, true );
		}
	}

	FRESH_DEFINE_CALLBACK( FantasyConsole, onStageTouchEnd, EventTouch )
	{
		// Update console buttons based on touch movements.
		//
		const auto iter = m_touchForVirtualButton.find( event.touchId() );
		if( iter != m_touchForVirtualButton.end() )
		{
			buttonState( iter->second, 0, false );
			m_touchForVirtualButton.erase( iter );
		}
	}

	FRESH_DEFINE_CALLBACK( FantasyConsole, onStageTouchCancelled, EventTouch )
	{
		onStageTouchEnd( event );
	}
}

