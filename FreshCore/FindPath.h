//
//  FindPath.h
//  Fresh
//
//  Created by Jeff Wofford on 12/15/11.
//  Copyright (c) 2011 jeffwofford.com. All rights reserved.
//

#ifndef Fresh_FindPath_h
#define Fresh_FindPath_h

#include <set>
#include <vector>
#include <utility>
#include <cassert>

using namespace std::placeholders;

namespace fr
{
	// Helper function for findPath()
	//
	template< typename NodeT, NodeT (*fnGetPriorPathNode)( NodeT ) >
	struct buildPath
	{
		explicit buildPath( NodeT nullNode_ ) : nullNode( nullNode_ ) {}
		
		template< typename IterT >
		void operator()( NodeT currentNode, IterT outPath )
		{
			while( currentNode != nullNode )
			{
				*outPath = currentNode;
				currentNode = fnGetPriorPathNode( currentNode );
				
				++outPath;
			}
		}
		
	private:
		NodeT nullNode;
	};

	// findPath() /////////////////////////////////////////////////////////////////////////////////////////////
	
	template< typename NodeT, 
		typename NeighborRangeT,
		typename TimeT,
		typename ScoreT = float,
		typename NodeSetT = std::set< NodeT >
	>
	class PathFinder
	{
	public:
		
		template<
			typename setPriorPathNodeT,     // void   (*setPriorPathNode)( NodeT, NodeT ),
			typename getScoreGT,			// ScoreT (*getScore)( NodeT ),
			typename getScoreHT,			// ScoreT (*getScore)( NodeT ),
			typename setScoreFT,			// void   (*setScore)( NodeT, ScoreT ),
			typename setScoreGT,			// void   (*setScore)( NodeT, ScoreT ),
			typename setScoreHT,			// void   (*setScore)( NodeT, ScoreT ),
			typename getHeuristicEstimateT
		>
		PathFinder( NodeT start, 
				    NodeT const goal, 
				    NodeT nullNode,
				    setPriorPathNodeT&& setPriorPathNode,
				    getScoreGT&& getScoreG,
				    getScoreHT&& getScoreH,
				    setScoreFT&& setScoreF,
				    setScoreGT&& setScoreG,
				    setScoreHT&& setScoreH,
				    getHeuristicEstimateT&& getHeuristicEstimate
		)
		:	m_start( start )
		,	m_goal( goal )
		,	m_null( nullNode )
		{
			++s_iVisit;
			
			// Assumes that for all nodes, isInClosedSet( n, iVisit ) is initially false.
			//
			assert( m_goal != m_null );
			
			// Initialize the start node.
			//
			assert( m_start != m_null );
			m_openSet.insert( m_start );
			
			// Initialize start node scores.
			//
			setPriorPathNode( m_start, m_null );
			setScoreG( m_start, ScoreT( 0 ) );
			setScoreH( m_start, getHeuristicEstimate( m_start, m_goal ));
			setScoreF( m_start, getScoreG( m_start ) + getScoreH( m_start ));
		}
		
