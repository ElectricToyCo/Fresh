//
//  FantasyConsole.h
//  Fresh
//
//  Created by Jeff Wofford on 6/22/18.
//  Copyright (c) 2018 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_FantasyConsole_h
#define Fresh_FantasyConsole_h

#include "Sprite.h"
#include "FreshOptional.h"
#include "ScreenEffects.h"
#include "Gamepad.h"
#include "EventKeyboard.h"
#include "FreshVector.h"
#include "FreshTileGrid.h"
#include "FantasyConsoleScreen.h"
#include "lua/lua.hpp"
#include "FreshException.h"

namespace fr
{
	class AudioCue;
	class Sound;
	class VirtualTrackball;

	class FantasyConsole : public Sprite
	{
		FRESH_DECLARE_CLASS( FantasyConsole, Sprite );
	public:

		static const char* COMMAND;
		static const char* MESSAGE;
		static const char* VIRTUAL_CONTROLS_SHOW;
		static const char* VIRTUAL_CONTROLS_HIDE;
		static const char* VIRTUAL_TRACKBALL_USED;

		class ConsoleEvent : public Event
		{
		public:
			using Event::Event;

			SYNTHESIZE( std::string, message )

		private:

			std::string m_message;
		};

		virtual ~FantasyConsole();

		SYNTHESIZE_GET( bool, compiled );
		SYNTHESIZE_GET( path, gamePath );
		SYNTHESIZE_GET( vec2i, baseSpriteSizeInTexels );

		std::string saveMemento() const;
		void loadMemento( const std::string& memento );

        path gamesBasePath() const;

		void clear();
		void loadGame( const path& gameFolderPath );
		void reload();

		void activated( bool activate );

		void console( const std::string& message ) const;
		void console( const std::string& message );

		void exploreFolder() const;

		bool hasScriptFunction( const std::string& fnName ) const;

		template< typename ReturnT = void, typename... Args >
		ReturnT callScriptFunction( const std::string& fnName, Args... args ) const;

		int numFonts() const;

		SYNTHESIZE( real, linePadding )

		virtual void onAddedToStage() override;
		virtual void onRemovingFromStage() override;
		virtual void update() override;
		virtual void render( TimeType relativeFrameTime, RenderInjector* injector = nullptr ) override;
		virtual void onTouchBegin( const EventTouch& event ) override;
		virtual void onTouchMove( const EventTouch& event ) override;
		virtual void onTouchEnd( const EventTouch& event ) override;
		virtual void onTouchCancelled( const EventTouch& event ) override;
		virtual void onWheelMove( const EventTouch& event ) override;

		virtual void onDragBegin( const EventTouch& event ) override;
		virtual void onDragMove( const EventTouch& event ) override;
		virtual void onDragEnd( const EventTouch& event ) override;

		vec2i screenDims() const;

		void setDitherPattern( uint bits );

		bool supportsVirtualControls() const;

		// Fundamental API
		//
        void text_scale( real scale );
		int text_line_hgt( real scale );
		void sprite_size( int wid, int hgt );
		double time();
        void _debug_trace( std::string message );       // Sends a message to the Xcode/VS/... output pane.
        std::tuple< bool, real, real, int, real > rect_collision_adjustment( real leftA, real topA, real rightA, real botA, real leftB, real topB, real rightB, real botB, real relVelX, real relVelY );
		void support_virtual_trackball( bool support );
		void support_virtual_buttons( int count );
		void show_controls_guides( bool show );

		using CoroutineId = int;
		CoroutineId cocreate();
		std::string costatus( CoroutineId id );
		void coresume( CoroutineId id );
		void yield();


		// Editor/console API
		//
		void execute( std::string command );

		int getLuaType( const std::string& path ) const;

		bool getValueBool( const std::string& path ) const;
		int getValueInt( const std::string& path ) const;
		real getValueNumber( const std::string& path ) const;
		std::string getValueString( const std::string& path ) const;

		void setValueBool( const std::string& path, bool value );
		void setValueInt( const std::string& path, int value );
		void setValueNumber( const std::string& path, real value );
		void setValueString( const std::string& path, const std::string& value );

		// "Injected" API declaration headers.
		//
#       include "ApiScreen.injected.h"
#		include "ApiRendering.injected.h"
#		include "ApiInput.injected.h"
#		include "ApiAudio.injected.h"
#		include "ApiMap.injected.h"
#		include "ApiTable.injected.h"
#       include "ApiMath.injected.h"
#       include "ApiNav.injected.h"

	protected:

		int buttonForTouchLocation( const vec2& location ) const;

		vec2 transform( const vec2& p ) const;
		vec2i eventToTouchPos( const vec2& pos );

		void resizeForStage();

		std::vector< uint > amendSpritesheetTexels( const unsigned char* texelBytesRGBA, vec2ui& inOutDimensions );

		vec2i textureDimensions() const;
		vec2i userSpriteAreaDimensions() const;
		vec2i spritesDimensions() const;
		int maxSpriteIndex() const;

		SYNTHESIZE( Texture::ptr, spriteSheet );
		vec2i spriteIndexToSpritePos( int spriteIndex ) const;
		vec2i spriteSheetPosToTexel( const vec2i& spritePos ) const;
		vec2 texelsToTexCoords( const vec2i& texels ) const;

		fr::rect blankTexCoords() const;

		vec2i fontGridSize() const;
		vec2i fontGlyphTexelSize() const;
		fr::rect glyphTexCoords( int font, char character ) const;

