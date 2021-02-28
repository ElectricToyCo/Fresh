//
//  PostEffect.cpp
//  Fresh
//
//  Created by Jeff Wofford on 6/10/15.
//  Copyright (c) 2015 Jeff Wofford. All rights reserved.
//

#include "PostEffect.h"
#include "Stage.h"

namespace
{
	using namespace fr;
	
	VertexBuffer::ptr quadBuffer()
	{
		static VertexBuffer::ptr vertexBufferQuad;
		
		if( !vertexBufferQuad )
		{
			vertexBufferQuad = createObject< VertexBuffer >();
			
			std::vector< Vector2f > points;		// 4 vertices of 2 2-float components (pos, texcoord) each.
			
			points.reserve( 8 );
			
			points.push_back( Vector2f( -1.0f,  -1.0f ));
			points.push_back( Vector2f(  0.0f,   0.0f ));
			
			points.push_back( Vector2f(  1.0f,  -1.0f ));
			points.push_back( Vector2f(  1.0f,   0.0f ));
			
			points.push_back( Vector2f( -1.0f,   1.0f ));
			points.push_back( Vector2f(  0.0f,   1.0f ));
			
			points.push_back( Vector2f(  1.0f,   1.0f ));
			points.push_back( Vector2f(  1.0f,   1.0f ));
			
			vertexBufferQuad->associateWithVertexStructure( Stage::getPos2TexCoord2VertexStructure() );
			vertexBufferQuad->loadVertices( points.begin(), points.end() );
		}
		return vertexBufferQuad;
	}
	
	template< typename T >
	void assignUniform( ShaderProgram& program, int uniformId, const std::string& uniformValueStr )
	{
		Destringifier destringifier( uniformValueStr );
		T uniformValue;
		destringifier >> uniformValue;
		program.setUniform( uniformId, uniformValue );
	}
	
	using UniformAssignment = std::function< void( ShaderProgram&, int, const std::string& ) >;
	UniformAssignment assignmentFunction( const std::string& type )
	{
		static std::map< std::string, UniformAssignment > assignments;
		if( assignments.empty() )
		{
			assignments[ "int" ] = std::bind( &assignUniform< int >, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 );
			assignments[ "float" ] = std::bind( &assignUniform< float >, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 );
			assignments[ "Color" ] = std::bind( &assignUniform< Color >, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 );
			assignments[ "vec2" ] = std::bind( &assignUniform< vec2 >, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 );
			assignments[ "vec3" ] = std::bind( &assignUniform< vec3 >, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 );
			assignments[ "vec4" ] = std::bind( &assignUniform< vec4 >, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 );
		}
		
		const auto iter = assignments.find( type );
		if( iter != assignments.end() )
		{
			return iter->second;
		}
		else
		{
			dev_error( "Could not find a uniform assignment for type '" << type << "'." );
			return UniformAssignment{};
		}
	}
}

namespace fr
{	
	FRESH_DEFINE_CLASS( EffectPass )
	DEFINE_VAR( EffectPass, ShaderProgram::ptr, m_program );
	DEFINE_VAR( EffectPass, RenderTarget::ptr, m_renderTarget );
	DEFINE_VAR( EffectPass, Texture::FilterMode, m_outputFilterMode );
	DEFINE_VAR( EffectPass, std::vector< Texture::ptr >, m_additionalInputTextures );
	DEFINE_VAR( EffectPass, Uniforms, m_uniforms );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( EffectPass )

