//
//  UIScrollView.h
//  Fresh
//
//  Created by Jeff Wofford on 8/17/17.
//  Copyright (c) 2017 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_UIScrollView_h
#define Fresh_UIScrollView_h

#include "Sprite.h"

namespace fr
{
	// A scroll view enables the dynamic (i.e. physics-like) dragging of its children.
	// It uses the calculated bounds of its children and its own frame (i.e. `frame()`)
	// to know what scroll directions are possible and how far.
	//
	class UIScrollView : public DisplayObjectContainer
	{
		FRESH_DECLARE_CLASS( UIScrollView, DisplayObjectContainer );
	public:
		
		class Delegate
		{
		public:
			virtual ~Delegate() {}
			virtual rect scrollViewContentBounds() const = 0;
			virtual void onScrollViewShowing( const rect& area ) {}
		};
		
		void delegate( Object::wptr delegate );
		// REQUIRES( !delegate || dynamic_cast< Delegate* >( delegate.get() ));

		void scrollToShow( const vec2& locationWithinContent, bool instant = false );
		
		virtual void onAddedToStage() override;
		virtual void update() override;

		vec2 scrollVelocity() const;
		
		virtual void onTouchBegin( const EventTouch& event ) override;
		virtual void onTouchMove( const EventTouch& event ) override;
		virtual void onTouchEnd( const EventTouch& event ) override;
		virtual void onWheelMove( const EventTouch& event ) override;
		
	protected:

		bool isScrollingToShowLocation() const;
		
		rect hostContentBounds() const;
		
		virtual void setupHost();
		virtual void setupTouchCatcher();
		
		virtual void updateScrollToShow();
		virtual void updateDynamics();
		virtual void updateCollision();
		
		virtual vec2 desiredPositionToShow( const vec2& pos ) const;
		
	private:
		
		VAR( DisplayObjectContainer::ptr, m_host );
		VAR( Sprite::ptr, m_touchCatcher );
		DVAR( vec2, m_dragScale, vec2( 1 ));
		DVAR( TimeType, m_scrollToShowDuration, 0.35 );
		VAR( Object::wptr, m_delegate );
		vec2 m_scrollLastPosition;
		bool m_isScrollDragging = false;
		
		vec2 m_locationWithinContentToShow;
		vec2 m_showStartingLocation;
		TimeType m_showStartTime = -1;
	};
	
}

#endif
