//
//  ScreenEffects.cpp
//  Fresh
//
//  Created by Jeff Wofford on 11/20/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "ScreenEffects.h"
#include "RenderTarget.h"
#include "FreshOpenGL.h"
#include "Stage.h"
#include "Application.h"

namespace
{
	using namespace fr;
	
	SimpleMesh::ptr barrelMesh( const Vector2i& nodeDimensions, const vec2& stageDimensions, real distortion )
	{
		// Create barrel-distortion mesh for this ScreenEffects.
		//
		auto barrelDistortion = [distortion]( const vec2& coord ) -> vec2
		{
			vec2 cc = coord - 0.5f;
			float dist = cc.dot( cc );
			return coord + (cc * (dist + distortion * dist * dist) * distortion );
		};
		
		std::vector< vec4 > nodes;
		for( int y = 0; y < nodeDimensions.y; ++y )
		{
			vec2 position( 0, y / real( nodeDimensions.y - 1 ));
			for( int x = 0; x < nodeDimensions.x; ++x )
			{
				position.x = x / real( nodeDimensions.x - 1 );
				
				vec2 texCoord = barrelDistortion( position );
				
				vec2 scaledPosition = ( position - 0.5f ) * stageDimensions;
				
				nodes.emplace_back( scaledPosition.x, scaledPosition.y, texCoord.x, texCoord.y );
			}
		}
		
		// Convert nodes to triangles.
		//
		auto index = [&]( int i, int j ) { return i + j * nodeDimensions.y; };
		
		std::vector< vec4 > vertices;
		for( int y = 0; y < nodeDimensions.y - 1; ++y )
		{
			for( int x = 0; x < nodeDimensions.x - 1; ++x )
			{
				vertices.emplace_back( nodes[ index( x    , y     )] );
				vertices.emplace_back( nodes[ index( x + 1, y     )] );
				vertices.emplace_back( nodes[ index( x    , y + 1 )] );
				
				vertices.emplace_back( nodes[ index( x    , y + 1 )] );
				vertices.emplace_back( nodes[ index( x + 1, y     )] );
				vertices.emplace_back( nodes[ index( x + 1, y + 1 )] );
			}
		}
		
		auto mesh = createObject< SimpleMesh >();
		mesh->create( Renderer::PrimitiveType::Triangles, vertices, Renderer::instance().createOrGetVertexStructure( "VS_Pos2TexCoord2" ) );
		
		mesh->calculateBounds( reinterpret_cast< vec2* >( &*vertices.begin() ), reinterpret_cast< vec2* >( &*vertices.end() ), 2 );	// Skipping texcoords.
		
		return mesh;
	}
}

namespace fr
{
	FRESH_DEFINE_CLASS( ScreenEffects )
	DEFINE_VAR( ScreenEffects, DisplayObjectContainer::ptr, m_inner );
	DEFINE_VAR( ScreenEffects, fr::ShaderProgram::ptr, m_shaderProgramPrimaryPost );
	DEFINE_VAR( ScreenEffects, fr::ShaderProgram::ptr, m_shaderProgramCopy );
	DEFINE_VAR( ScreenEffects, fr::ShaderProgram::ptr, m_shaderProgramBloomTonemap );
	DEFINE_VAR( ScreenEffects, fr::ShaderProgram::ptr, m_shaderProgramHorizontalBlur );
	DEFINE_VAR( ScreenEffects, fr::ShaderProgram::ptr, m_shaderProgramVerticalBlur );
	DEFINE_VAR( ScreenEffects, fr::ShaderProgram::ptr, m_shaderProgramComposite );
	DEFINE_VAR( ScreenEffects, fr::Texture::ptr, m_noiseTexture );
	DEFINE_VAR( ScreenEffects, std::vector< fr::Texture::ptr >, m_pixelBevelTextures );
	DEFINE_VAR( ScreenEffects, int, m_shaderMilliseconds );
	DEFINE_VAR( ScreenEffects, int, m_textureUnit1 );
	DEFINE_VAR( ScreenEffects, int, m_textureUnit2 );
	DEFINE_VAR( ScreenEffects, vec2, m_diffuseTextureSize );
	DEFINE_VAR( ScreenEffects, Quality, m_shaderQuality );
	DEFINE_VAR( ScreenEffects, ShaderStates, m_shaderStates )
	DEFINE_VAR( ScreenEffects, std::string, m_currentStateName );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( ScreenEffects )

