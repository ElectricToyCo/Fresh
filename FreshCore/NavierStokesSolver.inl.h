#define FOR_EACH_CELL for ( i=1 ; i<=m_mutableCellWidth; i++ ) { for ( j=1 ; j<=m_mutableCellWidth; j++ ) {
#define END_FOR }}

#define SWAP(x0,x) {Real * tmp=x0;x0=x;x=tmp;}

namespace jeffwofford
{
	
	template< typename Real >
	NavierStokesSolver< Real >::NavierStokesSolver( size_t width )
	:	m_mutableCellWidth( width )
	,	m_actualCellWidth( m_mutableCellWidth + 2 )
	,	m_nActualCells( m_actualCellWidth * m_actualCellWidth )
	,	m_boundaryBehavior( Permeable )
	,	m_maxDensity( -1 )
	{
		assert( width > 0 );
		
		allocate_data();
		clear();
	}

	template< typename Real >
	NavierStokesSolver< Real >::~NavierStokesSolver()
	{
		delete[] m_arrVelocityX;
		delete[] m_arrVelocityY;
		delete[] m_arrVelocityXPrev;
		delete[] m_arrVelocityYPrev;
		delete[] m_arrDensity;
		delete[] m_arrDensityPrev;
	}

	template< typename Real >
	void NavierStokesSolver< Real >::updateDensities( Real diff, Real dt )
	{
		addSource( m_arrDensity, m_arrDensityPrev, dt );
		
		Real* dens = m_arrDensity;
		Real* dens_prev = m_arrDensityPrev;
		
		SWAP ( dens, dens_prev ); updateDiffusion ( 0, dens, dens_prev, diff, dt );
		SWAP ( dens, dens_prev ); updateAdvection ( 0, dens, dens_prev, m_arrVelocityX, m_arrVelocityY, dt );

		if( m_maxDensity >= 0 )
		{
			clampDensities( m_maxDensity );
		}
	}

	template< typename Real >
	void NavierStokesSolver< Real >::updateVelocities( Real visc, Real dt )
	{
		Real* u = m_arrVelocityX;
		Real* u0 = m_arrVelocityXPrev;
		Real* v = m_arrVelocityY;
		Real* v0 = m_arrVelocityYPrev;
		
		addSource ( u, u0, dt ); addSource ( v, v0, dt );
		SWAP ( u0, u ); updateDiffusion ( 1, u, u0, visc, dt );
		SWAP ( v0, v ); updateDiffusion ( 2, v, v0, visc, dt );
		project ( u, v, u0, v0 );
		SWAP ( u0, u ); SWAP ( v0, v );
		updateAdvection ( 1, u, u0, u0, v0, dt ); updateAdvection ( 2, v, v0, u0, v0, dt );
		project ( u, v, u0, v0 );
	}

	template< typename Real >
	void NavierStokesSolver< Real >::allocate_data()
	{
		m_arrVelocityX			= new Real[ m_nActualCells ];
		m_arrVelocityY			= new Real[ m_nActualCells ];
		m_arrVelocityXPrev		= new Real[ m_nActualCells ];
		m_arrVelocityYPrev		= new Real[ m_nActualCells ];
		m_arrDensity			= new Real[ m_nActualCells ];
		m_arrDensityPrev		= new Real[ m_nActualCells ];
	}

	template< typename Real >
	void NavierStokesSolver< Real >::shiftCells( Real* arr, int offsetX, int offsetY, Real clearedValue /*= 0*/ )
	{
		// Copy all cells values by the given offset.
		//
		int directionI = offsetX > 0 ? -1 : 1;
		int directionJ = offsetY > 0 ? -1 : 1;

		size_t startI = directionI > 0 ? 1 : m_actualCellWidth - 2;
		size_t startJ = directionJ > 0 ? 1 : m_actualCellWidth - 2;

		for( size_t i = startI; i < m_actualCellWidth; i += directionI )
		{
			const size_t offsetI = i - offsetX;

			for( size_t j = startJ; j < m_actualCellWidth; j += directionJ )
			{
				const size_t offsetJ = j - offsetY;

				if( offsetI >= m_actualCellWidth ||
					offsetJ >= m_actualCellWidth )
				{
					// Offsetting from out of bounds. Set to the clear value.
					//
					arr[ getIndex(i,j) ] = clearedValue;
				}
				else
				{
					arr[ getIndex(i,j) ] = arr[ getIndex(offsetI,offsetJ) ];
				}				
			}
		}
	}

