//
//  AudioSession_iOS.mm
//  Fresh
//
//  Created by Jeff Wofford on 08/11/17.
//
//

#include "AudioSession.h"
#include "FreshDebug.h"
#import <AVFoundation/AVFoundation.h>

namespace
{
	fr::audiosession::Delegate* g_delegate = nullptr;
}

namespace fr
{
	namespace audiosession
	{
		void start( Delegate* delegate )
		{
			ASSERT( g_delegate == nullptr );
			ASSERT( delegate );
			
			g_delegate = delegate;
			
			[[AVAudioSession sharedInstance] setCategory: AVAudioSessionCategoryAmbient error: nil];
			[[AVAudioSession sharedInstance] setActive: YES error: nil];
			
			NSNotificationCenter *center = [NSNotificationCenter defaultCenter];

			[center addObserverForName:AVAudioSessionInterruptionNotification
								object:nil
								 queue:nil
							usingBlock:^(NSNotification *notification)
			{
				const bool interruptionBeginning = [[notification.userInfo valueForKey:AVAudioSessionInterruptionTypeKey] intValue] == AVAudioSessionInterruptionTypeBegan;
				if( interruptionBeginning )
				{
					g_delegate->onAudioInterruption();
					[[AVAudioSession sharedInstance] setActive: NO error: nil];
				}
				else
				{
					[[AVAudioSession sharedInstance] setActive: YES error: nil];
					g_delegate->onAudioResuming();
				}
			}];
			
			[center addObserverForName:AVAudioSessionSilenceSecondaryAudioHintNotification
								object:nil
								 queue:nil
							usingBlock:^(NSNotification *notification)
			 {
				 const bool secondaryAudioBeginning = [[notification.userInfo valueForKey:AVAudioSessionSilenceSecondaryAudioHintTypeKey] intValue] == AVAudioSessionSilenceSecondaryAudioHintTypeBegin;
				 if( secondaryAudioBeginning )
				 {
					 g_delegate->onOtherAudioPlaying();
				 }
				 else
				 {
					 g_delegate->onOtherAudioStopping();
				 }
			 }];			
		}
		
		bool isOtherAudioPlaying()
		{
			return [[AVAudioSession sharedInstance] secondaryAudioShouldBeSilencedHint];
		}
	}
}