	Texture::ptr EffectPass::runPass( SmartPtr< const PostEffect > effect,
									  Texture::ptr inputTexture,
									  Texture::ptr lastFrameTexture ) const
	{
		if( m_program && m_renderTarget )
		{
			auto& renderer = Renderer::instance();

			size_t i = 0;
			renderer.applyTexture( inputTexture, i++ );
			renderer.applyTexture( lastFrameTexture, i++ );
			
			for( auto texture : m_additionalInputTextures )
			{
				renderer.applyTexture( texture, i++ );
			}

			m_renderTarget->beginCapturing();

			renderer.useShaderProgram( m_program );
			
			renderer.updateUniformsForCurrentShaderProgram( effect );
			
			// Apply uniforms.
			//
			for( const auto& uniform : m_uniforms )
			{
				const std::string& uniformType = uniform.first;
				const std::string& uniformName = uniform.second;
				const std::string& uniformValueStr = effect->propertyValue( uniformName );
				if( uniformValueStr.empty() == false )
				{
					int uniformId = m_program->getUniformId( uniformName );
					if( uniformId >= 0 )
					{
						auto assignmentFn = assignmentFunction( uniformType );
						if( assignmentFn )
						{
							assignmentFn( *m_program, uniformId, uniformValueStr );
						}
					}
					else
					{
						dev_error( "Unrecognized shader uniform '" << uniformName << "'." );
					}
				}
				else
				{
					dev_error( "Unrecognized property for uniform '" << uniformName << "'." );
				}
			}
			
			renderer.setBlendMode( Renderer::BlendMode::None );
			
			renderer.drawGeometry( Renderer::PrimitiveType::TriangleStrip, quadBuffer(), 4 );
			m_renderTarget->endCapturing();
			
			auto passResultTexture = m_renderTarget->getCapturedTexture();
			renderer.applyTexture( passResultTexture );
			return passResultTexture;
		}
		else
		{
			return inputTexture;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	FRESH_DEFINE_CLASS( PostEffect )
	DEFINE_VAR( PostEffect, vec2, m_diffuseTextureSize );
	DEFINE_VAR( PostEffect, int, m_shaderMilliseconds );
	DEFINE_VAR( PostEffect, std::vector< EffectPass::ptr >, m_passes );
	DEFINE_VAR( PostEffect, RenderTarget::ptr, m_innerRenderTarget );
	DEFINE_VAR( PostEffect, Properties, m_properties );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( PostEffect )
	
	Vector2i PostEffect::virtualScreenDimensions() const
	{
		if( m_innerRenderTarget )
		{
			return Vector2i( m_innerRenderTarget->width(), m_innerRenderTarget->height() );
		}
		else
		{
			return Vector2i( 0 );
		}
	}
	
	real PostEffect::virtualScreenAspectRatio() const
	{
		auto dims = virtualScreenDimensions();
		if( dims.y > 0 )
		{
			return dims.x / static_cast< real >( dims.y );
		}
		else
		{
			return 0;
		}
	}
	
	const std::string& PostEffect::propertyValue( const std::string& propertyName ) const
	{
		const auto iter = m_properties.find( propertyName );
		if( iter != m_properties.end() )
		{
			return iter->second;
		}
		else
		{
			static std::string defaultString;
			return defaultString;
		}
	}

	void PostEffect::drawChildren( TimeType relativeFrameTime, RenderInjector* injector )
	{
		Renderer& renderer = Renderer::instance();
		
		// Setup the transformations for the inner virtual screen.
		//
		renderer.pushMatrix( Renderer::MAT_ModelView );
		renderer.setMatrixToIdentity( Renderer::MAT_ModelView );
		
		renderer.pushMatrix( Renderer::MAT_Texture );
		renderer.setMatrixToIdentity( Renderer::MAT_Texture );
		
		renderer.pushMatrix( Renderer::MAT_Projection );
		vec2 halfScreenDims = vector_cast< real >( virtualScreenDimensions() ) * 0.5f;
		if( halfScreenDims.x > 0 && halfScreenDims.y > 0 )
		{
			renderer.setOrthoProjection( -halfScreenDims.x, halfScreenDims.x, halfScreenDims.y, -halfScreenDims.y );
		}
		
		Super::drawChildren( relativeFrameTime, injector );
		
		renderer.popMatrix( Renderer::MAT_Projection );
		renderer.popMatrix( Renderer::MAT_Texture );
		renderer.popMatrix( Renderer::MAT_ModelView );
	}
	
	void PostEffect::render( TimeType relativeFrameTime, RenderInjector* injector )
	{
		TIMER_AUTO( PostEffect::render )
		
		if( !doesWantToRender() )
		{
			return;
		}
		
		// This is unusual ordering. First we draw the children.
		//
		if( m_innerRenderTarget )
		{
			m_innerRenderTarget->beginCapturing();
		}
		
		drawChildren( relativeFrameTime, injector );
		
		// Then grab the resulting renderTarget texture.
		//
		Texture::ptr thisFrameInputTexture = texture();

		if( m_innerRenderTarget )
		{
			m_innerRenderTarget->endCapturing();
			
			thisFrameInputTexture = m_innerRenderTarget->getCapturedTexture();
			
			// Setup standard uniforms.
			//
			Vector2i baseScreenDimensions( virtualScreenDimensions() );
			m_diffuseTextureSize = vector_cast< real >( baseScreenDimensions );
			
			m_shaderMilliseconds = int( stage().realTime() * 1000.0 );
		}
		
		Texture::ptr resultTexture = thisFrameInputTexture;
		for( const auto& pass : m_passes )
		{
			resultTexture = pass->runPass( this, resultTexture, m_priorFrameResultTexture );
		}
		
		// Finally, render myself as a sprite (but not as a container).
		//
		if( !injector || !injector->preDraw( relativeFrameTime, *this ))
		{
			preDraw( relativeFrameTime );
		}
		
		// Draw me as a sprite.
		//
		m_priorFrameInputTexture = thisFrameInputTexture;

		m_priorFrameResultTexture = resultTexture;
		texture( m_priorFrameResultTexture );

		drawSprite( relativeFrameTime );
		
		forEachComponent( std::bind( &DisplayObjectComponent::render, std::placeholders::_1, std::ref( *this ), relativeFrameTime ));
		
		if( !injector || !injector->postDraw( relativeFrameTime, *this ))
		{
			postDraw( relativeFrameTime );
		}
	}
}

