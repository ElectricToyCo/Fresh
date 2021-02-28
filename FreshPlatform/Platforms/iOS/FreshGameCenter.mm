//
//  FreshGameCenter.mm
//  Fresh
//
//  Created by Jeff Wofford on 10/7/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//
//	iOS Implementation
//
#include "FreshGameCenter.h"
#include "CommandProcessor.h"
#import "GameKit/GameKit.h"

extern UIWindow* g_rootWindow;

@interface GameCenterProxy : NSObject
{
@private
	GKLocalPlayer* gameKitPlayer;
}

@property(nonatomic, retain) GKLocalPlayer* gameKitPlayer;
@property(nonatomic, retain) NSMutableDictionary* achievementsDictionary;
@end

/////////////////////////////////////////////////

@implementation GameCenterProxy

@synthesize gameKitPlayer;
@synthesize achievementsDictionary;

- (void) loadAchievements
{
	achievementsDictionary = [[NSMutableDictionary alloc] init];
	
    [GKAchievement loadAchievementsWithCompletionHandler: ^(NSArray *achievements, NSError *error)
	 {
		 if( error == nil )
		 {
			 for (GKAchievement* achievement in achievements)
			 {
//				 trace( "Achievement " << [achievement.identifier UTF8String] << " is " << achievement.percentComplete << "% complete." );
				 [achievementsDictionary setObject: achievement forKey: achievement.identifier];
			 }
		 }
	 }];
}

- (GKAchievement*) getAchievementForIdentifier: (NSString*) identifier
{
    GKAchievement *achievement = [achievementsDictionary objectForKey:identifier];
    if (achievement == nil)
    {
        achievement = [[GKAchievement alloc] initWithIdentifier:identifier];
        [achievementsDictionary setObject:achievement forKey:achievement.identifier];
    }
    return achievement;
}

- (bool) hasAchievement: (NSString*) identifier
{
	return [self getAchievementForIdentifier: identifier] != nil;
}

- (void) authenticatedPlayer: (GKLocalPlayer*) localPlayer
{
	trace( "Local player is " << [localPlayer.displayName UTF8String] );
	gameKitPlayer = localPlayer;
	
	[self loadAchievements];
}

- (void) disableGameCenter
{
	gameKitPlayer = nil;
}

- (void) showAuthenticationDialogWhenReasonable: (UIViewController*) viewController
{
	UIViewController* controller = [g_rootWindow rootViewController];
	ASSERT( controller );
	[controller presentViewController: viewController animated: YES completion: nil];
}

// Should be called ASAP.
//
- (void) authenticateLocalPlayer
{
    GKLocalPlayer *localPlayer = [GKLocalPlayer localPlayer];
	
    // The act of setting a handler inspires iOS to authenticate the player when appropriate.
    localPlayer.authenticateHandler = ^(UIViewController *viewController, NSError *error)
    {
		if (viewController != nil)
		{
			[self showAuthenticationDialogWhenReasonable: viewController];
		}
		else if (localPlayer.isAuthenticated)
		{
			[self authenticatedPlayer: localPlayer];
		}
		else
		{
			trace( "Disabling game center" );
			[self disableGameCenter];
		}
	};
}

- (void) reportAchievementIdentifier: (NSString*) identifier percentComplete: (float) percent
{
    GKAchievement* achievement = [self getAchievementForIdentifier:identifier];
    if( achievement )
	{
		float priorPercent = achievement.percentComplete;
		float improvement = percent - priorPercent;
		if( improvement > 0.1f )		// Correcting for epsilon errors
		{
			achievement.percentComplete = percent;
			achievement.showsCompletionBanner = priorPercent < 100.0f && achievement.percentComplete >= 100.0f;
			
			[GKAchievement reportAchievements: @[achievement] withCompletionHandler: ^(NSError *error)
			 {
				 if (error != nil)
				 {
					 NSLog( @"%@", [error localizedDescription] );
				 }
			 }];
		}
	}
}

- (void) clearAllAchievementProgress
{
	achievementsDictionary = [[NSMutableDictionary alloc] init];
	[GKAchievement resetAchievementsWithCompletionHandler:^(NSError *error)
	 {
		 if (error != nil)
		 {
			 NSLog( @"%@", error );
		 }
	 }];
}


@end

////////////////////////////////////////////

namespace fr
{
	class GameCenterImpl
	{
	public:
		
		GameCenterImpl()
		:	m_gameCenterProxy( [[GameCenterProxy alloc] init] )
		{}
		
		void authenticate()
		{
			[m_gameCenterProxy authenticateLocalPlayer];
		}
		
		bool isAuthenticated() const
		{
			return m_gameCenterProxy && m_gameCenterProxy.gameKitPlayer != nil;
		}
		
		bool hasAchievement( const std::string& achievementName ) const
		{
			return isAuthenticated() && [m_gameCenterProxy hasAchievement:[NSString stringWithUTF8String: achievementName.c_str()]];
		}
		
		void setAchievementProgress( const std::string& achievementName, float proportionDone )
		{
			REQUIRES( 0.0f <= proportionDone );
			
			proportionDone = std::min( 1.0f, proportionDone );		// Clamp to [0,1]
			
			if( isAuthenticated() )
			{
				[m_gameCenterProxy reportAchievementIdentifier: [NSString stringWithUTF8String: achievementName.c_str()] percentComplete: proportionDone * 100.0f];
			}
		}
		
		void updateLeaderboardAttempt( const std::string& leaderboardName, int score )
		{
			if( isAuthenticated() )
			{
				NSString* leaderboardID = [NSString stringWithUTF8String: leaderboardName.c_str()];
				
				GKScore* scoreReporter = [[GKScore alloc] initWithLeaderboardIdentifier: leaderboardID];
				
				scoreReporter.value = score;
				scoreReporter.context = 0;
				
				[GKScore reportScores: @[scoreReporter] withCompletionHandler: nil];
			}
		}
		
		void clearAllAchievementProgress()
		{
			[m_gameCenterProxy clearAllAchievementProgress];
		}

	private:
		
		GameCenterProxy* m_gameCenterProxy;
	};
	
	////////////////////////////////////////////////////////////////////////////////////////
	
	GameCenter::GameCenter()
	:	m_impl( new GameCenterImpl )
	{}
	
	// Just here to desuade std::unique_ptr<> from inlining the call to the (unavailable) GameCenterImpl destructor.
	GameCenter::~GameCenter()
	{}

	void GameCenter::authenticate()
	{
		m_impl->authenticate();
	}
	
	bool GameCenter::isAuthenticated() const
	{
		return m_impl->isAuthenticated();
	}
	
	bool GameCenter::hasAchievement( const std::string& achievementName ) const
	{
		return m_impl->hasAchievement( achievementName );
	}
	
	void GameCenter::setAchievementProgress( const std::string& achievementName, float proportionDone )
	{
		m_impl->setAchievementProgress( achievementName, proportionDone );
	}
	
	void GameCenter::updateLeaderboardAttempt( const std::string& leaderboardName, int score )
	{
		m_impl->updateLeaderboardAttempt( leaderboardName, score );
	}

	void GameCenter::clearAllAchievementProgress()
	{
		m_impl->clearAllAchievementProgress();
	}
}

