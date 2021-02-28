//
//  Lighting.cpp
//  Fresh
//
//  Created by Jeff Wofford on 12/4/11.
//  Copyright (c) 2011 jeffwofford.com. All rights reserved.
//

#include "Lighting.h"
#include "Renderer.h"
#include "RenderTarget.h"
#include "Stage.h"
#include "Application.h"
#include "FreshTileGrid.h"
using namespace fr;

namespace
{
	SimpleMesh::ptr g_pWholeScreenQuad;
	SimpleMesh::ptr g_squareLightMesh;		
}

namespace fr
{
	FRESH_DEFINE_CLASS( LightMask )
	DEFINE_VAR( LightMask, DisplayObjectContainer::wptr, m_owner );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( LightMask )
	
	void LightMask::preDraw( TimeType relativeFrameTime )
	{
		if( m_owner )
		{
			rotation( angle( -m_owner->getTweenedState( (real) relativeFrameTime ).rotation() ));
		}
		Super::preDraw( 1.0 );
	}

	////////////////////////////////////////////////////////////////////////////////////
	
	FRESH_DEFINE_CLASS( LightSource )
	DEFINE_VAR( LightSource, real, m_radius );
	DEFINE_VAR( LightSource, real, m_overlayRadiusScalar );
	DEFINE_VAR( LightSource, real, m_spotNearRadius );
	DEFINE_VAR( LightSource, real, m_spotHalfArcAngleDegrees );	// If <= 0 or >= 360, specifies an omni light.
	DEFINE_VAR( LightSource, unsigned int, m_groups );
	DEFINE_VAR( LightSource, bool, m_doShadows );
	DEFINE_VAR( LightSource, DisplayObjectWithMesh::ptr, m_gradientHost );
	DEFINE_VAR( LightSource, LightMask::ptr, m_lightMask );
	DEFINE_VAR( LightSource, DisplayObjectWithMesh::ptr, m_overlay );
	DEFINE_VAR( LightSource, Color, m_destroyedColor );
	DEFINE_VAR( LightSource, real, m_destroyedRadius );
	DEFINE_VAR( LightSource, TimeType, m_destroyDuration );
	
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( LightSource )
	
	void LightSource::setDestroyStyle( TimeType duration, real destroyRadius, Color destroyColor )
	{
		m_destroyedColor = destroyColor;		
		m_destroyedRadius = destroyRadius;
		m_destroyDuration = duration;
	}
	
	void LightSource::destroyWithAnimation()
	{
		REQUIRES( !isMarkedForDeletion() && !isDestroying() );
		
		if( m_destroyDuration > 0 )
		{
			m_destroyInitialState.setStateFrom( *this );
			m_destroyInitialState.scale( m_radius );				// Using scale to store light radius.
			
			m_destroyedState.setStateFrom( *this );
			
			if( m_destroyedColor != NO_DESTROY_COLOR )
			{
				m_destroyedState.color( m_destroyedColor );
			}
			
			if( m_destroyedRadius >= 0 )
			{
				m_destroyedState.scale( m_destroyedRadius );
			}
			
			m_destroyStartTime = stage().time();
		}
		else
		{
			markForDeletion();
		}

		PROMISES( isMarkedForDeletion() || isDestroying() );
	}
	
	bool LightSource::isDestroying() const
	{
		return m_destroyStartTime >= 0;
	}
	
	void LightSource::update()
	{
		Super::update();
		
		updateDestroy();
	}
	
	void LightSource::updateDestroy()
	{
		if( isDestroying() )
		{
			TimeType relativeTime = ( stage().time() - m_destroyStartTime ) / m_destroyDuration;
			
			if( relativeTime >= 1.0 )
			{
				markForDeletion();
			}
			else
			{
				DisplayObjectState state = fr::DisplayObjectState::tweenerLinear( m_destroyInitialState, m_destroyedState, relativeTime );
				
				if( m_destroyedColor != NO_DESTROY_COLOR )
				{
					color( state.color() );
				}
				
				if( m_destroyedRadius >= 0 )
				{
					radius( state.scale().x );
				}
			}
		}														 
	}
	
