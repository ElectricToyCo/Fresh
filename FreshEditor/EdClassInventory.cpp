//
//  EdClassInventory.cpp
//  Fresh
//
//  Created by Jeff Wofford on 1/20/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "EdClassInventory.h"
#include "Objects.h"
#include "TextField.h"
#include "Stage.h"
#include "Editor.h"
#include "EdManipulator.h"

namespace
{
	const fr::real ROOT_OFFSET = 8;
	const fr::ObjectName PREVIEW_HOST_NAME = "~preview_host";
}

namespace fr
{
	
	
	class EdClassSelector : public Sprite
	{
		FRESH_DECLARE_CLASS( EdClassSelector, Sprite );
	public:
		
		EdClassSelector( const ClassInfo& assignedClassInfo, const ClassInfo& forClass )
		:	Sprite( assignedClassInfo, std::string( forClass.className() ) + " selector" )
		,	m_myClass( &forClass )
		,	m_textField( createObject< TextField >() )
		,	m_selectorHost( createObject< DisplayObjectContainer >( "selector host" ))
		,	m_touchCatcher( createObject< Sprite >() )
		{
			m_textField->isTouchEnabled( true );
			m_textField->setFont( "CalibriBold44" );
			m_textField->fontSize( 16 );
			m_textField->text( forClass.className() );
			m_textField->pivot( 0, m_textField->localBounds().height() * 0.5f );

			addChild( m_textField );
			
			m_touchCatcher->setTextureByName( "white_simple" );
			m_touchCatcher->blendMode( Renderer::BlendMode::AlphaPremultiplied );
			m_touchCatcher->color( Color::BarelyVisible );
			addChild( m_touchCatcher );
			
			addChild( m_selectorHost );
		}
		
		const ClassInfo& getClass() const
		{
			return *m_myClass;
		}
		
		SYNTHESIZE_GET( bool, enabled );
		void enabled( bool b )
		{
			if( m_enabled != b )
			{
				m_enabled = b;
				
				if( !m_enabled && m_selectionCallback )
				{
					removeEventListener( TAPPED, m_selectionCallback );
				}
				else if( m_selectionCallback )
				{
					addEventListener( TAPPED, m_selectionCallback );
				}
			}
		}
		
		void setSelectionListener( CallbackFunctionAbstract::ptr callback )
		{
			if( m_selectionCallback )
			{
				removeEventListener( TAPPED, m_selectionCallback );
			}
			
			m_selectionCallback = callback;
			
			if( m_enabled && callback )
			{
				addEventListener( TAPPED, m_selectionCallback );
			}
			
			forEachChildSelector( [&]( EdClassSelector& selector )
			{
				selector.setSelectionListener( callback );
			} );
		}
		
		bool addDescendantClass( const ClassInfo& descendantClass )
		{
			if( descendantClass.className() == m_myClass->className() )
			{
//				dev_warning( this << " found repeated class " << descendantClass.className() );
				return true;
			}
			
			if( descendantClass.isKindOf( *m_myClass ))
			{
				// Try adding to each child.
				//
				for( size_t i = 0; i < m_selectorHost->numChildren(); ++i )
				{
					EdClassSelector::ptr childSelector = dynamic_freshptr_cast< EdClassSelector::ptr >( m_selectorHost->getChildAt( i ));
					ASSERT( childSelector );
					
					if( childSelector->addDescendantClass( descendantClass ))
					{
						return true;
					}
				}
				
				// Failed to add a child. Add as a child, and reshuffle other children into this new child as appropriate.
				//
				EdClassSelector::ptr childSelector = new EdClassSelector( EdClassSelector::StaticGetClassInfo(), descendantClass );
				reshuffleChildrenToNewChild( *childSelector );
				m_selectorHost->addChild( childSelector );
				return true;
			}
			else
			{
				return false;
			}
		}
		
		void sort()
		{
			std::sort( m_selectorHost->begin(), m_selectorHost->end(), []( const DisplayObject::ptr a, const DisplayObject::ptr b )
					  {
						  return a->name() < b->name();
					  });
			
			forEachChildSelector( []( EdClassSelector& selector ) { selector.sort(); } );
		}
		
		void distributeChildrenVertically( real indentOffset, real gutterY )
		{
			real priorBottomY = immediateBounds().y * 0.5f;
			forEachChildSelector( [&]( EdClassSelector& selector )
			{
				const real thisHalfHeight = selector.immediateBounds().y * 0.5f;
				const real thisY = priorBottomY + thisHalfHeight + gutterY;

				selector.position( indentOffset, thisY );
				selector.distributeChildrenVertically( indentOffset, gutterY );
				
				priorBottomY = thisY + std::max( selector.immediateBounds().y * 0.5f, selector.localBounds().height() - selector.immediateBounds().y * 0.5f );
			} );
		}
		
