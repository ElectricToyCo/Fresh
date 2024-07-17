/*
 *  AudioSound.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 8/12/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#include "AudioSound.h"
#include "Objects.h"
#include "Property.h"
#include "FreshOpenAL.h"
#include "AudioSystem.h"
#include "AudioSystemImpl.h"

#if DEV_MODE && 0
#	define audio_trace(x) release_trace(x)
#else
#	define audio_trace(x)
#endif

namespace
{
	// OpenAL's linear clamped attenuation model does weird things, including ignoring ROLLOFF_FACTOR.
	// Distances seem to be interpreted wrongly--behaving effectively as if they were about half of what they actually are.
	// Scale all distances to compensate.
	const float OVERALL_ATTENUATION_DISTANCE_SCALAR = 2.0f;
}

namespace fr
{

	FRESH_DEFINE_CLASS( SoundGroup )
	DEFINE_VAR( SoundGroup, float, m_gain );
	DEFINE_VAR( SoundGroup, float, m_gainScalar );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( SoundGroup )

	void SoundGroup::gain( float gain_ )
	{
		FRESH_AUDIO_SYNCHRONIZE
		if( m_gain != gain_ )
		{
			m_gain = gain_;
			applyGain();
		}
	}

	void SoundGroup::gainScalar( float gainScalar_ )
	{
		FRESH_AUDIO_SYNCHRONIZE
		if( m_gainScalar != gainScalar_ )
		{
			m_gainScalar = gainScalar_;
			applyGain();
		}
	}

	void SoundGroup::applyGain()
	{
		FRESH_AUDIO_SYNCHRONIZE
		cleanupChildren();		// Maintain the sound list.

		for( auto child : m_children )
		{
			ASSERT( child );
			child->gainScalar( m_gain * m_gainScalar );
		}
	}

	bool SoundGroup::hasChild( SoundGroup::cptr child ) const
	{
		FRESH_AUDIO_SYNCHRONIZE
		REQUIRES( child );
		return m_children.end() != std::find( m_children.begin(), m_children.end(), child );
	}

	void SoundGroup::addChild( SoundGroup::ptr child )
	{
		FRESH_AUDIO_SYNCHRONIZE
		REQUIRES( child );
		cleanupChildren();		// Maintain the sound list.

		REQUIRES( !hasChild( child ));
		REQUIRES( !child->parentGroup() );	// Shouldn't have a parent.

		m_children.push_back( child );
		child->parentGroup( this );
		child->gainScalar( m_gain * m_gainScalar );

		PROMISES( child->parentGroup() == this );
		PROMISES( hasChild( child ));
	}

	void SoundGroup::removeChild( SoundGroup::ptr child )
	{
		FRESH_AUDIO_SYNCHRONIZE
		REQUIRES( child );
		REQUIRES( hasChild( child ));
		REQUIRES( child->parentGroup() == this );

		child->parentGroup( nullptr );
		m_children.remove( child );

		PROMISES( !hasChild( child ));
	}

	SoundGroup::Children::iterator SoundGroup::begin()
	{
		FRESH_AUDIO_SYNCHRONIZE
		return m_children.begin();
	}

	SoundGroup::Children::iterator SoundGroup::end()
	{
		FRESH_AUDIO_SYNCHRONIZE
		return m_children.end();
	}

	void SoundGroup::cleanupChildren()
	{
		FRESH_AUDIO_SYNCHRONIZE
		// Remove null sound pointers.
		//
#if 1
		m_children.remove_if( std::mem_fn( &WeakPtr< SoundGroup >::isNull ));
#else

		int nChildrenRemoved = 0;
		for( auto iter = m_children.begin(); iter != m_children.end(); /* increment within */ )
		{
			if( !*iter )
			{
				iter = m_children.erase( iter );
				++nChildrenRemoved;
			}
			else
			{
				++iter;
			}
		}

		if( nChildrenRemoved > 0 )
		{
			audio_trace( this << " deleted " << nSoundsRemoved << " sounds." );
		}
