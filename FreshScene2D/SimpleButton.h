/*
 *  SimpleButton.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/30/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_SIMPLE_BUTTON_H_INCLUDED
#define FRESH_SIMPLE_BUTTON_H_INCLUDED

#include "MovieClip.h"

namespace fr
{
	
	class SimpleButton : public MovieClip
	{
		FRESH_DECLARE_CLASS( SimpleButton, MovieClip )
	public:

		virtual void enabled( bool enabled );
		virtual bool enabled() const										{ return m_enabled; }
		
		virtual void finalizeKeyframes();								// Call this function after calling other keyframe-setup functions in order to ready the button for display.
		
		virtual void onTouchBegin( const EventTouch& event ) override;
		
		virtual bool isTouchable() const override;
		
		virtual void press();
		virtual void tapButton();

	protected:
		
		virtual void restoreDefaultState();
		virtual void onTapped( const EventTouch& event ) override;
		
		virtual void onAddedToParent() override;
		
		virtual void onTouchEndAnywhere( const EventTouch& event ) override;
		
	private:
		
		Sprite::ptr m_touchGlow;
		
		VAR( Object::wptr, m_onTapCallee );
		VAR( std::string, m_onTapCalleeMethodExpression );
		VAR( Texture::ptr, m_textureOut );
		VAR( Texture::ptr, m_textureDisabled );
		VAR( Texture::ptr, m_textureDown );
		VAR( Texture::ptr, m_textureUp );
		VAR( std::string, m_soundNameTapped );
		
		bool m_enabled = true;
	};
	
}

#endif
