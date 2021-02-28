//
//  EdObjectBrowser.cpp
//  Fresh
//
//  Created by Jeff Wofford on 6/8/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "EdObjectBrowser.h"
#include "Sprite.h"
#include "TextField.h"
#include "UIEditBox.h"
#include "Stage.h"
#include "Editor.h"

namespace fr
{
	
	FRESH_DEFINE_CLASS_UNPLACEABLE( EdObjectPreview )
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( EdObjectPreview )

	/////////////////////////////////////////////////////////////////////////////

	FRESH_DEFINE_CLASS_UNPLACEABLE( EdObjectBrowser )
	DEFINE_VAR( EdObjectBrowser, TextField::ptr, m_caption );
	DEFINE_VAR( EdObjectBrowser, UIDisplayGrid::ptr, m_displayGrid );
	DEFINE_VAR( EdObjectBrowser, ClassInfo::cptr, m_classPreview );
	DEFINE_VAR( EdObjectBrowser, ClassInfo::cptr, m_classStandIn );
	DEFINE_VAR( EdObjectBrowser, ObjectId, m_currentFilter );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( EdObjectBrowser )

	const char* EdObjectBrowser::BROWSER_OBJECT_TAPPED = "BrowserObjectTapped";

	void EdObjectBrowser::refresh()
	{
		ASSERT( m_displayGrid );
		
		// What objects are we browsing?
		//
		auto objects = getFilteredObjects( m_currentFilter );
		
		// Sort referenced objects alphabetically.
		//
		std::sort( objects.begin(), objects.end(), []( const Object::ptr& a, const Object::ptr& b )
				  {
					  const int classNameCompare = a->className().compare( b->className() );
					  if( classNameCompare == 0 )
					  {
						  return a->name() < b->name();
					  }
					  else
					  {
						  return classNameCompare < 0;
					  }
				  } );
		
		//
		// Add corresponding previews to the display grid, reusing preview objects where possible.
		//
		
		// Stow away the display grid's children (the prior previews).
		//
		
		std::map< Object::wptr, DisplayObjectContainer::ptr > priorPreviews;
		size_t i = 0;
		while( m_displayGrid->numChildren() )
		{
			DisplayObjectContainer::ptr priorPreview = m_displayGrid->getChildAt( 0 )->as< DisplayObjectContainer >();
			ASSERT( priorPreview );
			priorPreviews[ m_referencedObjects[ i++ ]] = priorPreview;
			m_displayGrid->removeChildAt( 0 );
		}
		
		m_referencedObjects.clear();
		
		// Now add a preview for each object, reusing preview objects where possible.
		//
		for( auto object : objects )
		{
			DisplayObjectContainer::ptr preview;
			
			auto existingPreviewPair = priorPreviews.find( object );
			if( existingPreviewPair == priorPreviews.end() )
			{
				preview = createPreview( *object );
			}
			else
			{
				preview = existingPreviewPair->second;
			}
			
			m_displayGrid->addChild( preview );
			m_referencedObjects.push_back( object );
		}
		
		m_displayGrid->arrangeChildren();

		ASSERT( m_displayGrid->numChildren() == m_referencedObjects.size() );
	}
	
	void EdObjectBrowser::populate( const ObjectId& filters )
	{
		ASSERT( m_displayGrid );
	
		if( m_currentFilter != filters )
		{
			m_currentFilter = filters;
			
			if( m_caption )
			{
				m_caption->text( m_currentFilter.toString() );
			}
			
			refresh();
		}
	}
	
	void EdObjectBrowser::clear()
	{
		ASSERT( m_displayGrid );
		
		m_displayGrid->removeChildren();

		m_referencedObjects.clear();
		
		ASSERT( m_displayGrid->numChildren() == m_referencedObjects.size() );
	}
	
	void EdObjectBrowser::selectedObject( Object::ptr object )
	{
		// Deselect any already-selected object
		//
		// TODO
		
		// Mark the associated preview as selected.
		//
		// TODO
		
		// Center the display grid view on the selected preview.
		//
		// TODO
	}
	
