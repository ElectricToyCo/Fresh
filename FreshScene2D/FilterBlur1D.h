//
//  FilterBlur1D.h
//  Fresh
//
//  Created by Jeff Wofford on 8/29/15.
//  Copyright (c) 2015 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_FilterBlur1D_h
#define Fresh_FilterBlur1D_h

#include "DisplayFilter.h"

namespace fr
{
	
	class FilterBlur1D : public DisplayFilter
	{
		FRESH_DECLARE_CLASS( FilterBlur1D, DisplayFilter );
	public:
		
		SYNTHESIZE( int, size );
		SYNTHESIZE( real, step );
		SYNTHESIZE( int, axis );
		SYNTHESIZE( real, offset );
		SYNTHESIZE( bool, glow );

		virtual ShaderProgram::ptr shaderProgram( Texture::ptr textureToFilter ) const override;
		
		int nTaps() const		{ return m_size * 2 + 1; }
		
	protected:
		
		virtual vec2 rectExpansion() const override;
		
	private:
		
		DVAR( int, m_size, 7 );
		DVAR( real, m_step, 1 );
		DVAR( int, m_axis, 0 );			// X axis.
		DVAR( real, m_offset, 0 );
		DVAR( bool, m_glow, false );
		
		mutable ShaderProgram::ptr m_program;
		mutable int m_lastSideTaps = -1;
		mutable real m_lastStep = -1;
		mutable bool m_lastGlow = false;
	};
	
}

#endif
