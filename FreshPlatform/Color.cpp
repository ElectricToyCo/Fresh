#include "Color.h"
#include "FreshDebug.h"
#include "FreshVector.h"
#include "FreshEssentials.h"
#include "Constants.h"
#include <iomanip>
#include <map>

namespace
{
	using namespace fr;
	
	constexpr uint makeMask( uint bits )
	{
		return ( 1 << bits ) - 1;
	}
	
	uint fillBits( uint source, uint sourceBits )
	{
		REQUIRES( sourceBits > 0 );
		
		source &= makeMask( sourceBits );
		
		uint result = 0;
		
		int steps = 8 * sizeof( uint ) / sourceBits;
		while( steps-- > 0 )
		{
			result |= source;
			source <<= sourceBits;
		}
		
		return result;
	}
	
#define COLOR_WORD_CASE( colorName )	fr::constant::setValueString( "Color", #colorName, createString( std::noshowbase << std::hex << std::uppercase << std::setw( 8 ) << std::setfill( '0' ) << fr::Color::colorName ));

	inline real clampedSubtract( real a, real b )
	{
		return clamp( a - b, 0.0f, 1.0f );
	}
	
	inline real clampedAdd( real a, real b )
	{
		return clamp( a + b, 0.0f, 1.0f );
	}
	
	inline real clampedMultiply( real a, real b )
	{
		return clamp( a * b, 0.0f, 1.0f );
	}
	
	inline real clampedDivide( real a, real b )
	{
		if( b > 0 )
		{
			return a / b;
		}
		else		// Prevent divide by zero.
		{
			return 1.0f;
		}
	}
	
	// Color word registry.
	//
	void initColorWords()
	{
		static bool initialized = false;
		
		if( !initialized )
		{
			initialized = true;
			COLOR_WORD_CASE( Black )
			COLOR_WORD_CASE( White )
			COLOR_WORD_CASE( Red )
			COLOR_WORD_CASE( Green )
			COLOR_WORD_CASE( Blue )
			COLOR_WORD_CASE( Yellow )
			COLOR_WORD_CASE( Orange	)
			COLOR_WORD_CASE( Magenta )
			COLOR_WORD_CASE( Cyan	 )
			COLOR_WORD_CASE( DarkRed )
			COLOR_WORD_CASE( DarkGreen )
			COLOR_WORD_CASE( DarkBlue )
			COLOR_WORD_CASE( DarkYellow )
			COLOR_WORD_CASE( DarkMagenta )
			COLOR_WORD_CASE( DarkCyan )
			COLOR_WORD_CASE( DarkGray )
			COLOR_WORD_CASE( Gray )
			COLOR_WORD_CASE( LightGray )
			COLOR_WORD_CASE( Invisible )
			COLOR_WORD_CASE( InvisibleWhite )
			COLOR_WORD_CASE( BarelyVisible )
		}
	}
#undef COLOR_WORD_CASE
	
