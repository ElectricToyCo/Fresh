//
//  FreshCloth.h
//  Fresh
//
//  Created by Jeff Wofford on 3/14/13.
//
//

#ifndef Fresh_FreshCloth_h
#define Fresh_FreshCloth_h

#include <vector>
#include <cassert>
#include <cmath>
#include <limits>
#include <functional>

namespace fr
{
	namespace cloth
	{
		template< typename VecT, typename RealT >
		struct Node
		{
			typedef VecT vec_t;
			typedef RealT real_t;

			vec_t lastPosition		= vec_t( 0, 0, 0 );
			vec_t position			= vec_t( 0, 0, 0 );
			vec_t acceleration		= vec_t( 0, 0, 0 );
			
			vec_t normal			= vec_t( 0, 0, 1 );
			vec_t texCoord			= vec_t( 0, 0, 0 );
			
			bool pinned				= false;
			
			std::vector< Node* > m_neighboringVertices;
			std::vector< Node* > m_neighboringNormals;
			
			vec_t velocity() const;
			
			template< typename time_t >
			void update( time_t deltaTime );
			void updateNormal();
			
			size_t numTriangleVertices() const;
			
			template< typename geometry_real_t >
			void getTriangles( geometry_real_t*& positions, geometry_real_t*& normals = nullptr, geometry_real_t*& texCoords = nullptr, ptrdiff_t strideBytes = 0 ) const;
			// If strideBytes == 0, strideBytes is inferred from the sizeof( geometry_real_t ) and the number of attributes requested (i.e. that are non-null).
			
		protected:
			
			template< typename geometry_real_t >
			void emitVertex( ptrdiff_t strideBytes, geometry_real_t*& positions, geometry_real_t*& normals = nullptr, geometry_real_t*& texCoords = nullptr ) const;
		};
		
		//////////////////////////////////////////////////////////////////////////////////////////////////
		
		template< typename NodeT >
		struct Constraint
		{
			typedef NodeT node_t;
			typedef typename NodeT::vec_t vec_t;
			typedef typename NodeT::real_t real_t;
			
			node_t* a = nullptr;
			node_t* b = nullptr;
			real_t restDistance = 0;
			
			Constraint( node_t* a_, node_t* b_ ) : a( a_ ), b( b_ ) {}
			
			void finishInitialization();
			
			void apply();
			void addNormals() const;
		};
		
		//////////////////////////////////////////////////////////////////////////////////////////////////
		
		template< size_t (*fnIndexer)( size_t, size_t, size_t ), typename NodeT, typename ConstraintT = Constraint< NodeT > >
		class NodeMesh
		{
		public:

			typedef NodeT node_t;
			typedef ConstraintT constraint_t;
			
			typedef typename node_t::vec_t vec_t;
			typedef typename node_t::real_t real_t;
			
			NodeMesh( size_t nCols, std::vector< node_t >&& nodes, std::vector< constraint_t >&& constraints );
			
			void pin( size_t x, size_t y );
			void unpin( size_t x, size_t y );
			
			template< typename fnPerNode >
			void forEachNode( fnPerNode&& fn );
			
			template< typename fnPerNode >
			void forEachNode( fnPerNode&& fn ) const;
			
			template< typename fnPerConstraint >
			void forEachConstraint( fnPerConstraint&& fn );
			
			template< typename fnPerConstraint >
			void forEachConstraint( fnPerConstraint&& fn ) const;
			
		private:
			
			size_t m_nCols = 0;

			std::vector< node_t > m_nodes;
			std::vector< constraint_t > m_constraints;			

			size_t index( size_t x, size_t y ) const;
			node_t& getNode( size_t x, size_t y );
			const node_t& getNode( size_t x, size_t y ) const;
		};
		
		//////////////////////////////////////////////////////////////////////////////////////////////////
		
		size_t indexRectangular( size_t x, size_t y, size_t nCols );
		
