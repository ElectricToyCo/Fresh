#ifndef COLOR_H_INCLUDED_
#define COLOR_H_INCLUDED_

#include "FreshVector.h"

namespace fr
{

	
	class Color
	{
	public:
	
		// Throughout, we talk about colors as if in big endian mode (like OpenGL does)
		// But this class is actually implemented with little endian ordering, internally.
		
		static const unsigned int Black		;
		static const unsigned int White		;
		static const unsigned int Red		;
		static const unsigned int Green		;
		static const unsigned int Blue		;
		static const unsigned int Yellow	;
		static const unsigned int Orange	;
		static const unsigned int Magenta	;
		static const unsigned int Cyan		;
		static const unsigned int DarkRed	;
		static const unsigned int DarkGreen	;
		static const unsigned int DarkBlue	;
		static const unsigned int DarkYellow;
		static const unsigned int DarkMagenta;
		static const unsigned int DarkCyan	;
		static const unsigned int DarkGray	;
		static const unsigned int Gray		;
		static const unsigned int LightGray	;
		static const unsigned int Invisible	;
		static const unsigned int InvisibleWhite;
		static const unsigned int BarelyVisible	;

		Color( unsigned int argb = White )
		{
			set( argb );
		}

		Color( unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255 )
		{
			set( r, g, b, a );
		}
		
		Color( real r, real g, real b, real a = 1.0f )
		{
			set( r, g, b, a );
		}
		
		explicit Color( const vec4& floats )
		{
			set( floats );
		}
		
		void set( real r, real g, real b, real a = 1.0f )
		{
			set( vec4{ r, g, b, a });
		}
		
		void set( unsigned int argb )
		{
			set(
				static_cast< unsigned char >(( argb & 0x00FF0000 ) >> 16 ),
				static_cast< unsigned char >(( argb & 0x0000FF00 ) >> 8 ),
				static_cast< unsigned char >(( argb & 0x000000FF )),
				static_cast< unsigned char >(( argb & 0xFF000000 ) >> 24 )
				);
		}
		
		void set( unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255 )
		{
			set( r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f );
		}

		void set( const vec4& floatsRGBA )
		{
#ifdef DEBUG
			for( size_t i = 0; i < 4; ++i )
			{
				REQUIRES( 0.0f <= floatsRGBA[i] && floatsRGBA[i] <= 1.0f );
			}
#endif

			m_components = floatsRGBA;
		}
		
		operator vec4() const
		{
			return getComponentsAsFloats();
		}
		
		unsigned int getARGB() const
		{
			unsigned int a, b, g, r;
			
			a = getA();
			b = getB();
			g = getG();
			r = getR();
			
			return ( a << 24 ) | ( r << 16 ) | ( g << 8 ) | b;
		}
		
		unsigned int getABGR() const
		{
			unsigned int a, b, g, r;
			
			a = getA();
			b = getB();
			g = getG();
			r = getR();
			
			return ( a << 24 ) | ( b << 16 ) | ( g << 8 ) | r;
		}

		unsigned int getBGRA() const
		{
			unsigned int a, b, g, r;
			
			a = getA();
			b = getB();
			g = getG();
			r = getR();
			
			return ( b << 24 ) | ( g << 16 ) | ( r << 8 ) | a;
		}
		
		unsigned int getRGBA() const
		{
			unsigned int a, b, g, r;
			
			a = getA();
			b = getB();
			g = getG();
			r = getR();
			
			return ( r << 24 ) | ( g << 16 ) | ( b << 8 ) | a;
		}
		
		unsigned int getXRGB() const
		{
			unsigned int b, g, r;
			
			b = getB();
			g = getG();
			r = getR();
			
			return ( 0xFF << 24 ) | ( r << 16 ) | ( g << 8 ) | b;
		}
		
		unsigned int getXBGR() const
		{
			unsigned int b, g, r;
			
			b = getB();
			g = getG();
			r = getR();
			
			return ( 0xFF << 24 ) | ( b << 16 ) | ( g << 8 ) | r;
		}
		
		unsigned int getRGBX() const
		{
			unsigned int b, g, r;
			
			b = getB();
			g = getG();
			r = getR();
			
			return ( r << 24 ) | ( g << 16 ) | ( b << 8 ) | 0xFF;
		}
		
		unsigned int getBGRX() const
		{
			unsigned int b, g, r;
			
			b = getB();
			g = getG();
			r = getR();
			
			return ( b << 24 ) | ( g << 16 ) | ( r << 8 ) | 0xFF;
		}
		