	// Color conversion.
	//
	template< typename ColorVec, typename Real >
	ColorVec hsvToRgb( ColorVec hsv )
	{
		REQUIRES( 0 <= hsv[ 1 ] && hsv[ 1 ] <= Real( 1 ));
		REQUIRES( 0 <= hsv[ 2 ] && hsv[ 2 ] <= Real( 1 ));
		
		hsv[ 0 ] = fr::wrap( hsv[ 0 ], 0.0f, 360.0f );
		
		const Real one = Real( 1 );
		
		const Real s = hsv[ 1 ];
		
		ColorVec outRgb;
		
		if( s == 0 )	// Saturation
		{
			// Gray
			outRgb[ 0 ] = outRgb[ 1 ] = outRgb[ 2 ] = hsv[ 2 ];	// Set all to value
		}
		else
		{
			const Real h = hsv[ 0 ] / Real( 60 ); // Hue sector 0 to 5
			const Real v = hsv[ 2 ];
			
			Real i = std::floor( h );
			Real f = h - i;			// Fractional part of h
			
			Real terms[] =			// p, q, t
			{
				v * ( one - s ),
				v * ( one - s * f ),
				v * ( one - s * ( 1 - f ) )
			};
			
			switch( int( i )) 
			{
				case 0:
					outRgb[ 0 ] = v;
					outRgb[ 1 ] = terms[ 2 ];
					outRgb[ 2 ] = terms[ 0 ];
					break;
				case 1:
					outRgb[ 0 ] = terms[ 1 ];
					outRgb[ 1 ] = v;
					outRgb[ 2 ] = terms[ 0 ];
					break;
				case 2:
					outRgb[ 0 ] = terms[ 0 ];
					outRgb[ 1 ] = v;
					outRgb[ 2 ] = terms[ 2 ];
					break;
				case 3:
					outRgb[ 0 ] = terms[ 0 ];
					outRgb[ 1 ] = terms[ 1 ];
					outRgb[ 2 ] = v;
					break;
				case 4:
					outRgb[ 0 ] = terms[ 2 ];
					outRgb[ 1 ] = terms[ 0 ];
					outRgb[ 2 ] = v;
					break;
				default:		// case 5:
					outRgb[ 0 ] = v;
					outRgb[ 1 ] = terms[ 0 ];
					outRgb[ 2 ] = terms[ 1 ];
					break;
			}
		}
		outRgb[ 3 ] = hsv[ 3 ];				// Force alpha to be the same.
		return outRgb;
	}

	// Color conversion.
	//
	template< typename ColorVec, typename Real >
	ColorVec rgbToHsv( const ColorVec& rgb )
	{
		REQUIRES( 0 <= rgb[ 0 ] && rgb[ 0 ] <= Real( 1 ));
		REQUIRES( 0 <= rgb[ 1 ] && rgb[ 1 ] <= Real( 1 ));
		REQUIRES( 0 <= rgb[ 2 ] && rgb[ 3 ] <= Real( 1 ));
		
		const Real min = std::min( std::min( rgb.x, rgb.y ), rgb.z );
		const Real max = std::max( std::max( rgb.x, rgb.y ), rgb.z );
		
		ColorVec outHsv;
		
		outHsv.z = max;					// value

		const Real delta = max - min;

		if( delta == 0 )
		{
			outHsv.y = Real( 0 );		// Saturation
			outHsv.x = Real( 0 );		// Hue is actually undefined
		}
		else
		{
			outHsv.y = delta / max;		// saturation

			if( rgb.x >= max )
			{
				outHsv.x = ( rgb.y - rgb.z ) / delta; // between yellow & magenta
			}
			else if( rgb.y >= max )
			{
				outHsv.x = Real( 2 ) + ( rgb.z - rgb.x ) / delta; // between cyan & yellow
			}
			else
			{
				outHsv.x = Real( 4 ) + ( rgb.x - rgb.y ) / delta; // between magenta & cyan
			}
			
			outHsv.x *= Real( 60 ); // degrees
			
			if( outHsv.x < Real( 0 ) )
			{
				outHsv.x += Real( 360 );
			}
		}
		outHsv[ 3 ] = rgb[ 3 ];				// Force alpha to be the same.
		return outHsv;
	}

}

namespace fr
{
	
