//
//  Leaderboard.h
//  Fresh
//
//  Created by Jeff Wofford on 12/13/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_Leaderboard_h
#define Fresh_Leaderboard_h

#include "Objects.h"
#include "Archive.h"
#include <map>

namespace fr
{
	
	class LeaderboardServer : public Object
	{
		FRESH_DECLARE_CLASS( LeaderboardServer, Object );
	public:
		
		enum class ScoreComparison
		{
			HigherIsBetter,
			LowerIsBetter
		};
		
		static const size_t NOT_FOUND = ~0UL;
		
		SYNTHESIZE( ScoreComparison, scoreComparison );
		
		virtual size_t proposedRank( const std::string& leaderboardName, int score ) const { return 0; };
		virtual size_t addScore( const std::string& leaderboardName, const std::string& userName, int score, const std::string& otherData, time_t time ) { return 0; }
		
		virtual size_t entryRank( const std::string& leaderboardName, const std::string& userName, int score, const std::string& otherData, time_t time ) const { return NOT_FOUND; }

		virtual size_t numScores( const std::string& leaderboardName ) const { return 0; }
		virtual bool canClearAllScores( const std::string& leaderboardName ) { return false; }
		virtual void clearAllScores( const std::string& leaderboardName ) {}
		
		typedef std::function< void( size_t index, size_t rank, const std::string& username, int score, const std::string& otherData ) > PerScoreFunction;
		
		enum class TraverseOrder
		{
			BestToWorst,
			WorstToBest
		};
		
		virtual void eachScore( const std::string& leaderboardName, size_t iStartScore, size_t maxScores, TraverseOrder traverseOrder, PerScoreFunction&& fn ) const {}

	private:
		
		DVAR( ScoreComparison, m_scoreComparison, ScoreComparison::HigherIsBetter );
	};
	
	FRESH_ENUM_STREAM_IN_BEGIN( LeaderboardServer, ScoreComparison )
	FRESH_ENUM_STREAM_IN_CASE( LeaderboardServer::ScoreComparison, HigherIsBetter )
	FRESH_ENUM_STREAM_IN_CASE( LeaderboardServer::ScoreComparison, LowerIsBetter )
	FRESH_ENUM_STREAM_IN_END()
	
	FRESH_ENUM_STREAM_OUT_BEGIN( LeaderboardServer, ScoreComparison )
	FRESH_ENUM_STREAM_OUT_CASE( LeaderboardServer::ScoreComparison, HigherIsBetter )
	FRESH_ENUM_STREAM_OUT_CASE( LeaderboardServer::ScoreComparison, LowerIsBetter )
	FRESH_ENUM_STREAM_OUT_END()


	/////////////////////////////////////////////////////////////////////////////////////////

	class LocalLeaderboard : public LeaderboardServer
	{
		FRESH_DECLARE_CLASS( LocalLeaderboard, LeaderboardServer);
	public:
		
		virtual size_t proposedRank( const std::string& leaderboardName, int score ) const override;
		virtual size_t addScore( const std::string& leaderboardName, const std::string& userName, int score, const std::string& otherData, time_t time ) override;
		
		virtual size_t numScores( const std::string& leaderboardName ) const override;
		
		virtual bool canClearAllScores( const std::string& leaderboardName ) override;
		virtual void clearAllScores( const std::string& leaderboardName ) override;
		
		virtual size_t entryRank( const std::string& leaderboardName, const std::string& userName, int score, const std::string& otherData, time_t time ) const override;
		
		virtual void eachScore( const std::string& leaderboardName, size_t iStartScore, size_t maxScores, TraverseOrder traverseOrder, PerScoreFunction&& fn ) const override;
		
		static LocalLeaderboard::ptr loadPersistent();

	protected:
		
		void savePersistent();
		
	private:
		
		typedef std::tuple< std::string,		// username
							int,				// score
							std::string,		// other data
							time_t				// time
				> Entry;
		
		typedef std::vector< Entry > Entries;

		typedef std::map< std::string, Entries > LeaderboardEntries;
		
		VAR( LeaderboardEntries, m_leaderboardEntries );

		virtual void sortEntries( Entries& entries ) const;
	};
}

#endif
