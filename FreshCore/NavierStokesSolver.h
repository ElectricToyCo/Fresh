#pragma once 

#include <cassert>

namespace jeffwofford
{	
	template< typename Real >
	class NavierStokesSolver
	{
	public:
		
		// Used to indicate whether the "current" (normally-writable,
		// in progress) velocity and density state or the "previous"
		// (completed) state are being dealt with in a function.
		enum SimState
		{
			Current,
			Previous,
			Both
		};

		enum BoundaryBehavior
		{
			Blocking,				// The default.
			Permeable,
			Wrapping
		};
		
		explicit NavierStokesSolver( size_t width );
		~NavierStokesSolver();
		
		void setBoundaryBehavior( BoundaryBehavior behavior )			{ m_boundaryBehavior = behavior; }

		void setMaxDensity( Real maxDensity )							{ m_maxDensity = maxDensity; }
		// Values < 0 prevent max density from being enforced.

		void shiftCells( SimState state, int offsetX, int offsetY, Real clearedDensity = 0, Real clearedVelocityX = 0, Real clearedVelocityY = 0 );

		void clear( SimState state = Both );
		// Resets cell values in the given state(s) to 0.
		
		void updateDensities( Real diff, Real dt );
		void updateVelocities( Real visc, Real dt );
		
		Real* getVelocityXArray( SimState state ) const;
		// REQUIRES( state != Both );
		Real* getVelocityYArray( SimState state ) const;
		// REQUIRES( state != Both );
		Real* getDensityArray( SimState state ) const;
		// REQUIRES( state != Both );
		
		Real& getCellVelocityX( SimState state, size_t x, size_t y );
		// REQUIRES( state != Both );
		Real& getCellVelocityY( SimState state, size_t x, size_t y );
		// REQUIRES( state != Both );
		Real& getCellDensity( SimState state, size_t x, size_t y );
		// REQUIRES( state != Both );

	protected:
		
		inline size_t getIndex( size_t x, size_t y ) const
		{
			return x + m_actualCellWidth * y;
		}
		
		void addSource( Real* arrDest, Real* arrSrc, Real dt );
		void setBoundaryCells( int b, Real* arrCells );
		void lin_solve( int b, Real * x, Real * x0, Real a, Real c );
		void updateDiffusion( int b, Real * x, Real * x0, Real diff, Real dt );
		void updateAdvection( int b, Real * d, Real * d0, Real * u, Real * v, Real dt );
		void project( Real * u, Real * v, Real * p, Real * div );
		
		void enforceBlockingBoundaries( int b, Real * x );
		void enforceWrappingBoundaries( int b, Real * x );
		void clampPositionToBoundary( Real& x, Real& y, size_t& i0, size_t& j0, size_t& i1, size_t& j1 );
		
		void clampDensities( Real maxDensity );

		void shiftCells( Real* arr, int offsetX, int offsetY, Real clearedValue = 0 );

	private:
		
		size_t m_mutableCellWidth;			// real of cells in one axis, except for edge cells.
		size_t m_actualCellWidth;			// real of cells in one axis, including edge cells.
		size_t m_nActualCells;				// real of actual cells in all both axes.
		
		Real* m_arrVelocityX;
		Real* m_arrVelocityY;
		Real* m_arrVelocityXPrev;
		Real* m_arrVelocityYPrev;
		Real* m_arrDensity;
		Real* m_arrDensityPrev;

		Real m_maxDensity;

		BoundaryBehavior m_boundaryBehavior;

		void allocate_data();
		
		// Prevent copying this object.
		//
		NavierStokesSolver( const NavierStokesSolver& );
		void operator=( const NavierStokesSolver& );
	};

}

#include "NavierStokesSolver.inl.h"
