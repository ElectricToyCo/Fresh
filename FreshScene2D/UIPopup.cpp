//
//  UIPopup.cpp
//  Fresh
//
//  Created by Jeff Wofford on 12/19/11.
//  Copyright (c) 2011 jeffwofford.com. All rights reserved.
//

#include "UIPopup.h"
#include "Stage.h"

namespace fr
{

	const char* UIPopup::SHOWN = "Shown";
	const char* UIPopup::HIDDEN = "Hidden";
	
	FRESH_DEFINE_CLASS( UIPopup )
	DEFINE_VAR( UIPopup, DisplayObjectContainer::ptr, m_host );
	DEFINE_VAR( UIPopup, bool, m_startHidden );
	DEFINE_VAR( UIPopup, bool, m_startBecomeShown );
	DEFINE_VAR( UIPopup, vec2, m_hiddenScale );
	DEFINE_VAR( UIPopup, Color, m_hiddenColor );
	DEFINE_VAR( UIPopup, vec2, m_hiddenTranslation );
	DEFINE_VAR( UIPopup, real, m_hiddenRotation );
	DEFINE_VAR( UIPopup, bool, m_visibleWhenHidden );
	DEFINE_VAR( UIPopup, bool, m_touchableWhenHiddenOrHiding );
	DEFINE_VAR( UIPopup, TimeType, m_defaultShowDuration );
	DEFINE_VAR( UIPopup, TimeType, m_defaultHideDuration );
	DEFINE_VAR( UIPopup, std::string, m_tweenShowType );
	DEFINE_VAR( UIPopup, std::string, m_tweenHideType );
	
	DEFINE_METHOD( UIPopup, show )
	DEFINE_METHOD( UIPopup, hide )
	DEFINE_METHOD( UIPopup, toggle )
	
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( UIPopup )
	
	bool UIPopup::isShown() const
	{
		return isBecomingShown() || isFullyShown() || isBecomingHidden();	// Either shown, going from shown to hidden, or going from hidden to shown.
	}
	
	bool UIPopup::isFullyShown() const
	{
		return !hasKeyframe( currentFrame() ) || currentLabel() == "shown";
	}
	
	bool UIPopup::isFullyHidden() const
	{
		return !isShown();
	}
	
	bool UIPopup::isBecomingShown() const
	{
		return m_isBecomingShown;
	}
	
	bool UIPopup::isBecomingHidden() const
	{
		return m_isBecomingHidden;
	}
	
	bool UIPopup::isHideQueued() const
	{
		return m_queuedHideDuration >= 0;
	}

	void UIPopup::showWithDuration( TimeType duration )
	{
		visible( true );
		
		if( !isBecomingShown() )
		{
			m_isBecomingShown = true;
			m_isBecomingHidden = false;
			
			const auto tweener = DisplayObjectState::getTweenerByName( m_tweenShowType );
			ASSERT( tweener );
			
			tweenToKeyframe( "shown", duration, tweener, true );
		}
		
		m_queuedHideDuration = -1.0;			// Killing queued hide.
		
		PROMISES( !isHideQueued() );
	}
	
	void UIPopup::hideWithDuration( TimeType duration, bool deleteWhenHidden, TimeType queueIfShowingWithDelay )
	{
		REQUIRES( duration >= 0 );
		
		m_deleteWhenHidden = deleteWhenHidden;

		if( isShown() && !isBecomingHidden() )	// If not already hiding.
		{
			if( queueIfShowingWithDelay >= 0 && isBecomingShown() )
			{
				// Queue this hide.
				//
				m_queuedHideDelay = queueIfShowingWithDelay;
				m_queuedHideDuration = duration;
				m_queuedHideShouldDelete = deleteWhenHidden;
			}
			else
			{
				// TODO watch for double-hides.
				
				if( duration <= 0 && deleteWhenHidden )
				{
					markForDeletion();
				}
				else
				{
					m_isBecomingShown = false;
					m_isBecomingHidden = true;

					const auto tweener = DisplayObjectState::getTweenerByName( m_tweenHideType );
					ASSERT( tweener );
					
					tweenToKeyframe( "hidden", duration, tweener, true );
				}
			}
		}
	}

