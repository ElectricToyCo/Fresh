//
//  TextMetrics.h
//  Fresh
//
//  Created by Jeff Wofford on 1/16/13.
//
//

#ifndef Fresh_TextMetrics_h
#define Fresh_TextMetrics_h

#include <algorithm>
#include <vector>
#include <cctype>			// For std::isspace
#include "FreshMath.h"

namespace fr
{
	class TextMetrics
	{
	public:
		
		// Types
		//
		struct Line
		{
			std::string text;
			rect bounds;
			
			float width() const { return bounds.width(); }
			float height() const { return bounds.height(); }
		};
		
		enum class Enforcement
		{
			None,
			Wrap,
			Ellipses,
			Truncate,
			Scale,
			ScaleDown,
			ScaleUp,
		};

		enum class Alignment
		{
			Left,
			Centered,
			Right
		};
		
		enum class VerticalAlignment
		{
			Top,
			Middle,
			Bottom
		};
		
		enum class LineFeeds
		{
			Unknown,
			Unix,			//	\n
			Mac9,			//	\r
			Windows,		//	\r\n
			Other,			//	\n\r
			Inconsistent
		};
		
		static const int NON_BREAKING_SPACE = 255;		// The ASCII nbsp character. Word wrapping ignores this--i.e. treats it as not space.

		typedef std::vector< Line > Lines;
		
		struct OutOfRange {};		// Exception type

		// Constructor
		//
		template< typename iter_t >
		explicit TextMetrics(iter_t stringBegin,
							 iter_t stringEnd,
							 float fontPixelSize,
							 float lineHeight,
							 const std::vector< float >& characterWidths,
							 float enforcedBoundX = 0,
							 float enforcedBoundY = 0,
							 Enforcement widthEnforcement = Enforcement::Wrap,
							 Alignment alignment = Alignment::Left,
							 VerticalAlignment verticalAlignment = VerticalAlignment::Top,
							 bool allowCStyleEscapes = true )
		:	m_fontPixelSize( fontPixelSize )
		,	m_lineHeight( lineHeight )
		,	m_characterWidths( characterWidths )
		,	m_allowCStyleEscapes( allowCStyleEscapes )
		{
			m_wasTextTruncatedX = m_wasTextTruncatedY = false;

			// Digest raw text into lines.
			//
			float lineTopY = 0;
			bool forcedNewline = false;
			while( stringBegin != stringEnd )
			{
				stringBegin = digestLine( stringBegin, stringEnd, enforcedBoundX, widthEnforcement, lineTopY, forcedNewline );
				
				lineTopY += m_lineHeight;
			}
			
			// We have a trailing newline. Add a blank line.
			//
			if( forcedNewline )
			{
				const real lineTop = lineTopY * m_fontPixelSize;
				m_lines.push_back( Line{ "", rect{ 0, lineTop, 0, lineTop + m_lineHeight * m_fontPixelSize }} );
			}
			
			// Trim lines to maintain the vertical enforced bounds.
			
			if( enforcedBoundY > 0 && m_lines.size() > enforcedBoundY )
			{
				m_wasTextTruncatedY = true;
				
				const size_t excessLines = m_lines.size() - static_cast< size_t >( enforcedBoundY );
				float adjustmentY = 0;
				
				switch( verticalAlignment )
				{
					default:
					case VerticalAlignment::Top:
						m_lines.erase( m_lines.end() - excessLines,  m_lines.end() );
						break;
						
					case VerticalAlignment::Middle:
					{
						m_lines.erase( m_lines.begin(), m_lines.begin() + excessLines / 2 );
						
						auto excessLinesOtherHalf = excessLines - excessLines / 2;
						m_lines.erase( m_lines.end() - excessLinesOtherHalf, m_lines.end() );
						
						// Adjust line positioning.
						adjustmentY = excessLines / 2 * -m_lineHeight * m_fontPixelSize;
						break;
					}
						
					case VerticalAlignment::Bottom:
					{
						m_lines.erase( m_lines.begin(), m_lines.begin() + excessLines );

						// Adjust line positioning.
						adjustmentY = excessLines * -m_lineHeight * m_fontPixelSize;
						break;
					}
				}
				
				if( adjustmentY != 0 )
				{
					std::for_each( m_lines.begin(), m_lines.end(), [adjustmentY]( Line& line )
								  {
									  line.bounds.translate( vec2( 0, adjustmentY ));
								  });
				}
				
			}
						
			// Calculate overall text dimensions and
			// adjust each line horizontally for alignment.
			//
			m_bounds.setToInverseInfinity();
			
			float offsetY;
			switch( verticalAlignment )
			{
				default:
				case VerticalAlignment::Top:
					offsetY = 0;
					break;
					
				case VerticalAlignment::Middle:
					offsetY = m_lineHeight * m_fontPixelSize * m_lines.size() * -0.5f;
					break;

				case VerticalAlignment::Bottom:
					offsetY = -m_lineHeight * m_fontPixelSize * m_lines.size();
					break;
			}
			
			std::for_each( m_lines.begin(), m_lines.end(), [&]( Line& line ) mutable
				{
					float offsetX = 0;
					switch( alignment )
					{
						case Alignment::Centered:
							offsetX = -0.5f * line.width();
							break;
							
						case Alignment::Right:
							offsetX = -line.width();
							break;
							
						case TextMetrics::Alignment::Left:
						default:
							break;
					};
					
					line.bounds.translate( vec2( offsetX, offsetY ));
					
					m_bounds.growToEncompass( line.bounds );
				}
			);			
		}
		
