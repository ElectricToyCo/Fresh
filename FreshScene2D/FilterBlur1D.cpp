//
//  FilterBlur1D.cpp
//  Fresh
//
//  Created by Jeff Wofford on 8/29/15.
//  Copyright (c) 2015 Jeff Wofford. All rights reserved.
//

#include "FilterBlur1D.h"

namespace
{
	using namespace fr;
	
	const std::string VERTEX_SHADER_PREFIX = R"EOF(
	
		uniform mat4 projectionMatrix;
		uniform mat4 modelViewMatrix;

		attribute vec2 position;
		attribute vec2 texCoord;

		varying lowp vec2 v_texCoord;
		varying lowp vec4 v_blurTexCoords[ $TAPS_DIV_2 ];

		void main()
		{
			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 0.0, 1.0 );
			v_texCoord = texCoord;
	)EOF";
	
	const std::string VERTEX_SHADER_SUFFIX = R"EOF(
		}
	)EOF";
	
	// See https://wiki.linaro.org/WorkingGroups/Middleware/Graphics/GLES2PortingTips

	const std::string FRAGMENT_SHADER_PREFIX = R"EOF(
		uniform sampler2D diffuseTexture;
		uniform mediump vec2 textureDimensions;
		uniform lowp vec4 color_multiply;

		varying lowp vec2 v_texCoord;
		varying lowp vec4 v_blurTexCoords[ $TAPS_DIV_2 ];

		float borderFactor( vec2 coords, vec2 dims )
		{
			bvec2 out1 = greaterThan (coords, vec2 (1,1));
			bvec2 out2 = lessThan (coords, vec2 (0,0));
			bool do_clamp = (any (out1) || any (out2));
			return float (!do_clamp);
		}
	
		void main()
		{
			vec2 tap;
			vec4 diffuse = texture2D( diffuseTexture, v_texCoord ) *
	)EOF";
			
	const std::string FRAGMENT_SHADER_SUFFIX_MULTIPLY = R"EOF(
			gl_FragColor = diffuse * color_multiply;
		}
	)EOF";

	const std::string FRAGMENT_SHADER_SUFFIX_ALPHA = R"EOF(
			float alpha = color_multiply.a * diffuse.a;
			gl_FragColor = vec4( color_multiply.rgb * alpha, alpha );
		}
	)EOF";

	real gaussian( real x, real mu, real sigma )
	{
		return std::exp( -((( x - mu ) / sigma ) * (( x - mu ) / sigma )) / 2.0 );
	}
	
	std::vector< real > gaussianKernel1D( int sideTaps )
	{
		REQUIRES( sideTaps >= 0 );
		const auto actualTaps = sideTaps * 2 + 1;
		
		std::vector< real > kernel( actualTaps );
		if( sideTaps > 0 )
		{
			// Calculate the initial kernel values.
			//
			const real sigma = sideTaps / 2.0f;
			
			real sum = 0;
			
			for( int col = 0; col < actualTaps; ++col )
			{
				const real weight = gaussian( col, sideTaps, sigma );
				
				kernel.at( col ) = weight;
				
				sum += weight;
			}
			
			// Normalize the kernel.
			//
			for( auto& x : kernel )
			{
				x /= sum;
			}
		}
		else
		{
			kernel[ 0 ] = 1.0f;
		}
		return kernel;
	}
}

namespace fr
{	
	FRESH_DEFINE_CLASS( FilterBlur1D )
	DEFINE_VAR( FilterBlur1D, int, m_size );
	DEFINE_VAR( FilterBlur1D, real, m_step );
	DEFINE_VAR( FilterBlur1D, int, m_axis );
	DEFINE_VAR( FilterBlur1D, real, m_offset );
	DEFINE_VAR( FilterBlur1D, bool, m_glow );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( FilterBlur1D )

	vec2 FilterBlur1D::rectExpansion() const
	{
		REQUIRES( m_step >= 0 );
		REQUIRES( 0 <= m_axis && m_axis < 2 );
		
		vec2 expansion;
		expansion[ m_axis ] = nTaps() * m_step;
		return expansion;
	}
	