	DEFINE_ACCESSOR( ScreenEffects, const float&, chromaticAberration );
	DEFINE_ACCESSOR( ScreenEffects, const float&, bloomBrightness );
	DEFINE_ACCESSOR( ScreenEffects, const float&, bloomContrast );
	DEFINE_ACCESSOR( ScreenEffects, const float&, bloomIntensity );
	DEFINE_ACCESSOR( ScreenEffects, const float&, bevelIntensity );
	DEFINE_ACCESSOR( ScreenEffects, const float&, noiseIntensity );
	DEFINE_ACCESSOR( ScreenEffects, const float&, burnIn );
	DEFINE_ACCESSOR( ScreenEffects, const float&, saturation );
	DEFINE_ACCESSOR( ScreenEffects, const vec4&, colorMultiplied );
	DEFINE_ACCESSOR( ScreenEffects, const vec4&, rescanColor );
    DEFINE_ACCESSOR( ScreenEffects, const real&, barrelDistortion );

	void ScreenEffects::postLoad()
	{
		Super::postLoad();

		if( !m_currentStateName.empty() )
		{
			transitionToState( m_currentStateName, 0.0 );
		}
	}
	
	void ScreenEffects::update()
	{
		// Shader interpolation.
		//
		auto now = stage().realTime();
		
		if( m_transitionDuration > 0 )
		{
			auto proportionalNow = proportion( now, m_transitionStartTime, m_transitionStartTime + m_transitionDuration );
			
			m_currentShaderState = m_tweener( m_transitionStartState, m_desiredShaderState, clamp( proportionalNow, 0.0, 1.0 ));
			
			if( proportionalNow >= 1.0 )
			{
				// Done.
				m_transitionDuration = 0;
			}
		}
		
        // Adjust screen if needed.
        //
        if( !mesh() || m_currentShaderState.barrelDistortion != m_lastSetupBarrelDistortion )
        {
            setupSimulatedScreen();
        }
        
		Super::update();
	}
	
	void ScreenEffects::clearColor( Color color )
	{
		if( m_inner && m_inner->renderTarget() )
		{
			m_inner->renderTarget()->clearColor( color );
		}
	}
    
    void ScreenEffects::resizeVirtualScreen( const Vector2i& size )
    {
        ASSERT( m_inner );
        ASSERT( m_inner->renderTarget() );
        
        if( size != virtualScreenDimensions() )
        {
            m_inner->renderTarget()->create(
                size.x, size.y,
                m_inner->renderTarget()->colorBufferFormat(),
                &m_inner->renderTarget()->depthBufferFormat() );
        }
    }

	Vector2i ScreenEffects::virtualScreenDimensions() const
	{
		ASSERT( m_inner );
		
		if( m_inner->renderTarget() )
		{
			return Vector2i( m_inner->renderTarget()->width(), m_inner->renderTarget()->height() );
		}
		else
		{
			return Vector2i( 320, 192 );
		}
	}
	
	real ScreenEffects::virtualScreenAspectRatio() const
	{
		auto dims = virtualScreenDimensions();
		return dims.x / real( dims.y );
	}

	void ScreenEffects::drawChildren( TimeType relativeFrameTime, RenderInjector* injector )
	{
		Renderer& renderer = Renderer::instance();
		
		// Setup the projection for the inner virtual screen.
		//
		renderer.pushMatrix( Renderer::MAT_Projection );
		vec2 halfScreenDims = vector_cast< real >( virtualScreenDimensions() ) * 0.5f;
		renderer.setOrthoProjection( -halfScreenDims.x, halfScreenDims.x, halfScreenDims.y, -halfScreenDims.y );
		
		Super::drawChildren( relativeFrameTime, injector );
		
		renderer.popMatrix( Renderer::MAT_Projection );
	}
	