	template< typename Real >
	void NavierStokesSolver< Real >::shiftCells( SimState state, int offsetX, int offsetY, Real clearedDensity /*= 0*/, Real clearedVelocityX /*= 0*/, Real clearedVelocityY /*= 0*/ )
	{
		shiftCells( getVelocityXArray( state ), offsetX, offsetY, clearedVelocityX );
		shiftCells( getVelocityYArray( state ), offsetX, offsetY, clearedVelocityY );
		shiftCells( getDensityArray( state ), offsetX, offsetY, clearedDensity );
	}

	template< typename Real >
	void NavierStokesSolver< Real >::clear( SimState state )
	{
		for ( size_t i = 0; i < m_nActualCells; ++i ) 
		{
			if( state == Current || state == Both )
			{
				m_arrVelocityX[i] = m_arrVelocityY[i] = m_arrDensity[i] = 0;
			}
			if( state == Previous || state == Both )
			{
				m_arrVelocityXPrev[i] = m_arrVelocityYPrev[i] = m_arrDensityPrev[i] = 0;
			}
		}
	}

	template< typename Real >
	Real* NavierStokesSolver< Real >::getVelocityXArray( SimState state ) const
	{
		assert( state != Both );		
		return state == Current ? m_arrVelocityX : m_arrVelocityXPrev;
	}
	
	template< typename Real >
	Real* NavierStokesSolver< Real >::getVelocityYArray( SimState state ) const
	{
		assert( state != Both );
		return state == Current ? m_arrVelocityY : m_arrVelocityYPrev;
	}
	
	template< typename Real >
	Real* NavierStokesSolver< Real >::getDensityArray( SimState state ) const
	{
		assert( state != Both );
		return state == Current ? m_arrDensity : m_arrDensityPrev;
	}
	
	template< typename Real >
	Real& NavierStokesSolver< Real >::getCellVelocityX( SimState state, size_t x, size_t y )
	{
		assert( state != Both );
		assert( 0 <= x && x < m_actualCellWidth );
		assert( 0 <= y && y < m_actualCellWidth );
		return state == Current ? 
		m_arrVelocityX[ getIndex( x, y ) ] :
		m_arrVelocityXPrev[ getIndex( x, y ) ];
	}

	template< typename Real >
	Real& NavierStokesSolver< Real >::getCellVelocityY( SimState state, size_t x, size_t y )
	{
		assert( state != Both );
		assert( 0 <= x && x < m_actualCellWidth );
		assert( 0 <= y && y < m_actualCellWidth );
		return state == Current ? 
		m_arrVelocityY[ getIndex( x, y ) ] :
		m_arrVelocityYPrev[ getIndex( x, y ) ];
	}

	template< typename Real >
	Real& NavierStokesSolver< Real >::getCellDensity( SimState state, size_t x, size_t y )
	{
		assert( state != Both );
		assert( 0 <= x && x < m_actualCellWidth );
		assert( 0 <= y && y < m_actualCellWidth );
		return state == Current ? 
		m_arrDensity[ getIndex( x, y ) ] :
		m_arrDensityPrev[ getIndex( x, y ) ];
	}

	template< typename Real >
	void NavierStokesSolver< Real >::addSource( Real* arrDest, Real* arrSrc, Real dt )
	{
		for( size_t i = 0; i < m_nActualCells; ++i )
		{
			arrDest[ i ] += dt * arrSrc[ i ];
		}
	}

	template< typename Real >
	void NavierStokesSolver< Real >::setBoundaryCells( int b, Real* arrCells )
	{
		switch( m_boundaryBehavior )
		{
		case Blocking:
			enforceBlockingBoundaries( b, arrCells );
			break;
		case Wrapping:
			enforceWrappingBoundaries( b, arrCells );
			break;
		default:
			break;		// Do nothing, i.e. "permeable" boundaries.
		}
	}

