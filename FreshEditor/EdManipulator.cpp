//
//  EdManipulator.cpp
//  Fresh
//
//  Created by Jeff Wofford on 2/8/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "EdManipulator.h"
#include "Editor.h"
#include "CommandProcessor.h"
#include "EdBoxSelector.h"
#include "FreshFile.h"
#include "Application.h"
#include "EdGroup.h"
#include "EdSelectionHarness.h"
#include "EdTimeline.h"

#if DEV_MODE && 0
#	define touch_trace( expression ) trace( expression )
#else
#	define touch_trace( expression )
#endif

namespace
{
	
	using namespace fr;
	
	const real MIN_GIZMO_CIRCLE_RADIUS = 32.0f;
	const real MAX_GIZMO_CIRCLE_RADIUS = 400.0f;
	const real WHEEL_TO_ZOOM_CHANGE_FACTOR = 1500.0f;
	
	const vec2 PASTE_OFFSET( 0 );
	
	const real NUDGE_SCALAR = 1.25f;		// Results in a nudge of 1/2 at the narrowest currently allowed scale.
	

	struct FunctorRenderFadeAncestors : public DisplayObject::RenderInjector
	{
		explicit FunctorRenderFadeAncestors( DisplayObject::wptr drawRoot )
		:	m_drawRoot( drawRoot )
		,	m_drawDepth( -1 )
		{}
		
		virtual bool preDrawOverride( TimeType relativeFrameTime, DisplayObject& object ) override
		{
			if( m_drawDepth < 0 )
			{
				if( m_drawRoot == &object )
				{
					// Start drawing properly.
					//
					m_drawDepth = 0;
				}
			}
			else
			{
				++m_drawDepth;
			}
			
			return false;
		}
		
		virtual bool drawOverride( TimeType relativeFrameTime, DisplayObject& object ) override
		{
			return m_drawDepth < 0;
		}
		
		virtual bool postDrawOverride( DisplayObject& object ) override
		{
			if( m_drawDepth >= 0 )
			{
				--m_drawDepth;
			}
			
			return false;
		}
	private:
		
		DisplayObject::wptr m_drawRoot;
		int m_drawDepth;
	};
	
	struct FunctorRenderSelected : public DisplayObject::RenderInjector
	{
		explicit FunctorRenderSelected( Editor& editor, const std::set< DisplayObject::wptr >& selectedObjects )
		:	m_selectedObjects( selectedObjects )
		{
			m_selectionDecorator->setTextureByName( "white_simple" );
			m_selectionDecorator->blendMode( Renderer::BlendMode::Add );
			m_selectionDecorator->isTouchEnabled( false );
			m_selectionDecorator->visible( false );
			
			editor.addChild( m_selectionDecorator );
		}
		
		virtual bool preDrawOverride( TimeType relativeFrameTime, DisplayObject& object ) override
		{
			++m_depth;
			return false;
		}
		
		virtual bool drawOverride( TimeType relativeFrameTime, DisplayObject& object ) override
		{
			return false;
		}
		
		virtual bool postDrawOverride( DisplayObject& object ) override
		{
			if( m_selectedObjects.find( &object ) != m_selectedObjects.end() )
			{
				const rect& bounds = object.localBounds();
				m_selectionDecorator->position( bounds.midpoint() );
				m_selectionDecorator->scale( bounds.dimensions() * 0.5f );

				m_selectionDecorator->color( object.editorLocked() ? Color( 0.5f, 0.4f, 0.52f, 0.125f ) :
																	 Color( 0.0f, 0.25f, 1.0f, 0.5f ));
				
				m_selectionDecorator->visible( true );
				m_selectionDecorator->render( 1.0 );
			}
			m_selectionDecorator->visible( false );
			
			--m_depth;
			return false;
		}
		
		virtual ~FunctorRenderSelected()
		{
			m_selectionDecorator->markForDeletion();
		}
		
	private:
		
		const std::set< DisplayObject::wptr >& m_selectedObjects;
		Color m_storedColor;
		int m_depth = 0;
		Sprite::ptr m_selectionDecorator = createObject< Sprite >( "selection decorator" );
	};
	
}

namespace fr
{
	FRESH_DEFINE_CLASS_UNPLACEABLE( ManipulatorComponent );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( ManipulatorComponent )
	
	FRESH_DEFINE_CLASS_UNPLACEABLE( TouchProxy );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( TouchProxy )

	/////////////////////////////////////////////
	
	FRESH_DEFINE_CLASS_UNPLACEABLE( Manipulator )
	DEFINE_VAR( Manipulator, WeakPtr< class Stage >, m_root );
	DEFINE_VAR( Manipulator, DisplayObject::wptr, m_base );
	DEFINE_VAR( Manipulator, DisplayObject::wptr, m_subject );
	DEFINE_VAR( Manipulator, SmartPtr< EdGizmoButton >, m_gizmoButtonRotate );
	DEFINE_VAR( Manipulator, SmartPtr< EdGizmoButton >, m_gizmoButtonScale );
	DEFINE_VAR( Manipulator, SmartPtr< EdGizmoButton >, m_gizmoButtonDelete );
	DEFINE_VAR( Manipulator, Sprite::ptr, m_editingBackground );
	DEFINE_VAR( Manipulator, Sprite::ptr, m_subjectOriginDisplay );
	DEFINE_VAR( Manipulator, Sprite::ptr, m_markupHost );
	DEFINE_VAR( Manipulator, DisplayObjectContainer::ptr, m_touchProxyHost );
	DEFINE_VAR( Manipulator, real, m_scaleGizmoButtons );
	DEFINE_VAR( Manipulator, real, m_touchProxySize );
	DEFINE_VAR( Manipulator, SmartPtr< EdBoxSelector >, m_boxSelector );
	DEFINE_VAR( Manipulator, bool, m_showGizmoScale );
	DEFINE_VAR( Manipulator, DisplayObject::ptr, m_toolbar );
	DEFINE_VAR( Manipulator, DisplayObjectContainer::ptr, m_harnessHost );
	DEFINE_VAR( Manipulator, ClassInfo::cptr, m_touchProxyClass );
	DEFINE_VAR( Manipulator, ClassMap, m_harnessClassMappings );
	
	DEFINE_METHOD( Manipulator, selectAll )
	DEFINE_METHOD( Manipulator, deselectAll )
	
	DEFINE_METHOD( Manipulator, cutSelected )
	DEFINE_METHOD( Manipulator, copySelected )
	DEFINE_METHOD( Manipulator, paste )
	DEFINE_METHOD( Manipulator, deleteSelected )

	Manipulator::Manipulator( CreateInertObject c )
	:	Super( c )
	{
		isDragEnabled( true );
	}
	
	Manipulator::Manipulator( const ClassInfo& assignedClassInfo, NameRef name )
	:	Super( assignedClassInfo, name )
	{
		isDragEnabled( true );
	}
	
	Manipulator::~Manipulator()		// Implemented here to support forward declarations in header.
	{}
	
	void Manipulator::setup( Editor::wptr editor )
	{
		m_editor = editor;
		ASSERT( m_editor );
		
		m_root = m_editor->root();
		m_base = m_editor->base();
		m_subject = m_editor->current();

		onSubjectChildrenAddedOrRemoved();
		
		m_injector = std::make_shared< FunctorRenderFadeAncestors >( m_subject );
		m_injector->setNext( std::make_shared< FunctorRenderSelected >( *m_editor, m_selectedObjects ));
		
		m_editor->addEventListener( EventTouch::TOUCH_BEGIN, FRESH_CALLBACK( onSpaceTouchBegin ));
		m_editor->addEventListener( TAPPED, FRESH_CALLBACK( onSpaceTapped ));
		m_editor->addEventListener( DRAG_BEGIN, FRESH_CALLBACK( onSpaceDragBegin ));
		m_editor->addEventListener( DRAG_MOVE, FRESH_CALLBACK( onSpaceDragMove ));
		m_editor->addEventListener( DRAG_END, FRESH_CALLBACK( onSpaceDragEnd ));
		m_editor->addEventListener( EventTouch::WHEEL_MOVE, FRESH_CALLBACK( onAnythingWheelMove ));

		createEditingBackground();
		
		if( m_editor->propertyPane() )
		{
			m_editor->propertyPane()->addEventListener( PropertyPane::SUBJECTS_CHANGED, FRESH_CALLBACK( onSubjectsChangedByPropertyPane ));
		}
		
		if( m_gizmoButtonRotate )
		{
			m_gizmoButtons.push_back( m_gizmoButtonRotate );
			
			m_gizmoButtonRotate->addEventListener( DRAG_BEGIN, FRESH_CALLBACK( onGizmoRotateButtonDown ));
			m_gizmoButtonRotate->addEventListener( DRAG_END, FRESH_CALLBACK( onGizmoRotateButtonUp ));
			m_gizmoButtonRotate->addEventListener( EdGizmoButton::UPDATED, FRESH_CALLBACK( onGizmoRotateButtonMove ));
			m_gizmoButtonRotate->visible( false );
		}
		
		if( m_gizmoButtonScale )
		{
			m_gizmoButtons.push_back( m_gizmoButtonScale );
			
			m_gizmoButtonScale->addEventListener( DRAG_BEGIN, FRESH_CALLBACK( onGizmoScaleButtonDown ));
			m_gizmoButtonScale->addEventListener( DRAG_END, FRESH_CALLBACK( onGizmoScaleButtonUp ));
			m_gizmoButtonScale->addEventListener( EdGizmoButton::UPDATED, FRESH_CALLBACK( onGizmoScaleButtonMove ));
			m_gizmoButtonScale->visible( false );
		}
		
		if( m_gizmoButtonDelete )
		{
			m_gizmoButtons.push_back( m_gizmoButtonDelete );
			
			m_gizmoButtonDelete->addEventListener( SimpleButton::TAPPED, FRESH_CALLBACK( onGizmoDeleteButtonTapped ));
			m_gizmoButtonDelete->visible( false );
		}
		
		ASSERT( m_toolbar );
		m_editor->setToolbar( m_toolbar );
		
		drawSubjectOrigin();
		
		m_selectedObjects.clear();
		onObjectsWereSelectedOrDeselected();
		
		if( m_editor->virtualKeys() )
		{
			virtualKeys().addEventListener( EventVirtualKey::VIRTUAL_KEY_DOWN, FRESH_CALLBACK( onVirtualKeyDown ));
			virtualKeys().addEventListener( EventVirtualKey::VIRTUAL_KEY_UP, FRESH_CALLBACK( onVirtualKeyUp ));
		}
	}
	
