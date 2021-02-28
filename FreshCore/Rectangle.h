/*
 *  Rectangle.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/19/08.
 *  Copyright 2008 jeffwofford.com. All rights reserved.
 *
 */

#ifndef RECTANGLE_H_INCLUDED_
#define RECTANGLE_H_INCLUDED_

#include "Vector2.h"
#include "FreshDebug.h"
#include "SerializeVector.h"
#include <limits>
#include <cmath>

namespace fr
{

	template< typename Real >
	class Rectangle
	{
	public:

		static const Rectangle INVERSE_INFINITE;

		typedef Real element_type;
		static const size_t nComponents = 4;

		Rectangle() : m_ulCorner( 0, 0 ), m_brCorner( 0, 0 ) {}	// Sets all sides to 0
		
		Rectangle( Real left_, Real top_, Real right_, Real bottom_ )
		:	m_ulCorner( left_, top_ )
		,	m_brCorner( right_, bottom_ )
		{}
		
		Rectangle( const Vector2< Real >& ulCorner_, const Vector2< Real >& brCorner_ )
		:	m_ulCorner( ulCorner_ )
		,	m_brCorner( brCorner_ )
		{}
		
		void set( Real left_, Real top_, Real right_, Real bottom_ )
		{
			m_ulCorner.set( left_, top_ );
			m_brCorner.set( right_, bottom_ );
		}
		void set( const Vector2< Real >& ulCorner_, const Vector2< Real >& brCorner_ )
		{
			m_ulCorner = ulCorner_;
			m_brCorner = brCorner_;
		}
		
		bool doesOverlap( const Rectangle& other, bool doEdgesInclude = true ) const
		{
			if( doEdgesInclude )
			{
				return !( other.right() < left() || other.left() > right() || other.bottom() < top() || other.top() > bottom() );
			}
			else
			{
				return !( other.right() <= left() || other.left() >= right() || other.bottom() <= top() || other.top() >= bottom() );
			}
		}

		bool doesEnclose( const Vector2< Real >& point, bool doEdgesInclude = true ) const
		{
			if( doEdgesInclude )
			{
				return point.x >= m_ulCorner.x && point.x <= m_brCorner.x && point.y >= m_ulCorner.y && point.y <= m_brCorner.y;
			}
			else
			{
				return point.x > m_ulCorner.x && point.x < m_brCorner.x && point.y > m_ulCorner.y && point.y < m_brCorner.y;
			}
		}
		
		bool doesEnclose( const Rectangle& other, bool doEdgesInclude = true ) const
		{
			return doesEnclose( other.ulCorner(), doEdgesInclude ) && doesEnclose( other.brCorner(), doEdgesInclude );
		}
		
		void growToEncompass( const Rectangle< Real >& other )
		{
			m_ulCorner.x = std::min( m_ulCorner.x, other.m_ulCorner.x );
			m_ulCorner.y = std::min( m_ulCorner.y, other.m_ulCorner.y );
			m_brCorner.x = std::max( m_brCorner.x, other.m_brCorner.x );
			m_brCorner.y = std::max( m_brCorner.y, other.m_brCorner.y );
		}
		
		void growToEncompass( const Vector2< Real >& vec )
		{
			m_ulCorner.x = std::min( m_ulCorner.x, vec.x );
			m_ulCorner.y = std::min( m_ulCorner.y, vec.y );
			m_brCorner.x = std::max( m_brCorner.x, vec.x );
			m_brCorner.y = std::max( m_brCorner.y, vec.y );
		}
		
		const Vector2< Real >& ulCorner() const										{ return m_ulCorner; }
		const Vector2< Real >& brCorner() const										{ return m_brCorner; }
		void ulCorner( const Vector2< Real >& p )									{ m_ulCorner = p; }
		void brCorner( const Vector2< Real >& p )									{ m_brCorner = p; }
		Vector2< Real > urCorner() const											{ return Vector2< Real >( m_brCorner.x, m_ulCorner.y ); }
		Vector2< Real > blCorner() const											{ return Vector2< Real >( m_ulCorner.x, m_brCorner.y ); }
		
		Vector2< Real > corner( int i ) const
		{
			switch( i )
			{
				default:
					ASSERT( false );
				case 0:
					return ulCorner();
				case 1:
					return urCorner();
				case 2:
					return brCorner();
				case 3:
					return blCorner();
			}
		}
		
		Vector2< Real > midpoint() const											{ return ( m_ulCorner + m_brCorner ) / Real( 2 ); }
		
		Vector2< Real > dimensions() const											{ return m_brCorner - m_ulCorner; }
		
		Real width() const															{ return m_brCorner.x - m_ulCorner.x; }
		Real height() const															{ return m_brCorner.y - m_ulCorner.y; }
		
		Real left() const															{ return m_ulCorner.x; }
		Real right() const															{ return m_brCorner.x; }
		Real top() const															{ return m_ulCorner.y; }
		Real bottom() const															{ return m_brCorner.y; }
		
		void left( Real x )															{ m_ulCorner.x = x; }
		void right( Real x )														{ m_brCorner.x = x; }
		void top( Real x )															{ m_ulCorner.y = x; }
		void bottom( Real x )														{ m_brCorner.y = x; }
		
		Vector2< Real > clamp( const Vector2< Real >& v ) const						{ return Vector2< Real >( fr::clamp( v.x, left(), right() ), fr::clamp( v.y, top(), bottom() )); }

