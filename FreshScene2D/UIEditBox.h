//
//  UIEditBox.h
//  Fresh
//
//  Created by Jeff Wofford on 3/21/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_UIEditBox_h
#define Fresh_UIEditBox_h

#include "MovieClip.h"
#include "EventKeyboard.h"

namespace fr
{
	
	class TextField;
	
	class UIEditBox : public Sprite
	{
		FRESH_DECLARE_CLASS( UIEditBox, Sprite )

	public:
		
		// Events
		//
		static const char* BEGIN_EDITING;
		static const char* END_EDITING;
		static const char* CHANGED;
		
		// Properties
		//
		SYNTHESIZE_GET( vec2, dimensions )
		void dimensions( const vec2& d );
		SYNTHESIZE( Color, backgroundColor )
		SYNTHESIZE( Color, borderColor )
		SYNTHESIZE( bool, multiline )
		SYNTHESIZE( bool, editable )
		
		const std::string& text() const;
		void text( const std::string& t );
		
		SYNTHESIZE_GET( SmartPtr< TextField >, textField )
		
		// Methods
		//
		SYNTHESIZE_GET( bool, editing )
		virtual void beginEditing();
		virtual void endEditing();
		
		virtual void onGainedKeyboardFocus() override;
		virtual void onLostKeyboardFocus() override;
		
		virtual void onTapped( const EventTouch& event ) override;
		virtual void onAddedToStage() override;
		
	protected:
		
		void drawFrame();
		void showPlaceholderTextIfNeeded();
		
		virtual void onInvalidKeystroke();
		
		void updateCaretPosition();
		
		FRESH_DECLARE_CALLBACK( UIEditBox, onKeyDown, EventKeyboard );

	private:
		
		SmartPtr< TextField > m_textField;
		SmartPtr< MovieClip > m_caret;
		
		DVAR( vec2, m_dimensions, vec2( 100, 40 ));
		DVAR( ClassInfo::cptr, m_textClass, nullptr );
		DVAR( ClassInfo::cptr, m_caretClass, nullptr );
		DVAR( Color, m_backgroundColor, Color::White );
		DVAR( Color, m_borderColor, Color::Gray );
		VAR( vec2, m_padding );
		VAR( std::string, m_placeholderText );	// Ignored if !m_initialText.empty()
		VAR( std::string, m_initialText );
		DVAR( bool, m_multiline, false );
		DVAR( bool, m_editable, true );
		
		bool m_showingPlaceholderText = false;
		bool m_editing = false;
		
		size_t m_insertionPos = 0;
	};
	
}

#endif
