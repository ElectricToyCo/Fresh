//
//  MusicManager.cpp
//  Fresh
//
//  Created by Jeff Wofford on 7/14/12.
//  Copyright (c) 2012 jeffwofford.com. All rights reserved.
//

#include "MusicManager.h"
#include "FreshTime.h"
#include "CommandProcessor.h"

#if DEV_MODE && 0
#	define trace_music( expr ) trace( expr )
#else
#	define trace_music( expr )
#endif

namespace fr
{
	/////////////////////////////////
	
	const char* MusicManager::MUSIC_SOUND_GROUP_NAME = "music_sound_group";
	
	FRESH_DEFINE_CLASS( MusicManager )
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( MusicManager )
	
	MusicManager::MusicManager( const fr::ClassInfo& assignedClassInfo, fr::Object::NameRef objectName )
	:	Super( assignedClassInfo, objectName )
	{
		m_channels[ Background ] = std::make_shared< Channel >();
		m_channels[ Incidental ] = std::make_shared< Channel >();

		// Add the per-channel sound groups to the root music sound group.
		//
		auto& group = soundGroup();
		group.addChild( m_channels[ Background ]->soundGroup() );
		group.addChild( m_channels[ Incidental ]->soundGroup() );
	}
	
	MusicManager::~MusicManager()
	{
#if FRESH_ALLOW_THREADING
		if( isUpdatingAsync() )
		{
			stopAsyncUpdate();
		}
#endif
	}
	
	fr::SoundGroup& MusicManager::soundGroup() const
	{
		return AudioSystem::instance().getSoundGroup( MUSIC_SOUND_GROUP_NAME );
	}
	
	bool MusicManager::isMuted() const
	{
		return m_isMuted;
	}
	
	void MusicManager::mute( bool mute_ )
	{
		if( mute_ != m_isMuted )
		{
			m_isMuted = mute_;
			soundGroup().gain( m_isMuted ? 0.0f : m_maxGain );
		}
	}

	void MusicManager::maxGain( real maxGain_ )
	{
		m_maxGain = maxGain_;
		if( !isMuted() )
		{
			soundGroup().gain( m_maxGain );
		}
	}
	
	void MusicManager::playMusic( Category category, const std::string& cueName, bool loop, TimeType fadeInDuration, TimeType fadeOutPriorMusicDuration )
	{
#if FRESH_ALLOW_THREADING
		m_mutex.lock();
#endif
		
		START_TIMER( playMusic );
		
		m_channels[ category ]->playPiece( cueName, loop, fadeInDuration, fadeOutPriorMusicDuration );

		REPORT_TIMER();

#if FRESH_ALLOW_THREADING
		m_mutex.unlock();
#endif
	}
	
	void MusicManager::fadeCurrentMusic( Category category, float newGain, TimeType fadeDuration, bool stopWhenZero )
	{
#if FRESH_ALLOW_THREADING
		m_mutex.lock();
#endif
		m_channels[ category ]->fadePrimary( newGain, fadeDuration, stopWhenZero );

#if FRESH_ALLOW_THREADING
		m_mutex.unlock();
#endif
	}
	
	void MusicManager::update()
	{
#if FRESH_ALLOW_THREADING
		m_mutex.lock();
#endif

		for( int category = 0; category < NUM_CATEGORIES; ++category )
		{
			m_channels[ category ]->update();
			
			// Duck if any upper category is playing.
			//
			if( category + 1 < NUM_CATEGORIES && m_channels[ category + 1 ]->isPlaying() )
			{
				m_channels[ category ]->duck();
			}
			else
			{
				m_channels[ category ]->unduck();
			}
		}

#if FRESH_ALLOW_THREADING
		m_mutex.unlock();
#endif
	}
	
	bool MusicManager::isUpdatingAsync() const
	{
#if FRESH_ALLOW_THREADING
		return !!m_threadRunLoop;
#else
		return false;
#endif
	}
	
#if FRESH_ALLOW_THREADING
	void MusicManager::startAsyncUpdate( TimeType updatesPerSecond )
	{
		m_killSignal = false;
		m_threadRunLoop.reset( new std::thread( &MusicManager::runUpdateLoop, this, 1.0 / updatesPerSecond ));
	}
	
