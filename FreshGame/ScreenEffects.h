//
//  ScreenEffects.h
//  Fresh
//
//  Created by Jeff Wofford on 11/20/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_ScreenEffects_h
#define Fresh_ScreenEffects_h

#include "DisplayObjectWithMesh.h"

namespace fr
{
	class ScreenEffects : public fr::DisplayObjectWithMesh
	{
		FRESH_DECLARE_CLASS( ScreenEffects, DisplayObjectWithMesh );
	public:
		
		struct ShaderState : public fr::SerializableStruct< ShaderState >
		{
			float chromaticAberration = 0.0f;
			float bloomBrightness = 0.0;
			float bloomContrast = 1.0f;
			float bloomIntensity = 0.0f;
			float bevelIntensity = 0.0f;
			float noiseIntensity = 0.0f;
			float burnIn = 0.0f;
			float saturation = 1.0f;
			vec4 colorMultiplied = vec4( 1 );
			vec4 rescanColor = vec4( 0, 0, 0, 1.0f );
            real barrelDistortion = 0.0f;
			int bevelType = 0;

			ShaderState()
			{
				STRUCT_BEGIN_PROPERTIES
				STRUCT_ADD_PROPERTY( chromaticAberration )
				STRUCT_ADD_PROPERTY( bloomBrightness )
				STRUCT_ADD_PROPERTY( bloomContrast )
				STRUCT_ADD_PROPERTY( bloomIntensity )
				STRUCT_ADD_PROPERTY( bevelIntensity )
				STRUCT_ADD_PROPERTY( noiseIntensity )
				STRUCT_ADD_PROPERTY( burnIn )
				STRUCT_ADD_PROPERTY( saturation )
				STRUCT_ADD_PROPERTY( colorMultiplied )
				STRUCT_ADD_PROPERTY( rescanColor )
                STRUCT_ADD_PROPERTY( barrelDistortion )
                STRUCT_ADD_PROPERTY( bevelType )
				STRUCT_END_PROPERTIES
			}
			
			bool operator==( const ShaderState& other ) const;
			
			ShaderState operator+( const ShaderState& other ) const;
			ShaderState operator-( const ShaderState& other ) const;
			ShaderState operator*( const ShaderState& other ) const;
			ShaderState operator/( const ShaderState& other ) const;
			ShaderState& operator+=( const ShaderState& other );
			ShaderState& operator-=( const ShaderState& other );
			ShaderState& operator*=( const ShaderState& other );
			ShaderState& operator/=( const ShaderState& other );
			ShaderState operator+( double t ) const;
			ShaderState operator-( double t ) const;
			ShaderState operator*( double t ) const;
			ShaderState operator/( double t ) const;
			ShaderState& operator+=( double t );
			ShaderState& operator-=( double t );
			ShaderState& operator*=( double t );
			ShaderState& operator/=( double t );
		};
		
		STRUCT_DECLARE_SERIALIZATION_OPERATORS( ShaderState )

		
		virtual void draw( TimeType relativeFrameTime, RenderInjector* injector ) override;
		
		void clearColor( Color color );
		
		enum class Quality
		{
			Low,
			Medium,
			High
		};
		
		SYNTHESIZE( Quality, shaderQuality )

		const int& shaderMilliseconds() const 			{ return m_shaderMilliseconds; }
		const float& chromaticAberration() const 		{ return m_currentShaderState.chromaticAberration; }
		const float& bloomBrightness() const 			{ return m_currentShaderState.bloomBrightness; }
		const float& bloomContrast() const 				{ return m_currentShaderState.bloomContrast; }
		const float& bloomIntensity() const 			{ return m_currentShaderState.bloomIntensity; }
		const float& bevelIntensity() const 			{ return m_currentShaderState.bevelIntensity; }
		const float& noiseIntensity() const 			{ return m_currentShaderState.noiseIntensity; }
		const float& burnIn() const 					{ return m_currentShaderState.burnIn; }
		const float& saturation() const 				{ return m_currentShaderState.saturation; }
		const vec4& colorMultiplied() const 			{ return m_currentShaderState.colorMultiplied; }
		const vec4& rescanColor() const 				{ return m_currentShaderState.rescanColor; }
        const real& barrelDistortion() const             { return m_currentShaderState.barrelDistortion; }

		const ShaderState& currentShaderState() const	{ return m_currentShaderState; }

		void transitionToState( const ShaderState& state, TimeType transitionTime );
		void transitionToState( const std::string& stateName, TimeType transitionTime );
		
