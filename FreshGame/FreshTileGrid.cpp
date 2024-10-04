//
//  FreshTileGrid.cpp
//  Fresh
//
//  Created by Jeff Wofford on 12/3/11.
//  Copyright (c) 2011 jeffwofford.com. All rights reserved.
//

#include "FreshTileGrid.h"
#include "DisplayObjectWithMesh.h"
#include "Stage.h"
#include "FindPath.h"
#include "RayCaster.h"
#include "CommandProcessor.h"
using namespace fr;

namespace 
{
	const Vector2i NULL_TILE_LOCATION{ std::numeric_limits< int >::min() };

	class TileGridCollisionWrapper
	{
	public:
		
		TileGridCollisionWrapper( const FreshTileGrid& tileGrid, bool ignoreDirection = true )
		:	m_tileGrid( tileGrid )
		,	m_ignoreDirection( ignoreDirection )
		{}
		
		bool operator()( const vec2& pos ) const
		{
			return m_tileGrid.getTile( pos ).isSolid();
		}

		bool operator()( const vec2& pos, const vec2& fromDirection ) const
		{
			if( m_ignoreDirection || fromDirection.isZero() )
			{
				return operator()( pos );
			}
			return !m_tileGrid.getTile( pos ).isNavigable( Direction( fromDirection ));
		}
		
		bool operator()( const Vector2i& pos ) const
		{
			return m_tileGrid.getTile( pos ).isSolid();
		}
		
		bool operator()( const Vector2i& pos, const vec2& fromDirection ) const
		{
			if( m_ignoreDirection || fromDirection.isZero() )
			{
				return operator()( pos );
			}
			
			return !m_tileGrid.getTile( pos ).isNavigable( Direction( fromDirection ));
		}
		
		bool operator()( const vec2& pos, real radius ) const
		{
			REQUIRES( radius >= 0 );
			const vec2 ul( pos - radius );
			const vec2 br( pos + radius );
			
			const Vector2i ulTileSpace( m_tileGrid.worldToTileSpace( ul ));
			const Vector2i brTileSpace( m_tileGrid.worldToTileSpace( br ));
			
			for( Vector2i loc( ulTileSpace ); loc.y <= brTileSpace.y; ++loc.y )
			{
				for( loc.x = ulTileSpace.x; loc.x <= brTileSpace.x; ++loc.x )
				{
					if( operator()( loc ))
					{
						return true;
					}
				}
			}
			
			return false;
		}

		bool operator()( const vec2& pos, real radius, const vec2& fromDirection ) const
		{
			if( m_ignoreDirection || fromDirection.isZero() )
			{
				return operator()( pos, radius );
			}
			
			REQUIRES( radius >= 0 );
			const vec2 ul( pos - radius );
			const vec2 br( pos + radius );
			
			const Vector2i ulTileSpace( m_tileGrid.worldToTileSpace( ul ));
			const Vector2i brTileSpace( m_tileGrid.worldToTileSpace( br ));
			
			for( Vector2i loc( ulTileSpace ); loc.y <= brTileSpace.y; ++loc.y )
			{
				for( loc.x = ulTileSpace.x; loc.x <= brTileSpace.x; ++loc.x )
				{
					if( operator()( loc, fromDirection ))
					{
						return true;
					}
				}
			}
			
			return false;
		}
		
	private:
		
		const FreshTileGrid& m_tileGrid;
		bool m_ignoreDirection = true;
	};
}

namespace fr
{
	FRESH_DEFINE_CLASS( TileTemplate )
	
	DEFINE_VAR( TileTemplate, Vector2i, m_atlasSubTexture );
	DEFINE_VAR( TileTemplate, bool, m_isSolid );
	DEFINE_VAR( TileTemplate, bool, m_doesRender );
	DEFINE_VAR( TileTemplate, bool, m_doesBlockLight );
	DEFINE_VAR( TileTemplate, Color, m_creationColor );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( TileTemplate )
	
	/////////////////////////////////////////////////////////////////////////////////// 
	
	FRESH_DEFINE_CLASS( Tile )
	
	DEFINE_VAR( Tile, TileTemplate::ptr, m_tileTemplate );
	DEFINE_VAR( Tile, Solidity, m_solidity );
	DEFINE_VAR( Tile, Segments, m_staticLightBlockers );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( Tile )
	
	Tile::Tile( const ClassInfo& assignedClassInfo, NameRef name )
	:	Super( assignedClassInfo, name )
	{
		isNavigable( Navigability::Inherit );
	}
	
	bool Tile::doesBlockLight() const
	{
		return tileTemplate()->doesBlockLight();
	}
	
	void Tile::finalizeStaticLightBlockers()
	{
		m_staticLightBlockers.shrink_to_fit();
	}
	
	bool Tile::getShadowTriangles( const vec2& lightPos, real lightRadius, std::vector< vec2 >& outTrianglePoints, real lightSpotHalfArc, const vec2& lightSpotDirection ) const
	{
		REQUIRES( lightRadius > 0 );
		
		for( auto iter = m_staticLightBlockers.begin(); iter != m_staticLightBlockers.end(); ++iter )
		{
			createTrianglesForBlocker( *iter, lightPos, lightRadius, outTrianglePoints, lightSpotHalfArc, lightSpotDirection );
		}
		
		return true;
	}
	

	///////////////////////////////////////////////////////////////////////////////////
	
	FRESH_DEFINE_CLASS( FreshTileGrid )

	DEFINE_VAR( FreshTileGrid, ClassInfo::cptr, m_tileClass );
	DEFINE_VAR( FreshTileGrid, TileTemplates, m_templates );
	DEFINE_VAR( FreshTileGrid, Vector2i, m_padding );
	DEFINE_VAR( FreshTileGrid, Tile::ptr, m_nullTile );
	DEFINE_VAR( FreshTileGrid, Vector2i, m_atlasSubdivisions );
	DEFINE_VAR( FreshTileGrid, vec2, m_texturesSubdivisionsPerTile );
	DEFINE_VAR( FreshTileGrid, vec2, m_tileToTextureOffset );
	DEFINE_VAR( FreshTileGrid, int, m_maxBlockerDistanceTileSpace );
	DEFINE_VAR( FreshTileGrid, std::string, m_gridText );
	DEFINE_VAR( FreshTileGrid, Texture::ptr, m_creationGridImage );
	DEFINE_VAR( FreshTileGrid, bool, m_nullTemplateIsSolid );
	DEFINE_VAR( FreshTileGrid, uint, m_collisionMask );
	DEFINE_VAR( FreshTileGrid, uint, m_collisionRefusalMask );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( FreshTileGrid )

