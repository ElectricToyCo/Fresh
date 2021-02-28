//
//  PostEffect.h
//  Fresh
//
//  Created by Jeff Wofford on 6/10/15.
//  Copyright (c) 2015 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_PostEffect_h
#define Fresh_PostEffect_h

#include "Sprite.h"

namespace fr
{
	class PostEffect;
	
	class EffectPass : public Object
	{
		FRESH_DECLARE_CLASS( EffectPass, Object );
	public:
		
		virtual Texture::ptr runPass( SmartPtr< const PostEffect > effect,
									  Texture::ptr inputTexture,
									  Texture::ptr lastFrameTexture ) const;
		
	private:
		
		VAR( ShaderProgram::ptr, m_program );
		VAR( RenderTarget::ptr, m_renderTarget );
		DVAR( Texture::FilterMode, m_outputFilterMode, Texture::FilterMode::Bilinear );
		VAR( std::vector< Texture::ptr >, m_additionalInputTextures );
		
		typedef std::vector< std::pair< std::string, std::string >> Uniforms;
		VAR( Uniforms, m_uniforms );
	};
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class PostEffect : public Sprite
	{
		FRESH_DECLARE_CLASS( PostEffect, Sprite );
	public:
		
		Vector2i virtualScreenDimensions() const;
		real virtualScreenAspectRatio() const;
		
		const std::string& propertyValue( const std::string& propertyName ) const;
		
		template< typename T >
		void propertyValue( const std::string& propertyName, const T& value );
		
	protected:
		
		virtual void drawChildren( TimeType relativeFrameTime, RenderInjector* injector ) override;
		virtual void render( TimeType relativeFrameTime, RenderInjector* injector ) override;
		
	private:

		VAR( RenderTarget::ptr, m_innerRenderTarget );
		DVAR( vec2, m_diffuseTextureSize, vec2( 320.0f, 192.0f ));
		DVAR( int, m_shaderMilliseconds, 0 );
		VAR( std::vector< EffectPass::ptr >, m_passes );

		typedef std::map< std::string, std::string > Properties;
		VAR( Properties, m_properties );
		
		Texture::ptr m_priorFrameInputTexture;
		Texture::ptr m_priorFrameResultTexture;
	};

	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	template< typename T >
	void PostEffect::propertyValue( const std::string& propertyName, const T& value )
	{
		std::ostringstream stream;
		stream << value;
		m_properties[ propertyName ] = stream.str();
	}

	template<>
	void PostEffect::propertyValue( const std::string& propertyName, const std::string& value )
	{
		m_properties[ propertyName ] = value;
	}

}

#endif