		template< typename NodeT, typename ConstraintT = Constraint< NodeT >>
		NodeMesh< indexRectangular, NodeT, ConstraintT > createRectangularCloth( size_t xNodes,
																				size_t yNodes,
																				typename NodeT::real_t width,
																				typename NodeT::real_t height,
																				typename NodeT::real_t taperProportion = typename NodeT::real_t( 1.0 ));

		size_t indexTriangular( size_t x, size_t y, size_t nCols );
		
		template< typename NodeT, typename ConstraintT = Constraint< NodeT >>
		NodeMesh< indexTriangular, NodeT, ConstraintT > createTriangularCloth( size_t sideLength,
																				typename NodeT::real_t width,
																				typename NodeT::real_t height );

		//////////////////////////////////////////////////////////////////////////////////////////////////
		
		template< typename NodeMeshT, typename time_t = double >
		class Simulator
		{
		public:
			
			typedef NodeMeshT node_mesh_t;
			typedef typename node_mesh_t::vec_t vec_t;
			typedef typename node_mesh_t::real_t real_t;
			typedef typename node_mesh_t::node_t node_t;
			typedef typename node_mesh_t::constraint_t constraint_t;
			
			Simulator( node_mesh_t&& clothStorage,
				  const vec_t& gravity_ = vec_t( 0, 0, 0 ),
				  const vec_t& windForce_ = vec_t( 0, 0, 0 ),
				  unsigned int nConstraintIterations = 1 );
			
			void windForce( const vec_t& f );
			const vec_t& windForce() const;

			void gravity( const vec_t& g );
			const vec_t& gravity() const;

			void update( time_t time );
			
			size_t numTriangleVertices() const;
			
			template< typename geometry_real_t >
			void getTriangles( geometry_real_t* positions, geometry_real_t* normals = nullptr, geometry_real_t* texCoords = nullptr, ptrdiff_t strideBytes = 0 ) const;

			node_mesh_t& nodeMesh();
			
		protected:
			
			void applyClothConstraints();
			void updateNormals();
			
		private:
			
			node_mesh_t m_nodeMesh;
			
			vec_t m_gravity;
			vec_t m_windForce;
			
			time_t m_lastTime = std::numeric_limits< time_t >::infinity();
			
			size_t m_nConstraintIterations = 1;
			
			mutable size_t m_nTriangleVerts = 0;	// Cached.
		};
		
		////////////////////////////////////////////////////////////////////////////////////////////
		// IMPLEMENTATION
		//
		
		/////////////////////////////////////////////////////////////////////////
		// NODE
		
		template< typename VecT, typename RealT >
		VecT Node< VecT, RealT >::velocity() const
		{
			return position - lastPosition;
		}
		
		template< typename VecT, typename RealT >
		template< typename time_t >
		void Node< VecT, RealT >::update( time_t deltaTime )
		{
			vec_t savedPosition = position;
			
			if( !pinned )
			{
				// Verlet integration.
				position += ( position - lastPosition ) + acceleration * static_cast< real_t >( 0.5 * deltaTime * deltaTime );
			}
			
			acceleration = vec_t( 0, 0, 0 );
			lastPosition = savedPosition;
		}
		
		template< typename VecT, typename RealT >
		void Node< VecT, RealT >::updateNormal()
		{
			normal = vec_t( 0, 0, 0 );
			
			for( size_t i = 0; i + 1 < m_neighboringNormals.size(); ++i )
			{
				normal += ( m_neighboringNormals[ i ]->position - position ).cross( m_neighboringNormals[ ( i + 1 ) ]->position - position );
			}
			
			normal.normalize();
		}
		
		template< typename VecT, typename RealT >
		size_t Node< VecT, RealT >::numTriangleVertices() const
		{
			return m_neighboringVertices.size() + size_t( m_neighboringVertices.size() * 0.5 + 0.5 );	// ceiling
		}
		