	Tile::ptr FreshTileGrid::createTile( ObjectNameRef name ) const
	{
		auto tileClass = m_tileClass;
		if( !tileClass )
		{
			tileClass = &Tile::StaticGetClassInfo();
		}
		
		return createObject< Tile >( *tileClass, name );
	}
	
	void FreshTileGrid::fillNullTiles()
	{
		for( Vector2i pos( 0, 0 ); pos.y < extents().y; ++pos.y )
		{
			for( pos.x = 0; pos.x < extents().x; ++pos.x )
			{
				if( m_tiles.cellAt( pos ) == nullptr )
				{
					Tile::ptr tile = createTile();
					tile->tileTemplate( nullTemplate() );
					m_tiles.setCellAt( pos, tile );
				}
			}
		}
	}
	
	size_t FreshTileGrid::numTileTemplates() const
	{
		return m_templates.size();
	}
	
	TileTemplate::ptr FreshTileGrid::tileTemplate( size_t i ) const
	{
		return m_templates.size() > i ? m_templates[ i ] : nullptr;
	}
	
	TileTemplate::ptr FreshTileGrid::addTileTemplate()
	{
		m_templates.push_back( createObject< TileTemplate >() );
		return m_templates.back();
	}
	
	void FreshTileGrid::postLoad()
	{
		Super::postLoad();
		
		//
		// Add stock templates.
		//
		
		// Null template
		//
		m_templates.push_back( createObject< TileTemplate >( name() + "null template" ));
		m_templates.back()->isSolid( m_nullTemplateIsSolid );
		m_templates.back()->doesRender( false );
		m_templates.back()->doesBlockLight( false );
		
		// Set the null tile.
		//
		if( !m_nullTile )
		{
			m_nullTile = createTile( name() + "null tile" );
			m_nullTile->tileTemplate( m_templates.back() );
		}

		// "Empty" template blocks movement but not light.
		//
		m_templates.push_back( createObject< TileTemplate >( name() + "empty template" ));
		m_templates.back()->isSolid( true );
		m_templates.back()->doesRender( false );
		m_templates.back()->doesBlockLight( false );

		if( m_creationGridImage )
		{
			loadGridFromTexture( *m_creationGridImage );
		}
		else if( !m_gridText.empty() )
		{
			loadGridFromText( m_gridText );
		}
		m_gridText.clear();		// Load-time only, so reduce memory usage by throwing away the text.
	}

	TileTemplate::ptr FreshTileGrid::nullTemplate() const
	{
		return m_templates.at( m_templates.size() - 2 );
	}
	
	TileTemplate::ptr FreshTileGrid::templateForColor( Color color ) const
	{
		if( color.getA() == 0 )
		{
			// Null template.
			//
			return m_templates.back();
		}
		else
		{
			auto iter = std::min_element( m_templates.begin(), m_templates.end(), [&]( TileTemplate::ptr a, TileTemplate::ptr b )
										 {
											 return colorDistanceSquared( a->creationColor(), color ) < colorDistanceSquared( b->creationColor(), color );
										 } );
			
			ASSERT( iter != m_templates.end() );
			return *iter;
		}
	}
	
	void FreshTileGrid::loadGridFromVector( const Vector2i& extents, const std::vector< size_t >& templateIndices )
	{
		m_tiles.resize( extents );
		
		for( Vector2i pos( 0, 0 ); pos.y < extents.y; ++pos.y )
		{
			for( pos.x = 0; pos.x < extents.x; ++pos.x )
			{
				const size_t cellIndex = pos.x + pos.y * extents.x;
				
				if( cellIndex < templateIndices.size() )
				{
					const size_t index = templateIndices[ cellIndex ];
					if( index < m_templates.size() )
					{
						Tile::ptr tile = createTile();
						
						auto theTemplate = m_templates[ index ];
						ASSERT( theTemplate );

						tile->tileTemplate( theTemplate );
					
						m_tiles.setCellAt( pos, tile );
					}
					else
					{
						dev_warning( "Tile template index out of range: " << index );
					}
				}
			}
		}
		
		calcStaticBlockers();
		markDirty();
	}
	
	void FreshTileGrid::loadGridFromTexture( const Texture& texture )
	{
		ASSERT( extents().isZero() );	// Don't set extents if loading from a texture.
		
		const auto& texels = texture.getLoadedTexels();
		
		const vec2i extents = vector_cast< int >( texture.dimensions() );
		m_tiles.resize( extents );
		
		for( Vector2i pos( 0, 0 ); pos.y < extents.y; ++pos.y )
		{
			for( pos.x = 0; pos.x < extents.x; ++pos.x )
			{
				const size_t texelIndex = pos.x + pos.y * extents.x;
				ASSERT( texelIndex < texels.size() );
				const Color texel = texels[ texelIndex ].getABGR();
				
				Tile::ptr tile = createTile();
				
				auto theTemplate = templateForColor( texel );
				ASSERT( theTemplate );
				
#if DEV_MODE
				if( theTemplate->creationColor() != texel )
				{
					dev_warning( "Template with creationColor " << theTemplate->creationColor() << " was chosen for a texel with color " << texel );
				}
#endif
				
				tile->tileTemplate( theTemplate );
				
				m_tiles.setCellAt( pos, tile );
			}
		}
		
		calcStaticBlockers();
		markDirty();
	}
	
