//
//  UIPanel.h
//  Fresh
//
//  Created by Jeff Wofford on 6/7/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_UIPanel_h
#define Fresh_UIPanel_h

#include "UIFrame.h"
#include "TextField.h"
#include "SimpleButton.h"

namespace fr
{
	
	class UIPanelContentHost : public MovieClip
	{
		FRESH_DECLARE_CLASS( UIPanelContentHost, MovieClip );
	public:

		SYNTHESIZE_GET( DisplayObject::ptr, content );
		void content( DisplayObject::ptr newContent );
		
		virtual void reshape( const vec2& contentDimensions );
		
		virtual void onAddedToStage() override;
		virtual void postLoad() override;

		virtual void update() override;
		
	protected:
		
		void constrainContent();
		
	private:
		
		VAR( DisplayObject::ptr, m_content );
		DVAR( bool, m_allowScrollingX, true );
		DVAR( bool, m_allowScrollingY, true );
		DVAR( bool, m_scaleToFitX, false );
		DVAR( bool, m_scaleToFitY, false );

		Sprite::ptr m_touchCatcher;
		
		vec2 m_lastReshapeSize;
		
		FRESH_DECLARE_CALLBACK( UIPanelContentHost, onWheel, EventTouch );
	};

	////////////////////////////////////////////////////////////////////////////////////
	
	class UIPanel : public MovieClip
	{
		FRESH_DECLARE_CLASS( UIPanel, MovieClip );
	public:
		
		std::string captionText() const;
		void captionText( const std::string& text );
		
		bool isExpanded() const;
		virtual void collapse();
		virtual void expand();
		void toggleCollapseExpand();

		virtual void postLoad() override;
		
	private:
		
		VAR( DisplayObjectContainer::ptr, m_host );				// Everything goes in here. This is what gets animated by keyframes.
		VAR( TextField::ptr, m_caption );
		VAR( DisplayObjectContainer::ptr, m_contentHostRoot );		// The content host sits in here.
		VAR( UIPanelContentHost::ptr, m_contentHost );				// All the *content* sits in here.
		VAR( SimpleButton::ptr, m_buttonExpandCollapse );			// Tap this to expand or collapse the whole panel.
		
		rect m_expandedFrame;

		FRESH_DECLARE_CALLBACK( UIPanel, onButtonExpandCollapseTapped, EventTouch );
	};
	
}

#endif
