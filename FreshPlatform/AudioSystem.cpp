/*
 *  AudioSystem.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 8/12/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#include "AudioSystem.h"
#include "Objects.h"
#include "CommandProcessor.h"
#include "FreshFile.h"
#include "FreshException.h"
#include "FreshOpenAL.h"
#include "AudioSystemImpl.h"

#if TARGET_OS_MAC
#	include <AudioToolbox/ExtendedAudioFile.h>
#	include <TargetConditionals.h>
#else
#	if ANDROID
#		include "AL/alext.h"		// For alcDevicePauseSOFT() and alcDeviceResumeSOFT()
#	endif
#	define USING_ALUT 1
#	include <AL/alut.h>
#endif

#if DEV_MODE && 0
#	define audio_trace(x) release_trace(x)
#else
#	define audio_trace(x)
#endif

namespace
{
	using namespace fr;

	const char DEFAULT_SOUND_GROUP_NAME[] = "~~default~~";

#if FRESH_ALLOW_THREADING
	std::recursive_mutex g_grandCentralMutex;
#endif
}

namespace fr
{
#if FRESH_ALLOW_THREADING
	namespace audio
	{
		std::unique_lock< std::recursive_mutex > lockMutex()
		{
			std::unique_lock< std::recursive_mutex > lock;

			try
			{
				lock = decltype( lock ){ g_grandCentralMutex };
			}
			catch( ... )
			{}

			return lock;
		}
	}
#endif

	class AudioSystemImpl
	{
	public:
#if !FRESH_NULL_AUDIO
		ALCdevice* m_device = nullptr;
		ALCcontext* m_context = nullptr;
		ALCcontext* m_currentContext = nullptr;
#endif
	};


	FRESH_DEFINE_CLASS( AudioSystem )

	bool AudioSystem::ready()
	{
		FRESH_AUDIO_SYNCHRONIZE
		return AudioSystem::doesExist() && AudioSystem::instance().isInitialized();
	}

	Sound::ptr AudioSystem::playSound( const std::string& cueName, const std::string& groupName, float initialGain )
	{
		FRESH_AUDIO_SYNCHRONIZE
		if( AudioSystem::doesExist() )
		{
			auto& audioSystem = AudioSystem::instance();
			if( audioSystem.isInitialized() )
			{
				auto sound = audioSystem.playSoundInternal( cueName, groupName, initialGain );
				if( !sound )
				{
					dev_warning( "Unable to play sound with cue " << cueName );
				}
				return sound;
			}
		}
		return nullptr;
	}

	Sound::ptr AudioSystem::createSound( const std::string& cueName, const std::string& groupName )
	{
		FRESH_AUDIO_SYNCHRONIZE
		if( AudioSystem::doesExist() )
		{
			auto& audioSystem = AudioSystem::instance();
			if( audioSystem.isInitialized() )
			{
				auto sound = audioSystem.createSoundInternal( cueName, groupName );
				if( !sound )
				{
					dev_warning( "Unable to play sound with cue " << cueName );
				}
				return sound;
			}
		}
		return nullptr;
	}

	Sound::ptr AudioSystem::playSoundInternal( const std::string& cueName, const std::string& groupName, float initialGain )
	{
		FRESH_AUDIO_SYNCHRONIZE
		auto result = createSoundInternal( cueName, groupName );
		if( result )
		{
			if( initialGain >= 0 )
			{
				result->gain( initialGain );
			}

			result->play();
		}
		return result;
	}

	Sound::ptr AudioSystem::createSoundInternal( const std::string& cueName, const std::string& groupName )
	{
		FRESH_AUDIO_SYNCHRONIZE
		AudioCue::ptr cue;

		auto iter = m_cachedCues.find( cueName );
		if( iter != m_cachedCues.end() )
		{
			cue = iter->second;
		}

		if( !cue )
		{
			try
			{
				cue = createOrGetObject< AudioCue >( cueName );
			}
			catch( const FreshException& e )
			{
				dev_error( "Unable to load AudioCue named '" << cueName << "'." );
			}

			if( cue )
			{
				m_cachedCues[ cueName ] = cue;
			}
			else
			{
				return nullptr;
			}
		}

		// Register the cue if it hasn't been registered yet.
		//
		if( std::find( m_cues.begin(), m_cues.end(), cue ) == m_cues.end() )
		{
			m_cues.push_back( cue );
		}

		auto result = cue->createSound();

		if( result )
		{
			const std::string actualGroupName = groupName.empty() ? DEFAULT_SOUND_GROUP_NAME : groupName;
			getSoundGroup( actualGroupName ).addChild( result );
		}

		return result;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	AudioSystem::AudioSystem( fr::CreateInertObject c )
	:	Super( c )
	,	ObjectSingleton< AudioSystem >( nullptr )
	,	m_impl( new AudioSystemImpl{} )
	{}

	AudioSystem::AudioSystem( const ClassInfo& assignedClassInfo, NameRef objectName /* = DEFAULT_OBJECT_NAME */ )
	:	Object( assignedClassInfo, objectName )
	,	ObjectSingleton< AudioSystem >( this )
	,	m_impl( new AudioSystemImpl{} )
	{
		initialize();

		// Create the sound group package.
		//
		m_soundGroupPackage = createPackage( "~sound_groups" );

		// Create the default sound group.
		//
		getDefaultSoundGroup();

		// Create the audiotrace command.
		//
		{
			auto boundFn( std::bind( &AudioSystem::reportDevStatistics, this ) );
			auto caller = make_caller< void >( boundFn );
			CommandProcessor::instance().registerCommand( this, "audiotrace", "Reports development statistics for audio", caller );
		}
	}

	AudioSystem::~AudioSystem()
	{
		shutdown();
		if( CommandProcessor::doesExist() )
		{
			CommandProcessor::instance().unregisterAllCommandsForHost( this );
		}
	}

	void AudioSystem::initialize()
	{
#if !FRESH_NULL_AUDIO

		FRESH_AUDIO_SYNCHRONIZE

		REQUIRES( !isInitialized() );

		fr::audiosession::start( this );

		const ALCchar* deviceName = alcGetString( nullptr, ALC_DEFAULT_DEVICE_SPECIFIER );
		trace( "Attempting to open audio device " << deviceName );

		m_impl->m_device = alcOpenDevice( nullptr );
		if( m_impl->m_device )
		{
			// Report audio device information.
			//
			ALCint frequency	 = 0;
			ALCint refresh		 = 0;
			ALCint monoSources	 = 0;
			ALCint stereoSources = 0;

			const ALCchar* deviceName = alcGetString(m_impl->m_device, ALC_DEVICE_SPECIFIER);
			alcGetIntegerv(m_impl->m_device, ALC_FREQUENCY, 1, &frequency);
			alcGetIntegerv(m_impl->m_device, ALC_REFRESH, 1, &refresh);
			alcGetIntegerv(m_impl->m_device, ALC_MONO_SOURCES, 1, &monoSources);
			alcGetIntegerv(m_impl->m_device, ALC_STEREO_SOURCES, 1, &stereoSources);

			trace( "Opened audio device " << deviceName );
			trace( " - Frequency: " << frequency << " Hz" );
			trace( " - Refresh Rate: " << refresh << " Hz" );
			trace( " - Mono Sources: " << monoSources );
			trace( " - Stereo Sources: " << stereoSources );
		}
		else
		{
			con_error( "Unable to open OpenAL device. Audio will be disabled." );
			return;
		}

		m_impl->m_context = alcCreateContext( m_impl->m_device, nullptr );
		if( !m_impl->m_context )
		{
			con_error( "Unable to create OpenAL context. Audio will be disabled. Error: " << std::hex << alcGetError( m_impl->m_device ) );
			return;
		}

		alcMakeContextCurrent( m_impl->m_context );
		m_impl->m_currentContext = m_impl->m_context;
		ASSERT( alcGetCurrentContext() );

#if USING_ALUT
		// Initialize ALUT (Windows, Linux)
		//
		int bogusArgc = 1;
		const char* bogusArgv[] = { "whatever" };

		trace( "Initializing ALUT." );
		alutInitWithoutContext( &bogusArgc, const_cast< char** >( bogusArgv ));
#endif

		trace( "Initialized OpenAL with context " << reinterpret_cast< size_t >( m_impl->m_context ));

#	if !FRESH_EMSCRIPTEN		// TODO Reinstate when Emscripten adds distance model/listener support.
		// Set the distance attenuation model to linear clamped.
		//
		alDistanceModel( AL_LINEAR_DISTANCE_CLAMPED );

		// Set the default listener position and orientation.
		//
		alListener3f( AL_POSITION, 0.0f, 0.0f, 0.0f );

		float listenerOrientation[ 6 ] =
		{
			0,  0.0f,  1.0f,			// "AT" vector: points down the -Z axis.
			0, -1.0f,  0.0f,			// "UP" vector is along the negative Y axis ("up" on the 2D screen).
		};
		alListenerfv( AL_ORIENTATION, listenerOrientation );
#	endif

		HANDLE_AL_ERRORS();
#endif
	}

	bool AudioSystem::isInitialized() const
	{
		FRESH_AUDIO_SYNCHRONIZE
#if !FRESH_NULL_AUDIO
		return m_impl->m_device && m_impl->m_context;
#else
		return true;
#endif
	}

	void AudioSystem::shutdown()
	{
		FRESH_AUDIO_SYNCHRONIZE

#if !FRESH_NULL_AUDIO
		if( m_impl->m_context )
		{
			alcDestroyContext( m_impl->m_context );
			m_impl->m_context = nullptr;
			m_impl->m_currentContext = nullptr;
		}

		if( m_impl->m_device )
		{
			alcCloseDevice( m_impl->m_device );
			m_impl->m_device = nullptr;
		}

#if USING_ALUT
		alutExit();
#endif

		PROMISES( !isInitialized() );
#endif
	}

	void AudioSystem::setListenerPosition( const vec3& position )
	{
		FRESH_AUDIO_SYNCHRONIZE

#if !FRESH_NULL_AUDIO && !FRESH_EMSCRIPTEN
		alListener3f( AL_POSITION, position.x, position.y, position.z );
#endif
	}

	void AudioSystem::setDefaultAttenuationRange( const Range< float >& range )
	{
		FRESH_AUDIO_SYNCHRONIZE
		m_defaultAttenuationRange = range;
	}

	Range< float > AudioSystem::getDefaultAttenuationRange()
	{
		FRESH_AUDIO_SYNCHRONIZE
		return m_defaultAttenuationRange;
	}

	SoundGroup& AudioSystem::getSoundGroup( const std::string& groupName )
	{
		FRESH_AUDIO_SYNCHRONIZE

		ASSERT( !groupName.empty() );
		ASSERT( m_soundGroupPackage );

		SoundGroup::ptr soundGroup;
		if( !( soundGroup = m_soundGroupPackage->request< SoundGroup >( groupName )))
		{
			soundGroup = createObject< SoundGroup >( *m_soundGroupPackage, groupName );
			ASSERT( soundGroup );
			m_soundGroups.push_back( soundGroup );
		}
		ASSERT( soundGroup );
		return *soundGroup;
	}

	SoundGroup& AudioSystem::getDefaultSoundGroup()
	{
		FRESH_AUDIO_SYNCHRONIZE

		return getSoundGroup( DEFAULT_SOUND_GROUP_NAME );	// Safe because we're confident we'll avoid the m_soundGroups.push_back() call above.
	}

	void AudioSystem::suspend()
	{
		FRESH_AUDIO_SYNCHRONIZE

#if !FRESH_NULL_AUDIO

#	if ANDROID

		// TODO: Move to AudioSession_Android.cpp
		alcDevicePauseSOFT( m_impl->m_device );

#	else	// not ANDROID

		// Suspend the context
		//
		if( m_impl->m_currentContext )
		{
			alcMakeContextCurrent( nullptr );
			m_impl->m_currentContext = nullptr;
			alcSuspendContext( m_impl->m_context );
		}
#	endif
#endif
	}

	void AudioSystem::unsuspend()
	{
		FRESH_AUDIO_SYNCHRONIZE

#if !FRESH_NULL_AUDIO

#	if ANDROID

		// TODO: Move to AudioSession_Android.cpp
		alcDeviceResumeSOFT( m_impl->m_device );

#	else	// not ANDROID

		// Restore the context
		//
		if( m_impl->m_currentContext != m_impl->m_context )
		{
			alcMakeContextCurrent( m_impl->m_context );
			m_impl->m_currentContext = m_impl->m_context;
			alcProcessContext( m_impl->m_context );
		}
#	endif
#endif
	}

	void AudioSystem::purgeUnusedCues()
	{
		FRESH_AUDIO_SYNCHRONIZE

		m_cues.remove_if( std::mem_fn( &WeakPtr< AudioCue >::isNull ));

		// Purge unused sounds too.
		//
		for( auto cue : m_cues )
		{
			cue->purgeUnusedSounds();
		}
	}

	void AudioSystem::onAudioInterruption()
	{
		suspend();
	}

	void AudioSystem::onAudioResuming()
	{
		unsuspend();
	}

	//////////////
	// TODO: automatic music suspension and resumption on these two events.
	void AudioSystem::onOtherAudioPlaying()
	{}

	void AudioSystem::onOtherAudioStopping()
	{}
	//////////////

	void AudioSystem::reportDevStatistics()
	{
		FRESH_AUDIO_SYNCHRONIZE

		purgeUnusedCues();

		trace( "AUDIO CUES" );

		size_t cueBytes = 0;
		size_t nSounds = 0;
		for( auto cue : m_cues )
		{
			auto bytes = cue->getMemorySize();

			ASSERT( cue );
			trace( "\t" << cue << ": " << getByteCountString( bytes ) << " @ " << cue->sampleRate() << " (" << std::setprecision( 4 ) << cue->getLengthSeconds() << "s) " );

			cueBytes += bytes;

			nSounds += cue->reportDevStatistics();
		}

		trace( m_cues.size() << " cues with " << getByteCountString( cueBytes ) << " total" );

		trace( nSounds << " sounds total" );
	}


	/////////////////////////////////////////////////////////////////////////////////

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( Voice )

	FRESH_DEFINE_CLASS( Voice )
	Voice::Voice( const ClassInfo& assignedClassInfo, NameRef name )
	:	Object( assignedClassInfo, name )
	{}

	Sound::ptr Voice::playSound( const std::string& cueName, const std::string& groupName, Priority priority_, float initialGain )
	{
		REQUIRES( priority_ != NO_PRIORITY );

		if( doesDefeatPriority( priority(), priority_ ))
		{
			// Stop existing sound.
			//
			if( m_sound && m_sound->isPlaying() )
			{
				m_sound->stop();
			}

			// Play new sound.
			//
			return m_sound = AudioSystem::playSound( cueName, groupName, initialGain );
		}
		else
		{
			return nullptr;
		}
	}

	void Voice::stopSound()
	{
		if( m_sound )
		{
			m_sound->stop();
			m_sound = nullptr;
		}
	}

	Voice::Priority Voice::priority() const
	{
		if( !m_sound || !m_sound->isPlaying() )
		{
			return NO_PRIORITY;
		}
		else
		{
			return m_priority;
		}
	}

	Sound::ptr Voice::playingSound() const
	{
		return ( m_sound && m_sound->isPlaying()) ? m_sound : nullptr;
	}

	bool Voice::doesDefeatPriority( Priority existing, Priority later ) const
	{
		if( existing == NO_PRIORITY )
		{
			return later != NO_PRIORITY;
		}
		else
		{
			return later != NO_PRIORITY && later >= existing;
		}
	}

}

