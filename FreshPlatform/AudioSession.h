//
//  AudioSession.h
//  Fresh
//
//  Created by Jeff Wofford on 8/11/17.
//
//

#ifndef AudioSession_h
#define AudioSession_h

namespace fr
{
	namespace audiosession
	{
		class Delegate
		{
		public:
			virtual void onAudioInterruption() {}
			virtual void onAudioResuming() {}
			virtual void onOtherAudioPlaying() {}
			virtual void onOtherAudioStopping() {}
			virtual ~Delegate() {}
		};
		
		void start( Delegate* delegate );
		bool isOtherAudioPlaying();
	}
}

#endif /* AudioSession_h */