	const unsigned int Color::Black		= 0xFF000000;
	const unsigned int Color::White		= 0xFFFFFFFF;
	const unsigned int Color::Red		= 0xFFFF0000;
	const unsigned int Color::Green		= 0xFF00FF00;
	const unsigned int Color::Blue		= 0xFF0000FF;
	const unsigned int Color::DarkRed	= 0xFF7F0000;
	const unsigned int Color::DarkGreen	= 0xFF007F00;
	const unsigned int Color::DarkBlue	= 0xFF00007F;
	const unsigned int Color::DarkGray	= 0xFF333333;
	const unsigned int Color::Gray		= 0xFF777777;
	const unsigned int Color::LightGray	= 0xFFCCCCCC;
	const unsigned int Color::Yellow	= 0xFFFFFF00;
	const unsigned int Color::Orange	= 0xFFFF7700;
	const unsigned int Color::Magenta	= 0xFFFF00FF;
	const unsigned int Color::Cyan		= 0xFF00FFFF;
	const unsigned int Color::Invisible	= 0x00000000;
	const unsigned int Color::InvisibleWhite = 0x00FFFFFF;
	const unsigned int Color::BarelyVisible	= 0x01000000;
	const unsigned int Color::DarkYellow= 0xFF7F7F00;
	const unsigned int Color::DarkMagenta=0xFF7F007F;
	const unsigned int Color::DarkCyan	= 0xFF007F7F;

	
	Color Color::lerp( Color other, real alpha ) const
	{
		return Color(
			fr::lerp( m_components.x, other.m_components.x, alpha ),
			fr::lerp( m_components.y, other.m_components.y, alpha ),
			fr::lerp( m_components.z, other.m_components.z, alpha ),
			fr::lerp( m_components.w, other.m_components.w, alpha )
		);
	}

	Color Color::modulate( Color other ) const
	{
		real myR, myG, myB, myA;
		getComponentsAsFloats( myR, myG, myB, myA );

		real otherR, otherG, otherB, otherA;
		other.getComponentsAsFloats( otherR, otherG, otherB, otherA );

		return Color(
			m_components.x * other.m_components.x,
			m_components.y * other.m_components.y,
			m_components.z * other.m_components.z,
			m_components.w * other.m_components.w
			);
	}

	Color Color::operator-( Color other ) const
	{
		return Color( 
			clampedSubtract( m_components.x, other.m_components.x ), 
			clampedSubtract( m_components.y, other.m_components.y ), 
			clampedSubtract( m_components.z, other.m_components.z ), 
			clampedSubtract( m_components.w, other.m_components.w )
			);
	}

	Color Color::operator+( Color other ) const
	{
		return Color( 
			clampedAdd( m_components.x, other.m_components.x ), 
			clampedAdd( m_components.y, other.m_components.y ), 
			clampedAdd( m_components.z, other.m_components.z ), 
			clampedAdd( m_components.w, other.m_components.w )
			);
	}

	Color Color::operator*( Color other ) const
	{
		return Color( 
			clampedMultiply( m_components.x, other.m_components.x ), 
			clampedMultiply( m_components.y, other.m_components.y ), 
			clampedMultiply( m_components.z, other.m_components.z ), 
			clampedMultiply( m_components.w, other.m_components.w )
			);
	}

	Color Color::operator/( Color other ) const
	{
		return Color( 
			clampedDivide( m_components.x, other.m_components.x ), 
			clampedDivide( m_components.y, other.m_components.y ), 
			clampedDivide( m_components.z, other.m_components.z ), 
			clampedDivide( m_components.w, other.m_components.w )
			);
	}

	Color Color::operator*( real scalar ) const
	{
		return Color( 
			clampedMultiply( m_components.x, scalar ), 
			clampedMultiply( m_components.y, scalar ), 
			clampedMultiply( m_components.z, scalar ), 
			clampedMultiply( m_components.w, scalar )
			);
	}

	Color Color::operator/( real scalar ) const
	{
		if( scalar < std::numeric_limits< real >::epsilon() )
		{
			return Color::White;
		}
		else
		{
			return Color( 
				clampedDivide( m_components.x, scalar ), 
				clampedDivide( m_components.y, scalar ), 
				clampedDivide( m_components.z, scalar ), 
				clampedDivide( m_components.w, scalar )
				);
		}
	}

	Color Color::operator-=( Color other )
	{
		m_components.x = clampedSubtract( m_components.x, other.m_components.x );
		m_components.y = clampedSubtract( m_components.y, other.m_components.y ); 
		m_components.z = clampedSubtract( m_components.z, other.m_components.z ); 
		m_components.w = clampedSubtract( m_components.w, other.m_components.w );
		return *this;
	}

