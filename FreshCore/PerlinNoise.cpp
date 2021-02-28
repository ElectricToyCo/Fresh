#include "PerlinNoise.h"
using namespace fr;

namespace
{
	inline real s_curve( real t )
	{
		return t * t * ( 3.0f - 2.0f * t );
	}
	
	void normalize2( real v[2] )
	{
		real s;
		
		s = std::sqrt(v[0] * v[0] + v[1] * v[1]);
		v[0] = v[0] / s;
		v[1] = v[1] / s;
	}
	
	void normalize3( real v[3] )
	{
		real s;
		
		s = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
		v[0] = v[0] / s;
		v[1] = v[1] / s;
		v[2] = v[2] / s;
	}
}

namespace fr
{

	Noise::Noise( uint seed )
	:	m_generator( seed )
	,	m_distribution( 0, RAND_MAX )
	,	random( std::bind( m_distribution, m_generator ))
	{
		this->seed( seed );
	}
	
	void Noise::seed( uint newSeed )
	{
		m_generator.seed( newSeed );
		
		int i, j, k;
		
		for( i = 0; i < B; ++i )
		{
			p[ i ] = i;
			
			g1[ i ] = (real)((random() % (B + B)) - B) / B;
			
			for( j = 0; j < 2 ; ++j )
			{
				g2[i][j] = (real)((random() % (B + B)) - B) / B;
			}
			normalize2( g2[i] );
			
			for( j = 0; j < 3; ++j )
			{
				g3[i][j] = (real)((random() % (B + B)) - B) / B;
			}
			normalize3( g3[i] );
		}
		
		while( --i )
		{
			k = p[ i ];
			const int inner = random() % B;
			
			p[ i ] = p[ inner ];
			p[ inner ] = k;
		}
		
		for( i = 0 ; i < B + 2 ; i++)
		{
			p[B + i] = p[i];
			g1[B + i] = g1[i];
			for (j = 0 ; j < 2 ; j++)
			{
				g2[B + i][j] = g2[i][j];
			}
			for (j = 0 ; j < 3 ; j++)
			{
				g3[B + i][j] = g3[i][j];
			}
		}
	}
	
#define setup(i,b0,b1,r0,r1)\
	t = vec[i] + N;\
	b0 = ((int)t) & BM;\
	b1 = (b0+1) & BM;\
	r0 = t - (int)t;\
	r1 = r0 - 1.0f;
	
	real Noise::at( real arg )
	{
		int bx0, bx1;
		real rx0, rx1, sx, t, u, v, vec[1];

		vec[0] = arg;

		setup( 0, bx0,bx1, rx0,rx1);

		sx = s_curve( rx0 );

		u = rx0 * g1[ p[ bx0 ] ];
		v = rx1 * g1[ p[ bx1 ] ];

		return lerp( u, v, sx );
	}

	real Noise::at( const vec2& vec )
	{
		int bx0, bx1, by0, by1, b00, b10, b01, b11;
		real rx0, rx1, ry0, ry1, *q, sx, sy, a, b, t, u, v;
		int i, j;

		setup(0, bx0,bx1, rx0,rx1);
		setup(1, by0,by1, ry0,ry1);

		i = p[ bx0 ];
		j = p[ bx1 ];

		b00 = p[ i + by0 ];
		b10 = p[ j + by0 ];
		b01 = p[ i + by1 ];
		b11 = p[ j + by1 ];

		sx = s_curve(rx0);
		sy = s_curve(ry0);

#define at2(rx,ry) ( rx * q[0] + ry * q[1] )

		q = g2[ b00 ] ; u = at2(rx0,ry0);
		q = g2[ b10 ] ; v = at2(rx1,ry0);
		a = lerp( u, v, sx );

		q = g2[ b01 ] ; u = at2(rx0,ry1);
		q = g2[ b11 ] ; v = at2(rx1,ry1);
		b = lerp( u, v, sx );

		return lerp( a, b, sy );
	}

	real Noise::at( const vec3& vec )
	{
		int bx0, bx1, by0, by1, bz0, bz1, b00, b10, b01, b11;
		real rx0, rx1, ry0, ry1, rz0, rz1, *q, sy, sz, a, b, c, d, t, u, v;
		int i, j;

		setup(0, bx0,bx1, rx0,rx1);
		setup(1, by0,by1, ry0,ry1);
		setup(2, bz0,bz1, rz0,rz1);

		i = p[ bx0 ];
		j = p[ bx1 ];

		b00 = p[ i + by0 ];
		b10 = p[ j + by0 ];
		b01 = p[ i + by1 ];
		b11 = p[ j + by1 ];

		t  = s_curve(rx0);
		sy = s_curve(ry0);
		sz = s_curve(rz0);

#define at3(rx,ry,rz) ( rx * q[0] + ry * q[1] + rz * q[2] )

		q = g3[ b00 + bz0 ] ; u = at3(rx0,ry0,rz0);
		q = g3[ b10 + bz0 ] ; v = at3(rx1,ry0,rz0);
		a = lerp( u, v, t );

		q = g3[ b01 + bz0 ] ; u = at3(rx0,ry1,rz0);
		q = g3[ b11 + bz0 ] ; v = at3(rx1,ry1,rz0);
		b = lerp( u, v, t );

		c = lerp( a, b, sy );

		q = g3[ b00 + bz1 ] ; u = at3(rx0,ry0,rz1);
		q = g3[ b10 + bz1 ] ; v = at3(rx1,ry0,rz1);
		a = lerp( u, v, t );

		q = g3[ b01 + bz1 ] ; u = at3(rx0,ry1,rz1);
		q = g3[ b11 + bz1 ] ; v = at3(rx1,ry1,rz1);
		b = lerp( u, v, t );

		d = lerp( a, b, sy );

		return lerp( c, d, sz );
	}
}