	void Manipulator::update()
	{
		if( visible() )
		{
			updateTouchProxies();
			
			// Gizmo buttons should anti-scale to stay the same size in screen space.
			//
			const vec2 gizmoButtonScale = 1.0f / scale() * m_scaleGizmoButtons;
			
			for( auto gizmoButton : m_gizmoButtons )
			{
				gizmoButton->scale( gizmoButtonScale );
			}
		}
		
		Super::update();
	}

	real Manipulator::minZoomScale() const
	{
		return 0.01f;
	}
	
	real Manipulator::maxZoomScale() const
	{
		return 10.0f;
	}

	bool Manipulator::hitTestPoint( const vec2& localLocation, HitTestFlags flags ) const
	{
		if( Super::hitTestPoint( localLocation, flags ))
		{
			return true;
		}
		else if( m_subject )
		{
			const vec2 subjectLocation = localToSubject( localLocation );
			return m_subject->hitTestPoint( subjectLocation, flags );
		}
		else
		{
			return false;
		}
	}
	
	rect Manipulator::localBounds() const
	{
		rect bounds = Super::localBounds();
		
		if( m_subject )
		{
			bounds.growToEncompass( subjectToLocal( m_subject->localBounds() ));
		}
		
		return bounds;
	}
	
	void Manipulator::render( TimeType relativeFrameTime, RenderInjector* injector )
	{
		ASSERT( !injector );
		
		if( !doesWantToRender() ) return;
		
		if( m_subject )
		{
			ASSERT( m_root );
			
			// Transform into my space.
			//
			setupTransforms( relativeFrameTime );
			
			// Draw background, if any.
			//
			if( m_editingBackground )
			{
				m_editingBackground->render( relativeFrameTime, injector );
			}
			
			// Draw subject
			//
			m_root->recordPreviousState( true /* recursive */ );		// Force subject to ignore past render states and tweening.
			m_root->render( relativeFrameTime, m_injector.get() );
			
			// Finish transforms.
			//
			unsetupTransforms();
		}
		
		Super::render( relativeFrameTime, injector );
	}
	
	DisplayObject::ptr Manipulator::getSubjectTouchTarget( const vec2& location ) const
	{
		// Are we touching any touch proxies?
		//
		if( m_touchProxyHost )
		{
			// Go in reverse order to see nearest proxies first.
			for( int i = m_touchProxyHost->numChildren() - 1; i >= 0; --i )
			{
				TouchProxy::ptr proxy = dynamic_freshptr_cast< TouchProxy::ptr >( m_touchProxyHost->getChildAt( i ) );
				ASSERT( proxy );
				
				if( proxy->hitTestPoint( proxy->parentToLocal( location ), HTF_RequireEditable ))
				{
					return dynamic_freshptr_cast< DisplayObject::ptr >( proxy->forObject() );
				}
			}
		}
		
		DisplayObjectContainer::ptr container = dynamic_freshptr_cast< DisplayObjectContainer::ptr >( m_subject );
		if( container )
		{
			// Which, if any, of the subject's children are being touched?
			//
			const vec2 SubjectLocation = localToSubject( location );
			DisplayObject::ptr target = container->getTopChildUnderPoint( SubjectLocation, HTF_RequireEditable );
			
			if( target )
			{
				return target;
			}
		}
		
		return nullptr;
	}
	
	bool Manipulator::isSelected( DisplayObject::ptr object ) const
	{
		return m_selectedObjects.find( object ) != m_selectedObjects.end();
	}
	
	void Manipulator::select( DisplayObject::ptr object )
	{
		TIMER_AUTO( Manipulator::select )
		
		ASSERT( m_selectedObjectChangeStartState.empty() );
		
		// Refuse to be selected if locked.
		//
		if( !object->editorLocked() && m_selectedObjects.find( object ) == m_selectedObjects.end() )
		{
			m_selectedObjects.insert( object );
			onObjectsWereSelectedOrDeselected();
		}
	}
	
	void Manipulator::deselect( DisplayObject::ptr object )
	{
		ASSERT( m_selectedObjectChangeStartState.empty() );
		
		if( m_selectedObjects.erase( object ) )
		{
			onObjectsWereSelectedOrDeselected();
		}
	}
	
	void Manipulator::toggleSelection( DisplayObject::ptr object )
	{
		// Don't know whether to add or remove.
		// Try erasing this object and see how many actually get erased...
		//
		const size_t nErased = m_selectedObjects.erase( object );
		if( nErased == 0 && !object->editorLocked() )
		{
			// None got erased, so it needs to be added.
			m_selectedObjects.insert( object );
		}
		onObjectsWereSelectedOrDeselected();
	}
	
	void Manipulator::selectAll()
	{
		if( m_isChanging )
		{
			return;
		}
		
		ASSERT( m_selectedObjectChangeStartState.empty() );

		DisplayObjectContainer::ptr container = dynamic_freshptr_cast< DisplayObjectContainer::ptr >( m_subject );
		if( container )
		{
			for( size_t i = 0; i < container->numChildren(); ++i )
			{
				select( container->getChildAt( i ));
			}
		}
	}
	
	void Manipulator::deselectAll()
	{
		if( m_isChanging )
		{
			return;
		}
		
		if( !m_selectedObjects.empty() )
		{
			m_selectedObjects.clear();
			onObjectsWereSelectedOrDeselected();
		}
	}
	
	void Manipulator::onSelectedObjectsChanged()
	{
		TIMER_AUTO( Manipulator::onSelectedObjectsChanged )

		updateHarnesses();

		updateSelectionMarkup();
		
		if( m_editor->propertyPane() )
		{
			m_editor->propertyPane()->updatePane();
		}
	}
	
	void Manipulator::onObjectsChangedExternally()
	{
		updateHarnessCreation();
		updateHarnesses();

		// Cleanup touch proxies.
		//
		updateTouchProxies();
		
		// Cleanup selected objects.
		//
		size_t nDeleted = 0;
		for( auto iter = m_selectedObjects.begin(); iter != m_selectedObjects.end(); /* iteration within */ )
		{
			if( iter->isNull() )
			{
				++nDeleted;
				iter = m_selectedObjects.erase( iter);
			}
			else
			{
				++iter;
			}
		}
		
		// If the actual members of m_selectedObjects changed, update panes and such.
		//
		onSubjectChildrenAddedOrRemoved();
	}
	
	void Manipulator::onObjectsWereSelectedOrDeselected()
	{
		TIMER_AUTO( Manipulator::onObjectsWereSelectedOrDeselected )

		updateHarnessCreation();
		
		for( auto gizmoButton : m_gizmoButtons )
		{
			gizmoButton->resetOrbitPosition();
		}
		
		updateSelectionMarkup();

		// Update the property pane.
		//
		if( m_editor->propertyPane() )
		{
			PropertyPane::Subjects subjects( m_selectedObjects.begin(), m_selectedObjects.end() );
			m_editor->propertyPane()->beginEditing( subjects );
		}
				
		onSelectedObjectsChanged();
		
		if( m_editor->timeline() )
		{
			m_editor->timeline()->onObjectsWereSelectedOrDeselected( m_selectedObjects );
		}
	}

