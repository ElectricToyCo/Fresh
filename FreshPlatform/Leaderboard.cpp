//
//  Leaderboard.cpp
//  Fresh
//
//  Created by Jeff Wofford on 12/13/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//

#include "Leaderboard.h"
#include "FreshFile.h"
#include "Property.h"

namespace
{
	fr::path leaderboardPersistentStorePath()
	{
		auto leaderboardPath = fr::getDocumentPath( "leaderboard.xml" );
		fr::create_directories( leaderboardPath.parent_path() );
		return leaderboardPath;
	}
}

namespace fr
{	
	FRESH_DEFINE_CLASS( LeaderboardServer )
	DEFINE_VAR( LeaderboardServer, ScoreComparison, m_scoreComparison );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( LeaderboardServer )
	
	////////////////////////////////////////////////
	
	FRESH_DEFINE_CLASS( LocalLeaderboard )
	DEFINE_VAR( LocalLeaderboard, LeaderboardEntries, m_leaderboardEntries );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( LocalLeaderboard )
	
	size_t LocalLeaderboard::proposedRank( const std::string& leaderboardName, int score ) const
	{
		auto iter = m_leaderboardEntries.find( leaderboardName );
		if( iter != m_leaderboardEntries.end() )
		{
			const auto& entries = iter->second;
			
			// We assume they're already sorted.
			
			for( size_t place = 0; place < entries.size(); ++place )
			{
				const auto& entry = entries[ place ];
				const auto existingScore = std::get< 1 >( entry );
				
				bool newScoreIsBetter = score > existingScore;
				
				if( scoreComparison() == ScoreComparison::LowerIsBetter )
				{
					newScoreIsBetter = score < existingScore;
				}
				
				if( newScoreIsBetter )
				{
					return place;
				}
			}
			
			return entries.size();
		}
		else
		{
			return 0;
		}
	}
	
	size_t LocalLeaderboard::addScore( const std::string& leaderboardName, const std::string& userName, int score, const std::string& otherData, time_t time )
	{
		auto& entries = m_leaderboardEntries[ leaderboardName ];
		entries.emplace_back( userName, score, otherData, time );
		
		sortEntries( entries );
		
		savePersistent();
		
		return entryRank( leaderboardName, userName, score, otherData, time );
	}
	
	size_t LocalLeaderboard::entryRank( const std::string& leaderboardName, const std::string& userName, int score, const std::string& otherData, time_t time ) const
	{
		const auto iter = m_leaderboardEntries.find( leaderboardName );
		if( iter != m_leaderboardEntries.end() )
		{
			auto& entries = iter->second;
			const auto iterEntry = std::find( entries.begin(), entries.end(), Entry{ userName, score, otherData, time } );
			if( iterEntry != entries.end() )
			{
				return std::distance( entries.begin(), iterEntry );
			}
			else
			{
				return NOT_FOUND;
			}
		}
		else
		{
			return NOT_FOUND;
		}
	}
	
	size_t LocalLeaderboard::numScores( const std::string& leaderboardName ) const
	{
		const auto iter = m_leaderboardEntries.find( leaderboardName );
		if( iter != m_leaderboardEntries.end() )
		{
			return iter->second.size();
		}
		else
		{
			// No such leaderboard.
			return 0;
		}
	}
	
	void LocalLeaderboard::sortEntries( Entries& entries ) const
	{
		auto scoreDelta = []( int scoreA, int scoreB ) { return scoreA - scoreB; };
		
		std::sort( entries.begin(), entries.end(), [&]( const Entry& a, const Entry& b )
				  {
					  const int delta = scoreDelta( std::get< 1 >( a ), std::get< 1 >( b ));
					  
					  if( delta == 0 )
					  {
						  // Compare times. Early scores always beat ties.
						  //
						  return std::get< 3 >( a ) < std::get< 3 >( b );
					  }
					  else
					  {
						  return scoreComparison() == ScoreComparison::LowerIsBetter ? delta < 0 : delta > 0;
					  }
				  } );
	}
	
	bool LocalLeaderboard::canClearAllScores( const std::string& leaderboardName )
	{
		return true;
	}

	void LocalLeaderboard::clearAllScores( const std::string& leaderboardName )
	{
		const auto& iter = m_leaderboardEntries.find( leaderboardName );
		if( iter != m_leaderboardEntries.end() )
		{
			iter->second.clear();
		}
		
		savePersistent();
	}
	
	void LocalLeaderboard::eachScore( const std::string& leaderboardName, size_t iStartScore, size_t maxScores, TraverseOrder traverseOrder, PerScoreFunction&& fn ) const
	{
		auto iEndScore = iStartScore + maxScores;
		
		if( iEndScore > iStartScore )
		{
			const auto& iter = m_leaderboardEntries.find( leaderboardName );
			if( iter != m_leaderboardEntries.end() )
			{
				const auto& entries = iter->second;
				ptrdiff_t step = 1;
				
				if( traverseOrder == TraverseOrder::WorstToBest )
				{
					// Traverse backward.
					//
					step = -1;
					
					iStartScore = entries.size() - iStartScore - 1;
					iEndScore = iStartScore - maxScores;
					if( iEndScore > iStartScore )
					{
						iEndScore = -1;
					}
				}
				
				for( size_t index = 0, rank = iStartScore; rank < entries.size() && rank != iEndScore; rank += step, ++index )
				{
					const auto& entry = entries[ rank ];
					fn( index, rank, std::get< 0 >( entry ), std::get< 1 >( entry ), std::get< 2 >( entry ));
				}
			}
		}
	}
	
	LocalLeaderboard::ptr LocalLeaderboard::loadPersistent()
	{
		const auto path = leaderboardPersistentStorePath();
	
		LocalLeaderboard::ptr leaderboard;
		try
		{
			Package::ptr package = loadPackage( path );
			package->forEachMember( [&]( const Object::ptr& object )
								   {
									   if( !leaderboard )
									   {
										   leaderboard = object->as< LocalLeaderboard >();
									   }
								   } );
		}
		catch( const std::exception& e )
		{
			dev_warning( "Could not open leaderboard persistent store at '" << path << "'. Creating a clear one." );
			leaderboard = createObject< LocalLeaderboard >( getTransientPackage(), "~leaderboard" );
		}
		
		return leaderboard;
	}
	
	void LocalLeaderboard::savePersistent()
	{
		Package::ptr package = createPackage( "~leaderboard" );
		package->add( this );
		package->save( leaderboardPersistentStorePath() );
	}
}