		Rectangle intersection( const Rectangle& other ) const
		{
			Rectangle result( std::max( m_ulCorner.x, other.m_ulCorner.x ),
							  std::max( m_ulCorner.y, other.m_ulCorner.y ),
							  std::min( m_brCorner.x, other.m_brCorner.x ),
							  std::min( m_brCorner.y, other.m_brCorner.y ));
			
			// Check for non-intersections.
			if( result.m_ulCorner.x > result.m_brCorner.x )
			{
				result.m_ulCorner.x = infinity();
				result.m_brCorner.x = negativeInfinity();
			}
			if( result.m_ulCorner.y > result.m_brCorner.y )
			{
				result.m_ulCorner.y = infinity();
				result.m_brCorner.y = negativeInfinity();
			}
			return result;
		}
		
		// Positive for expand; negative for contract.
		void expandContract( const Vector2< Real >& amount )
		{
			m_ulCorner -= amount;
			m_brCorner += amount;
			
			// Check for over-contraction and clamp the reduction to give dimensions of 0.
			//
			for( int i = 0; i < 2; ++i )
			{
				if( amount[i] < 0 && m_ulCorner[i] > m_brCorner[i] )
				{
					m_ulCorner[i] = m_brCorner[i] = ( m_ulCorner[i] + m_brCorner[i] ) / Real( 2 );
				}
			}
		}
		
		void expandContract( Real amount )
		{
			expandContract( Vector2< Real >( amount ));
		}
		
		void translate( const Vector2< Real >& translation )
		{
			m_ulCorner += translation;
			m_brCorner += translation;
		}
		
		static Real infinity()
		{
			if( std::numeric_limits< Real >::has_infinity )
			{
				return std::numeric_limits< Real >::infinity();
			}
			else
			{
				return std::numeric_limits< Real >::max();
			}
		}
		
		static Real negativeInfinity()
		{
			if( std::numeric_limits< Real >::has_infinity )
			{
				return -std::numeric_limits< Real >::infinity();
			}
			else
			{
				return std::numeric_limits< Real >::min();
			}
		}
		
		void setToInverseInfinity()
		{
			// Sets the rectangle so that any growToEncompass() operation on a non-infinite value will
			// cause the rectangle to precisely encompass the target geometry.
			//
			m_ulCorner.x = m_ulCorner.y = infinity();
			m_brCorner.x = m_brCorner.y = negativeInfinity();
		}
		
		bool isInverseInfiniteWidth() const
		{
			return m_ulCorner.x > m_brCorner.x;
		}
		
		bool isInverseInfiniteHeight() const
		{
			return m_ulCorner.y > m_brCorner.y;
		}
		
		bool isWellFormed() const
		{
			return m_ulCorner.x <= m_brCorner.x &&
			       m_ulCorner.y <= m_brCorner.y;
		}
		
		bool operator==( const Rectangle& other ) const
		{
			return m_ulCorner == other.m_ulCorner && m_brCorner == other.m_brCorner;
		}
		bool operator!=( const Rectangle& other ) const
		{
			return !operator==( other );
		}
		
		Real operator[]( size_t index ) const
		{
			switch( index )
			{
				default:
					ASSERT( false );
				case 0:
					return m_ulCorner.x;
				case 1:
					return m_ulCorner.y;
				case 2:
					return m_brCorner.x;
				case 3:
					return m_brCorner.y;
			}
			return 0;
		}
		
		Real& operator[]( size_t index )
		{
			switch( index )
			{
				default:
					ASSERT( false );
				case 0:
					return m_ulCorner.x;
				case 1:
					return m_ulCorner.y;
				case 2:
					return m_brCorner.x;
				case 3:
					return m_brCorner.y;
			}
		}
		
		void regularize()
		{
			// Max sure that the "bottom" is really >= the "top", and so forth.
			//
			if( m_brCorner.x < m_ulCorner.x )
			{
				std::swap( m_brCorner.x, m_ulCorner.x );
			}
			if( m_brCorner.y < m_ulCorner.y )
			{
				std::swap( m_brCorner.y, m_ulCorner.y );
			}
		}
		
	private:
		
		Vector2< Real > m_ulCorner;
		Vector2< Real > m_brCorner;
	};

	template< typename Real >
	std::ostream& operator<<( std::ostream& out, const Rectangle< Real >& rectangle )
	{
		SerializeVector::write( out, rectangle );
		return out;
	}
	
	template< typename Real >
	std::istream& operator>>( std::istream& in, Rectangle< Real >& rectangle )
	{
		SerializeVector::read( in, rectangle );
		return in;
	}

	typedef Rectangle< int > Rectanglei;
	typedef Rectangle< unsigned int > Rectangleui;
	typedef Rectangle< float > Rectanglef;
	typedef Rectangle< double > Rectangled;

	template<>
	inline unsigned int Rectangle< unsigned int >::negativeInfinity()
	{
		return std::numeric_limits< unsigned int >::min();
	}

	template<> const Rectangle< int >			Rectangle< int >::INVERSE_INFINITE;
	template<> const Rectangle< unsigned int >  Rectangle< unsigned int >::INVERSE_INFINITE;
	template<> const Rectangle< float >			Rectangle< float >::INVERSE_INFINITE;
	template<> const Rectangle< double >		Rectangle< double >::INVERSE_INFINITE;
}

#undef RECT_MIN
#undef RECT_MAX

#endif