	void Manipulator::onSubjectChildrenAddedOrRemoved()
	{
		if( m_editor->timeline() )
		{
			m_editor->timeline()->setup( m_subject, m_base->as< DisplayObjectContainer >() );
		}
		
		onObjectsWereSelectedOrDeselected();
	}
	
	void Manipulator::createEditingBackground()
	{
		ASSERT( m_root );

		if( m_root->hasStage() && m_editingBackground )
		{
			Stage& rootStage = m_root->stage();
			
			m_editingBackground->scale( rootStage.stageDimensions() * 0.5f );
			m_editingBackground->color( rootStage.clearColor() );
		}
	}
	
	void Manipulator::updateSelectionMarkup()
	{
		TIMER_AUTO( Manipulator::updateSelectionMarkup )

		ASSERT( m_markupHost );
		
		Graphics& markupGraphics = m_markupHost->graphics();
		markupGraphics.clear();
		
		if( !m_selectedObjects.empty() )
		{
			vec2 circleCenter;
			real circleRadius = MAX_GIZMO_CIRCLE_RADIUS;
			
			// Find the minimum bounding circle that encompasses all selected objects' bounding boxes.
			//
			std::vector< vec2 > boxPoints;
			boxPoints.reserve( m_selectedObjects.size() * 4 );
			
			for( auto selectedObject : m_selectedObjects )
			{
				ASSERT( selectedObject );
				const rect bounds = selectedObject->localBounds();
				
				size_t iCorner = boxPoints.size();	// Prepare to traverse the points I'm about to add.
				
				// Add the corners of the piece.
				//
				boxPoints.push_back( bounds.ulCorner() );
				boxPoints.push_back( bounds.brCorner() );
				boxPoints.push_back( bounds.urCorner() );
				boxPoints.push_back( bounds.blCorner() );
				
				// Transform the corners from the object's space to subject space.
				//
				while( iCorner < boxPoints.size() )
				{
					vec2& corner = boxPoints[ iCorner ];
					
					corner = m_subject->globalToLocal( selectedObject->localToGlobal( corner ));
					
					++iCorner;
				}
			}
			
			if( m_selectedObjects.size() == 1 )
			{
				const auto& theObject = *(*m_selectedObjects.begin());
				
				circleCenter = m_subject->globalToLocal( theObject.localToGlobal( theObject.pivot() ));
				circleRadius = findMinimumBoundingCircleRadiusWithCenter( circleCenter, boxPoints.begin(), boxPoints.end() );
			}
			else
			{
				findMinimumBoundingCircle( boxPoints.begin(), boxPoints.end(), /*out*/ circleCenter, /*out*/ circleRadius );
			}
			
			// Transform the radius as a point to subject space.
			//
			vec2 transformedRadius( circleRadius, 0 );
			circleRadius = m_subject->localToGlobalPoint( transformedRadius ).length();
			
			// Constrain the circle radius not to be too big or too small.
			//
			const vec2 circleRadiusBounds = globalToLocalPoint( vec2( MIN_GIZMO_CIRCLE_RADIUS, MAX_GIZMO_CIRCLE_RADIUS ));
			
			circleRadius = clamp( circleRadius, circleRadiusBounds.x, circleRadiusBounds.y );
			
			// Draw the circle.
			//
			circleCenter = subjectToLocal( circleCenter );
			markupGraphics.drawCircle( circleCenter, circleRadius, circleRadius, 64 );
			
			// Update the gizmo buttons.
			//
			for( auto gizmoButton : m_gizmoButtons )
			{
				gizmoButton->visible( true );
				gizmoButton->orbitCenter( circleCenter );
				gizmoButton->orbitRadius( circleRadius );
				gizmoButton->updateForConstraintChange();
			}

			if( m_gizmoButtonScale )
			{
				m_gizmoButtonScale->visible( m_showGizmoScale );
			}
		}
		else
		{
			for( auto gizmoButton : m_gizmoButtons )
			{
				gizmoButton->visible( false );
			}
		}
	}
	
	void Manipulator::drawSubjectOrigin()
	{
		ASSERT( m_subject );
		ASSERT( m_subjectOriginDisplay );
		
		m_subjectOriginDisplay->position( m_subject->localToGlobal( vec2::ZERO ));
		
		Graphics& graphics = m_subjectOriginDisplay->graphics();
		
		graphics.clear();
		
		const real size = 6.0f;
		graphics.lineStyle( Color::LightGray );
		graphics.moveTo( -size,  0.0f );
		graphics.lineTo(  size,  0.0f );
		graphics.moveTo(  0.0f, -size );
		graphics.lineTo(  0.0f,  size );
	}
	
	VirtualKeys& Manipulator::virtualKeys() const
	{
		ASSERT( m_editor->virtualKeys() );
		return *m_editor->virtualKeys();
	}
	
	DisplayObjectContainer::ptr Manipulator::subjectAsContainer() const
	{
		return dynamic_freshptr_cast< DisplayObjectContainer::ptr >( m_subject );
	}
	
	TouchProxy::ptr Manipulator::findTouchProxyForSubjectChild( DisplayObject::ptr child ) const
	{
		REQUIRES( child );
		if( m_touchProxyHost )
		{
			for( auto iter : *m_touchProxyHost )
			{
				TouchProxy::ptr proxy = dynamic_freshptr_cast< TouchProxy::ptr >( iter );
				ASSERT( proxy );
				if( proxy->forObject() == child )
				{
					return proxy;
				}
			}
		}
		return nullptr;
	}
	
	void Manipulator::updateTouchProxies()
	{
		ASSERT( m_subject );
		
		// For each of the subject's children, make sure that very small objects (in screen space) receive a touch proxy,
		// and that others don't.
		//
		if( m_touchProxyHost && m_touchProxyClass )
		{
			// Mark all touch proxies invisible.
			//
			for( auto proxy : *m_touchProxyHost )
			{
				proxy->visible( false );
			}
			
			DisplayObjectContainer::ptr subjectContainer = subjectAsContainer();
			
			if( subjectContainer )
			{
				for( auto child : *subjectContainer )
				{
					if( !child->isEditable() )	// No touch proxies for non-editable objects, please.
					{
						continue;
					}
					
					// Is this object small in world space?
					rect childBounds = child->localBounds();
					rect childBoundsSubject = m_subject->globalToLocal( child->localToGlobal( childBounds ));
					rect childBoundsEditorSpace = localToGlobal( subjectToLocal( childBoundsSubject ));
					
					const real boundsSize = average( childBoundsEditorSpace.width(), childBoundsEditorSpace.height() );
					
					const bool wantsTouchProxy = boundsSize <= m_touchProxySize;
					
					if( wantsTouchProxy )
					{
						TouchProxy::ptr proxy = findTouchProxyForSubjectChild( child );
						
						if( !proxy )
						{
							// Make one.
							//
							proxy = createObject< TouchProxy >( *m_touchProxyClass );
							ASSERT( proxy );
							proxy->forObject( child );
							m_touchProxyHost->addChild( proxy );
						}
						
						// Update the proxy's transformation.
						//
						proxy->position( subjectToLocal( m_subject->globalToLocal( child->localToGlobal( child->pivot() ))));
						proxy->rotation( subjectToLocal( m_subject->globalToLocal( child->parent()->localToGlobal( child->rotation() ))));
						proxy->scale( 1.0f / scale() * getProxySizeToScaleFactor( proxy, m_touchProxySize ));
						proxy->visible( true );
					}
				}
			}
			
			// Remove any still-invisible proxies.
			//
			for( size_t i = 0; i < m_touchProxyHost->numChildren(); ++i )
			{
				auto proxy = m_touchProxyHost->getChildAt( i );
				if( !proxy->visible() )
				{
					m_touchProxyHost->removeChildAt( i );
					--i;
				}
			}
		}
	}
	
	void Manipulator::checkBoxSelectionMove( const vec2& localSpacePoint )
	{
		if( !m_isDraggingHUD && m_boxSelector && m_boxSelector->visible() )
		{
			// Update box selection.
			//
			m_boxSelector->endPoint( localSpacePoint );
			
			updateSelectionInBox( m_boxSelector->selectionRectangle() );
		}
	}

	void Manipulator::updateSelectionInBox( const rect& selectionRectangle )
	{
		ASSERT( m_boxSelector );
		
		// For every object inside the box...
		//
		DisplayObjectContainer::ptr subjectContainer = subjectAsContainer();
		
		if( subjectContainer )
		{
			const bool isInvertingSelection = virtualKeys().isVirtualKeyDown( "EditorModifier - SelectionInvert" );
			
			const rect SubjectRect = localToSubject( selectionRectangle );
			
			EdBoxSelector::SelectedObjects overlappingObjects;
			
			for( size_t i = 0; i < subjectContainer->numChildren(); ++i )
			{
				auto subjectChild = subjectContainer->getChildAt( i );
				
				if( !subjectChild->isEditable() )
				{
					continue;
				}
				
				const bool wasOriginallySelected = m_selectedObjectsAtBoxSelectionStart.find( subjectChild ) != m_selectedObjectsAtBoxSelectionStart.end();
				
				const bool overlaps = SubjectRect.doesOverlap( subjectChild->bounds() );
				
				bool selected;
				
				if( overlaps )
				{
					selected = true;
					
					if( isInvertingSelection )
					{
						selected = !wasOriginallySelected;
					}
				}
				else
				{
					selected = wasOriginallySelected;
				}
	
				if( selected )
				{
					select( subjectChild );
				}
				else
				{
					deselect( subjectChild );
				}
			}
		}
	}
	