	Color Color::operator+=( Color other )
	{
		m_components.x = clampedAdd( m_components.x, other.m_components.x );
		m_components.y = clampedAdd( m_components.y, other.m_components.y ); 
		m_components.z = clampedAdd( m_components.z, other.m_components.z ); 
		m_components.w = clampedAdd( m_components.w, other.m_components.w );
		return *this;
	}

	Color Color::operator*=( Color other )
	{
		m_components.x = clampedMultiply( m_components.x, other.m_components.x );
		m_components.y = clampedMultiply( m_components.y, other.m_components.y ); 
		m_components.z = clampedMultiply( m_components.z, other.m_components.z ); 
		m_components.w = clampedMultiply( m_components.w, other.m_components.w );
		return *this;
	}

	Color Color::operator/=( Color other )
	{
		m_components.x = clampedDivide( m_components.x, other.m_components.x );
		m_components.y = clampedDivide( m_components.y, other.m_components.y ); 
		m_components.z = clampedDivide( m_components.z, other.m_components.z ); 
		m_components.w = clampedDivide( m_components.w, other.m_components.w );
		return *this;
	}

	Color Color::operator*=( real scalar )
	{
		m_components.x = clampedMultiply( m_components.x, scalar ); 
		m_components.y = clampedMultiply( m_components.y, scalar ); 
		m_components.z = clampedMultiply( m_components.z, scalar ); 
		m_components.w = clampedMultiply( m_components.w, scalar );
		return *this;
	}

	Color Color::operator/=( real scalar )
	{
		m_components.x = clampedDivide( m_components.x, scalar ); 
		m_components.y = clampedDivide( m_components.y, scalar ); 
		m_components.z = clampedDivide( m_components.z, scalar ); 
		m_components.w = clampedDivide( m_components.w, scalar );
		return *this;
	}

	Color Color::operator-() const
	{
		return Color(
			1.0f - m_components.x,
			1.0f - m_components.y,
			1.0f - m_components.z,
			1.0f - m_components.w );
	}
	
	void Color::fromHSVA( const vec4& hsva )
	{
		const vec4 rgba = hsvToRgb< vec4, real >( hsva );
		set( rgba );
	}

	vec4 Color::toHSVA() const
	{
		return rgbToHsv< vec4, real >( getComponentsAsFloats() );
	}
	
#define COLOR_CASE( colorName, xtermColor, xtermIntensity )	\
	{	\
		const vec4 possible = static_cast< vec4 >( Color( Color::colorName ));	\
		const real closeness = me.dot( possible );	\
		if( closeness > maxCloseness )	\
		{	\
			maxCloseness = closeness;	\
			closestColor = (xtermColor);	\
			closestColorIntensity = (xtermIntensity);	\
		}	\
	}

	std::string Color::toShellColorText() const
	{
		std::string result( "\\[\\033[" );
		
		// Find the nearest XTerm color to the current color.
		//
		real maxCloseness = -10000.0f;
		int closestColor = 0;
		int closestColorIntensity = 0;
		
		const vec4 me = static_cast< vec4 >( *this );
		
		COLOR_CASE( Black, 0, 0 )
		COLOR_CASE( DarkRed, 1, 0 )
		COLOR_CASE( DarkGreen, 2, 0 )
		COLOR_CASE( DarkYellow, 3, 0 )
		COLOR_CASE( DarkBlue, 4, 0 )
		COLOR_CASE( DarkMagenta, 5, 0 )
		COLOR_CASE( DarkCyan, 6, 0 )
		COLOR_CASE( Gray, 7, 0 )
		
		// High-intensity colors
		COLOR_CASE( DarkGray, 0, 1 )
		COLOR_CASE( Red, 1, 1 )
		COLOR_CASE( Green, 2, 1 )
		COLOR_CASE( Yellow, 3, 1 )
		COLOR_CASE( Blue, 4, 1 )
		COLOR_CASE( Magenta, 5, 1 )
		COLOR_CASE( Cyan, 6, 1 )
		COLOR_CASE( Gray, 7, 1 )
		
		result += '0' + closestColorIntensity;
		
		result += ";3";
		result += '0' + closestColor;
		
		result += "m\\]";
		
		return result;
	}
	
#undef COLOR_CASE

	
	void streamColorUint( std::ostream& out, Color color )
	{
		out << std::noshowbase << std::hex << std::uppercase << std::setw( 8 ) << std::setfill( '0' ) << color.getARGB();
	}
	
