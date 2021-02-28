//
//  UIEditBox.cpp
//  Fresh
//
//  Created by Jeff Wofford on 3/21/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "UIEditBox.h"
#include "TextField.h"
#include "Stage.h"

namespace fr
{
	
	FRESH_DEFINE_CLASS( UIEditBox )
	DEFINE_VAR( UIEditBox, vec2, m_dimensions);
	DEFINE_VAR( UIEditBox, bool, m_multiline );
	DEFINE_VAR( UIEditBox, bool, m_editable );
	DEFINE_VAR( UIEditBox, ClassInfo::cptr, m_textClass );
	DEFINE_VAR( UIEditBox, ClassInfo::cptr, m_caretClass );
	DEFINE_VAR( UIEditBox, Color, m_backgroundColor );
	DEFINE_VAR( UIEditBox, Color, m_borderColor );
	DEFINE_VAR( UIEditBox, vec2, m_padding );
	DEFINE_VAR( UIEditBox, std::string, m_placeholderText );
	DEFINE_VAR( UIEditBox, std::string, m_initialText );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( UIEditBox )
	
	const char* UIEditBox::BEGIN_EDITING = "BeginEditing";
	const char* UIEditBox::END_EDITING = "EndEditing";
	const char* UIEditBox::CHANGED = "Changed";
	
	void UIEditBox::dimensions( const vec2& d )
	{
		ASSERT( m_textField );
		
		m_dimensions = d;
		drawFrame();
		m_textField->enforcedBounds( d );
	}
	
	const std::string& UIEditBox::text() const
	{
		ASSERT( m_textField );
		return m_textField->text();
	}
	
	void UIEditBox::text( const std::string& t )
	{
		ASSERT( m_textField );
		
		const bool changing = t != m_textField->text();
		if( changing )
		{
			m_showingPlaceholderText = false;
			
			m_textField->text( t );
		
			updateCaretPosition();
			
			Event e( CHANGED, this );
			dispatchEvent( &e );
		}
	}
	
	void UIEditBox::beginEditing()
	{
		ASSERT( m_editable );
		ASSERT( !m_editing );
				
		m_editing = true;
		
		m_caret->visible( m_editing );
		m_caret->gotoAndPlay( 0 );

		stage().addEventListener( EventKeyboard::KEY_DOWN, FRESH_CALLBACK( onKeyDown ));
		
		// Clear placeholder text, if any.
		//
		if( m_showingPlaceholderText )
		{
			text( "" );
			m_insertionPos = 0;
			m_showingPlaceholderText = false;
		}
		
		updateCaretPosition();
		
		Event e( BEGIN_EDITING, this );
		dispatchEvent( &e );
	}
	
	void UIEditBox::endEditing()
	{
		ASSERT( m_editing );
		
		m_editing = false;
		
		m_caret->visible( m_editing );

		stage().removeEventListener( EventKeyboard::KEY_DOWN, FRESH_CALLBACK( onKeyDown ));

		// If the box is still devoid of text, put the placeholder text back in.
		//
		showPlaceholderTextIfNeeded();

		Event e( END_EDITING, this );
		dispatchEvent( &e );
	}

	void UIEditBox::onGainedKeyboardFocus()
	{
		if( m_editable )
		{
			if( !m_editing )
			{
				beginEditing();
			}
		}
	}
	
	void UIEditBox::onLostKeyboardFocus()
	{
		if( m_editing )
		{
			endEditing();
		}
	}

	void UIEditBox::onTapped( const EventTouch& event )
	{
		// Situate the insertion position based on the tap location.
		//
		if( !m_showingPlaceholderText )
		{
			ASSERT( m_textField );
			TextMetrics metrics = m_textField->metrics();
			
			m_insertionPos = metrics.characterIndexFromPixelPosition( event.location() - m_textField->position() );
			
			if( m_insertionPos >= m_textField->textSize() )
			{
				m_insertionPos = m_textField->textSize();
			}
			
			updateCaretPosition();
		}
		
		if( stage().keyboardFocusHolder() != this )
		{
			stage().takeKeyboardFocus( this );
		}
	}
	