		void loadScript( const path& path );
		void loadSpritesheet( const path& path );
		void loadAudioFolder( const path& path );
		void loadAudio( const path& path );

		void createLua();
		void destroyLua();

		CoroutineId addCoroutineState( lua_State* state );
		lua_State* findCoroutineState( CoroutineId id ) const;
		void removeCoroutineState( CoroutineId id );

		bool tryLua( const std::string& context, std::function< int() >&& luaCall ) const;

		void vertex( const vec2& position, const vec2& texCoords, const Color& color, const Color& additiveColor = 0 );
		void quad( const fr::rect& positions, const fr::rect& texCoords, const Color& color = Color::White, const Color& additiveColor = 0 );

		void keyState( Keyboard::Key key, int charCode, bool value );
		void buttonState( int button, int player, bool value );
		void updateInput();

		vec2i maxMapLayerSize() const;

		void loadMap( const path& mapSource );
		void loadMapValues( const vec2i& size, std::istream& spriteIndexText, size_t layer = 0 );

	private:

		DVAR( path, m_gamesFolder, "games" );
		VAR( path, m_gamePath );

		DVAR( vec2i, m_baseSpriteSizeInTexels, vec2i{ 16 } );
		DVAR( Texture::FilterMode, m_filterMode, Texture::FilterMode::Nearest );
		DVAR( real, m_linePadding, 1.0f );

		DVAR( vec2i, m_defaultScreenSize, vec2i( 240, 136 ));

        VAR( ScreenEffects::ShaderState, m_consoleShaderState );

        DVAR( real, m_globalTextScale, 1.0f );

		vec2 m_cameraTranslation;

		float m_stageAspectRatioForLastSpecifiedScreenSize = 0;
		vec2i m_lastSpecifiedScreenSize;

		size_t m_tickCount = 0;

		FantasyConsoleScreen::ptr m_screen;
        
        FreshTileGrid::ptr m_tileGrid;
        std::unordered_map< int, TileTemplate::ptr > m_spriteTileTemplates;
        
		vec2i m_userSpriteAreaDimensions;	// Size in texels of the sprite sheet provided by the user (sprites.png).
		vec2ui m_fontTexelDimensions;		// Size in texels of the area within the sprite sheet where fonts live.
		vec2i m_fontsBaseTexelPos;			// Texel position of the UL corner of the topmost font in the spritesheet.

		int m_supportVirtualButtons = 2;
		std::unordered_map< EventTouch::TouchId, int > m_touchForVirtualButton;
		SmartPtr< VirtualTrackball > m_virtualTrackball;
		void updateVirtualTrackball();
		fr::EventTouch scaleTrackballEvent( const fr::EventTouch& event ) const;

		bool inTrackpadRegion( const vec2& touchPos ) const;

		SmartPtr< Sound > m_musicSound;

		Texture::ptr m_spriteSheet;
		std::vector< uint > m_spriteSheetTexels;
		std::vector< unsigned char > m_spriteFlags;

		std::vector< vec2i > m_mapLayerSizes;
		std::vector< std::vector< uint >> m_mapSpriteLayers;

		mat4 m_ditherPattern;

		struct Vertex
		{
			vec2 position;
			vec2 texCoord;
			vec4 color;
			vec4 additiveColor;
			mat4 ditherPattern;

			Vertex( const vec2& p, const vec2& t, const vec4& c, const vec4& a, const mat4& d )
			:	position( p ), texCoord( t ), color( c ), additiveColor( a ), ditherPattern( d )
			{}
		};

		std::vector< Vertex > m_drawVertices;
		VertexStructure::ptr m_drawVertexStructure;
		ShaderProgram::ptr m_primitivesShaderProgram;

		lua_State* m_lua = nullptr;
		bool m_compiled = false;

		CoroutineId m_nextAvailableCoroutineStateId = 1;
		std::unordered_map< size_t, lua_State* > m_coroutineStates;

		std::unordered_map< std::string, SmartPtr< AudioCue >> m_audioCues;

		void drawPrimitives( const std::vector< Vertex >& drawVertices ) const;

		void onKeyChange( const EventKeyboard& event, bool down );
		void onButtonChange( const EventGamepadButton& event, bool down );

        void updateScreenEffects() const;

        vec2 hostScreenDimensions() const;

		FRESH_DECLARE_CALLBACK( FantasyConsole, onKeyDown, EventKeyboard );
		FRESH_DECLARE_CALLBACK( FantasyConsole, onKeyUp, EventKeyboard );
		FRESH_DECLARE_CALLBACK( FantasyConsole, onGamepadAttached, EventGamepad );
		FRESH_DECLARE_CALLBACK( FantasyConsole, onGamepadDetached, EventGamepad );
		FRESH_DECLARE_CALLBACK( FantasyConsole, onButtonDown, EventGamepadButton );
		FRESH_DECLARE_CALLBACK( FantasyConsole, onButtonUp, EventGamepadButton );
		FRESH_DECLARE_CALLBACK( FantasyConsole, onAxisMoved, EventGamepadAxis );
		FRESH_DECLARE_CALLBACK( FantasyConsole, onStageTouchBegin, EventTouch );
		FRESH_DECLARE_CALLBACK( FantasyConsole, onStageTouchEnd, EventTouch );
		FRESH_DECLARE_CALLBACK( FantasyConsole, onStageTouchCancelled, EventTouch );
		};
}

// Usable only within member functions of FantasyConsole.
#define console_trace( msg ) console( createString( msg ));

// Template implementation header.
#include "FantasyConsole.inl.h"

#endif