		template<
			typename setPriorPathNodeT,		// void (*setPriorPathNode)( NodeT, NodeT ),
			typename getScoreFT,			// ScoreT (*getScore)( NodeT ),
			typename getScoreGT,			// ScoreT (*getScore)( NodeT ),
			typename getScoreHT,			// ScoreT (*getScore)( NodeT ),
			typename setScoreFT,			// void (*setScore)( NodeT, ScoreT ),
			typename setScoreGT,			// void (*setScore)( NodeT, ScoreT ),
			typename setScoreHT,			// void (*setScore)( NodeT, ScoreT ),
			typename betterScoreFT,			// bool (*betterScoreF)( NodeT, NodeT ),
			typename isInClosedSetT,		// bool (*isInClosedSet)( NodeT, size_t ),
			typename addToClosedSetT,		// void (*addToClosedSet)( NodeT, size_t ),
			typename getNeighborRangeT,		// NeighborRangeT (*getNeighborRange)( NodeT ),
			typename nodeDistanceT,			// ScoreT (*nodeDistance)( NodeT, NodeT ),
			typename getHeuristicEstimateT,	// ScoreT (*getHeuristicEstimate)( NodeT, NodeT ),
			typename getTimeT				// TimeT time(),
		>
		bool findPath( bool& ranOutOfTime, 
					  setPriorPathNodeT&& setPriorPathNode,	
					  getScoreFT&& getScoreF,		
					  getScoreGT&& getScoreG,		
					  getScoreHT&& getScoreH,		
					  setScoreFT&& setScoreF,			
					  setScoreGT&& setScoreG,			
					  setScoreHT&& setScoreH,			
					  betterScoreFT&& betterScoreF,		
					  isInClosedSetT&& isInClosedSet,	
					  addToClosedSetT&& addToClosedSet,	
					  getNeighborRangeT&& getNeighborRange,	
					  nodeDistanceT&& nodeDistance,		
					  getHeuristicEstimateT&& getHeuristicEstimate,
					  getTimeT&& time,
					  TimeT timeLimit = TimeT( 0 ) )
		{			
			TimeT startTime = time();
			ranOutOfTime = false;
			
			// Consider the best of each open node.
			//
			while( !m_openSet.empty() )
			{
				// Stop (pause) if out of time.
				//
				if( timeLimit > 0 && time() - startTime > timeLimit )
				{
					ranOutOfTime = true;
					break;
				}
				
				// Find the open node with the best estimated cost (score_f).
				//
				auto iterBest = std::min_element( m_openSet.begin(), m_openSet.end(), betterScoreF );
				auto best = *iterBest;
				assert( best != m_null );
				
				if( best == m_goal )
				{
					// Found a complete path, now embedded in the nodes.
					// Call buildPath< NodeT, getPriorPathNode >( m_goal, outPath ); to recover.
					//
					return true;
				}
				
				// Move this node from the open to the closed set.
				//
				addToClosedSet( best, s_iVisit );
				m_openSet.erase( iterBest );
				
				// Evaluate all neighbors.
				//
				for( auto neighbor : getNeighborRange( best ))
				{
					assert( best != neighbor );
					
					if( !isInClosedSet( neighbor, s_iVisit ))		// Neighbor is not closed.
					{
						bool tentativeIsBetter = false;
						const ScoreT tentativeScoreG = getScoreG( best ) + nodeDistance( best, neighbor );
						
						if( m_openSet.find( neighbor ) == m_openSet.end() )		// Neighbor is not open.
						{
							m_openSet.insert( neighbor );
							tentativeIsBetter = true;
						}
						else
						{
							tentativeIsBetter = tentativeScoreG < getScoreG( neighbor );
						}
						
						if( tentativeIsBetter )
						{
							const ScoreT hScore = getHeuristicEstimate( neighbor, m_goal );
							
							setPriorPathNode( neighbor, best );
							setScoreG( neighbor, tentativeScoreG );
							setScoreH( neighbor, hScore );
							setScoreF( neighbor, tentativeScoreG + hScore );
						}
					}
					
				}	// End initializing neighbors.			
				
			}	// End evaluating open nodes.
			
			// No path found.
			//
			return false;
		}
		
	private:
		
		NodeT m_start, m_goal, m_null;
		NodeSetT m_openSet;
		static size_t s_iVisit;
	};
	
	// Define s_iVisit ///////////////////////////////
	//
	template< typename NodeT, 
		typename NeighborRangeT,
		typename TimeT,
		typename ScoreT,
		typename NodeSetT
	>
	size_t PathFinder< NodeT, 
		NeighborRangeT, 
		TimeT, 
		ScoreT, 
		NodeSetT >
	::s_iVisit = 0;
	
	///////////////////////////////////////////////////////////////////////////////////
	// USE CASE
	
	namespace findPathExample
	{
		struct Node
		{
			typedef float Cost;
			
			// Graph structure
			//
			typedef std::vector< Node* > Neighbors;
			typedef std::vector< Cost >	 NeighborCosts;
			
			Neighbors neighbors;
			NeighborCosts costs;
						
			Neighbors::iterator getNeighborsBegin() { return neighbors.begin(); }
			Neighbors::iterator getNeighborsEnd()	{ return neighbors.end(); }
			
			struct IterRange : public std::pair< Neighbors::iterator, Neighbors::iterator >
			{
				IterRange( Neighbors::iterator begin_, Neighbors::iterator end_ )
				:	std::pair< Neighbors::iterator, Neighbors::iterator >( begin_, end_ ) {}
				
				Neighbors::iterator begin() { return first; }
				Neighbors::iterator end() { return second; }
			};
			IterRange getNeighborsRange() 			{ return IterRange( getNeighborsBegin(), getNeighborsEnd() ); }

			
			// Pathfinding data and accessors
			//
			size_t iLastVisit = 0;
			Cost scores[ 3 ];		// f, g, and h scores.
			
			Node* priorPathNode = nullptr;
			
			Cost  getScore( size_t type ) const			{ return scores[ type ]; }
			void  setScore( size_t type, Cost score )	{ scores[ type ] = score; }
			
			Node* getPriorPathNode() const						{ return priorPathNode; }
			void  setPriorPathNode( Node* node )				{ priorPathNode = node; }
		};


