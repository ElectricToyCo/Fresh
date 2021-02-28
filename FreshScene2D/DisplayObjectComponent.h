//
//  DisplayObjectComponent.h
//  Fresh
//
//  Created by Jeff Wofford on 7/3/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_DisplayObjectComponent_h
#define Fresh_DisplayObjectComponent_h

#include "EventDispatcher.h"
#include "EventTouch.h"

namespace fr
{
	class DisplayObject;
	
	class DisplayObjectComponent : public EventDispatcher
	{
		FRESH_DECLARE_CLASS( DisplayObjectComponent, EventDispatcher );
	public:
		
		virtual bool onAddedToHost( DisplayObject& host ) { return false; }
		virtual bool onRemovingFromHost( DisplayObject& host ) { return false; }
		
		virtual bool preUpdate( DisplayObject& host ) { return false; }
		virtual bool postUpdate( DisplayObject& host ) { return false; }

		virtual bool preRender( DisplayObject& host, TimeType relativeFrameTime ) { return false; }
		virtual bool render( DisplayObject& host, TimeType relativeFrameTime ) { return false; }

		virtual bool onTouchBegin( DisplayObject& host, const EventTouch& event ) { return false; }
		virtual bool onTouchMove( DisplayObject& host, const EventTouch& event ) { return false; }
		virtual bool onTouchEnd( DisplayObject& host, const EventTouch& event ) { return false; }
		virtual bool onTouchCancelled( DisplayObject& host, const EventTouch& event ) { return false; }
		virtual bool onWheelMove( DisplayObject& host, const EventTouch& event ) { return false; }

		virtual bool onBeginPlay( DisplayObject& host ) { return false; }
		virtual bool onEndPlay( DisplayObject& host ) { return false; }

		virtual bool onAddedToStage( DisplayObject& host ) { return false; }
		virtual bool onAddedToParent( DisplayObject& host ) { return false; }
		virtual bool onRemovingFromParent( DisplayObject& host ) { return false; }
		
	};
	
}

#endif