	void UIEditBox::onAddedToStage()
	{
		Super::onAddedToStage();
		
		drawFrame();

		// Setup the textfield.
		//
		if( !m_textField )
		{
			if( m_textClass )
			{
				m_textField = createObject< TextField >( *m_textClass );
			}
			else
			{
				m_textField = createObject< TextField >();
			}
			addChild( m_textField );
		}
		ASSERT( m_textField );
		
		m_textField->position( m_padding );
		
		m_textField->enforcedBounds( m_dimensions - m_padding * 2.0f );
		m_textField->text( m_initialText );
		
		showPlaceholderTextIfNeeded();
		
		// Setup the caret
		//
		if( !m_caret )
		{
			if( m_caretClass )
			{
				m_caret = createObject< MovieClip >( *m_caretClass );
			}
			else
			{
				m_caret = createObject< MovieClip >();
			}
			addChild( m_caret );
		}
		
		ASSERT( m_caret );
		
		m_caret->visible( false );
	}

	void UIEditBox::drawFrame()
	{
		Graphics& graphics_ = graphics();
		
		graphics_.clear();
		
		graphics_.lineStyle( m_borderColor );
		graphics_.beginFill( m_backgroundColor );
		graphics_.drawRect( 0, 0, m_dimensions.x, m_dimensions.y );
		graphics_.endFill();
	}
	
	void UIEditBox::showPlaceholderTextIfNeeded()
	{
		ASSERT( m_textField );
		
		if( text().empty() )
		{
			text( m_placeholderText );
			m_showingPlaceholderText = true;
		}
	}
	
	FRESH_DEFINE_CALLBACK( UIEditBox, onKeyDown, EventKeyboard )
	{
		if( stage().keyboardFocusHolder() != this ) return;
		
		auto charCode = event.charCode();
		
		// Convert carriage returns to newlines.
		//
		if( charCode == '\r' )
		{
			charCode = '\n';
		}
		
		auto myText = text();
		
		if( m_insertionPos > myText.size() )
		{
			// Hm. Got too big somehow. Probably the text changed without the user's intervention.
			//
			m_insertionPos = myText.size();
		}
		
		//
		// Is this one of those adjustment keys that move the caret around, delete things, etc.?
		//
		
		const auto key = event.key();
		
		const TextMetrics& metrics = m_textField->metrics();
		switch( key )
		{
			case Keyboard::Backspace:
				if( m_insertionPos > 0 )
				{
					myText.erase( myText.begin() + --m_insertionPos );
					text( myText );
				}
				else
				{
					onInvalidKeystroke();
				}
				break;
				
			case Keyboard::Delete:
				if( m_insertionPos < myText.size() )
				{
					myText.erase( myText.begin() + m_insertionPos );
					text( myText );
				}
				else
				{
					onInvalidKeystroke();
				}
				break;
				
			case Keyboard::RightArrow:
				if( m_insertionPos < myText.size() )
				{
					++m_insertionPos;
					updateCaretPosition();
				}
				else
				{
					onInvalidKeystroke();
				}
				break;
				
			case Keyboard::LeftArrow:
				if( m_insertionPos > 0 )
				{
					--m_insertionPos;
					updateCaretPosition();
				}
				else
				{
					onInvalidKeystroke();
				}
				break;
			case Keyboard::DownArrow:
			{
				auto newPos = metrics.lineStepPos( m_insertionPos, 1 );
				if( newPos != static_cast< size_t >( -1 ))
				{
					m_insertionPos = newPos;
					updateCaretPosition();
				}
				else
				{
					onInvalidKeystroke();
				}
				break;
			}
				
			case Keyboard::UpArrow:
			{
				auto newPos = metrics.lineStepPos( m_insertionPos, -1 );
				if( newPos != static_cast< size_t >( -1 ) )
				{
					m_insertionPos = newPos;
					updateCaretPosition();
				}
				else
				{
					onInvalidKeystroke();
				}
				break;
			}
				
			default:
				
				// Is this an actual character that wants inserting?
				//
				if( charCode )
				{
					ASSERT( m_textField );
					
					// Handle enter/return differently depending on whether we're multiline or not.
					//
					if( !m_multiline && charCode == '\n' )
					{
						stage().releaseKeyboardFocus( this );
					}
					else if( m_textField->isPrintableCharacter( charCode ))
					{
						myText.insert( myText.begin() + m_insertionPos++, charCode );
						
						text( myText );
					}
				}
				break;
		}
	}

	void UIEditBox::onInvalidKeystroke()
	{
		// Beep, flash or whatever to let the user know we can't accept this input.
		//
		// TODO
	}
	
	void UIEditBox::updateCaretPosition()
	{
		if( m_caret && m_caret->visible() )
		{
			ASSERT( m_textField );
			const TextMetrics& metrics = m_textField->metrics();
			
			const vec2 metricsPosition = metrics.characterPixelPosition< vec2 >( m_insertionPos );

			m_caret->position( m_textField->position() + metricsPosition );
		}
	}
}