	void Manipulator::endBoxSelection()
	{
		if( m_boxSelector )
		{
			m_boxSelector->visible( false );
		}
		m_selectedObjectsAtBoxSelectionStart.clear();
	}
	
	////////////////////////////////////////////////////////////////////////////////////////////////
	// MANIPULATOR INPUT HANDLING
	////////////////////////////////////////////////////////////////////////////////////////////////
	
	// All touches begin in one of two general places:
	//		(1) in the manipulator
	//		(2) "below" the manipulator.
	// These are further distinguished into two classes each, giving for touch start categories:
	//		(1.1) within the subject itself
	//		(1.2) within the manipulator's user interface
	//		(2.1) within the blank space of the editor
	//		(2.2) within some actual child of the editor.
	// Awkwardly, case 2.2 includes the case where the manipulator is touched, since the manipulator
	// is a child of the editor. But we don't need to dwell on this distinction since the manipulator
	// uniformly ignores case 2.2.
	//
	// The cases can each be distinguished according to the callback function for which they result in a
	// call and the data situation within that callback.
	//		(1.1) onTouchBegin(), where getTopDescendantUnderPoint() returns null.
	//		(1.2) onTouchBegin(), where it doesn't. We handle this case in the typical (i.e. base class's)
	//			  way.
	//		(2.1) onTouchBeginAnywhere(), where event.target() == m_editor.
	//
	// In practice this means that there are only two interesting cases: 1.1 and 2.1, which are easily
	// distinguishable.
	//
	// The next complication comes from the challenge of distinguishing between "taps" and "drags."
	// A drag begins when a tap is ruled out either when the touch moves too far from its origin
	// or when too much time has passed. This gives us two classes of "gesture":
	//		(A) tap
	//		(B) drag
	// Taps can be dealt with through a single event, whereas a drag has three parts: begin, move, and end.
	// (Taps are further distinguished between single- and double-taps, but this is not a significant
	// complication.)
	//
	// So there is some necessary complication in converting underlying touch notification functions into
	// four classes of handler functions, one for each (tap | drag) and one for each (subject | space).
	//		onSubjectTapped()
	//		onSpaceTapped()
	//		onSubjectDragBegin()	onSubjectDragMove()		onSubjectDragEnd()
	//		onSpaceDragBegin()		onSpaceDragMove()		onSpaceDragEnd()
	//
	
	// SEMANTIC INPUT HANDLING /////////////////////////////////////////////////////////////////////////////
	
	void Manipulator::onSubjectTapped( const EventTouch& event )
	{
		touch_trace( "onSubjectTapped( " << event.target() << " )" );
		
		ASSERT( m_touchTarget );
		
		if( event.tapCount() == 1 )
		{
			if( !isSelected( m_touchTarget ) && !virtualKeys().isVirtualKeyDown( "EditorModifier - AppendSelection" ) )
			{
				deselectAll();
			}
			
			toggleSelection( m_touchTarget );
		}
		else if( event.tapCount() == 2 )
		{
			m_editor->editChild( m_touchTarget );
		}
		
		m_touchTarget = nullptr;
	}
	
	void Manipulator::onSubjectDragBegin( const EventTouch& event )
	{
		touch_trace( "onSubjectDragBegin( " << event.target() << " )" );
		
		if( m_isChanging || m_editor->isTouchActionInProgress() ) return;
		ASSERT( m_touchTarget );

		// If the touch target isn't selected, make it the only selected thing.
		//
		if( !isSelected( m_touchTarget ))
		{
			deselectAll();
			select( m_touchTarget );
		}
		
		const bool tearAway = virtualKeys().isVirtualKeyDown( "EditorModifier - TearAwayCopy" );
		
		if( tearAway )
		{
			if( !m_selectedObjects.empty() )
			{
			
				auto manifest = getSelectionManifest();				// Copy
				createFromManifest( manifest );						// Paste
				
				ASSERT( !m_selectedObjects.empty() );
				
				m_touchTarget = *m_selectedObjects.begin();
			}
		}

		beginChangingObjects();
	}
	
	void Manipulator::onSubjectDragMove( const EventTouch& event )
	{
		touch_trace( "onSubjectDragMove( " << event.target() << " )" );

		TIMER_AUTO( Manipulator::onSubjectDragMove )

		// Update piece dragging.
		//
		ASSERT( m_touchTarget );
		
		const vec2 originalStartLocation = localToSubject( m_touchStartLocation );
		const vec2 endLocation = localToSubject( event.location() );
		
		vec2 delta = endLocation - originalStartLocation;
		
		const bool isLockingMovement = virtualKeys().isVirtualKeyDown( "EditorModifier - LockMovementToIntervals" );
		
		if( isLockingMovement )
		{
			if( delta.lengthSquared() < 4.0f * 4.0f )
			{
				delta.setToZero();
			}
			
			delta.snapToMajorAxis();
		}
		
		for( auto object : m_selectedObjects )
		{
			// Move the object.
			//
			if( m_selectedObjectChangeStartState.find( object ) != m_selectedObjectChangeStartState.end() )
			{
				if( !object->editorLocked() )
				{
					object->position( m_selectedObjectChangeStartState[ object ].position() + delta );
				}
			}
		}
		
		// Redraw selection boxes.
		//
		onSelectedObjectsChanged();
	}
	
	void Manipulator::onSubjectDragEnd( const EventTouch& event )
	{
		touch_trace( "onSubjectDragEnd( " << event.target() << " )" );

		endChanging();
		m_touchTarget = nullptr;

		updateHarnesses();
		
		m_editor->saveHistoryState();
	}
	
	FRESH_DEFINE_CALLBACK( Manipulator, onSpaceTouchBegin, EventTouch )
	{
		if( event.target() != m_editor ) return;
		if( !visible() ) return;
		if( m_editor->isEditedRootPlaying() ) return;

		touch_trace( "onSpaceTouchBegin( " << event.target() << " )" );
		
		if( isChanging() )
		{
			// Must not have received an "end" event.
			//
			onSpaceDragEnd( event );
		}

		if( !m_touchTarget )
		{
			const bool wantsDragCamera = virtualKeys().isVirtualKeyDown( "EditorModifier - CameraMove" );
			if( wantsDragCamera )
			{
				beginChanging();
				m_isDraggingCamera = true;
				m_touchStartLocation = event.location();	// Editor space.
				m_cameraStartDragLocation = position();
			}
		}
	}
	
	FRESH_DEFINE_CALLBACK( Manipulator, onSpaceTapped, EventTouch )
	{
		if( event.target() != m_editor ) return;
		if( !visible() ) return;
		if( m_editor->isEditedRootPlaying() ) return;

		touch_trace( "onSpaceTapped( " << event.target() << " )" );

		if( !m_isDraggingCamera )
		{
			// "Space" between elements was tapped.
			//
			if( event.tapCount() == 1 )
			{
				deselectAll();
			}
			else if( event.tapCount() == 2 )
			{
				m_editor->editParent();
			}
		}
		
		endChanging();
	}
	
	FRESH_DEFINE_CALLBACK( Manipulator, onSpaceDragBegin, EventTouch )
	{
		if( event.target() != m_editor ) return;
		if( !visible() ) return;
		if( m_isChanging || m_editor->isTouchActionInProgress() ) return;
		if( m_isDraggingHUD ) return;
		if( m_touchTarget ) return;		
		if( m_editor->isEditedRootPlaying() ) return;
	
		touch_trace( "onSpaceDragBegin( " << event.target() << " )" );
		
		m_editor->addEventListener( EventTouch::TOUCH_END, FRESH_CALLBACK( onTouchEndAnywhere ));

		const bool wantsDragCamera = virtualKeys().isVirtualKeyDown( "EditorModifier - CameraMove" );
		if( wantsDragCamera )
		{
			beginChanging();
			m_isDraggingCamera = true;
			m_touchStartLocation = event.location();	// Editor space.
			m_cameraStartDragLocation = position();
		}
		else
		{
			beginChanging();
			
			// Start box selection.
			//
			if( !m_boxSelector )
			{
				m_boxSelector = createObject< EdBoxSelector >();
				addChild( m_boxSelector );
			}
			
			m_boxSelector->visible( true );
			
			m_selectedObjectsAtBoxSelectionStart = m_selectedObjects;
			
			const auto& selectionPoint = parentToLocal( event.location() );
			m_boxSelector->startPoint( selectionPoint );
			m_boxSelector->endPoint( selectionPoint );
		}
	}
	