		template< typename VecT, typename RealT >
		template< typename geometry_real_t >
		void Node< VecT, RealT >::getTriangles( geometry_real_t*& positions, geometry_real_t*& normals, geometry_real_t*& texCoords, ptrdiff_t strideBytes ) const
		{
			if( strideBytes == 0 )
			{
				strideBytes = sizeof( geometry_real_t ) * ( ( positions ? 3 : 0 )	// For positions
														   +	( normals ? 3 : 0 )	// For normals
														   +	( texCoords ? 2 : 0 ) // For texcoords.
														   );
			}
			
			size_t pairCount = 0;
			for( auto neighbor : m_neighboringVertices )
			{
				if( pairCount == 0 )
				{
					emitVertex( strideBytes, positions, normals, texCoords );
				}
				
				neighbor->emitVertex( strideBytes, positions, normals, texCoords );
				
				pairCount = ( pairCount + 1 ) % 3;
			}
		}

		template< typename VecT, typename RealT >
		template< typename geometry_real_t >
		void Node< VecT, RealT >::emitVertex( ptrdiff_t strideBytes, geometry_real_t*& positions, geometry_real_t*& normals, geometry_real_t*& texCoords ) const
		{
			char* positionsBytes = reinterpret_cast< char* >( positions );
			char* normalsBytes = reinterpret_cast< char* >( normals );
			char* texCoordsBytes = reinterpret_cast< char* >( texCoords );
			
			// Assign each component of each attribute.
			//
			for( int i = 0; i < 3; ++i )
			{
				if( positions )
				{
					*positions++ = position[ i ];
				}
				
				if( normals )
				{
					*normals++ = normal[ i ];
				}
				
				if( texCoords && i < 2 )
				{
					*texCoords++ = texCoord[ i ];
				}
			}

			// Advance the raw pointers by the stride.
			//
			if( positions )
			{
				positions = reinterpret_cast< geometry_real_t* >( positionsBytes + strideBytes );
			}

			if( normals )
			{
				normals = reinterpret_cast< geometry_real_t* >( normalsBytes + strideBytes );
			}
			
			if( texCoords )
			{
				texCoords = reinterpret_cast< geometry_real_t* >( texCoordsBytes + strideBytes );
			}
		}

		
		/////////////////////////////////////////////////////////////////////////
		// CONSTRAINT
		
		template< typename NodeT >
		void Constraint< NodeT >::finishInitialization()
		{
			assert( a && b );
			restDistance = ( a->position - b->position ).length();
		}
		
		template< typename NodeT >
		void Constraint< NodeT >::Constraint::apply()
		{
			assert( a && b );
			
			const vec_t posDelta = a->position - b->position;
			
			const real_t actualDistance = posDelta.length();
			
			vec_t adjustment = posDelta.normal() * ( restDistance - actualDistance );
			
			// Push coterminal nodes apart a little.
			//
			if( actualDistance < std::numeric_limits< real_t >::epsilon() )
			{
				adjustment += vec_t( 0.1, 0.1, 0.1 );
			}
			
			//
			// Apply adjustment.
			//
			
			// Apply equally if neither node is pinned. Else all to one node or to no node at all.
			//
			if( !a->pinned && !b->pinned )
			{
				adjustment *= 0.5;
			}
			
			if( !a->pinned )
			{
				a->position += adjustment;
			}
			
			if( !b->pinned )
			{
				b->position -= adjustment;
			}
		}
		
		/////////////////////////////////////////////////////////////////////////
		// NODE MESH
		
		template< size_t (*fnIndexer)( size_t, size_t, size_t ), typename NodeT, typename ConstraintT >
		NodeMesh< fnIndexer, NodeT, ConstraintT >::NodeMesh( size_t nCols, std::vector< node_t >&& nodes, std::vector< constraint_t >&& constraints )
		:	m_nCols( nCols )
		,	m_nodes( std::forward< std::vector< node_t >>( nodes ))
		,	m_constraints( std::forward< std::vector< constraint_t >>( constraints ))
		{}
		
