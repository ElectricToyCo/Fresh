/*
 *  AudioCue.h
 *
 *  Created by Jeff Wofford on 8/12/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_AUDIO_CUE_H_INCLUDED_
#define FRESH_AUDIO_CUE_H_INCLUDED_

#include "Singleton.h"
#include "Object.h"
#include "Asset.h"
#include "FreshFile.h"
#include "FreshVector.h"
#include "AudioSound.h"

namespace fr
{
	class AudioCue : public Asset
	{
		FRESH_DECLARE_CLASS( AudioCue, Asset )
	public:
		
		enum Format
		{
			F_Mono8,
			F_Mono16,
			F_Stereo8,
			F_Stereo16
		};
		
		virtual ~AudioCue();
		
		SYNTHESIZE( bool, allowAsyncLoad );
		
		SYNTHESIZE_GET( double, sampleRate );
		SYNTHESIZE_GET( Format, format );

		SYNTHESIZE( Range< float >, rangeGain );
		SYNTHESIZE( Range< float >, rangePitch );
		SYNTHESIZE( size_t, maxSimultaneousSounds );
		SYNTHESIZE( bool, doLoop );
		SYNTHESIZE( bool, newSoundsTrumpOldSounds );

		double getLengthSeconds() const;
		size_t getLengthSamples() const;
		size_t getBytesPerSample() const;
		
		size_t secondsToBytes( double seconds ) const;
		
		using ErrorHandler = std::function< void( std::string ) >;
		virtual void loadFile( const fr::path& filePath, ErrorHandler&& errorHandler = {} );
		
		Sound::ptr createSound();
		void purgeUnusedSounds();
		
		template< typename FnT >
		void eachSound( FnT&& fn ) const
		{
			for( const auto& sound : m_sounds )
			{
				fn( sound );
			}
		}
		
		size_t loadedBytes() const;
		
		size_t reportDevStatistics() const;
		
		// Inherited from Asset.
		virtual size_t getMemorySize() const override;
		
	private:
		
		double m_sampleRate = 0;
		Format m_format = F_Stereo16;

		std::list< Sound::ptr > m_sounds;
		
		std::shared_ptr< class AudioCueImpl > m_impl;
		
		DVAR( Range< float >, m_rangeGain, Range< float >( 1.0f, 1.0f ) );
		DVAR( Range< float >, m_rangePitch, Range< float >( 1.0f, 1.0f ) );
		DVAR( size_t, m_maxSimultaneousSounds, 0 );	// Zero means unlimited (or rather, limited brutally by OpenAL).
		DVAR( bool, m_doLoop, false );
		DVAR( bool, m_newSoundsTrumpOldSounds, true );
		DVAR( bool, m_allowAsyncLoad, true );
		
		void enqueueBuffersTo( Sound& sound ) const;
		friend class AudioCueImpl;
		friend class AudioCueLoader;
	};
	
	////////////////////////////////////////////////////////////////////////
	
	class AudioCueLoader : public AssetLoader_Default
	{
		FRESH_DECLARE_CLASS( AudioCueLoader, AssetLoader_Default )
	public:
		virtual void loadAsset( Asset::ptr asset ) override
		{
			REQUIRES( asset );
			
			Super::loadAsset( asset );
			
			AudioCue::ptr audioCue = dynamic_freshptr_cast< AudioCue::ptr >( asset );
			ASSERT( audioCue );
		
			audioCue->loadFile( getResourcePath( getCompletePath() ));
			audioCue->rangeGain( m_rangeGain );
			audioCue->rangePitch( m_rangePitch );
			audioCue->maxSimultaneousSounds( m_maxSimultaneousSounds );
			audioCue->doLoop( m_doLoop );
			audioCue->newSoundsTrumpOldSounds( m_newSoundsTrumpOldSounds );
			audioCue->allowAsyncLoad( m_allowAsyncLoad );
		}
		
	protected:
		
		DVAR( Range< float >, m_rangeGain, Range< float >( 1.0f, 1.0f ) );
		DVAR( Range< float >, m_rangePitch, Range< float >( 1.0f, 1.0f ) );
		DVAR( size_t, m_maxSimultaneousSounds, 0 );	// Zero means unlimited (or rather, limited brutally by OpenAL).
		DVAR( bool, m_doLoop, false );
		DVAR( bool, m_newSoundsTrumpOldSounds, true );
		DVAR( bool, m_allowAsyncLoad, true );
	};
}

#endif
