//
//  EdManipulator.h
//  Fresh
//
//  Created by Jeff Wofford on 2/8/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_EdManipulator_h
#define Fresh_EdManipulator_h

#include "Sprite.h"
#include "EdGizmoButton.h"
#include "UIRadioButtons.h"
#include "FreshTime.h"
#include "VirtualKeys.h"

namespace fr
{
	
	class Manipulator;
	class ManipulatorComponent : public DisplayObjectComponent
	{
		FRESH_DECLARE_CLASS( ManipulatorComponent, DisplayObjectComponent );
	public:
		
		virtual bool onVirtualKeyDown( Manipulator& host, const EventVirtualKey& event ) { return false; }
		virtual bool onVirtualKeyUp( Manipulator& host, const EventVirtualKey& event ) { return false; }
	};
	
	/////////////////////////////////////////////
	
	class TouchProxy : public Sprite
	{
		FRESH_DECLARE_CLASS( TouchProxy, Sprite );
	public:
		
		SYNTHESIZE( DisplayObject::wptr, forObject );
				
	private:
		
		DisplayObject::wptr m_forObject;
	};

	/////////////////////////////////////////////
	
	class Editor;
	class EdBoxSelector;
	class EdSelectionHarness;
	
	// A Manipulator is a stand-in for a DisplayObject that is being edited.
	// The edited object is the Manipulator's "subject".
	// The Manipulator handles redirecting and reinterpreting input events
	// that would otherwise go to the DisplayObject for ordinary processing.
	// It also avoids updating its subject, and renders the subject in addition to its other children.
	// In effect, the Manipulator handles the job of making an edited object act in an
	// edited way--which would be the Editor's job, but the Manipulator allows
	// this task to vary depending on the class of the subject.
	//
	class Manipulator : public Sprite
	{
	public:
		
		virtual ~Manipulator();
		
		SYNTHESIZE_GET( WeakPtr< Editor >, editor );
		SYNTHESIZE_GET( DisplayObject::wptr, subject );
		SYNTHESIZE_GET( bool, isChanging );
		
		virtual void setup( WeakPtr< Editor > editor );
		
		virtual bool hitTestPoint( const vec2& localLocation, HitTestFlags flags ) const override;
		virtual rect localBounds() const override;
		virtual void render( TimeType relativeFrameTime, RenderInjector* injector = nullptr ) override;
		
		virtual bool isSelected( DisplayObject::ptr object ) const;
		virtual void select( DisplayObject::ptr object );
		virtual void deselect( DisplayObject::ptr object );
		virtual void toggleSelection( DisplayObject::ptr object );
		virtual void selectAll();
		virtual void deselectAll();
		
		virtual void cutSelected();
		virtual void copySelected();
		virtual void paste();
		virtual void deleteSelected();
		
		virtual void groupSelected( DisplayObjectContainer::ptr group = nullptr );
		// If !group, an EdGroup is created.
		virtual void ungroupSelected( ClassInfo::cptr groupClass = nullptr );
		// !groupClass, EdGroup is assumed.
		
		virtual void lockSelected( bool doLockElseUnlock );
		
		std::string getSelectionManifest() const;
		void createFromManifest( const std::string& manifest );
		
		virtual void nudgeSelection( const vec2& step );
		
		virtual void onTouchBegin( const EventTouch& event ) override;
		virtual void onTouchMove( const EventTouch& event ) override;
		virtual void onTouchEnd( const EventTouch& event ) override;

		virtual void onDragBegin( const EventTouch& event ) override;
		virtual void onDragMove( const EventTouch& event ) override;
		virtual void onDragEnd( const EventTouch& event ) override;
		
		virtual void update() override;
		
		virtual real minZoomScale() const;
		virtual real maxZoomScale() const;

		virtual void onSelectedObjectsChanged();			// Called when the properties of one or more of the selected objects changed, or when the list of selected objects itself changed.