		void highlightSelectedClassName( ClassInfo::NameRef name )
		{
			Color textColor;
			
			const bool highlightMe = ( m_myClass->className() == name );
			
			if( !m_enabled )
			{
				textColor = 0xffcccccc;
			}
			else if( !m_myClass->isNative() )
			{
				textColor = highlightMe ? 0xffffeeee : 0xffffdddd;
			}
			else
			{
				textColor = highlightMe ? Color::White : Color::LightGray;
			}
			
			m_textField->color( textColor );

			forEachChildSelector( [&]( EdClassSelector& selector )
			{
				selector.highlightSelectedClassName( name );
			} );
		}
				
		void forEachChildSelector( const std::function< void( EdClassSelector& selector ) >& fnPerSelector )
		{
			for( size_t i = 0; i < m_selectorHost->numChildren(); ++i )
			{
				EdClassSelector::ptr childSelector = dynamic_freshptr_cast< EdClassSelector::ptr >( m_selectorHost->getChildAt( i ));
				ASSERT( childSelector );
				
				fnPerSelector( *childSelector );
			}
		}
		
		virtual void onAddedToStage() override
		{
			Super::onAddedToStage();

			// Setup preview.
			//
			if( !m_myClass->isAbstract() && m_myClass->isKindOf( "DisplayObject" ))
			{
				m_preview = createObject< DisplayObject >( *m_myClass );
				
				if( m_preview )
				{
					m_preview->wantsUpdate( false );
					addChild( m_preview );
					
					// Resize preview to required max dimensions.
					//
					vec2 previewDimensions = m_preview->localBounds().dimensions();

					// If the preview has no dimensions, use a default, blank preview.
					//
					if( previewDimensions.isZero() || !previewDimensions.isFinite() )
					{
						removeChild( m_preview );
						
						auto actualPreview = m_preview;
						
						Sprite::ptr previewHost = createObject< Sprite >( PREVIEW_HOST_NAME );
						previewHost->setTextureByName( "editor_touch_proxy" );
						m_preview = previewHost;
						
						previewHost->addChild( actualPreview );
						
						addChild( m_preview );
						
						previewDimensions = m_preview->localBounds().dimensions();
					}
					
					ASSERT( !previewDimensions.isZero() && previewDimensions.isFinite() );

					const real fitRatio = getFitRatio( previewDimensions, vec2( m_previewSize ));
					m_preview->scale( fitRatio );
					m_preview->position( m_textField->localBounds().right() + m_previewPadding + m_previewSize * 0.5f, 0 );
					
					m_preview->isDragEnabled( true );
					m_preview->doMoveWithDrag( true );
					
					m_preview->addEventListener( EventTouch::TOUCH_BEGIN, FRESH_CALLBACK( onPreviewTouchBegin ));
					m_preview->addEventListener( DisplayObject::DRAG_BEGIN, FRESH_CALLBACK( onPreviewDragBegin ));
					m_preview->addEventListener( DisplayObject::DRAG_MOVE, FRESH_CALLBACK( onPreviewDragMove ));
					m_preview->addEventListener( DisplayObject::DRAG_END, FRESH_CALLBACK( onPreviewDragEnd ));
				}
			}
			
			m_touchCatcher->scale( immediateBounds() * 0.5f );		// TODO really should fill the whole width of the panel.
			m_touchCatcher->pivot( -1.0f, 0 );
		}
		
	protected:
		
		Editor& editor() const
		{
			return *stage().as< Editor >();
		}
		
		Manipulator& currentManipulator() const
		{
			return editor().currentManipulator();
		}
		
		bool isPreviewOverManipulator() const
		{
			m_preview->visible( false );
			auto topDescendant = stage().getTopDescendantUnderPoint( stage().globalToLocal( m_preview->parent()->localToGlobal( m_preview->position() )));
			m_preview->visible( true );
			
			const auto& manipulator = currentManipulator();
			
			return !topDescendant || topDescendant == &manipulator || topDescendant->hasAncestor( &manipulator );
		}
		
		vec2 immediateBounds() const
		{
			// Just the bounds enclosing this selector itself, not the child selectors.
			//
			return vec2( bounds().width(),
						 std::max( m_textField->bounds().height(), m_preview ? m_previewSize : 0 ));
		}
		
		bool takeDescendantSelector( SmartPtr< EdClassSelector > selector )
		{
			// Is this a descendant class?
			if( selector->getClass().isKindOf( *m_myClass ))
			{
				// Is it one of my children's descendants?
				for( size_t i = 0; i < m_selectorHost->numChildren(); ++i )
				{
					EdClassSelector::ptr childSelector = dynamic_freshptr_cast< EdClassSelector::ptr >( m_selectorHost->getChildAt( i ));
					if( childSelector->takeDescendantSelector( selector ))
					{
						return true;
					}
				}
				
				selector->parent()->removeChild( selector );
				m_selectorHost->addChild( selector );
				return true;
			}
			else
			{
				return false;
			}
		}
		
