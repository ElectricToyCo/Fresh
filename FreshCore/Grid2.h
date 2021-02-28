//
//  Grid2.h
//  Fresh
//
//  Created by Jeff Wofford on 12/2/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_Grid2_h
#define Fresh_Grid2_h

#include "FreshVector.h"

namespace fr
{
	
	template< typename CellT >
	class Grid2
	{
	public:
		
		typedef CellT element_t;
		typedef const CellT& ref_t;
		
		Grid2();
		explicit Grid2( const Vector2i& gridSize, const vec2& cellSize = vec2( 1 ), ref_t initialValue = element_t{} );
		// REQUIRES( gridSize.x > 0 && gridSize.y > 0 && cellSize.x > 0 && cellSize.y > 0 );
		
		void resize( const Vector2i& newGridSize, const Vector2i& oldOffsetIntoNew = Vector2i( 0 ), ref_t newCellInitialValue = element_t{} );
		void resizeToInclude( const Vector2i& pos, ref_t newCellInitialValue = element_t{} );
		
		SYNTHESIZE_GET( Vector2i, gridSize )
		SYNTHESIZE( vec2, cellSize )
		
		bool inBounds( const vec2& pos ) const;
		bool inBounds( const Vector2i& pos ) const;
		
		ref_t cellAt( const vec2& pos ) const;
		ref_t cellAt( const Vector2i& pos ) const;
		// REQUIRES( inBounds( pos ));

		void setCellAt( const vec2& pos, ref_t cellValue );
		void setCellAt( const Vector2i& pos, ref_t cellValue );
		// REQUIRES( inBounds( pos ));
		
		Vector2i worldToCell( const vec2& pos ) const;
		
		vec2 cellRelative( const Vector2i& pos, const vec2& inner ) const;
		vec2 cellUL( const Vector2i& pos ) const;
		vec2 cellCenter( const Vector2i& pos ) const;
		
		template< typename FunctionT >
		void eachCell( FunctionT&& fn ) const
		{
			Vector2i cellPos;
			for( cellPos.y = 0; cellPos.y < m_gridSize.y; ++cellPos.y )
			{
				for( cellPos.x = 0; cellPos.x < m_gridSize.x; ++cellPos.x )
				{
					fn( cellAt( cellPos ), cellPos );
				}
			}
		}
		
		std::vector< element_t >& cells() { return m_cells; }
		
	protected:
		
		size_t index( const Vector2i& cellPos ) const;
		
	private:
		
		Vector2i m_gridSize;
		vec2 m_cellSize;
		std::vector< element_t > m_cells;
		
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Implementation
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template< typename CellT >
	Grid2< CellT >::Grid2()
	{}
	
	template< typename CellT >
	Grid2< CellT >::Grid2( const Vector2i& gridSize, const vec2& cellSize, ref_t initialValue )
	:	m_gridSize( gridSize )
	,	m_cellSize( cellSize )
	,	m_cells( gridSize.x * gridSize.y, initialValue )
	{
		REQUIRES( gridSize.x > 0 && gridSize.y > 0 && cellSize.x > 0 && cellSize.y > 0 );
	}
	
	template< typename CellT >
	void Grid2< CellT >::resize( const Vector2i& newGridSize, const Vector2i& oldOffsetIntoNew, ref_t newCellInitialValue )
	{
		REQUIRES( newGridSize.x > 0 && newGridSize.y > 0 );
		
		if( newGridSize != m_gridSize || !oldOffsetIntoNew.isZero() )
		{
			auto oldGrid = m_cells;
			
			m_cells.resize( newGridSize.x * newGridSize.y, newCellInitialValue );
			
			// Copy old grid into new.
			//
			for( Vector2i dest( 0, 0 ); dest.y < newGridSize.y; ++dest.y )
			{
				for( dest.x = 0; dest.x < newGridSize.x; ++dest.x )
				{
					const auto src = dest - oldOffsetIntoNew;
					
					if( inBounds( src ) )
					{
						setCellAt( dest, oldGrid[ index( src ) ] );
					}
				}
			}
			
			m_gridSize = newGridSize;
		}
	}
	
	template< typename CellT >
	void Grid2< CellT >::resizeToInclude( const Vector2i& pos, ref_t newCellInitialValue )
	{
		Vector2i newGridSize( m_gridSize );
		Vector2i offset;
		
		for( int i = 0; i < 2; ++i )
		{
			if( newGridSize[ i ] <= pos[ i ] )
			{
				// Grow right/down if pos is "too large" for the current extents.
				//
				newGridSize[ i ] += pos[ i ] - newGridSize[ i ] + 1;
				offset[ i ] = 0;
			}
			else if( pos[ i ] < 0 )
			{
				// Grow left/up if pos is negative.
				//
				newGridSize[ i ] -= pos[ i ];
				offset[ i ] = pos[ i ];
			}
		}
		
		resize( newGridSize, offset, newCellInitialValue );
	}
	
	template< typename CellT >
	bool Grid2< CellT >::inBounds( const vec2& pos ) const
	{
		return inBounds( worldToCell( pos ));
	}
	
	template< typename CellT >
	bool Grid2< CellT >::inBounds( const Vector2i& pos ) const
	{
		return  0 <= pos.x && pos.x < m_gridSize.x &&
		0 <= pos.y && pos.y < m_gridSize.y;
	}
	
	template< typename CellT >
	typename Grid2< CellT >::ref_t Grid2< CellT >::cellAt( const vec2& pos ) const
	{
		return cellAt( worldToCell( pos ));
	}
	
	template< typename CellT >
	typename Grid2< CellT >::ref_t Grid2< CellT >::cellAt( const Vector2i& pos ) const
	{
		REQUIRES( inBounds( pos ));
		const size_t i = index( pos );
		ASSERT( i < m_cells.size() );
		return m_cells[ i ];
	}
	
	template< typename CellT >
	void Grid2< CellT >::setCellAt( const vec2& pos, ref_t cellValue )
	{
		return setCellAt( worldToCell( pos ), cellValue );
	}
	
	template< typename CellT >
	void Grid2< CellT >::setCellAt( const Vector2i& pos, ref_t cellValue )
	{
		REQUIRES( inBounds( pos ));
		
		const size_t i = index( pos );
		ASSERT( i < m_cells.size() );
		m_cells[ i ] = cellValue;
	}
	
	template< typename CellT >
	Vector2i Grid2< CellT >::worldToCell( const vec2& pos ) const
	{
		// std::floor() is important for correct behavior in negative space.
		return Vector2i( static_cast< int >( std::floor( pos.x / m_cellSize.x )),
						 static_cast< int >( std::floor( pos.y / m_cellSize.y ) ));
	}
	
	template< typename CellT >
	vec2 Grid2< CellT >::cellRelative( const Vector2i& pos, const vec2& inner ) const
	{
		return ( vector_cast< real >( pos ) + inner ) * m_cellSize;
	}
	
	template< typename CellT >
	vec2 Grid2< CellT >::cellUL( const Vector2i& pos ) const
	{
		return cellRelative( pos, vec2( 0 ));
	}
	
	template< typename CellT >
	vec2 Grid2< CellT >::cellCenter( const Vector2i& pos ) const
	{
		return cellRelative( pos, vec2( 0.5f ));
	}
	
	template< typename CellT >
	size_t Grid2< CellT >::index( const Vector2i& cellPos ) const
	{
		return cellPos.y * m_gridSize.x + cellPos.x;
	}
}

#endif