	FRESH_DEFINE_CALLBACK( Manipulator, onSpaceDragMove, EventTouch )
	{
		if( event.target() != m_editor ) return;
		if( !visible() ) return;
		if( m_editor->isEditedRootPlaying() ) return;
		
		touch_trace( "onSpaceDragMove( " << event.target() << " )" );

		if( m_touchTarget )
		{
			EventTouch transformedEvent( event, event.target(), parentToLocal( event.location() ), parentToLocal( event.previousLocation() ) );
			onSubjectDragMove( transformedEvent );
			return;
		}

		if( m_isDraggingCamera )
		{
			const vec2 movementDelta = event.location() - m_touchStartLocation;			// Editor space.
			position( m_cameraStartDragLocation + movementDelta );
		}
		else
		{
			checkBoxSelectionMove( parentToLocal( event.location() ));
		}
	}
	
	FRESH_DEFINE_CALLBACK( Manipulator, onSpaceDragEnd, EventTouch )
	{
		if( event.target() != m_editor ) return;
		if( !visible() ) return;
		if( m_editor->isEditedRootPlaying() ) return;
		
		touch_trace( "onSpaceDragEnd( " << event.target() << " )" );

		if( m_touchTarget )
		{
			EventTouch transformedEvent( event, event.target(), parentToLocal( event.location() ), parentToLocal( event.previousLocation() ) );
			onSubjectDragEnd( transformedEvent );
		}

		endBoxSelection();

		endChanging();
	}
	
	FRESH_DEFINE_CALLBACK( Manipulator, onAnythingWheelMove, EventTouch )
	{
		if( m_editor->isEditedRootPlaying() ) return;
		
		// Only respond if the target is within the edited world proper.
		//
		DisplayObject::ptr target = dynamic_freshptr_cast< DisplayObject::ptr >( event.target() );
		bool isInEditorWorld = target && ( target == m_editor || target == this || hasDescendant( target ));
		
		if( !isInEditorWorld )
		{
			return;
		}
		
		const vec2 focalPosition = event.location();
		
		const real zoomChange = -event.wheelDelta().y / WHEEL_TO_ZOOM_CHANGE_FACTOR;
		
		const real oldScale = scale().x;
		
		real newScale = oldScale;
		
		newScale *= 1.0f + clamp( zoomChange, -0.5f, 0.5f );
		
		newScale = clamp( newScale, minZoomScale(), maxZoomScale() );
		
		scale( newScale );

		position((( position() - focalPosition ) / oldScale ) * newScale + focalPosition );
		
		updateSelectionMarkup();
	}
	
	FRESH_DEFINE_CALLBACK( Manipulator, onTouchEndAnywhere, EventTouch )
	{
		touch_trace( "onTouchEndAnywhere( " << event.target() << " )" );
		m_editor->removeEventListener( EventTouch::TOUCH_END, FRESH_CALLBACK( onTouchEndAnywhere ));
		
		if( m_editor->isEditedRootPlaying() ) return;

		endBoxSelection();
		endChanging();
	}


	// RAW INPUT HANDLING //////////////////////////////////////////////////////////////////////////////////
	
	void Manipulator::onTouchBegin( const EventTouch& event )
	{
		// Called when a touch goes down on some element of the manipulator itself:
		// Either the subject or a touch proxy.
		
		if( m_editor->isTouchActionInProgress() ) return;
		if( m_editor->isEditedRootPlaying() ) return;
		
		touch_trace( "onTouchBegin( " << event.target() << " )" );

		TIMER_AUTO( Manipulator::onTouchBegin )
		
		m_editor->addEventListener( EventTouch::TOUCH_END, FRESH_CALLBACK( onTouchEndAnywhere ));
		                            
		const bool wantsDragCamera = virtualKeys().isVirtualKeyDown( "EditorModifier - CameraMove" );
		if( wantsDragCamera )
		{
			m_touchTarget = nullptr;
			beginChanging();
			m_isDraggingCamera = true;
			m_touchStartLocation = localToParent( event.location());	// Editor space.
			m_cameraStartDragLocation = position();
		}
		else
		{
			Super::onTouchBegin( event );
			
			// Only entertain subject dragging if we're not touching something else.
			//
			DisplayObject::ptr target = dynamic_freshptr_cast< DisplayObject::ptr >( event.target() );
			
			m_isDraggingHUD = target && isPartOfManipulatorHUD( *target );

			if( !m_isDraggingHUD )
			{
				const bool mightBeInSubject = !m_isChanging && target && ( target == this || ( m_touchProxyHost && m_touchProxyHost->hasDescendant( target ) ));
				if( mightBeInSubject )
				{
					m_touchTarget = getSubjectTouchTarget( event.location() );	// May be null.
					m_touchStartLocation = event.location();
				}
				else
				{
					for( auto gizmoButton : m_gizmoButtons )
					{
						if( gizmoButton->hasDescendant( target ))
						{
							m_isDraggingHUD = true;
							break;
						}
					}
				}
			}
		}
	}
		
	void Manipulator::onTouchMove( const EventTouch& event )
	{
		// Called when a touch moves over the subject or a proxy.
		// onTouchBegin() *might not* have been called previously.

		if( m_editor->isEditedRootPlaying() ) return;

		touch_trace( "onTouchMove( " << event.target() << " )" );

		if( m_isDraggingCamera )
		{
			const vec2 movementDelta = localToParent( event.location() ) - m_touchStartLocation;			// Editor space.
			position( m_cameraStartDragLocation + movementDelta );
		}
		else
		{
			Super::onTouchMove( event );
			checkBoxSelectionMove( event.location() );
		}
	}

	void Manipulator::onTouchEnd( const EventTouch& event )
	{
		// Called when a touch is lifted over the subject or a proxy.
		// onTouchBegin() and onTouchMove() *might not* have been called previously.

		if( m_editor->isEditedRootPlaying() ) return;
		
		touch_trace( "onTouchEnd( " << event.target() << " )" );

		Super::onTouchEnd( event );
		
		endBoxSelection();
		endChanging();
	}

	void Manipulator::onTapped( const EventTouch& event )
	{
		// Called just before onTouchEnd() when the touch from beginning to end was sufficiently motionless and quick.
		// If onTapped() is called, no drag functions will have been called.

		if( m_editor->isEditedRootPlaying() ) return;

		touch_trace( "onTapped( " << event.target() << " )" );

		Super::onTapped( event );
		
		if( m_touchTarget )
		{
			onSubjectTapped( event );
		}		
	}
	
	void Manipulator::onDragBegin( const EventTouch& event )
	{
		// Called just before onTouchMove() when the touch has moved significantly from its start location or
		// when sufficient time has passed.
		
		touch_trace( "onDragBegin( " << event.target() << " )" );

		if( m_touchTarget )
		{
			onSubjectDragBegin( event );
		}
		Super::onDragBegin( event );
	}
	
	void Manipulator::onDragMove( const EventTouch& event )
	{
		// Called for every move of the touch *anywhere* on the stage after onDragBegin() is called.
		
		touch_trace( "onDragMove( " << event.target() << " )" );

		checkBoxSelectionMove( event.location() );
		
		if( !m_boxSelector || !m_boxSelector->visible() )
		{
			if( m_touchTarget )
			{
				onSubjectDragMove( event );
			}
		}
		Super::onDragMove( event );
	}
	
	void Manipulator::onDragEnd( const EventTouch& event )
	{
		// Called for any lift of the touch *anywhere* on the stage after onDragBegin() is called.
		
		touch_trace( "onDragEnd( " << event.target() << " )" );

		endBoxSelection();
		
		if( m_touchTarget )
		{
			onSubjectDragEnd( event );
		}
		Super::onDragEnd( event );

		endChanging();
	}
	
	FRESH_DEFINE_CALLBACK( Manipulator, onGizmoRotateButtonDown, EventTouch )
	{
		if( event.phase() == Event::Capturing )
		{
			touch_trace( "onGizmoRotateButtonDown( " << event.target() << " )" );
			
			ASSERT( m_isDraggingHUD );
			
			beginChangingObjects();
			
			m_startGizmoRotation = m_gizmoButtonRotate->rotation();
			
			updateSelectionMarkup();
			m_startGizmoCenter = m_gizmoButtonRotate->orbitCenter();
		}
	}
	
	FRESH_DEFINE_CALLBACK( Manipulator, onGizmoRotateButtonUp, EventTouch )
	{
		touch_trace( "onGizmoRotateButtonUp( " << event.target() << " )" );
		
		endChanging();
		
		updateHarnesses();
		
		m_editor->saveHistoryState();
	}
	
