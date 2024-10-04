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
		typename TimeT,
		typename ScoreT = float,
		typename NodeSetT = std::set< NodeT >
	>
	class PathFinder
	{
	public:

		using HeuristicEstimateFnT = std::function< ScoreT( NodeT, NodeT ) >;
		using GetNeighborsFnT = std::function< std::vector< NodeT >( NodeT ) >;
		using NodeDistanceFnT = std::function< ScoreT( NodeT, NodeT ) >;
		using GetTimeFnT = std::function< TimeT() >;

		PathFinder( const NodeT& start,
					const NodeT& goal,
					const NodeT& nullNode,
					HeuristicEstimateFnT&& heuristicEstimate,
                    GetNeighborsFnT&& getNeighbors,
					NodeDistanceFnT&& nodeDistance,
					GetTimeFnT&& time
			)
		:	m_start( start )
		,	m_goal( goal )
		,	m_null( nullNode )
		,	m_heuristicEstimate( std::move( heuristicEstimate ))
        ,	m_getNeighbors( std::move( getNeighbors ))
        ,	m_nodeDistance( std::move( nodeDistance ))
		,	m_time( std::move( time ))
        {
            // Assumes that for all nodes, isInClosedSet( n, iVisit ) is initially false.
            //
            assert( m_goal != m_null );

            // Initialize the start node.
            //
            assert( m_start != m_null );
            m_openSet.insert( m_start );

            // Initialize start node scores.
            //
            const auto heuristicScore = m_heuristicEstimate( m_start, m_goal );
            
            m_priorPathNode[ m_start ] = m_null;
            m_scoreG[ m_start ] = ScoreT( 0 );
            m_scoreF[ m_start ] = heuristicScore;
		}

		bool findPath( bool& ranOutOfTime, TimeT timeLimit = TimeT( 0 ) )
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
                auto iterBest = std::min_element( m_openSet.begin(), m_openSet.end(), [this]( const auto& a, const auto& b ) { return getScore( m_scoreF, a ) < getScore( m_scoreF, b ); } );
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
                m_closedSet.insert( best );
				m_openSet.erase( iterBest );

				// Evaluate all neighbors.
				//
                const auto bestG = getScore( m_scoreG, best );
                const auto neighbors = m_getNeighbors( best );
				for( auto neighbor : neighbors )
				{
					assert( best != neighbor );

					if( m_closedSet.find( neighbor ) == m_closedSet.end() )		// Neighbor is not closed.
					{
						const ScoreT tentativeScoreG = bestG + m_nodeDistance( best, neighbor );

                        if( tentativeScoreG < getScore( m_scoreG, neighbor ))
                        {
                            const ScoreT hScore = m_heuristicEstimate( neighbor, m_goal );

                            m_priorPathNode[ neighbor ] = best;
                            m_scoreG[ neighbor ] = tentativeScoreG;
                            m_scoreF[ neighbor ] = tentativeScoreG + hScore;

							m_openSet.insert( neighbor );
                        }
					}

				}	// End initializing neighbors.

			}	// End evaluating open nodes.

			// No path found.
			//
			return false;
		}

        std::vector< NodeT > path() const
        {
            std::vector< NodeT > result;
            
            auto currentNode = m_goal;
            while( currentNode != m_null )
            {
                result.push_back( currentNode );
                currentNode = m_priorPathNode.find( currentNode )->second;
            }
            
            // The path comes back in reverse order. Straighten it out.
            //
            std::reverse( result.begin(), result.end() );
            
            return result;
        }


    protected:

        bool getPriorPathNode( const NodeT& forNode, NodeT& outNode ) const
        {
            const auto iter = m_priorPathNode.find( forNode );
            if( iter != m_priorPathNode.end() )
            {
                outNode = iter->second;
                return true;
            }
            else
            {
                return false;
            }
        }

        ScoreT getScore( const std::unordered_map< NodeT, ScoreT >& map, const NodeT& node ) const
        {
            const auto iter = map.find( node );
            if( iter != map.end() )
            {
                return iter->second;
            }
            else
            {
                return std::numeric_limits< ScoreT >::infinity();
            }
        }

	private:

		NodeT m_start, m_goal, m_null;
		NodeSetT m_openSet;
        NodeSetT m_closedSet;

        std::unordered_map< NodeT, NodeT > m_priorPathNode;
        std::unordered_map< NodeT, ScoreT > m_scoreF;
        std::unordered_map< NodeT, ScoreT > m_scoreG;

		HeuristicEstimateFnT m_heuristicEstimate;
        GetNeighborsFnT m_getNeighbors;
        NodeDistanceFnT m_nodeDistance;
        GetTimeFnT m_time;
	};

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

            const Neighbors& getNeighbors() const { return neighbors; }

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
		const Node::Neighbors& getNeighbors( Node* p );
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
		const Node::Neighbors& getNeighbors( Node* p ) { return p->getNeighbors(); }
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
			double,
			Node::Cost
			> pathFinder( &( nodes[ iStart ] ),
						  &( nodes[ iGoal ] ),
						  nullptr,
						  std::bind( getHeuristicEstimate, _1, _2 ),
                          std::bind( getNeighbors, _1 ),
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
                path = pathFinder.path();
			}
		}
	}
}

#endif
