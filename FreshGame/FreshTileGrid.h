//
//  FreshTileGrid.h
//  Fresh
//
//  Created by Jeff Wofford on 12/3/11.
//  Copyright (c) 2011 jeffwofford.com. All rights reserved.
//

#ifndef Fresh_TileGrid_h
#define Fresh_TileGrid_h

#include "Texture.h"
#include "DisplayObjectWithMesh.h"
#include "FreshMath.h"
#include "Grid2.h"
#include "Segment.h"

namespace fr
{
	
	class TileTemplate : public Object
	{
		FRESH_DECLARE_CLASS( TileTemplate, Object )
	public:
		
		SYNTHESIZE( const Vector2i&, atlasSubTexture );
		SYNTHESIZE( bool, isSolid );
		SYNTHESIZE( bool, doesRender );
		SYNTHESIZE( bool, doesBlockLight );
		SYNTHESIZE_GET( Color, creationColor );
		
	private:
		
		DVAR( Vector2i, m_atlasSubTexture, Vector2i::ZERO );
		DVAR( bool, m_isSolid, true );
		DVAR( bool, m_doesRender, true );
		DVAR( bool, m_doesBlockLight, true );
		DVAR( Color, m_creationColor, Color::White );
	};

	/////////////////////////////////////////////////////////////////////////////////// 
	
	class Tile : public EventDispatcher
	{
		FRESH_DECLARE_CLASS( Tile, EventDispatcher )
	public:
		
		enum class Solidity
		{
			Inherit,		// Inherit from template.
			Empty,
			Solid
		};
		
		enum class Navigability
		{
			Inherit,		// Inherit from isSolid().
			Navigable,
			Unnavigable
		};
		
		SYNTHESIZE( bool, doDispatchEvents );
		
		SYNTHESIZE( TileTemplate::ptr, tileTemplate );
		
		bool isSolid() const;
		void isSolid( Solidity solidity )						{ m_solidity = solidity; }
		
		bool doesBlockLight() const;
		
		bool isNavigable( Direction fromDirection ) const;
		void isNavigable( Navigability navigability );
		void isNavigable( Navigability navigability, Direction fromDirection );
		
		virtual real navDistanceScalar() const { return 1.0f; }	// Cost of moving into this tile.
		
		// Lighting ///////////////////////////
		//
		void addStaticLightBlocker( const Segment& blocker );
		void clearStaticLightBlockers()							{ m_staticLightBlockers.clear(); }
		size_t nStaticLightBlockers() const						{ return m_staticLightBlockers.size(); }
		size_t capacityStaticLightBlockers() const				{ return m_staticLightBlockers.capacity(); }
		void finalizeStaticLightBlockers();
		
		bool getShadowTriangles( const vec2& lightPos, real lightRadius, std::vector< vec2 >& outTrianglePoints, real lightSpotHalfArc = 0, const vec2& lightSpotDirection = vec2::ZERO ) const;
		// REQUIRES( lightRadius > 0 );
		// PROMISES NOTHING. No triangles are produced if the lightPos is embedded
		// in a blocking tile.
		// Returns false iff the lightPos is inside a blocking tile.

		
		// Navigation /////////////////////////
		//
		SYNTHESIZE( const Vector2i&, priorPathNode );
		real getPathScore( unsigned int type ) const		{ ASSERT( type < 3 );	return m_pathScores[ type ]; }
		void setPathScore( unsigned int type, real score )	{ ASSERT( type < 3 );	m_pathScores[ type ] = score; }
		SYNTHESIZE( size_t, iLastVisit );
		
	private:
		
		VAR( TileTemplate::ptr, m_tileTemplate );
		DVAR( Solidity, m_solidity, Solidity::Inherit );
		VAR( Segments, m_staticLightBlockers );
		Navigability m_navigableDirections[ Direction::NUM_DIRECTIONS ];
		
		size_t m_nOverlappingActors = 0;
		
		// Navigation data.
		//
		Vector2i m_priorPathNode;
		real m_pathScores[ 3 ];
		size_t m_iLastVisit = 0;
		
		bool m_doDispatchEvents = false;
	};
	
	FRESH_ENUM_STREAM_IN_BEGIN( Tile, Solidity )
	FRESH_ENUM_STREAM_IN_CASE( Tile::Solidity, Inherit )
	FRESH_ENUM_STREAM_IN_CASE( Tile::Solidity, Empty )
	FRESH_ENUM_STREAM_IN_CASE( Tile::Solidity, Solid )
	FRESH_ENUM_STREAM_IN_END()
	
