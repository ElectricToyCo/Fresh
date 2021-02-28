/*
 *  DisplayObject.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/18/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#include "DisplayObject.h"
#include "DisplayObjectContainer.h"
#include "Objects.h"
#include "Renderer.h"
#include "DisplayFilter.h"
#include "FreshVector.h"
#include "Stage.h"
#include "Graphics.h"
#include "Profiler.h"
#include "CommandProcessor.h"

#if DEV_MODE && 0
#	define trace_drag( expr ) release_trace( expr )
#else
#	define trace_drag( expr )
#endif

namespace
{
	fr::ShaderProgram::wptr g_stockShaderProgramTextured;
	fr::ShaderProgram::wptr g_stockShaderProgramUntextured;	
}

namespace fr
{
	const char* DisplayObject::TAPPED = "Tapped";
	const char* DisplayObject::DRAG_BEGIN = "DragBegin";
	const char* DisplayObject::DRAG_MOVE = "DragMove";
	const char* DisplayObject::DRAG_END = "DragEnd";

	bool DisplayObject::RenderInjector::preDraw( TimeType relativeFrameTime, DisplayObject& object )
	{
		bool stompOuter = preDrawOverride( relativeFrameTime, object );
		
		if( m_next )
		{
			return m_next->preDraw( relativeFrameTime, object ) || stompOuter;
		}
		else
		{
			return stompOuter;
		}
	}

	bool DisplayObject::RenderInjector::draw( TimeType relativeFrameTime, DisplayObject& object )
	{
		bool stompOuter = drawOverride( relativeFrameTime, object );
		
		if( m_next )
		{
			return m_next->draw( relativeFrameTime, object ) || stompOuter;
		}
		else
		{
			return stompOuter;
		}
	}
	
	bool DisplayObject::RenderInjector::postDraw(  TimeType relativeFrameTime, DisplayObject& object )
	{
		bool stompOuter = postDrawOverride( relativeFrameTime, object );
		
		if( m_next )
		{
			return m_next->postDraw( relativeFrameTime, object ) || stompOuter;
		}
		else
		{
			return stompOuter;
		}
	}


	//////////////////////////////////
	
	FRESH_DEFINE_CLASS_PLACEABLE( DisplayObject )

	DEFINE_VAR( DisplayObject, vec2, m_position );
	DEFINE_VAR( DisplayObject, angle, m_rotation );
	DEFINE_VAR( DisplayObject, vec2, m_scale );
	DEFINE_VAR( DisplayObject, vec2, m_pivot );
	DEFINE_VAR( DisplayObject, vec2, m_parentFrameAttachPoint );
	DEFINE_VAR( DisplayObject, size_t, m_lifespan );
	DEFINE_VAR( DisplayObject, real, m_positionQuantum );
	DEFINE_VAR( DisplayObject, rect, m_frame );
	DEFINE_VAR( DisplayObject, bool, m_inheritParentFrame );
	DEFINE_VAR_FLAG( DisplayObject, size_t, m_nUpdates, PropFlag::NoEdit );
	DEFINE_VAR( DisplayObject, Color, m_color );
	DEFINE_VAR( DisplayObject, Color, m_colorAdditive );
	DEFINE_VAR( DisplayObject, vec2, m_moveWithDragAxisScalars );
	DEFINE_VAR( DisplayObject, bool, m_isTouchEnabled );
	DEFINE_VAR( DisplayObject, bool, m_isDragEnabled );
	DEFINE_VAR( DisplayObject, bool, m_doMoveWithDrag );
	DEFINE_VAR_FLAG( DisplayObject, bool, m_isMarkedForDeletion, PropFlag::NoEdit );
	DEFINE_VAR( DisplayObject, bool, m_visible );
	DEFINE_VAR_FLAG( DisplayObject, bool, m_isKeyframeVisible, PropFlag::NoEdit );
	DEFINE_VAR( DisplayObject, bool, m_wantsUpdate );
	DEFINE_VAR( DisplayObject, bool, m_ignoreFrameAttachment );
	DEFINE_VAR( DisplayObject, Renderer::BlendMode, m_blendMode );
	DEFINE_VAR( DisplayObject, Renderer::StencilMode, m_maskStencilReadMode );
	DEFINE_VAR_FLAG( DisplayObject, fr::SmartPtr< ShaderProgram >, m_shaderProgram, PropFlag::LoadDefault );
	DEFINE_VAR( DisplayObject, DisplayObject::ptr, m_mask );
	DEFINE_VAR( DisplayObject, std::vector< SmartPtr< DisplayObjectComponent >>, m_components );
	DEFINE_VAR( DisplayObject, RenderTarget::ptr, m_renderTarget );
	DEFINE_VAR( DisplayObject, RenderTarget::ptr, m_filteringRenderTarget );
	DEFINE_VAR( DisplayObject, std::vector< SmartPtr< DisplayFilter >>, m_filters );
	DEFINE_VAR( DisplayObject, real, m_minStageDistanceForDrag );
	DEFINE_VAR( DisplayObject, TimeType, m_minRealDurationForDrag );
	DEFINE_VAR_FLAG( DisplayObject, fr::WeakPtr< DisplayObjectContainer >, m_parent, PropFlag::NoEdit );
	DEFINE_VAR_FLAG( DisplayObject, fr::WeakPtr< Stage >, m_stage, PropFlag::NoEdit );
	DEFINE_VAR( DisplayObject, bool, m_isEditingEnabled );
	DEFINE_VAR_FLAG( DisplayObject, bool, m_editorLocked, PropFlag::NoEdit );
#if DEV_MODE
	DEFINE_VAR_FLAG( DisplayObject, bool, m_debugBreakRendering, PropFlag::Transient );
#endif
	
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( DisplayObject )
	
	DisplayObject::DisplayObject( const ClassInfo& assignedClassInfo, NameRef objectName /* = DEFAULT_OBJECT_NAME */ )
	:	EventDispatcher( assignedClassInfo, objectName )
	,	m_previousState( DisplayObjectState::create( *this ))
	{
		doctorClass< DisplayObject >( [&] ( ClassInfo& classInfo, DisplayObject& defaultObject )
									 {
										 defaultObject.shaderProgram( getStockShaderProgram( true ) );
									 } );
	}

	DisplayObject::~DisplayObject()
	{
		onEndPlay();
	}
	
	void DisplayObject::mask( DisplayObject::ptr mask )
	{
		REQUIRES( mask != this );
		
		if( m_mask )
		{
			m_mask->propagateStage( nullptr );
		}
		
		m_mask = mask;
		
		if( m_mask && hasStage() )
		{
			m_mask->propagateStage( m_stage );
			m_mask->propagateStageNotification();
		}
	}
	
	bool DisplayObject::hasStage() const
	{
		return (bool) m_stage;
	}
	
	Stage& DisplayObject::stage() const
	{
		ASSERT( hasStage() );
		return *m_stage;
	}
	
	WeakPtr< DisplayObjectContainer > DisplayObject::parent() const
	{
		return m_parent;
	}

	bool DisplayObject::hasAncestor( DisplayObjectContainer::cwptr container, int maxDepth /* = -1 */ ) const
	{
		REQUIRES( container );
		
		if( maxDepth == 0 )
		{
			return false;
		}
		
		if( !m_parent )
		{
			return false;
		}
		
		if( container == m_parent )
		{
			return true;
		}
		else
		{
			return m_parent->hasAncestor( container, maxDepth - 1 );
		}
	}

	void DisplayObject::markForDeletion()							
	{
		ASSERT( !m_isMarkedForDeletion ); 
		m_parent = nullptr;	// Necessary here because e.g. DisplayObjectContainer::removeMarkedChildren() doesn't bother telling children they're being removed.
		m_stage = nullptr;
		m_isMarkedForDeletion = true; 
	}

	vec2 DisplayObject::localToGlobal( const vec2& location ) const
	{
		if( !m_parent )
		{
			return location;
		}
		else
		{
			return m_parent->localToGlobal( localToParent( location ));
		}
	}

	vec2 DisplayObject::globalToLocal( const vec2& location ) const
	{
		if( !m_parent )
		{
			return location;
		}
		else
		{
			return parentToLocal( m_parent->globalToLocal( location ));
		}
	}

	vec2 DisplayObject::properParentAttachOffset() const
	{
		REQUIRES( parent() );
		const auto& parentFrame = parent()->frame();
		
		return vec2( lerp( parentFrame.left(), parentFrame.right(),  ( 1.0f + m_parentFrameAttachPoint.x ) * 0.5f ),
					 lerp( parentFrame.top(),  parentFrame.bottom(), ( 1.0f + m_parentFrameAttachPoint.y ) * 0.5f ));
	}

	vec2 DisplayObject::localToParent( const vec2& location ) const
	{
		vec2 parentLocation = location - m_pivot;
		parentLocation *= m_scale;
		parentLocation.rotate( m_rotation );
		
		vec2 attachPoint;
		if( parent() )
		{
			attachPoint = parentAttachOffset();
		}
		
		return parentLocation + m_position + attachPoint;
	}

	vec2 DisplayObject::parentToLocal( const vec2& location ) const
	{
		vec2 attachPoint;
		if( parent() )
		{
			attachPoint = parentAttachOffset();
		}	
		
		vec2 localLocation = location - ( m_position + attachPoint );
		localLocation.rotate( -m_rotation );
		localLocation /= m_scale;
		return localLocation + m_pivot;
	}

	rect DisplayObject::localToGlobal( const rect& rectangle ) const
	{
		if( !m_parent )
		{
			return rectangle;
		}
		else
		{
			return m_parent->localToGlobal( localToParent( rectangle ));
		}
	}

	rect DisplayObject::globalToLocal( const rect& rectangle ) const
	{
		if( !m_parent )
		{
			return rectangle;
		}
		else
		{
			return parentToLocal( m_parent->globalToLocal( rectangle ));
		}	
	}

	rect DisplayObject::localToParent( const rect& rectangle ) const
	{
		if( rectangle.isInverseInfiniteWidth() || rectangle.isInverseInfiniteHeight() )
		{
			return rectangle;
		}
		
		vec2 corners[ 4 ];
		
		corners[ 0 ] = rectangle.ulCorner();
		corners[ 1 ] = rectangle.urCorner();
		corners[ 2 ] = rectangle.brCorner();
		corners[ 3 ] = rectangle.blCorner();
		
		rect rect;
		rect.setToInverseInfinity();
		
		for( int i = 0; i < 4; ++i )
		{
			rect.growToEncompass( localToParent( corners[ i ] ));
		}
		
		return rect;
	}

	rect DisplayObject::parentToLocal( const rect& rectangle ) const
	{
		if( rectangle.isInverseInfiniteWidth() || rectangle.isInverseInfiniteHeight() )
		{
			return rectangle;
		}
		
		vec2 corners[ 4 ];
		
		corners[ 0 ] = rectangle.ulCorner();
		corners[ 1 ] = rectangle.urCorner();
		corners[ 2 ] = rectangle.brCorner();
		corners[ 3 ] = rectangle.blCorner();
		
		rect rect;
		rect.setToInverseInfinity();
		
		for( int i = 0; i < 4; ++i )
		{
			rect.growToEncompass( parentToLocal( corners[ i ] ));
		}
		
		return rect;
	}

	angle DisplayObject::localToGlobal( angle rotation ) const
	{
		if( !m_parent )
		{
			return rotation;
		}
		else
		{
			return m_parent->localToGlobal( localToParent( rotation ));
		}
	}

	angle DisplayObject::globalToLocal( angle rotation ) const
	{
		if( !m_parent )
		{
			return rotation;
		}
		else
		{
			return parentToLocal( m_parent->globalToLocal( rotation ));
		}
	}

	angle DisplayObject::localToParent( angle rotation ) const
	{
		return m_rotation + rotation;
	}

	angle DisplayObject::parentToLocal( angle rotation ) const
	{
		return rotation - m_rotation;
	}

	vec2 DisplayObject::localToGlobalPoint( const vec2& point ) const
	{
		if( !m_parent )
		{
			return point;
		}
		else
		{
			return m_parent->localToGlobalPoint( localToParentPoint( point ));
		}
	}
	
	vec2 DisplayObject::globalToLocalPoint( const vec2& point ) const
	{
		if( !m_parent )
		{
			return point;
		}
		else
		{
			return parentToLocalPoint( m_parent->globalToLocalPoint( point ));
		}
	}
	
	vec2 DisplayObject::localToParentPoint( const vec2& point ) const
	{
		vec2 parentPoint = m_scale * point;
		parentPoint.rotate( m_rotation );
		return parentPoint;
	}
	
	vec2 DisplayObject::parentToLocalPoint( const vec2& point ) const
	{
		vec2 parentPoint( point );
		parentPoint.rotate( -m_rotation );
		parentPoint /= m_scale;
		return parentPoint;
	}

	vec2 DisplayObject::localToGlobalScale( const vec2& scale ) const
	{
		if( !m_parent )
		{
			return scale;
		}
		else
		{
			return m_parent->localToGlobalScale( localToParentScale( scale ));
		}
	}
	
	vec2 DisplayObject::globalToLocalScale( const vec2& scale ) const
	{
		if( !m_parent )
		{
			return scale;
		}
		else
		{
			return parentToLocalScale( m_parent->globalToLocalScale( scale ));
		}
	}
	
	vec2 DisplayObject::localToParentScale( const vec2& scale ) const
	{
		return m_scale * scale;
	}
	
	vec2 DisplayObject::parentToLocalScale( const vec2& scale ) const
	{
		return scale / m_scale;
	}

	void DisplayObject::recordPreviousState( bool recursive )
	{
		m_hasMeaningfulPreviousState = true;
		m_previousState.setStateFrom( *this );
		
		if( recursive && m_mask )		// Recurse to the mask as if it were a child.
		{
			m_mask->recordPreviousState( recursive );
		}		
	}
	
	bool DisplayObject::doUpdate() const
	{
		return m_wantsUpdate && !isMarkedForDeletion() && hasStage();
	}
	
	void DisplayObject::update()
	{
		TIMER_AUTO( DisplayObject::update )
		
		// Update rough measure of lifespan.
		//
		++m_nUpdates;
		
		// Look for dragging based on time.
		//
		if( couldStartDrag() )
		{
			const auto dragTime = ( getAbsoluteTimeSeconds() - m_touchStartTime );
			
			if( m_minRealDurationForDrag > 0 && dragTime >= m_minRealDurationForDrag )
			{
				trace_drag( this << " started dragging due to time " << dragTime );
				
				// Build a touch event that simulates us just having moved the touch.
				EventTouch event( EventTouch::TOUCH_MOVE,
								 m_potentialDragLastTouchId,
								 EventTouch::TouchPhase::Move,
								 0,
								 m_potentialDragLastLocation,
								 m_potentialDragLastPreviousLocation,
								 1,
								 0,
								 vec2::ZERO,
								 this,
								 this );
				startDrag( event );
			}
		}
		
		forEachComponent( std::bind( &DisplayObjectComponent::preUpdate, std::placeholders::_1, std::ref( *this )));
		
		// Is it time to die?
		//
		if( m_lifespan > 0 && m_nUpdates >= m_lifespan )
		{
			// Yes.
			
			// Allow subclasses to do other life-terminating things.
			//
			onLifespanCompleted();
			
			// Kill self.
			//
			if( !isMarkedForDeletion() )
			{
				markForDeletion();
			}

			return;	// Abort following code in this function (if any).
		}
		
		// If we have a mask, update it--much as if it were a child.
		//
		if( m_mask )
		{
			m_mask->update();
		}
		
		forEachComponent( std::bind( &DisplayObjectComponent::postUpdate, std::placeholders::_1, std::ref( *this )));
	}

	void DisplayObject::preRender( TimeType relativeFrameTime )
	{
		forEachComponent( std::bind( &DisplayObjectComponent::preRender, std::placeholders::_1, std::ref( *this ), relativeFrameTime ));
	}

	void DisplayObject::render( TimeType relativeFrameTime, RenderInjector* injector )
	{
		TIMER_AUTO( DisplayObject::render )

		checkDebugBreakpoint( "render", *this );
		
		// We don't render if:
		//
		if( !doesWantToRender() )
		{
			return;
		}
		
		if( !injector || !injector->preDraw( relativeFrameTime, *this ))
		{
			preDraw( relativeFrameTime );
		}
		
		draw( relativeFrameTime, injector );
		
		forEachComponent( std::bind( &DisplayObjectComponent::render, std::placeholders::_1, std::ref( *this ), relativeFrameTime ));
		
		if( !injector || !injector->postDraw( relativeFrameTime, *this ))
		{
			postDraw( relativeFrameTime );
		}
	}

	void DisplayObject::preDraw( TimeType relativeFrameTime )
	{
		Renderer& renderer = Renderer::instance();

#if DEV_MODE
		if( m_shaderProgram && !m_shaderProgram->isLinked() )
		{
			con_error( this << " using " << m_shaderProgram << " found the program was not linked. Will try default program." );
			m_shaderProgram = nullptr;
		}
#endif
		
		// Setup the shader program.
		//
		if( !m_shaderProgram )
		{
			m_shaderProgram = getStockShaderProgram( true );
		}
		
		// Set transforms.
		//
		setupTransforms( relativeFrameTime );
		
		// Filtering trumps m_renderTarget. TODO Remove m_renderTarget.
		//
		if( m_filters.empty() == false )
		{
			const rect renderArea = localBounds();
			const vec2 renderSize = renderArea.dimensions();

			if( renderSize.x > 0 && renderSize.y > 0 )
			{
				vec2i adjustedRenderSize = vector_cast< int >( renderSize );
				
				for( int i = 0; i < 2; ++i )
				{
					// TODO Adjust arbitrarily, per directives, for arbitrary downsizing etc.
					adjustedRenderSize[ i ] = clamp( adjustedRenderSize[ i ], 1, Texture::maxAllowedSize() );
				}
				
				// Setup the render target for filtering.
				//
				if( !m_filteringRenderTarget )
				{
					m_filteringRenderTarget = createObject< RenderTarget >();
					m_filteringRenderTarget->clearColor( Color::Invisible );
					m_filteringRenderTarget->doInitialClearOnCapture( true );
				}
				
				// Establish the filter rendering target.
				//
				RenderTarget::BufferFormat format{ RenderTarget::ColorComponentType::UnsignedByte, RenderTarget::OutputType::Texture };
					
				m_filteringRenderTarget->create( adjustedRenderSize.x, adjustedRenderSize.y, format );
				
				m_filteringRenderTarget->beginCapturing();

				// Adjust further rendering to sit appropriately within the render target area.
				//
				renderer.pushMatrix( Renderer::MAT_ModelView );
				renderer.setMatrixToIdentity( Renderer::MAT_ModelView );
				
				renderer.pushMatrix( Renderer::MAT_Projection );
				renderer.setOrthoProjection( renderArea.left(), renderArea.right(), renderArea.bottom(), renderArea.top() );
			}
			else
			{
				m_filteringRenderTarget = nullptr;
			}
		}

		if( !m_filteringRenderTarget && m_renderTarget )
		{
			// Setup the output render target if desired.
			//
			m_renderTarget->beginCapturing();
			
			renderer.pushMatrix();
			renderer.setMatrixToIdentity();
		}
		
		// Render the mask, if any.
		//
		if( m_mask )
		{
			renderer.setStencilMode( Renderer::StencilMode::DrawToStencil );
			m_mask->render( relativeFrameTime );
			renderer.setStencilMode( m_maskStencilReadMode );
		}
		
		if( m_shaderProgram )
		{
			// Prepare shader program and color.
			//
			renderer.useShaderProgram( m_shaderProgram );

			renderer.setBlendMode( calculatedBlendMode() );
			
			renderer.pushColor();
			
			DisplayObjectState state = getTweenedState( relativeFrameTime );
			
			renderer.multiplyColor( state.color() );
			renderer.addColor( state.colorAdditive() );
		}
	}

	void DisplayObject::draw( TimeType relativeFrameTime, RenderInjector* injector )
	{
		// Implemented in subclasses
	}

	void DisplayObject::postDraw( TimeType relativeFrameTime )
	{
		Renderer& renderer = Renderer::instance();

		// Restore ordinary stencil mode.
		//
		if( m_mask )
		{
			renderer.setStencilMode( Renderer::StencilMode::Ignore );
		}

		renderer.popColor();
		
		// Finish the render target and render any filters.
		//
		if( m_filteringRenderTarget )
		{
			ASSERT( m_filteringRenderTarget->isCapturing() );
			m_filteringRenderTarget->endCapturing();
			
			Texture::ptr capturedTexture = m_filteringRenderTarget->getCapturedTexture();
			const rect captureRect = localBounds();
			
			renderer.popMatrix( Renderer::MAT_ModelView );
			renderer.popMatrix( Renderer::MAT_Projection );

			renderer.pushMatrix( Renderer::MAT_Texture );
			renderer.setMatrixToIdentity( Renderer::MAT_Texture );

			for( auto filter : m_filters )
			{
				ASSERT( filter );
				filter->render( capturedTexture, captureRect, relativeFrameTime );
			}
			
			renderer.popMatrix( Renderer::MAT_Texture );
		}
		else if( m_renderTarget )
		{
			ASSERT( m_renderTarget->isCapturing() );
			m_renderTarget->endCapturing();

			renderer.popMatrix();
		}
		
		unsetupTransforms();
		
		renderer.reportErrors();
	}

	bool DisplayObject::isTouchable() const
	{ 
		// An object participates in touch events iff it:
		return 
			!isMarkedForDeletion() && 							// Is alive AND
			( isTouchEnabled() ) &&								// Believes in touch at the user level AND
			doesWantToRender();									// Is visible.
	}

	void DisplayObject::setupTransforms( TimeType relativeFrameTime )
	{
		Renderer& renderer = Renderer::instance();
		
		// For performance, only transform if needed.
		//
		const DisplayObjectState state = getTweenedState( relativeFrameTime );
		
		const vec2 attachOffset = parentAttachOffset();
		
		bool needsRootTranslate = !attachOffset.isZero();
		bool needsTranslate = !state.position().isZero();
		bool needsRotate = state.rotation() != 0;
		bool needsScale = state.scale().x != 1.0f || state.scale().y != 1.0f;
		bool needsPivot = !state.pivot().isZero();
		
		m_didPushMatrixDuringPreDraw = needsRootTranslate || needsTranslate || needsRotate || needsScale || needsPivot;
		if( m_didPushMatrixDuringPreDraw )
		{
			renderer.pushMatrix();
		}

		if( needsTranslate || needsRootTranslate )
		{
			auto pos = state.position() + attachOffset;
			if( m_positionQuantum > 0 )
			{
				pos.x = std::floor( pos.x / m_positionQuantum ) * m_positionQuantum;
				pos.y = std::floor( pos.y / m_positionQuantum ) * m_positionQuantum;
			}
			
			renderer.translate( pos );
		}
		if( needsRotate )
		{
			renderer.rotate( angle( state.rotation() ));
		}
		if( needsScale )
		{
			renderer.scale( state.scale() );
		}
		if( needsPivot )
		{			
			auto pivot = -state.pivot();
			if( m_positionQuantum > 0 )
			{
				pivot.x = std::floor( pivot.x / m_positionQuantum ) * m_positionQuantum;
				pivot.y = std::floor( pivot.y / m_positionQuantum ) * m_positionQuantum;
			}
			renderer.translate( pivot );
		}
	}

	void DisplayObject::unsetupTransforms()
	{
		if( m_didPushMatrixDuringPreDraw )	// Assumes the answer to this call has not changed since the call to setupTransforms().
		{
			Renderer::instance().popMatrix();
			m_didPushMatrixDuringPreDraw = false;
		}
	}

	bool DisplayObject::rejectsOnTheBasisOfHitFlags( HitTestFlags flags ) const
	{
		if(( flags & HTF_RequireTouchable ) && !isTouchable() )
		{
			return true;
		}
		
		if(( flags & HTF_RequireVisible ) && !visible() )
		{
			return true;
		}

		if(( flags & HTF_RequireEditable ) && !isEditable() )
		{
			return true;
		}
		
		return false;
	}
	
	bool DisplayObject::hitTestPoint( const vec2& localLocation, HitTestFlags flags ) const
	{
		return false;
	}

	bool DisplayObject::hitTestMask( const vec2& localLocation, HitTestFlags flags ) const
	{
		if( !m_mask )
		{
			return true;
		}
		else
		{
			const vec2 inMaskLocation = m_mask->parentToLocal( localLocation );
			return m_mask->hitTestPoint( inMaskLocation, flags );
		}
	}

	void DisplayObject::onTapped( const EventTouch& event )
	{
		EventTouch tapEvent( TAPPED, event.touchId(), event.touchPhase(), event.tapCount(), event.location(), event.previousLocation(), event.nTouches(), event.iThisTouch(), event.wheelDelta(), event.target(), this );
		dispatchEvent( &tapEvent );
	}

	void DisplayObject::onDragBegin( const EventTouch& event )
	{
		EventTouch dragEvent( DRAG_BEGIN, event.touchId(), event.touchPhase(), event.tapCount(), event.location(), event.previousLocation(), event.nTouches(), event.iThisTouch(), event.wheelDelta(), event.target(), this );
		dispatchEvent( &dragEvent );
	}
	
	void DisplayObject::onDragMove( const EventTouch& event )
	{
		if( m_doMoveWithDrag )
		{
			auto delta = localToParent( event.location() - m_dragLocalStartLocation );
			
			delta *= m_moveWithDragAxisScalars;
			
			position( delta );
		}
		
		EventTouch dragEvent( DRAG_MOVE, event.touchId(), event.touchPhase(), event.tapCount(), event.location(), event.previousLocation(), event.nTouches(), event.iThisTouch(), event.wheelDelta(), event.target(), this );
		dispatchEvent( &dragEvent );
	}
	
	void DisplayObject::onDragEnd( const EventTouch& event )
	{
		EventTouch dragEvent( DRAG_END, event.touchId(), event.touchPhase(), event.tapCount(), event.location(), event.previousLocation(), event.nTouches(), event.iThisTouch(), event.wheelDelta(), event.target(), this );
		dispatchEvent( &dragEvent );
	}
	
	void DisplayObject::onTouchCommon( EventTouch& event )
	{
		if( !isTouchable() )
		{
			return;
		}
		
		event.currentTarget( this );
		
		if( event.target() == this )
		{
			event.phase( Event::AtTarget );
		}
		
		dispatchEvent( &event );
	}
	
	void DisplayObject::onTouchBegin( const EventTouch& event )
	{
		TIMER_AUTO( DisplayObject::onTouchBegin )
		
		EventTouch modifiedEvent( event );
		onTouchCommon( modifiedEvent );

		forEachComponent( std::bind( &DisplayObjectComponent::onTouchBegin, std::placeholders::_1, std::ref( *this ), std::ref( modifiedEvent )));
		
		if(( modifiedEvent.phase() == Event::Bubbling || modifiedEvent.phase() == Event::AtTarget ) && ( !m_isDragging && isTouchable() ))
		{
			m_touchStartTime = getAbsoluteTimeSeconds();
			
			if( isDragEnabled() )
			{
				trace_drag( this << "Beginning potential drag with id: " << modifiedEvent.touchId() );
				
				m_potentialDragLastTouchId = modifiedEvent.touchId();
				m_potentialDragLastIndex = modifiedEvent.iThisTouch();
				m_potentialDragLastNumTouches = modifiedEvent.nTouches();
				m_potentialDragLastLocation = modifiedEvent.location();
				m_potentialDragLastPreviousLocation = modifiedEvent.previousLocation();
				
				m_dragLocalStartLocation = modifiedEvent.location();
				m_dragStageStartLocation = localToGlobal( m_dragLocalStartLocation );
				
				stage().addEventListener( EventTouch::TOUCH_MOVE, FRESH_CALLBACK( onTouchMoveAnywherePossibleDrag ));
			}
			stage().addEventListener( EventTouch::TOUCH_END, FRESH_CALLBACK( onTouchEndAnywhere ));
			stage().addEventListener( EventTouch::TOUCH_CANCELLED, FRESH_CALLBACK( onTouchEndAnywhere ));
		}
	}
	
	bool DisplayObject::couldStartDrag() const
	{
		return !m_isDragging && m_touchStartTime >= 0 && isDragEnabled();
	}
	
	void DisplayObject::startDrag( const EventTouch& event )
	{
		// Start the drag properly.
		//
		m_isDragging = true;
		
		stage().removeEventListener( EventTouch::TOUCH_MOVE, FRESH_CALLBACK( onTouchMoveAnywherePossibleDrag ));
		
		// Ask the stage to let us know if the touch is released outside of us.
		//
		stage().addEventListener( EventTouch::TOUCH_MOVE, FRESH_CALLBACK( onTouchMoveAnywhereForDrag ));
		stage().addEventListener( EventTouch::TOUCH_END, FRESH_CALLBACK( onTouchEndAnywhereForDrag ));
		stage().addEventListener( EventTouch::TOUCH_CANCELLED, FRESH_CALLBACK( onTouchEndAnywhereForDrag ));
		
		m_draggingTouchId = event.touchId();

		onDragBegin( event );
	}
		
	void DisplayObject::onTouchMove( const EventTouch& event )
	{
		TIMER_AUTO( DisplayObject::onTouchMove )
		
		EventTouch modifiedEvent( event );
		onTouchCommon( modifiedEvent );
		
		forEachComponent( std::bind( &DisplayObjectComponent::onTouchMove, std::placeholders::_1, std::ref( *this ), std::ref( modifiedEvent )));
	}

	void DisplayObject::onTouchEnd( const EventTouch& event )
	{
		TIMER_AUTO( DisplayObject::onTouchEnd )
		
		trace_drag( this << " phase: " << event.phase() << " dragId: " << m_draggingTouchId << " eventId: " << event.touchId() );

		EventTouch modifiedEvent( event );
		onTouchCommon( modifiedEvent );

		if( !m_isDragging && m_touchStartTime >= 0 && ( modifiedEvent.phase() == Event::Bubbling || modifiedEvent.phase() == Event::AtTarget ))
		{
			trace_drag( this << " tapped" );
			onTapped( modifiedEvent );
		}
		
		forEachComponent( std::bind( &DisplayObjectComponent::onTouchEnd, std::placeholders::_1, std::ref( *this ), std::ref( modifiedEvent )));

		if( modifiedEvent.phase() == Event::Bubbling || modifiedEvent.phase() == Event::AtTarget )
		{
			m_touchStartTime = -1;
		}
	}

	FRESH_DEFINE_CALLBACK( DisplayObject, onTouchEndAnywhere, EventTouch )
	{
		trace_drag( this << " phase: " << event.phase() << " touchStartTime: " << m_touchStartTime << " eventId: " << event.touchId() );
		
		if( event.phase() == Event::Bubbling || event.phase() == Event::AtTarget )
		{
			trace_drag( this << " nulling touchStartTime." );
			m_touchStartTime = -1;
			
			if( hasStage() )
			{
				stage().removeEventListener( EventTouch::TOUCH_END, FRESH_CALLBACK( onTouchEndAnywhere )  );
				stage().removeEventListener( EventTouch::TOUCH_CANCELLED, FRESH_CALLBACK( onTouchEndAnywhere ));
			}
		}
	}
	
	void DisplayObject::onTouchCancelled( const EventTouch& event )
	{
		onTouchEnd( event );
	}
	
	void DisplayObject::onWheelMove( const EventTouch& event )
	{
		EventTouch modifiedEvent( event );
		onTouchCommon( modifiedEvent );

		forEachComponent( std::bind( &DisplayObjectComponent::onWheelMove, std::placeholders::_1, std::ref( *this ), std::ref( modifiedEvent )));
	}

	void DisplayObject::onBeginPlay()
	{
		m_hasReceivedBeginPlayNotification = true;

		forEachComponent( std::bind( &DisplayObjectComponent::onBeginPlay, std::placeholders::_1, std::ref( *this ) ));
	}
	
	void DisplayObject::onEndPlay()
	{
		m_hasReceivedBeginPlayNotification = false;

		forEachComponent( std::bind( &DisplayObjectComponent::onEndPlay, std::placeholders::_1, std::ref( *this ) ));
	}
	
	bool DisplayObject::isEditable() const
	{
		return m_isEditingEnabled && m_isKeyframeVisible;
	}
	
	void DisplayObject::onAddedToStage()
	{
		// If the stage is playing, then it has at least started sending out onBeginPlay() notifications.
		// In that case, we'll notify ourselves.
		//
		if( stage().isPlaying() && !m_hasReceivedBeginPlayNotification )
		{
			onBeginPlay();

			forEachComponent( std::bind( &DisplayObjectComponent::onAddedToStage, std::placeholders::_1, std::ref( *this ) ));
		}
	}

	void DisplayObject::onAddedToParent()
	{
		forEachComponent( std::bind( &DisplayObjectComponent::onAddedToParent, std::placeholders::_1, std::ref( *this ) ));
	}
	
	void DisplayObject::onRemovingFromParent()
	{
		REQUIRES( m_parent );
		
		forEachComponent( std::bind( &DisplayObjectComponent::onRemovingFromParent, std::placeholders::_1, std::ref( *this ) ));
		
		if( hasStage() )
		{
			propagateStageRemovalNotification();
		}
		
		m_isKeyframeVisible = true;		// No longer affected by a MovieClip parent, for example.
		m_parent = nullptr;
		
		m_hasReceivedParentNotification = false;
	}
	
	void DisplayObject::onRemovingFromStage()
	{
		ASSERT( hasStage() );		// Should be called before actual removal.

		m_isDragging = false;
		stage().removeEventListener( EventTouch::TOUCH_MOVE, FRESH_CALLBACK( onTouchMoveAnywherePossibleDrag ));
		stage().removeEventListener( EventTouch::TOUCH_MOVE, FRESH_CALLBACK( onTouchMoveAnywhereForDrag ));
		stage().removeEventListener( EventTouch::TOUCH_END, FRESH_CALLBACK( onTouchEndAnywhereForDrag ));
		stage().removeEventListener( EventTouch::TOUCH_CANCELLED, FRESH_CALLBACK( onTouchEndAnywhereForDrag ));
	}

	Object::ptr DisplayObject::createClone( NameRef name ) const
	{
		auto copy = dynamic_freshptr_cast< DisplayObject::ptr >( Super::createClone( name ));
		ASSERT( copy );
		
		if( copy->parent() )
		{
			copy->onRemovingFromParent();
		}

		return copy;
	}
	
	bool DisplayObject::verifyTree() const
	{
#if defined( DEBUG )
		if( isMarkedForDeletion() ) return true;
		
		bool correct = !parent() || m_hasReceivedParentNotification;
		ASSERT( correct );
		
		correct = ( !hasStage() || m_hasReceivedStageNotification ) && correct;
		ASSERT( correct );
		
		// This assumption is false for masks, who do have a stage but not a parent.
		correct = ( parent() || ( !hasStage() || this == &stage() )) && correct;	// If we don't have a parent, we'd better not have a stage.
		ASSERT( correct );
		
		return correct;
#else
		return true;
#endif
	}

	rect DisplayObject::localBounds() const
	{
		return rect( 0, 0, 0, 0 );
	}
	
	rect DisplayObject::bounds() const
	{
		return localToParent( localBounds() );
	}
	
	rect DisplayObject::frame() const
	{
		if( m_inheritParentFrame && parent() )
		{
			const auto parentHalfSize = parent()->frame().dimensions() * 0.5f;
			return rect( -parentHalfSize, parentHalfSize );
		}
		else
		{
			PROMISES( m_frame.isWellFormed() );
			return m_frame;
		}
	}
	
	void DisplayObject::frame( const rect& r )
	{
		REQUIRES( r.isWellFormed() );
		m_frame = r;
	}

	void DisplayObject::traversePreOrder( const std::function< void( SmartPtr< DisplayObject > )>& fnPerObject, int maxDepth, int depth )
	{
		if( depth <= maxDepth || maxDepth < 0 )
		{
			fnPerObject( this );
		}
	}
	
	void DisplayObject::traversePostOrder( const std::function< void( SmartPtr< DisplayObject > )>& fnPerObject, int maxDepth, int depth )
	{
		if( depth <= maxDepth || maxDepth < 0 )
		{
			fnPerObject( this );
		}
	}

	void DisplayObject::traversePrePostOrder(const std::function< void( SmartPtr< DisplayObject > )>& fnPreObject, 
									  const std::function< void( SmartPtr< DisplayObject > )>& fnPostObject,
									  int maxDepth, int depth )
	{
		if( depth <= maxDepth || maxDepth < 0 )
		{
			fnPreObject( this );
			fnPostObject( this );
		}
	}
	
	ShaderProgram::ptr DisplayObject::getStockShaderProgram( bool textured )
	{
		Renderer& renderer = Renderer::instance();
		
		if( textured )
		{
			if( !g_stockShaderProgramTextured )
			{
				g_stockShaderProgramTextured = renderer.createOrGetShaderProgram( "PlainVanilla" );
			}
			return g_stockShaderProgramTextured;
		}
		else
		{
			if( !g_stockShaderProgramUntextured )
			{
				g_stockShaderProgramUntextured = renderer.createOrGetShaderProgram( "PlainVanillaUntextured" );
			}
			return g_stockShaderProgramUntextured;
		}
	}
	
	SmartPtr< VertexStructure > DisplayObject::getPos2VertexStructure()
	{
		VertexStructure::ptr vertexStructure = Renderer::instance().createOrGetVertexStructure( "VS_Pos2" );
		
		if( vertexStructure->getVertexSizeInBytes() == 0 )
		{
			vertexStructure->addAttribute( "position", 2, VertexStructure::Float, VertexStructure::Position );
		}
		
		return vertexStructure;
	}
	
	SmartPtr< VertexStructure > DisplayObject::getPos2TexCoord2VertexStructure()
	{
		VertexStructure::ptr vertexStructure = Renderer::instance().createOrGetVertexStructure( "VS_Pos2TexCoord2" );
		
		if( vertexStructure->getVertexSizeInBytes() == 0 )
		{
			vertexStructure->addAttribute( "position", 2, VertexStructure::Float, VertexStructure::Position );
			vertexStructure->addAttribute( "texCoord", 2, VertexStructure::Float, VertexStructure::TexCoord );
		}
		
		return vertexStructure;
	}
	
	FRESH_DEFINE_CALLBACK( DisplayObject, onTouchMoveAnywherePossibleDrag, EventTouch )
	{
		ASSERT( !m_isDragging );
		
		if( isDragEnabled() )
		{
			// Consider starting a drag.
			if( couldStartDrag() && ( event.phase() == Event::AtTarget || event.phase() == Event::Bubbling ))
			{
				const vec2 localLocation( globalToLocal( static_freshptr_cast< DisplayObject::cptr >( event.currentTarget() )->localToGlobal( event.location() )));
				const vec2 lastLocalLocation( globalToLocal( static_freshptr_cast< DisplayObject::cptr >( event.currentTarget() )->localToGlobal( event.previousLocation() )));
				
				m_potentialDragLastTouchId = event.touchId();
				m_potentialDragLastLocation = localLocation;
				m_potentialDragLastPreviousLocation = lastLocalLocation;

				EventTouch transformedTouch( event, event.target(), m_potentialDragLastLocation, m_potentialDragLastPreviousLocation );
				transformedTouch.currentTarget( this );
				
				const vec2 eventLocationStageSpace = localToGlobal( transformedTouch.location() );
				
				// Have we moved far enough or held the touch long enough to constitute a drag?
				//
				const auto dragDistance = distance( eventLocationStageSpace, m_dragStageStartLocation );
				trace_drag( this << " dragDistance=" << dragDistance );

				if( dragDistance >= m_minStageDistanceForDrag )
				{
					trace_drag( this << " started dragging due to distance " << dragDistance );
					startDrag( transformedTouch );
				}
			}
		}
		else
		{
			stage().removeEventListener( EventTouch::TOUCH_MOVE, FRESH_CALLBACK( onTouchMoveAnywherePossibleDrag ));
		}
	}

	FRESH_DEFINE_CALLBACK( DisplayObject, onTouchMoveAnywhereForDrag, EventTouch )
	{
		ASSERT( m_isDragging );
		ASSERT( isDragEnabled() );
		
		trace_drag( this << " phase: " << event.phase() << " dragId: " << m_draggingTouchId << " eventId: " << event.touchId() );
		
		// Ignore if not the same touch id.
		//
		if( m_draggingTouchId != event.touchId() )
		{
			return;
		}
			
		const vec2 localLocation( globalToLocal( static_freshptr_cast< DisplayObject::cptr >( event.currentTarget() )->localToGlobal( event.location() )));
		const vec2 lastLocalLocation( globalToLocal( static_freshptr_cast< DisplayObject::cptr >( event.currentTarget() )->localToGlobal( event.previousLocation() )));
		EventTouch transformedEvent( event, event.target(), localLocation, lastLocalLocation );
		transformedEvent.currentTarget( this );
		onDragMove( transformedEvent );
	}
	
	FRESH_DEFINE_CALLBACK( DisplayObject, onTouchEndAnywhereForDrag, EventTouch )
	{
		trace_drag( this << " phase: " << event.phase() << " dragId: " << m_draggingTouchId << " eventId: " << event.touchId() );
		
		// Ignore if not the same touch id.
		//
		if( m_draggingTouchId != event.touchId() )
		{
			return;
		}
		
		if( event.phase() == Event::AtTarget || event.phase() == Event::Bubbling )		// Respond to the "later" notification. This makes onTouchEnd() get called first.
		{
			if( hasStage() )
			{
				stage().removeEventListener( EventTouch::TOUCH_MOVE, FRESH_CALLBACK( onTouchMoveAnywhereForDrag ));
				stage().removeEventListener( EventTouch::TOUCH_END, FRESH_CALLBACK( onTouchEndAnywhereForDrag ));
				stage().removeEventListener( EventTouch::TOUCH_CANCELLED, FRESH_CALLBACK( onTouchEndAnywhereForDrag ));
			}
		
			if( m_isDragging )
			{
				ASSERT( isDragEnabled() );
				
				const vec2 localLocation( globalToLocal( static_freshptr_cast< DisplayObject::cptr >( event.currentTarget() )->localToGlobal( event.location() )));
				const vec2 lastLocalLocation( globalToLocal( static_freshptr_cast< DisplayObject::cptr >( event.currentTarget() )->localToGlobal( event.previousLocation() )));
				EventTouch transformedEvent( event, event.target(), localLocation, lastLocalLocation );
				transformedEvent.currentTarget( this );
				onDragEnd( transformedEvent );
			}
			m_isDragging = false;
			m_touchStartTime = -1;
		}
	}
	
	void DisplayObject::parent( DisplayObjectContainer::wptr newParent )
	{
		REQUIRES( newParent );
		REQUIRES( !parent() );
		m_parent = newParent;
	}

	void DisplayObject::propagateStage( Stage::wptr stage )
	{
		m_stage = stage;
		
		if( m_mask )
		{
			m_mask->propagateStage( stage );
		}
	}
	
	void DisplayObject::propagateStageNotification()
	{
		REQUIRES( !isMarkedForDeletion() );
		REQUIRES( hasStage() );
		if( !m_hasReceivedStageNotification )
		{
			onAddedToStage();
			m_hasReceivedStageNotification = true;
			
			if( m_mask )
			{
				m_mask->propagateStageNotification();
			}
		}
	}

	void DisplayObject::propagateStageRemovalNotification()
	{
		m_hasReceivedStageNotification = false;
		
		onRemovingFromStage();
		
		if( m_mask )
		{
			m_mask->propagateStageRemovalNotification();
		}
		
		m_stage = nullptr;
	}
	
	void DisplayObject::propagateParentNotification()
	{
		REQUIRES( parent() || ( m_stage == this ));
		
		if( !m_hasReceivedParentNotification )
		{
			onAddedToParent();
			m_hasReceivedParentNotification = true;
		}		
	}

	bool DisplayObject::hasComponent( SmartPtr< DisplayObjectComponent > component ) const
	{
		return std::find( m_components.begin(), m_components.end(), component ) != m_components.end();
	}
	
	void DisplayObject::addComponent( SmartPtr< DisplayObjectComponent > component )
	{
		REQUIRES( component );
		REQUIRES( !hasComponent( component ));
		
		m_components.push_back( component );
		
		component->onAddedToHost( *this );
	}

	void DisplayObject::removeComponent( SmartPtr< DisplayObjectComponent > component )
	{
		REQUIRES( component );
		REQUIRES( hasComponent( component ));

		component->onRemovingFromHost( *this );
		
		m_components.erase( std::find( m_components.begin(), m_components.end(), component ));
		
		PROMISES( !hasComponent( component ));
	}
	
}


