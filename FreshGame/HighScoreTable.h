//
//  HighScoreTable.h
//  Fresh
//
//  Created by Jeff Wofford on 12/13/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_HighScoreTable_h
#define Fresh_HighScoreTable_h

#include "UIDisplayGrid.h"

namespace fr
{
	class LeaderboardServer;
	class HighScoreTable : public UIDisplayGrid
	{
		FRESH_DECLARE_CLASS( HighScoreTable, UIDisplayGrid );
	public:
		
		typedef std::function< std::string( int ) > ConversionFn;
		
		SYNTHESIZE_GET( size_t, maxScoresToDisplay );
		
		virtual bool wouldPlace( int score ) const;
		virtual size_t addScore( const std::string& userName, int score, const std::string& otherData = "", bool highlighted = false );

		// Set the function used to convert scores from int to string.
		//
		void setLeaderboard( SmartPtr< LeaderboardServer > leaderboardServer, const std::string& leaderboardName );
		
		void reload();		// From server
		
		size_t numScoresDisplayed() const;
		
		size_t highlightedIndex() const;		// Returns ~0 if no highlight.
		void clearHighlight();
		
	protected:
		
		virtual DisplayObjectContainer::ptr addRow( size_t index, size_t rank, const std::string& userName, int score, const std::string& otherData, bool highlighted );
		
	private:
		
		DVAR( size_t, m_maxScoresToDisplay, 10 );
		VAR( ClassInfo::cptr, m_rowClass );
		VAR( ClassInfo::cptr, m_highlightedRowClass );		
		DVAR( int, m_order, -1 );		// >= 0 worst-to-best, else best-to-worst
		DVAR( std::string, m_rankFieldName, "rank" );
		DVAR( std::string, m_rowUsernameFieldName, "username" );
		DVAR( std::string, m_rowScoreFieldName, "score" );
		DVAR( std::string, m_rowOtherDataFieldName, "other" );

		SmartPtr< LeaderboardServer > m_leaderboardServer;
		std::string m_leaderboardName;
		
		size_t m_highlightedRank = ~0UL;
	};
	
}

#endif
