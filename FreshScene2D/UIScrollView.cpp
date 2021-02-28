//
//  UIScrollView.cpp
//  Fresh
//
//  Created by Jeff Wofford on 8/17/17.
//  Copyright (c) 2017 Jeff Wofford. All rights reserved.
//

#include "UIScrollView.h"
#include "Stage.h"
#include "FreshMath.h"

namespace
{
	using namespace fr;
	
	const real SCROLL_DAMPING = 0.1f;
	const real WHEEL_SCALE = -0.1f;
}

namespace fr
{	
	FRESH_DEFINE_CLASS( UIScrollView )
	DEFINE_VAR( UIScrollView, DisplayObjectContainer::ptr, m_host );
	DEFINE_VAR( UIScrollView, Sprite::ptr, m_touchCatcher );
	DEFINE_VAR( UIScrollView, vec2, m_dragScale );
	DEFINE_VAR( UIScrollView, TimeType, m_scrollToShowDuration );
	DEFINE_VAR( UIScrollView, Object::wptr, m_delegate );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( UIScrollView )
	
	void UIScrollView::delegate( Object::wptr delegate )
	{
		REQUIRES( !delegate || dynamic_cast< Delegate* >( delegate.get() ));
		m_delegate = delegate;
	}
	
	bool UIScrollView::isScrollingToShowLocation() const
	{
		return m_showStartTime >= 0;
	}
	
	void UIScrollView::scrollToShow( const vec2& locationWithinContent, bool instant )
	{
		m_showStartTime = -1;
		
		if( m_isScrollDragging )
		{
			return;
		}
		
		if( instant )
		{
			const auto pos = desiredPositionToShow( locationWithinContent );
			m_host->position( pos );
			updateCollision();
			m_scrollLastPosition = m_host->position();
		}
		else
		{
			m_showStartTime = stage().time();
			m_locationWithinContentToShow = locationWithinContent;
			m_showStartingLocation = m_host->position();
		}
	}

	void UIScrollView::onAddedToStage()
	{
		if( doUpdate() )
		{
			setupHost();
			setupTouchCatcher();
			
			// Setup mask.
			//
			if( !mask() )
			{
				const auto theMask = createObject< Sprite >( getTransientPackage() );
				theMask->setTextureByName( "white_simple" );
				theMask->position( frame().midpoint() );
				theMask->scale( frame().dimensions() * 0.5f );
				mask( theMask );
			}
		}
		
		Super::onAddedToStage();
		
		m_scrollLastPosition = position();
	}
	
	void UIScrollView::update()
	{
		if( !doUpdate() )
		{
			return;
		}
		
		if( isScrollingToShowLocation() )
		{
			updateScrollToShow();
		}
		else if( !m_isScrollDragging )
		{
			updateDynamics();
		}
		
		updateCollision();

		Super::update();

		if( auto delegate = dynamic_cast< Delegate* >( m_delegate.get() ))
		{
			delegate->onScrollViewShowing( m_host->parentToLocal( frame() ));
		}
	}
	
	vec2 UIScrollView::scrollVelocity() const
	{
		return m_host->position() - m_scrollLastPosition;
	}
	
	void UIScrollView::onTouchBegin( const EventTouch& event )
	{
		Super::onTouchBegin( event );
		m_scrollLastPosition = m_host->position();
		m_isScrollDragging = true;
		m_showStartTime = -1;
	}

	void UIScrollView::onTouchMove( const EventTouch& event )
	{
		m_scrollLastPosition = m_host->position();
		m_host->position( m_host->position() + m_dragScale * ( event.location() - event.previousLocation() ));
		Super::onTouchMove( event );
	}

	void UIScrollView::onTouchEnd( const EventTouch& event )
	{
		Super::onTouchEnd( event );
		m_isScrollDragging = false;
	}

	void UIScrollView::onWheelMove( const EventTouch& event )
	{
		Super::onWheelMove( event );
		m_host->position( m_host->position() + m_dragScale * ( event.wheelDelta() * WHEEL_SCALE ));
	}

	void UIScrollView::setupHost()
	{
		if( !m_host )
		{
			m_host = createObject< DisplayObjectContainer >( getTransientPackage() );		// Actually contains the popup's children.
			m_host->inheritParentFrame( true );
			m_host->isTouchEnabled( true );
			m_host->minStageDistanceForDrag( 2 );
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
	}
	
	void UIScrollView::setupTouchCatcher()
	{
		if( !m_touchCatcher )
		{
			m_touchCatcher = createObject< Sprite >( getTransientPackage() );
			m_touchCatcher->isTouchEnabled();
			m_touchCatcher->setTextureByName( "white_simple" );
			m_touchCatcher->blendMode( Renderer::BlendMode::AlphaPremultiplied );
			m_touchCatcher->color( Color::BarelyVisible );
			m_touchCatcher->position( frame().midpoint() );
			m_touchCatcher->scale( frame().dimensions() * 0.5f );
		}
		
		if( !hasChild( m_touchCatcher ))
		{
			addChildAt( m_touchCatcher, 0 );
		}
	}
	
	void UIScrollView::updateScrollToShow()
	{
		if( m_showStartTime < 0 )
		{
			// Nothing to do.
			return;
		}
		
		const auto elapsed = std::min( m_scrollToShowDuration, stage().time() - m_showStartTime );
		if( elapsed >= m_scrollToShowDuration )
		{
			// Done. Do one last iteration.
			//
			m_showStartTime = -1;
		}

		const auto desired = desiredPositionToShow( m_locationWithinContentToShow );
		TweenerSinEaseInOut<vec2> tweener;
		const auto pos = tweener( m_showStartingLocation, desired, elapsed / m_scrollToShowDuration );
		m_host->position( pos );
		updateCollision();
		m_scrollLastPosition = m_host->position();
	}
	
	void UIScrollView::updateDynamics()
	{
		const auto deltaTime = static_cast< real >( stage().secondsPerFrame());
		
		m_host->position( updateVerlet( m_host->position(), m_scrollLastPosition, SCROLL_DAMPING, {}, deltaTime ));
	}
	
	rect UIScrollView::hostContentBounds() const
	{
		rect contentBounds = m_host->bounds();
		if( auto delegate = dynamic_cast< const Delegate* >( m_delegate.get() ))
		{
			const auto delegateBounds = delegate->scrollViewContentBounds();
			if( delegateBounds.isWellFormed() )
			{
				contentBounds = delegateBounds;
			}
		}
		return contentBounds;
	}
	
	void UIScrollView::updateCollision()
	{
		const auto& myFrame = frame();
		if( !myFrame.isWellFormed() )
		{
			return;
		}

		const auto contentBounds = hostContentBounds();
		
		if( !contentBounds.isWellFormed() )
		{
			return;
		}
		
		const vec2 pos = fr::clampVector( contentBounds.ulCorner(), myFrame.brCorner() - contentBounds.dimensions(), myFrame.ulCorner() );
		
		m_host->position( m_host->position() + ( pos - contentBounds.ulCorner() ));
	}
	
	vec2 UIScrollView::desiredPositionToShow( const vec2& pos ) const
	{
		// How would we position the host such that this position within its content
		// would sit at the center of our frame?
		//
		const auto& myFrame = frame();
		return -m_host->localToParentPoint( pos ) - myFrame.ulCorner();
	}
}

