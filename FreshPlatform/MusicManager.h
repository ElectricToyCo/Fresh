//
//  MusicManager.h
//  Fresh
//
//  Created by Jeff Wofford on 7/14/12.
//  Copyright (c) 2012 jeffwofford.com. All rights reserved.
//

#ifndef Fresh_MusicManager_h
#define Fresh_MusicManager_h

#include "AudioSystem.h"
#include "Tweener.h"
#include "FreshThread.h"

namespace fr
{
	
	class MusicManager : public fr::Object
	{
		FRESH_DECLARE_CLASS( MusicManager, Object );
	public:
		
		enum Category	// Higher categories "duck" lower ones.
		{
			Background,
			Incidental,
			NUM_CATEGORIES
		};
		
		static const char* MUSIC_SOUND_GROUP_NAME;
		
		virtual ~MusicManager();

		bool isMuted() const;
		void mute( bool mute_ );
		
		SYNTHESIZE_GET( real, maxGain )
		void maxGain( real maxGain_ );
		
		void playMusic( Category category, const std::string& cueName, bool loop = true, TimeType fadeInDuration = 0.0, TimeType fadeOutPriorMusicDuration = 1.0 );
		void fadeCurrentMusic( Category category, float newGain, TimeType fadeDuration = 1.0, bool stopWhenZero = false );
		
		void update();
		
		bool isUpdatingAsync() const;
#if FRESH_ALLOW_THREADING
		void startAsyncUpdate( TimeType updatesPerSecond = 20.0 );
		void stopAsyncUpdate();
#endif
		void setDucking( Category category, float gain, TimeType duration );
		
		fr::SoundGroup& soundGroup() const;
		
		void stopAllAndPurge();
		
	private:
		
		class FadeController
		{
		public:

			FadeController();
			virtual ~FadeController();

			void playSound( NameRef cueName );
			void stopSound();
			bool usesCueName( NameRef cueName ) const;
			
			void play();
			void looping( bool loop );
			bool isPlaying() const;
			
			virtual void update();
			virtual void fadeToGain( float gain, TimeType duration, bool stopWhenZero = false );

			fr::SoundGroup::ptr soundGroup() const
			{
				return m_soundGroup;
			}
			
		protected:
			
			virtual void onDoneFading();
			
		private:
			
			fr::Sound::ptr m_sound;
			fr::SoundGroup::ptr m_soundGroup;

			float m_fadeStartGain = 0;
			float m_fadeEndGain = -1.0f;
			
			bool m_doStopWhenGainZero = false;
			
			fr::TweenerQuadEaseOut< float > m_tweener;
			
			FRESH_PREVENT_COPYING( FadeController );
		};
		
		//////////////////////////////////////////////////
		
		class Channel : public FadeController 		// One per category
		{
		public:
			
			Channel();
			
			void playPiece( const std::string& cueName, bool loop, TimeType fadeInDuration = 0.0, TimeType fadeOutPriorMusicDuration = 1.0 );
			bool isPlaying() const;
			
			virtual void update() override;
			virtual void fadeToGain( float gain, TimeType duration, bool stopWhenZero = false ) override;
			
			SYNTHESIZE( float, duckGain )
			SYNTHESIZE( TimeType, duckTime )
			
			void duck();
			void unduck();
			
			void fadePrimary( float newGain, TimeType duration, bool stopWhenZero = false );
			
			void stop();
			
			std::string debugInfo() const;
			
		private:
			
			float m_duckGain = 0.2f;
			TimeType m_duckTime = 2.0;
			
			enum Pieces
			{
				Primary,
				Secondary,
				NUM_PIECES
			};
			
			std::unique_ptr< FadeController > m_pieceFadeControllers[ NUM_PIECES ];
			
			bool m_isDucking = false;
			bool m_isUnducking = false;
			
			virtual void onDoneFading() override;			
		};

		std::shared_ptr< Channel > m_channels[ NUM_CATEGORIES ];
		
		bool m_isMuted = false;
		float m_maxGain = 1.0f;
		
		void runUpdateLoop( TimeType realSecondsPerUpdate );
		
#if FRESH_ALLOW_THREADING
		std::unique_ptr< std::thread > m_threadRunLoop;
		std::recursive_mutex m_mutex;
		std::atomic< bool > m_killSignal;
#endif
	};

}

#endif