	void ScreenEffects::draw( TimeType relativeFrameTime, RenderInjector* injector )
	{
		Renderer& renderer = Renderer::instance();
		Super::draw( relativeFrameTime, injector );
		
		if( !m_inner->renderTarget() )
		{
			return;
		}
		
		if( !m_vertexBufferQuad )
		{
			m_vertexBufferQuad = createObject< VertexBuffer >();
			
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
			
			m_vertexBufferQuad->associateWithVertexStructure( Stage::getPos2TexCoord2VertexStructure() );
			m_vertexBufferQuad->loadVertices( points.begin(), points.end() );
		}

		ASSERT( m_inner );
		
		auto rawScreenTexture = m_inner->renderTarget()->getCapturedTexture();
		
		// Calculate texture sizes and texel scales for shader use.
		//
		Vector2i baseScreenDimensions( virtualScreenDimensions() );
        const bool didScreenDimensionsChange = vector_cast< real >( baseScreenDimensions ) != m_diffuseTextureSize;
        
		m_diffuseTextureSize = vector_cast< real >( baseScreenDimensions );

		////////////////////////////////////////////////////////////////
		// APPLY POST-EFFECT SHADERS
		//

		Texture::ptr passResultTexture = rawScreenTexture;
		
		if( m_shaderQuality >= Quality::Medium )
		{
			auto shaderPass = [&]( size_t renderTarget, ShaderProgram::ptr program )
			{
				ASSERT( program );
				ASSERT( renderTarget < NUM_RENDER_TARGETS );
                
                m_renderTarget[ renderTarget ]->beginCapturing();
				renderer.useShaderProgram( program );
				renderer.updateUniformsForCurrentShaderProgram( this );
				renderer.setBlendMode( Renderer::BlendMode::None );
				
				renderer.drawGeometry( Renderer::PrimitiveType::TriangleStrip, m_vertexBufferQuad, 4 );
				m_renderTarget[ renderTarget ]->endCapturing();
				passResultTexture = m_renderTarget[ renderTarget ]->getCapturedTexture();
				
				renderer.applyTexture( passResultTexture );
			};
			
			// Ready FBOs.
			//
			for( size_t i = 0; i < NUM_RENDER_TARGETS; ++i )
			{
				if( didScreenDimensionsChange || !m_renderTarget[ i ] )
				{
					Vector2i renderTargetSize = baseScreenDimensions;
					Texture::FilterMode filterMode = Texture::FilterMode::Nearest;		// TODO: Nearest for raster game
					
					switch( i )
					{
						default:
							break;
						case 2:
						case 3:
							renderTargetSize /= 2;
							filterMode = Texture::FilterMode::Bilinear;
							break;
						case 4:
							filterMode = Texture::FilterMode::Bilinear;
							break;
						case 5:
							filterMode = Texture::FilterMode::Bilinear;
							renderTargetSize = Application::instance().getWindowDimensions();
							break;
					}
					
					RenderTarget::BufferFormat bufferFormat( RenderTarget::ColorComponentType::UnsignedByte,
															 RenderTarget::OutputType::Texture );
					
					m_renderTarget[ i ] = createObject< RenderTarget >();
					m_renderTarget[ i ]->create( renderTargetSize.x, renderTargetSize.y, bufferFormat );
					m_renderTarget[ i ]->doInitialClearOnCapture( true );
					m_renderTarget[ i ]->clearColor( Color::Black );
					m_renderTarget[ i ]->getCapturedTexture()->filterMode( filterMode );
					
				}
			}
			
			m_shaderMilliseconds = int( stage().realTime() * 1000.0 );
			
			// APPLY NOISE AND RESCAN.
			
			renderer.applyTexture( rawScreenTexture );
			renderer.applyTexture( m_noiseTexture, 1 );
			renderer.applyTexture( m_priorFrameTexture, 2 );
			shaderPass( 0, m_shaderProgramPrimaryPost );

			auto priorToBloomTexture = passResultTexture;

			// Simply copy this render target texture for use next frame for burn-in and such.
			//
			shaderPass( 1, m_shaderProgramCopy );
			m_priorFrameTexture = passResultTexture;
			
			passResultTexture = priorToBloomTexture;

			// BLOOM
			
			if( m_shaderQuality >= Quality::High )
			{
				// Apply tonemap to produce glow texture.
				//
				shaderPass( 2, m_shaderProgramBloomTonemap );
				
				// Apply horizontal blur.
				//
				shaderPass( 3, m_shaderProgramHorizontalBlur );

				// Apply vertical blur.
				//
				shaderPass( 4, m_shaderProgramVerticalBlur );

				// Composite the blurred bloom with the original texture.
				//
				ASSERT( m_pixelBevelTextures.size() > 0 );
				const auto pixelBevelTexture = m_pixelBevelTextures[ m_currentShaderState.bevelType % m_pixelBevelTextures.size() ];
				
				renderer.applyTexture( priorToBloomTexture, 0 );
				renderer.applyTexture( passResultTexture, 1 );
				renderer.applyTexture( pixelBevelTexture, 2 );
				shaderPass( 5, m_shaderProgramComposite );
			}
		}
		
		// Show final FBO.
		texture( passResultTexture );
	}