		SYNTHESIZE_GET( rect, bounds )
		
		// For each line
		//
		template< typename function_t >
		void forEachLine( function_t fnPerLine ) const
		{
			size_t iLine = 0;
			std::for_each( m_lines.begin(), m_lines.end(), [&]( const Line& line )
						  {
							  fnPerLine( line, iLine++ );
						  });
		}
		
		size_t numLines() const
		{
			return m_lines.size();
		}
		
		// Width and height
		//
		float width() const
		{
			return m_bounds.width();
		}
		float height() const
		{
			return m_bounds.height();
		}
		
		template< typename vec_t >
		vec_t characterPixelPosition( const size_t iCharacter ) const
		{
			size_t i = 0;
			
			vec_t pos( 0 );
			
			for( const auto& line : m_lines )
			{
				pos[ 0 ] = line.bounds[ 0 ];
				
				bool abort = false;
				for( auto c : line.text )
				{
					if( i++ == iCharacter )
					{
						// Adjust for this line's alignment.
						//
						abort = true;
						break;
					}
					
					pos[ 0 ] += getSimpleCharWidth( c );
				}
				
				if( abort )
				{
					break;
				}
				
				if( i >= iCharacter )
				{
					break;
				}
				
				++i;
				pos[ 1 ] += line.height();
			}
			
			if( iCharacter > i )
			{
				pos[ 0 ] = 0;
			}
			
			return pos;
		}
		
		template< typename vec_t >
		size_t characterIndexFromPixelPosition( vec_t pos ) const
		{
			// returns size_t( -1 ) if out of bounds.
			
			// Which line does this position fall in?
			//
			pos -= m_bounds.ulCorner();
			
			const size_t iLine = static_cast< size_t >( pos[ 1 ] / ( m_lineHeight * m_fontPixelSize ));
			
			if( iLine >= m_lines.size() )
			{
				return -1;
			}
			else
			{
				size_t iChar = 0;
				
				// Count the characters in preceding lines.
				//
				for( size_t i = 0; i + 1 <= iLine; ++i )
				{
					iChar += m_lines[ i ].text.size() + 1;
				}
				
				const size_t lineStartChar = iChar;
				
				const Line& line = m_lines[ iLine ];

				float charX = 0;
				
				if( pos[ 0 ] < charX )
				{
					// Fell off left end of line.
					return lineStartChar;
				}
				else
				{
					for( auto c : line.text )
					{
						if( pos[ 0 ] < charX  )
						{
							return iChar - 1;
						}
							
						++iChar;
						charX += getSimpleCharWidth( c );
					}
					
					// Fell off right end of line.
					return iChar;
				}
			}
		}
		
