/*
 *  ParticleEmitter.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 7/24/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#include "ParticleEmitter.h"
#include "Texture.h"
#include "Renderer.h"
#include "Stage.h"
#include "Objects.h"

namespace
{
	fr::VertexBuffer::ptr g_particleSpriteVertexBuffer;
	fr::VertexBuffer::ptr g_particleSparkVertexBuffer;	
}

namespace fr
{
	
	STRUCT_DEFINE_SERIALIZATION_OPERATORS( ParticleEmitter::AttractorRepulsor )

	FRESH_DEFINE_CLASS( ParticleEmitter )


	DEFINE_VAR( ParticleEmitter, RenderMode,			m_renderMode );
	DEFINE_VAR( ParticleEmitter, DisplayObject::wptr,	m_spatialBasis );
	DEFINE_VAR( ParticleEmitter, TimeType,				m_cycleDuration );
	DEFINE_VAR( ParticleEmitter, TimeType,				m_particlesPerSecond );
	DEFINE_VAR( ParticleEmitter, float,				m_sparkTailScalar );
	DEFINE_VAR( ParticleEmitter, float,				m_sparkMinTailLength );
	DEFINE_VAR( ParticleEmitter, VecColorKeyframes, 	m_vecKeyframesColor );
	DEFINE_VAR( ParticleEmitter, VecVector2Keyframes, 	m_vecKeyframesScale );
	DEFINE_VAR( ParticleEmitter, bool, 				m_isSpawningEnabled );
	DEFINE_VAR( ParticleEmitter, bool,					m_destroyWhenAllDead );
	DEFINE_VAR( ParticleEmitter, Rectanglef,			m_rectSpawnArea );
	DEFINE_VAR( ParticleEmitter, angle,				m_rectSpawnAreaRotation );
	DEFINE_VAR( ParticleEmitter, Vector2f,				m_circleSpawnAreaCenter );
	DEFINE_VAR( ParticleEmitter, float,				m_circleSpawnAreaRadius );
	DEFINE_VAR( ParticleEmitter, SmartPtr< Texture >,	m_particleTexture );
	DEFINE_VAR( ParticleEmitter, Vector2f,				m_baseScale );
	DEFINE_VAR( ParticleEmitter, Vector2f,				m_particleForce );
	DEFINE_VAR( ParticleEmitter, Vector2f,				m_spawnBaseVelocity );
	DEFINE_VAR( ParticleEmitter, Range< float >,		m_spawnVelocityAngleRange );
	DEFINE_VAR( ParticleEmitter, Range< float >,		m_spawnSpeedRange );
	DEFINE_VAR( ParticleEmitter, Range< float >,		m_spawnRotationRange );
	DEFINE_VAR( ParticleEmitter, Range< float >,		m_spawnAngularVelocityRange );
	DEFINE_VAR( ParticleEmitter, Range< Vector2f >,	m_spawnScaleRange );
	DEFINE_VAR( ParticleEmitter, float, 				m_velocityDamping );
	DEFINE_VAR( ParticleEmitter, float,				m_angularDamping );
	DEFINE_VAR( ParticleEmitter, Range< Color >,		m_perFrameColorFlickerRange );
	DEFINE_VAR( ParticleEmitter, Range< Color >,		m_perParticleColorRange );
	DEFINE_VAR( ParticleEmitter, std::vector< AttractorRepulsor >, m_attractorRepulsors );
	DEFINE_VAR( ParticleEmitter, Vector2f,			m_textureSubdivisionSize );
	DEFINE_VAR( ParticleEmitter, uint,				m_textureSubdivisionsX );
	DEFINE_VAR( ParticleEmitter, uint,				m_textureSubdivisionsY );
	DEFINE_VAR( ParticleEmitter, TimeType,			m_animFramesPerSecond );
	DEFINE_VAR( ParticleEmitter, bool,				m_doRandomizeAnimStart );
	DEFINE_VAR( ParticleEmitter, size_t,			m_startingParticles );
	DEFINE_VAR( ParticleEmitter, bool,				m_creationWarmup );
	DEFINE_VAR( ParticleEmitter, bool,				m_creationSpawnBurst );
	DEFINE_VAR( ParticleEmitter, bool,				m_creationPauseAfterBurst );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( ParticleEmitter )
	

	ParticleEmitter::ParticleEmitter( const ClassInfo& assignedClassInfo, NameRef objectName /* = DEFAULT_OBJECT_NAME */ )
	:	DisplayObject( assignedClassInfo, objectName )
	{
		velocityDamping( m_velocityDamping );
		angularDamping( m_angularDamping );
		
		// Setup global particle render geometry.
		//
		if( !g_particleSpriteVertexBuffer )
		{		
			const float SMALL_UV = 0.0f;
			const float LARGE_UV = 1.0f - SMALL_UV;
			
			Vector2f arrRenderGeometry[ 8 ] = 
			{
				Vector2f( -0.5f,  0.5f ),				// BL pos
				Vector2f(  SMALL_UV,  LARGE_UV),		// BL uv
				Vector2f(  0.5f,  0.5f ),				// BR pos
				Vector2f(  LARGE_UV,  LARGE_UV ),		// BR uv
				Vector2f( -0.5f, -0.5f ),				// UL pos
				Vector2f(  SMALL_UV,  SMALL_UV ),		// UL uv
				Vector2f(  0.5f, -0.5f ),				// UR pos
				Vector2f(  LARGE_UV,  SMALL_UV ),		// UR uv
			};
			
			VertexBuffer::ptr vertexBuffer = createObject< VertexBuffer >();
			vertexBuffer->associateWithVertexStructure( Stage::getPos2TexCoord2VertexStructure() );
			vertexBuffer->loadVertices( arrRenderGeometry, arrRenderGeometry + 8 );
			g_particleSpriteVertexBuffer = vertexBuffer;
		}
		ASSERT( g_particleSpriteVertexBuffer );

		if( !g_particleSparkVertexBuffer )
		{
			const float SMALL_UV = 0.0f;
			const float LARGE_UV = 1.0f - SMALL_UV;
			
			Vector2f arrRenderGeometry[ 8 ] = 
			{
				Vector2f( -0.5f,  1.0f ),				// BL pos
				Vector2f(  SMALL_UV,  SMALL_UV ),		// BL uv
				Vector2f(  0.5f,  1.0f ),				// BR pos
				Vector2f(  LARGE_UV,  SMALL_UV ),		// BR uv
				Vector2f( -0.5f, 0.0f ),				// UL pos
				Vector2f(  SMALL_UV,  LARGE_UV ),		// UL uv
				Vector2f(  0.5f, 0.0f ),				// UR pos
				Vector2f(  LARGE_UV,  LARGE_UV ),		// UR uv
			};
			
			VertexBuffer::ptr vertexBuffer = createObject< VertexBuffer >();
			vertexBuffer->associateWithVertexStructure( Stage::getPos2TexCoord2VertexStructure() );
			vertexBuffer->loadVertices( arrRenderGeometry, arrRenderGeometry + 8 );
			g_particleSparkVertexBuffer = vertexBuffer;
		}
		ASSERT( g_particleSparkVertexBuffer != 0 );
		
		m_spatialBasis = this;
		isTouchEnabled( false );
	}

	void ParticleEmitter::startSpawning()
	{
		m_isSpawningEnabled = true;
		m_spawnTimeAccumulator = 0;
	}

	void ParticleEmitter::pauseSpawning()
	{
		m_isSpawningEnabled = false;
	}

	bool ParticleEmitter::isSpawningPaused()
	{
		return !m_isSpawningEnabled;
	}

	void ParticleEmitter::setMarkForDeletionWhenAllParticlesDead( bool doMark )
	{
		m_destroyWhenAllDead = doMark;
	}

	void ParticleEmitter::reset()
	{
		m_iBeginLiveParticles = m_iEndLiveParticles = 0;
		m_spawnTimeAccumulator = 0;
	}

	void ParticleEmitter::normalizeIndices()
	{
		// Ensure that all the begin index fits into the range (0, m_particles.size()).

		if( m_particles.empty() )
		{
			return;
		}
		
		ASSERT( m_iBeginLiveParticles <= m_iEndLiveParticles );
		
		size_t nLivingParticles = m_iEndLiveParticles - m_iBeginLiveParticles;
		
		m_iBeginLiveParticles = m_iBeginLiveParticles % m_particles.size();
		m_iEndLiveParticles = m_iBeginLiveParticles + nLivingParticles;
		
		ASSERT( m_iBeginLiveParticles <= m_iEndLiveParticles );
	}
	
	uint ParticleEmitter::numSubdivisions() const
	{
		const auto result = std::max( 1U, m_textureSubdivisionsX * m_textureSubdivisionsY );
		PROMISES( result > 0 );
		return result;
	}
	
	uint ParticleEmitter::subdivision( uint x, uint y ) const
	{
		auto result = x + y * m_textureSubdivisionsX;
		PROMISES( result < numSubdivisions() );
		return result;
	}
	
	void ParticleEmitter::subdivisionCoords( uint subdivision, uint& outX, uint& outY ) const
	{
		subdivision = std::min( subdivision, numSubdivisions() - 1 );
		outX = m_textureSubdivisionsX > 0 ? subdivision % m_textureSubdivisionsX : 0;
		outY = m_textureSubdivisionsX > 0 ? subdivision / m_textureSubdivisionsX : 0;
		PROMISES( m_textureSubdivisionsX == 0 || outX < m_textureSubdivisionsX );
		PROMISES( m_textureSubdivisionsX == 0 || outY < m_textureSubdivisionsY );
	}
	
	bool ParticleEmitter::doAnimateParticles() const
	{
		return m_animFramesPerSecond > 0;
	}
	
	void ParticleEmitter::maxParticles( size_t particles )
	{
		const size_t oldMaxParticles = m_particles.size();

		if( oldMaxParticles != particles )
		{
			normalizeIndices();
			
			//
			// Ensure that particle indices still fit into a sensible range.
			//
			
			if( particles > oldMaxParticles )
			{
				//
				// Increasing the number of particles.
				//
				
				// Make space for the new particles at the end of the vector.
				//
				m_particles.resize( particles );
				
				// We need to copy particles that are alive, but to the "left" of the begin index,
				// back to the end of the vector.
				//
				for( size_t iParticle = oldMaxParticles; iParticle < m_iEndLiveParticles; ++iParticle )
				{
					ASSERT( iParticle % particles != m_iBeginLiveParticles );		// Don't stomp the beginning particle.
					m_particles[ iParticle % particles ] = m_particles[ iParticle % oldMaxParticles ];
				}
			}
			else
			{
				// Decreasing the number of particles.
				
				// We're suddenly killing the oldest particles here, which may cause visual hitches.
				
				// Copy the newest particles into the location identified as "begin". 
				//
				size_t nLivingParticles = m_iEndLiveParticles - m_iBeginLiveParticles;
				
				size_t nParticlesToRetain = std::min( nLivingParticles, particles );
				
				size_t iOldestParticleToRetain = m_iEndLiveParticles - nParticlesToRetain;
				ASSERT( iOldestParticleToRetain >= m_iBeginLiveParticles );
				
				Particles tempVec( nParticlesToRetain );
				
				for( size_t i = 0; i < nParticlesToRetain; ++i )
				{
					tempVec[ i ] = m_particles[ ( i + iOldestParticleToRetain ) % oldMaxParticles ];
				}
				
				// Shrink and copy the vector.
				//
				m_particles.resize( particles );
				std::copy( tempVec.begin(), tempVec.end(), m_particles.begin() );
				
				// Finally update the indices to use the new vector.
				//
				m_iBeginLiveParticles = 0;
				m_iEndLiveParticles = m_iBeginLiveParticles + nParticlesToRetain;
			}
			
			ASSERT( m_iBeginLiveParticles <= m_iEndLiveParticles );
			
			ASSERT( m_cycleDuration > 0 );
			m_particlesPerSecond = m_particles.size() / m_cycleDuration;
			
			// This is a bit of a hack. m_startingParticles is necessarily used in postLoad() to set the number of particles.
			// But if you create a particleEmitter inside another object's postLoad() event, setting the max particles manually,
			// (i.e. by calling this function), then when postLoad() is automatically called for the emitter later (in loadManifest()),
			// then startParticles will be used again.
			// By forcing it to be the same as max particles, we disable this behavior in ParticleEmitter::postLoad().
			//
			m_startingParticles = particles;
		}
	}

	void ParticleEmitter::cycleTime( TimeType secondsToRespawnAllParticles )
	{
		m_cycleDuration = secondsToRespawnAllParticles;
		
		ASSERT( m_cycleDuration > 0 );
		m_particlesPerSecond = m_particles.size() / m_cycleDuration;
	}

	void ParticleEmitter::spatialBasis( DisplayObject::ptr basisObject )
	{
		m_spatialBasis = basisObject;
		
		if( !basisObject )
		{
			m_spatialBasis = this;
		}
	}

	void ParticleEmitter::spawnBurst( size_t nParticles /*= 0*/, bool doPauseAfterBurst /* = true */ )
	{
		REQUIRES( nParticles <= maxParticles() );
		
		if( nParticles == 0 )
		{
			nParticles = maxParticles();
		}

		// Spawn a bunch of particles immediately, all at the same time, with a delay afterward, so that there is a simultaneous burst of particles.
		
		TimeType now = stage().time();
		
		for( size_t i = 0; i < nParticles; ++i )
		{
			spawnParticle( now );
		}
		
		m_spawnTimeAccumulator = 0;
		
		if( doPauseAfterBurst )
		{
			pauseSpawning();
		}
	}

	void ParticleEmitter::warmUp()
	{
		// Pre-spawns and pre-simulates all the particles as if the emitter had already been operating for a full cycle.
		
		reset();
		m_isSpawningEnabled = true;

		const TimeType secondsPerFrame = stage().secondsPerFrame();
		int iUpdate = 0;
		
		while( m_iEndLiveParticles < m_particles.size() )
		{
			updateParticles( iUpdate * secondsPerFrame, false );
			
			++iUpdate;
		}
		
		TimeType lastSpawnTime = m_particles.back().spawnTime;
		
		// Adjust all particle birth times to reflect the present time.
		//
		const TimeType now = stage().time();
		const TimeType absoluteLastSpawnTime = now - lastSpawnTime;

		for( size_t i = 0; i < m_particles.size(); ++i )
		{
			m_particles[ i ].spawnTime += absoluteLastSpawnTime;
		}
		
		m_spawnTimeAccumulator = 0;
	}

	void ParticleEmitter::setParticleTextureByName( const std::string& textureName )
	{
		particleTexture( Renderer::instance().createTexture( textureName ));
	}

	void ParticleEmitter::spawnPositionArea( const Rectanglef& spawnArea, angle rectRotation /*= 0*/ )
	{
		m_rectSpawnArea = spawnArea;
		m_rectSpawnAreaRotation = rectRotation;
		m_circleSpawnAreaRadius = 0;
	}

	void ParticleEmitter::spawnPositionArea( const Vector2f& circleCenter, float radius )
	{
		REQUIRES( radius > 0 );
		m_circleSpawnAreaCenter = circleCenter;
		m_circleSpawnAreaRadius = radius;
	}

	void ParticleEmitter::velocityDamping( float damping )
	{
		m_velocityDamping = damping;
		
		m_twoMinusVelocityDamping = 2.0f - m_velocityDamping;
		m_oneMinusVelocityDamping = 1.0f - m_velocityDamping;
	}

	void ParticleEmitter::angularDamping( float damping )
	{
		m_angularDamping = damping;
		
		m_twoMinusAngularDamping = 2.0f - m_angularDamping;
		m_oneMinusAngularDamping = 1.0f - m_angularDamping;	
	}

	void ParticleEmitter::particleForce( const Vector2f& force )
	{
		m_particleForce = force;
	}

	size_t ParticleEmitter::numAttractorRepulsors() const
	{
		return m_attractorRepulsors.size();
	}
	
	void ParticleEmitter::addAttractorRepulsor( const Vector2f& location, float power, bool usesInverseDistance, float lateralDisplacement )
	{
		m_attractorRepulsors.emplace_back( AttractorRepulsor( location, power, usesInverseDistance, lateralDisplacement ));
	}
	
	void ParticleEmitter::removeAttractorRepulsor( size_t which )
	{
		REQUIRES( which < numAttractorRepulsors() );

		m_attractorRepulsors.erase( m_attractorRepulsors.begin() + which );
	}
	
	void ParticleEmitter::setAttractorRepulsor( size_t which, const Vector2f& location, float power, bool usesInverseDistance, float lateralDisplacement )
	{
		REQUIRES( which < numAttractorRepulsors() );
		
		m_attractorRepulsors[ which ] = AttractorRepulsor( location, power, usesInverseDistance, lateralDisplacement );
	}
	
	void ParticleEmitter::addColorKeyframe( float normalizedTime, Color color )
	{
		REQUIRES( 0 <= normalizedTime && normalizedTime <= 1.0 );
		m_vecKeyframesColor.push_back( std::make_pair( normalizedTime, color ));
		m_isVecKeyframesColorSorted = false;
	}

	void ParticleEmitter::addScaleKeyframe( float normalizedTime, const Vector2f& scale )
	{
		REQUIRES( 0 <= normalizedTime && normalizedTime <= 1.0 );
		m_vecKeyframesScale.push_back( std::make_pair( normalizedTime, scale ));
		m_isVecKeyframesScaleSorted = false;
	}

	void ParticleEmitter::clearColorKeyframes()
	{
		m_vecKeyframesColor.clear();
		m_isVecKeyframesColorSorted = false;
	}

	void ParticleEmitter::clearScaleKeyframes()
	{
		m_vecKeyframesScale.clear();
		m_isVecKeyframesScaleSorted = false;
	}

	rect ParticleEmitter::localBounds() const
	{
		rect bounds( 0, 0, 0, 0 );		// Minimal bounds.
		
		for( size_t iParticle = m_iBeginLiveParticles; iParticle < m_iEndLiveParticles; ++iParticle )
		{
			bounds.growToEncompass( m_particles[ iParticle % m_particles.size() ].position );
		}
		
		return bounds;
	}

	void ParticleEmitter::update()
	{
		DisplayObject::update();
		
		if( !doUpdate() )
		{
			return;
		}
		
		// To facilitate changing the number of particles from the command prompt, 
		// we keep listening to m_startingParticles. If it diverges from getMaxParticles(),
		// we change the number of particles to match.
		//
		if( m_startingParticles != maxParticles() )
		{
			maxParticles( m_startingParticles );
		}

		const TimeType now = stage().time();
		
		const bool shouldStepAnimations = numSubdivisions() > 1 && ( m_animFramesPerSecond > 0 && (( now - m_lastAnimStepTime ) >= ( 1.0 / m_animFramesPerSecond )));
		
		if( shouldStepAnimations )
		{
			m_lastAnimStepTime = now;
		}
		
		updateParticles( now, shouldStepAnimations );
		
		// If all the particles are dead, perhaps the emitter should kill itself.
		//
		if( m_destroyWhenAllDead && ( m_particles.size() == 0 || m_iBeginLiveParticles == m_iEndLiveParticles ))
		{
			markForDeletion();
		}
	}

	void ParticleEmitter::onAddedToStage()
	{
		Super::onAddedToStage();
		
		// Set values that need precalculation of secondary values.
		//
		angularDamping( m_angularDamping );
		velocityDamping( m_velocityDamping );
		particleForce( m_particleForce );
		
		// Apply m_startingParticle if needed.
		//
		if( m_startingParticles )
		{
			maxParticles( m_startingParticles );
		}
		
		// Warm up?
		//
		if( m_creationWarmup )
		{
			warmUp();
		}
		
		// Spawn initial burst?
		//
		if( m_creationSpawnBurst )
		{
			spawnBurst( 0, m_creationPauseAfterBurst );
		}
	}

	void ParticleEmitter::recordPreviousState( bool recursive )
	{
		m_previousParticles = m_particles;
		
		Super::recordPreviousState( recursive );
	}
				
	size_t ParticleEmitter::numLiveParticles() const
	{
		return m_iEndLiveParticles - m_iBeginLiveParticles;
	}

	namespace
	{
		inline bool compareTimeKeyframesColor( const std::pair< float, Color >& a, const std::pair< float, Color >& b )
		{
			return a.first < b.first;
		}	

		inline bool compareTimeKeyframesVector2f( const std::pair< float, Vector2f >& a, const std::pair< float, Vector2f >& b )
		{
			return a.first < b.first;
		}	
		
	}

	void ParticleEmitter::updateParticles( TimeType now, bool doStepAnimations )
	{
		// Ensure that the keyframe vectors are sorted.
		//
		if( !m_isVecKeyframesColorSorted )
		{
			std::sort( m_vecKeyframesColor.begin(), m_vecKeyframesColor.end(), compareTimeKeyframesColor );
			m_isVecKeyframesColorSorted = true;
		}
		if( !m_isVecKeyframesScaleSorted )
		{
			std::sort( m_vecKeyframesScale.begin(), m_vecKeyframesScale.end(), compareTimeKeyframesVector2f );
			m_isVecKeyframesScaleSorted = true;
		}
		
		// Update particle physics.
		//
		const TimeType deltaTime = stage().secondsPerFrame();
		const Vector2f forceTimesDeltaTimeSquared = m_particleForce * static_cast< float >( deltaTime * deltaTime );
		
		ASSERT( m_iBeginLiveParticles <= m_iEndLiveParticles );
		for( size_t iParticle = m_iBeginLiveParticles; iParticle < m_iEndLiveParticles; ++iParticle )
		{
			updateParticle( m_particles[ iParticle % m_particles.size() ], doStepAnimations, forceTimesDeltaTimeSquared );
		}
		
		// Spawn new particles.
		//
		if( m_isSpawningEnabled && m_particlesPerSecond > 0 )
		{
			const TimeType secondsPerSpawn = 1.0 / m_particlesPerSecond;
			
			m_spawnTimeAccumulator += stage().secondsPerFrame();
			while( m_spawnTimeAccumulator >= stage().secondsPerFrame() )
			{
				spawnParticle( now );
				
				m_spawnTimeAccumulator -= secondsPerSpawn;
			}
		}
	}

	void ParticleEmitter::draw( TimeType relativeFrameTime, RenderInjector* injector )
	{
		if( !( !injector || !injector->draw( relativeFrameTime, *this )))
		{
			return;
		}
		
		if( !m_particleTexture )
		{
			return;
		}
		
		if( m_iBeginLiveParticles == m_iEndLiveParticles )
		{
			// No living particles.
			//
			return;
		}
		
		Renderer& renderer = Renderer::instance();
		
		renderer.pushColor();
		m_storedBaseColor = renderer.getColorMultiply();
		
		renderer.applyTexture( m_particleTexture );
		
		if( blendMode() == Renderer::BlendMode::None )
		{
			renderer.setBlendMode( Renderer::getBlendModeForTextureAlphaUsage( m_particleTexture->alphaUsage() ));
		}

		int particleStep = -1;
		
		VertexBuffer::ptr vertexBuffer;
		if( m_renderMode == RenderMode::Sprite )
		{	
			vertexBuffer =  g_particleSpriteVertexBuffer;
		}
		else if( m_renderMode == RenderMode::Spark )
		{
			vertexBuffer =  g_particleSparkVertexBuffer;
		}
		
		const TimeType now = stage().time();
		
		if( m_previousParticles.size() != m_particles.size() )
		{
			m_previousParticles = m_particles;
		}

		const real deltaTime = static_cast< real >( relativeFrameTime );
				
		// Render in order from oldest to newest.
		//
		size_t iStartParticle = particleStep > 0 ? m_iBeginLiveParticles : m_iEndLiveParticles - 1;
		size_t iEndParticle = particleStep > 0 ? m_iEndLiveParticles : m_iBeginLiveParticles - 1;
		for( size_t iParticle = iStartParticle; iParticle != iEndParticle; iParticle += particleStep )
		{
			auto index = iParticle % m_particles.size();

			auto& priorParticle = m_previousParticles[ index ];
			auto& currentParticle = m_particles[ index ];
			
			// Just spawned?
			if( currentParticle.spawnTime > priorParticle.spawnTime )
			{
				priorParticle = currentParticle;
			}
			
			Particle particle = particleLerp( priorParticle, currentParticle, deltaTime );
			TimeType normalizedAge = ( now - particle.spawnTime ) / m_cycleDuration;
			
			// If particle is old, remove it.
			//
			if( normalizedAge > 1.0 )
			{
				++m_iBeginLiveParticles;
				continue;
			}
			
			ASSERT( 0 <= normalizedAge && normalizedAge <= 1.0 );
			normalizedAge = clamp( normalizedAge, (TimeType) 0.0, (TimeType) 1.0 );
			
			// Develop particle color.
			//
			Color color = getKeyframedColor( normalizedAge );
			
			// Modulate with flicker color.
			Color flickerColor = lerp( m_perFrameColorFlickerRange.min, m_perFrameColorFlickerRange.max, randInRange( 0.0f, 1.0f ));
			color = color.modulate( flickerColor );

			// Modulate with per-particle color.
			color = color.modulate( particle.color );
			
			// Don't render particles with 0 alpha.
			//
			if( color.getA() == 0 )
			{
				continue;
			}
			
			Vector2f scale = particle.scale * getKeyframedScale( normalizedAge );

			vec2 particlePosition = particle.position;
			
			// Transform into spatial basis object's space.
			//
			auto basis = spatialBasis();
			
			if( basis != this )
			{
				particlePosition = basis->localToGlobal( particlePosition );
				particlePosition = globalToLocal( particlePosition );
			}
			
			if( m_renderMode == RenderMode::Sprite )
			{
				angle particleRotation = lerp( particle.lastRotation, particle.rotation, relativeFrameTime );;
				
				if( basis != this )
				{
					particleRotation = basis->localToGlobal( particleRotation );
					particleRotation = globalToLocal( particleRotation );
				}
				
				renderer.color( color );
				
				// Setup texture transform if animating.
				//
				const bool isAnimatingTexCoords = m_textureSubdivisionSize.x != 1.0f || m_textureSubdivisionSize.y != 1.0f;
				
				if( isAnimatingTexCoords )
				{
					uint subdivisionX, subdivisionY;
					subdivisionCoords( particle.animFrame, subdivisionX, subdivisionY );
					
					renderer.pushMatrix( Renderer::MAT_Texture );
					renderer.scale( m_textureSubdivisionSize, Renderer::MAT_Texture );
					renderer.translate( float( subdivisionX ), float( subdivisionY ), Renderer::MAT_Texture );
				}
				
				renderer.pushMatrix();
				
				renderer.translate( particlePosition );
				renderer.scale( scale );
				renderer.rotate( particleRotation );

				renderer.updateUniformsForCurrentShaderProgram( this );
				renderer.drawGeometry( Renderer::PrimitiveType::TriangleStrip, vertexBuffer, 4 );

				renderer.popMatrix();
				
				if( isAnimatingTexCoords )
				{
					renderer.popMatrix( Renderer::MAT_Texture );
				}
			}
			else if( m_renderMode == RenderMode::Spark )
			{
				vec2 lastParticlePosition( particle.lastPosition.x, particle.lastPosition.y );
				
				if( basis != this )
				{
					lastParticlePosition = basis->localToGlobal( lastParticlePosition );
					lastParticlePosition = globalToLocal( lastParticlePosition );
				}
				
				const vec2 particleDelta = particlePosition - lastParticlePosition;
				
				angle particleRotation = particleDelta.angle() - angle( 90.0f /* degrees */ );
				
				scale.y = std::max( m_sparkMinTailLength, particleDelta.length() * m_sparkTailScalar );
				
				renderer.color( color );
				
				renderer.pushMatrix();
				
				renderer.translate( particlePosition );
				renderer.rotate( particleRotation );
				renderer.scale( scale );
				
				renderer.updateUniformsForCurrentShaderProgram( this );
				renderer.drawGeometry( Renderer::PrimitiveType::TriangleStrip, vertexBuffer, 4 );
				renderer.popMatrix();
			}
		}
		
		renderer.popColor();
	}
	
	DisplayObject::wptr ParticleEmitter::spatialBasis() const
	{
		if( m_spatialBasis )
		{
			return m_spatialBasis;
		}
		else
		{
			return const_cast< ParticleEmitter* >( this );
		}
	}

	Vector2f ParticleEmitter::getRandomSpawnPosition() const
	{
		vec2 basisPosition = vec2::ZERO;
		
		auto basis = spatialBasis();
		
		if( basis != this )
		{
			basisPosition = localToGlobal( basisPosition );
			basisPosition = basis->globalToLocal( basisPosition );
		}
		
		if( m_circleSpawnAreaRadius <= 0 )
		{
			Vector2f spawnPosition( randInRange( m_rectSpawnArea.left(), m_rectSpawnArea.right() ), 
									randInRange( m_rectSpawnArea.top(), m_rectSpawnArea.bottom() ));
			
			spawnPosition.rotate( m_rectSpawnAreaRotation );
			
			return basisPosition + spawnPosition;
		}
		else
		{
			float randomAngle = randInRange( 0.0f, (float) TWO_PI );
			float randomRadius = randInRange( 0.0f, m_circleSpawnAreaRadius );
			return Vector2f( basisPosition.x + cos( randomAngle ) * randomRadius + m_circleSpawnAreaCenter.x, 
							 basisPosition.y + sin( randomAngle ) * randomRadius + m_circleSpawnAreaCenter.y );
		}
	}

	Vector2f ParticleEmitter::getRandomSpawnVelocity() const
	{
		float randomAngleRadians = degreesToRadians( randInRange( m_spawnVelocityAngleRange.min, m_spawnVelocityAngleRange.max ) );
		float randomSpeed = randInRange( m_spawnSpeedRange.min, m_spawnSpeedRange.max );
		return m_spawnBaseVelocity + Vector2f( std::cos( randomAngleRadians ) * randomSpeed, std::sin( randomAngleRadians ) * randomSpeed );
	}

	float ParticleEmitter::getRandomSpawnRotation() const
	{
		return randInRange( m_spawnRotationRange.min, m_spawnRotationRange.max );
	}

	float ParticleEmitter::getRandomSpawnAngularVelocity() const
	{
		return randInRange( m_spawnAngularVelocityRange.min, m_spawnAngularVelocityRange.max );
	}

	Vector2f ParticleEmitter::getRandomSpawnScale() const
	{
		return randInRange( m_spawnScaleRange.min, m_spawnScaleRange.max );
	}

	void ParticleEmitter::spawnParticle( TimeType birthDate )
	{	
		if( m_particles.empty() )
		{
			return;
		}
		
		Particle& particle = m_particles[ m_iEndLiveParticles % m_particles.size() ];
		
		particle.position = getRandomSpawnPosition();
		particle.lastPosition = particle.position - getRandomSpawnVelocity();
		particle.rotation = getRandomSpawnRotation();
		particle.lastRotation = particle.rotation - getRandomSpawnAngularVelocity();
		particle.scale = getRandomSpawnScale();
		particle.spawnTime = birthDate;
		particle.color = lerp( m_perParticleColorRange.min, m_perParticleColorRange.max, randInRange( 0.0f, 1.0f ));
		particle.animFrame = 0;
		
		if( m_doRandomizeAnimStart || !doAnimateParticles() )
		{
			particle.animFrame = randInRange( 0U, numSubdivisions() );
		}
		
		++m_iEndLiveParticles;
		
		// Old particles should have died so that an adequately small number remains.
		//	
		size_t iNotionalBegin = m_iEndLiveParticles - m_particles.size();
		
		if( iNotionalBegin < m_iEndLiveParticles )	// Detect unsigned underflow.
		{
			m_iBeginLiveParticles = std::max( m_iBeginLiveParticles, iNotionalBegin );
		}
		
		ASSERT( m_iBeginLiveParticles <= m_iEndLiveParticles );
	}

	void ParticleEmitter::addAttractorForce( const AttractorRepulsor& attractor, Particle& particle )
	{
		Vector2f delta = attractor.location - particle.position;
		
		if( attractor.lateralDisplacement != 0 )
		{
			auto normal = delta * attractor.lateralDisplacement;
			normal.quickRot90();
			delta += normal;
		}
		
		float distSquared = delta.lengthSquared();
		float dist = std::sqrt( distSquared );
		
		if( dist > 0 )
		{
			Vector2f force;
			if( attractor.doesUseInverseDistance )
			{
				force = ( delta / dist ) * ( attractor.power / distSquared );	// Gravitational (inverse) physics: weaker force as distance grows
			}
			else
			{
				force = delta * attractor.power;								// Rubberband physics: greater force as distance grows.
			}
			particle.lastPosition -= force;
		}
	}

	void ParticleEmitter::forEachParticle( std::function< bool( Particle& ) >&& fnPerParticle )
	{
		ASSERT( m_iBeginLiveParticles <= m_iEndLiveParticles );
		for( size_t iParticle = m_iBeginLiveParticles; iParticle < m_iEndLiveParticles; ++iParticle )
		{
			const auto index = iParticle % m_particles.size();
			auto& particle = m_particles[ index ];
			
			bool teleported = fnPerParticle( particle );
			
			if( teleported && m_previousParticles.size() > index )
			{
				m_previousParticles[ index ] = particle;
			}
		}
	}
	
	void ParticleEmitter::updateParticle( Particle& particle, bool doStepAnimation, const Vector2f& forceTimesDeltaTimeSquared )
	{
		for( const auto& attractor : m_attractorRepulsors )
		{
			addAttractorForce( attractor, particle );
		}
		
		// Verlet integration.
		//
		const Vector2f storedPosition = particle.position;
		particle.position = storedPosition * m_twoMinusVelocityDamping - particle.lastPosition * m_oneMinusVelocityDamping + forceTimesDeltaTimeSquared;
		particle.lastPosition = storedPosition;
		
		const real storedRotation = particle.rotation;
		particle.rotation = storedRotation * m_twoMinusAngularDamping - particle.lastRotation * m_oneMinusAngularDamping;
		particle.lastRotation = storedRotation;
		
		// Texture subdivision animation.
		//
		if( doStepAnimation )
		{
			particle.animFrame = ( particle.animFrame + 1 ) % numSubdivisions();
		}
	}

	Color ParticleEmitter::getKeyframedColor( TimeType normalizedTime ) const
	{
		if( m_vecKeyframesColor.empty() )
		{
			return m_storedBaseColor;
		}
		
		Color keyframeColor;
		getKeyframedValue( m_vecKeyframesColor.begin(), m_vecKeyframesColor.end(), normalizedTime, keyframeColor );
		return m_storedBaseColor.modulate( keyframeColor );
	}

	Vector2f ParticleEmitter::getKeyframedScale( TimeType normalizedTime ) const
	{
		if( m_vecKeyframesScale.empty() )
		{
			return m_baseScale;
		}
		
		Vector2f scale;
		getKeyframedValue( m_vecKeyframesScale.begin(), m_vecKeyframesScale.end(), normalizedTime, scale );
		return m_baseScale * scale;
	}

	bool ParticleEmitter::hitTestPoint( const vec2& localLocation, HitTestFlags flags ) const
	{
		if( rejectsOnTheBasisOfHitFlags( flags ))
		{
			return false;
		}
		
		if( !hitTestMask( localLocation, flags ))
		{
			return false;
		}
		
		// Do our bounds touch the location?
		if( localBounds().doesEnclose( localLocation, true ))
		{
			return true;
		}
		
		return false;
	}

	ParticleEmitter::Particle ParticleEmitter::particleLerp( const Particle& a, const Particle& b, real alpha )
	{
		return Particle{
			lerp( a.position, b.position, alpha ),
			lerp( a.lastPosition, b.lastPosition, alpha ),
			lerp( a.rotation, b.rotation, alpha ),
			lerp( a.lastRotation, b.lastRotation, alpha ),
			lerp( a.scale, b.scale, alpha ),
			b.spawnTime,
			a.color.lerp( b.color, alpha ),
			b.animFrame
		};
	}
	
}