	bool LightSource::mightAffect( const vec2& pos, real radius ) const
	{
		if( isMarkedForDeletion() )
		{
			return false;
		}
		
		// Trivially reject too-far positions.
		//
		const vec2 delta = pos - m_position;
		
		const auto totalRadius = m_radius + radius;
		
		if( delta.lengthSquared() >= totalRadius * totalRadius )
		{
			return false;
		}
		
		return true;
	}
	
	Color LightSource::getLitColorAt( const vec2& pos, unsigned int includeGroups, bool ignoreShadows ) const
	{
		if(( includeGroups != 0 && !( includeGroups & m_groups )) || !mightAffect( pos ))
		{
			return Color::Black;
		}
		
		const vec2 delta = pos - m_position;
		
		// For spotlights, reject out-of-range positions.
		//
		if( isSpotLight() )
		{
			const vec2 direction = vec2::makeAngleNormal( m_rotation );
			
			if( direction.dot( delta.normal() ) < std::cos( degreesToRadians( m_spotHalfArcAngleDegrees )))
			{
				return Color::Black;				
			}			
		}
		
		if( !ignoreShadows && m_doShadows && isPointInShadow( pos ))
		{
			return Color::Black;
		}
		
		const real distance = delta.length();		
		return lerp( Color::Black, color(), 1.0f - distance / m_radius );
	}
	
	bool LightSource::isPointInShadow( const vec2& pos ) const
	{
		const vec2 lightSpacePos( pos - m_position );
		for( auto iterPoint = m_shadowTriangles.begin(); iterPoint != m_shadowTriangles.end(); iterPoint += 3 )
		{
			if( isPointInTriangle( *iterPoint, *( iterPoint + 1 ), *( iterPoint + 2 ), lightSpacePos ))
			{
				return true;
			}
		}
		return false;
	}
	
	void LightSource::onAddedToStage()
	{
		Super::onAddedToStage();
		
		ASSERT( m_gradientHost->texture() );
		
		// If I have a texture, transfer it to my gradient host.
		//
		if( texture() )
		{
			m_gradientHost->texture( texture() );
			texture( nullptr );
		}
		
		// Set scale based on radius.
		//
		if( m_radius > 0 )
		{
			setupGradientDraw();
		}
	}

	void LightSource::setGradientTexture( SmartPtr< fr::Texture > texture )
	{
		m_gradientHost->texture( texture );
		setupGradientDraw();
	}
	
	void LightSource::setGradientTextureByName( const std::string& name )
	{
		m_gradientHost->setTextureByName( name );
		setupGradientDraw();
	}
	
	WeakPtr< FreshTileGrid > LightSource::tileGrid( WeakPtr< FreshTileGrid > tileGrid_ )
	{
		if( m_tileGrid != tileGrid_ )
		{
			m_tileGrid = tileGrid_;
			m_isDirty = true;
		}
		return m_tileGrid;
	}
	
	void LightSource::radius( real r )
	{
		REQUIRES( r > 0 );
		if( m_radius != r )
		{
			m_radius = r;
			m_isDirty = true;

			setupGradientDraw();
		}
	}
	
	void LightSource::spotHalfArcAngleDegrees( real angle )
	{
		if( m_spotHalfArcAngleDegrees != angle )
		{
			m_spotHalfArcAngleDegrees = angle;
			if( m_radius )
			{
				setupGradientDraw();
			}
		}
	}
	
	void LightSource::spotNearRadius( real radius )
	{
		if( m_spotNearRadius != radius )
		{
			m_spotNearRadius = radius;
			if( m_radius )
			{
				setupGradientDraw();
			}
		}
	}
	