		template< size_t (*fnIndexer)( size_t, size_t, size_t ), typename NodeT, typename ConstraintT >
		void NodeMesh< fnIndexer, NodeT, ConstraintT >::pin( size_t x, size_t y )
		{
			getNode( x, y ).pinned = true;
		}
		
		template< size_t (*fnIndexer)( size_t, size_t, size_t ), typename NodeT, typename ConstraintT >
		void NodeMesh< fnIndexer, NodeT, ConstraintT >::unpin( size_t x, size_t y )
		{
			getNode( x, y ).pinned = false;
		}
		
		template< size_t (*fnIndexer)( size_t, size_t, size_t ), typename NodeT, typename ConstraintT >
		template< typename fnPerNode >
		void NodeMesh< fnIndexer, NodeT, ConstraintT >::forEachNode( fnPerNode&& fn )
		{
			for( auto& node : m_nodes )
			{
				fn( node );
			}
		}
		
		template< size_t (*fnIndexer)( size_t, size_t, size_t ), typename NodeT, typename ConstraintT >
		template< typename fnPerNode >
		void NodeMesh< fnIndexer, NodeT, ConstraintT >::forEachNode( fnPerNode&& fn ) const
		{
			for( const auto& node : m_nodes )
			{
				fn( node );
			}
		}
		
		template< size_t (*fnIndexer)( size_t, size_t, size_t ), typename NodeT, typename ConstraintT >
		template< typename fnPerConstraint >
		void NodeMesh< fnIndexer, NodeT, ConstraintT >::forEachConstraint( fnPerConstraint&& fn )
		{
			for( auto& constraint : m_constraints )
			{
				fn( constraint );
			}
		}

		template< size_t (*fnIndexer)( size_t, size_t, size_t ), typename NodeT, typename ConstraintT >
		template< typename fnPerConstraint >
		void NodeMesh< fnIndexer, NodeT, ConstraintT >::forEachConstraint( fnPerConstraint&& fn ) const
		{
			for( const auto& constraint : m_constraints )
			{
				fn( constraint );
			}
		}
		
		template< size_t (*fnIndexer)( size_t, size_t, size_t ), typename NodeT, typename ConstraintT >
		size_t NodeMesh< fnIndexer, NodeT, ConstraintT >::index( size_t x, size_t y ) const
		{
			return fnIndexer( x, y, m_nCols );
		}
		
		template< size_t (*fnIndexer)( size_t, size_t, size_t ), typename NodeT, typename ConstraintT >
		NodeT& NodeMesh< fnIndexer, NodeT, ConstraintT >::getNode( size_t x, size_t y )
		{
			return m_nodes[ index( x, y )];
		}
		
		template< size_t (*fnIndexer)( size_t, size_t, size_t ), typename NodeT, typename ConstraintT >
		const NodeT& NodeMesh< fnIndexer, NodeT, ConstraintT >::getNode( size_t x, size_t y ) const
		{
			return m_nodes[ index( x, y )];
		}
		

		/////////////////////////////////////////////////////////////////////////
		// CLOTH RECTANGULAR
		
		inline size_t indexRectangular( size_t x, size_t y, size_t nCols )
		{
			return x + y * nCols;
		}
		
