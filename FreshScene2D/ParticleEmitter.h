/*
 *  ParticleEmitter.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 7/24/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_PARTICLE_EMITTER_H_INCLUDED
#define FRESH_PARTICLE_EMITTER_H_INCLUDED

#include "DisplayObject.h"

namespace fr
{
	
	class Texture;	
	
	class ParticleEmitter : public DisplayObject
	{
		FRESH_DECLARE_CLASS( ParticleEmitter, DisplayObject )
		
	public:
		
		enum class RenderMode
		{
			Sprite,
			Spark,
		};
			
		//
		// Definitely call these if you want the emitter to do anything.
		//
		
		void maxParticles( size_t particles );
		size_t maxParticles() const													{ return m_particles.size(); }
		void cycleTime( TimeType secondsToRespawnAllParticles );		// The number of seconds (as defined by Stage::time()) necessary to respawn a complete cycle of particles as defined by maxParticles()
			// Note that in any case, the current system will not produce more than one particle per frame.
		size_t numLiveParticles() const;
		
		SYNTHESIZE( SmartPtr< Texture >, particleTexture );
		void setParticleTextureByName( const std::string& textureName );
		
		//
		// All the following functions are optional.
		//
		
		void spawnBurst( size_t nParticles = 0, bool doPauseAfterBurst = true );
			// REQUIRES( nParticles <= getMaxParticles() );
			// If nParticles == 0, getMaxParticles() will be spawned.
		
		void warmUp();
			// Pre-spawns all the particles as if the emitter had already been operating for a full cycle.

		void spatialBasis( DisplayObject::ptr basisObject );
			// Sets the object that will be used as the spatial basis for the particles' transformations.
			// By default this is *this*.
			// If basisObject == 0, the basis will be set to *this*.

		void startSpawning();
		void pauseSpawning();
		bool isSpawningPaused();
		void setMarkForDeletionWhenAllParticlesDead( bool doMark );
		
		void reset();		// Kills all particles and starts over.
		
		SYNTHESIZE( RenderMode, renderMode );
		SYNTHESIZE( float, sparkTailScalar );
		SYNTHESIZE( float, sparkMinTailLength );
		
		SYNTHESIZE( Vector2f, baseScale );
		
		void spawnPositionArea( const Rectanglef& spawnArea, angle rectRotation = angle( 0U ) );
		void spawnPositionArea( const Vector2f& circleCenter, float radius );
		// REQUIRES( radius > 0 );
		
		SYNTHESIZE( Vector2f,			spawnBaseVelocity );
		SYNTHESIZE( Range< float >,		spawnVelocityAngleRange );
		SYNTHESIZE( Range< float >,		spawnSpeedRange );
		SYNTHESIZE( Range< float >,		spawnRotationRange );
		SYNTHESIZE( Range< float >,		spawnAngularVelocityRange );
		SYNTHESIZE( Range< Vector2f >,	spawnScaleRange );
		SYNTHESIZE( Range< Color >,		perFrameColorFlickerRange );
		SYNTHESIZE( Range< Color >,		perParticleColorRange );
		
		SYNTHESIZE_GET( float, velocityDamping );
		void velocityDamping( float damping );
		SYNTHESIZE_GET( float, angularDamping );
		void angularDamping( float damping );
		
		void particleForce( const Vector2f& force );

		// Attractor-Repulsors.
		// power > 0 for attractor, < 0 for repulsor, == 0 for disabled.
		//
		size_t numAttractorRepulsors() const;
		void addAttractorRepulsor( const Vector2f& location, float power, bool usesInverseDistance = false, float lateralDisplacement = 0 );
		void removeAttractorRepulsor( size_t which );
		// REQUIRES( which < numAttractorRepulsors() );
		void setAttractorRepulsor( size_t which, const Vector2f& location, float power, bool usesInverseDistance = false, float lateralDisplacement = 0 );
		// REQUIRES( which < numAttractorRepulsors() );

		// Color/scale keyframes
		//
		void addColorKeyframe( float normalizedTime, Color color );
			// REQUIRES( 0 <= normalizedTime && normalizedTime <= 1.0 );
		void addScaleKeyframe( float normalizedTime, const Vector2f& scale );
		void addScaleKeyframe( float normalizedTime, float scale )					{ addScaleKeyframe( normalizedTime, Vector2f( scale, scale )); }
			// REQUIRES( 0 <= normalizedTime && normalizedTime <= 1.0 );
		
		void clearColorKeyframes();
		void clearScaleKeyframes();
		
		void setPerFrameColorFlickerRange( Color min, Color max );
		
		virtual rect localBounds() const override;

		virtual void update() override;
		
		virtual bool hitTestPoint( const vec2& localLocation, HitTestFlags flags ) const override;
		
		virtual void onAddedToStage() override;
		
		virtual void recordPreviousState( bool recursive = false ) override;
		
	protected:
		
		struct Particle
		{
			Vector2f			position;
			Vector2f			lastPosition;
			float				rotation = 0;
			float				lastRotation = 0;
			Vector2f			scale;
			TimeType			spawnTime = -1.0;
			Color				color;
			uint				animFrame = 0;
			
			Particle() {}
			
			Particle( const Vector2f& p,
					  const Vector2f& lp,
					  float r,
					  float lr,
					  const Vector2f& s,
					  TimeType st,
					  Color c,
					  uint af )
			:	position( p )
			,	lastPosition( lp )
			,	rotation( r )
			,	lastRotation( lr )
			,	scale( s )
			,	spawnTime( st )
			,	color( c )
			,	animFrame( af )
			{}
		};
		
		struct AttractorRepulsor : public SerializableStruct< AttractorRepulsor >
		{
			Vector2f	location;
			float		power;
			bool		doesUseInverseDistance;
			float		lateralDisplacement;
			
			explicit AttractorRepulsor( const Vector2f& loc = Vector2f::ZERO, float pow = 0, bool invert = false, float displacement = 0 )
			:	location( loc )
			,	power( pow )
			,	doesUseInverseDistance( invert )
			,	lateralDisplacement( displacement )
			{
				STRUCT_BEGIN_PROPERTIES
				STRUCT_ADD_PROPERTY( location )
				STRUCT_ADD_PROPERTY( power )
				STRUCT_ADD_PROPERTY( doesUseInverseDistance )
				STRUCT_ADD_PROPERTY( lateralDisplacement )
				STRUCT_END_PROPERTIES
			}
			
			bool operator==( const AttractorRepulsor& other ) const
			{
				return location == other.location &&
				power == other.power &&
				doesUseInverseDistance == other.doesUseInverseDistance &&
				lateralDisplacement == other.lateralDisplacement;
			}
		};
		
		STRUCT_DECLARE_SERIALIZATION_OPERATORS( AttractorRepulsor )
		
		typedef std::vector< Particle > Particles;
		
		void forEachParticle( std::function< bool( Particle& ) >&& fnPerParticle );
	
		virtual void updateParticles( TimeType now, bool doStepAnimations );
		
		virtual void draw( TimeType relativeFrameTime, RenderInjector* injector = nullptr ) override;
		
		DisplayObject::wptr spatialBasis() const;
		
		Vector2f getRandomSpawnPosition() const;
		Vector2f getRandomSpawnVelocity() const;
		float getRandomSpawnRotation() const;
		float getRandomSpawnAngularVelocity() const;
		Vector2f getRandomSpawnScale() const;
		void spawnParticle( TimeType birthDate );
		
		Color getKeyframedColor( TimeType normalizedTime ) const;
		Vector2f getKeyframedScale( TimeType normalizedTime ) const;
		
		void normalizeIndices();
			// Ensures that all particle indices fit into the range (0, m_particles.size()).

		// Texture subdivisions
		//
		uint numSubdivisions() const;
		uint subdivision( uint x, uint y ) const;
		void subdivisionCoords( uint subdivision, uint& outX, uint& outY ) const;
		bool doAnimateParticles() const;
		
	private:
		
		typedef std::vector< std::pair< float, Color > > VecColorKeyframes;
		typedef VecColorKeyframes::iterator VecColorKeyframesI;
		typedef VecColorKeyframes::const_iterator VecColorKeyframesCI;
		
		typedef std::vector< std::pair< float, Vector2f > > VecVector2Keyframes;
		typedef VecVector2Keyframes::iterator VecVector2KeyframesI;
		typedef VecVector2Keyframes::const_iterator VecVector2KeyframesCI;


		DVAR( RenderMode, m_renderMode, RenderMode::Sprite );

		VAR( DisplayObject::wptr, m_spatialBasis );
		
		Particles					m_particles;
		Particles					m_previousParticles;
		size_t		 				m_iBeginLiveParticles = 0;
		size_t		 				m_iEndLiveParticles = 0;			// 1 past the last live particle.
		
		bool						m_isVecKeyframesColorSorted = false;
		bool						m_isVecKeyframesScaleSorted = false;
		
		mutable Color				m_storedBaseColor = Color::White;
		
		TimeType					m_spawnTimeAccumulator = 0;
		
		DVAR( TimeType,			m_cycleDuration, 1.0 );
		DVAR( TimeType,			m_particlesPerSecond, 0 );
		DVAR( float,				m_sparkTailScalar, 1.0f );
		DVAR( float,				m_sparkMinTailLength, 0 );
		
		VAR( VecColorKeyframes, 	m_vecKeyframesColor );
		VAR( VecVector2Keyframes, 	m_vecKeyframesScale );
		
		DVAR( bool, 				m_isSpawningEnabled, true );
		DVAR( bool,				m_destroyWhenAllDead, false );
		
		// Exactly one of these two areas are used. Rect is used iff m_circleSpawnAreaRadius <= 0
		//
		DVAR( Rectanglef,			m_rectSpawnArea, Rectanglef( 0, 0, 0, 0 ));
		DVAR( angle,				m_rectSpawnAreaRotation, 0U );
		VAR( Vector2f,				m_circleSpawnAreaCenter);
		DVAR( float,				m_circleSpawnAreaRadius, 0 );
		
		VAR( SmartPtr< Texture >,	m_particleTexture );

		DVAR( Vector2f,			m_baseScale, Vector2f( 1.0f ) );
		
		VAR( Vector2f,				m_particleForce );
		
		VAR( Vector2f,				m_spawnBaseVelocity );
		DVAR( Range< float >,		m_spawnVelocityAngleRange, Range< float >( 0.0f, 360.0f ));
		VAR( Range< float >,		m_spawnSpeedRange );
		VAR( Range< float >,		m_spawnRotationRange );
		VAR( Range< float >,		m_spawnAngularVelocityRange );
		DVAR( Range< Vector2f >,	m_spawnScaleRange, Range< Vector2f >( Vector2f( 1.0f, 1.0f ), Vector2f( 1.0f, 1.0f )));
		DVAR( float, 				m_velocityDamping, 0 );
		float										m_twoMinusVelocityDamping = 0;
		float										m_oneMinusVelocityDamping = 0;
		DVAR( float,				m_angularDamping, 0 );
		float										m_twoMinusAngularDamping = 0;
		float										m_oneMinusAngularDamping = 0;
		
		DVAR( Range< Color >,		m_perFrameColorFlickerRange, Range< Color >( Color::White, Color::White ));
		DVAR( Range< Color >,		m_perParticleColorRange, Range< Color >( Color::White, Color::White ) );
		
		VAR( std::vector< AttractorRepulsor >, m_attractorRepulsors );

		// Support for multi-frame and animated particles.
		//
		DVAR( Vector2f,			m_textureSubdivisionSize, Vector2f( 1.0f, 1.0f ));	// In texture coordinates.
		DVAR( uint,				m_textureSubdivisionsX, 1 );
		DVAR( uint,				m_textureSubdivisionsY, 1 );
		DVAR( TimeType,			m_animFramesPerSecond, 12.0f );	// If > 0, all particles animate through the texture subdivisions at this frame rate
																					// Else, each particle chooses a random subdivision and shows it throughout its lifetime.
		DVAR( bool,				m_doRandomizeAnimStart, true );	// Only relevant if m_animFramesPerSecond > 0. Iff true, each particle starts with a random subdivision.
		
		TimeType m_lastAnimStepTime = -1;
			 
		// Creation parameters
		//
		DVAR( size_t,				m_startingParticles, 5 );
		DVAR( bool,				m_creationWarmup, false );
		DVAR( bool,				m_creationSpawnBurst, false );
		DVAR( bool,				m_creationPauseAfterBurst, false );	// Irrelevant if !m_creationSpawnBurst
		
		void updateParticle( Particle& particle, bool doStepAnimation, const Vector2f& forceTimesDeltaTimeSquared );
		void addAttractorForce( const AttractorRepulsor& attractor, Particle& particle );
		
		static Particle particleLerp( const Particle& a, const Particle& b, real alpha );

		
	};
	
	FRESH_ENUM_STREAM_IN_BEGIN( ParticleEmitter, RenderMode )
	FRESH_ENUM_STREAM_IN_CASE( ParticleEmitter::RenderMode, Sprite )
	FRESH_ENUM_STREAM_IN_CASE( ParticleEmitter::RenderMode, Spark )
	FRESH_ENUM_STREAM_IN_END()
	
	FRESH_ENUM_STREAM_OUT_BEGIN( ParticleEmitter, RenderMode )
	FRESH_ENUM_STREAM_OUT_CASE( ParticleEmitter::RenderMode, Sprite )
	FRESH_ENUM_STREAM_OUT_CASE( ParticleEmitter::RenderMode, Spark )
	FRESH_ENUM_STREAM_OUT_END()	
}

#endif