		virtual void onObjectsChangedExternally();			// Called when the subject may have gained or lost children through some external means (e.g. editor object undo or redo).
		
		DisplayObject::ptr createObjectOfClass( const ClassInfo& objectClass, const vec2& location );
		
		vec2 localToSubject( const vec2& location ) const;
		vec2 subjectToLocal( const vec2& location ) const;
		rect localToSubject( const rect& r ) const;
		rect subjectToLocal( const rect& r ) const;
		angle localToSubject( angle a ) const;
		angle subjectToLocal( angle a ) const;
		vec2 localToSubjectPoint( const vec2& point ) const;
		vec2 subjectToLocalPoint( const vec2& point ) const;
		vec2 localToSubjectScale( const vec2& scale ) const;
		vec2 subjectToLocalScale( const vec2& scale ) const;

		ClassName harnessClassForClass( ClassNameRef forClass ) const;
		void addHarnessClass( ClassNameRef forClass, ClassNameRef harnessClass );
		
		vec2 transformPoint( const DisplayObject& from, const DisplayObject& to, const vec2& point ) const;

	protected:
		
		VirtualKeys& virtualKeys() const;
		
		DisplayObjectContainer::ptr subjectAsContainer() const;
		
		virtual DisplayObject::ptr getSubjectTouchTarget( const vec2& location ) const;
		virtual void onObjectsWereSelectedOrDeselected();	// Called iff the list of selected objects changed.
		virtual void onSubjectChildrenAddedOrRemoved();		// Called iff a child may have been added or removed from the subject.
		
		virtual void createEditingBackground();
		
		virtual void updateSelectionMarkup();
		
		DisplayObject::ptr createObjectOfSelectedClass( const vec2& location );
		
		TimeType realTime() const
		{
			return clocksToSeconds( fr::getAbsoluteTimeClocks() - m_startTimeReal );
		}
		
		void drawSubjectOrigin();
		
		TouchProxy::ptr findTouchProxyForSubjectChild( DisplayObject::ptr child ) const;
		void updateTouchProxies();

		void checkBoxSelectionMove( const vec2& localSpacePoint );
		void updateSelectionInBox( const rect& selectionRectangle );
		void endBoxSelection();
		
		static real getProxySizeToScaleFactor( Sprite::ptr proxy, real size );

		void beginChangingObjects();
		void beginChanging();
		void endChanging();
		
		virtual bool isPartOfManipulatorHUD( const DisplayObject& object ) const;
		
		std::vector< DisplayObject::wptr > selectedObjectsInDepthOrder() const;

		SmartPtr< EdSelectionHarness > harnessForSubjectChild( DisplayObject::cwptr child ) const;
		virtual void updateHarnessCreation();
		virtual void updateHarnesses();
		
		template< typename fn_t >
		bool forEachManipulatorComponent( fn_t&& fn ) const;

	private:
		
		VAR( WeakPtr< class Stage >, m_root );
		VAR( DisplayObject::wptr, m_base );
		VAR( DisplayObject::wptr, m_subject );
		VAR( SmartPtr< EdGizmoButton >, m_gizmoButtonRotate );
		VAR( SmartPtr< EdGizmoButton >, m_gizmoButtonScale );
		VAR( SmartPtr< EdGizmoButton >, m_gizmoButtonDelete );
		VAR( Sprite::ptr, m_editingBackground );
		VAR( Sprite::ptr, m_subjectOriginDisplay );
		VAR( Sprite::ptr, m_markupHost );
		VAR( DisplayObjectContainer::ptr, m_touchProxyHost );
		VAR( DisplayObject::ptr, m_toolbar );
		VAR( DisplayObjectContainer::ptr, m_harnessHost );
		VAR( ClassInfo::cptr, m_touchProxyClass );
		DVAR( real, m_scaleGizmoButtons, 0.4f );
		DVAR( real, m_touchProxySize, 30.0f );
		VAR( SmartPtr< EdBoxSelector >, m_boxSelector );
		DVAR( bool, m_showGizmoScale, true );

