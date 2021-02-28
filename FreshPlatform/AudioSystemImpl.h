//
//  AudioSystemImpl.h
//  Fresh
//
//  Created by Jeff Wofford on 12/12/16.
//
//

#ifndef AudioSystemImpl_h
#define AudioSystemImpl_h

#include "FreshThread.h"

#if FRESH_ALLOW_THREADING

namespace fr
{
	namespace audio
	{
		std::unique_lock< std::recursive_mutex > lockMutex();
	}
}

#	define FRESH_AUDIO_SYNCHRONIZE auto lock_ = audio::lockMutex();

#else

#	define FRESH_AUDIO_SYNCHRONIZE

#endif

#endif /* AudioSystemImpl_h */