	FRESH_DEFINE_CALLBACK( Manipulator, onGizmoRotateButtonMove, EventTouch )
	{
		touch_trace( "onGizmoRotateButtonMove( " << event.target() << " )" );
		ASSERT( m_isChanging );
		
		const angle gizmoRotation = m_gizmoButtonRotate->rotation();
		angle rotationDelta = gizmoRotation - m_startGizmoRotation;
		
		const bool isLockingMovement = virtualKeys().isVirtualKeyDown( "EditorModifier - LockMovementToIntervals" );
		
		if( isLockingMovement )
		{
			rotationDelta.fromDegrees( roundToNearest( rotationDelta.toDegrees< real >(), 45.0f ));
		}
		
		const vec2 orbitCenter = localToSubject( m_startGizmoCenter );

		// Rotate all selection items to reflect the gizmo's movement.
		//
		for( auto object : m_selectedObjects )
		{
			if( !object->editorLocked() )
			{
				ASSERT( m_selectedObjectChangeStartState.find( object ) != m_selectedObjectChangeStartState.end() );
				const DisplayObjectState& startState = m_selectedObjectChangeStartState[ object ];
				
				const vec2 parentAttachOffset = object->parentAttachOffset();
				
				const vec2 startDelta(( startState.position() + parentAttachOffset ) - orbitCenter );
				
				const real dist = startDelta.length();

				angle orbitRotation = startDelta.angle();
				
				orbitRotation += rotationDelta;
				
				vec2 newDelta( dist, 0 );
				newDelta.rotate( orbitRotation );
			
				object->position( orbitCenter + newDelta - parentAttachOffset );
				
				object->rotation( angle( startState.rotation() ) + rotationDelta );
			}
		}
		
		// Update the orientation of the gizmo buttons.
		//
		
		for( auto gizmoButton : m_gizmoButtons )
		{
			if( gizmoButton != m_gizmoButtonRotate )
			{
				gizmoButton->rotation( gizmoRotation + ( gizmoButton->defaultRotation() - m_gizmoButtonRotate->defaultRotation() ));
				gizmoButton->updateForConstraintChange();
			}
		}
		
		onSelectedObjectsChanged();
	}
	
	FRESH_DEFINE_CALLBACK( Manipulator, onGizmoScaleButtonDown, EventTouch )
	{
		if( event.phase() == Event::Capturing )
		{
			touch_trace( "onGizmoScaleButtonDown( " << event.target() << " )" );
		
			ASSERT( m_isDraggingHUD );

			beginChangingObjects();
			m_startGizmoDistance = m_gizmoButtonScale->orbitRadius();
			m_startGizmoCenter = m_gizmoButtonRotate->orbitCenter();
		}
	}
	
	FRESH_DEFINE_CALLBACK( Manipulator, onGizmoScaleButtonUp, EventTouch )
	{
		touch_trace( "onGizmoScaleButtonUp( " << event.target() << " )" );
		
		endChanging();

		updateHarnesses();

		m_editor->saveHistoryState();
	}
	 
	FRESH_DEFINE_CALLBACK( Manipulator, onGizmoScaleButtonMove, EventTouch )
	{
		touch_trace( "onGizmoScaleButtonMove( " << event.target() << " )" );
		
		ASSERT( m_isChanging );

		const real gizmoDistance = m_gizmoButtonScale->orbitRadius();
		const vec2 orbitCenter = localToSubject( m_startGizmoCenter );

		real distanceScale = gizmoDistance / m_startGizmoDistance;
		
		const bool isLockingMovement = virtualKeys().isVirtualKeyDown( "EditorModifier - LockMovementToIntervals" );

		if( isLockingMovement )
		{
			// TODO would be nicer if it rounded to the nearest 1 / power of 2 when distanceScale < 1.
			distanceScale = std::max( 0.5f, roundToNearest( distanceScale, 0.5f ) );
		}
		
		// Scale all selection items to reflect the gizmo's movement.
		//
		for( auto object : m_selectedObjects )
		{
			if( !object->editorLocked() )
			{
				ASSERT( m_selectedObjectChangeStartState.find( object ) != m_selectedObjectChangeStartState.end() );
				const DisplayObjectState& startState = m_selectedObjectChangeStartState[ object ];
				
				const vec2 parentAttachOffset = object->parentAttachOffset();
				
				const vec2 startDelta(( startState.position() + parentAttachOffset ) - orbitCenter );
				
				const real startDist = startDelta.length();

				const vec2 newDelta = startDelta.normal() * ( startDist * distanceScale );
				
				object->position( orbitCenter + newDelta - parentAttachOffset );
				object->scale( startState.scale() * distanceScale );
			}
		}
		
		onSelectedObjectsChanged();
	}

	FRESH_DEFINE_CALLBACK( Manipulator, onGizmoDeleteButtonTapped, EventTouch )
	{
		if( event.phase() == Event::Capturing )
		{
			touch_trace( "onGizmoDeleteButtonTapped( " << event.target() << " )" );
			deleteSelected();
			
			m_isDraggingHUD = false;
		}
	}
	
	FRESH_DEFINE_CALLBACK( Manipulator, onSubjectsChangedByPropertyPane, Event )
	{
		updateHarnesses();
		m_editor->saveHistoryState();
	}
	
	DisplayObject::ptr Manipulator::createObjectOfClass( const ClassInfo& objectClass, const vec2& location )
	{
		DisplayObject::ptr newObject;
		
		DisplayObjectContainer::ptr subjectContainer = subjectAsContainer();
		
		if( subjectContainer )
		{
			// Create an object of the selected class at this location.
			//
			// Begin dragging this new piece.
			//
			ASSERT( m_editor->classInventory() );
			
			const vec2 subjectLocation = localToSubject( location );
			
			//
			// Create the object.
			//
			
			// Establish the editedPackage as the "capturing" package so that Object detritus that are
			// created as a result of this object being created are also placed into this package.
			//
			newObject = createObject< DisplayObject >( &m_editor->editedPackage(), objectClass );
			ASSERT( newObject );
			
			newObject->position( subjectLocation );
			subjectContainer->addChild( newObject );
			
			onSubjectChildrenAddedOrRemoved();
			
			dev_trace( "Created " << newObject << " as a child of " << (*newObject->parent()) << " at " << newObject->position() );
		}
		else
		{
			// Warn that this is impossible.
			//
			dev_warning( "Cannot create child object in non-container object " << m_subject );
		}
		return newObject;
	}

	DisplayObject::ptr Manipulator::createObjectOfSelectedClass( const vec2& location )
	{
		const std::string creationType = m_editor->classInventory()->getSelectedClassName();
		ASSERT( !creationType.empty() );
		
		auto creationClass = getClass( creationType );
		ASSERT( creationClass );
		
		if( creationClass->isAbstract() )
		{
			dev_warning( this << " cannot create instance of abstract class '" << creationClass->className() << "'." );
			return nullptr;
		}
		
		return createObjectOfClass( *creationClass, location );
	}
	
	template< typename transformed_t >
	inline transformed_t localToSubjectTyped( const DisplayObject& root, const DisplayObject& subject, const transformed_t& transformed )
	{
		return subject.globalToLocal( root.localToGlobal( root.parentToLocal( transformed )));
	}

	template< typename transformed_t >
	inline transformed_t subjectToLocalTyped( const DisplayObject& root, const DisplayObject& subject, const transformed_t& transformed )
	{
		return root.localToParent( root.globalToLocal( subject.localToGlobal( transformed )));
	}
	
	vec2 Manipulator::localToSubject( const vec2& location ) const
	{
		ASSERT( m_root );
		ASSERT( m_subject );
		return localToSubjectTyped( *m_root, *m_subject, location );
	}
	
	vec2 Manipulator::subjectToLocal( const vec2& location ) const
	{
		ASSERT( m_root );
		ASSERT( m_subject );
		return subjectToLocalTyped( *m_root, *m_subject, location );
	}
	
	rect Manipulator::localToSubject( const rect& r ) const
	{
		ASSERT( m_root );
		ASSERT( m_subject );
		return localToSubjectTyped( *m_root, *m_subject, r );
	}
	
	rect Manipulator::subjectToLocal( const rect& r ) const
	{
		ASSERT( m_root );
		ASSERT( m_subject );
		return subjectToLocalTyped( *m_root, *m_subject, r );
	}
	
	angle Manipulator::localToSubject( angle a ) const
	{
		ASSERT( m_root );
		ASSERT( m_subject );
		return localToSubjectTyped( *m_root, *m_subject, a );
	}
	
	angle Manipulator::subjectToLocal( angle a ) const
	{
		ASSERT( m_root );
		ASSERT( m_subject );
		return subjectToLocalTyped( *m_root, *m_subject, a );
	}
	
	vec2 Manipulator::localToSubjectPoint( const vec2& point ) const
	{
		ASSERT( m_root );
		ASSERT( m_subject );
		return m_subject->globalToLocalPoint( m_root->localToGlobalPoint( m_root->parentToLocalPoint( point )));
	}
	