	void LightSource::preRender( TimeType relativeFrameTime )
	{		
		// If I've changed since the last time I cached, then update the cache.
		//		
		State currentState;
		recordState( currentState, relativeFrameTime );
		
		if( m_isDirty || currentState != m_lastCachedState )
		{
			updateCachedLightingInfo( currentState );
		}
		
		Super::preRender( relativeFrameTime );
	}
	
	void LightSource::recordState( State& state, TimeType relativeFrameTime ) const
	{
		DisplayObjectState displayObjectState = getTweenedState( relativeFrameTime );
		
		state.position = displayObjectState.position();
		state.rotation = angle( isSpotLight() ? displayObjectState.rotation() : 0 );
		state.radius = m_radius;
		state.spotNearRadius = m_spotNearRadius;
		state.spotHalfArcAngleDegrees = m_spotHalfArcAngleDegrees;
		state.color = displayObjectState.color();
		state.doShadows = m_doShadows;
	}
	
	void LightSource::updateCachedLightingInfo( const State& currentState )
	{
		if( currentState.doShadows && m_radius > 0 && hasStage() )
		{
			m_shadowTriangles.clear();
			
			// Convert blockers to shadow triangles.
			//
			vec2 spotDirection = vec2::makeAngleNormal( currentState.rotation );

			if( m_tileGrid )
			{
				const Tile& tile = m_tileGrid->getTile( currentState.position );
				tile.getShadowTriangles( currentState.position, currentState.radius, m_shadowTriangles, currentState.spotHalfArcAngleDegrees, spotDirection );
			}
			
//			createTrianglesForBlocker( blocker, currentState.position, currentState.radius, m_shadowTriangles, currentState.spotHalfArcAngleDegrees, spotDirection );
			
			// Convert shadow triangles to a VBO, if any are available.
			//
			if( m_shadowTriangles.empty() )
			{
				// Disable mask.
				//
				m_lightMask->visible( false );
			}
			else
			{
				// Send the triangle points into a VBO for rendering.
				//
				if( !m_vertexStructure )
				{
					m_vertexStructure = stage().getPos2VertexStructure();
				}
				ASSERT( m_vertexStructure );
				
				m_lightMask->visible( true );
				m_lightMask->mesh()->create( Renderer::PrimitiveType::Triangles,
									  m_shadowTriangles,
									  m_vertexStructure );
			}
		}
		else
		{
			// Disable mask.
			//
			m_lightMask->visible( false );
		}
		
		m_lastCachedState = currentState;
		m_isDirty = false;
	}
	
	void LightSource::setupGradientDraw()
	{
		ASSERT( m_radius > 0 );
		
		if( isSpotLight() )
		{
			// Setup a trapezoid or triangle facing along the +X axis.
			//
			ASSERT( m_spotNearRadius >= 0 );
			ASSERT( m_spotNearRadius < m_radius );
			
			const real spotNearRadiusRelative = m_spotNearRadius / m_radius;
			
			const vec2 edgeNormals[] =		// 0 => left, 1 => right
			{
				vec2::makeAngleNormal( angle( -m_spotHalfArcAngleDegrees )),
				vec2::makeAngleNormal( angle(  m_spotHalfArcAngleDegrees ))
			};
			
			std::vector< vec2 > points( 6 + (( m_spotNearRadius > 0 ) ? 2 : 0 ));	// 3 or 4 corners * 2 vec2s each.			
			size_t i = 0;
			
			points[ i++ ] = edgeNormals[ 0 ] * spotNearRadiusRelative;								// Near left position
			points[ i++ ].set( 0.0f, 0.0f );
			points[ i++ ].set( 1.0f, edgeNormals[ 0 ].y / edgeNormals[ 0 ].x );						// Far  left position
			points[ i++ ].set( 1.0f, 0.0f );
			
			// Do we need a trapezoid or a triangle?
			//
			if( m_spotNearRadius > 0 )
			{
				points[ i++ ] = edgeNormals[ 1 ] * spotNearRadiusRelative;							// Near right position
				points[ i++ ].set( 0.0f, 0.0f );
			}
			
			// Conclude the trapezoid or triangle.
			//
			points[ i++ ].set( 1.0f, edgeNormals[ 1 ].y / edgeNormals[ 1 ].x );						// Far right position
			points[ i++ ].set( 0.0f, 1.0f );
			
			ASSERT( i == points.size() );
			
			SimpleMesh::ptr pSpotlightMesh = createObject< SimpleMesh >();
			pSpotlightMesh->create( Renderer::PrimitiveType::TriangleStrip, 
								   points,
								   Renderer::instance().createOrGetVertexStructure( "VS_Pos2TexCoord2" ),
								   2 );
			pSpotlightMesh->calculateBounds( points, 2 );
			
			m_gradientHost->mesh( pSpotlightMesh );			
		}
		else
		{
			// Setup simple quad.
			//
			if( !g_squareLightMesh )
			{
				g_squareLightMesh = getObject< SimpleMesh >( "SM_ZeroCenteredDiameter2" );
			}
			m_gradientHost->mesh( g_squareLightMesh );
		}
		m_gradientHost->scale( m_radius / ( m_gradientHost->texture()->dimensions().x * Application::instance().config().contentScale() ));
		
		if( m_overlay )
		{
			m_overlay->scale(( m_radius * m_overlayRadiusScalar ) / ( m_overlay->texture()->dimensions().x * Application::instance().config().contentScale() ));
		}
	}
	