	template< typename Real >
	void NavierStokesSolver< Real >::enforceBlockingBoundaries( int b, Real * x )
	{
		for ( size_t i=1 ; i <= m_mutableCellWidth; ++i ) 
		{
			// Set the left edge.
			x[getIndex(0  ,i)] = b==1 ? -x[getIndex(1,i)] : x[getIndex(1,i)];

			// Set the right edge.
			x[getIndex(m_mutableCellWidth+1,i)] = b==1 ? -x[getIndex(m_mutableCellWidth,i)] : x[getIndex(m_mutableCellWidth,i)];

			// Set the top edge.
			x[getIndex(i,0  )] = b==2 ? -x[getIndex(i,1)] : x[getIndex(i,1)];

			// Set the bottom edge.
			x[getIndex(i,m_mutableCellWidth+1)] = b==2 ? -x[getIndex(i,m_mutableCellWidth)] : x[getIndex(i,m_mutableCellWidth)];
		}
		
		// Set the four corners.
		x[getIndex(0  ,0  )] = 0.5f*(x[getIndex(1,0  )]+x[getIndex(0  ,1)]);		// UL
		x[getIndex(0  ,m_mutableCellWidth+1)] = 0.5f*(x[getIndex(1,m_mutableCellWidth+1)]+x[getIndex(0  ,m_mutableCellWidth)]);	// BL
		x[getIndex(m_mutableCellWidth+1,0  )] = 0.5f*(x[getIndex(m_mutableCellWidth,0  )]+x[getIndex(m_mutableCellWidth+1,1)]);	// UR
		x[getIndex(m_mutableCellWidth+1,m_mutableCellWidth+1)] = 0.5f*(x[getIndex(m_mutableCellWidth,m_mutableCellWidth+1)]+x[getIndex(m_mutableCellWidth+1,m_mutableCellWidth)]);	// BR
	}

	template< typename Real >
	void NavierStokesSolver< Real >::enforceWrappingBoundaries( int b, Real * x )
	{
		for ( size_t i=1 ; i <= m_mutableCellWidth; ++i ) 
		{
			// Set the left edge.
			x[getIndex(0  ,i)] = b==1 ? -x[getIndex(m_mutableCellWidth,i)] : x[getIndex(m_mutableCellWidth,i)];

			// Set the right edge.
			x[getIndex(m_mutableCellWidth+1,i)] = b==1 ? -x[getIndex(1,i)] : x[getIndex(1,i)];

			// Set the top edge.
			x[getIndex(i,0  )] = b==2 ? -x[getIndex(i,m_mutableCellWidth)] : x[getIndex(i,m_mutableCellWidth)];

			// Set the bottom edge.
			x[getIndex(i,m_mutableCellWidth+1)] = b==2 ? -x[getIndex(i,1)] : x[getIndex(i,1)];
		}
		
		// Set the four corners.
		x[getIndex(0  ,0  )] = 0.5f*(x[getIndex(m_mutableCellWidth,0  )]+x[getIndex(0  ,m_mutableCellWidth)]);		// UL
		x[getIndex(0  ,m_mutableCellWidth+1)] = 0.5f*(x[getIndex(m_mutableCellWidth,m_mutableCellWidth+1)]+x[getIndex(0  ,1)]);	// BL
		x[getIndex(m_mutableCellWidth+1,0  )] = 0.5f*(x[getIndex(1,0  )]+x[getIndex(m_mutableCellWidth+1,m_mutableCellWidth)]);	// UR
		x[getIndex(m_mutableCellWidth+1,m_mutableCellWidth+1)] = 0.5f*(x[getIndex(1,m_mutableCellWidth+1)]+x[getIndex(m_mutableCellWidth+1,1)]);	// BR
	}

	template< typename Real >
	void NavierStokesSolver< Real >::lin_solve ( int b, Real * x, Real * x0, Real a, Real c )
	{
		size_t i, j, k;
		
		for ( k=0 ; k<20 ; k++ ) {
			FOR_EACH_CELL
			x[getIndex(i,j)] = (x0[getIndex(i,j)] + a*(x[getIndex(i-1,j)]+x[getIndex(i+1,j)]+x[getIndex(i,j-1)]+x[getIndex(i,j+1)]))/c;
			END_FOR
			setBoundaryCells( b, x );
		}
	}

	template< typename Real >
	void NavierStokesSolver< Real >::updateDiffusion( int b, Real * x, Real * x0, Real diff, Real dt )
	{
		Real a=dt*diff*m_mutableCellWidth*m_mutableCellWidth;
		lin_solve ( b, x, x0, a, 1+4*a );
	}

