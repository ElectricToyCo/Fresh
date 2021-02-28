//
//  SpatialHash.h
//  Fresh
//
//  Created by Jeff Wofford on 4/13/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_SpatialHash_h
#define Fresh_SpatialHash_h

#include <vector>
#include <algorithm>

namespace fr
{
	template< typename PosT, typename ValueT >
	class SpatialHash
	{
	public:
		
		typedef PosT position_type;
		
		typedef ValueT value_type;
		typedef ValueT& reference;
		typedef const ValueT& const_reference;
		
		SpatialHash( size_t maxBins, real spatialBinSize );
		
		void clear();
		void add( const position_type& minCorner, const position_type& maxCorner, const_reference element );
				
		template< typename Function >
		void eachPair( Function&& fn ) const;

		template< typename Function >
		void eachOverlapping( const position_type& minCorner, const position_type& maxCorner, Function&& fn ) const;

	protected:
		
		inline size_t hash( size_t x, size_t y ) const
		{
			return ( x * 1640531513ul ^ y * 2654435789ul ) % m_bins.size();
		}
		
		template< typename Function >
		void eachBin( const position_type& minCorner, const position_type& maxCorner, Function&& fn ) const;
		
	private:

		typedef std::vector< value_type > Bin;
		std::vector< Bin > m_bins;
		real m_binSize;
		
		mutable std::vector< std::pair< value_type, value_type >> m_pairs;	// Just used in eachPair(), but kept here to avoid re-allocations.
	};
	
	
	///////////////////////////////////////////////////////
	
	template< typename PosT, typename ValueT >
	SpatialHash< PosT, ValueT >::SpatialHash( size_t maxBins, real spatialBinSize )
	:	m_bins( maxBins )
	,	m_binSize( spatialBinSize )
	{}
	
	template< typename PosT, typename ValueT >
	void SpatialHash< PosT, ValueT >::clear()
	{
		for( auto& bin : m_bins )
		{
			bin.clear();
		}
	}
	
	template< typename PosT, typename ValueT >
	void SpatialHash< PosT, ValueT >::add( const position_type& minCorner, const position_type& maxCorner, const_reference element )
	{
		eachBin( minCorner, maxCorner, [&]( Bin& bin )
		{
			bin.push_back( element );
		} );
	}
	
	template< typename PosT, typename ValueT >
	template< typename Function >
	void SpatialHash< PosT, ValueT >::eachPair( Function&& fn ) const
	{
		// Find all unique collision pairs.
		//
		for( auto& bin : m_bins )
		{
			if( bin.size() > 1 )
			{
				for( auto outer = bin.begin(); outer + 1 != bin.end(); ++outer )
				{
					for( auto inner = outer + 1; inner != bin.end(); ++inner )
					{
						if( *inner != *outer )
						{
							// Ensure that the pairs (A,B) and (B,A) are identical.
							auto lesser = *outer;
							auto greater = *inner;
							if( greater < lesser )
							{
								std::swap( lesser, greater );
							}
							m_pairs.emplace_back( lesser, greater );
						}
					}
				}
			}
		}
		
		// Ensure that pairs are unique.
		//
		std::sort( m_pairs.begin(), m_pairs.end() );
		m_pairs.erase( std::unique( m_pairs.begin(), m_pairs.end() ), m_pairs.end() );
		
		// Report pairs one at a time.
		//
		for( const auto& pair : m_pairs )
		{
			fn( pair.first, pair.second );
		}
		
		m_pairs.clear();
	}
	
	template< typename PosT, typename ValueT >
	template< typename Function >
	void SpatialHash< PosT, ValueT >::eachOverlapping( const position_type& minCorner, const position_type& maxCorner, Function&& fn ) const
	{
		eachBin( minCorner, maxCorner, [&]( const Bin& bin )
				{
					for( auto& value: bin )
					{
						fn( value );
					}
				} );
	}
	
	template< typename PosT, typename ValueT >
	template< typename Function >
	void SpatialHash< PosT, ValueT >::eachBin( const position_type& minCorner, const position_type& maxCorner, Function&& fn ) const
	{
		static_assert( position_type::nComponents == 2, "SpatialHash requires a 2d-vector" );
		
		// Calculate quantized corners.
		//
		int mins[ position_type::nComponents ];
		int maxs[ position_type::nComponents ];
		for( size_t i = 0; i < position_type::nComponents; ++i )
		{
			mins[ i ] = static_cast< int >( std::floor( minCorner[ i ] / m_binSize ));
			maxs[ i ] = static_cast< int >( std::floor( maxCorner[ i ] / m_binSize ));
		}
		
		// Step from the min corner to the max.
		//
		for( int y = mins[ 1 ]; y <= maxs[ 1 ]; ++y )
		{
			for( int x = mins[ 0 ]; x <= maxs[ 0 ]; ++x )
			{
				const size_t index = hash( x, y );
				auto& bin = const_cast< Bin& >( m_bins[ index ] );
				
				fn( bin );
			}
		}
	}
}

#endif
