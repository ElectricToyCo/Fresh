/*
 *  AudioSystem.h
 *	Fresh
 *
 *  Created by Jeff Wofford on 8/12/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_AUDIO_SYSTEM_H_INCLUDED_
#define FRESH_AUDIO_SYSTEM_H_INCLUDED_

#include "Singleton.h"
#include "Object.h"
#include "FreshEssentials.h"
#include "Asset.h"
#include "FreshVector.h"
#include "AudioCue.h"
#include "AudioSound.h"
#include "AudioSession.h"

namespace fr
{
	class AudioSystem : public Object, public ObjectSingleton< AudioSystem >, public fr::audiosession::Delegate
	{
	public:
				
		virtual ~AudioSystem();
			// REQUIRES( !isInitialized() );
		
		bool isInitialized() const;
		void initialize();
		// REQUIRES( !isInitialized() );
		// PROMISES( isInitialized() );
		void shutdown();
		// REQUIRES( isInitialized() );
		// PROMISES( !isInitialized() );
		
		static bool ready();
		
		static Sound::ptr playSound( const std::string& cueName, const std::string& groupName = "", float initialGain = -1 );
		// REQUIRES( isInitialized() );
		// PROMISES( result );
		// If initialGain < 0 it is ignored.
		static Sound::ptr createSound( const std::string& cueName, const std::string& groupName = "" );
		// REQUIRES( isInitialized() );
		// PROMISES( result );
		
		void setListenerPosition( const vec3& position );
		void setListenerPosition( const vec2& position )					{ setListenerPosition( vec3( position.x, position.y, 0 )); }
		void setDefaultAttenuationRange( const Range< float >& range );
		Range< float > getDefaultAttenuationRange();
		
		SoundGroup& getSoundGroup( const std::string& groupName );			// Creates the group if it doesn't already exist.
		SoundGroup& getDefaultSoundGroup();

		void purgeUnusedCues();
		
		// For management of phone call interruptions.
		//
		void suspend();
		void unsuspend();
		
		void reportDevStatistics();
		
		virtual void onAudioInterruption() override;
		virtual void onAudioResuming() override;
		virtual void onOtherAudioPlaying() override;
		virtual void onOtherAudioStopping() override;

	private:
		
		std::unique_ptr< class AudioSystemImpl > m_impl;
		
		Range< float >	m_defaultAttenuationRange = Range< float >( 0.0f, 100.0f );
		
		std::list< AudioCue::wptr > m_cues;
		std::vector< SoundGroup::ptr > m_soundGroups;
		
		std::unordered_map< std::string, AudioCue::wptr > m_cachedCues;
		
		Package::ptr m_soundGroupPackage;
		
		Sound::ptr playSoundInternal( const std::string& cueName, const std::string& groupName = "", float initialGain = -1 );
		// REQUIRES( isInitialized() );
		// PROMISES( result );
		// If initialGain < 0 it is ignored.
		Sound::ptr createSoundInternal( const std::string& cueName, const std::string& groupName = "" );
		// REQUIRES( isInitialized() );
		// PROMISES( result );
		
		FRESH_DECLARE_CLASS( AudioSystem, Object )
	};

	///////////////////////////////////////////////////////////
	
	class Voice : public Object
	{
	public:

		typedef unsigned int Priority;
		static const unsigned int NO_PRIORITY = ~0;
		static const unsigned int MIN_PRIORITY = 0;
		
		Sound::ptr playSound( const std::string& assetName, const std::string& groupName = "", Priority priority_ = MIN_PRIORITY, float initialGain = -1 );
			// REQUIRES( priorsity != NO_PRIORITY );
			// If a sound is already playing, the new sound will replace it if priority_ >= priority().
		
		void stopSound();
			// Stop any playing sound. No effect if no sound is playing.
		
		Priority priority() const;
			// returns NO_PRIORITY if no sound is currently playing.
		
		Sound::ptr playingSound() const;

	protected:
		
		bool doesDefeatPriority( Priority existing, Priority later ) const;
		
	private:
		
		Sound::ptr m_sound;
		Priority m_priority = NO_PRIORITY;
		
		FRESH_DECLARE_CLASS( Voice, Object )
	};
}

#endif
