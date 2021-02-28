//
//  FreshGameCenter_Null.cpp
//  Fresh
//
//  Created by Jeff Wofford on 10/7/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//
//	Null implementation does nothing.

#include "../FreshGameCenter.h"

namespace fr
{
	class GameCenterImpl {};
	
	GameCenter::GameCenter()
	{}
	
	GameCenter::~GameCenter()
	{}

	void GameCenter::authenticate()
	{}
	
	bool GameCenter::isAuthenticated() const
	{
		return false;
	}
	
	bool GameCenter::hasAchievement( const std::string& achievementName ) const
	{
		return false;
	}
	
	void GameCenter::setAchievementProgress( const std::string& achievementName, float proportionDone )
	{}
	
	void GameCenter::updateLeaderboardAttempt( const std::string& leaderboardName, int score )
	{}
	
	void GameCenter::clearAllAchievementProgress()
	{}
}