	vec2 Manipulator::subjectToLocalPoint( const vec2& point ) const
	{
		ASSERT( m_root );
		ASSERT( m_subject );
		return m_root->localToParentPoint( m_root->globalToLocalPoint( m_subject->localToGlobalPoint( point )));
	}
	
	vec2 Manipulator::localToSubjectScale( const vec2& scale ) const
	{
		ASSERT( m_root );
		ASSERT( m_subject );
		return m_subject->globalToLocalScale( m_root->localToGlobalScale( m_root->parentToLocalScale( scale )));
	}
	
	vec2 Manipulator::subjectToLocalScale( const vec2& scale ) const
	{
		ASSERT( m_root );
		ASSERT( m_subject );
		return m_root->localToParentScale( m_root->globalToLocalScale( m_subject->localToGlobalScale( scale )));
	}

	void Manipulator::cutSelected()
	{
		copySelected();
		deleteSelected();
	}
	
	void Manipulator::copySelected()
	{
		auto manifest = getSelectionManifest();
		copyToPasteboard( manifest );
	}
	
	void Manipulator::paste()
	{
		createFromManifest( pasteFromPasteboard() );
		m_editor->saveHistoryState();
	}
	
	void Manipulator::deleteSelected()
	{
		for( auto selectedObject : m_selectedObjects )
		{
			auto selected = dynamic_freshptr_cast< DisplayObject::ptr >( selectedObject );
			selectedObject->parent()->removeChild( selected );
			if( auto proxy = findTouchProxyForSubjectChild( selected ))
			{
				m_touchProxyHost->removeChild( proxy );
			}
		}
		m_selectedObjects.clear();
		
		onSubjectChildrenAddedOrRemoved();
		m_editor->saveHistoryState();
	}
	
	void Manipulator::groupSelected( DisplayObjectContainer::ptr group )
	{
		DisplayObjectContainer::ptr subjectContainer = subjectAsContainer();
		if( subjectContainer && !m_isChanging && !m_selectedObjects.empty() )
		{
			// Create a group to hold the selection.
			//
			if( !group )
			{
				group = createObject< EdGroup >( m_editor->editedPackage() );
			}
			
			auto orderedObjects = selectedObjectsInDepthOrder();
			
			// Place selected objects into the group.
			//
			for( DisplayObject::ptr object : orderedObjects )		// Can't use auto here because we actually need to keep the object alive, and auto translates to WeakPtr< DisplayObject >.
			{
				ASSERT( object );
				if( auto childsParent = object->parent() )
				{
					childsParent->removeChild( object );
				}
				group->addChild( object );
			}
			
			group->rectify();
			
			subjectContainer->addChild( group );
			
			onSubjectChildrenAddedOrRemoved();
			
			// Adjust selection.
			//
			deselectAll();
			select( group );
			
			m_editor->saveHistoryState();
		}
	}
	
	void Manipulator::ungroupSelected( ClassInfo::cptr groupClass )
	{
		DisplayObjectContainer::ptr subjectContainer = subjectAsContainer();
		if( !m_isChanging && subjectContainer )
		{
			if( !groupClass )
			{
				groupClass = &EdGroup::StaticGetClassInfo();
			}
			
			// For every selected object that is also a group...
			//
			decltype( m_selectedObjects ) newlySelectedObjects;
			
			size_t nGroupsUngrouped = 0;
			
			for( auto iter = m_selectedObjects.begin(); iter != m_selectedObjects.end(); ++iter )
			{
				auto object = *iter;
				if( auto group = object->as< DisplayObjectContainer >() )
				{
					if( group->isA( *groupClass ))
					{
						ASSERT( subjectContainer == group->parent() );
						
						const size_t groupDepth = group->parent()->getChildIndex( group );
						const size_t nDisgorgedObjects = group->disgorge();
						
						// Record the newly created objects.
						//
						for( size_t i = 0; i < nDisgorgedObjects; ++i )
						{
							auto child = subjectContainer->getChildAt( groupDepth + i );
							
							if( !child->editorLocked() )
							{
								newlySelectedObjects.insert( child );
							}
						}
						
						++nGroupsUngrouped;
					}
				}
			}
			
			if( nGroupsUngrouped > 0 )
			{
				m_selectedObjects = newlySelectedObjects;

				onSubjectChildrenAddedOrRemoved();
				m_editor->saveHistoryState();
			}
		}
	}
	
	void Manipulator::lockSelected( bool doLockElseUnlock )
	{
		if( !m_isChanging )
		{
			for( auto object : m_selectedObjects )
			{
				object->editorLocked( doLockElseUnlock );
			}
			
			if( doLockElseUnlock )	// If locking, stop selecting.
			{
				m_selectedObjects.clear();
				onObjectsWereSelectedOrDeselected();
			}
			
			m_editor->saveHistoryState();
		}
	}
	
	std::string Manipulator::getSelectionManifest() const
	{
		DisplayObjectContainer::ptr subjectContainer = subjectAsContainer();
		ASSERT( subjectContainer );
		
		DisplayPackage& editedPackage = m_editor->editedPackage();
		
		DisplayObjectContainer::ptr selectionRoot = createObject< DisplayObjectContainer >( "selection_root" );
		
		DisplayPackage::ptr selectionPackage = createPackage< DisplayPackage >();
		selectionPackage->root( selectionRoot );
		
		std::map< const DisplayObject::ptr, int > selectedObjectDepths;
		
		// Add all selected objects as pending to the saver.
		//
		for( auto object : m_selectedObjects )
		{
			// Remember the object's order.
			//
			selectedObjectDepths[ DisplayObject::ptr( object ) ] = subjectContainer->getChildIndex( object );
			
			// Detach the object from the subject.
			//
			subjectContainer->removeChild( object );
			
			// Insert it into the selection root.
			//
			selectionRoot->addChild( object );
		}
		
		// Bring in objects that the selected objects depend on.
		//
		selectionPackage->collect( [&] ( const Object& object ) -> bool
								   {
									   DisplayObject::cptr displayable( dynamic_cast< const DisplayObject* >( &object ));
									   
									   // We include this object in the package iff:
									   
									   return selectionRoot == displayable ||					// It's the selection root itself OR
									   (
										editedPackage.has( &object ) &&						// It's part of the edited package AND
										// AT LEAST ONE OF:
										(
										 !displayable	||								// Not a display object.
										 displayable->hasAncestor( selectionRoot )		// Is "above" one of the selected objects in the display tree.
										 )
										);
								   } );
		
		// Actually save the objects.
		//
		std::ostringstream saveState;
		selectionPackage->save( saveState );
		
		// Add the objects back into the subject with the same depths as before.
		//
		std::vector< DisplayObject::ptr > selectedObjectsInOrder( m_selectedObjects.begin(), m_selectedObjects.end() );
		std::sort( selectedObjectsInOrder.begin(), selectedObjectsInOrder.end(), [&]( DisplayObject::ptr a, DisplayObject::ptr b )
				  {
					  return selectedObjectDepths[ a ] < selectedObjectDepths[ b ];
				  } );
		
		for( auto object : selectedObjectsInOrder )
		{
			selectionRoot->removeChild( object );
			subjectContainer->addChildAt( object, selectedObjectDepths[ DisplayObject::ptr( object ) ] );
		}
		
		return saveState.str();
	}
	
	void Manipulator::createFromManifest( const std::string& manifest )
	{
		DisplayObjectContainer::ptr subjectContainer = subjectAsContainer();
		
		if( subjectContainer )
		{
			deselectAll();
			
			// Convert to XML.
			//
			auto element = stringToXmlElement( manifest );
			
			if( element )
			{
				// Load these objects as a package.
				//
				DisplayPackage::ptr selectionPackage = createPackage< DisplayPackage >();
				selectionPackage->loadFromElement( *element );
				
				DisplayObjectContainer::ptr selectionRoot = dynamic_freshptr_cast< DisplayObjectContainer::ptr >( selectionPackage->root());
				
				if( selectionRoot )
				{
					DisplayPackage& editedPackage = m_editor->editedPackage();
					
					// Merge objects from the paste package into the edited package, renaming as needed.
					//
					editedPackage.merge( *selectionPackage, Package::MergePolicy::KeepBothRenamingNew );
					
					// Move the objects from the selection root into the subject.
					//
					while( selectionRoot->numChildren() > 0 )
					{
						DisplayObject::ptr displayObject = selectionRoot->getChildAt( 0 );
						selectionRoot->removeChildAt( 0 );
						
						displayObject->position( displayObject->position() + PASTE_OFFSET );
						
						subjectContainer->addChild( displayObject );
						onSubjectChildrenAddedOrRemoved();
						
						select( displayObject );
					}
					
				}
				else
				{
					dev_warning( "Paste package contained no selection root." );
				}
			}
		}
	}
	