	void FreshTileGrid::loadGridFromText( const std::string& text )
	{
		// When loading from text we are less respectful of the text's implied dimensions
		// than we are when loading from a texture.
		//
		
		std::istringstream tileStream( text );

		vec2i extents;
		tileStream >> extents.x >> std::ws >> extents.y;
		
		if( extents.x <= 0 || extents.y <= 0 || extents.x >= 4096 || extents.y >= 4096 )
		{
			FRESH_THROW( FreshException, "TileGrid loading from text got out-of-range extents " << extents );
		}
		
		m_tiles.resize( extents );
		
		for( Vector2i pos( 0, 0 ); pos.y < extents.y; ++pos.y )
		{
			for( pos.x = 0; pos.x < extents.x; ++pos.x )
			{
				// Which template does the next tile want to use?
				
				// Read the symbol for this tile's template. Skipping whitespace is assumed.
				//
				char templateCharacter = '@';	// Default to "empty" (non-light-blocking) template.
				
				if( pos.x >= m_padding.x && pos.y >= m_padding.y && tileStream )
				{
					tileStream >> templateCharacter;	// If this fails that's just fine.
				}
				
				size_t iTemplate;
				
				if( templateCharacter == '.' )	// Null template
				{
					iTemplate = m_templates.size() - 2;
				}
				else if( templateCharacter == '@' )	// "Empty" template blocks movement but not light.
				{
					iTemplate = m_templates.size() - 1;
				}
				else
				{
					iTemplate = templateCharacter - 'a';
					
					if( iTemplate >= m_templates.size() )
					{
						trace( "FRESH WARNING: While loading " << toString() << ", found invalid template character '" << templateCharacter << "'." );
						iTemplate = 0;
					}
				}
				
				TileTemplate::ptr pSelectedTemplate = m_templates.at( iTemplate );
				
				// Set the tile at this location to have the requested template.
				//
				Tile::ptr tile = createTile();
				
				ASSERT( pSelectedTemplate );
				tile->tileTemplate( pSelectedTemplate );
				
				m_tiles.setCellAt( pos, tile );
			}
		}
		
		calcStaticBlockers();
		markDirty();
	}
	
	void FreshTileGrid::markDirty()
	{
		m_dirty = true;
	}
	
	void FreshTileGrid::drawMesh( TimeType relativeFrameTime )
	{
		if( m_dirty )
		{
			updateTileDisplay();
			ASSERT( !m_dirty );
		}
		
		Super::drawMesh( relativeFrameTime );
	}
	
	void FreshTileGrid::updateTileDisplay()
	{
		if( !texture() )
		{
			m_dirty = false;
			return;
		}

		ASSERT( m_atlasSubdivisions.x > 0 && m_atlasSubdivisions.y > 0 );
		ASSERT( extents().x > 0 && extents().y > 0 );
		
		std::vector< vec2 > points;
		
		// *6 because 6 points per tile. *2 because two vec2's per point
		points.reserve( extents().x * extents().y * 6 * 2 );
		
		for( Vector2i pos( 0, 0 ); pos.y < extents().y; ++pos.y )
		{
			for( pos.x = 0; pos.x < extents().x; ++pos.x )
			{
				createFloorPointsForTile( pos, points );
			}
		}
		
		SimpleMesh::ptr floorMesh = createObject< SimpleMesh >( name() + "floor mesh" );
		
		if( points.empty() == false )
		{
			floorMesh->create( Renderer::PrimitiveType::Triangles, points, Renderer::instance().createOrGetVertexStructure( "VS_Pos2TexCoord2" ), 2 ); // 2 = vec2 for pos + vec2 for texcoord
			floorMesh->calculateBounds( points, 2 );
		}
		
		mesh( floorMesh );
		
		auto meshShader = Renderer::instance().createOrGetShaderProgram( "SP_FloorTile" );
		ASSERT( meshShader );
		shaderProgram( meshShader );
		
		m_dirty = false;
	}

	void FreshTileGrid::createFloorPointsForTile( const Vector2i& pos, std::vector< vec2 >& inOutPoints ) const
	{
		ASSERT( m_atlasSubdivisions.x > 0 && m_atlasSubdivisions.y > 0 );
		ASSERT( texture() );
		
		const auto myTexture = texture();
		
		const Tile::ptr tile = m_tiles.cellAt( pos );

		// We'll inset every tile's texture coordinates by 1/2 a pixel on all sides in order to avoid bleeding during texture sampling.
		const vec2 pixelSizeInUVSpace = 1.0f / vector_cast< real >( myTexture->dimensions() );
		
		if( tile )
		{
			ASSERT( tile->tileTemplate() );

			if( tile->tileTemplate()->doesRender() )
			{
				// TODO Could cache these constants across all calls. No big deal though. This function is seldom called.
				
				// Given the floor texture atlas, how big in TEXTURE COORDINATES is a single subdivision?
				// For example, for a 4x2 atlas, each subdivision would be 0.25 x 0.5 texture coordinate units.
				//
				const vec2 subdivisionSizeTexCoords( 1.0f / vec2( m_atlasSubdivisions.x, m_atlasSubdivisions.y ) );	
				
				// How far should we offset the texture coordinates for this particular tile? This depends on the tile's gridspace position.
				// These are in subdivision coordinates, so if we're offseting by 1/2, that's half the width of a subdivision--not the whole atlas.
				//
				const vec2 tileSubdivisionTexCoordOffset( m_tileToTextureOffset + vec2( std::fmod( pos.x * m_texturesSubdivisionsPerTile.x, 1.0f ), std::fmod( pos.y * m_texturesSubdivisionsPerTile.y, 1.0f )));
				
				// What are the corners of our texture coordinate rectangle?
				//
				const vec2 minUVs( subdivisionSizeTexCoords * ( tileSubdivisionTexCoordOffset + vec2( tile->tileTemplate()->atlasSubTexture().x, tile->tileTemplate()->atlasSubTexture().y )) + 0.5f * pixelSizeInUVSpace );
				const vec2 maxUVs( minUVs + subdivisionSizeTexCoords * m_texturesSubdivisionsPerTile - pixelSizeInUVSpace );
				
				const real tileSize_ = tileSize();
				
				inOutPoints.push_back( tileSize_ * vec2( pos.x		  , pos.y        ));
				inOutPoints.push_back( vec2( minUVs.x, minUVs.y ));
				
				inOutPoints.push_back( tileSize_ * vec2( pos.x + 1.0f, pos.y        ));
				inOutPoints.push_back( vec2( maxUVs.x, minUVs.y ));
				
				inOutPoints.push_back( tileSize_ * vec2( pos.x       , pos.y + 1.0f ));
				inOutPoints.push_back( vec2( minUVs.x, maxUVs.y ));
				
				inOutPoints.push_back( tileSize_ * vec2( pos.x       , pos.y + 1.0f ));
				inOutPoints.push_back( vec2( minUVs.x, maxUVs.y ));
				
				inOutPoints.push_back( tileSize_ * vec2( pos.x + 1.0f, pos.y        ));
				inOutPoints.push_back( vec2( maxUVs.x, minUVs.y ));
				
				inOutPoints.push_back( tileSize_ * vec2( pos.x + 1.0f, pos.y + 1.0f ));
				inOutPoints.push_back( vec2( maxUVs.x, maxUVs.y ));
			}
		}
	}
	