		template< typename NodeT, typename ConstraintT >
		NodeMesh< indexRectangular, NodeT, ConstraintT > createRectangularCloth( size_t xNodes,
																				size_t yNodes,
																				typename NodeT::real_t width,
																				typename NodeT::real_t height,
																				typename NodeT::real_t taperProportion )
		{
			typedef typename NodeT::vec_t vec_t;
			typedef typename NodeT::real_t real_t;
			typedef NodeMesh< indexRectangular, NodeT, ConstraintT > node_mesh_t;
			
			assert( xNodes > 1 && yNodes > 1 );
			
			const size_t nCols = xNodes;
			
			std::vector< NodeT > nodes( xNodes * yNodes );
			std::vector< ConstraintT > constraints;
			
			const real_t yMidpoint = height * real_t( 0.5 );
			
			// Initialize node positions and create constraints.
			//
			for( size_t y = 0; y < yNodes; ++y )
			{
				const real_t rowProportion = real_t( y ) / ( yNodes - 1 );

				for( size_t x = 0; x < xNodes; ++x )
				{
					const real_t colProportion = real_t( x ) / ( xNodes - 1 );
					
					const real_t colHeightProportion = ( real_t( 1 ) - ( colProportion * ( real_t( 1 ) - taperProportion )));
					
					NodeT& node = nodes.at( indexRectangular( x, y, nCols ));
					node.position.x = width * colProportion;
					node.position.y = yMidpoint + height * ( rowProportion - real_t( 0.5 )) * colHeightProportion;
					node.lastPosition = node.position;
					
					node.texCoord.x = colProportion;
					node.texCoord.y = rowProportion;

					// All but the first row/col:
					//
					if( x > 0 && y > 0 )
					{
						node.m_neighboringVertices.push_back( &nodes.at( indexRectangular( x - 1, y    , nCols )));				// West
						node.m_neighboringVertices.push_back( &nodes.at( indexRectangular( x    , y - 1, nCols )));				// North
					}
					
					// All but last row/col:
					//
					if( x + 1 < xNodes && y + 1 < yNodes )
					{
						constraints.emplace_back( ConstraintT{ &node, &nodes.at( indexRectangular( x + 1, y    , nCols )) } );	// East
						constraints.emplace_back( ConstraintT{ &node, &nodes.at( indexRectangular( x + 1, y + 1, nCols )) } );	// Southeast
						
						node.m_neighboringVertices.push_back( &nodes.at( indexRectangular( x + 1, y    , nCols )));				// East
						node.m_neighboringVertices.push_back( &nodes.at( indexRectangular( x    , y + 1, nCols )));				// South
					}
					
					// All but the last row
					//
					if( y + 1 < yNodes )
					{
						constraints.emplace_back( ConstraintT{ &node, &nodes.at( indexRectangular( x    , y + 1, nCols )) } );		// South
					}
					
					// All but the first column and last row:
					//
					if( x > 0  && y + 1 < yNodes )
					{
						constraints.emplace_back( ConstraintT{ &node, &nodes.at( indexRectangular( x - 1, y + 1, nCols )) } );		// Southwest
					}
					
					node.m_neighboringNormals = node.m_neighboringVertices;
				}
			}
			
			// Finish initializing constraints.
			//
			for( auto& constraint : constraints )
			{
				constraint.finishInitialization();
			}
			
			return node_mesh_t( nCols, std::move( nodes ), std::move( constraints ) );
		}

		/////////////////////////////////////////////////////////////////////////
		// CLOTH TRIANGULAR
		
		inline size_t triangularNumber( size_t i )
		{
			size_t n = 0;
			while( i > 0 )
			{
				n += i;
				--i;
			}
			return n;
		}
		
		inline size_t rowLength( size_t y, size_t nCols )
		{
			return nCols - y;
		}
		
		inline size_t indexTriangular( size_t x, size_t y, size_t nCols )
		{
			// TODO constant time way to do this?
			//
			size_t rowOffset = 0;
			while( y > 0 )
			{
				rowOffset += nCols - y + 1;
				--y;
			}
			
			return x + rowOffset;
		}
		