	ShaderProgram::ptr FilterBlur1D::shaderProgram( Texture::ptr textureToFilter ) const
	{
		REQUIRES( textureToFilter );
		REQUIRES( m_size >= 0 );
		REQUIRES( m_step >= 0 );
		REQUIRES( 0 <= m_axis && m_axis < 2 );
		
		if( !m_program || m_lastSideTaps != m_size || m_lastStep != m_step || m_lastGlow != m_glow )
		{
			if( m_size > 0 )
			{
				std::ostringstream sideTapsString;
				sideTapsString << m_size;
				
				// Create the shader source files.
				//
				auto vertexShaderPrefix = VERTEX_SHADER_PREFIX;
				
				replaceAll( vertexShaderPrefix, "$TAPS_DIV_2", sideTapsString.str() );
				
				auto fragmentShaderPrefix = FRAGMENT_SHADER_PREFIX;
				replaceAll( fragmentShaderPrefix, "$TAPS_DIV_2", sideTapsString.str() );
				
				std::ostringstream vertexShaderSource;
				vertexShaderSource << vertexShaderPrefix << std::setprecision( 7 ) << std::fixed;
				
				std::ostringstream fragmentShaderSource;
				fragmentShaderSource << fragmentShaderPrefix << std::setprecision( 7 ) << std::fixed;
				
				// Calculate the kernel.
				//
				const auto kernel = gaussianKernel1D( m_size );
				
				// Fill in the initial, central fragment shader sample weight.
				//
				const real centralWeight = kernel.at( m_size );
				fragmentShaderSource << centralWeight << ";\n";
				
				// Fill in the side tap code.
				//
				const real texStep = m_step / textureToFilter->dimensions().x;
				const auto totalSideTaps = m_size * 2;
				const auto splitPoint = m_size;
				
				for( int tap = 0; tap < totalSideTaps; ++tap )
				{
					const int coordIndex = tap / 2;
					const std::string tupleString = isEven( tap ) ? "xy" : "zw";
					
					const int adjustedTap = tap < splitPoint ? tap : tap + 1;

					vec2 texOffset;
					texOffset[ m_axis ] += ( adjustedTap - splitPoint - m_offset ) * texStep;
					
					vertexShaderSource << "v_blurTexCoords[ " << coordIndex << " ]." << tupleString << " = v_texCoord + vec2( " << texOffset.x << ", " << texOffset.y << " );\n";

					const real weight = kernel.at( adjustedTap );
					fragmentShaderSource << "tap = v_blurTexCoords[ " << coordIndex << " ]." << tupleString << ";\n";
					fragmentShaderSource << "diffuse += mix( vec4( 0 ), texture2D( diffuseTexture, tap ) * " << weight << ", borderFactor( tap, textureDimensions ) );\n";
				}
				
				vertexShaderSource << VERTEX_SHADER_SUFFIX;
				
				const auto& suffix = m_glow ? FRAGMENT_SHADER_SUFFIX_ALPHA : FRAGMENT_SHADER_SUFFIX_MULTIPLY;
				
				fragmentShaderSource << suffix;
				
				VertexStructure::ptr vertexStructure = Renderer::instance().createOrGetVertexStructure( "VS_Pos2TexCoord2" );
				ASSERT( vertexStructure );
				
				release_trace( "TODO Debugging. Loading blur shaders." );
				m_program = ShaderProgram::createFromSource( vertexShaderSource.str(), fragmentShaderSource.str(), vertexStructure );
				
				if( m_program->isLinked() )
				{
					m_program->bindUniformToObjectMethod< mat4 >( "projectionMatrix", ObjectId( "Renderer'renderer'" ), "getProjectionMatrix" );
					m_program->bindUniformToObjectMethod< mat4 >( "modelViewMatrix", ObjectId( "Renderer'renderer'" ), "getModelViewMatrix" );
					m_program->bindUniformToObjectMethod< Color >( "color_multiply", ObjectId( "Renderer'renderer'" ), "getColorMultiply" );
				}
			}
			else
			{
				return Super::shaderProgram( textureToFilter );
			}
			
			m_lastSideTaps = m_size;
			m_lastStep = m_step;
			m_lastGlow = m_glow;
		}

		// TODO:
//		if( m_program && m_program->isLinked() )
//		{
//			const auto texDimensionsUniformID = m_program->getUniformId( "textureDimensions" );
//			if( texDimensionsUniformID >= 0 )
//			{
//				m_program->setUniform( texDimensionsUniformID, vector_cast< real >( textureToFilter->dimensions() ));
//			}
//		}
		
		return m_program;
	}
}