		size_t lineStepPos( size_t curPosition, int lineStep ) const
		{
			const auto p = characterPixelPosition< vec2 >( curPosition );
			return characterIndexFromPixelPosition( p + vec2( 0, lineStep * m_lineHeight * m_fontPixelSize ));
		}
		
	protected:
		
		float getSimpleCharWidth( int c ) const
		{
			float characterWidth = m_fontPixelSize;
			if( (size_t) c < m_characterWidths.size() )
			{
				characterWidth = m_characterWidths[ c ] * m_fontPixelSize;
			}
			return characterWidth;
		}
		
		template< typename iter_t >
		float getSimpleTextWidth( iter_t stringBegin, iter_t stringEnd ) const
		{
			float width = 0;
			for( ; stringBegin != stringEnd; ++stringBegin )
			{
				width += getSimpleCharWidth( *stringBegin );
			}
			return width;
		}

		template< typename iter_t >
		void eatUntilNewLine( iter_t& inOutStringBegin, iter_t stringEnd )
		{
			// Keep skipping characters until we reach the end of the line or the text.
			while( inOutStringBegin != stringEnd )
			{
				int c = getNextChar( inOutStringBegin, stringEnd );

				if( c == '\n' )
				{
					break;
				}
			}
			
		}

		void shrinkLineToFit( Line& line, float desiredLineWidth ) const
		{
			// Now keep chopping down the line until it's short enough, with the ellipses word, to fit.
			//
			while( !line.text.empty() && line.width() > desiredLineWidth )
			{
				const float thisCharacterWidth = getSimpleCharWidth( line.text.back() );
				line.text.pop_back();
				line.bounds[ 2 ] -= thisCharacterWidth;
			}
		}
		
		template< typename iter_t >
		iter_t digestLine( iter_t lineBegin, iter_t textEnd, float enforcedBoundX, Enforcement widthEnforcement, float topY, bool& forcedNewline )
		{
			m_lines.resize( m_lines.size() + 1 );
			Line& line = m_lines.back();
			
			line.bounds[ 0 ] = line.bounds[ 2 ] = 0;
			line.bounds[ 1 ] = topY * m_fontPixelSize;
			line.bounds[ 3 ] = line.bounds[ 1 ] + m_lineHeight * m_fontPixelSize;
			
			while( lineBegin != textEnd )
			{
				std::string word;
				
				auto originalLineBegin = lineBegin;
				forcedNewline = false;
				bool wantsNewLine = digestWord( /*in out*/ lineBegin, textEnd, std::back_inserter( word ), enforcedBoundX, forcedNewline );
				
				if( !word.empty() )
				{
					const float wordPixelWidth = getSimpleTextWidth( word.begin(), word.end() );
					const float newWidth = line.width() + wordPixelWidth;
					
					Enforcement currentPolicy = widthEnforcement;
					
					// If we're at the beginning of a line, don't try to wrap: just truncate for now.
					if( line.width() == 0 && currentPolicy == Enforcement::Wrap )
					{
						currentPolicy = Enforcement::Truncate;
					}
					
					// Have we gone past the end of the line?
					//
					if( enforcedBoundX > 0 && ( newWidth > enforcedBoundX ))
					{
						// Yes. Don't add the word.

						wantsNewLine = true;

						switch( currentPolicy )
						{
							default:
							case Enforcement::None:
							case Enforcement::Scale:
							case Enforcement::ScaleDown:
							case Enforcement::ScaleUp:
							{
								// Go ahead and add the word.

								line.text += word;
								line.bounds[ 2 ] = newWidth;

								// Just keep going.
								break;
							}
								
							case Enforcement::Truncate:
							{
								// Go ahead and add the word.
								
								line.text += word;
								line.bounds[ 2 ] = newWidth;
								
								shrinkLineToFit( line, enforcedBoundX );

								// Skip the rest of the incoming text, up until the next newline.
								eatUntilNewLine( lineBegin, textEnd );
								break;
							}
							case Enforcement::Wrap:
							{
								// If the text thus far ends in a break (e.g. space), delete it.
								//
								while( !line.text.empty() && std::isspace( line.text.back() ))
								{
									const float thisCharacterWidth = getSimpleCharWidth( line.text.back() );
									line.text.pop_back();
									line.bounds[ 2 ] -= thisCharacterWidth;
								}
								
								// Send this word on to the next line.
								//
								lineBegin = originalLineBegin;
								
								break;	// from switch
							}
							case Enforcement::Ellipses:
							{
								// Go ahead and add the word.

								line.text += word;
								line.bounds[ 2 ] = newWidth;

								word = "...";
								const float ellipsesWidth = getSimpleTextWidth( word.begin(), word.end() );
								const float desiredLineWidth = enforcedBoundX - ellipsesWidth;

								shrinkLineToFit( line, desiredLineWidth );
								
								// Add the ellipses back in.
								//
								line.text += word;
								line.bounds[ 2 ] += ellipsesWidth;
								
								eatUntilNewLine( lineBegin, textEnd );
								
								break;	// from switch
							}
						}
					}
					else
					{
						// Still have room in the bounds. Add the word.
						//
						line.text += word;
						line.bounds[ 2 ] = newWidth;
					}
				}
				
				if( wantsNewLine )
				{
					break;
				}
			}
			
			return lineBegin;
		}
		