		template< typename NodeT, typename ConstraintT >
		NodeMesh< indexTriangular, NodeT, ConstraintT > createTriangularCloth( size_t sideLength,
																			  typename NodeT::real_t width,
																			  typename NodeT::real_t height )
		{
			typedef typename NodeT::vec_t vec_t;
			typedef typename NodeT::real_t real_t;
			typedef NodeMesh< indexTriangular, NodeT, ConstraintT > node_mesh_t;
			
			assert( sideLength > 1);
			
			std::vector< NodeT > nodes( triangularNumber( sideLength ));
			std::vector< ConstraintT > constraints;
			
			const real_t xMidpoint = width * real_t( 0.5 );
			
			// Initialize node positions and create constraints.
			//
			for( size_t y = 0; y < sideLength; ++y )
			{
				const real_t rowProportion = real_t( y ) / ( sideLength - 1 );
				const real_t rowWidthProportion = real_t( 1 ) - rowProportion;
				const auto nColsInRow = rowLength( y, sideLength );
				const auto nColsInNextRow = rowLength( y + 1, sideLength );
				
				for( size_t x = 0; x < nColsInRow; ++x )
				{
					const real_t colProportion = real_t( x ) / ( nColsInRow - 1 );
					
					NodeT& node = nodes.at( indexTriangular( x, y, sideLength ));
					node.position.x = xMidpoint + width * ( colProportion - real_t( 0.5 )) * rowWidthProportion;
					node.position.y = height * rowProportion;
					node.lastPosition = node.position;
					
					node.texCoord.x = colProportion;
					node.texCoord.y = rowProportion;
					
					// Establish hexagonal relationships: W, NW, NE, E, SE, SW (thinking in grid-like terms).

					if( x > 0 )
					{
						node.m_neighboringNormals.push_back( &nodes.at( indexTriangular( x - 1, y    , sideLength )));				// West
					}
					
					if( y > 0 )
					{
						node.m_neighboringNormals.push_back( &nodes.at( indexTriangular( x    , y - 1, sideLength )));				// North

						if( x > 0 )
						{
							node.m_neighboringVertices.push_back( &nodes.at( indexTriangular( x - 1, y    , sideLength )));			// West
							node.m_neighboringVertices.push_back( &nodes.at( indexTriangular( x    , y - 1, sideLength )));			// North
						}
					}
					
					if( x + 1 < nColsInRow )
					{
						node.m_neighboringNormals.push_back( &nodes.at( indexTriangular( x + 1, y    , sideLength )));				// East
						constraints.emplace_back( ConstraintT{ &node, &nodes.at( indexTriangular( x + 1, y    , sideLength )) } );	// East
					}
					
					if( x < nColsInNextRow && y + 1 < sideLength )
					{
						node.m_neighboringNormals.push_back( &nodes.at( indexTriangular( x    , y + 1, sideLength )));				// South
						constraints.emplace_back( ConstraintT{ &node, &nodes.at( indexTriangular( x    , y + 1, sideLength )) } );	// South

						if( x + 1 < nColsInRow)
						{
							node.m_neighboringVertices.push_back( &nodes.at( indexTriangular( x + 1, y    , sideLength )));				// East
							node.m_neighboringVertices.push_back( &nodes.at( indexTriangular( x    , y + 1, sideLength )));				// South
						}
					}
					
					if( x > 0 && y + 1 < sideLength )
					{
						node.m_neighboringNormals.push_back( &nodes.at( indexTriangular( x - 1, y + 1, sideLength )));				// Southwest
					}

					if( x + 1 < nColsInRow && y > 0 )
					{
						node.m_neighboringNormals.push_back( &nodes.at( indexTriangular( x + 1, y - 1, sideLength )));				// Northeast
						constraints.emplace_back( ConstraintT{ &node, &nodes.at( indexTriangular( x + 1, y - 1, sideLength )) } );	// Northeast
					}
				}
			}
			
			// Finish initializing constraints.
			//
			for( auto& constraint : constraints )
			{
				constraint.finishInitialization();
			}
			
			return node_mesh_t( sideLength, std::move( nodes ), std::move( constraints ) );
		}

		/////////////////////////////////////////////////////////////////////////
		// CLOTH SIMULATOR
		