	/////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////

	FRESH_DEFINE_CLASS( Lighting )
	DEFINE_VAR( Lighting, Color, m_ambientColor );
	DEFINE_VAR( Lighting, real, m_ambientColorScalar );
	DEFINE_VAR( Lighting, real, m_desiredAmbientColorScalar );
	DEFINE_VAR( Lighting, real, m_ambientColorScalarLerpAlpha );
	DEFINE_VAR( Lighting, SmartPtr< RenderTarget >, m_pRenderTarget );
	DEFINE_VAR( Lighting, WeakPtr< FreshTileGrid >, m_tileGrid );

	
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( Lighting )
	
	Lighting::Lighting( const ClassInfo& assignedClassInfo, NameRef objectName )
	:	Super( assignedClassInfo, objectName )
	,	m_ambientColor( Color::Black )
	,	m_pRenderTarget( createObject< RenderTarget >( name() + "RT lighting" ))
	{
		// TODO Scale offscreen buffer for lighting to an appropriate size for correct performance on this particular device.
		//
		const unsigned int lightBufferTextureDivisor = 1;
		
		// Scale down the lightmap for slower platforms.
		//
		const vec2 screenSize = vector_cast< real >( Renderer::instance().getViewport().dimensions() );
		assert( screenSize.x > 0 && screenSize.y > 0 );
		
		// TODO quantize the lightmap size and location in order to eliminate "crawling" of stationary shadows.
		
		RenderTarget::BufferFormat depthBufferFormat{ RenderTarget::ColorComponentType::UnsignedByte, RenderTarget::OutputType::Renderbuffer };
		
		m_pRenderTarget->create( static_cast< unsigned int >( screenSize.x / lightBufferTextureDivisor ),
								static_cast< unsigned int >( screenSize.y / lightBufferTextureDivisor ),
								RenderTarget::BufferFormat{ RenderTarget::ColorComponentType::UnsignedByte, RenderTarget::OutputType::Texture },
								&depthBufferFormat );
		
		Texture::ptr pRenderTargetTexture = m_pRenderTarget->getCapturedTexture();
		
		// Turn off bilinear filtering for screen-resolution lightmaps.
		//
		if( lightBufferTextureDivisor == 1 )
		{
			pRenderTargetTexture->filterMode( Texture::FilterMode::Nearest );
		}
		
		texture( pRenderTargetTexture );
		blendMode( Renderer::BlendMode::Multiply );
		
		// Use the full-screen shader program.
		//
		shaderProgram( Renderer::instance().createOrGetShaderProgram( "SP_WholeScreen" ));
		
		if( !g_pWholeScreenQuad )
		{
			g_pWholeScreenQuad = createOrGetObject< SimpleMesh >( "SM_WholeScreenQuad" );
		}
		ASSERT( g_pWholeScreenQuad && g_pWholeScreenQuad->isReadyToDraw());		
	}
	