		void reshuffleChildrenToNewChild( EdClassSelector& newChild )
		{
			forEachChildSelector( [&]( EdClassSelector& selector )
			{
				newChild.takeDescendantSelector( &selector );
			} );
		}
		
	private:
		
		ClassInfo::cptr m_myClass;
		VAR( TextField::ptr, m_textField );
		VAR( DisplayObjectContainer::ptr, m_selectorHost );
		DVAR( real, m_previewSize, 32.0f );
		DVAR( real, m_previewPadding, 10.0f );
		
		DisplayObject::ptr m_preview;
		Sprite::ptr m_touchCatcher;
		
		CallbackFunctionAbstract::wptr m_selectionCallback;
		
		bool m_enabled = true;
		
		DisplayObjectState m_previewDragStartState;

		FRESH_DECLARE_CALLBACK( EdClassSelector, onPreviewTouchBegin, EventTouch );
		FRESH_DECLARE_CALLBACK( EdClassSelector, onPreviewTouchEndAnywhere, EventTouch );
		FRESH_DECLARE_CALLBACK( EdClassSelector, onPreviewDragBegin, EventTouch );
		FRESH_DECLARE_CALLBACK( EdClassSelector, onPreviewDragMove, EventTouch );
		FRESH_DECLARE_CALLBACK( EdClassSelector, onPreviewDragEnd, EventTouch );
	};
	
	FRESH_DEFINE_CLASS_UNPLACEABLE( EdClassSelector )
	DEFINE_VAR( EdClassSelector, TextField::ptr, m_textField );
	DEFINE_VAR( EdClassSelector, DisplayObjectContainer::ptr, m_selectorHost );
	DEFINE_VAR( EdClassSelector, real, m_previewSize );
	DEFINE_VAR( EdClassSelector, real, m_previewPadding );
	
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( EdClassSelector )
	
	EdClassSelector::EdClassSelector( const ClassInfo& assignedClassInfo, NameRef name )
	:	EdClassSelector( assignedClassInfo, DisplayObject::StaticGetClassInfo() )
	{}

	FRESH_DEFINE_CALLBACK( EdClassSelector, onPreviewTouchBegin, EventTouch )
	{
		editor().beginTouchAction();
		stage().addEventListener( EventTouch::TOUCH_END, FRESH_CALLBACK( onPreviewTouchEndAnywhere ));
	}
	
	FRESH_DEFINE_CALLBACK( EdClassSelector, onPreviewTouchEndAnywhere, EventTouch )
	{
		stage().removeEventListener( EventTouch::TOUCH_END, FRESH_CALLBACK( onPreviewTouchEndAnywhere ));
		editor().endTouchAction();
	}
	
	FRESH_DEFINE_CALLBACK( EdClassSelector, onPreviewDragBegin, EventTouch )
	{
		DisplayObject::ptr preview = event.currentTarget()->as< DisplayObject >();
		ASSERT( preview == m_preview );
		
		m_previewDragStartState.setStateFrom( *preview );
		
		auto previewParent = preview->parent();
// TODO Really necessary?		ASSERT( previewParent == this );
		
		preview->position( editor().globalToLocal( previewParent->localToGlobal( preview->position() )));
		
		previewParent->removeChild( preview );
		editor().addChild( preview );
	}
	
	FRESH_DEFINE_CALLBACK( EdClassSelector, onPreviewDragMove, EventTouch )
	{
		DisplayObject::ptr preview = event.currentTarget()->as< DisplayObject >();
		ASSERT( preview == m_preview );

		Color previewColor = m_previewDragStartState.color();
		vec2 previewScale = m_previewDragStartState.scale();
		
		if( isPreviewOverManipulator() )
		{
			// Situate the preview so that it looks ready to put into the world.
			//
			previewColor.setA( 0.5f );
			
			auto& manipulator = currentManipulator();
			
			const vec2 originalScale = dynamic_cast< const DisplayObject* >( preview->classInfo().defaultObject() )->scale();
			
			previewScale = manipulator.localToGlobalScale( manipulator.subjectToLocalScale( originalScale ));
		}
		
		preview->color( previewColor );
		preview->scale( previewScale );
	}
	