	void Manipulator::nudgeSelection( const vec2& step )
	{
		const vec2 adjustedStep = localToSubjectPoint( step / scale() * NUDGE_SCALAR );
		
		for( auto selectedObject : m_selectedObjects )
		{
			selectedObject->position( selectedObject->position() + adjustedStep );
		}
		
		if( !m_selectedObjects.empty() )
		{
			onSelectedObjectsChanged();
			m_editor->saveHistoryState();
		}
	}
	
	real Manipulator::getProxySizeToScaleFactor( Sprite::ptr proxy, real size )
	{
		if( Texture::ptr proxyTexture = proxy->texture() )
		{
			return size / ( std::min( proxyTexture->getOriginalDimensions().x, proxyTexture->getOriginalDimensions().y ));
		}
		else
		{
			return 1.0f;
		}
	}

	void Manipulator::beginChangingObjects()
	{
		beginChanging();
		
		for( auto object : m_selectedObjects )
		{
			ASSERT( object );
			m_selectedObjectChangeStartState[ object ].setStateFrom( *object );
		}
	}
	
	void Manipulator::beginChanging()
	{
		trace( "beginChanging" );
		
		
		ASSERT( !m_isChanging );
		ASSERT( !m_isDraggingCamera );
		ASSERT( m_selectedObjectChangeStartState.empty() );
		ASSERT( !m_boxSelector || !m_boxSelector->visible() );
		
		m_isChanging = true;
		m_editor->beginTouchAction();
	}
	
	void Manipulator::endChanging()
	{
		trace( "endChanging" );
		
		if( m_isChanging )
		{
			m_selectedObjectChangeStartState.clear();
			m_isChanging = false;
			m_isDraggingCamera = false;
			m_isDraggingHUD = false;
			m_editor->endTouchAction();
		}
	}

	bool Manipulator::isPartOfManipulatorHUD( const DisplayObject& object ) const
	{
		return false;
	}
	
	std::vector< DisplayObject::wptr > Manipulator::selectedObjectsInDepthOrder() const
	{
		std::vector< DisplayObject::wptr > objects;
		std::copy_if( m_selectedObjects.begin(), m_selectedObjects.end(), std::back_inserter( objects ), [&]( const DisplayObject::wptr& p )
		{
			return !p.isNull();
		} );
		
		std::sort( objects.begin(), objects.end(), [&]( const DisplayObject::wptr& a, const DisplayObject::wptr& b )
				  {
					  ASSERT( a && b );
					  const auto theirParent = a->parent();
					  ASSERT( theirParent && theirParent == b->parent() );
					  return theirParent->getChildIndex( a ) < theirParent->getChildIndex( b );
				  } );
		
		return objects;
	}

	FRESH_DEFINE_CALLBACK( Manipulator, onVirtualKeyDown, EventVirtualKey )
	{
		if( !visible() ) return;
		if( m_editor->isEditedRootPlaying() ) return;
		if( stage().keyboardFocusHolder() ) return;
		
		bool overridden = forEachManipulatorComponent( std::bind( &ManipulatorComponent::onVirtualKeyDown,
											   std::placeholders::_1,
											   std::ref( *this ),
											   std::ref( event )));
		
		if( !overridden )
		{
			const auto& keyName = event.keyName();
			
			if( keyName == "NudgeUp" )
			{
				nudgeSelection( vec2( 0, -1 ));
			}
			else if( keyName == "NudgeDown" )
			{
				nudgeSelection( vec2( 0, 1 ));
			}
			else if( keyName == "NudgeLeft" )
			{
				nudgeSelection( vec2( -1, 0 ));
			}
			else if( keyName == "NudgeRight" )
			{
				nudgeSelection( vec2( 1, 0 ));
			}
		}
	}
	
	FRESH_DEFINE_CALLBACK( Manipulator, onVirtualKeyUp, EventVirtualKey )
	{
		if( !visible() ) return;
		if( m_editor->isEditedRootPlaying() ) return;
		if( stage().keyboardFocusHolder() ) return;
		
		bool overridden = forEachManipulatorComponent( std::bind( &ManipulatorComponent::onVirtualKeyUp,
											   std::placeholders::_1,
											   std::ref( *this ),
											   std::ref( event )));
		
		if( !overridden )
		{
			const auto& keyName = event.keyName();
			
			if( keyName == "CreateGroup" )
			{
				groupSelected();
			}
			else if( keyName == "DisgorgeGroup" )
			{
				ungroupSelected();
			}
			else if( keyName == "LockSelected" )
			{
				lockSelected( true );
			}
			else if( keyName == "UnlockSelected" )
			{
				lockSelected( false );
			}
		}
	}
	
	
	ClassName Manipulator::harnessClassForClass( ClassNameRef forClass ) const
	{
		ClassInfo::cptr const classInfo = getClass( forClass );
		ASSERT( classInfo );
		
		// Walk through looking for the closest match.
		//
		int bestDepth = std::numeric_limits< int >::max();
		ClassName bestHarnessClass;
		
		for( const auto& pair : m_harnessClassMappings )
		{
			ClassInfo::cptr const possibleSelectionClass = getClass( pair.first );
			const int depth = classInfo->getSuperClassDepth( *possibleSelectionClass );
			
			if( depth >= 0 && depth < bestDepth )
			{
				bestDepth = depth;
				bestHarnessClass = pair.second;
			}
		}
		
		return bestHarnessClass;
	}
	
	void Manipulator::addHarnessClass( ClassNameRef forClass, ClassNameRef harnessClass )
	{
		m_harnessClassMappings[ forClass ] = harnessClass;
	}

	SmartPtr< EdSelectionHarness > Manipulator::harnessForSubjectChild( DisplayObject::cwptr child ) const
	{
		ASSERT( m_harnessHost );
		if( child )
		{
			for( auto harness : *m_harnessHost )
			{
				if( auto actualHarness = harness->as< EdSelectionHarness >())
				{
					if( actualHarness->selection() == child )
					{
						return actualHarness;
					}
				}
			}
		}
		return nullptr;
	}
	
	void Manipulator::updateHarnessCreation()
	{
		ASSERT( m_harnessHost );
		
		// Hide all harnesses.
		//
		for( auto harness : *m_harnessHost )
		{
			harness->visible( false );
		}
		
		// Create harnesses for all selected objects.
		//
		for( auto selected : m_selectedObjects )
		{
			if( selected )
			{
				auto harness = harnessForSubjectChild( selected );
				if( !harness )
				{
					// Create a new harness.
					//
					auto harnessClassName = harnessClassForClass( selected->className() );
					
					if( !harnessClassName.empty() )
					{
						auto harnessClass = getClass( harnessClassName );
						if( harnessClass )
						{
							harness = createObject< EdSelectionHarness >( *harnessClass );
							m_harnessHost->addChild( harness );
							ASSERT( harness );
							harness->manipulator( this );
							harness->selection( selected );
						}
						else
						{
							con_error( this << " had unrecognized harness class '" << harnessClassName << "' for selected class '" << selected->className() << "'." );
						}
					}
				}
				else
				{
					harness->visible( true );
				}
			}
		}
		
		// Kill invisible (i.e. unused) harnesses.
		//
		for( size_t i = 0; i < m_harnessHost->numChildren(); ++i )
		{
			auto harness = m_harnessHost->getChildAt( i )->as< EdSelectionHarness >();
			if( !harness->selection() || !harness->visible() )
			{
				m_harnessHost->removeChildAt( i );
				--i;
			}
		}
	}
	
	void Manipulator::updateHarnesses()
	{
		for( auto child : *m_harnessHost )
		{
			auto harness = child->as< EdSelectionHarness >();
			ASSERT( harness );
			harness->updateFromSelection();
		}
	}
	
	vec2 Manipulator::transformPoint( const DisplayObject& from, const DisplayObject& to, const vec2& point ) const
	{
		if( !&from || !&to || &from == &to )
		{
			return point;
		}
		else
		{
			auto globalPoint = from.localToGlobal( point );
			
			auto subjectContainer = subjectAsContainer();
			
			if( subjectContainer )
			{
				if( &from == subjectContainer || from.hasAncestor( subjectContainer ))
				{
					if( &to == subjectContainer || to.hasAncestor( subjectContainer ))
					{
						// Subject space to subject space.
						//
						// No further change.
					}
					else
					{
						// Subject space to outside space.
						//
						auto SubjectPoint = subjectContainer->globalToLocal( globalPoint );
						auto manipulatorSpacePoint = subjectToLocal( SubjectPoint );
						globalPoint = localToGlobal( manipulatorSpacePoint );
					}
				}
				else if( &to == subjectContainer || to.hasAncestor( subjectContainer ))
				{
					// Outside to subject space.
					//
					auto manipulatorSpacePoint = globalToLocal( globalPoint );
					globalPoint = subjectContainer->localToGlobal( localToSubject( manipulatorSpacePoint ));
				}
			}
			
			return to.globalToLocal( globalPoint );
		}
	}
	
	
}

