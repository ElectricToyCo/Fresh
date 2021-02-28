//
//  AudioSession_Null.cpp
//  Fresh
//
//  Created by Jeff Wofford on 08/11/17.
//
//

#include "AudioSession.h"
#include "FreshDebug.h"

namespace fr
{
	namespace audiosession
	{
		void start( Delegate* )
		{}
		
		bool isOtherAudioPlaying()
		{
			return false;
		}
	}
}