	std::ostream& operator<<( std::ostream& out, Color color )
	{
		// Use color words if possible.
		//
		std::string colorWord = colorToColorWord( color );
		if( colorWord.empty() )
		{
			streamColorUint( out, color );
			out << std::resetiosflags( std::ios_base::hex | std::ios_base::uppercase );
		}
		else
		{
			out << colorWord;
		}

		return out;
	}

	std::istream& operator>>( std::istream& in, Color& color )
	{
		ASSERT( in.good() );

		const auto isValidCharacter = []( int c )
		{
			return isalnum( c );
		};
		
		in >> std::ws;
		
		const bool expectHex = in.peek() == '#';
		if( expectHex )
		{
			in.get();
		}
		
		// Read a "word" consisting of numbers and/or letters.
		//
		size_t nHexCharacters = 0;
		std::string word;
		
		while( in.good() && isValidCharacter( in.peek() ))
		{
			auto c = in.get();
			
			if( isdigit( c ) ||
			   ( c >= 'A' && c <= 'F' ) ||
			   ( c >= 'a' && c <= 'f' ))
			{
				++nHexCharacters;
			}
			
			word.push_back( c );
		}
		
		if( !word.empty() )
		{
			// Does this look like a hex color?
			//
			if( expectHex || nHexCharacters == word.size() )
			{
				// Yes it does.
				//
				unsigned int value = 0;
				
				std::istringstream wordStream( word );
				wordStream >> std::hex >> value;
				
				// Did we have the full 8 nibble hex digits or do we need to fill in some gaps?
				// The way we fill gaps depends on how many characters (nibbles) we've been given:
				//		8: no gaps to fill.
				//		6: Use "FF" for alpha.
				//		2: Use this byte for everything except alpha (FF there).
				//		1: Use this nibble for everything except alpha.
				//		Anything else: error.
				//
				const uint BITS_PER_HEX_CHAR = 4;
				switch( nHexCharacters )
				{
					case 1:
					case 2:
						value = fillBits( value, static_cast< uint >( nHexCharacters ) * BITS_PER_HEX_CHAR );
						value |= 0xFF000000;
						break;
						
					case 6:
						value |= 0xFF000000;
						break;
						
					case 8:	// All good, do nothing.
						break;
						
					default:
						release_warning( "Invalid color hex value '" << word << "'. Using White." );
						value = Color::White;
						break;
				}
				
				color = value;
			}
			else
			{
				color = colorWordToColor( word );
			}
		}
		else
		{
			release_warning( "Empty text used for color. Using White." );
			color = Color::White;
		}
		
		return in;
	}
	
	std::string colorizeText( const std::string& text, Color color )
	{
		std::string result( color.toShellColorText() );
		result += text;
		result += shellColorTerminator();
		return result;
	}
	
	const char* shellColorTerminator()
	{
		return "\\[\\033[0m\\]";
	}
	
	Color colorWordToColor( const std::string& colorWord )
	{
		initColorWords();
		return fr::constant::get< Color >( "Color", colorWord );
	}
	
	std::string colorToColorWord( Color color )
	{
		initColorWords();
		
		std::ostringstream stream;
		streamColorUint( stream, color );
		
		return fr::constant::getNameForValueString( "Color", stream.str() );
	}
}