	void Lighting::draw( TimeType relativeFrameTime, RenderInjector* injector )
	{
		Renderer& renderer = Renderer::instance();
		
		renderer.pushDebugMarker( "Lighting" );
		
		// Capture child rendering in a render target.
		//
		if( m_pRenderTarget )
		{
			m_pRenderTarget->doInitialClearOnCapture( true );
			m_pRenderTarget->clearColor( m_ambientColor * m_ambientColorScalar );
			m_pRenderTarget->beginCapturing();
		}
		
		// Just draw children, not the full sprite action.
		//
		DisplayObjectContainer::draw( relativeFrameTime, injector );

		// Render the render target.
		//
		if( m_pRenderTarget )
		{
			m_pRenderTarget->endCapturing();

			// Render the lighting sprite with the captured texture
			// but use our special shader program that ignores transformation.
			// This avoids pushing and popping the matrix, translating etc.
			//
			renderer.applyTexture( texture() );
			
			renderer.setBlendMode( blendMode() );

			renderer.useShaderProgram( shaderProgram() );
			shaderProgram()->updateBoundUniforms();

			g_pWholeScreenQuad->draw();
		}
		
		renderer.popDebugMarker();
	}

	Color Lighting::getLitColorAt( const vec2& pos, unsigned int includeGroups, bool ignoreShadows, bool includeAmbient ) const
	{
		Color result = includeAmbient ? m_ambientColor * m_ambientColorScalar : Color::Black;
		
		eachLightSource( [&]( const LightSource& lightSource )
		{
			result += lightSource.getLitColorAt( pos, includeGroups, ignoreShadows );
		} );
		
		return result;
	}
	
	std::vector< LightSource::ptr > Lighting::getNearestLightSources( const vec2& pos, size_t maxLights, unsigned int includeGroups, bool ignoreShadows )
	{
		// Feed all lights into a list.
		//
		std::vector< LightSource::ptr > lightSources;
		
		eachLightSource( [&]( LightSource& lightSource )
						{
							lightSources.push_back( &lightSource );
						} );
		
		// Sort them by distance.
		//
		std::sort( lightSources.begin(), lightSources.end(), [&]( LightSource::ptr a, LightSource::ptr b )
																 {
																	 return ( a->position() - pos ).lengthSquared() < ( b->position() - pos ).lengthSquared();
																 } );
		
		// Cull out invalid ones.
		//
		size_t nIncluded = 0;
		auto firstToErase = lightSources.begin();
		for( auto iter = lightSources.begin(); ( maxLights == 0 || nIncluded < maxLights ) && iter != lightSources.end(); ++iter )
		{
			Color lightColor = (*iter)->getLitColorAt( pos, includeGroups, ignoreShadows );
			
			if( lightColor != Color::Black )
			{
				*firstToErase = *iter;
				++firstToErase;
				
				++nIncluded;
			}
		}
		
		// Get rid of excess sources.
		//
		lightSources.erase( firstToErase, lightSources.end() );
		
		return lightSources;
	}
	
	void Lighting::dirtyAffectedLights( const vec2& pos, real radius )
	{
		eachLightSource( [&]( LightSource& lightSource )
		{
			if( lightSource.mightAffect( pos, radius ))
			{
				lightSource.setDirty();
			}
		} );
	}
	
	void Lighting::update()
	{
		eachLightSource( [&]( LightSource& lightSource )
						{
							lightSource.tileGrid( m_tileGrid );
						} );
		
		
		Super::update();
		
		// Update ambient lighting color scalar.
		//
		m_ambientColorScalar = lerp( m_ambientColorScalar, m_desiredAmbientColorScalar, m_ambientColorScalarLerpAlpha );
	}
}