		template< typename iter_t, typename outIter_t >
		bool digestWord( iter_t& inOutWordBegin, iter_t textEnd, outIter_t target, float enforcedBoundX, bool& forcedNewline )
		{
			forcedNewline = false;
			while( inOutWordBegin != textEnd )
			{
				const int c = getNextChar( inOutWordBegin, textEnd );
				
				if( c == '\n' )
				{
					// End of word, with newline.
					forcedNewline = true;
					return true;
				}
				
				*target++ = c;
				
				if( c != NON_BREAKING_SPACE && std::isspace( c ))
				{
					// End of word. No need for newline.
					return false;
				}
			}
			
			// Reached end of text. Ask for a new line--though actually we're done.
			return true;
		}
		
		template< typename iter_t >
		int getNextChar( iter_t& ic, iter_t end )
		{
			bool inEscapedState = false;
			while( ic != end )
			{
				const int c = *ic;
				++ic;
				if( !inEscapedState )
				{
					if( m_allowCStyleEscapes && c == '\\' )
					{
						inEscapedState = true;
					}
					else
					{
						return c;
					}
				}
				else
				{
					inEscapedState = false;

					int d = 0;
					if( ic != end )
					{
						d = *ic;
					}
					
					switch( c )
					{
						case '\n':				// \n	=> UNIX
						{
							if( d == '\r' )		// \n\r => Acorn BBS/RISC OS
							{
								// Move past this double newline.
								++ic;
								discoverLineFeedStyle( LineFeeds::Other );
							}
							else
							{
								discoverLineFeedStyle( LineFeeds::Unix );
							}
							// In any case suppress this newline.
						}
						break;
							
						case '\r':				// \r	=> Mac <= 9, Commodore, etc.
						{
							if( d == '\n' )		// \n\r => Windows
							{
								// Move past this double newline.
								++ic;
								discoverLineFeedStyle( LineFeeds::Windows );
							}
							else
							{
								discoverLineFeedStyle( LineFeeds::Mac9 );
							}
							// In any case suppress this newline.
						}
						break;
							
						case 'n':
							return '\n';
							
						default:
							return c;
					}
				}
			}
			return 0;
		}
		
		void discoverLineFeedStyle( LineFeeds lineFeeds )
		{
			if( m_lineFeeds == LineFeeds::Unknown )
			{
				m_lineFeeds = lineFeeds;
			}
			else if( m_lineFeeds != lineFeeds )
			{
				m_lineFeeds = LineFeeds::Inconsistent;
			}
		}

	private:
		
		Lines m_lines;
		rect m_bounds;
		
		const float m_fontPixelSize = 0;
		const float m_lineHeight = 0;
		const std::vector< float > m_characterWidths;
		
