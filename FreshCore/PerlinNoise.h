#pragma once

#include "FreshVector.h"
#include <random>

namespace fr
{
	class Noise
	{
	public:
		
		explicit Noise( uint seed = 1 );
		
		void seed( uint newSeed );
		
		real at( real pos );
		real at( const vec2& pos );
		real at( const vec3& pos );
		
	private:
		
		std::default_random_engine m_generator;
		std::uniform_int_distribution< int > m_distribution;
		std::function< int() > random;
		
		static const int B = 0x100;
		static const int BM = 0xff;
		
		static const int N = 0x1000;
		static const int NP = 12;   /* 2^N */
		static const int NM = 0xfff;
		
		int  p[ B + B + 2 ];
		real g3[ B + B + 2 ][3];
		real g2[ B + B + 2 ][2];
		real g1[ B + B + 2 ];
	};
}
