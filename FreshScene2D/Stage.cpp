/*
 *  Stage.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/24/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#include "Stage.h"
#include "Application.h"
#include "Renderer.h"
#include "Objects.h"
#include "Profiler.h"
#include "CommandProcessor.h"

namespace fr
{
	FRESH_DEFINE_CLASS_UNPLACEABLE( Stage )

	DEFINE_VAR( Stage, vec2, m_stageDimensions );
	DEFINE_VAR( Stage, bool, m_substepForRealTimeAdjustment );
	DEFINE_VAR( Stage, bool, m_doShowTimingStatistics );
	DEFINE_VAR( Stage, TimeType, m_minFrameRateForRealTimeAdjustment );
	DEFINE_VAR( Stage, VirtualKeys::ptr, m_virtualKeys );
	DEFINE_VAR( Stage, DisplayObject::ptr, m_keyboardFocusHolder );
	DEFINE_VAR( Stage, Color, m_clearColor );
	DEFINE_VAR( Stage, bool, m_wantsClear );
	
	DEFINE_ACCESSOR( Stage, float, timeFloat );
	DEFINE_ACCESSOR( Stage, float, realTimeFloat );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( Stage )
	
	Stage::Stage( const ClassInfo& assignedClassInfo, NameRef objectName )
	:	Stage( assignedClassInfo, vec2( 1024.0f, 0.0f ), objectName )
	{}

	Stage::Stage( const ClassInfo& assignedClassInfo, const vec2& stageDimensions, NameRef objectName /* = DEFAULT_OBJECT_NAME */ )
	:	DisplayObjectContainer( assignedClassInfo, objectName )
	,	TimeServer( createObject< CallbackScheduler >( &getTransientPackage(), CallbackScheduler::StaticGetClassInfo() ))
	,	m_stageDimensions( stageDimensions )
	,	m_startTimeReal( getAbsoluteTimeClocks() )
	{
		frame( rect( m_stageDimensions * -0.5f, m_stageDimensions * 0.5f ));
		
		// I am my own stage. This allows my children to understand that I "have" a stage, and that they can copy it
		// just the way all children do their parents' stages.
		//
		propagateStage( this );
		
		// Create the dlist command.
		//
		{
			auto caller = stream_function< void() >( std::bind( &Stage::traceSceneTree, this ) );
			auto command = CommandProcessor::instance().registerCommand( this, "dlist", "displays the scene graph for the current stage.", caller );
		}
	}
	
	Stage::~Stage()
	{
		onEndPlay();
		if( CommandProcessor::doesExist() )
		{
			CommandProcessor::instance().unregisterAllCommandsForHost( this );
		}
	}

	real Stage::unitsPerScreenPixel() const
	{
		return m_stageDimensions.x / Application::instance().getWindowDimensions().x;
	}
	
	real Stage::unitsPerScreenInch() const
	{
		return unitsPerScreenPixel() * Application::instance().pixelsPerScreenInch();
	}
	
	rect Stage::safeAreaInsets() const
	{
		const auto insets = Application::instance().safeAreaInsets();
		const auto screenToStageScale = unitsPerScreenPixel();
		return {
			insets.left() * screenToStageScale,
			insets.top() * screenToStageScale,
			insets.right() * screenToStageScale,
			insets.bottom() * screenToStageScale,
		};
	}
	
	TimeType Stage::realTime() const
	{
		return clocksToSeconds( getAbsoluteTimeClocks() - m_startTimeReal );
	}

	TimeType Stage::frameRate() const
	{
		const auto frameRate = Application::instance().desiredFramesPerSecond();
		PROMISES( frameRate > 0 );
		return frameRate;
	}

	TimeType Stage::secondsPerFrame() const
	{
		ASSERT( frameRate() > 0 );
		return 1.0 / frameRate();
	}

	void Stage::onAppMayTerminate()
	{}
	
	void Stage::onAppWaking()
	{}
	
	void Stage::onAppSleeping()
	{}
	
	void Stage::update()
	{
		if( !doUpdate() ) return;
		
		TIMER_AUTO_FUNC
		
		try
		{
			TIMER_AUTO( Stage::update )
			
			if( !m_isUpdatePaused )
			{
#if DEV_MODE && 0
				
				// Force testing of random timing problems.
				//
				accurateSleep( randInRange( SystemClock( 0 ), getClocksPerFrame() / 2 ));
#endif
				
				const SystemClock now = getAbsoluteTimeClocks();
				
				if( m_substepForRealTimeAdjustment )
				{			
					// Update until we have exhausted the amount of alloted time for the frame.
					//
					if( m_frameStartTimeReal == 0 )
					{
						m_frameStartTimeReal = now;
					}
					
					SystemClock deltaTimeReal = m_lastFrameDurationReal = now - m_frameStartTimeReal;

					// Prevent excessive time being spent looping over updates. If the frame rate < 10fps, we'll just have to suck it up.
					//
					static SystemClock maxClocksPerFrame = m_minFrameRateForRealTimeAdjustment > 0 ? secondsToClocks( 1.0 / m_minFrameRateForRealTimeAdjustment ) : 0;
					if( maxClocksPerFrame > 0 && deltaTimeReal > maxClocksPerFrame )
					{
						
#if DEV_MODE && 1
						trace( "STAGE TIMING: Clamping delta time to upper limit (deltaTimeReal=" << clocksToSeconds( deltaTimeReal ) << "s)." );
#endif
						
						deltaTimeReal = maxClocksPerFrame;
					}
					
#if DEV_MODE && 1
					auto initialAccumulator = m_timeAccumulator;
#endif
					
					m_timeAccumulator += deltaTimeReal;
					
					const SystemClock clocksToSpend = clocksPerFrame();
					
					int nStepsTaken = 0;
					while( m_timeAccumulator >= clocksToSpend )
					{
						// Record the prior state for all DisplayObjects before changing them.
						//
						recordPreviousState( true );

						updateStep();
						++nStepsTaken;
						m_timeAccumulator -= clocksToSpend;
					}
									
#if DEV_MODE && 1
					
					if( m_doShowTimingStatistics )
					{
						// Report timing statistics.
						//
						const TimeType realSeconds = clocksToSeconds( now - m_startTimeReal );
						release_trace( "STAGE TIMING: Real: " << std::setw( 10 ) << realSeconds
								<< " Game: " << std::setw( 10 ) << time()
								<< " Diff: " << std::setw( 10 ) << ( time() - realSeconds ) << " (" << std::setw( 5 ) << std::setprecision( 1 ) << ( time() / realSeconds * 100.0 ) << "%)."
								<< " deltaReal: " << std::setw( 10 ) << m_lastFrameDurationReal
								<< " deltaReal (clamped): " << std::setw( 10 ) << deltaTimeReal
								<< " steps: " << std::setw( 10 ) << nStepsTaken
								<< " now (clocks): " << std::setw( 10 ) << now
								<< " initial accum: " << std::setw( 10 ) << initialAccumulator
								<< " resulting accum: " << std::setw( 10 ) << m_timeAccumulator
								<< " proportion: " << std::setw( 5 ) << std::setprecision( 1 ) << getProportionTimeThroughFrame()
						);
					}
#endif
					
				}
				else
				{
					updateStep();
				}
				
				m_frameStartTimeReal = now;
			}
		}
		catch( const std::exception& e )
		{
			con_error( "std::exception with message '" << e.what() << "'." );
		}
		catch( ... )
		{
			con_error( "Unknown exception." );
		}
	}

	TimeType Stage::getProportionTimeThroughFrame() const
	{
		if( m_substepForRealTimeAdjustment )
		{
			return
			(( m_nUpdates > 1 ) && m_timeAccumulator <= clocksPerFrame() ) ?
			( static_cast< TimeType >( m_timeAccumulator ) / clocksPerFrame()  ) :
			1.0;
		}
		else
		{
			return 1.0;
		}
	}

	void Stage::updateStep()
	{
		// Update everything.
		//
		++m_nUpdates;
		m_simulatedTime += clocksPerFrame();
		
		updateScheduledCallbacks();
		
		DisplayObjectContainer::update();				
	}

	void Stage::traceSceneTree()
	{
		traceSceneTreeRecursive( this );
	}
	
	void Stage::traceSceneTreeRecursive( DisplayObject::ptr root, int depth, int maxDepth )
	{
		std::ostringstream stream;
		
		if( root && ( maxDepth == 0 || depth < maxDepth ))
		{
			int indents = depth;
			while( indents-- > 0 ) stream << "\t";
			
			stream << root->toString();
			
			trace( stream.str() );
			
			// Recurse.
			//
			DisplayObjectContainer::ptr container = dynamic_freshptr_cast< DisplayObjectContainer::ptr >( root );
			
			if( container )
			{
				for( size_t i = 0; i < container->numChildren(); ++i )
				{
					traceSceneTreeRecursive( container->getChildAt( i ), depth + 1, maxDepth);
				}
			}
		}
	}

	void Stage::render( TimeType /* ignored */, RenderInjector* injector )
	{
		if( !m_isRenderPaused )
		{
			// Figure out how far along between the last frame and the current frame we want to render,
			// based on the amount of time between the last real time frame start and the end of the frame.
			//
			const TimeType proportionTimeThroughFrame = getProportionTimeThroughFrame();

			Renderer& renderer = Renderer::instance();
			
			if( m_isRootOfRendering )
			{
				// Setup the base transformations.
				//
				const vec2 projectionDimensions( m_stageDimensions * 0.5f );
				
				renderer.setMatrixToIdentity( Renderer::MAT_Projection );
				renderer.setOrthoProjection( -projectionDimensions.x, projectionDimensions.x, -projectionDimensions.y, projectionDimensions.y, -1000.0f, 1000.0f );
				
				// TODO clear through render target
				if( m_wantsClear )
				{
					renderer.clearColor( m_clearColor );
					renderer.clear();
				}
			}
			
			// Render all objects at this tweened position.
			//
			DisplayObjectContainer::preRender( proportionTimeThroughFrame );
			DisplayObjectContainer::render( proportionTimeThroughFrame, injector );
		}
	}

	void Stage::onTouchBegin( const EventTouch& event )
	{
		TIMER_AUTO( Stage::onTouchBegin )
		
		// Release the keyboard focus if the current focus object isn't under this touch.
		//
		if( m_keyboardFocusHolder )
		{
			DisplayObjectContainer::ptr targetAsContainer = dynamic_freshptr_cast< DisplayObjectContainer::ptr >( event.target() );
			
			if( event.target() != m_keyboardFocusHolder && ( !targetAsContainer || !targetAsContainer->hasDescendant( m_keyboardFocusHolder )))
			{
				releaseKeyboardFocus( m_keyboardFocusHolder );
			}
		}
		   
		onTouchCommon( event, [this]( const EventTouch& aEvent ){ Super::onTouchBegin( aEvent ); } );
	}

	void Stage::onTouchMove( const EventTouch& event )
	{
		TIMER_AUTO( Stage::onTouchMove )
		onTouchCommon( event, [this]( const EventTouch& aEvent ){ Super::onTouchMove( aEvent ); } );
	}

	void Stage::onTouchEnd( const EventTouch& event )
	{
		TIMER_AUTO( Stage::onTouchEnd )
		onTouchCommon( event, [this]( const EventTouch& aEvent ){ Super::onTouchEnd( aEvent ); } );
	}
	
	void Stage::onTouchCancelled( const EventTouch& event )
	{
		TIMER_AUTO( Stage::onTouchCancelled )
		onTouchCommon( event, [this]( const EventTouch& aEvent ){ Super::onTouchCancelled( aEvent ); } );
	}

	void Stage::onWheelMove( const EventTouch& event )
	{
		TIMER_AUTO( Stage::onWheelMove )
		onTouchCommon( event, [this]( const EventTouch& aEvent ){ Super::onWheelMove( aEvent ); } );
	}
	
	DisplayObject::ptr Stage::keyboardFocusHolder() const
	{
		return m_keyboardFocusHolder;
	}
	
	void Stage::takeKeyboardFocus( DisplayObject::ptr keyboardHandler )
	{
		if( m_keyboardFocusHolder )
		{
			releaseKeyboardFocus( m_keyboardFocusHolder );
		}
		
		m_keyboardFocusHolder = keyboardHandler;
		keyboardHandler->onGainedKeyboardFocus();
	}
	
	void Stage::releaseKeyboardFocus( DisplayObject::ptr keyboardHandler )
	{
		if( m_keyboardFocusHolder == keyboardHandler )
		{
			m_keyboardFocusHolder = nullptr;
			keyboardHandler->onLostKeyboardFocus();
		}
	}

	void Stage::onGainedFocus()
	{
		if( m_virtualKeys )
		{
			m_virtualKeys->clearKeysDown();
		}
	}

	void Stage::onLostFocus()
	{}

	void Stage::onWindowReshape( const Vector2i& newWindowDimensions )
	{
		computeStageDimensions( vector_cast< real >( newWindowDimensions ));
	}
	
	void Stage::onKeyUp( const EventKeyboard& event )
	{
		if( m_virtualKeys )
		{
			m_virtualKeys->onKeyUp( event );
		}
		
		dispatchEvent( &event );
	}
	
	void Stage::onKeyDown( const EventKeyboard& event )
	{
		if( m_virtualKeys )
		{
			m_virtualKeys->onKeyDown( event );
		}
		
		dispatchEvent( &event );
	}

	void Stage::fixupStageDimensions( const vec2& windowDimensions )
	{
		m_originalStageDimensions = m_stageDimensions;
		
		if( m_targetAspectRatio <= 0 && m_originalStageDimensions.x > 0 && m_originalStageDimensions.y > 0 )
		{
			m_targetAspectRatio = m_originalStageDimensions.x / m_originalStageDimensions.y;
		}
		
		computeStageDimensions( windowDimensions );
	}
	
	void Stage::computeStageDimensions( const vec2& windowDimensions )
	{
		m_stageDimensions = m_originalStageDimensions;

		ASSERT( windowDimensions.y > 0 );
		
		const float aspectRatio = windowDimensions.x / windowDimensions.y;
		ASSERT( aspectRatio > 0 );
		
		// If one of the stageDimensions is <= 0,
		// we set it based on the other dimension's value and the current window aspect ratio.
		// m_targetAspectRatio is ignored.
		//
		if( m_stageDimensions.x <= 0 || m_stageDimensions.y <= 0 )
		{
			// What's the aspect ratio for the window?
			//
			// If both stage dimensions are ridiculous, make them the same as the window.
			if( m_stageDimensions.x <= 0 && m_stageDimensions.y <= 0 )
			{
				m_stageDimensions = windowDimensions;
			}
			else
			{
				if( m_stageDimensions.x <= 0 )
				{
					m_stageDimensions.x = m_stageDimensions.y * aspectRatio;
				}
				else
				{
					ASSERT( m_stageDimensions.y <= 0 );
					m_stageDimensions.y = m_stageDimensions.x / aspectRatio;
				}
			}
			
#if DEV_MODE
			trace( "Stage resolution " << m_stageDimensions << " for window of size " << windowDimensions << " (aspect " << aspectRatio << ")." );
#endif
		}
		else
		{
			ASSERT( m_targetAspectRatio > 0 );
			
			int fixedAxis = 0;
			real adjustmentAxisScalar = 1.0f / aspectRatio;
			
			if( aspectRatio > m_targetAspectRatio )
			{
				fixedAxis = 1;
				adjustmentAxisScalar = aspectRatio;
			}
			
			const int adjustedAxis = ( fixedAxis + 1 ) & 1;
			
			m_stageDimensions[ adjustedAxis ] = m_stageDimensions[ fixedAxis ] * adjustmentAxisScalar;
		}
		
		// Update the stage frame.
		//
		frame( rect( m_stageDimensions * -0.5f, m_stageDimensions * 0.5f ));
		
		ASSERT( m_stageDimensions.x > 0 && m_stageDimensions.y > 0 );
	}
	
	void Stage::onStageLoaded()
	{
		frame( rect( m_stageDimensions * -0.5f, m_stageDimensions * 0.5f ));

		// Notify the whole tree that they've been added to their parent.
		//
		propagateParentNotification();
		
		// Send out the identity of this stage to all descendants.
		//
		propagateStage( this );
		
		// Notify all descendants that the tree is now complete.
		//
		propagateStageNotification();
		
		ASSERT( verifyTree() );
		
		if( m_keyboardFocusHolder )
		{
			takeKeyboardFocus( m_keyboardFocusHolder );
		}
	}
	
	void Stage::beginPlay()
	{
		onBeginPlay();
		m_isPlaying = true;
	}
	
	void Stage::endPlay()
	{
		m_isPlaying = false;
		onEndPlay();
	}
	
	void Stage::onTouchCommon( const EventTouch& event, const std::function< void( const EventTouch& ) >& superMethod )
	{
		if( !isTouchEnabled() ) return;
		
		vec2 transformedLocation = Renderer::instance().screenToWorld2D( event.location() );
		
		// What is the target of this event?
		//
		DisplayObject::ptr target = getTopDescendantUnderPoint( transformedLocation );
		
		if( !target )
		{
			target = this;
		}
		
		EventTouch transformedTouch( event, target, transformedLocation, Renderer::instance().screenToWorld2D( event.previousLocation() ));
		
		superMethod( transformedTouch );
	}
	
	void Stage::setupTransforms( TimeType relativeFrameTime )
	{
		Super::setupTransforms( relativeFrameTime );
		m_cachedTransformMatrix = Renderer::instance().getModelViewMatrix();
	}	
}