#endif
	}

	/////////////////////////////////////////////////////////////////////////////////////

	FRESH_DEFINE_CLASS( Sound )

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( Sound )

	Sound::Sound( const ClassInfo& assignedClassInfo, NameRef name )
	:	Super( assignedClassInfo, name )
	{
		FRESH_AUDIO_SYNCHRONIZE

#if !FRESH_NULL_AUDIO

		if( !AudioSystem::ready() ) return;

		alGenSources( 1, &m_idALSource );

		usesListenerRelativePosition( true );		// By default use relative positioning so that if no position is specified, the sound plays fully.

		Range< float > range = AudioSystem::instance().getDefaultAttenuationRange();

		maxUnattenuatedDistance( range.min );		// Not a virtual call, you realize.
		maxAudibleDistance( range.max );			// Not a virtual call either.

		HANDLE_AL_ERRORS();

		ASSERT( m_idALSource > 0 );
#endif
	}

	Sound::~Sound()
	{
		FRESH_AUDIO_SYNCHRONIZE

#if !FRESH_NULL_AUDIO
		if( m_idALSource )
		{
			alDeleteSources( 1, &m_idALSource );
			m_idALSource = 0;
		}
#endif
	}

	void Sound::cue( SmartPtr< AudioCue > cue_ )
	{
		FRESH_AUDIO_SYNCHRONIZE
		REQUIRES( cue_ );
		ASSERT( !m_cue );
		m_cue = cue_;
	}

	SmartPtr< AudioCue > Sound::cue() const
	{
		FRESH_AUDIO_SYNCHRONIZE
		return m_cue;
	}

	void Sound::enqueueBuffer( uint cueBuffer )
	{
		FRESH_AUDIO_SYNCHRONIZE
#if !FRESH_NULL_AUDIO
		ASSERT( m_idALSource > 0 );
		ASSERT( cueBuffer );

		m_hasBuffers = true;
		alSourceQueueBuffers( m_idALSource, 1, &cueBuffer );
		HANDLE_AL_ERRORS();

		if( m_pendingPlay )
		{
			play();
		}
#endif
	}

	void Sound::enqueueBuffers( const uint* cueBuffers, size_t nBuffers )
	{
		FRESH_AUDIO_SYNCHRONIZE
#if !FRESH_NULL_AUDIO
		ASSERT( m_idALSource > 0 );

		m_hasBuffers = true;
		alSourceQueueBuffers( m_idALSource, static_cast< ALsizei >( nBuffers ), cueBuffers );
		HANDLE_AL_ERRORS();

		if( m_pendingPlay )
		{
			play();
		}
#endif
	}

	void Sound::play()
	{
		FRESH_AUDIO_SYNCHRONIZE

#if !FRESH_NULL_AUDIO
		if( m_hasBuffers )
		{
			alSourcePlay( m_idALSource );
			HANDLE_AL_ERRORS();
		}
		m_pendingPlay = !m_hasBuffers;
#endif
	}

	void Sound::pause()
	{
		FRESH_AUDIO_SYNCHRONIZE
#if !FRESH_NULL_AUDIO
		m_pendingPlay = false;
		alSourcePause( m_idALSource );
		HANDLE_AL_ERRORS();
#endif
	}

	void Sound::stop()
	{
		FRESH_AUDIO_SYNCHRONIZE
#if !FRESH_NULL_AUDIO
		m_pendingPlay = false;
		alSourceStop( m_idALSource );
		HANDLE_AL_ERRORS();
#endif
	}

	bool Sound::isPlaying() const
	{
		FRESH_AUDIO_SYNCHRONIZE
#if !FRESH_NULL_AUDIO
		ALint sourceState;
		alGetSourcei( m_idALSource, AL_SOURCE_STATE, &sourceState );

		return m_pendingPlay || sourceState == AL_PLAYING;
#else
		return false;
#endif
	}

	TimeType Sound::getPlayHeadSeconds() const
	{
		FRESH_AUDIO_SYNCHRONIZE
#if !FRESH_NULL_AUDIO
		ALfloat playhead;
		alGetSourcef( m_idALSource, AL_SEC_OFFSET, &playhead );
		return TimeType( playhead );
#else
		return 0;
#endif
	}

	void Sound::looping( bool doLoop )
	{
		FRESH_AUDIO_SYNCHRONIZE
#if !FRESH_NULL_AUDIO
		alSourcei( m_idALSource, AL_LOOPING, doLoop ? AL_TRUE : AL_FALSE );
		HANDLE_AL_ERRORS();
#endif
	}

	bool Sound::looping() const
	{
		FRESH_AUDIO_SYNCHRONIZE
#if !FRESH_NULL_AUDIO
		ALint sourceState;
		alGetSourcei( m_idALSource, AL_LOOPING, &sourceState );

		return sourceState == AL_TRUE;
#else
		return false;
#endif
	}

	void Sound::applyGain()
	{
		FRESH_AUDIO_SYNCHRONIZE
#if !FRESH_NULL_AUDIO
		const float cumulativeGain = gain() * gainScalar();
		ASSERT( 0.0f <= cumulativeGain && cumulativeGain <= 1.0f );

		alSourcef( m_idALSource, AL_GAIN, cumulativeGain );
		HANDLE_AL_ERRORS();

		Super::applyGain();
#endif
	}

	void Sound::pitchScalar( float scale )
	{
		FRESH_AUDIO_SYNCHRONIZE
#if !FRESH_NULL_AUDIO
		REQUIRES( scale > 0 );

		alSourcef( m_idALSource, AL_PITCH, scale );
		HANDLE_AL_ERRORS();
#endif
	}

	float Sound::pitchScalar() const
	{
		FRESH_AUDIO_SYNCHRONIZE
#if !FRESH_NULL_AUDIO
		float result = 0;
		alGetSourcef( m_idALSource, AL_PITCH, &result );

		PROMISES( result > 0 );

		return result;
#endif
	}

	bool Sound::usesListenerRelativePosition() const
	{
		FRESH_AUDIO_SYNCHRONIZE
#if !FRESH_NULL_AUDIO
		int result = false;
		alGetSourcei( m_idALSource, AL_SOURCE_RELATIVE, &result );
		return result == AL_TRUE;
#else
		return true;
#endif
	}

	void Sound::usesListenerRelativePosition( bool uses )
	{
		FRESH_AUDIO_SYNCHRONIZE
#if !FRESH_NULL_AUDIO && !FRESH_EMSCRIPTEN
		alSourcei( m_idALSource, AL_SOURCE_RELATIVE, uses ? AL_TRUE : AL_FALSE );
		HANDLE_AL_ERRORS();
#endif
	}

	vec3 Sound::position() const
	{
		FRESH_AUDIO_SYNCHRONIZE
#if !FRESH_NULL_AUDIO
		vec3 result;
		alGetSourcefv( m_idALSource, AL_POSITION, result );
		return result;
#else
		return vec3{};
#endif
	}

	void Sound::position( const vec3& pos )
	{
		FRESH_AUDIO_SYNCHRONIZE
#if !FRESH_NULL_AUDIO && !FRESH_EMSCRIPTEN
		if( !pos.isZero() )
		{
			usesListenerRelativePosition( false );				// Force the position to be absolute. It's only relative by default to avoid attenuation for default sounds.
		}
		alSourcefv( m_idALSource, AL_POSITION, pos );
		HANDLE_AL_ERRORS();

		PROMISES( pos.isZero() || usesListenerRelativePosition() == false );
		PROMISES( position() == pos );
#endif
	}

	float Sound::maxAudibleDistance() const
	{
		FRESH_AUDIO_SYNCHRONIZE
#if !FRESH_NULL_AUDIO
		float result = 0;
		alGetSourcef( m_idALSource, AL_MAX_DISTANCE, &result );
		result /= OVERALL_ATTENUATION_DISTANCE_SCALAR;
		return result;
#else
		return 0;
#endif
	}

	void Sound::maxAudibleDistance( float dist ) const
	{
		FRESH_AUDIO_SYNCHRONIZE
#if !FRESH_NULL_AUDIO && !FRESH_EMSCRIPTEN
		if( !AudioSystem::ready() ) return;

		if( dist < 0 )
		{
			Range< float > range = AudioSystem::instance().getDefaultAttenuationRange();
			dist = range.max;
		}

		dist *= OVERALL_ATTENUATION_DISTANCE_SCALAR;
		alSourcef( m_idALSource, AL_MAX_DISTANCE, dist );
		HANDLE_AL_ERRORS();
#endif
	}

	float Sound::maxUnattenuatedDistance() const
	{
		FRESH_AUDIO_SYNCHRONIZE
#if !FRESH_NULL_AUDIO
		float result = 0;
		alGetSourcef( m_idALSource, AL_REFERENCE_DISTANCE, &result );
		result /= OVERALL_ATTENUATION_DISTANCE_SCALAR;
		return result;
#else
		return 0;
#endif
	}

	void Sound::maxUnattenuatedDistance( float dist )
	{
		FRESH_AUDIO_SYNCHRONIZE
#if !FRESH_NULL_AUDIO && !FRESH_EMSCRIPTEN
		if( !AudioSystem::ready() ) return;

		if( dist < 0 )
		{
			Range< float > range = AudioSystem::instance().getDefaultAttenuationRange();
			dist = range.min;
		}

		dist *= OVERALL_ATTENUATION_DISTANCE_SCALAR;
		alSourcef( m_idALSource, AL_REFERENCE_DISTANCE, dist );
		HANDLE_AL_ERRORS();
#endif
	}
}