	FRESH_DEFINE_CALLBACK( EdClassSelector, onPreviewDragEnd, EventTouch )
	{
		DisplayObject::ptr preview = event.currentTarget()->as< DisplayObject >();
		ASSERT( preview == m_preview );
		
		// Check whether the manipulator is ready to "catch" this object.
		//
		if( isPreviewOverManipulator() )
		{
			// Yes it is.
			
			DisplayObject::ptr actualPreview = preview;
			
			// Is this "preview" actually a preview host (used for otherwise invisible previews?).
			//
			if( preview->name() == PREVIEW_HOST_NAME )
			{
				actualPreview = preview->as< Sprite >()->getChildAt( 0 );
			}
			
			// Create the new object.
			//
			auto& manipulator = currentManipulator();
			auto createdObject = manipulator.createObjectOfClass( actualPreview->classInfo(), manipulator.globalToLocal( preview->parent()->localToGlobal( preview->position() )) );
			manipulator.deselectAll();
			manipulator.select( createdObject );
			editor().saveHistoryState();
		}

		// Reset the preview object itself.
		//
		preview->parent()->removeChild( preview );
		addChild( preview );
		m_previewDragStartState.applyStateTo( *preview );
		
		editor().endTouchAction();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	
	FRESH_DEFINE_CLASS_UNPLACEABLE( EdClassInventory )
	DEFINE_VAR( EdClassInventory, ClassInfo::cptr, m_rootClass );
	DEFINE_VAR( EdClassInventory, ClassFilter::ptr, m_classFilter );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( EdClassInventory )
	
	ClassInfo::NameRef EdClassInventory::getSelectedClassName() const
	{
		ASSERT( !m_selectedClassName.empty() );
		return m_selectedClassName;
	}
	
	void EdClassInventory::setSelectedClassName( ClassInfo::NameRef className )
	{
		ASSERT( !className.empty() );
		m_selectedClassName = className;
		
		// Highlight.
		//
		auto root = getRootSelector();
		ASSERT( root );
		root->highlightSelectedClassName( className );
	}
	
	void EdClassInventory::refresh()
	{
		//
		// Build the tree of available classes and pseudoclasses.
		//
		
		forEachClass( [this] ( const ClassInfo& classInfo )
		{
			addClass( classInfo );
			
		}, &DisplayObject::StaticGetClassInfo() );
				
		ASSERT( getRootSelector() );
		getRootSelector()->sort();
		
		getRootSelector()->position( 0, ROOT_OFFSET );
		getRootSelector()->distributeChildrenVertically( 10, 2 );
		
		getRootSelector()->setSelectionListener( FRESH_CALLBACK( onSelectorTapped ));

		if( m_selectedClassName.empty() )
		{
			setSelectedClassName( m_rootClass->className() );
		}
	}
	
	SmartPtr< EdClassSelector > EdClassInventory::getRootSelector()
	{
		if( numChildren() == 0 )
		{
			EdClassSelector::ptr rootSelector = new EdClassSelector( EdClassSelector::StaticGetClassInfo(), *m_rootClass );
			addChild( rootSelector );
			
			// Disable the root class if it's not allowed.
			//
			rootSelector->enabled( isAllowed( rootSelector->getClass().className() ));
		}
		
		return dynamic_freshptr_cast< EdClassSelector::ptr >( getChildAt( 0 ));
	}
	
	void EdClassInventory::addClass( const ClassInfo& classInfo )
	{
		if( !isAllowed( classInfo.className() ) || !classInfo.isKindOf( *m_rootClass ))
		{
			return;
		}
		
		EdClassSelector::ptr rootSelector = getRootSelector();
		
		if( rootSelector->getClass().className() != classInfo.className() )	// Don't re-add the root class
		{
			VERIFY( rootSelector->addDescendantClass( classInfo ));
		}
	}
	
	void EdClassInventory::onAddedToStage()
	{
		refresh();
	}

	FRESH_DEFINE_CALLBACK( EdClassInventory, onSelectorTapped, Event )
	{
		const ClassInfo& selectorClass = EdClassSelector::StaticGetClassInfo();
		
		DisplayObject::ptr tapped = dynamic_freshptr_cast< DisplayObject::ptr >( event.target() );
		EdClassSelector::ptr selector;
		
		while( tapped && !tapped->isA( selectorClass ))
		{
			tapped = tapped->parent();
		}
		
		selector = dynamic_freshptr_cast< EdClassSelector::ptr >( tapped );
		ASSERT( selector );
		
		setSelectedClassName( selector->getClass().className() );
	}

	bool EdClassInventory::isAllowed( ClassInfo::NameRef className ) const
	{
		// No matter what, don't allow unplaceable classes.
		//
		auto classInfo = getClass( className );
		ASSERT( classInfo );
		if( !classInfo->isPlaceable() )
		{
			return false;
		}
		
		// Never ever allow EdClassInventory itself.
		//
		if( className == this->className() )
		{
			return false;
		}
	
		if( m_classFilter )
		{
			return m_classFilter->includes( *classInfo );
		}
		else
		{
			return true;
		}
	}
	
}