	void MusicManager::stopAsyncUpdate()
	{
		if( m_threadRunLoop )
		{
			// Tell the thread to stop.
			m_killSignal = true;
			m_threadRunLoop->join();
			m_threadRunLoop.reset();
		}
	}
	
	void MusicManager::runUpdateLoop( TimeType realSecondsPerUpdate )
	{
		std::chrono::milliseconds delay( static_cast< long long >( realSecondsPerUpdate * 1000.0 ));

		while( !m_killSignal )
		{
			// Wait for the time for the next update.
			//
			std::this_thread::sleep_for( delay );
			
			m_mutex.lock();
			update();
			m_mutex.unlock();
		}
	}
#endif
	
	void MusicManager::stopAllAndPurge()
	{
#if FRESH_ALLOW_THREADING
		stopAsyncUpdate();
#endif
		
		for( int i = 0; i < NUM_CATEGORIES; ++i )
		{
			m_channels[ i ]->stop();
		}
		
		AudioSystem::instance().purgeUnusedCues();
	}

	/////////////////////////////////////////////////////////////
	
	MusicManager::FadeController::FadeController()
	:	m_soundGroup( createObject< SoundGroup >( getTransientPackage() ) )
	{}
	
	MusicManager::FadeController::~FadeController() 
	{
		stopSound();
	}

	bool MusicManager::FadeController::usesCueName( NameRef cueName ) const
	{
		return m_sound && m_sound->cue()->name() == cueName;
	}
	
	bool MusicManager::FadeController::isPlaying() const
	{
		return m_sound && m_sound->isPlaying();
	}
	
	void MusicManager::FadeController::play()
	{
		ASSERT( m_sound );
		m_sound->play();
	}
	
	void MusicManager::FadeController::looping( bool loop )
	{
		ASSERT( m_sound );
		m_sound->looping( loop );
	}

	void MusicManager::FadeController::playSound( NameRef cueName )
	{
		ASSERT( m_soundGroup );
		
		stopSound();
		
		m_sound = AudioSystem::playSound( cueName );
		if( m_sound )
		{
			m_sound->parentGroup()->removeChild( m_sound );
			m_soundGroup->addChild( m_sound );
		}
		
		m_fadeEndGain = -1;		// Abort fading.
	}
	
	void MusicManager::FadeController::stopSound()
	{
		if( m_sound )
		{
			m_sound->stop();
			m_soundGroup->removeChild( m_sound );
			m_sound = nullptr;
		}
		m_fadeEndGain = -1;
	}

	void MusicManager::FadeController::update()
	{
		if( m_fadeEndGain >= 0 )
		{
			const auto now = getAbsoluteTimeSeconds();
			const auto normalizedTime = m_tweener.normalizedTime( now );
			ASSERT( normalizedTime >= 0 );
			
			if( normalizedTime >= 1.0f )
			{
				onDoneFading();
			}
			else
			{
				m_soundGroup->gain( m_tweener( m_fadeStartGain, m_fadeEndGain, now ));
			}
		}
	}

	void MusicManager::FadeController::fadeToGain( float gain, TimeType duration, bool stopWhenZero )
	{
		REQUIRES( gain >= 0 );
		
		m_doStopWhenGainZero = stopWhenZero;
		
		if( duration > 0 )
		{
			m_tweener.timeStart( getAbsoluteTimeSeconds() );
			m_tweener.timeEnd( m_tweener.timeStart() + duration );
			m_fadeStartGain = m_soundGroup->gain();
			
			if( m_fadeStartGain != gain )
			{
				m_fadeEndGain = gain;		
			}
			else
			{
				m_fadeEndGain = -1;
			}
		}
		else
		{
			// Just make it so.
			//
			m_fadeEndGain = gain;
			onDoneFading();
		}
	}
	
	void MusicManager::FadeController::onDoneFading()
	{
		m_soundGroup->gain( m_fadeEndGain );
		
		if( m_doStopWhenGainZero && m_fadeEndGain == 0 )
		{
			for( auto soundGroupChild : *m_soundGroup )
			{
				if( auto sound = dynamic_freshptr_cast< Sound::wptr >( soundGroupChild ))
				{
					sound->stop();
				}
			}

			stopSound();
			
			AudioSystem::instance().purgeUnusedCues();
		}
		
		m_fadeEndGain = -1;
		m_doStopWhenGainZero = false;
	}
	