	FRESH_ENUM_STREAM_OUT_BEGIN( Tile, Solidity )
	FRESH_ENUM_STREAM_OUT_CASE( Tile::Solidity, Inherit )
	FRESH_ENUM_STREAM_OUT_CASE( Tile::Solidity, Empty )
	FRESH_ENUM_STREAM_OUT_CASE( Tile::Solidity, Solid )
	FRESH_ENUM_STREAM_OUT_END()
	
	inline bool Tile::isSolid() const
	{
		if( m_solidity == Solidity::Inherit && m_tileTemplate )
		{
			return m_tileTemplate->isSolid();
		}
		else
		{
			return m_solidity == Solidity::Solid;
		}
	}
	
	inline bool Tile::isNavigable( Direction fromDirection ) const
	{
		auto nav = m_navigableDirections[ fromDirection.index() ];
		return nav == Navigability::Navigable || ( nav == Navigability::Inherit && !isSolid() );
	}
	
	inline void Tile::isNavigable( Navigability navigability )
	{
		// Assign same value to all.
		for( int i = 0; i < Direction::NUM_DIRECTIONS; ++i )
		{
			m_navigableDirections[ i ] = navigability;
		}
	}
	
	inline void Tile::isNavigable( Navigability navigability, Direction fromDirection )
	{
		m_navigableDirections[ fromDirection.index() ] = navigability;
	}
	
	/////////////////////////////////////////////////////////////////////////////////// 
	
	class FreshTileGrid : public DisplayObjectWithMesh
	{
		FRESH_DECLARE_CLASS( FreshTileGrid, DisplayObjectWithMesh )
	public:
		
		SYNTHESIZE_GET( uint, collisionMask )
		SYNTHESIZE_GET( uint, collisionRefusalMask )

		vec2i extents() const;
		real tileSize() const;
		real tileSize( real size );

		
		void resize( const Vector2i& newExtents, const Vector2i& oldOffsetIntoNew = Vector2i::ZERO );
		// 	REQUIRES( newExtents.x > 0 && newExtents.y > 0 );
		void resizeToInclude( const Vector2i& pos );
		void resizeToInclude( const vec2& pos  );

		bool isInBounds( const Vector2i& pos ) const;
		bool isInBounds( const vec2& pos ) const;
		
		const Tile& getTile( const Vector2i& pos ) const;
		const Tile& getTile( const vec2& pos ) const;
		Tile& getTile( const Vector2i& pos );
		Tile& getTile( const vec2& pos );

        void setTile( const Vector2i& pos, Tile::ptr tile );
        void setTile( const vec2& pos, Tile::ptr tile );

		Vector2i worldToTileSpace( const vec2& pos ) const
		{
			return m_tiles.worldToCell( pos );
		}
		
		vec2 tileUL( const Vector2i& pos ) const
		{
			return m_tiles.cellUL( pos );
		}
		
		vec2 tileCenter( const Vector2i& pos ) const
		{
			return m_tiles.cellCenter( pos );
		}
		
		rect getMinimalBoundsWorldSpace( real padding = 0 ) const;
		
		size_t numTileTemplates() const;
		TileTemplate::ptr tileTemplate( size_t i ) const;
		TileTemplate::ptr addTileTemplate();

		virtual void postLoad() override;

		// Collision and line-of-sight.
		//
		bool canSee( const vec2& source, const vec2& target, vec2* pOptionalOutHitPoint = 0, bool forNavigation = false ) const;
		bool canSee( const vec2& source, const vec2& target, real radius, vec2* pOptionalOutHitPoint = 0, bool forNavigation = false ) const;

		bool findValidDestination( vec2& outDestination, const vec2& desiredDestination, real radius, const vec2& origin ) const;
		// Assigns outDestination to a location between desiredDestination and origin (inclusive) that does not intersect any solid geometry.
		// Returns true if a non-intersecting location within the range was found. Else returns false and the value of outDestination is undefined.
		// REQUIRES( radius >= 0 );

		// Navigation
		//		
		typedef std::vector< Vector2i > Path;
		typedef std::vector< vec2 > WorldSpacePath;
		
		bool findClosestPath( const Vector2i& start, const Vector2i& goal, Path& outPath, real actorRadius = 0 );
		bool findClosestPath( const Vector2i& start, const Vector2i& goal, WorldSpacePath& outPath, real actorRadius = 0 );
		bool findClosestPath( const vec2& start, const vec2& goal, Path& outPath, real actorRadius = 0 )
		{
			return findClosestPath( worldToTileSpace( start ), worldToTileSpace( goal ), outPath, actorRadius );
		}
		