	void ScreenEffects::setupSimulatedScreen()
	{
		auto barrel = barrelMesh( Vector2i( 16 ), stage().stageDimensions(), m_currentShaderState.barrelDistortion );
		mesh( barrel );
		
		m_lastSetupBarrelDistortion = m_currentShaderState.barrelDistortion;
		
		// Scale to fit to window.
		//
		const auto fittingScale = vec2( std::max( 1.0f, lerp( 1.0f, virtualScreenAspectRatio() / Application::instance().getWindowAspectRatio(), 0.4f )), 1.0f );
		scale( fittingScale );		
	}
	
	void ScreenEffects::onAddedToStage()
	{
		Super::onAddedToStage();

		setupSimulatedScreen();
		
		if( m_inner && m_inner->renderTarget() )
		{
			m_inner->renderTarget()->getCapturedTexture()->filterMode( Texture::FilterMode::Nearest );	// TODO: Nearest for raster game.
		}
	}
	
	bool ScreenEffects::ShaderState::operator==( const ShaderState& other ) const
	{
		return std::memcmp( this, &other, sizeof( *this )) == 0;
	}

	ScreenEffects::ShaderState ScreenEffects::ShaderState::operator+( const ShaderState& other ) const
	{
		ShaderState result( *this );
		result += other;
		return result;
	}

	ScreenEffects::ShaderState ScreenEffects::ShaderState::operator-( const ShaderState& other ) const
	{
		ShaderState result( *this );
		result -= other;
		return result;
	}

	ScreenEffects::ShaderState ScreenEffects::ShaderState::operator*( const ShaderState& other ) const
	{
		ShaderState result( *this );
		result *= other;
		return result;
	}

	ScreenEffects::ShaderState ScreenEffects::ShaderState::operator/( const ShaderState& other ) const
	{
		ShaderState result( *this );
		result /= other;
		return result;
	}

	ScreenEffects::ShaderState& ScreenEffects::ShaderState::operator+=( const ShaderState& other )
	{
		chromaticAberration += other.chromaticAberration;
		bloomBrightness += other.bloomBrightness;
		bloomContrast += other.bloomContrast;
		bloomIntensity += other.bloomIntensity;
		bevelIntensity += other.bevelIntensity;
		noiseIntensity += other.noiseIntensity;
		burnIn += other.burnIn;
		saturation += other.saturation;
		colorMultiplied += other.colorMultiplied;
		rescanColor += other.rescanColor;
        barrelDistortion += other.barrelDistortion;

		return *this;
	}

	ScreenEffects::ShaderState& ScreenEffects::ShaderState::operator-=( const ShaderState& other )
	{
		chromaticAberration -= other.chromaticAberration;
		bloomBrightness -= other.bloomBrightness;
		bloomContrast -= other.bloomContrast;
		bloomIntensity -= other.bloomIntensity;
		bevelIntensity -= other.bevelIntensity;
		noiseIntensity -= other.noiseIntensity;
		burnIn -= other.burnIn;
		saturation -= other.saturation;
		colorMultiplied -= other.colorMultiplied;
		rescanColor -= other.rescanColor;
        barrelDistortion -= other.barrelDistortion;

		return *this;
	}

	ScreenEffects::ShaderState& ScreenEffects::ShaderState::operator*=( const ShaderState& other )
	{
		chromaticAberration *= other.chromaticAberration;
		bloomBrightness *= other.bloomBrightness;
		bloomContrast *= other.bloomContrast;
		bloomIntensity *= other.bloomIntensity;
		bevelIntensity *= other.bevelIntensity;
		noiseIntensity *= other.noiseIntensity;
		burnIn *= other.burnIn;
		saturation *= other.saturation;
		colorMultiplied *= other.colorMultiplied;
		rescanColor *= other.rescanColor;
        barrelDistortion *= other.barrelDistortion;

		return *this;
	}