	/////////////////////////////////////////////////////////////
		
	MusicManager::Channel::Channel()
	{
		for( int i = 0; i < NUM_PIECES; ++i )
		{
			m_pieceFadeControllers[ i ].reset( new FadeController() );
			soundGroup()->addChild( m_pieceFadeControllers[ i ]->soundGroup() );
		}
	}
	
	void MusicManager::Channel::playPiece( const std::string& cueName, bool loop, TimeType fadeInDuration, TimeType fadeOutPriorMusicDuration )
	{
		// Maybe we're already playing this sound here?
		//
		if( !m_pieceFadeControllers[ Primary ]->usesCueName( cueName ) || !m_pieceFadeControllers[ Primary ]->isPlaying() )
		{
			// Fade out prior music in this category.
			//
			if( m_pieceFadeControllers[ Primary ]->isPlaying() )
			{
				// Clear the secondary slot before moving the primary back into it.
				//
				m_pieceFadeControllers[ Secondary ]->stopSound();
				
				// Shift it back to the "outgoing" slot.
				//
				std::swap( m_pieceFadeControllers[ Primary ], m_pieceFadeControllers[ Secondary ] );
				
				// Fade it out.
				//
				m_pieceFadeControllers[ Secondary ]->fadeToGain( 0, fadeOutPriorMusicDuration );
			}
			
			m_pieceFadeControllers[ Primary ]->playSound( cueName );
		}
		
		m_pieceFadeControllers[ Primary ]->fadeToGain( 1.0f, fadeInDuration );
		m_pieceFadeControllers[ Primary ]->looping( loop );
		
		if( !m_pieceFadeControllers[ Primary ]->isPlaying() )
		{
			m_pieceFadeControllers[ Primary ]->play();
		}
	}
	
	bool MusicManager::Channel::isPlaying() const
	{
		for( int i = 0; i < NUM_PIECES; ++i )
		{
			ASSERT( m_pieceFadeControllers[ i ] );
			if( m_pieceFadeControllers[ i ]->isPlaying() )
			{
				return true;
			}
		}
		return false;
	}

	void MusicManager::Channel::update()
	{
		FadeController::update();
		
		for( int i = 0; i < NUM_PIECES; ++i )
		{
			ASSERT( m_pieceFadeControllers[ i ] );
			m_pieceFadeControllers[ i ]->update();
		}
	}
	
	void MusicManager::Channel::fadeToGain( float gain, TimeType duration, bool stopWhenZero )
	{
		FadeController::fadeToGain( gain, duration, stopWhenZero );
		m_isDucking = m_isUnducking = false;
	}
	
	void MusicManager::Channel::duck()
	{
		if( !m_isDucking )
		{
			fadeToGain( m_duckGain, m_duckTime );
			m_isDucking = true;
		}
	}
	
	void MusicManager::Channel::unduck()
	{
		if( !m_isUnducking )
		{
			fadeToGain( 1.0f, m_duckTime );
			m_isUnducking = true;
		}
	}

	void MusicManager::Channel::fadePrimary( float newGain, TimeType duration, bool stopWhenZero )
	{
		m_pieceFadeControllers[ Primary ]->fadeToGain( newGain, duration, stopWhenZero );
	}

	void MusicManager::Channel::stop()
	{
		for( int i = 0; i < NUM_PIECES; ++i )
		{
			m_pieceFadeControllers[ i ]->stopSound();
		}
	}
	
	void MusicManager::Channel::onDoneFading()
	{
		FadeController::onDoneFading();
		
		m_isDucking = m_isUnducking = false;
	}
	
	std::string MusicManager::Channel::debugInfo() const
	{
		std::ostringstream stream;

		for( int i = 0; i < 2; ++i )
		{
			stream << "\t" << i << ": " << ( m_pieceFadeControllers[ i ]->isPlaying() ? "playing" : "paused " ) << " with gain " << m_pieceFadeControllers[ i ]->soundGroup()->gain() << "\n";
		}
		
		return stream.str();
	}
}