	void FreshTileGrid::calcStaticBlockers()
	{
		//
		// Identify light blockers, "digesting" them into tiles based on a trivial visibility test.
		// That is, for each tile, add all the blockers that that tile could potentially see
		// to that tile.
		//
		for( Vector2i pos( 0, 0 ); pos.y < extents().y; ++pos.y )
		{
			for( pos.x = 0; pos.x < extents().x; ++pos.x )
			{
				auto& tile = *m_tiles.cellAt( pos );
				tile.clearStaticLightBlockers();
				
				// Does this tile block light?
				//
				if( !tile.doesBlockLight() )
				{
					// No. Determine what blockers it can see.
					//
					calcPotentiallyVisibleBlockersForTile( pos );
				}
			}
		}
	}
	
	void FreshTileGrid::calcPotentiallyVisibleBlockersForTile( const Vector2i& basePos )
	{
		//
		// Identify all blockers that can be seen from the base tile,
		// culling when we hit clearly non-seeable tiles.
		// Work outward from the base position to make culling more efficient.
		//
		// Get blockers for this tile.
		//
		getBlockersSurroundingTile( basePos, basePos );
		
		// Consider all other same-axis tiles, stopping for a given direction when we hit a blocker.
		//
		for( int axis = 0; axis < 2; ++axis )
		{
			Vector2i pos( basePos );
			
			for( int sign = -1; sign <= 1; sign += 2 )
			{
				for( int coord = basePos[ axis ] + sign; coord >= 0 && coord < extents()[ axis ]; coord += sign )
				{
					pos[ axis ] = coord;
					getBlockersSurroundingTile( pos, basePos );
					
					if( m_tiles.cellAt( pos )->doesBlockLight() )
					{
						// We're done along this direction.
						//
						break;
					}
				}
			}
		}
		
		// Consider each adjacent quadrant, ignoring the quadrant if it is blocked
		// by both "sponsoring" base neighbors.
		//
		Vector2i quadrantDir( 1, 0 );
		Vector2i quadrantOtherDir( 0, 1 );
		for( int quadrant = 0; quadrant < 4; quadrantDir.quickRot90(), quadrantOtherDir.quickRot90(), ++quadrant )
		{
			// Is this quadrant trivially blocked off by its two "sponsors"?
			//
			Vector2i sponsorPos( basePos + quadrantDir );
			const Tile& sponsor = getTile( sponsorPos );
			
			if( sponsor.doesBlockLight() )
			{
				// Check the other sponsor.
				//
				Vector2i otherSponsorPos( basePos + quadrantOtherDir );
				const Tile& otherSponsor = getTile( otherSponsorPos );
				
				if( otherSponsor.doesBlockLight() )
				{
					// Both "sponsors" block light. Ignore this whole quadrant.
					//
					continue;
				}
			}
			
			// At least one sponsor doesn't block light. Process this quadrant.
			//
			
			// Calculate the bounding rectangle for the quadrant.
			//
			Vector2i quadrantDiagonal( quadrantDir + quadrantOtherDir );
			Vector2i quadrantMins( basePos + quadrantDiagonal );
			Vector2i quadrantMaxs( extents() );
			
			for( int axis = 0; axis < 2; ++axis )
			{
				if( quadrantDiagonal[ axis ] < 0 )
				{
					quadrantMaxs[ axis ] = quadrantMins[ axis ] + 1;	// +1 because the loop below goes up until (but not including when) pos == max
					quadrantMins[ axis ] = 0;
				}
			}
			
			// Process every tile in the quadrant.
			//
			for( Vector2i pos( quadrantMins ); pos.y < quadrantMaxs.y; ++pos.y )
			{
				for( pos.x = quadrantMins.x; pos.x < quadrantMaxs.x; ++pos.x )
				{
					getBlockersSurroundingTile( pos, basePos );
				}
			}
		}
		
		// TODO Simplify blockers by conjoining colinear, coterminal blockers.
		
		getTile( basePos ).finalizeStaticLightBlockers();
	}
	
