//
//  HighScoreTable.cpp
//  Fresh
//
//  Created by Jeff Wofford on 12/13/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//

#include "HighScoreTable.h"
#include "Leaderboard.h"
#include "TextGroup.h"
#include "TextField.h"

namespace
{
	using namespace fr;
	
	std::string defaultRankToTextConversion( size_t rank )
	{
		std::ostringstream stream;
		stream << ( rank + 1 ) << ".";
		return stream.str();
	}
	
	std::string defaultScoreToTextConversion( int score )
	{
		std::ostringstream stream;
		stream << score;
		return stream.str();
	}
	
	void setRowFieldText( const fr::DisplayObjectContainer& row, const std::string& text, const std::string& fieldSubname )
	{
		if( !text.empty() && !fieldSubname.empty() )
		{
			if( auto textGroup = row.getDescendantByName< TextGroup >( fieldSubname ))
			{
				textGroup->text( text );
			}
			else if( auto textField = row.getDescendantByName< TextField >( fieldSubname ))
			{
				textField->text( text );
			}
		}
	}
	
	inline LeaderboardServer::TraverseOrder intToOrder( int step )
	{
		return step >= 0 ? LeaderboardServer::TraverseOrder::WorstToBest : LeaderboardServer::TraverseOrder::BestToWorst;
	}
}

namespace fr
{	
	FRESH_DEFINE_CLASS( HighScoreTable )
	DEFINE_VAR( HighScoreTable, size_t, m_maxScoresToDisplay );
	DEFINE_VAR( HighScoreTable, ClassInfo::cptr, m_rowClass );
	DEFINE_VAR( HighScoreTable, ClassInfo::cptr, m_highlightedRowClass );
	DEFINE_VAR( HighScoreTable, std::string, m_rankFieldName );
	DEFINE_VAR( HighScoreTable, std::string, m_rowUsernameFieldName );
	DEFINE_VAR( HighScoreTable, std::string, m_rowScoreFieldName );
	DEFINE_VAR( HighScoreTable, std::string, m_rowOtherDataFieldName );
	DEFINE_VAR( HighScoreTable, int, m_order );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( HighScoreTable )
	
	void HighScoreTable::setLeaderboard( SmartPtr< LeaderboardServer > leaderboardServer, const std::string& leaderboardName )
	{
		if( m_leaderboardServer != leaderboardServer || m_leaderboardName != leaderboardName )
		{
			m_leaderboardServer = leaderboardServer;
			m_leaderboardName = leaderboardName;
			
			reload();
		}
	}
	
	void HighScoreTable::reload()
	{
		// Clear the current display.
		//
		removeChildren();
		
		if( m_leaderboardServer )
		{
			// Load at most m_maxScoresToDisplay scores starting with indexOfStartScore
			//
			bool beenHighlighted = false;
			m_leaderboardServer->eachScore( m_leaderboardName,
										   0,
										   m_maxScoresToDisplay,
										   intToOrder( m_order ),
										   [&]( size_t index, size_t rank, const std::string& userName, int score, const std::string& otherData )
										   {
											   const bool highlighted = !beenHighlighted && m_highlightedRank == rank;
											   addRow( index, rank, userName, score, otherData, highlighted );
											   beenHighlighted = beenHighlighted || highlighted;
										   } );
		}
		
		arrangeChildren();
	}

	bool HighScoreTable::wouldPlace( int score ) const
	{
		if( m_leaderboardServer )
		{
			const auto proposedRank = m_leaderboardServer->proposedRank( m_leaderboardName, score );
			return proposedRank < m_maxScoresToDisplay;
		}
		else
		{
			return false;
		}
	}

	size_t HighScoreTable::highlightedIndex() const
	{
		return m_highlightedRank;
	}
	
	void HighScoreTable::clearHighlight()
	{
		m_highlightedRank = ~0UL;
	}
	
	size_t HighScoreTable::addScore( const std::string& userName, int score, const std::string& otherData, bool highlighted )
	{
		ASSERT( m_leaderboardServer );
		
		m_highlightedRank = m_leaderboardServer->addScore( m_leaderboardName, userName, score, otherData, time( nullptr ));
		
		reload();
		
		return m_highlightedRank;
	}
	
	size_t HighScoreTable::numScoresDisplayed() const
	{
		return numChildren();
	}
	
	DisplayObjectContainer::ptr HighScoreTable::addRow( size_t index, size_t rank, const std::string& userName, int score, const std::string& otherData, bool highlighted )
	{
		const std::string rankText = defaultRankToTextConversion( rank );
		
		const std::string scoreText = defaultScoreToTextConversion( score );
		
		if( scoreText.empty() )
		{
			release_warning( "Score of " << score << " converted to empty string." );
		}
		
		auto rowClass = m_rowClass;
		if( highlighted && m_highlightedRowClass )
		{
			rowClass = m_highlightedRowClass;
		}
		
		ASSERT( rowClass );
		
		auto row = createObject< DisplayObjectContainer >( *rowClass );
		ASSERT( row );
		addChild( row );

		setRowFieldText( *row, rankText, m_rankFieldName );
		setRowFieldText( *row, userName, m_rowUsernameFieldName );
		setRowFieldText( *row, scoreText, m_rowScoreFieldName );
		setRowFieldText( *row, otherData, m_rowOtherDataFieldName );
				
		return row;
	}
}

