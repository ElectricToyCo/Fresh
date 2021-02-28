//
//  FreshWorld.h
//  Fresh
//
//  Created by Jeff Wofford on 12/3/14.
//  Copyright (c) 2014 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_World_h
#define Fresh_World_h

#include "DisplayObjectContainer.h"
#include "TimeServer.h"

namespace fr
{
	class Camera;
	class Lighting;
	
	class Attachment : public Object
	{
		FRESH_DECLARE_CLASS( Attachment, Object );
	public:
		
		SYNTHESIZE_GET( WeakPtr< DisplayObject >, attached );
		SYNTHESIZE_SET_DECL( WeakPtr< DisplayObject >, attached );
		
		SYNTHESIZE_GET( WeakPtr< DisplayObject >, to );
		SYNTHESIZE_SET_DECL( WeakPtr< DisplayObject >, to );

		SYNTHESIZE( vec2, offset );
		
		bool valid() const;
		virtual void update();
		
	private:
		
		VAR( WeakPtr< DisplayObject >, m_attached );
		VAR( WeakPtr< DisplayObject >, m_to );
		VAR( vec2, m_offset );
	};

	///////////////////////////////////////////////////////////////////////////
	
	class FreshActor;
	
	class FreshWorld : public DisplayObjectContainer, public TimeServer
	{
		FRESH_DECLARE_CLASS( FreshWorld, DisplayObjectContainer );
	public:
		
		SYNTHESIZE_GET( SmartPtr< Camera >, camera );
		
		virtual void update() override;
		
		virtual void attach( DisplayObject& attached, DisplayObject& to, const vec2& offset = vec2::ZERO );
		virtual vec2 detach( DisplayObject& attached, DisplayObject& to );
		
		SmartPtr< Lighting > lighting() const;
		
		virtual TimeType time() const override;
		
	protected:
		
		SYNTHESIZE_SET_DECL( SmartPtr< Camera >, camera );
		virtual void updateActors( const std::vector< SmartPtr< FreshActor > >& actors );
		virtual void updateAttachments();
		virtual void updateCamera();
		
		virtual void resolveCollisions( const std::vector< SmartPtr< FreshActor > >& actors );
		
	private:
		
		DVAR( vec2, m_gravity, vec2( 0, 200 ));
		VAR( SmartPtr< Camera >, m_camera );

		VAR( std::vector< Attachment::ptr >, m_attachments );
	};
	
}

#endif