		typedef std::map< ClassName, ClassName > ClassMap;
		VAR( ClassMap, m_harnessClassMappings );
		
		std::set< DisplayObject::wptr >						m_selectedObjects;
		std::map< DisplayObject::wptr, DisplayObjectState >	m_selectedObjectChangeStartState;

		std::set< DisplayObject::wptr >						m_selectedObjectsAtBoxSelectionStart;

		WeakPtr< Editor > m_editor;
		
		DisplayObject::ptr m_touchTarget;
		vec2 m_touchStartLocation = vec2::ZERO;
		
		vec2 m_cameraStartDragLocation;
		
		angle m_startGizmoRotation;
		vec2  m_startGizmoCenter;
		real  m_startGizmoDistance;
		
		fr::SystemClock m_startTimeReal = fr::getAbsoluteTimeClocks();
		
		std::shared_ptr< fr::DisplayObject::RenderInjector > m_injector;
		
		std::vector< SmartPtr< EdGizmoButton >> m_gizmoButtons;
		
		bool m_isChanging = false;
		bool m_isDraggingCamera = false;
		bool m_isDraggingHUD = false;
		
		virtual void onTapped( const EventTouch& event ) override;

		FRESH_DECLARE_CALLBACK( Manipulator, onVirtualKeyDown, EventVirtualKey );
		FRESH_DECLARE_CALLBACK( Manipulator, onVirtualKeyUp, EventVirtualKey );

		FRESH_DECLARE_CALLBACK( Manipulator, onGizmoRotateButtonDown, EventTouch );
		FRESH_DECLARE_CALLBACK( Manipulator, onGizmoRotateButtonUp, EventTouch );
		FRESH_DECLARE_CALLBACK( Manipulator, onGizmoRotateButtonMove, EventTouch );
		
		FRESH_DECLARE_CALLBACK( Manipulator, onGizmoScaleButtonDown, EventTouch );
		FRESH_DECLARE_CALLBACK( Manipulator, onGizmoScaleButtonUp, EventTouch );
		FRESH_DECLARE_CALLBACK( Manipulator, onGizmoScaleButtonMove, EventTouch );
		
		FRESH_DECLARE_CALLBACK( Manipulator, onGizmoDeleteButtonTapped, EventTouch );
		
		FRESH_DECLARE_CALLBACK( Manipulator, onSubjectsChangedByPropertyPane, Event );	

		// SEMANTIC INPUT HANDLING
		
		virtual void onSubjectTapped( const EventTouch& event );
		virtual void onSubjectDragBegin( const EventTouch& event );
		virtual void onSubjectDragMove( const EventTouch& event );
		virtual void onSubjectDragEnd( const EventTouch& event );

		FRESH_DECLARE_CALLBACK( Manipulator, onSpaceTouchBegin, EventTouch );
		FRESH_DECLARE_CALLBACK( Manipulator, onSpaceTapped, EventTouch );
		FRESH_DECLARE_CALLBACK( Manipulator, onSpaceDragBegin, EventTouch );
		FRESH_DECLARE_CALLBACK( Manipulator, onSpaceDragMove, EventTouch );
		FRESH_DECLARE_CALLBACK( Manipulator, onSpaceDragEnd, EventTouch );
		FRESH_DECLARE_CALLBACK( Manipulator, onAnythingWheelMove, EventTouch );
		FRESH_DECLARE_CALLBACK( Manipulator, onTouchEndAnywhere, EventTouch );
		
		FRESH_DECLARE_CLASS( Manipulator, Sprite );
	};

	///////////////////////////////////////////////////////////////////////////
	// IMPLEMENTATION
	//
	
	template< typename fn_t >
	bool Manipulator::forEachManipulatorComponent( fn_t&& fn ) const
	{
		return forEachComponent< ManipulatorComponent >( [&]( const ManipulatorComponent::ptr& component )
										  {
											  return fn( component );
										  } );
	}
	

}

#endif