	void FreshTileGrid::getBlockersSurroundingTile( const Vector2i& tilePos, const Vector2i& blockerHostPos ) const
	{
		// Make sure this tile isn't ridiculously far from the host position--farther than any light should be able to reach.
		//
		if( distanceSquared( tilePos, blockerHostPos ) <= m_maxBlockerDistanceTileSpace * m_maxBlockerDistanceTileSpace )
		{
			
			Tile& tile = *m_tiles.cellAt( tilePos );
			Tile& blockerHost = *m_tiles.cellAt( blockerHostPos );
			
			// Does this tile block light?
			//
			if( !tile.doesBlockLight() )
			{
				// No. Do any of its neighbors pose "walls" that block light?
				// For each of this tile's four cardinal neighbors...
				//
				for( int axis = 0; axis < 2; ++axis )
				{
					const int otherAxis = ( axis + 1 ) & 1;
					
					for( int sign = -1; sign <= 1; sign += 2 )
					{
						Vector2i neighborPos( tilePos );
						neighborPos[ axis ] += sign;
						
						// Tiles on the outside of the level never cast shadows.
						//
						if( neighborPos.x < 0 || neighborPos.x >= extents().x ||
						    neighborPos.y < 0 || neighborPos.y >= extents().y )
						{
							continue;
						}
						const Tile& neighbor = *m_tiles.cellAt( neighborPos );
						
						// Does this neighbor block light?
						//
						if( neighbor.doesBlockLight() )
						{
							// Yes. Make a light blocker between the base tile and this neighbor.
							//
							Segment segment;
							
							segment.first[ axis ] = segment.second[ axis ] =
							tilePos[ axis ] + ( sign < 0 ? 0.0f : 1.0f );
							
							segment.first [ otherAxis ] = tilePos[ otherAxis ];
							segment.second[ otherAxis ] = tilePos[ otherAxis ] + 1.0f;
							
							// Ensure that the segment's begin and end points point toward
							// the non-blocking tile on their "right" side.
							//
							if(( axis ^ ( sign < 0 ? 0 : 1 )) == 0 )
							{
								std::swap( segment.first, segment.second );
							}
							
							// Scale blockers to world space.
							//
							segment.first  *= tileSize();
							segment.second *= tileSize();
							
							// Add the blocker.
							//
							blockerHost.addStaticLightBlocker( segment );
						}
					}
				}
			}
		}
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// NAVIGATION

	struct TileGridNavigationHelper
	{
		static FreshTileGrid::ptr tileGrid;
		static real actorSize;
		
		typedef Vector2i	Node;
		typedef real		Cost;
		
		static Tile& tile( Node n )
		{
			ASSERT( tileGrid );
			ASSERT( tileGrid->isInBounds( n ));
			return tileGrid->getTile( n );
		}
		
		static fr::Direction direction( Node from, Node to )
		{
			return fr::Direction( to - from );
		}
		
		static Node getPriorPathNode( Node n ) 
		{ 
			return tile( n ).priorPathNode(); 
		}		
		
		static void setPriorPathNode( Node n, Node prior ) 
		{
			ASSERT( n != prior );
			tile( n ).priorPathNode( prior ); 
		}
		static Cost getScore( Node n, unsigned int type )
		{
			return tile( n ).getPathScore( type );
		}
		static void setScoreF( Node n, Cost cost ) 
		{ 
			return tile( n ).setPathScore( 0, cost ); 
		}
		static void setScoreG( Node n, Cost cost ) 
		{ 
			return tile( n ).setPathScore( 1, cost ); 
		}
		static void setScoreH( Node n, Cost cost ) 
		{ 
			return tile( n ).setPathScore( 2, cost ); 
		}
		static bool betterScoreF( Node n, Node q )
		{
			return tile( n ).getPathScore( 0 ) < tile( q ).getPathScore( 0 );
		}
		static bool isInClosedSet( Node n, size_t iVisit )
		{
			return tile( n ).iLastVisit() == iVisit;
		}
		static void addToClosedSet( Node n, size_t iVisit )
		{
			tile( n ).iLastVisit( iVisit );
		}
		static FreshTileGrid::NeighborRange getNeighborRange( Node n ) 
		{ 
			ASSERT( tileGrid );
			return tileGrid->getNeighborRange( n, actorSize );
		}											   
		static Cost nodeDistance( Node from, Node to ) 
		{ 
			if( from == to ) return 0;
			
			// Is one of these paths blocked? If so the cost is infinite.
			//
			fr::Direction dir = direction( from, to );
			if( !tileGrid->isInBounds( to ) || !tile( to ).isNavigable( dir ))
			{
				return std::numeric_limits< Cost >::infinity();
			}
			else
			{
				Vector2i delta( from - to );
				const Cost actualDistance = std::sqrt( (Cost) delta.x * (Cost) delta.x + (Cost) delta.y * (Cost) delta.y );
				const Cost scalar = tile( to ).navDistanceScalar();
				
				return actualDistance * scalar;
			}
		}
		static Cost getHeuristicEstimate( Node n, Node q ) 
		{ 
			return nodeDistance( n, q );
		}
		
	};
	
	FreshTileGrid::ptr TileGridNavigationHelper::tileGrid = nullptr;
	real TileGridNavigationHelper::actorSize = 0;
	
	FreshTileGrid::NeighborIterator::NeighborIterator( FreshTileGrid& tileGrid, const Vector2i& tilePos, real actorRadius, bool isEnd_, bool skipUnreachableNeighbors ) 
	:	m_tileGrid( tileGrid )
	,	m_actorRadius( actorRadius )
	,	m_centralTilePos( tilePos ) 
	,	m_currentNeighborOffset( isEnd_ ? Vector2i::ZERO : Vector2i( 1, 0 ))
	,	m_nextStepDir( isEnd_ ? Vector2i::ZERO : Vector2i( 0, 1 ))
	,	m_skipUnreachableNeighbors( skipUnreachableNeighbors )
	{
		// If we're not the end, find a legal tile.
		//
		if( !isEnd() && !m_tileGrid.isValidPathingNeighbor( m_centralTilePos, getCurrentNeighborPos(), m_actorRadius, m_skipUnreachableNeighbors ))
		{
			++(*this);
		}
		ASSERT( isEnd() || m_tileGrid.isValidPathingNeighbor( m_centralTilePos, getCurrentNeighborPos(), m_actorRadius, m_skipUnreachableNeighbors ));
	}
	
	Vector2i FreshTileGrid::NeighborIterator::operator*() const
	{
		ASSERT( !isEnd() );	// Can't access the end() neighbor.
		
		return getCurrentNeighborPos();
	}
	
	FreshTileGrid::NeighborIterator& FreshTileGrid::NeighborIterator::operator++()
	{
		ASSERT( !isEnd() );	// Can't move past the end() neighbor.
		
		do
		{	
			// Are we on the last neighbor (at 1, -1)?
			//
			if( m_currentNeighborOffset == Vector2i( 1, -1 ))
			{
				// Move to the end.
				//
				m_currentNeighborOffset.setToZero();
				m_nextStepDir.setToZero();
				ASSERT( isEnd() );
			}
			else
			{
				m_currentNeighborOffset += m_nextStepDir;
				
				// Time to change directions?
				//
				if( std::abs( m_currentNeighborOffset.x ) == 1 && std::abs( m_currentNeighborOffset.y ) == 1 )
				{
					m_nextStepDir.quickRot90();
				}
			}
			
		} while( !isEnd() && !m_tileGrid.isValidPathingNeighbor( m_centralTilePos, getCurrentNeighborPos(), m_actorRadius, m_skipUnreachableNeighbors ));
		
		// Either we're at the end or we're looking at a legal tile.
		//
		ASSERT( isEnd() || m_tileGrid.isValidPathingNeighbor( m_centralTilePos, getCurrentNeighborPos(), m_actorRadius, m_skipUnreachableNeighbors ));
		
		return *this;
	}
	
	FreshTileGrid::NeighborIterator FreshTileGrid::NeighborIterator::operator++( int )
	{
		NeighborIterator copy( *this );
		
		++(*this);
		
		return copy;
	}
	
	bool FreshTileGrid::NeighborIterator::isEnd() const
	{
		return m_currentNeighborOffset == Vector2i::ZERO;
	}
	
	Vector2i FreshTileGrid::NeighborIterator::getCurrentNeighborPos() const
	{
		ASSERT( !isEnd() );	// Can't access the end() neighbor.
		return m_centralTilePos + m_currentNeighborOffset;
	}
	
	bool FreshTileGrid::NeighborIterator::operator!=( const NeighborIterator& other ) const
	{
		return &m_tileGrid != &other.m_tileGrid ||
			m_centralTilePos != other.m_centralTilePos ||
			m_currentNeighborOffset != other.m_currentNeighborOffset;
	}
	
	FreshTileGrid::NeighborIterator FreshTileGrid::getNeighborBegin( const Vector2i& tilePos, real actorSize )
	{
		return NeighborIterator( *this, tilePos, actorSize );
	}

	FreshTileGrid::NeighborIterator FreshTileGrid::getNeighborEnd( const Vector2i& tilePos, real actorSize )
	{
		return NeighborIterator( *this, tilePos, actorSize, true );
	}
	
	FreshTileGrid::NeighborRange    FreshTileGrid::getNeighborRange( const Vector2i& tilePos, real actorSize )
	{
		return NeighborRange( getNeighborBegin( tilePos, actorSize ), getNeighborEnd( tilePos, actorSize ));
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	vec2i FreshTileGrid::extents() const
	{
		return m_tiles.gridSize();
	}
	
	real FreshTileGrid::tileSize() const
	{
		return m_tiles.cellSize().x;
	}
	
	real FreshTileGrid::tileSize( real size )
	{
		m_tiles.cellSize( vec2{ size, size } );
		return size;
	}

	void FreshTileGrid::resize( const Vector2i& newExtents, const Vector2i& oldOffsetIntoNew )
	{
		REQUIRES( newExtents.x > 0 && newExtents.y > 0 );
		m_tiles.resize( newExtents, oldOffsetIntoNew, {} );
		
		fillNullTiles();
	}
	
	void FreshTileGrid::resizeToInclude( const Vector2i& pos )
	{
		m_tiles.resizeToInclude( pos, {} );
		fillNullTiles();
	}
	
	void FreshTileGrid::resizeToInclude( const vec2& pos )
	{
		resizeToInclude( worldToTileSpace( pos ));
	}
	
	// For any of the above, if useOpaqueTemplate, a fully opaque, solid, and non-light-transmitting template will be used.
	// Else a solid, non-rendering, but light-transmitting template will be used.

	bool FreshTileGrid::isValidPathingNeighbor( const Vector2i& fromTile, const Vector2i& potentialNeighbor, real actorRadius, bool skipUnreachableNeighbors ) const
	{
		if( !isInBounds( potentialNeighbor ))
		{
			// Invalid neighbor is out of bounds.
			return false;
		}

		const Vector2i delta = potentialNeighbor - fromTile;
		
		// "Thick" actors (thicker than a tile) should use the wrapper to see whether this cell is non-solid in a general way.
		//
		if( skipUnreachableNeighbors && actorRadius >= tileSize() * 0.5f )
		{
			TileGridCollisionWrapper wrapper( *this, false );
			
			return !wrapper( tileCenter( potentialNeighbor ), actorRadius, fr::Direction( delta ));
		}

		// Does a path from fromTile to potentialNeighbor cut across a solid tile?
		//
		if( delta.x == 0 || delta.y == 0 )
		{
			// Cardinal directions are necessarily fine, so long as the neighbor is navigable or we're not rejecting unreachable neighbors.
			//
			return !skipUnreachableNeighbors || getTile( potentialNeighbor ).isNavigable( fr::Direction( delta ));
		}

		// Check both directions.
		for( int axis = 0; axis <= 1; ++axis )
		{
			Vector2i cutAcrossNeighbor( fromTile );
			cutAcrossNeighbor[ axis ] += delta[ axis ];
			
			if( getTile( cutAcrossNeighbor ).isSolid() )
			{
				return false;
			}
		}	
		
		// No problems.
		return true;		
	}
	
	rect FreshTileGrid::getMinimalBoundsWorldSpace( real padding ) const
	{
		rect bounds(
					std::numeric_limits< real >::max(),
					std::numeric_limits< real >::max(),
					std::numeric_limits< real >::min(),
					std::numeric_limits< real >::min()
		);
		
		for( Vector2i pos( 0, 0 ); pos.y < extents().y; ++pos.y )
		{
			for( pos.x = 0; pos.x < extents().x; ++pos.x )
			{
				if( !getTile( pos ).isSolid() )
				{
					bounds.growToEncompass( tileCenter( pos ) );
				}
			}
		}
		
		bounds.left( bounds.left() - padding );
		bounds.top( bounds.top() - padding );
		bounds.right( bounds.right() + padding );
		bounds.bottom( bounds.bottom() + padding );
		
		return bounds;
	}

	inline real lengthSquared( const vec2& v ) { return v.lengthSquared(); }
	
	bool FreshTileGrid::canSee( const vec2& source, const vec2& target, vec2* pOptionalOutHitPoint, bool forNavigation ) const
	{
		const vec2 delta = target - source;
		const real distance = delta.length();

		if( distance <= 0 )
		{
			return true;
		}
		
		TileGridCollisionWrapper wrapper( *this, !forNavigation );
		
		typedef RayCaster< 2, real, vec2, TileGridCollisionWrapper, lengthSquared > MyRayCaster;

		MyRayCaster::Intersection intersection;
		
		bool foundIntersection = MyRayCaster::findRayIntersection( wrapper, 
																   source,
																   delta / distance,
																   intersection,
																   distance,
																   vec2( tileSize() ));
		
		if( foundIntersection && pOptionalOutHitPoint )
		{
			*pOptionalOutHitPoint = intersection.hitPoint;
		}
																				   
		return !foundIntersection;
	}
	
	bool FreshTileGrid::canSee( const vec2& source, const vec2& target, real radius, vec2* pOptionalOutHitPoint, bool forNavigation ) const
	{		
		if( radius <= 0 )
		{
			// Use the regular non-"thick" version.
			//
			return canSee( source, target, pOptionalOutHitPoint, forNavigation );
		}
		
		// Do several ray casts from the left side of the ray to the right side,
		// making sure that no gap between rays is larger than the profile of a tile at this angle.
		//
		const real diameter = radius * 2.0f;
		
		const vec2 delta( target - source );
		const vec2 rayNormal( delta.normal() );
		vec2 tangent( rayNormal );
		tangent.quickRot90();
		
		const real adjustedTileSize = tileSize();		// TODO Expand based on rotation.
		const size_t nNeededCasts = diameter / adjustedTileSize + 2;
		
		const real maxCast = static_cast< real >( nNeededCasts - 1 ); 
		
		assert( nNeededCasts >= 2 );	// At least one for the "left" and another for the "right"
		assert( maxCast >= 1.0f );
		
		bool couldSee = true;
		for( size_t iCast = 0; couldSee && iCast < nNeededCasts; ++iCast )
		{
			const real adjustment = ( iCast / maxCast - 0.5f ) * diameter;
			
			const vec2 adjustedSource = source + tangent * adjustment;
			const vec2 adjustedTarget = target + tangent * adjustment;
			
			couldSee = canSee( adjustedSource, adjustedTarget, pOptionalOutHitPoint, forNavigation );
		}
		
		return couldSee;
	}
	
	///////////////////////////////////////////////////////////////////////////////////
	
	struct HelperIsClear
	{
		HelperIsClear( const TileGridCollisionWrapper& wrapper ) : m_wrapper( wrapper ) {}
		
		bool operator()( const vec2& loc, real radius ) const { return !m_wrapper( loc, radius ); }
		
	private:
		
		const TileGridCollisionWrapper& m_wrapper;
	};

	struct HelperIsClearTileCentersOnly
	{
		HelperIsClearTileCentersOnly( const FreshTileGrid& tileGrid, const TileGridCollisionWrapper& wrapper ) : m_tileGrid( tileGrid ), m_wrapper( wrapper ) {}
		
		bool operator()( const vec2& loc, real radius ) const { return !m_wrapper( m_tileGrid.tileCenter( m_tileGrid.worldToTileSpace( loc )), radius ); }
		
	private:
		
		const FreshTileGrid& m_tileGrid;
		const TileGridCollisionWrapper& m_wrapper;
	};

	bool FreshTileGrid::findValidDestination( vec2& outDestination, const vec2& desiredDestination, real radius, const vec2& origin ) const
	{
		TileGridCollisionWrapper wrapper( *this, false );

		// Quick first test: is the requested point valid?
		//
		if( isInBounds( desiredDestination ) && !wrapper( desiredDestination, radius ) )
		{
			outDestination = desiredDestination;
			return true;
		}
		
		// Consider the rectangle of tiles bounded by origin in one corner and desiredDestination in the other corner.
		// Traverse every unblocked tile within this rectangle, giving the tile a score proportional to its distance from desiredDestination
		// (with smaller distances better) and its distance from the line passing between desiredDestination and origin
		// (with smaller distances better). The tile with the lowest score is the best destination tile.
		
		Vector2i originTile = worldToTileSpace( origin );
		Vector2i destinationTile = worldToTileSpace( desiredDestination );
		
		// Scoot out the origin and destination tiles so that we consider "farther out" tiles. 
		// This is to avoid invalid destinations when the origin is close to the wall.
		//
		Vector2i originAdjustment = Vector2i( sign( originTile.x - destinationTile.x ), sign( originTile.y - destinationTile.y ));
		Vector2i destAdjustment = -originAdjustment;
		
		
#if 0	// TODO Testing limiting the expansion of the search range on the far side.
		destAdjustment *= 1 + static_cast< int >( radius / ( tileSize() * 0.5f ));	// Give more leeway on the far side, so there's enough space to really work.
#endif
		
		for( int axis = 0; axis < 2; ++axis )
		{
			if( originAdjustment[ axis ] == 0 )	// The dest and origin are along the same row or column. Widen the search area in both directions.
			{
				originAdjustment[ axis ]	=  1;
				destAdjustment[ axis ]		= -1;
			}
		}
		
		originTile += originAdjustment;
		destinationTile += destAdjustment;
		
		const Vector2i steps( signNoZero( originTile.x - destinationTile.x ), 
							  signNoZero( originTile.y - destinationTile.y ));
		
		const vec2 beeLineNormal = ( origin - desiredDestination ).normal();
		
		outDestination.set( -1.0f, -1.0f );
		real minTileScore = std::numeric_limits< real >::max();
		Vector2i p;
		for( p.y = destinationTile.y; p.y != originTile.y + steps.y; p.y += steps.y )
		{
			for( p.x = destinationTile.x; p.x != originTile.x + steps.x; p.x += steps.x )
			{
				// Is this tile clear?
				//
				const vec2 proposedPoint = tileCenter( p );
				if( isInBounds( proposedPoint ) && !wrapper( proposedPoint, radius ) )
				{
					// Yes it is. Score it.
					//
					const vec2 tileDelta = proposedPoint - desiredDestination;
					const real distToDestination = tileDelta.length();
					real tileScore = 0;
					if( distToDestination > 0 )
					{
						const real dot = ( tileDelta / distToDestination ).dot( beeLineNormal );
						tileScore = distToDestination + ( 1.0f - dot );
					}
					
					if( tileScore < minTileScore )
					{
						minTileScore = tileScore;
						outDestination = proposedPoint;
						
						if( tileScore <= tileSize() * 0.5f )	// Can't get better than that. Early out.
						{
							goto finished;
						}
					}
				}
			}
		}
		
	finished:
		
		if( outDestination.x >= 0 && outDestination.y >= 0 )
		{
			// Adjust the path so that it is as close to the desiredDestination as possible.
			// Adjust each axis independently, one at a time, moving the outDestination toward the desiredDestination
			// in that axis until a clear spot is found.
			//
			for( int axis = 0; axis < 2; ++axis )
			{
				const real adjustmentDelta = desiredDestination[ axis ] - outDestination[ axis ];
				vec2 adjustmentDirection( 0, 0 );
				adjustmentDirection[ axis ] = sign( adjustmentDelta );
				
				const vec2 closeLocation = findClearLocationByBisection( outDestination, adjustmentDirection, std::abs( adjustmentDelta ), HelperIsClearTileCentersOnly( *this, wrapper ), 1.0f, radius );
				outDestination += adjustmentDirection * (( closeLocation - outDestination ).length() - radius );
			}
			return true;
		}
		else
		{
			return false;
		}
	}

	bool FreshTileGrid::findClosestPath( const Vector2i& start, const Vector2i& goal, Path& outPath, real actorRadius )
	{
		TileGridNavigationHelper::tileGrid = this;
		TileGridNavigationHelper::actorSize = actorRadius;
		
		// Find a path through the graph.
		//
		PathFinder< 
		Vector2i,
		NeighborRange,
		SystemClock,
		real
		> pathFinder( start, 
					 goal, 
					 NULL_TILE_LOCATION,
					 std::bind( TileGridNavigationHelper::setPriorPathNode, _1, _2 ),
					 std::bind( TileGridNavigationHelper::getScore, _1, 1 ),
					 std::bind( TileGridNavigationHelper::getScore, _1, 2 ),
					 std::bind( TileGridNavigationHelper::setScoreF, _1, _2 ),
					 std::bind( TileGridNavigationHelper::setScoreG, _1, _2 ),
					 std::bind( TileGridNavigationHelper::setScoreH, _1, _2 ),
					 std::bind( TileGridNavigationHelper::getHeuristicEstimate, _1, _2 )
					 );
		
		bool needsMoreTime = true;
		bool foundPath = false;
		while( needsMoreTime )
		{
			foundPath = pathFinder.findPath( needsMoreTime,
											std::bind( TileGridNavigationHelper::setPriorPathNode, _1, _2 ),
											std::bind( TileGridNavigationHelper::getScore, _1, 0 ),
											std::bind( TileGridNavigationHelper::getScore, _1, 1 ),
											std::bind( TileGridNavigationHelper::getScore, _1, 2 ),
											std::bind( TileGridNavigationHelper::setScoreF, _1, _2 ),
											std::bind( TileGridNavigationHelper::setScoreG, _1, _2 ),
											std::bind( TileGridNavigationHelper::setScoreH, _1, _2 ),
											std::bind( TileGridNavigationHelper::betterScoreF, _1, _2 ),
											std::bind( TileGridNavigationHelper::isInClosedSet, _1, _2 ),
											std::bind( TileGridNavigationHelper::addToClosedSet, _1, _2 ),
											std::bind( TileGridNavigationHelper::getNeighborRange, _1 ),
											std::bind( TileGridNavigationHelper::nodeDistance, _1, _2 ),
											std::bind( TileGridNavigationHelper::getHeuristicEstimate, _1, _2 ),
											fr::getAbsoluteTimeClocks											
											);
		}
		
		outPath.clear();
		if( foundPath )
		{
			fr::buildPath< Vector2i, TileGridNavigationHelper::getPriorPathNode > pathBuilder( NULL_TILE_LOCATION );
			pathBuilder( goal, std::back_inserter( outPath ));
			
			// The path comes back from the builder in reverse order. Straighten it out.
			//		
			std::reverse( outPath.begin(), outPath.end() );
		}
		
		TileGridNavigationHelper::tileGrid = nullptr;
		
		return foundPath;
	}

	bool FreshTileGrid::findClosestPath( const Vector2i& start, const Vector2i& goal, WorldSpacePath& outPath, real actorRadius )
	{
		Path path;
		bool foundPath = findClosestPath( start, goal, path, actorRadius );
		
		outPath.clear();
		if( foundPath )
		{
			outPath.reserve( path.size() );
			convertToWorldSpacePath( path.begin(), path.end(), std::back_inserter( outPath ));
		}
		
		return foundPath;
	}

	void FreshTileGrid::smoothPath( WorldSpacePath& path, const vec2& initialPosition, real avoidanceRadius, real tooCloseNodeRadiusSquared ) const
	{
		// For all path elements i < n - 1, if element i - 1 can "see" element i + 1, then element i is removed.
		//
		
		const vec2* pLastFirmPosition = &initialPosition;
		
		for( auto iterPathNode = path.begin();; )
		{
			auto iterNextNode = iterPathNode;
			++iterNextNode;
			
			if( iterNextNode == path.end() )
			{
				// There is no "next node". We're done.
				break;
			}
			
			// Is this node really close to the last one?
			if( tooCloseNodeRadiusSquared > 0 && distanceSquared( *iterPathNode, *pLastFirmPosition ) < tooCloseNodeRadiusSquared )
			{
				// Skip this node.
				//
				iterPathNode = path.erase( iterPathNode );
				continue;
			}
			
			const vec2& nextNode = *iterNextNode;
			
			// We can eliminate this node if it's very close to the last firm position or if the last firm position can see the next node.
			//
			if( canSee( *pLastFirmPosition, nextNode, avoidanceRadius, nullptr /* don't care about hitpoint */, true ))
			{
				iterPathNode = path.erase( iterPathNode );
			}
			else
			{
				// Can't see it. I guess the current node is firm.
				//
				pLastFirmPosition = &*iterPathNode;
				++iterPathNode;
			}
		}
	}
}