		template< typename NodeMeshT, typename time_t >
		Simulator< NodeMeshT, time_t >::Simulator( node_mesh_t&& clothStorage,
												 const vec_t& gravity_,
												 const vec_t& f,
												 unsigned int nConstraintIterations )
		:	m_nodeMesh( std::forward< node_mesh_t >( clothStorage ))
		,	m_gravity( gravity_ )
		,	m_windForce( f )
		,	m_nConstraintIterations( nConstraintIterations )
		{}
		
		template< typename NodeMeshT, typename time_t >
		void Simulator< NodeMeshT, time_t >::windForce( const vec_t& f )
		{
			m_windForce = f;
		}
		
		template< typename NodeMeshT, typename time_t >
		const typename Simulator< NodeMeshT, time_t >::vec_t& Simulator< NodeMeshT, time_t >::windForce() const
		{
			return m_windForce;
		}
		
		template< typename NodeMeshT, typename time_t >
		void Simulator< NodeMeshT, time_t >::gravity( const vec_t& g )
		{
			m_gravity = g;
		}

		template< typename NodeMeshT, typename time_t >
		const typename Simulator< NodeMeshT, time_t >::vec_t& Simulator< NodeMeshT, time_t >::gravity() const
		{
			return m_gravity;
		}

		template< typename NodeMeshT, typename time_t >
		void Simulator< NodeMeshT, time_t >::applyClothConstraints()
		{
			m_nodeMesh.forEachConstraint( std::mem_fn( &constraint_t::apply ));			
		}
		
		template< typename NodeMeshT, typename time_t >
		void Simulator< NodeMeshT, time_t >::update( time_t time )
		{
			time_t deltaTime = 0;
			
			if( m_lastTime < time )	// Not true initially, because lastTime is huge.
			{
				deltaTime = time - m_lastTime;
			}
			
			const auto windNormal = m_windForce.normal();
			
			// Update nodes.
			//
			m_nodeMesh.forEachNode( [&] ( node_t& node )
			{
				// Apply gravity.
				//
				node.acceleration += m_gravity;
				
				// Apply wind.
				//
				node.acceleration += m_windForce * std::max( std::abs( windNormal.dot( -node.normal )), 0.05f );
				
				// Update integration.
				//
				node.update( deltaTime );
			} );
			
			// Apply constraints.
			//
			for( unsigned int iConstraintUpdate = 0; iConstraintUpdate < m_nConstraintIterations; ++iConstraintUpdate )
			{
				applyClothConstraints();
			}
			
			updateNormals();
			
			m_lastTime = time;
		}

		template< typename NodeMeshT, typename time_t >
		size_t Simulator< NodeMeshT, time_t >::numTriangleVertices() const
		{
			if( m_nTriangleVerts == 0 )	// Previously calculated?
			{
				// No. Calculate and cache.
				m_nodeMesh.forEachNode( [&] ( const node_t& node )
				{
					m_nTriangleVerts += node.numTriangleVertices();
				} );
			}
			
			return m_nTriangleVerts;
		}
		
		template< typename NodeMeshT, typename time_t >
		template< typename geometry_real_t >
		void Simulator< NodeMeshT, time_t >::getTriangles( geometry_real_t* positions, geometry_real_t* normals, geometry_real_t* texCoords, ptrdiff_t strideBytes ) const
		{
			m_nodeMesh.forEachNode( [&] ( const node_t& node )
			{
				node.getTriangles( positions, normals, texCoords, strideBytes );
			} );
		}
		
		template< typename NodeMeshT, typename time_t >
		NodeMeshT& Simulator< NodeMeshT, time_t >::nodeMesh()
		{
			return m_nodeMesh;
		}
		
		template< typename NodeMeshT, typename time_t >
		void Simulator< NodeMeshT, time_t >::updateNormals()
		{
			m_nodeMesh.forEachNode( std::mem_fn( &node_t::updateNormal ));
		}
	}
}

#endif