		bool findClosestPath( const vec2& start, const vec2& goal, WorldSpacePath& outPath, real actorRadius = 0 )
		{
			return findClosestPath( worldToTileSpace( start ), worldToTileSpace( goal ), outPath, actorRadius );
		}

		template< typename IterT >
		void convertToWorldSpacePath( Path::const_iterator begin, Path::const_iterator end, IterT out );

		void smoothPath( WorldSpacePath& path, const vec2& initialPosition, real avoidanceRadius, real tooCloseNodeRadiusSquared = 0 ) const;

		template< typename EvaluateT >
		vec2 findClearLocationNearby( const vec2& location, real actorRadius, real maxDistance, const EvaluateT& fnEvaluate ) const;
		// Returns the center of a non-solid tile, if available, that is within a linear range of [minDistance,maxDistance] units from location.
		// If no such clear tile is available, returns negative coordinates.
		// REQUIRES( minDistance <= maxDistance );
		// REQUIRES( minDistance >= 0 );
		
		struct NeighborIterator
		{
			NeighborIterator( FreshTileGrid& tileGrid, const Vector2i& tilePos, real actorRadius, bool isEnd = false, bool skipUnreachableNeighbors = true );
			
			Vector2i operator*() const;			
			NeighborIterator& operator++();
			NeighborIterator operator++( int );
			
			bool operator!=( const NeighborIterator& other ) const;
			
		protected:

			bool isEnd() const;
			Vector2i getCurrentNeighborPos() const;
			
		private:
			
			FreshTileGrid& m_tileGrid;
			real m_actorRadius;
			
			Vector2i m_centralTilePos;
			
			Vector2i m_currentNeighborOffset;
			Vector2i m_nextStepDir;
			
			bool m_skipUnreachableNeighbors;
		};
		
		struct NeighborRange : public std::pair< NeighborIterator, NeighborIterator >
		{
			NeighborRange( NeighborIterator begin_, NeighborIterator end_ )
			:	std::pair< NeighborIterator, NeighborIterator >( begin_, end_ )
			{}
			
			NeighborIterator begin() { return first; }
			NeighborIterator end() { return second; }
		};
		
		NeighborIterator getNeighborBegin( const Vector2i& tilePos, real actorSize = 0 );
		NeighborIterator getNeighborEnd( const Vector2i& tilePos, real actorSize = 0 );
		NeighborRange	 getNeighborRange( const Vector2i& tilePos, real actorSize = 0 );

		void loadGridFromVector( const Vector2i& extents, const std::vector< size_t >& templateIndices );
		void loadGridFromTexture( const Texture& texture );

	protected:
		
		void markDirty();		// Indicates that the visuals should be updated.
		
		void createFloorPointsForTile( const Vector2i& pos, std::vector< vec2 >& inOutPoints ) const;

		void calcStaticBlockers();
		void calcPotentiallyVisibleBlockersForTile( const Vector2i& pos );
		
		void getBlockersSurroundingTile( const Vector2i& tilePos, const Vector2i& blockerHostPos ) const;
		// Returns true iff the tile at tilePos blocks light.

		bool isValidPathingNeighbor( const Vector2i& fromTile, const Vector2i& potentialNeighbor, real actorRadius, bool skipUnreachableNeighbors = true ) const;
		
		void updateTileDisplay();
		
		virtual void drawMesh( TimeType relativeFrameTime ) override;

		TileTemplate::ptr nullTemplate() const;
		TileTemplate::ptr templateForColor( Color color ) const;
		
		void loadGridFromText( const std::string& text );
		
		Tile::ptr createTile( ObjectNameRef name = DEFAULT_OBJECT_NAME ) const;
		
		void fillNullTiles();
		
	private:
		
		typedef std::vector< TileTemplate::ptr > TileTemplates;
		typedef Grid2< Tile::ptr > Tiles;
		
		VAR( ClassInfo::cptr, m_tileClass );
		VAR( TileTemplates, m_templates );
		DVAR( Vector2i, m_padding, Vector2i::ZERO );
		VAR( Tile::ptr, m_nullTile );
		DVAR( Vector2i, m_atlasSubdivisions, Vector2i( 1, 1 ));
		DVAR( vec2, m_texturesSubdivisionsPerTile, vec2( 0.5f, 0.5f ));
		DVAR( vec2, m_tileToTextureOffset, vec2::ZERO );
		DVAR( int, m_maxBlockerDistanceTileSpace, 100 );
		VAR( std::string, m_gridText );
		VAR( Texture::ptr, m_creationGridImage );		// If non-null, this trumps m_gridText.
		
