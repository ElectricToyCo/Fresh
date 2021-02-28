//
//  FreshGameCenter_Amazon.cpp
//  Fresh
//
//  Created by Jeff Wofford on 11/25/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//
//	Amazon implementation implements GameCircle services.
//  See https://developer.amazon.com/appsandservices/apis/engage/gamecircle

#include "FreshGameCenter.h"

namespace
{
	// TODO This is game-specific!
	const char* const GAMECIRCLE_API_KEY = "eyJhbGciOiJSU0EtU0hBMjU2IiwidmVyIjoiMSJ9.eyJhcHBGYW1pbHlJZCI6ImFtem4xLmFwcGxpY2F0aW9uLjM3NDYwNGQ3ZTM1NzQ3NDhiZDBmMDg4ZGJlNTI3Y2Q4IiwiaWQiOiI4NTQ4OTk3ZC05MDdhLTRlZTAtYWNmYy04YmEwMzZmYWFmNTAiLCJhcHBJZCI6ImFtem4xLmFwcGxpY2F0aW9uLWNsaWVudC5kNTg4NjI0M2I0NTA0ZjY4OWIwOWI2NjZiOWMyYmE4MCIsImVuZHBvaW50cyI6eyJhdXRoeiI6Imh0dHBzOi8vd3d3LmFtYXpvbi5jb20vYXAvb2EiLCJ0b2tlbkV4Y2hhbmdlIjoiaHR0cHM6Ly9hcGkuYW1hem9uLmNvbS9hdXRoL28yL3Rva2VuIn0sImlzcyI6IkFtYXpvbiIsInBrZyI6ImNvLmVsZWN0cmljdG95LmRpZyIsInRydXN0UG9vbCI6bnVsbCwidmVyIjoiMyIsImFwcFZhcmlhbnRJZCI6ImFtem4xLmFwcGxpY2F0aW9uLWNsaWVudC5kNTg4NjI0M2I0NTA0ZjY4OWIwOWI2NjZiOWMyYmE4MCIsInR5cGUiOiJBUElLZXkiLCJpYXQiOiIxNDE2OTM1NTc0NjM1IiwiY2xpZW50SWQiOiJhbXpuMS5hcHBsaWNhdGlvbi1vYTItY2xpZW50LjRmNDUzODk1NjExYjQ2ZjFhOGIwMmRhMTc4MGFhNGFhIiwiYXBwc2lnIjoiNTU6QTM6OUM6NTc6Mzg6Mjc6Rjg6OTg6REI6ODM6M0Y6NUY6REE6Qjc6NUI6OTkifQ==.Gncu3c6Co+vm15P2UqlCF4VQYe9bwjEKfcDdYDOjgKqPTdFo4T4nntt4bWIB/Wmyui9AqBkOcqJkEut7eEeAwg5vi4vHuXonGonUUrmV0JA/njxfUgwlg6N7XfbkIluzYu4rJVlGTfjbOD42ssyRFOOlPM/BWjUWQQU6SYl+LaEJpU9xRmo3qbQsMVQQvyQUxkA+8wxJsANsdZv1Ng7v+hJYOtWUtD93LV17xzH5oaOCiSSqFN44u7Vw4VHVpak0TijYkK/WV3h2x75lgsGEbPqgS0ulRS6RPgTUG9K7+a0giLBxsTEbQvp0mbKcl7hmOSNrZsfJ4xXY2bAqcqzmIw=="
}

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

