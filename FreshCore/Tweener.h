/*
 *  Tweener.h
 *	See http://www.robertpenner.com/easing/, http://easings.net/, http://www.gizma.com/easing/
 *  Fresh
 *
 *  Created by Jeff Wofford on 7/22/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_TWEENER_H_INCLUDED
#define FRESH_TWEENER_H_INCLUDED

#include "FreshMath.h"
#include "Property.h"

#ifdef _MSC_VER
#	pragma warning( push )
#	pragma warning( disable: 4244 )
#endif

namespace fr
{
	
	// BASE
	//
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class Tweener
	{
	public:
	
		Tweener()
		:	m_timeStart( 0 )
		,	m_timeEnd( 1.0 )
		{}
	
		virtual ~Tweener() {}
	
		SYNTHESIZE( TTime, timeStart )			// 0.0 by default.
		SYNTHESIZE( TTime, timeEnd )			// 1.0 by default.
		
		virtual T operator()( TRef start, TRef end, TTime time ) const = 0;
	
		ALWAYS_INLINE TTime normalizedTime( TTime time ) const
		{
			return ( time - m_timeStart ) / ( m_timeEnd - m_timeStart );
		}

	private:
		
		TTime m_timeStart;
		TTime m_timeEnd;

	};

	// LINEAR
	//
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerLinear : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerLinear() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			return start + ( end - start ) * this->normalizedTime( time );
		}
	};
	
	
	// SIN
	//
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerSinEaseIn : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerSinEaseIn() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			TTime t = this->normalizedTime( time );
			return ( start - end ) * cos( t * HALF_PI ) + ( end - start ) + start;
		}
		
	};
	
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerSinEaseOut : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerSinEaseOut() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			TTime t = this->normalizedTime( time );
			return ( end - start ) * sin( t * HALF_PI ) + start;
		}
		
	};
	
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerSinEaseInOut : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerSinEaseInOut() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			TTime t = this->normalizedTime( time );
			return ( start - end ) * 0.5 * ( cos( PI * t ) - 1 ) + start;
		}
		
	};
	
	// QUAD
	//
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerQuadEaseIn : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerQuadEaseIn() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			TTime t = this->normalizedTime( time );
			return start + ( end - start ) * t * t;
		}

	};

	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerQuadEaseOut : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerQuadEaseOut() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			TTime t = this->normalizedTime( time );
			return start + ( start - end ) * ( t * ( t - 2 ));
		}
	};

	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerQuadEaseInOut : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerQuadEaseInOut() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			TTime t = this->normalizedTime( time ) * 2;
			if( t < 1 ) return ( end - start ) * ( 0.5 * t * t ) + start ;
			else
			{
				return ( start - end ) * ( 0.5 * (( t - 1 ) * ( t - 3 ) - 1 )) + start;
			}
		}
	};

	// CUBIC
	//
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerCubicEaseIn : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerCubicEaseIn() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			TTime t = this->normalizedTime( time );
			return ( end - start ) * ( t * t * t ) + start;
		}

	};

	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerCubicEaseOut : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerCubicEaseOut() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			TTime t = this->normalizedTime( time ) - 1;
			return ( end - start ) * ( t * t * t + 1 ) + start;
		}

	};

	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerCubicEaseInOut : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerCubicEaseInOut() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			TTime t = this->normalizedTime( time ) * 2;
			if( t < 1 ) return ( end - start ) * ( 0.5 * t * t * t ) + start;
			else
			{
				t -= 2;
				return ( end - start ) * 0.5 * ( t * t * t + 2 ) + start;
			}
		}
	};

	// QUART
	//
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerQuartEaseIn : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerQuartEaseIn() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			TTime t = this->normalizedTime( time );
			return ( end - start ) * t * t * t * t + start;
		}
		
	};
	
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerQuartEaseOut : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerQuartEaseOut() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			TTime t = this->normalizedTime( time ) - 1;
			return -( end - start ) * ( t * t * t * t - 1 ) + start;
		}
		
	};
	
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerQuartEaseInOut : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerQuartEaseInOut() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			TTime t = this->normalizedTime( time ) * 2;
			
			if( t < 1 )
			{
				return ( end - start ) / 2 * t * t * t * t + start;
			}
			else
			{
				t -= 2;
				return -( end - start ) / 2 * ( t * t * t * t - 2 ) + start;
			}
		}
	};
	
	// QUINT
	//
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerQuintEaseIn : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerQuintEaseIn() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			TTime t = this->normalizedTime( time );
			return ( end - start ) * t * t * t * t * t + start;
		}
		
	};
	
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerQuintEaseOut : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerQuintEaseOut() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			TTime t = this->normalizedTime( time ) - 1;
			return -( end - start ) * ( t * t * t * t * t + 1 ) + start;
		}
	};
	
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerQuintEaseInOut : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerQuintEaseInOut() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			TTime t = this->normalizedTime( time ) * 2;
			
			if( t < 1 )
			{
				return ( end - start ) / 2 * t * t * t * t * t + start;
			}
			else
			{
				t -= 2;
				return -( end - start ) / 2 * ( t * t * t * t * t + 2 ) + start;
			}
			
		}
	};
	
	// EXPO
	//
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerExpoEaseIn : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerExpoEaseIn() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			TTime t = this->normalizedTime( time );
			if( t == 0 )
			{
				return start;
			}
			else
			{
				return ( end - start ) * std::pow( 2, 10 * ( t - 1 )) + start;
			}
		}
		
	};
	
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerExpoEaseOut : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerExpoEaseOut() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			TTime t = this->normalizedTime( time );
			if( t < 1 )
			{
				return ( end - start ) * ( -std::pow( 2, -10 * t ) + 1 ) + start;
			}
			else
			{
				return end;
			}
		}
	};
	
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerExpoEaseInOut : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerExpoEaseInOut() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			TTime t = this->normalizedTime( time );
			
			if( t == 0 )
			{
				return start;
			}
			else if( t < 1 )
			{
				t *= 2;
				if( t < 1 )
				{
					return ( end - start ) * 0.5 * std::pow( 2, 10 * ( t - 1 )) + start;
				}
				else
				{
					return ( end - start ) * 0.5 * ( -std::pow( 2, -10 * ( t - 1 )) + 2 ) + start;
				}
			}
			else
			{
				return end;
			}
		}
	};
	
	// CIRC
	//
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerCircEaseIn : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerCircEaseIn() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			TTime t = this->normalizedTime( time );
			const auto c = end - start;
			
			return -c * (std::sqrt( 1 - t * t ) - 1 ) + start;
		}
		
	};
	
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerCircEaseOut : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerCircEaseOut() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			TTime t = this->normalizedTime( time ) - 1;
			const auto c = end - start;
			return c * std::sqrt( 1 - ( t * t )) + start;
		}
	};
	
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerCircEaseInOut : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerCircEaseInOut() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			TTime t = this->normalizedTime( time ) * 2;
			const auto c = end - start;
			
			if( t < 1 )
			{
				return -c * 0.5 * ( std::sqrt( 1 - t * t ) - 1 ) + start;
			}
			else
			{
				return  c * 0.5 * ( std::sqrt( 1 - t * ( t - 2 )) + 1 ) + start;
			}
		}
	};
	
	// BACK
	//
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerBackEaseIn : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerBackEaseIn() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			const TTime t = time - this->timeStart();
			const auto c = end - start;
			const TTime s = 1.70158;
			const TTime postFix = this->normalizedTime( time );
			return c * postFix * t * (( s + 1 ) * t - s ) + start;
		}
		
	};
	
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerBackEaseOut : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerBackEaseOut() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			TTime t = this->normalizedTime( time ) - 1;
			const auto c = end - start;
			const TTime s = 1.70158;
			return c * ( t * t * (( s + 1 ) * t + s ) + 1 ) + start;
		}
	};
	
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerBackEaseInOut : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerBackEaseInOut() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			TTime t = this->normalizedTime( time ) * 2;
			const auto c = end - start;
			TTime s = 1.70158 * 1.525;
			if( t < 1 )
			{
				return c * 0.5 * ( t * t * (( s + 1 ) * t - s )) + start;
			}
			else
			{
				t -= 2;
				return c * 0.5 * ( t * t * (( s + 1 ) * t + s ) + 2 ) + start;
			}
		}
	};
	
	// ELASTIC
	//
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerElasticEaseIn : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerElasticEaseIn() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			auto b = start;
			auto c = end - start;
			auto d = this->timeEnd() - this->timeStart();
			auto t = time - this->timeStart();
			
			if (t==0) { return b; }
			if ((t/=d)==1) { return b+c; }
			
			TTime p=d*.3f;
			T a=c;
			TTime s=p/4;
			T postFix = a * std::pow(2,10*(t-=1)); // this is a fix, again, with post-increment operators
			return -(postFix * std::sin((t*d-s)*(2 * fr::PI )/p )) + b;
		}
		
	};
	
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerElasticEaseOut : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerElasticEaseOut() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			auto b = start;
			auto c = end - start;
			auto d = this->timeEnd() - this->timeStart();
			auto t = time - this->timeStart();
			
			if (t==0) { return b; }
			if ((t/=d)==1) { return b+c; }
			
			TTime p=d*.3f;
			T a=c;
			TTime s=p/4;
			return (a*std::pow(2,-10*t) * std::sin( (t*d-s)*(fr::TWO_PI)/p ) + c + b);
		}
	};
	
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerElasticEaseInOut : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerElasticEaseInOut() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			auto b = start;
			auto c = end - start;
			auto d = this->timeEnd() - this->timeStart();
			auto t = time - this->timeStart();
			
			if (t==0) { return b; }
			if ((t/=d/2)==2) { return b+c; }
			
			TTime p=d*(0.3f*1.5f);
			T a=c;
			TTime s=p/4;
			
			if (t < 1) {
				T postFix =a*std::pow(2,10*(t-=1)); // postIncrement is evil
				return (postFix * std::sin( (t*d-s)*(fr::TWO_PI)/p )) * -0.5f + b;
			}
			T postFix =  a*pow(2,-10*(t-=1)); // postIncrement is evil
			return postFix * std::sin( (t*d-s)*(fr::TWO_PI)/p )*.5f + c + b;
		}
	};
	
	
	// BOUNCE
	//
	template< typename T, typename TTime >
	inline T bounceEaseOut( TTime t, T b, T c, TTime d )
	{
		if ((t/=d) < (1/2.75)) {
			return c*(7.5625*t*t) + b;
		} else if (t < (2/2.75)) {
			auto postFix = t-=(1.5/2.75);
			return c*(7.5625*(postFix)*t + 0.75) + b;
		} else if (t < (2.5/2.75)) {
			auto postFix = t-=(2.25/2.75);
			return c*(7.5625*(postFix)*t + 0.9375) + b;
		} else {
			auto postFix = t-=(2.625/2.75);
			return c*(7.5625*(postFix)*t + 0.984375) + b;
		}
	}
	
	template< typename T, typename TTime >
	inline T bounceEaseIn( TTime t, T b, T c, TTime d )
	{
		return c - bounceEaseOut( d-t, T{}, c, d ) + b;
	}
	
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerBounceEaseIn : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerBounceEaseIn() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			const auto b = start;
			const auto c = end - start;
			const auto d = this->timeEnd() - this->timeStart();
			const auto t = time - this->timeStart();
		
			return bounceEaseIn( t, b, c, d );
		}
		
	};
	
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerBounceEaseOut : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerBounceEaseOut() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			const auto b = start;
			const auto c = end - start;
			const auto d = this->timeEnd() - this->timeStart();
			const auto t = time - this->timeStart();

			return bounceEaseOut( t, b, c, d );
		}
	};
	
	template< typename T, typename TRef = const T&, typename TTime = TimeType >
	class TweenerBounceEaseInOut : public Tweener< T, TRef, TTime >
	{
	public:
		TweenerBounceEaseInOut() {}
		
		virtual T operator()( TRef start, TRef end, TTime time ) const
		{
			auto b = start;
			auto c = end - start;
			auto d = this->timeEnd() - this->timeStart();
			auto t = time - this->timeStart();
			
			if (t < d/2) return bounceEaseIn (t*2, T{}, c, d) * .5f + b;
			else return bounceEaseOut (t*2-d, T{}, c, d) * .5f + c*.5f + b;
		}
	};
}

#ifdef _MSC_VER
#	pragma warning( pop )
#endif

#endif
