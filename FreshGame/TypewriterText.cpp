//
//  TypewriterText.cpp
//  Fresh
//
//  Created by Jeff Wofford on 12/9/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "TypewriterText.h"
#include "TextMetrics.h"
#include "Stage.h"
#include "AudioSystem.h"
using namespace fr;

namespace
{
	void unescapeText( std::string& text )
	{
		// Replace all instances of "\*" with "*", but "\n" becomes the newline characters.
		//
		for( auto iter = text.begin(); iter != text.end(); ++iter )
		{
			if( *iter == '\\' )
			{
				auto next = iter + 1;
				if( next != text.end() )
				{
					iter = text.erase( iter );
					
					auto replacement = *iter;
					switch( replacement )
					{
						default: break;
						case 'n': replacement = '\n'; break;
						case 't': replacement = '\t'; break;
					}
					
					*iter = replacement;
				}
			}
		}
	}
	
	void placeholdWord( std::string& destText, const std::string& srcText, size_t iSrc )
	{
		bool sawSpace = false;
		while( iSrc < srcText.size() )
		{
			auto c = srcText[ iSrc ];
			
			if( std::isspace( c ))
			{
				sawSpace = true;
			}
			else
			{
				if( sawSpace )
				{
					// Done skipping trailing space.
					break;
				}
			}
			destText.push_back( static_cast< std::string::value_type >( TextMetrics::NON_BREAKING_SPACE ));
			++iSrc;
		}
	}

}

namespace fr
{	
	FRESH_DEFINE_CLASS( TypewriterText )
	DEFINE_VAR( TypewriterText, std::string, m_text );
	DEFINE_VAR( TypewriterText, TextGroup::ptr, m_textGroup );
	DEFINE_VAR( TypewriterText, TimeType, m_initialDelay );
	DEFINE_VAR( TypewriterText, TimeType, m_characterRevealDelay );
	DEFINE_VAR( TypewriterText, TimeType, m_repeatMessageDelay );
	DEFINE_VAR( TypewriterText, TimeType, m_newlineDelay );
	DEFINE_VAR( TypewriterText, TimeType, m_pausePunctuationDelay );
	DEFINE_VAR( TypewriterText, TimeType, m_endOfSentenceDelay );
	DEFINE_VAR( TypewriterText, std::string, m_audioCharacter );
	DEFINE_VAR( TypewriterText, std::string, m_audioNewline );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( TypewriterText )

	TextGroup& TypewriterText::textGroup() const
	{
		ASSERT( m_textGroup );
		return *m_textGroup;
	}
	
	void TypewriterText::text( const std::string& text_ )
	{
		m_text = text_;
		unescapeText( m_text );
		m_lastProcessedText = m_text;
		
		m_lastSrc = 0;
		m_lastDest = 0;
		m_nextRevealTime = m_nextMessageRepeatTime = 0;
		m_revealedText.clear();
		textGroup().text( m_revealedText );
	}
	
	void TypewriterText::restartDisplay()
	{
		text( m_text );
	}
	
	void TypewriterText::update()
	{
		Super::update();
		
		// Supporting console functionality: detect mysterious changes in the set text.
		//
		if( m_lastProcessedText != text() )
		{
			// Preserve next message repeat time.
			//
			const auto repeatTime = m_nextMessageRepeatTime;
			text( text() );
			m_nextMessageRepeatTime = repeatTime;
		}
		
		// Waiting to repeat the message?
		//
		if( m_nextMessageRepeatTime == 0 )
		{
			// No. Time to reveal?
			//
			while( stage().time() >= m_nextRevealTime )
			{
				// Yes. More message left to reveal?
				//
				//
				if( m_lastSrc < m_text.size() )
				{
					// Yes. Schedule the next reveal.
					//
					m_nextRevealTime = stage().time() + m_characterRevealDelay;
					
					// Reveal a character.
					
					if( m_lastDest == m_revealedText.size() )
					{
						placeholdWord( m_revealedText, m_text, m_lastSrc );
					}
					
					auto c = m_text.at( m_lastSrc );
					
					m_revealedText.at( m_lastDest ) = c;
					++m_lastDest;
					++m_lastSrc;
					
					ASSERT( m_textGroup );
					m_textGroup->text( m_revealedText );

					auto soundName = m_audioCharacter;
					
					if( c == '\n' )
					{
						// Newline.
						//
						if( !m_audioNewline.empty() )
						{
							soundName = m_audioNewline;
						}
						
						// Delay.
						//
						if( m_newlineDelay > 0 )
						{
							m_nextRevealTime = stage().time() + m_newlineDelay;
						}
					}
					else if(( c == '.' || c == '!' || c == '?' )
							&& m_endOfSentenceDelay > 0 )
					{
						m_nextRevealTime = stage().time() + m_endOfSentenceDelay;
					}
					else if(( c == ',' || c == ';' || c == ':' )
							&& m_pausePunctuationDelay > 0 )
					{
						m_nextRevealTime = stage().time() + m_pausePunctuationDelay;
					}

					if( !soundName.empty() )
					{
						AudioSystem::playSound( soundName );
					}
				}
				else
				{
					// Done with the message. Schedule the next repeat.
					//
					m_nextMessageRepeatTime = stage().time() + m_repeatMessageDelay;
					break;
				}
			}
		}
		else if( stage().time() >= m_nextMessageRepeatTime )
		{
			// Time to repeat the message.
			//
			m_nextMessageRepeatTime = 0;
			text( text() );
		}
	}
	
	void TypewriterText::postLoad()
	{
		Super::postLoad();
		
		if( !m_textGroup )
		{
			// Look for a child text group.
			m_textGroup = getChildByName< TextGroup >( "", NameSearchPolicy::Substring );
			ASSERT( m_textGroup );
		}
		
		// Grab a descendant TextField to help with metrics.
		//
		m_textField = m_textGroup->getChildByName< TextField >( "", NameSearchPolicy::Substring );
		ASSERT( m_textField );
	}
	
	void TypewriterText::onAddedToStage()
	{
		Super::onAddedToStage();
		
		m_nextMessageRepeatTime = stage().time() + m_initialDelay;
	}
}

