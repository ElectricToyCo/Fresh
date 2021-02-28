//
//  TypewriterText.h
//  Fresh
//
//  Created by Jeff Wofford on 12/9/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_TyperwriterText_h
#define Fresh_TyperwriterText_h

#include "FreshEssentials.h"
#include "DisplayObjectContainer.h"
#include "TextGroup.h"
#include "TextField.h"

namespace fr
{
	class TypewriterText : public DisplayObjectContainer
	{
		FRESH_DECLARE_CLASS( TypewriterText, DisplayObjectContainer );
	public:
		
		SYNTHESIZE_GET( std::string, text )
		void text( const std::string& text_ );
		
		void restartDisplay();
		
		virtual void update() override;

		TextGroup& textGroup() const;
		
		virtual void onAddedToStage() override;
		virtual void postLoad() override;
		
	private:
		
		VAR( std::string, m_text );
		VAR( TextGroup::ptr, m_textGroup );
		DVAR( TimeType, m_initialDelay, 0 );
		DVAR( TimeType, m_characterRevealDelay, 3 );
		DVAR( TimeType, m_repeatMessageDelay, 0 );
		DVAR( TimeType, m_newlineDelay, 0 );
		DVAR( TimeType, m_pausePunctuationDelay, 0 );
		DVAR( TimeType, m_endOfSentenceDelay, 0 );
		VAR( std::string, m_audioCharacter );
		VAR( std::string, m_audioNewline );
		
		TextField::ptr m_textField;
		
		std::string m_lastProcessedText;
		
		std::string m_revealedText;
		size_t m_lastSrc;
		size_t m_lastDest;
		
		TimeType m_nextRevealTime = 0;
		TimeType m_nextMessageRepeatTime = 0;
	};
	
}

#endif
