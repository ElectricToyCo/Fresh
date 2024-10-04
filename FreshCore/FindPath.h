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

	// PathFinder /////////////////////////////////////////////////////////////////////////////////////////////
	//
	// This implementation may seem overcomplicated and laborious to use but the complexity arises from the
	// need for speed. You configure this PathFinder using functors of your own choosing, and the primary
	// goal here is to enable you to store data associated with pathfinding directly in the nodes that
	// the pathfinder examines. This tends to be much faster and somewhat smaller in memory than if the
	// system stored all the data in local `vectors` and `maps`.
	//
	// Along the same lines, this implementation is complicated by the fact that you have to first
	// create a PathFinder object and then call its `pathFind()` function. Why do that? Because this
	// allows you to perform a single pathfinding query over the course of several calls. The benefit
	// is that if each query takes a long time, you can set a time limit on each call and iteratively
	// find the longest path. In essence, you can make pathfinding concurrent with your game.
	//
	// See the "USE CASE" down below the implementation for an example of how to use this system.
	//

	template< typename NodeT,
		typename NeighborRangeT,
		typename TimeT,
		typename ScoreT = float,
		typename NodeSetT = std::set< NodeT >
	>
	class PathFinder
	{
	public:

		using SetPriorPathNodeFnT = std::function< void( NodeT, NodeT ) >;
		using GetScoreFnT = std::function< ScoreT( NodeT ) >;
		using SetScoreFnT = std::function< void( NodeT, ScoreT ) >;
		using HeuristicEstimateFnT = std::function< ScoreT( NodeT, NodeT ) >;
		using IsInClosedSetFnT = std::function< bool( NodeT, size_t ) >;
		using AddToClosedSetFnT = std::function< void( NodeT, size_t ) >;
		using GetNeighborRangeFnT = std::function< NeighborRangeT( NodeT ) >;
		using NodeDistanceFnT = std::function< ScoreT( NodeT, NodeT ) >;
		using GetTimeFnT = std::function< TimeT() >;

		PathFinder( const NodeT& start,
					const NodeT& goal,
					const NodeT& nullNode,
					SetPriorPathNodeFnT&& setPriorPathNode,
				    GetScoreFnT&& getScoreF,
				    GetScoreFnT&& getScoreG,
				    GetScoreFnT&& getScoreH,
				    SetScoreFnT&& setScoreF,
				    SetScoreFnT&& setScoreG,
				    SetScoreFnT&& setScoreH,
					HeuristicEstimateFnT&& heuristicEstimate,
					IsInClosedSetFnT&& isInClosedSet,
					AddToClosedSetFnT&& addToClosedSet,
					GetNeighborRangeFnT&& getNeighborRange,
					NodeDistanceFnT&& nodeDistance,
					GetTimeFnT&& time
			)
		:	m_start( start )
		,	m_goal( goal )
		,	m_null( nullNode )
		,	m_setPriorPathNode( std::move( setPriorPathNode ))
		,	m_getScoreF( std::move( getScoreF ))
        ,	m_getScoreG( std::move( getScoreG ))
        ,	m_getScoreH( std::move( getScoreH ))
        ,	m_setScoreF( std::move( setScoreF ))
        ,	m_setScoreG( std::move( setScoreG ))
		,	m_setScoreH( std::move( setScoreH ))
		,	m_heuristicEstimate( std::move( heuristicEstimate ))
        ,	m_isInClosedSet( std::move( isInClosedSet ))
        ,	m_addToClosedSet( std::move( addToClosedSet ))
        ,	m_getNeighborRange( std::move( getNeighborRange ))
        ,	m_nodeDistance( std::move( nodeDistance ))
		,	m_time( std::move( time ))
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
			m_setPriorPathNode( m_start, m_null );
			m_setScoreG( m_start, ScoreT( 0 ) );
			m_setScoreH( m_start, m_heuristicEstimate( m_start, m_goal ));
			m_setScoreF( m_start, m_getScoreG( m_start ) + m_getScoreH( m_start ));
		}

		bool findPath(bool& ranOutOfTime, TimeT timeLimit = TimeT( 0 ) )
		{
			TimeT startTime = m_time();
			ranOutOfTime = false;

			// Consider the best of each open node.
			//
			while( !m_openSet.empty() )
			{
				// Stop (pause) if out of time.
				//
				if( timeLimit > 0 && m_time() - startTime > timeLimit )
				{
					ranOutOfTime = true;
					break;
				}

				// Find the open node with the best estimated cost (score_f).
				//
				auto iterBest = std::min_element( m_openSet.begin(), m_openSet.end(), [this]( const auto& a, const auto& b ) { return m_getScoreF( a ) < m_getScoreF( b ); } );
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
				m_addToClosedSet( best, s_iVisit );
				m_openSet.erase( iterBest );

				// Evaluate all neighbors.
				//
                const auto bestG = m_getScoreG( best );
				for( auto neighbor : m_getNeighborRange( best ))
				{
					assert( best != neighbor );

					if( !m_isInClosedSet( neighbor, s_iVisit ))		// Neighbor is not closed.
					{
						const ScoreT tentativeScoreG = bestG + m_nodeDistance( best, neighbor );
                        
                        if( tentativeScoreG < m_getScoreG( neighbor ))
                        {
                            const ScoreT hScore = m_heuristicEstimate( neighbor, m_goal );
                            
                            m_setPriorPathNode( neighbor, best );
                            m_setScoreG( neighbor, tentativeScoreG );
                            m_setScoreH( neighbor, hScore );
                            m_setScoreF( neighbor, tentativeScoreG + hScore );

                            if( m_openSet.find( neighbor ) == m_openSet.end() )        // Neighbor is not open.
                            {
                                m_openSet.insert( neighbor );
                            }
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

		SetPriorPathNodeFnT m_setPriorPathNode;
		GetScoreFnT m_getScoreF;
		GetScoreFnT m_getScoreG;
		GetScoreFnT m_getScoreH;
		SetScoreFnT m_setScoreF;
		SetScoreFnT m_setScoreG;
		SetScoreFnT m_setScoreH;
		HeuristicEstimateFnT m_heuristicEstimate;
        IsInClosedSetFnT m_isInClosedSet;
        AddToClosedSetFnT m_addToClosedSet;
        GetNeighborRangeFnT m_getNeighborRange;
        NodeDistanceFnT m_nodeDistance;
        GetTimeFnT m_time;
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
                          std::bind( getScore, _1, 0 ),
						  std::bind( getScore, _1, 1 ),
						  std::bind( getScore, _1, 2 ),
						  std::bind( setScoreF, _1, _2 ),
						  std::bind( setScoreG, _1, _2 ),
						  std::bind( setScoreH, _1, _2 ),
						  std::bind( getHeuristicEstimate, _1, _2 ),
                          std::bind( isInClosedSet, _1, _2 ),
                          std::bind( addToClosedSet, _1, _2 ),
                          std::bind( getNeighborRange, _1 ),
                          std::bind( nodeDistance, _1, _2 ),
						  []() { return 0; }
						  );

			bool needsMoreTime = true;
			bool foundPath = false;
			while( needsMoreTime )
			{
				foundPath = pathFinder.findPath( needsMoreTime );
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