	void UIPopup::update()
	{
		Super::update();

		if( !isMarkedForDeletion() )
		{
			// Check for queued hide.
			//
			if( m_queuedHideDuration >= 0 && m_showCompleteTime > 0 && ( stage().time() - m_showCompleteTime ) >= m_queuedHideDelay )
			{
				TimeType savedDuration = m_queuedHideDuration;
				
				m_queuedHideDuration = -1.0;
				m_queuedHideDelay = -1.0;
				m_showCompleteTime = 0;
				
				hideWithDuration( savedDuration, m_queuedHideShouldDelete, 0 );
			}
		}
	}

	void UIPopup::onTweenComplete()
	{
		if( m_isBecomingShown )
		{
			onShowCompleted();
		}
		else if( m_isBecomingHidden )
		{
			onHideCompleted();
		}
	}
	
	void UIPopup::onShowCompleted()
	{
		m_isBecomingShown = false;
		m_showCompleteTime = hasStage() ? stage().time() : 0;
		
		Event event( SHOWN, this );
		dispatchEvent( &event );
	}
	
	void UIPopup::onHideCompleted()
	{
		m_isBecomingHidden = false;
		
		visible( m_visibleWhenHidden );

		Event event( HIDDEN, this );
		dispatchEvent( &event );
		
		if( !isMarkedForDeletion() && m_deleteWhenHidden )
		{
			markForDeletion();
		}		
	}

	void UIPopup::onAddedToStage()
	{
		if( doUpdate() )
		{
			setupHost();
		}
		
		// Must come last; otherwise MovieClip::onAddedToStage() will try to setup the initial keyframe based on the initial child state, which excludes the popup host.
		Super::onAddedToStage();
	}
	
	bool UIPopup::isTouchable() const
	{
		return Super::isTouchable() && ( m_touchableWhenHiddenOrHiding || ( isFullyShown() || isBecomingShown() ));
	}
	
	void UIPopup::setupHost()
	{
		if( !m_host )
		{
			m_host = createObject< DisplayObjectContainer >( getTransientPackage() );		// Actually contains the popup's children.
			m_host->inheritParentFrame( true );
			m_host->isTouchEnabled( true );
		}
		
		// Shove children up into host.
		//
		auto copyChildren = m_children;
		for( auto child : copyChildren )
		{
			if( child != m_host )
			{
				removeChild( child );
				m_host->addChild( child );
			}
		}

		// Add the host.
		//
		if( !hasChild( m_host ))
		{
			addChild( m_host );
		}
		
		// Construct default keyframes.
		//
		{
			Keyframe keyframe;
			keyframe.setChildState( m_host->name(), DisplayObjectState());
			setKeyframe( "shown", keyframe );
		}
		{
			Keyframe keyframe;
			keyframe.setChildState( m_host->name(), DisplayObjectState( m_hiddenTranslation, m_hiddenScale, m_hiddenRotation, m_hiddenColor ));
			setKeyframe( "hidden", keyframe );
		}				
		
		// Start shown or hidden as required.
		//
		stop();
		
		if( m_startHidden )
		{
			gotoKeyframe( "hidden" );
			visible( m_visibleWhenHidden );

			if( m_startBecomeShown )
			{
				show();
			}
		}
		else
		{
			gotoKeyframe( "shown" );
		}
		
		ASSERT( !isBecomingShown() || ( m_startHidden && m_startBecomeShown ));
		ASSERT( !isBecomingHidden() );	
	}
	
	void UIPopup::hiddenScale( const vec2& s )
	{
		m_hiddenScale = s;
		updateHiddenKeyframe();
	}
	
	void UIPopup::hiddenColor( Color c )
	{
		m_hiddenColor = c;
		updateHiddenKeyframe();
	}
	
	void UIPopup::hiddenTranslation( const vec2& t )
	{
		m_hiddenTranslation = t;
		updateHiddenKeyframe();		
	}
	
	void UIPopup::hiddenRotation( real a )
	{
		m_hiddenRotation = a;
		updateHiddenKeyframe();
	}

	void UIPopup::updateHiddenKeyframe()
	{
		if( m_host )
		{
			// Update the host's keyframe.
			//
			getKeyframe( "hidden" ).setChildState( m_host->name(), DisplayObjectState( m_hiddenTranslation, m_hiddenScale, m_hiddenRotation, m_hiddenColor ));
			
			// If we're already hidden, apply the state change immediately.
			//
			if( !isShown() && !isBecomingHidden() )
			{
				gotoKeyframe( "hidden" );
			}
		}
	}
}