	EdObjectPreview::ptr EdObjectBrowser::createPreview( Object& forObject ) const
	{
		EdObjectPreview::ptr preview = createObject< EdObjectPreview >( *m_classPreview, "preview_for_" + forObject.name() );
		if( !preview )
		{
			dev_warning( this << " could not create a EdObjectPreview of class " << m_classPreview );
			return nullptr;
		}
		
		DisplayObjectContainer::ptr innards = preview->getChildByName< DisplayObjectContainer >( "contents", DisplayObjectContainer::NameSearchPolicy::ExactMatch );
		if( !innards )
		{
			dev_warning( *preview << " had no 'contents' child." );
			return nullptr;
		}
		
		preview->forObject( &forObject );
		preview->addEventListener( SimpleButton::TAPPED, FRESH_CALLBACK( onPreviewTapped ));
								  
		DisplayObject::ptr previewProper;
		
		// Is the object displayable?
		//
		bool needStandIn = true;
		
#if 0	// TODO Display object previews disabled as being too bug-prone for now.
		if( DisplayObject::cptr referencedDisplayObject = forObject.as< DisplayObject >() )
		{
			const auto& dimensions = referencedDisplayObject->localBounds().dimensions();
			
			if( dimensions.x > 0 && dimensions.y > 0 && !isInfinite( dimensions.x ) && !isInfinite( dimensions.y ))
			{
				needStandIn = false;
				
				// Copy and show it.
				//
				previewProper = dynamic_freshptr_cast< DisplayObject::ptr >( forObject.createClone() );
				ASSERT( previewProper );
				
				previewProper->scale( 1 );
				
				Color color( previewProper->color() );
				color.setA( 1.0f );
				
				if( !previewProper->visible() )
				{
					color.setA( 0.5f );
				}
				
				previewProper->color( color );
				previewProper->visible( true );
				previewProper->doUpdate( false );
			}
		}
#endif
		
		if( needStandIn )
		{
			previewProper = createStandIn( forObject );
		}

		innards->addChild( previewProper );
		
		// Create a text field showing the object's name.
		//
		
		TextField::ptr objectName = preview->getDescendantByName< TextField >( "object_name_text", DisplayObjectContainer::NameSearchPolicy::ExactMatch );
		if( objectName )
		{
			objectName->text( forObject.toString() );
			
			auto previewScale = objectName->localBounds().width() / previewProper->localBounds().width();
			previewProper->scale( previewScale );
			
			objectName->position( previewProper->position() + vec2( 0, previewProper->bounds().height() * 0.5 + 2.0f ));	// TODO 2.0f is magic.			
		}
		else
		{
			dev_warning( this << ": Preview " << preview << " had no descendant TextField named 'object_name_text'." );
		}
		
		return preview;
	}
	
	DisplayObject::ptr EdObjectBrowser::createStandIn( Object& forObject ) const
	{
		Sprite::ptr standIn;
		if( auto texture = forObject.as< Texture >() )
		{
			standIn = createObject< Sprite >();
			standIn->texture( texture );
		}
		else
		{
			// Create a bogus stand-in.
			//
			standIn = createObject< Sprite >( *m_classStandIn );
			if( !standIn )
			{
				dev_warning( this << " could not create a stand-in Sprite of class " << m_classStandIn );
			}
		}
		
		return standIn;
	}
	
	FRESH_DEFINE_CALLBACK( EdObjectBrowser, onPreviewTapped, EventTouch )
	{
		auto& editor = *stage().as< Editor >();
		
		if( auto editBox = editor.beginObjectBrowserEditBoxModification() )
		{
			auto preview = event.currentTarget()->as< EdObjectPreview >();
			ASSERT( preview );
			
			const auto objectString = preview->forObject()->toString();
			
			if( !editBox->editing() )
			{
				editBox->beginEditing();
			}
			editBox->text( objectString );
			
			editor.endObjectBrowserEditBoxModification();
		}
		
		Event myEvent( BROWSER_OBJECT_TAPPED, event.currentTarget(), event.currentTarget() );
		dispatchEvent( &myEvent );
	}
}

