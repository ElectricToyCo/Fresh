//
//  UIPanel.cpp
//  Fresh
//
//  Created by Jeff Wofford on 6/7/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "UIPanel.h"
#include "Stage.h"

namespace fr
{	

	FRESH_DEFINE_CLASS_UNPLACEABLE( UIPanelContentHost )
	DEFINE_VAR( UIPanelContentHost, DisplayObject::ptr, m_content );
	DEFINE_VAR( UIPanelContentHost, bool, m_allowScrollingX );
	DEFINE_VAR( UIPanelContentHost, bool, m_allowScrollingY );
	DEFINE_VAR( UIPanelContentHost, bool, m_scaleToFitX );
	DEFINE_VAR( UIPanelContentHost, bool, m_scaleToFitY );
	
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( UIPanelContentHost )

	void UIPanelContentHost::content( DisplayObject::ptr newContent )
	{
		if( m_content )
		{
			removeChild( m_content );
		}
		
		m_content = newContent;
		
		if( m_content )
		{
			addChild( m_content );
			constrainContent();
		}
	}
	
	void UIPanelContentHost::reshape( const vec2& contentDimensions )
	{
		m_lastReshapeSize = frame().dimensions();
		
		// Resize the mask.
		//
		ASSERT( mask() );
		mask()->scale( m_lastReshapeSize * 0.5f );		// 0.5f because white_simple has dimensions of 2x2.
		
		// Resize the touch catcher.
		//
		ASSERT( m_touchCatcher );
		m_touchCatcher->scale( m_lastReshapeSize * 0.5f );
		
		constrainContent();
	}
	
	void UIPanelContentHost::onAddedToStage()
	{
		Super::onAddedToStage();
		
		if( !m_touchCatcher )
		{
			m_touchCatcher = createObject< Sprite >( name() + "_touch_catcher" );
			m_touchCatcher->setTextureByName( "white_simple" );
			m_touchCatcher->color( Color::BarelyVisible );
			m_touchCatcher->blendMode( Renderer::BlendMode::AlphaPremultiplied );
			addChild( m_touchCatcher );
		}
		
		if( m_content )
		{
			if( !hasChild( m_content ))
			{
				addChild( m_content );
			}
			else
			{
				swapChildren( m_content, m_touchCatcher );
			}
			ASSERT( getChildIndex( m_content ) > getChildIndex( m_touchCatcher ));
		}

		addEventListener( EventTouch::WHEEL_MOVE, FRESH_CALLBACK( onWheel ));

		// Create the clipping mask.
		//
		auto myMask = createObject< Sprite >( name() + "_mask" );
		myMask->setTextureByName( "white_simple" );
		myMask->color( Color::Magenta );
		mask( myMask );
		
		reshape( frame().dimensions() );
	}

	void UIPanelContentHost::update()
	{
		Super::update();
		
		if( m_lastReshapeSize != frame().dimensions() )
		{
			reshape( frame().dimensions() );
		}
		
		// TODO slow. Either remove or reinstate.
		// constrainContent();
	}
	
	void UIPanelContentHost::constrainContent()
	{
		if( m_content )
		{
			const auto& contentSize = m_content->localBounds().dimensions();
			
			if( contentSize.x > 0 && contentSize.y > 0 )
			{
				vec2 contentScale( m_content->scale() );
				
				if( m_scaleToFitX )
				{
					contentScale.x = m_lastReshapeSize.x / contentSize.x;
					
					if( !m_scaleToFitY )		// Preserve uniform scaling.
					{
						contentScale.y = contentScale.x;
					}
				}
				if( m_scaleToFitY )
				{
					contentScale.y = m_lastReshapeSize.y / contentSize.y;

					if( !m_scaleToFitX )		// Preserve uniform scaling.
					{
						contentScale.x = contentScale.y;
					}
				}
				
				m_content->scale( contentScale );
			}
			
			auto contentPosition = m_content->position();
			
			const auto& contentBounds = m_content->bounds().dimensions();
			contentPosition.x = clamp( contentPosition.x, -( contentBounds.x - m_lastReshapeSize.x ), 0.0f );
			contentPosition.y = clamp( contentPosition.y, -( contentBounds.y - m_lastReshapeSize.y ), 0.0f );
			m_content->position( contentPosition );
		}
	}

	void UIPanelContentHost::postLoad()
	{
		Super::postLoad();
	}
	
	FRESH_DEFINE_CALLBACK( UIPanelContentHost, onWheel, EventTouch )
	{
		if( m_content && ( m_allowScrollingX || m_allowScrollingY ))
		{
			vec2 adjustment( -event.wheelDelta() );
			
			if( !m_allowScrollingX )
			{
				adjustment.x = 0;
			}
			if( !m_allowScrollingY )
			{
				adjustment.y = 0;
			}			
			
			m_content->position( m_content->position() + adjustment );
			constrainContent();
		}
	}
	
	////////////////////////////////////////////////////////////////////////////////////
	
	FRESH_DEFINE_CLASS( UIPanel )
	DEFINE_VAR( UIPanel, DisplayObjectContainer::ptr, m_host );
	DEFINE_VAR( UIPanel, TextField::ptr, m_caption );
	DEFINE_VAR( UIPanel, DisplayObjectContainer::ptr, m_contentHostRoot );
	DEFINE_VAR( UIPanel, UIPanelContentHost::ptr, m_contentHost );
	DEFINE_VAR( UIPanel, SimpleButton::ptr, m_buttonExpandCollapse );
	DEFINE_METHOD( UIPanel, toggleCollapseExpand )
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( UIPanel )
	
	std::string UIPanel::captionText() const
	{
		if( m_caption )
		{
			return m_caption->text();
		}
		return "";
	}
	
	void UIPanel::captionText( const std::string& text )
	{
		if( m_caption )
		{
			m_caption->text( text );
		}
	}

	bool UIPanel::isExpanded() const
	{
		return false;
	}
	
	void UIPanel::collapse()
	{
		// TODO
	}
	
	void UIPanel::expand()
	{
		// TODO
	}

	void UIPanel::toggleCollapseExpand()
	{
		if( isExpanded() )
		{
			collapse();
		}
		else
		{
			expand();
		}
	}

	void UIPanel::postLoad()
	{
		Super::postLoad();
		
		if( m_host )
		{
			m_expandedFrame = m_frame;
			
			if( m_contentHost )
			{
				if( !hasDescendant( m_contentHost ))
				{
					if( m_contentHostRoot )
					{
						m_contentHostRoot->addChild( m_contentHost );
					}
					else
					{
						m_host->addChild( m_contentHost );
					}
				}
			}
			
			if( m_buttonExpandCollapse )
			{
				m_buttonExpandCollapse->addEventListener( SimpleButton::TAPPED, FRESH_CALLBACK( onButtonExpandCollapseTapped ));
			}
		}
	}
	
	FRESH_DEFINE_CALLBACK( UIPanel, onButtonExpandCollapseTapped, EventTouch )
	{
		toggleCollapseExpand();
	}
}

