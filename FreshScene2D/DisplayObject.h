/*
 *  DisplayObject.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/18/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#ifndef CTC_DISPLAY_OBJECT_INCLUDED
#define CTC_DISPLAY_OBJECT_INCLUDED

#include "Vector2.h"
#include "Angle.h"
#include "Color.h"
#include "Rectangle.h"
#include "EventDispatcher.h"
#include "ShaderProgram.h"
#include "Renderer.h"
#include "RenderTarget.h"
#include "DisplayObjectState.h"
#include "DisplayObjectComponent.h"
#include "EventTouch.h"
#include <functional>

namespace fr
{
	
	class DisplayObjectContainer;
	class VertexStructure;
	class Stage;
	class DisplayFilter;
	
	class DisplayObject : public EventDispatcher
	{
		FRESH_DECLARE_CLASS( DisplayObject, EventDispatcher )
	public:
		
		static const char* TAPPED;
		static const char* DRAG_BEGIN;
		static const char* DRAG_MOVE;
		static const char* DRAG_END;

		virtual ~DisplayObject();
		
		bool hasStage() const;
		Stage& stage() const;
		
		WeakPtr< DisplayObjectContainer > parent() const;
		virtual bool hasAncestor( WeakPtr< const DisplayObjectContainer > container, int maxDepth = -1 ) const;
			// maxDepth specifies the maximum distance up the ancestry tree 
			// before we give up looking for this ancestor.
			// If maxDepth < 0, maxDepth is infinite.
			// REQUIRES( container );
				
		SYNTHESIZE_GET( SmartPtr< DisplayObject >, mask )
		
		SYNTHESIZE( SmartPtr< RenderTarget >, renderTarget );
		
		void mask( SmartPtr< DisplayObject > mask );
		// REQUIRES( mask != this );
		// REQUIRES( !mask || !dynamic_cast< DisplayObjectContainer* >( this ) || !mask->hasAncestor( dynamic_cast< DisplayObjectContainer* >( this )));
		
		virtual void markForDeletion();

		bool isMarkedForDeletion() const						{ return m_isMarkedForDeletion; }
		
		SYNTHESIZE( bool, visible )
		
		void setKeyframeVisible( bool visible_ )				{ m_isKeyframeVisible = visible_;		}
		bool isKeyframeVisible() const							{ return m_isKeyframeVisible;		}
				
		void color( Color c )									{ m_color = c; }
		Color color() const										{ return m_color; }

		void colorAdditive( Color color )						{ m_colorAdditive = color; }
		Color colorAdditive() const								{ return m_colorAdditive; }
		
		void shaderProgram( ShaderProgram::ptr program )		{ m_shaderProgram = program; }
		ShaderProgram::ptr shaderProgram() const				{ return m_shaderProgram; }
		
		virtual Renderer::BlendMode calculatedBlendMode() const	{ return m_blendMode; }
		
		SYNTHESIZE( Renderer::BlendMode, blendMode )
		SYNTHESIZE( Renderer::StencilMode, maskStencilReadMode )
		
		void position( const vec2& v )							{ m_position = v; }
		void position( real x, real y )							{ m_position.set( x, y ); }
		const vec2& position() const							{ return m_position; }
		
		void rotation( angle a )								{ m_rotation = a; }
		angle rotation() const									{ return m_rotation; }
		
		void scale( const vec2& v )								{ m_scale = v; }
		void scale( real x, real y )							{ m_scale.set( x, y ); }
		void scale( real uniformScale )							{ m_scale.x = m_scale.y = uniformScale; }
		const vec2& scale() const								{ return m_scale; }

		void pivot( const vec2& pivot_ )						{ m_pivot = pivot_; }
		void pivot( real pivotX, real pivotY )					{ m_pivot.x = pivotX; m_pivot.y = pivotY; }
		const vec2& pivot() const								{ return m_pivot; }

		void parentFrameAttachPoint( const vec2& p )			{ parentFrameAttachPoint( p.x, p.y ); }
		void parentFrameAttachPoint( real x, real y )			{ m_parentFrameAttachPoint.set( x, y ); }
		const vec2& parentFrameAttachPoint() const				{ return m_parentFrameAttachPoint; }
		
		SYNTHESIZE( bool, ignoreFrameAttachment )
		
		inline vec2 parentAttachOffset() const;
		// REQUIRES( parent() );

		enum HitTestFlags
		{
			HTF_IncludeAll				= 0x0,
			HTF_RequireVisible			= 0x1,
			HTF_RequireTouchable		= ( 0x2 | HTF_RequireVisible ),
			HTF_RequireEditable			= 0x4,
		};
		
		virtual bool rejectsOnTheBasisOfHitFlags( HitTestFlags flags ) const;
		
		virtual bool hitTestPoint( const vec2& localLocation, HitTestFlags flags ) const;
		virtual bool hitTestMask( const vec2& localLocation, HitTestFlags flags ) const;
		// Returns true if no mask, else true iff the point touches the mask.
		
		virtual rect localBounds() const;
		rect bounds() const;
		
		rect frame() const;
		// PROMISES( result.isWellFormed() );
		
		virtual void frame( const rect& r );
		// REQUIRES( r.isWellFormed() );
		
		SYNTHESIZE( bool, inheritParentFrame )
		
		vec2 localToGlobal( const vec2& location ) const;
		vec2 globalToLocal( const vec2& location ) const;
		vec2 localToParent( const vec2& location ) const;
		vec2 parentToLocal( const vec2& location ) const;
		
		rect localToGlobal( const rect& rectangle ) const;
		rect globalToLocal( const rect& rectangle ) const;
		rect localToParent( const rect& rectangle ) const;
		rect parentToLocal( const rect& rectangle ) const;

		// For rotations
		//
		angle localToGlobal( angle rotation ) const;
		angle globalToLocal( angle rotation ) const;
		angle localToParent( angle rotation ) const;
		angle parentToLocal( angle rotation ) const;
		
		// For points (no translation applied)
		//
		vec2 localToGlobalPoint( const vec2& point ) const;
		vec2 globalToLocalPoint( const vec2& point ) const;
		vec2 localToParentPoint( const vec2& point ) const;
		vec2 parentToLocalPoint( const vec2& point ) const;
		
		// For scales (no rotation or translation applied )
		//
		vec2 localToGlobalScale( const vec2& scale ) const;
		vec2 globalToLocalScale( const vec2& scale ) const;
		vec2 localToParentScale( const vec2& scale ) const;
		vec2 parentToLocalScale( const vec2& scale ) const;
		
		
		void setLifespan( size_t nTicks ) 									{ if( nTicks == 0 ) { m_lifespan = 0; } else { m_lifespan = m_nUpdates + nTicks; } }
		
		SYNTHESIZE_GET( size_t, nUpdates )
		SYNTHESIZE( bool, wantsUpdate )
		bool doUpdate() const;
		
		virtual void update();

		struct RenderInjector
		{
			virtual ~RenderInjector() {}
			bool preDraw( TimeType relativeFrameTime, DisplayObject& object );
			bool draw( TimeType relativeFrameTime, DisplayObject& object );
			bool postDraw( TimeType relativeFrameTime, DisplayObject& object );
			
			void setNext( std::shared_ptr< RenderInjector > next ) { m_next = next; }
			
		private:
			
			virtual bool preDrawOverride( TimeType relativeFrameTime, DisplayObject& object ) = 0;
			virtual bool drawOverride( TimeType relativeFrameTime, DisplayObject& object ) = 0;
			virtual bool postDrawOverride( TimeType relativeFrameTime, DisplayObject& object ) = 0;

			std::shared_ptr< RenderInjector > m_next;
		};
		
		virtual void preRender( TimeType relativeFrameTime );
		virtual void render( TimeType relativeFrameTime, RenderInjector* injector = nullptr );
		
		virtual void draw( TimeType relativeFrameTime, RenderInjector* injector = nullptr );

		DisplayObjectState getPreviousState() const					{ return m_hasMeaningfulPreviousState ? m_previousState : getCurrentState(); }
		DisplayObjectState getCurrentState() const;
		DisplayObjectState getTweenedState( TimeType alpha, const Tweener< DisplayObjectState >& tweener = DisplayObjectState::tweenerLinear ) const;
		// REQUIRES( 0 <= alpha && alpha <= 1.0f );
		
		// In all versions, "touchLocation" is in *local* space.
		//
		virtual void onTouchBegin( const EventTouch& event );
		virtual void onTouchMove( const EventTouch& event );
		virtual void onTouchEnd( const EventTouch& event );
		virtual void onTouchCancelled( const EventTouch& event );
		virtual void onWheelMove( const EventTouch& event );
		
		SYNTHESIZE( bool, isTouchEnabled );
		virtual bool isTouchable() const;
				
		SYNTHESIZE( bool, isDragEnabled );
		SYNTHESIZE( bool, doMoveWithDrag );
		SYNTHESIZE( vec2, moveWithDragAxisScalars );
		SYNTHESIZE( real, minStageDistanceForDrag );
		SYNTHESIZE( TimeType, minRealDurationForDrag );
		bool isDragging() const { return m_isDragging; }
		
		virtual void onGainedKeyboardFocus() {}
		virtual void onLostKeyboardFocus() {}
		
		virtual bool doesWantToRender() const
		{
			// We want to render if:
			//
			return
			!m_isMarkedForDeletion && 									// We're alive.
			m_isKeyframeVisible && 										// The keyframe says we should be visible.
			m_visible &&
			( m_color.getA() + m_colorAdditive.getA() ) > 0;				// We think we should be visible.
		}
		
		virtual void traversePreOrder( const std::function< void( SmartPtr< DisplayObject > )>& fnPerObject, int maxDepth = -1, int depth = 0 ); 
		virtual void traversePostOrder( const std::function< void( SmartPtr< DisplayObject > )>& fnPerObject, int maxDepth = -1, int depth = 0 ); 
		virtual void traversePrePostOrder(const std::function< void( SmartPtr< DisplayObject > )>& fnPreObject, 
										  const std::function< void( SmartPtr< DisplayObject > )>& fnPostObject,
										  int maxDepth = -1, int depth = 0 ); 

		
		virtual void setupTransforms( TimeType relativeFrameTime );
		virtual void unsetupTransforms();
		
		virtual void recordPreviousState( bool recursive = false );
		
		static ShaderProgram::ptr getStockShaderProgram( bool textured );
		static SmartPtr< VertexStructure > getPos2VertexStructure();
		static SmartPtr< VertexStructure > getPos2TexCoord2VertexStructure();
		
		virtual void onBeginPlay();
		virtual void onEndPlay();
		// If you override onEndPlay(), you should also override your destructor
		// and have it call onEndPlay() directly to preserve your subclass-specific
		// behaviors.
		SYNTHESIZE_GET( bool, hasReceivedBeginPlayNotification );
		
		virtual bool isEditable() const;
		
		SYNTHESIZE( bool, isEditingEnabled )
		SYNTHESIZE( bool, editorLocked )
		
		// Tree fixup
		//
		void parent( WeakPtr< DisplayObjectContainer > newParent );
		virtual void propagateStage( WeakPtr< Stage > stage );
		// REQUIRES( stage );
		virtual void propagateStageNotification();
		// REQUIRES( hasStage() );
		virtual void propagateParentNotification();
		// REQUIRES( parent() );
		virtual void propagateStageRemovalNotification();

		virtual void onAddedToStage();
		virtual void onAddedToParent();
		virtual void onRemovingFromParent();
		virtual void onRemovingFromStage();
		
		virtual Object::ptr createClone( NameRef name = DEFAULT_OBJECT_NAME ) const override;
		
		// Components.
		//
		bool hasComponent( SmartPtr< DisplayObjectComponent > component ) const;
		void addComponent( SmartPtr< DisplayObjectComponent > component );
		// REQUIRES( component );
		// REQUIRES( !hasComponent( component ));
		// hasComponent( component ) is *NOT* promised, simply because components fairly often self-destruct immediately after being added.
		
		void removeComponent( SmartPtr< DisplayObjectComponent > component );
		// REQUIRES( component );
		// REQUIRES( hasComponent( component ));
		// PROMISES( !hasComponent( component ));
		
		template< typename component_t >
		SmartPtr< component_t > firstComponentOfType() const;
		
		template< typename component_t = DisplayObjectComponent, typename fn_t >
		bool forEachComponent( fn_t&& fn ) const;
		// Returns true iff a component "overrode" (i.e. wants to trump) the normal behavior. Callers
		// are free to honor or ignore this value.
		

		virtual bool verifyTree() const;	// For debugging.
				
	protected:
				
		VAR( vec2, m_position );
		VAR( angle, m_rotation );
		DVAR( vec2, m_scale, vec2( 1.0f, 1.0f ));
		VAR( vec2, m_pivot );
		VAR( vec2, m_parentFrameAttachPoint );
		DVAR( size_t, m_lifespan, 0 );	// 0 means infinite

		DVAR( real, m_positionQuantum, 0 );

		VAR( rect, m_frame );
		DVAR( bool, m_inheritParentFrame, false );
		
		DVAR( real, m_minStageDistanceForDrag, 10.0f );
		DVAR( TimeType, m_minRealDurationForDrag, 0.5 );		// If <= 0, drag is not induced by time delay--only distance.
		
		SYNTHESIZE_GET( vec2, dragStageStartLocation );
		SYNTHESIZE_GET( vec2, dragLocalStartLocation );
		
		bool isTransformed() const
		{
			return !m_parentFrameAttachPoint.isZero() || !m_position.isZero() || m_rotation != angle( 0U ) || m_scale.x != 1.0f || m_scale.y != 1.0f;
		}
		
		virtual void preDraw( TimeType relativeFrameTime );
		virtual void postDraw( TimeType relativeFrameTime );
		
		virtual void onLifespanCompleted()									{}
		
		bool m_didPushMatrixDuringPreDraw = false;		// TODO Bit of a hack I'm afraid. Recording the stack depth might be more general.
		
		static inline void drawObject( SmartPtr< DisplayObject > displayObject, TimeType relativeFrameTime )
		{
			displayObject->draw( relativeFrameTime );
		}
		
		void onTouchCommon( EventTouch& event );
		
		
		// Semantic input support.
		//
		virtual void onTapped( const EventTouch& event );
		virtual void onDragBegin( const EventTouch& event );
		virtual void onDragMove( const EventTouch& event );
		virtual void onDragEnd( const EventTouch& event );
		
		virtual bool couldStartDrag() const;
		
		void startDrag( const EventTouch& event );
		
		FRESH_DECLARE_CALLBACK( DisplayObject, onTouchEndAnywhere, EventTouch );
		FRESH_DECLARE_CALLBACK( DisplayObject, onTouchMoveAnywherePossibleDrag, EventTouch );
		FRESH_DECLARE_CALLBACK( DisplayObject, onTouchMoveAnywhereForDrag, EventTouch );
		FRESH_DECLARE_CALLBACK( DisplayObject, onTouchEndAnywhereForDrag, EventTouch );
		
	private:

		DisplayObjectState m_previousState;
		
		VAR( WeakPtr< DisplayObjectContainer >, m_parent );
		VAR( WeakPtr< Stage >, m_stage );
		
		DVAR( size_t, m_nUpdates, 0 );
		DVAR( Color, m_color, Color::White );
		DVAR( Color, m_colorAdditive, 0x00000000 );

		VAR( SmartPtr< ShaderProgram >, m_shaderProgram );
						
		DVAR( Renderer::BlendMode, m_blendMode, Renderer::BlendMode::None );
		DVAR( Renderer::StencilMode, m_maskStencilReadMode, Renderer::StencilMode::MaskInclusive );
		
		VAR( SmartPtr< DisplayObject >, m_mask );

		VAR( std::vector< SmartPtr< DisplayObjectComponent >>, m_components );
			
		DVAR( vec2, m_moveWithDragAxisScalars, vec2( 1 ));
		
		DVAR( bool, m_isTouchEnabled, true );
		DVAR( bool, m_isDragEnabled, false );
		DVAR( bool, m_doMoveWithDrag, false );
		DVAR( bool, m_isMarkedForDeletion, false );
		DVAR( bool, m_visible, true );
		DVAR( bool, m_isKeyframeVisible, true );
		DVAR( bool, m_wantsUpdate, true );
		DVAR( bool, m_ignoreFrameAttachment, true );

		DVAR( bool, m_isEditingEnabled, true );
		DVAR( bool, m_editorLocked, false );	// For objects that are generally editable, makes them temporarily uneditable.

		VAR( RenderTarget::ptr, m_renderTarget );

		VAR( RenderTarget::ptr, m_filteringRenderTarget );
		VAR( std::vector< SmartPtr< DisplayFilter >>, m_filters );

		
		vec2 m_dragStageStartLocation;
		vec2 m_dragLocalStartLocation;
		TimeType m_touchStartTime = -1;
		
		EventTouch::TouchId m_potentialDragLastTouchId = nullptr;
		vec2 m_potentialDragLastLocation;
		vec2 m_potentialDragLastPreviousLocation;
		size_t m_potentialDragLastNumTouches = 0;
		size_t m_potentialDragLastIndex = 0;
		
		bool m_isDragging = false;
		EventTouch::TouchId m_draggingTouchId;
		
		bool m_hasMeaningfulPreviousState = false;

		bool m_hasReceivedStageNotification = false;
		bool m_hasReceivedParentNotification = false;
		bool m_hasReceivedBeginPlayNotification = false;
		
#if DEV_MODE
		DVAR( bool, m_debugBreakRendering, false );
#endif				
		
		vec2 properParentAttachOffset() const;
	};
	
	/////////////////////////////////////////////////////////////////////////////////////
	// INLINES
	
	inline DisplayObjectState DisplayObject::getCurrentState() const
	{
		return DisplayObjectState::create( *this );
	}

	inline DisplayObjectState DisplayObject::getTweenedState( TimeType alpha, const Tweener< DisplayObjectState >& tweener ) const
	{
		REQUIRES( 0 <= alpha && alpha <= 1.0 );
		
		const DisplayObjectState& previousState = getPreviousState();
		DisplayObjectState currentState = getCurrentState();
		
		return previousState.getTweenedStateTo( currentState, alpha, tweener );
	}

	template< typename component_t >
	SmartPtr< component_t > DisplayObject::firstComponentOfType() const
	{
		for( const auto& component : m_components )
		{
			if( auto castComponent = dynamic_freshptr_cast< SmartPtr< component_t >>( component ))
			{
				return castComponent;
			}
		}
		return nullptr;
	}
	
	template< typename component_t, typename fn_t >
	bool DisplayObject::forEachComponent( fn_t&& fn ) const
	{
		if( !m_components.empty() )	// Optimization: skip the whole function if we have no components.
		{
			decltype( m_components ) copy( m_components );	// Make a copy so that changes to m_components won't disturb the loop.
			for( const auto& component : copy )
			{
				if( auto castComponent = dynamic_freshptr_cast< SmartPtr< component_t >>( component ))
				{
					bool overridden = fn( castComponent );
					
					if( overridden )
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	inline vec2 DisplayObject::parentAttachOffset() const
	{
		if( m_ignoreFrameAttachment || !parent() )
		{
			return vec2::ZERO;
		}
		else
		{
			return properParentAttachOffset();
		}
	}
}


#endif