		unsigned int getA() const
		{
			return static_cast< unsigned int >( m_components.w * 255.0f );
		}
		unsigned int getR() const
		{
			return static_cast< unsigned int >( m_components.x * 255.0f );
		}
		unsigned int getG() const
		{
			return static_cast< unsigned int >( m_components.y * 255.0f );
		}
		unsigned int getB() const
		{
			return static_cast< unsigned int >( m_components.z * 255.0f );
		}
		
		void setA( unsigned int x ) { m_components.w = static_cast< real >( x ) / 255.0f; }
		void setR( unsigned int x ) { m_components.x = static_cast< real >( x ) / 255.0f; }
		void setG( unsigned int x ) { m_components.y = static_cast< real >( x ) / 255.0f; }
		void setB( unsigned int x ) { m_components.z = static_cast< real >( x ) / 255.0f; }

		void setA( float x ) { m_components.w = x; }
		void setR( float x ) { m_components.x = x; }
		void setG( float x ) { m_components.y = x; }
		void setB( float x ) { m_components.z = x; }

		real luminance() const
		{
			real r, g, b, a;
			getComponentsAsFloats( r, g, b, a );
			
			return ((0.2126f*r) + (0.7152f*g) + (0.0722f*b)) * a;
		}
		
		void getComponentsAsFloats( real& outR, real& outG, real& outB, real& outA ) const
		{
			outR = m_components.x;
			outG = m_components.y;
			outB = m_components.z;
			outA = m_components.w;
			
			PROMISES( 0 <= outR && outR <= (real)1.0 );
			PROMISES( 0 <= outG && outG <= (real)1.0 );
			PROMISES( 0 <= outB && outB <= (real)1.0 );
			PROMISES( 0 <= outA && outA <= (real)1.0 );
		}

		vec4 getComponentsAsFloats() const
		{
			return m_components;
		}
		
		void getComponentsAsBytes( unsigned char& outR, unsigned char& outG, unsigned char& outB, unsigned char& outA ) const
		{
			outR = getR();
			outG = getG();
			outB = getB();
			outA = getA();
		}

		Color lerp( Color other, real alpha ) const;
		Color modulate( Color other ) const;

		bool operator == ( unsigned int argb ) const
		{
			return getARGB() == argb;
		}
		
		bool operator != ( unsigned int argb ) const
		{
			return getARGB() != argb;
		}
		
		bool operator == ( Color other ) const
		{
			return m_components == other.m_components;
		}
		
		bool operator != ( Color other ) const
		{
			return m_components != other.m_components;
		}
		
		// Component-wise operations.
		Color operator-( Color other ) const;
		Color operator+( Color other ) const;
		Color operator*( Color other ) const;
		Color operator/( Color other ) const;
		Color operator*( real scalar ) const;
		Color operator/( real scalar ) const;

		Color operator-=( Color other );
		Color operator+=( Color other );
		Color operator*=( Color other );
		Color operator/=( Color other );
		Color operator*=( real scalar );
		Color operator/=( real scalar );
		
		Color operator-() const;


		void fromHSVA( const vec4& hsva );
		vec4 toHSVA() const;
		
		static Color makeHSVA( const vec4& hsva )
		{
			Color color;
			color.fromHSVA( hsva );
			return color;
		}
		
		std::string toShellColorText() const;
		
	private:

		vec4 m_components;

	};

	template<typename TAlpha>
	inline Color lerp( Color a, Color b, TAlpha alpha )
	{
		return a.lerp( b, static_cast< real >( alpha ));
	}

	template<typename ColorValueA, typename ColorValueB, typename TAlpha>
	inline Color colorLerp( ColorValueA a, ColorValueB b, TAlpha alpha )
	{
		return Color{ a }.lerp( Color{ b }, static_cast< real >( alpha ));
	}
	
	template<typename TAlpha>
	inline Color lerp( const Range< Color >& r, TAlpha alpha )
	{
		return r.min.lerp( r.max, static_cast< real >( alpha ));
	}

	inline real colorDistanceSquared( Color a, Color b )
	{
		vec4 av = a.getComponentsAsFloats();
		vec4 bv = b.getComponentsAsFloats();
		
		return distanceSquared( av, bv );
	}
	
	inline real colorDistance( Color a, Color b )
	{
		return std::sqrt( colorDistanceSquared( a, b ));
	}
	
	inline Color randInRange( Color a, Color b )
	{
		vec4 av = a.getComponentsAsFloats();
		vec4 bv = b.getComponentsAsFloats();
		
		return Color( randInRange( av, bv ));
	}
	
	std::ostream& operator<<( std::ostream& out, Color color );
	std::istream& operator>>( std::istream& in, Color& color );

	Color colorWordToColor( const std::string& colorWord );
	std::string colorToColorWord( Color color );
	
	std::string colorizeText( const std::string& text, Color color );
	const char* shellColorTerminator();
	
}		// END namespace fr

#endif