		bool m_wasTextTruncatedX;
		bool m_wasTextTruncatedY;
		bool m_allowCStyleEscapes;
		
		LineFeeds m_lineFeeds = LineFeeds::Unknown;
	};
	
	FRESH_ENUM_STREAM_IN_BEGIN( TextMetrics, Alignment )
	FRESH_ENUM_STREAM_IN_CASE( TextMetrics::Alignment, Left )
	FRESH_ENUM_STREAM_IN_CASE( TextMetrics::Alignment, Centered )
	FRESH_ENUM_STREAM_IN_CASE( TextMetrics::Alignment, Right )
	FRESH_ENUM_STREAM_IN_END()
	
	FRESH_ENUM_STREAM_OUT_BEGIN( TextMetrics, Alignment )
	FRESH_ENUM_STREAM_OUT_CASE( TextMetrics::Alignment, Left )
	FRESH_ENUM_STREAM_OUT_CASE( TextMetrics::Alignment, Centered )
	FRESH_ENUM_STREAM_OUT_CASE( TextMetrics::Alignment, Right )
	FRESH_ENUM_STREAM_OUT_END()
	
	FRESH_ENUM_STREAM_IN_BEGIN( TextMetrics, VerticalAlignment )
	FRESH_ENUM_STREAM_IN_CASE( TextMetrics::VerticalAlignment, Top )
	FRESH_ENUM_STREAM_IN_CASE( TextMetrics::VerticalAlignment, Middle )
	FRESH_ENUM_STREAM_IN_CASE( TextMetrics::VerticalAlignment, Bottom )
	FRESH_ENUM_STREAM_IN_END()
	
	FRESH_ENUM_STREAM_OUT_BEGIN( TextMetrics, VerticalAlignment )
	FRESH_ENUM_STREAM_OUT_CASE( TextMetrics::VerticalAlignment, Top )
	FRESH_ENUM_STREAM_OUT_CASE( TextMetrics::VerticalAlignment, Middle )
	FRESH_ENUM_STREAM_OUT_CASE( TextMetrics::VerticalAlignment, Bottom )
	FRESH_ENUM_STREAM_OUT_END()
	
	FRESH_ENUM_STREAM_IN_BEGIN( TextMetrics, Enforcement )
	FRESH_ENUM_STREAM_IN_CASE( TextMetrics::Enforcement, None )
	FRESH_ENUM_STREAM_IN_CASE( TextMetrics::Enforcement, Wrap )
	FRESH_ENUM_STREAM_IN_CASE( TextMetrics::Enforcement, Ellipses )
	FRESH_ENUM_STREAM_IN_CASE( TextMetrics::Enforcement, Truncate )
	FRESH_ENUM_STREAM_IN_CASE( TextMetrics::Enforcement, Scale )
	FRESH_ENUM_STREAM_IN_CASE( TextMetrics::Enforcement, ScaleDown )
	FRESH_ENUM_STREAM_IN_CASE( TextMetrics::Enforcement, ScaleUp )
	FRESH_ENUM_STREAM_IN_END()
	
	FRESH_ENUM_STREAM_OUT_BEGIN( TextMetrics, Enforcement )
	FRESH_ENUM_STREAM_OUT_CASE( TextMetrics::Enforcement, None )
	FRESH_ENUM_STREAM_OUT_CASE( TextMetrics::Enforcement, Wrap )
	FRESH_ENUM_STREAM_OUT_CASE( TextMetrics::Enforcement, Ellipses )
	FRESH_ENUM_STREAM_OUT_CASE( TextMetrics::Enforcement, Truncate )
	FRESH_ENUM_STREAM_OUT_CASE( TextMetrics::Enforcement, Scale )
	FRESH_ENUM_STREAM_OUT_CASE( TextMetrics::Enforcement, ScaleDown )
	FRESH_ENUM_STREAM_OUT_CASE( TextMetrics::Enforcement, ScaleUp )
	FRESH_ENUM_STREAM_OUT_END()
	
}

#endif
