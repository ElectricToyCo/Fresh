//
//  UIPopup.h
//  Fresh
//
//  Created by Jeff Wofford on 12/19/11.
//  Copyright (c) 2011 jeffwofford.com. All rights reserved.
//

#ifndef Fresh_Popup_h
#define Fresh_Popup_h

#include "MovieClip.h"

namespace fr
{
	
	class UIPopup : public MovieClip
	{
		FRESH_DECLARE_CLASS( UIPopup, MovieClip )
	public:

		static const char* SHOWN;
		static const char* HIDDEN;

		SYNTHESIZE_GET( DisplayObjectContainer::ptr, host );
		SYNTHESIZE( bool, startHidden );

		SYNTHESIZE_GET( const vec2&, hiddenScale );
		SYNTHESIZE_GET( Color, hiddenColor );
		SYNTHESIZE_GET( const vec2&, hiddenTranslation );
		SYNTHESIZE_GET( real, hiddenRotation );
		SYNTHESIZE( TimeType, defaultShowDuration );
		SYNTHESIZE( TimeType, defaultHideDuration );
		
		void hiddenScale( const vec2& s );
		void hiddenColor( Color c );
		void hiddenTranslation( const vec2& t );
		void hiddenRotation( real a );
		
		bool isShown() const;			// It's shown if it's fully shown, becoming shown, or becoming hidden.
		bool isFullyShown() const;		// It's fully shown only if it's shown and not becoming anything.
		bool isFullyHidden() const;
		bool isBecomingShown() const;
		bool isBecomingHidden() const;
		virtual void showWithDuration( TimeType duration );
		void show() { showWithDuration( m_defaultShowDuration ); }
		// PROMISES( !isHideQueued() );
		virtual void hideWithDuration( TimeType duration, bool deleteWhenHidden = false, TimeType queueIfShowingWithDelay = -1.0 );
		void hide() { hideWithDuration( m_defaultHideDuration ); }
		// REQUIRES( duration >= 0 );
		// If queueIfShowingWithDelay >= 0 and the popup is currently becoming shown, then after the 
		// popup finishes being shown, it will be hidden after queueIfShowingWithDelay seconds.
		
		virtual void toggle() { if( isShown() ) hide(); else show(); }
		
		virtual void showOrHide( bool showElseHide ) { if( showElseHide ) show(); else hide(); }
		
		bool isHideQueued() const;

		virtual bool isTouchable() const override;
		
		virtual void onAddedToStage() override;		
		virtual void update() override;
		
	protected:
		
		virtual void onTweenComplete() override;
		virtual void onShowCompleted();
		virtual void onHideCompleted();
		
		void setupHost();
		void updateHiddenKeyframe();
		
	private:
		
		VAR( DisplayObjectContainer::ptr, m_host );
		DVAR( vec2, m_hiddenScale, vec2( 1.0f, 1.0f ));
		DVAR( Color, m_hiddenColor, Color::Invisible );
		DVAR( vec2, m_hiddenTranslation, vec2::ZERO );
		DVAR( real, m_hiddenRotation, 0 );
		DVAR( TimeType, m_defaultShowDuration, 0 );
		DVAR( TimeType, m_defaultHideDuration, 0 );
		DVAR( std::string, m_tweenShowType, "QuadEaseInOut" );
		DVAR( std::string, m_tweenHideType, "QuadEaseInOut" );
		DVAR( bool, m_visibleWhenHidden, false );
		DVAR( bool, m_touchableWhenHiddenOrHiding, false );
		DVAR( bool, m_startHidden, true );
		DVAR( bool, m_startBecomeShown, false );	// Only meaningful if m_startHidden.
		
		TimeType m_delayedShowStartTime = 0;
		TimeType m_delayedShowDuration = 0;
		
		TimeType m_queuedHideDuration = -1.0;
		TimeType m_queuedHideDelay = -1.0;
		TimeType m_showCompleteTime = 0;

		bool m_queuedHideShouldDelete = false;

		bool m_deleteWhenHidden = false;
		
		bool m_isBecomingShown = false;
		bool m_isBecomingHidden = false;
		
	};
}

#endif