		DVAR( bool, m_nullTemplateIsSolid, false );
		
		DVAR( uint, m_collisionMask, ~0 );
		DVAR( uint, m_collisionRefusalMask, 0 );

		Tiles m_tiles;
		
		bool m_dirty = true;
	};

	inline void Tile::addStaticLightBlocker( const Segment& blocker )
	{
		m_staticLightBlockers.push_back( blocker );
	}
	
	inline bool FreshTileGrid::isInBounds( const Vector2i& pos ) const
	{
		return  0 <= pos.x && pos.x < extents().x &&
				0 <= pos.y && pos.y < extents().y;
	}

	inline bool FreshTileGrid::isInBounds( const vec2& pos ) const
	{
		return isInBounds( worldToTileSpace( pos ));
	}	
	
	inline const Tile& FreshTileGrid::getTile( const Vector2i& pos ) const
	{
		return const_cast< FreshTileGrid* >( this )->getTile( pos );
	}
	
	inline Tile& FreshTileGrid::getTile( const Vector2i& pos )
	{
		if( isInBounds( pos ) )
		{
			if( const auto tile = m_tiles.cellAt( pos ) )
			{
				return *tile;
			}
		}
		return *m_nullTile;
	}
	
	inline const Tile& FreshTileGrid::getTile( const vec2& pos ) const
	{
		return getTile( worldToTileSpace( pos ));
	}
	
	inline Tile& FreshTileGrid::getTile( const vec2& pos )
	{
		return getTile( worldToTileSpace( pos ));
	}

    inline void FreshTileGrid::setTile( const Vector2i& pos, Tile::ptr tile )
    {
        ASSERT( isInBounds( pos ));
        m_tiles.setCellAt( pos, tile );
    }

    inline void FreshTileGrid::setTile( const vec2& pos, Tile::ptr tile )
    {
        setTile( worldToTileSpace( pos ), tile );
    }

	template< typename IterT >
	void FreshTileGrid::convertToWorldSpacePath( Path::const_iterator begin, Path::const_iterator end, IterT out )
	{
		const real tileSize_ = tileSize();
		
		while( begin != end )
		{
			*out = vec2( begin->x + 0.5f, begin->y + 0.5f ) * tileSize_;
			
			++begin;
			++out;
		}
	}

	template< typename EvaluateT >
	vec2 FreshTileGrid::findClearLocationNearby( const vec2& location, real actorRadius, real maxDistance, const EvaluateT& fnEvaluate ) const
	{
		REQUIRES( actorRadius >= 0 );
		const auto excessTileOverlap = static_cast< int >( std::floor(( actorRadius * 2.0f ) / tileSize() ));		
		
		const auto isTileAreaClear = [&]( const Vector2i& tile )
		{
			Vector2i ulMax( tile - excessTileOverlap );
			Vector2i brMax( tile + excessTileOverlap );

			for( Vector2i loc( ulMax ); loc.y <= brMax.y; ++loc.y )
			{
				for( loc.x = ulMax.x; loc.x <= brMax.x; ++loc.x )
				{
					if( !isInBounds( loc ) || getTile( loc ).isSolid() )
					{
						return false;
					}
				}
			}
			return true;
		};
		
		// Find bounds.
		//
		Vector2i ulMax( worldToTileSpace( location - maxDistance ));
		Vector2i brMax( worldToTileSpace( location + maxDistance ));
		
		typedef std::pair< real, vec2 > TileScore;
		
		std::vector< TileScore > tileScores;
		
		for( Vector2i loc( ulMax ); loc.y < brMax.y; ++loc.y )
		{
			for( loc.x = ulMax.x; loc.x < brMax.x; ++loc.x )
			{
				// Are the tiles around this tile clear?
				//
				if( isTileAreaClear( loc ))
				{
					const vec2 worldSpaceTileCenter = tileCenter( loc );
					const real score = fnEvaluate( worldSpaceTileCenter );
						
					if( score > 0 )
					{
						tileScores.push_back( std::make_pair( score, worldSpaceTileCenter ));
					}
				}
			}
		}
		
		// Sort
		//
		std::sort( tileScores.begin(), tileScores.end() );
		
		if( !tileScores.empty() )
		{
			return tileScores.back().second;
		}
		else
		{
			return vec2( -std::numeric_limits< real >::infinity() );
		}
	}

}

#endif
