/*
 *  Stage.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/24/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_STAGE_H_INCLUDED
#define FRESH_STAGE_H_INCLUDED

#include "DisplayObjectContainer.h"
#include "ObjectMethod.h"
#include "FreshTime.h"
#include "TimeServer.h"
#include "EventKeyboard.h"
#include "VirtualKeys.h"
#include "DisplayPackage.h"

namespace fr
{
	
	class Stage : public DisplayObjectContainer, public TimeServer
	{
		FRESH_DECLARE_CLASS( Stage, DisplayObjectContainer )
	public:
		
		explicit Stage( const ClassInfo& assignedClassInfo, const vec2& stageDimensions, NameRef objectName = DEFAULT_OBJECT_NAME );
		virtual ~Stage();

		// Inherited from TimeServer.
		// The number of simulated seconds (not corresponding directly to real time) since the stage was created.
		virtual TimeType time() const override
		{
			return clocksToSeconds( m_simulatedTime );
		}
		
		SYNTHESIZE( vec2, stageDimensions );
		rect stageBounds() const							{ return rect( m_stageDimensions * -0.5f, m_stageDimensions * 0.5f ); }
		real stageAspectRatio() const						{ return m_stageDimensions.x / m_stageDimensions.y; }
		real unitsPerScreenPixel() const;
		real unitsPerScreenInch() const;
		
		rect safeAreaInsets() const;
		
		float timeFloat() const							{ return static_cast< float >( time() ); }
		TimeType realTime() const;						// The number of simulated seconds (not corresponding directly to real time) since the stage was created.
		float realTimeFloat() const						{ return static_cast< float >( realTime() ); }
		TimeType frameRate() const;							// The assumed (not necessarily actual) number of frames per second.
		TimeType secondsPerFrame() const;
		SystemClock clocksPerFrame() const					{ return secondsToClocks( secondsPerFrame() ); }
		TimeType frameStartTimeReal() const					{ return clocksToSeconds( m_frameStartTimeReal ); }
		TimeType lastFrameDurationReal() const				{ return clocksToSeconds( m_lastFrameDurationReal ); }
		
		bool isUpdatePaused() const			{ return m_isUpdatePaused; }
		void setUpdatePaused( bool paused )	{ m_isUpdatePaused = paused; }
		void toggleUpdatePaused()			{ m_isUpdatePaused = !m_isUpdatePaused; }
		bool isRenderPaused() const			{ return m_isRenderPaused; }
		void setRenderPaused( bool paused )	{ m_isRenderPaused = paused; }
		void toggleRenderPaused()			{ m_isRenderPaused = !m_isRenderPaused; }

		SYNTHESIZE( DisplayPackage::ptr, stagePackage );
		
		virtual void onAppMayTerminate();
		virtual void onAppWaking();
		virtual void onAppSleeping();

		virtual void update() override;
		virtual void render( TimeType relativeFrameTime, RenderInjector* injector ) override;
		
		virtual void onTouchBegin( const EventTouch& event ) override;
		virtual void onTouchMove( const EventTouch& event ) override;
		virtual void onTouchEnd( const EventTouch& event ) override;
		virtual void onTouchCancelled( const EventTouch& event ) override;
		virtual void onWheelMove( const EventTouch& event ) override;
		
		virtual void onGainedFocus();
		virtual void onLostFocus();
		
		virtual void onWindowReshape( const Vector2i& newWindowDimensions );

		// Keyboard support
		//
		virtual void onKeyUp( const EventKeyboard& event );
		virtual void onKeyDown( const EventKeyboard& event );
		
		DisplayObject::ptr keyboardFocusHolder() const;
		virtual void takeKeyboardFocus( DisplayObject::ptr keyboardHandler );
		virtual void releaseKeyboardFocus( DisplayObject::ptr keyboardHandler );
		
		SYNTHESIZE( Color, clearColor );

		void fixupStageDimensions( const vec2& windowDimensions );
		// Either leaves the stage dimensions alone, or if one of the dimensions is currently 0,
		// sets that dimensions so that the stage has the same aspect ratio as the window.

		virtual void onStageLoaded();
		
		SYNTHESIZE_GET( bool, isPlaying );
		void beginPlay();
		void endPlay();

		virtual void setupTransforms( TimeType relativeFrameTime ) override;
		const mat4& transformMatrix() const						{ return m_cachedTransformMatrix; }
		
		SYNTHESIZE( bool, isRootOfRendering );
		
		SYNTHESIZE_GET( VirtualKeys::ptr, virtualKeys )
				
	protected:
		
		DVAR( bool, m_substepForRealTimeAdjustment, true );
		DVAR( bool, m_doShowTimingStatistics, false );
	
		virtual void updateStep();
		
		void computeStageDimensions( const vec2& windowDimensions );
		
		void traceSceneTree();
		static void traceSceneTreeRecursive( DisplayObject::ptr root, int depth = 0, int maxDepth = 0 );
		
		TimeType getProportionTimeThroughFrame() const;

		virtual void onTouchCommon( const EventTouch& event, const std::function< void( const EventTouch& ) >& superMethod );
		
	private:
		
		vec2 m_originalStageDimensions;
		real m_targetAspectRatio = 0;
		
		size_t m_nUpdates = 0;
		
		DVAR( vec2, m_stageDimensions, vec2( 1024.0f, 0 ));
		DVAR( TimeType, m_minFrameRateForRealTimeAdjustment, 10.0 );
		VAR( VirtualKeys::ptr, m_virtualKeys );
		VAR( DisplayObject::ptr, m_keyboardFocusHolder );
		
		
		DisplayPackage::ptr m_stagePackage;

		SystemClock m_timeAccumulator = 0;
		SystemClock m_simulatedTime = 0;
		
		SystemClock m_startTimeReal = 0;
		SystemClock m_frameStartTimeReal = 0;
		SystemClock m_lastFrameDurationReal = 0;
		
		bool m_isUpdatePaused = false;
		bool m_isRenderPaused = false;
		
		bool m_isRootOfRendering = true;
		
		bool m_isPlaying = false;
		
		mat4 m_cachedTransformMatrix = mat4::IDENTITY;

		DVAR( Color, m_clearColor, Color( 0.2f, 0.2f, 0.25f ));
		DVAR( bool, m_wantsClear, true );
		
		DECLARE_ACCESSOR( Stage, float, timeFloat );
		DECLARE_ACCESSOR( Stage, float, realTimeFloat );
	};
	
}

#endif