		Node* getPriorPathNode( Node* p );
		void setPriorPathNode( Node* p, Node* prior );
		Node::Cost getScore( Node* p, size_t type );
		void setScoreF( Node* p, Node::Cost cost );
		void setScoreG( Node* p, Node::Cost cost );
		void setScoreH( Node* p, Node::Cost cost );
		bool betterScoreF( Node* p, Node* q );
		bool isInClosedSet( Node* p, size_t iVisit );
		void addToClosedSet( Node* p, size_t iVisit );
		Node::IterRange getNeighborRange( Node* p );
		Node::Cost nodeDistance( Node* p, Node* q );
		Node::Cost getHeuristicEstimate( Node* p, Node* q );
		double time();

		Node* getPriorPathNode( Node* p ) { return p->getPriorPathNode(); }
		void setPriorPathNode( Node* p, Node* prior ) { return p->setPriorPathNode( prior ); }
		Node::Cost getScore( Node* p, size_t type ) { return p->getScore( type ); }
		void setScoreF( Node* p, Node::Cost cost ) { return p->setScore( 0, cost ); }
		void setScoreG( Node* p, Node::Cost cost ) { return p->setScore( 1, cost ); }
		void setScoreH( Node* p, Node::Cost cost ) { return p->setScore( 2, cost ); }
		bool betterScoreF( Node* p, Node* q )
		{
			return p->getScore( 0 ) < q->getScore( 0 );
		}
		bool isInClosedSet( Node* p, size_t iVisit ) { return p->iLastVisit == iVisit; }
		void addToClosedSet( Node* p, size_t iVisit ) { p->iLastVisit = iVisit; }
		Node::IterRange getNeighborRange( Node* p ) { return p->getNeighborsRange(); }
		Node::Cost nodeDistance( Node* p, Node* q ) { return 0.0f;			/* TODO */ }
		Node::Cost getHeuristicEstimate( Node* p, Node* q ) { return 0.0f;	/* TODO */ }
		double time() { return 0; }	// Unused.
		
		inline void pathfindTest( size_t nNodes, size_t maxNeighbors, Node::Cost maxCost, size_t iStart = 0, size_t iGoal = -1 )
		{
			if( iGoal >= nNodes )
			{
				iGoal = nNodes - 1;
			}
			
			// Create graph.
			//
			std::vector< Node > nodes( nNodes );
			for( size_t iNode = 0; iNode < nNodes; ++iNode )
			{
				// Link to neighbors.
				//
				const size_t nNeighbors = rand() % ( maxNeighbors + 1 );
				nodes[ iNode ].neighbors.resize( nNeighbors );
				nodes[ iNode ].costs.resize( nNeighbors );
				for( size_t iNeighbor = 0; iNeighbor < nNeighbors; ++iNeighbor )
				{
					nodes[ iNode ].neighbors[ iNeighbor ] = &( nodes[ rand() % nNodes ] );
					nodes[ iNode ].costs[ iNeighbor ] = ( rand() / (Node::Cost) RAND_MAX ) * maxCost;
				}
			}
			
			static size_t iVisit = 0;
			++iVisit;
			
			// Find a path through the graph.
			//
			
			PathFinder< 
			Node*,
			Node::IterRange,
			double,
			Node::Cost
			> pathFinder( &( nodes[ iStart ] ), 
						  &( nodes[ iGoal ] ), 
						  nullptr,
                          std::bind( setPriorPathNode, _1, _2 ),
						  std::bind( getScore, _1, 1 ),
						  std::bind( getScore, _1, 2 ),
						  std::bind( setScoreF, _1, _2 ),
						  std::bind( setScoreG, _1, _2 ),
						  std::bind( setScoreH, _1, _2 ),
						  std::bind( getHeuristicEstimate, _1, _2 )
						  );
			
			bool needsMoreTime = true;
			bool foundPath = false;
			while( needsMoreTime )
			{
				foundPath = pathFinder.findPath( needsMoreTime,
												  std::bind( setPriorPathNode, _1, _2 ),
												  std::bind( getScore, _1, 0 ),
												  std::bind( getScore, _1, 1 ),
												  std::bind( getScore, _1, 2 ),
												  std::bind( setScoreF, _1, _2 ),
												  std::bind( setScoreG, _1, _2 ),
												  std::bind( setScoreH, _1, _2 ),
												  std::bind( betterScoreF, _1, _2 ),
												  std::bind( isInClosedSet, _1, _2 ),
												  std::bind( addToClosedSet, _1, _2 ),
												  std::bind( getNeighborRange, _1 ),
												  std::bind( nodeDistance, _1, _2 ),
												  std::bind( getHeuristicEstimate, _1, _2 ),
												  time
		
				);
			}
			
			std::vector< Node* > path;
			if( foundPath )
			{
				buildPath< Node*, &getPriorPathNode > pathBuilder( nullptr );
				pathBuilder( &( nodes[ iGoal ]), std::back_inserter( path ));
			}
		}
	}
}

#endif