		virtual void onAddedToStage() override;
		
        void resizeVirtualScreen( const Vector2i& size );
		Vector2i virtualScreenDimensions() const;
		real virtualScreenAspectRatio() const;
		
		virtual void update() override;
		virtual void postLoad() override;
		
	protected:

		virtual void drawChildren( TimeType relativeFrameTime, RenderInjector* injector ) override;
		virtual void setupSimulatedScreen();
		
	private:
		
		VAR( DisplayObjectContainer::ptr, m_inner );
		VAR( fr::ShaderProgram::ptr, m_shaderProgramPrimaryPost );
		VAR( fr::ShaderProgram::ptr, m_shaderProgramCopy );
		VAR( fr::ShaderProgram::ptr, m_shaderProgramBloomTonemap );
		VAR( fr::ShaderProgram::ptr, m_shaderProgramHorizontalBlur );
		VAR( fr::ShaderProgram::ptr, m_shaderProgramVerticalBlur );
		VAR( fr::ShaderProgram::ptr, m_shaderProgramComposite );
		VAR( fr::Texture::ptr, m_noiseTexture );
		VAR( std::vector< fr::Texture::ptr >, m_pixelBevelTextures );

		VAR( ShaderState, m_currentShaderState );
		
		using ShaderStates = std::map< std::string, ShaderState >;
		
		VAR( ShaderStates, m_shaderStates )
		VAR( std::string, m_currentStateName );
		
		DVAR( int, m_shaderMilliseconds, 0 );
		
		ShaderState m_transitionStartState;
		ShaderState m_desiredShaderState;
		TimeType m_transitionDuration = 0;
		TimeType m_transitionStartTime = -1;
		fr::TweenerQuadEaseInOut< ShaderState > m_tweener;
		
		real m_lastSetupBarrelDistortion = 0;
		
		DVAR( int, m_textureUnit1, 1 );
		DVAR( int, m_textureUnit2, 2 );
		DVAR( vec2, m_diffuseTextureSize, vec2( 320.0f, 192.0f ));
		
		DVAR( Quality, m_shaderQuality, Quality::High );

		static const size_t NUM_RENDER_TARGETS = 6;
		fr::RenderTarget::ptr m_renderTarget[ NUM_RENDER_TARGETS ];
		fr::VertexBuffer::ptr m_vertexBufferQuad;
		fr::Texture::ptr m_priorFrameTexture;
		
		std::unordered_map< std::string, int > m_uniformIDs;

		DECLARE_ACCESSOR( ScreenEffects, const float&, chromaticAberration );
		DECLARE_ACCESSOR( ScreenEffects, const float&, bloomBrightness );
		DECLARE_ACCESSOR( ScreenEffects, const float&, bloomContrast );
		DECLARE_ACCESSOR( ScreenEffects, const float&, bloomIntensity );
		DECLARE_ACCESSOR( ScreenEffects, const float&, bevelIntensity );
		DECLARE_ACCESSOR( ScreenEffects, const float&, noiseIntensity );
		DECLARE_ACCESSOR( ScreenEffects, const float&, burnIn );
		DECLARE_ACCESSOR( ScreenEffects, const float&, saturation );
		DECLARE_ACCESSOR( ScreenEffects, const vec4&, colorMultiplied );
		DECLARE_ACCESSOR( ScreenEffects, const vec4&, rescanColor );
        DECLARE_ACCESSOR( ScreenEffects, const real&, barrelDistortion );
	};
	
	FRESH_ENUM_STREAM_IN_BEGIN( ScreenEffects, Quality )
	FRESH_ENUM_STREAM_IN_CASE( ScreenEffects::Quality, Low )
	FRESH_ENUM_STREAM_IN_CASE( ScreenEffects::Quality, Medium )
	FRESH_ENUM_STREAM_IN_CASE( ScreenEffects::Quality, High )
	FRESH_ENUM_STREAM_IN_END()
	
	FRESH_ENUM_STREAM_OUT_BEGIN( ScreenEffects, Quality )
	FRESH_ENUM_STREAM_OUT_CASE( ScreenEffects::Quality, Low )
	FRESH_ENUM_STREAM_OUT_CASE( ScreenEffects::Quality, Medium )
	FRESH_ENUM_STREAM_OUT_CASE( ScreenEffects::Quality, High )
	FRESH_ENUM_STREAM_OUT_END()
}

#endif
