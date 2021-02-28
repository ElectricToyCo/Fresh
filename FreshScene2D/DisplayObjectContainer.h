/*
 *  DisplayObjectContainer.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/18/08.
 *  Copyright 2008 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_DISPLAY_OBJECT_CONTAINER_H_INCLUDED_
#define FRESH_DISPLAY_OBJECT_CONTAINER_H_INCLUDED_

#include "Object.h"
#include "Rectangle.h"
#include "DisplayObject.h"
#include "ObjectStreamFormatter.h"
#include <vector>

namespace fr
{
	
	class DisplayObjectContainer : public DisplayObject
	{
	public:
		
		typedef std::vector< SmartPtr< DisplayObject > > Children;
		
		virtual ~DisplayObjectContainer();
		
		void addChild( DisplayObject::ptr displayObject );
			// REQUIRES( this != displayObject );
			// REQUIRES( displayObject );
			// REQUIRES( getChildIndex( displayObject ) == size_t( -1 ));
			// REQUIRES( displayObject->parent() == 0 );
			// PROMISES( getChildIndex( displayObject ) < numChildren() );
			// PROMISES( displayObject->parent() == this );
		
		virtual void addChildAt( DisplayObject::ptr displayObject, size_t index );
			// REQUIRES( index >= 0 && index <= numChildren() );
			// REQUIRES( this != displayObject );
			// REQUIRES( displayObject );
			// REQUIRES( !displayObject->parent());
			// REQUIRES( getChildIndex( displayObject ) == size_t( -1 ));
			// PROMISES( getChildIndex( displayObject ) < numChildren() );
			// PROMISES( displayObject->parent() == this );

		void removeChild( DisplayObject::ptr displayObject );
			// REQUIRES( displayObject != 0 );
			// REQUIRES( getChildIndex( displayObject )  < numChildren() );
			// PROMISES( getChildIndex( displayObject ) < 0 );
			// PROMISES( !displayObject->parent() );
		
		virtual void removeChildAt( size_t iChild );
			// REQUIRES( iChild < numChildren() );
		
		void removeMarkedChildren();
			// Removes all children for which isMarkedForDeletion() == true.
		
		DisplayObject::ptr getChildAt( size_t iChild ) const;
			// REQUIRES( iChild < numChildren() );
			// PROMISES( result );
		
		template< typename child_t >
		SmartPtr< child_t > getChildOfTypeAt( size_t iChild ) const;
			// REQUIRES nothing
			// PROMISES nothing
		
		size_t  getChildIndex( DisplayObject::cptr displayObject ) const;
			// REQUIRES( displayObject );
			// PROMISES NOTHING (-1 means it ain't here)

		template< typename child_t >
		size_t  getChildOfTypeIndex( SmartPtr< const child_t > displayObject ) const;
			// REQUIRES( displayObject );
			// PROMISES NOTHING (-1 means it ain't here)

		void swapChildren( DisplayObject::ptr childA, DisplayObject::ptr childB );
			// Reverses the order of these two children in the display order.
			// REQUIRES( getChildIndex( childA ) < numChildren() );
			// REQUIRES( getChildIndex( childB ) < numChildren() );

		void swapChildren( size_t childA, size_t childB );
			// REQUIRES( childA < numChildren() );
			// REQUIRES( childB < numChildren() );

		size_t  numChildren() const;

		template< typename child_t >
		size_t  getNumChildrenOfType() const;
		
		template< typename child_t >
		size_t  getNumDescendantsOfType() const;
		
		Children::iterator begin();
		Children::const_iterator begin() const;
		Children::iterator end();
		Children::const_iterator end() const;
		
		void removeChildren( size_t iStart = 0, size_t iEnd = -1 );
		
		template< typename PredicateT >
		void removeChildren( PredicateT&& predicate )
		{
			fr::removeElements( m_children, std::move( predicate ));
		}
		
		bool hasChild( DisplayObject::cptr displayObject ) const;
		bool hasDescendant( DisplayObject::cptr displayObject ) const;
		
		enum class NameSearchPolicy
		{
			ExactMatch,
			Substring
		};
		
		// getChildByName(): Finds the first child with the given name.
		// For substring searches, an empty substring matches anything.
		DisplayObject::ptr getChildByName( NameRef name, NameSearchPolicy nameSearchPolicy = NameSearchPolicy::Substring ) const;

		template< typename child_t >
		SmartPtr< child_t > getChildByName( NameRef name, NameSearchPolicy nameSearchPolicy = NameSearchPolicy::Substring ) const;
		
		template< typename child_t >
		SmartPtr< child_t > getDescendantByName( NameRef name, NameSearchPolicy nameSearchPolicy = NameSearchPolicy::Substring ) const;
		
		virtual rect localBounds() const override;
		
		rect getChildrenBounds() const;
		
		virtual size_t disgorge();
		// Moves all children into this container's parent, preserving Z-order.
		// Then removes itself from its parent.
		// Children are also repositioned to remove the container's transformation.
		// This function *will likely* result in self-destruction.
		// REQUIRES( parent() );
		// Returns the number of children disgorged.

		vec2 rectify();
		// Repositions the container to sit at the center of its children
		// and adjusts children to maintain the same parent-space position.
		// Returns the position adjustment offset that was used
		
		template< typename Comparator >
		void sortChildren( Comparator&& comparator )
		{
			std::sort( m_children.begin(), m_children.end(), std::forward< Comparator >( comparator ));
		}

		virtual void update() override;
		
		virtual void onTouchBegin( const EventTouch& event ) override;
		virtual void onTouchMove( const EventTouch& event ) override;
		virtual void onTouchEnd( const EventTouch& event ) override;
		virtual void onWheelMove( const EventTouch& event ) override;
		
		virtual bool hitTestPoint( const vec2& localLocation, HitTestFlags flags ) const override;
		
		Children getChildrenUnderPoint( const vec2& location, HitTestFlags flags = HTF_RequireTouchable ) const;
		Children getDescendantsUnderPoint( const vec2& location, HitTestFlags flags = HTF_RequireTouchable ) const;
		DisplayObject::ptr getTopChildUnderPoint( const vec2& location, HitTestFlags flags = HTF_RequireTouchable ) const;
		DisplayObject::ptr getTopDescendantUnderPoint( const vec2& location, HitTestFlags flags = HTF_RequireTouchable ) const;

		virtual void recordPreviousState( bool recursive = false ) override;

		template< typename child_t, typename function_t >
		void forEachChild( function_t&& fn );

		template< typename child_t, typename function_t >
		void forEachChild( function_t&& fn ) const;
				
		template< typename child_t, typename function_t >
		void forEachDescendant( function_t&& fn, bool preOrderTraversal = true );
	
		template< typename child_t, typename function_t >
		void forEachDescendant( function_t&& fn, bool preOrderTraversal = true ) const;
		
		virtual void onBeginPlay() override;
		virtual void onEndPlay() override;

		virtual void preRender( TimeType relativeFrameTime ) override;
		
		virtual void postLoad() override;
		
		Object::ptr createClone( NameRef name = DEFAULT_OBJECT_NAME ) const override;
		
		virtual bool verifyTree() const override;
				
	protected:

		virtual void draw( TimeType relativeFrameTime, RenderInjector* injector ) override;
		
		virtual void updateChildren();
		virtual void drawChildren( TimeType relativeFrameTime, RenderInjector* injector );
		
		virtual void onAddedChild( DisplayObject::ptr child ) {}
		
		typedef void (DisplayObject::*TouchFunction)( const EventTouch& event );
		
		void onTouchCommonContainer( const EventTouch& event, const std::function< void( const EventTouch& ) >& superMethod, TouchFunction childMethod );
		
		void propogateEventThroughoutHierarchy( TouchFunction fnMutator, const EventTouch& event );
		void propogateEventThroughLineageToTarget( TouchFunction fnMutator, const EventTouch& event );

		// Tree fixup.
		//
		virtual void propagateStage( WeakPtr< Stage > stage ) override;
		// REQUIRES( stage );
		virtual void propagateStageNotification() override;
		// REQUIRES( hasStage() );
		virtual void propagateParentNotification() override;
		// REQUIRES( parent() );
		virtual void propagateStageRemovalNotification() override;
		
		virtual bool hasDraggingDescendant() const;
		virtual bool couldStartDrag() const override;
		
		bool isNewChildCatcherEnabled() const;
		void enableNewChildCatcher();
		void disableNewChildCatcher();
		void clearNewChildCatcher();
		
		void debugCleanupDuplicateChildren();
	
		VAR( Children, m_children );
		
	private:
		
		bool	 m_enableNewChildCatcher = false;		// When true, all newly added children are also added to m_newChildCatcher. Removed children are also removed from there.
		std::set< DisplayObject::wptr > m_newChildCatcher;
		
		FRESH_DECLARE_CLASS( DisplayObjectContainer, DisplayObject )
	};
	
	//////////////////////////////////////////////////////////////////////////////////////////
	
	class ObjectStreamFormatterDisplayHierarchy : public ObjectStreamFormatterXml
	{
	public:
		ObjectStreamFormatterDisplayHierarchy( Stringifier& stringifier, 
											  unsigned int initialIndentLevel = 0, 
											  ClassInfo::NameRef classIdFilter = "", 
											  Object::NameRef objectIdFilter = "" )
		:	ObjectStreamFormatterXml( stringifier, initialIndentLevel )
		,	m_classFilter( classIdFilter )
		,	m_objectFilter( objectIdFilter )
		{}
		
		virtual void beginObject( const Object* object )
		{
			writeIndents();
			
			if( auto displayObject = object->as< DisplayObject >() )
			{			
				getStringifier() << "<" << object->className() << " name='" << object->name() << "' pos='" << displayObject->position() << "'>\n";
				indent( 1 );
			
				if( auto container = displayObject->as< DisplayObjectContainer >() )
				{
					// Trace children.
					//
					for( size_t iChild = 0; iChild < container->numChildren(); ++iChild )
					{
						DisplayObject::ptr child = container->getChildAt( iChild );
						
						if( child->matchesFilters( m_classFilter, m_objectFilter ))
						{
							child->serialize( *this );
						}
					}	
				}
			}
			else
			{
				// Not actually a display object.
				//
				getStringifier() << "<" << object->className() << " name='" << object->name() << "'>\n";
				indent( 1 );
			}
		}
		
	private:
		
		std::string m_classFilter;
		std::string m_objectFilter;
	};
	
	
	////////////////////////////////////////////////////////////////////////////////
	// INLINES
	
	// May return nullptr.
	template< typename ancestor_t >
	inline SmartPtr< ancestor_t > firstAncestorOfType( const DisplayObject& object )
	{
		SmartPtr< DisplayObjectContainer > ancestor = object.parent();
		while( ancestor )
		{
			auto castAncestor = ancestor->as< ancestor_t >();
			if( castAncestor )
			{
				return castAncestor;
			}
			
			ancestor = ancestor->parent();
		}
		return nullptr;
	}
	
	template< typename T = DisplayObject >
	inline T& getExpectedDescendant( const fr::DisplayObjectContainer& root, const std::string& name = "" )
	{
		auto descendant = root.getDescendantByName< T >( name );
		ASSERT( descendant );
		return *descendant;
	}

	inline size_t DisplayObjectContainer::numChildren() const
	{
		return m_children.size();
	}
	
	inline DisplayObjectContainer::Children::iterator DisplayObjectContainer::begin()
	{
		return m_children.begin();
	}
	
	inline DisplayObjectContainer::Children::const_iterator DisplayObjectContainer::begin() const
	{
		return m_children.begin();
	}
	
	inline DisplayObjectContainer::Children::iterator DisplayObjectContainer::end()
	{
		return m_children.end();
	}

	inline DisplayObjectContainer::Children::const_iterator DisplayObjectContainer::end() const
	{
		return m_children.end();
	}
	
	inline void DisplayObjectContainer::removeChildren( size_t iStart, size_t iEnd )
	{
		while( iStart < numChildren() && iStart < iEnd )
		{
			removeChildAt( iStart );
			--iEnd;
		}
	}
	
	inline bool DisplayObjectContainer::hasChild( DisplayObject::cptr displayObject ) const
	{
		return getChildIndex( displayObject ) < numChildren();
	}
	
	template< typename child_t >
	SmartPtr< child_t > DisplayObjectContainer::getChildByName( NameRef name, NameSearchPolicy nameSearchPolicy ) const
	{
		for( auto child : m_children )
		{
			if( auto castChild = child->as< child_t >() )
			{
				bool nameMatches = ( nameSearchPolicy == NameSearchPolicy::ExactMatch ) ?
									( castChild->name() == name ) :
									( name.empty() || castChild->name().find( name ) != std::string::npos );
				
				if( nameMatches )
				{
					return castChild;
				}
			}
		}
		return nullptr;
	}
	
	template< typename child_t >
	SmartPtr< child_t > DisplayObjectContainer::getDescendantByName( NameRef name, NameSearchPolicy nameSearchPolicy ) const
	{
		auto descendant = getChildByName< child_t >( name, nameSearchPolicy );
		if( descendant )
		{
			return descendant;
		}
		else
		{
			for( auto child : *this )
			{
				DisplayObjectContainer::ptr childContainer = dynamic_freshptr_cast< DisplayObjectContainer::ptr >( child );
				if( childContainer )
				{
					descendant = childContainer->getDescendantByName< child_t >( name, nameSearchPolicy );
					if( descendant )
					{
						return descendant;
					}
				}
			}
		}
		return nullptr;
	}

	template< typename child_t, typename function_t >
	void DisplayObjectContainer::forEachChild( function_t&& fn )
	{
		auto copy( m_children );
		for( auto child : copy )
		{
			if( auto convertedChild = child->as< child_t >() )
			{
				fn( *convertedChild );
			}
		}
	}
	
	template< typename child_t, typename function_t >
	void DisplayObjectContainer::forEachChild( function_t&& fn ) const
	{
		auto copy( m_children );
		for( auto child : copy )
		{
			if( auto convertedChild = child->as< const child_t >() )
			{
				fn( *convertedChild );
			}
		}
	}

	template< typename child_t, typename function_t >
	void DisplayObjectContainer::forEachDescendant( function_t&& fn, bool preOrderTraversal )
	{
		auto copy( m_children );
		for( auto child : copy )
		{
			if( preOrderTraversal )
			{
				if( auto convertedChild = child->as< child_t >() )
				{
					fn( *convertedChild );
				}
			}
				
			if( auto container = dynamic_freshptr_cast< DisplayObjectContainer::ptr >( child ))
			{
				container->forEachDescendant< child_t >( fn, preOrderTraversal );
			}

			if( !preOrderTraversal )
			{
				if( auto convertedChild = child->as< child_t >() )
				{
					fn( *convertedChild );
				}
			}
		}
	}

	template< typename child_t, typename function_t >
	void DisplayObjectContainer::forEachDescendant( function_t&& fn, bool preOrderTraversal ) const
	{
		auto copy( m_children );
		for( auto child : copy )
		{
			if( preOrderTraversal )
			{
				if( auto convertedChild = child->as< child_t >() )
				{
					fn( *convertedChild );
				}
			}
			
			if( auto container = dynamic_freshptr_cast< DisplayObjectContainer::cptr >( child ))
			{
				container->forEachDescendant< child_t >( fn, preOrderTraversal );
			}
			
			if( !preOrderTraversal )
			{
				if( auto convertedChild = child->as< child_t >() )
				{
					fn( *convertedChild );
				}
			}
		}
	}
	
	template< typename child_t >
	size_t DisplayObjectContainer::getNumChildrenOfType() const
	{
		size_t count = 0;
		forEachChild< child_t >( [&count]( const child_t& ) { ++count; } );
		return count;
	}
	
	template< typename child_t >
	size_t  DisplayObjectContainer::getNumDescendantsOfType() const
	{
		size_t count = 0;
		forEachDescendant< child_t >( [&count]( const child_t& ) { ++count; } );
		return count;
	}
	

	template< typename child_t >
	SmartPtr< child_t > DisplayObjectContainer::getChildOfTypeAt( size_t iChild ) const
	{
		if( iChild >= m_children.size() )
		{
			return nullptr;
		}
		
		for( size_t i = 0; i < numChildren(); ++i )		// Using indexing because the number of children may change during traversal.
		{
			auto child = m_children[ i ];
			if( auto convertedChild = child->as< child_t >() )
			{
				if( iChild == 0 )
				{
					return convertedChild;
				}
				--iChild;
			}
		}
		
		return nullptr;
	}
	
	template< typename child_t >
	size_t DisplayObjectContainer::getChildOfTypeIndex( SmartPtr< const child_t > displayObject ) const
	{
		REQUIRES( displayObject );
		
		size_t iChild = 0;
		for( size_t i = 0; i < numChildren(); ++i )		// Using indexing because the number of children may change during traversal.
		{
			auto child = m_children[ i ];
			if( auto convertedChild = child->as< child_t >() )
			{
				if( convertedChild == displayObject )
				{
					return iChild;
				}
				
				++iChild;
			}
		}
		return size_t( -1 );
	}
	
}

#endif
