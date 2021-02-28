//
//  FreshGameCenter.h
//  Fresh
//
//  Created by Jeff Wofford on 10/7/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_FreshGameCenter_h
#define Fresh_FreshGameCenter_h

#include <string>
#include <memory>

namespace fr
{
	class GameCenterImpl;
	class GameCenter
	{
	public:
		
		GameCenter();
		~GameCenter();
		
		void authenticate();
		// Async. May raise async UI elements in host operating system.

		bool isAuthenticated() const;
		
		bool hasAchievement( const std::string& achievementName ) const;
		
		void setAchievementProgress( const std::string& achievementName, float proportionDone );
		//	If !isAuthenticated(), this function does nothing.
		//	REQUIRES( 0.0f <= proportionDone );
		//  Values of proportionDone > 1.0f will be clamped to 1.0f.
		
		
		void updateLeaderboardAttempt( const std::string& leaderboardName, int score );
		//	If !isAuthenticated(), this function does nothing.
		
		void clearAllAchievementProgress();
		
	private:
		
		std::unique_ptr< GameCenterImpl > m_impl;
		
	};
	
}

#endif