	template< typename Real >
	void NavierStokesSolver< Real >::clampPositionToBoundary( Real& x, Real& y, size_t& i0, size_t& j0, size_t& i1, size_t& j1 )
	{
		switch( m_boundaryBehavior )
		{
		case Blocking:
		case Permeable:
		default:
			if (x<0.5f) x=0.5f; 
			if (x>m_mutableCellWidth+0.5f) x=m_mutableCellWidth+0.5f; 
		
			if (y<0.5f) y=0.5f; 
			if (y>m_mutableCellWidth+0.5f) y=m_mutableCellWidth+0.5f; 
			i0=(size_t)x; i1=i0+1;
			j0=(size_t)y; j1=j0+1;
			break;
		case Wrapping:
			while (x<0.5f) x += m_mutableCellWidth;
			while (x>m_mutableCellWidth+0.5f) x -= m_mutableCellWidth; 
		
			while (y<0.5f) y += m_mutableCellWidth;
			while (y>m_mutableCellWidth+0.5f) y -= m_mutableCellWidth; 
			i0=(size_t)x; i1 = ( i0+1 ) % m_actualCellWidth;
			j0=(size_t)y; j1 = ( j0+1 ) % m_actualCellWidth;
			break;
		}

		assert( 0 <= x && x < m_actualCellWidth );
		assert( 0 <= y && y < m_actualCellWidth );
		assert( 0 <= i0 && i0 < m_actualCellWidth );
		assert( 0 <= i1 && i1 < m_actualCellWidth );
		assert( 0 <= j0 && j0 < m_actualCellWidth );
		assert( 0 <= j1 && j1 < m_actualCellWidth );
	}

	template< typename Real >
	void NavierStokesSolver< Real >::clampDensities( Real maxDensity )
	{
		for( size_t i = 0; i < m_actualCellWidth; ++i ) 
		{ 
			for ( size_t j = 1; j < m_actualCellWidth; ++j ) 
			{
				m_arrDensity[ getIndex(i,j)] = std::min( maxDensity, m_arrDensity[ getIndex(i,j)] );
			}
		}
	}

	template< typename Real >
	void NavierStokesSolver< Real >::updateAdvection( int b, Real * d, Real * d0, Real * u, Real * v, Real dt )
	{
		size_t i, j, i0, j0, i1, j1;
		Real x, y, s0, t0, s1, t1, dt0;
		
		dt0 = dt*m_mutableCellWidth;
		FOR_EACH_CELL

		// Find the position of the particle that would move to this density cell
		// from some other density cell during this timestep.
		x = i-dt0*u[getIndex(i,j)]; y = j-dt0*v[getIndex(i,j)];

		// Clamp the position of this particle based on the bounds of the world.

		clampPositionToBoundary( x, y, i0, j0, i1, j1 );

		// Determine the weights to be given to the four samples adjacent to 
		// the particle's starting location (bilinear filtering).
		s1 = x-i0; s0 = 1-s1; t1 = y-j0; t0 = 1-t1;
		
		// Read a weighted sample from the density grid from the four points
		// nearest the particle.
		d[getIndex(i,j)] =	s0*(t0*d0[getIndex(i0,j0)]+t1*d0[getIndex(i0,j1)]) +
							s1*(t0*d0[getIndex(i1,j0)]+t1*d0[getIndex(i1,j1)]);
		
		END_FOR
		setBoundaryCells( b, d );
	}

	template< typename Real >
	void NavierStokesSolver< Real >::project ( Real * u, Real * v, Real * p, Real * div )
	{
		size_t i, j;
		
		FOR_EACH_CELL
		div[getIndex(i,j)] = -0.5f*(u[getIndex(i+1,j)]-u[getIndex(i-1,j)]+v[getIndex(i,j+1)]-v[getIndex(i,j-1)])/m_mutableCellWidth;
		p[getIndex(i,j)] = 0;
		END_FOR	
		setBoundaryCells( 0, div ); setBoundaryCells( 0, p );
		
		lin_solve ( 0, p, div, 1, 4 );
		
		FOR_EACH_CELL
		u[getIndex(i,j)] -= 0.5f*m_mutableCellWidth*(p[getIndex(i+1,j)]-p[getIndex(i-1,j)]);
		v[getIndex(i,j)] -= 0.5f*m_mutableCellWidth*(p[getIndex(i,j+1)]-p[getIndex(i,j-1)]);
		END_FOR
		setBoundaryCells( 1, u ); setBoundaryCells( 2, v );
	}

}