	ScreenEffects::ShaderState& ScreenEffects::ShaderState::operator/=( const ShaderState& other )
	{
		chromaticAberration /= other.chromaticAberration;
		bloomBrightness /= other.bloomBrightness;
		bloomContrast /= other.bloomContrast;
		bloomIntensity /= other.bloomIntensity;
		bevelIntensity /= other.bevelIntensity;
		noiseIntensity /= other.noiseIntensity;
		burnIn /= other.burnIn;
		saturation /= other.saturation;
		colorMultiplied /= other.colorMultiplied;
		rescanColor /= other.rescanColor;
        barrelDistortion /= other.barrelDistortion;

		return *this;
	}

	ScreenEffects::ShaderState ScreenEffects::ShaderState::operator+( double t ) const
	{
		ShaderState result( *this );
		result += t;
		return result;
	}
	
	ScreenEffects::ShaderState ScreenEffects::ShaderState::operator-( double t ) const
	{
		ShaderState result( *this );
		result -= t;
		return result;
	}
	
	ScreenEffects::ShaderState ScreenEffects::ShaderState::operator*( double t ) const
	{
		ShaderState result( *this );
		result *= t;
		return result;
	}
	
	ScreenEffects::ShaderState ScreenEffects::ShaderState::operator/( double t ) const
	{
		ShaderState result( *this );
		result /= t;
		return result;
	}
	
	ScreenEffects::ShaderState& ScreenEffects::ShaderState::operator+=( double t )
	{
		chromaticAberration += t;
		bloomBrightness += t;
		bloomContrast += t;
		bloomIntensity += t;
		bevelIntensity += t;
		noiseIntensity += t;
		burnIn += t;
		saturation += t;
		colorMultiplied += t;
		rescanColor += t;
        barrelDistortion += t;

		return *this;
	}
	
	ScreenEffects::ShaderState& ScreenEffects::ShaderState::operator-=( double t )
	{
		chromaticAberration -= t;
		bloomBrightness -= t;
		bloomContrast -= t;
		bloomIntensity -= t;
		bevelIntensity -= t;
		noiseIntensity -= t;
		burnIn -= t;
		saturation -= t;
		colorMultiplied -= t;
		rescanColor -= t;
        barrelDistortion -= t;

		return *this;
	}
	
	ScreenEffects::ShaderState& ScreenEffects::ShaderState::operator*=( double t )
	{
		chromaticAberration *= t;
		bloomBrightness *= t;
		bloomContrast *= t;
		bloomIntensity *= t;
		bevelIntensity *= t;
		noiseIntensity *= t;
		burnIn *= t;
		saturation *= t;
		colorMultiplied *= t;
		rescanColor *= t;
        barrelDistortion *= t;

		return *this;
	}
	
	ScreenEffects::ShaderState& ScreenEffects::ShaderState::operator/=( double t )
	{
		chromaticAberration /= t;
		bloomBrightness /= t;
		bloomContrast /= t;
		bloomIntensity /= t;
		bevelIntensity /= t;
		noiseIntensity /= t;
		burnIn /= t;
		saturation /= t;
		colorMultiplied /= t;
		rescanColor /= t;
        barrelDistortion /= t;

		return *this;
	}
	
	void ScreenEffects::transitionToState( const ShaderState& destinationState, TimeType transitionTime )
	{
		m_transitionDuration = transitionTime;
		
		if( m_transitionDuration > 0 )
		{
			m_transitionStartState = m_currentShaderState;
			m_desiredShaderState = destinationState;
			m_transitionStartTime = stage().realTime();
		}
		else
		{
			m_currentShaderState = m_desiredShaderState = destinationState;
		}
	}

	void ScreenEffects::transitionToState( const std::string& stateName, TimeType transitionTime )
	{
		const auto iter = m_shaderStates.find( stateName );
		ASSERT( iter != m_shaderStates.end() );
		
		m_currentStateName = stateName;
		transitionToState( iter->second, transitionTime );
	}
	
	STRUCT_DEFINE_SERIALIZATION_OPERATORS( ScreenEffects::ShaderState )
}

