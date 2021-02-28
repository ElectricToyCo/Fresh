/*
 *  DisplayObjectContainer.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/18/08.
 *  Copyright 2008 jeffwofford.com. All rights reserved.
 *
 */

#include "DisplayObjectContainer.h"
#include "Objects.h"
#include "Profiler.h"
#include "Stage.h"
#include "CommandProcessor.h"


#ifdef FRESH_DEBUG_HITTESTS
#	define trace_hittest( expr ) trace( expr )
#else
#	define trace_hittest( expr )
#endif

namespace fr
{

	FRESH_DEFINE_CLASS( DisplayObjectContainer )

	DEFINE_VAR_FLAG( DisplayObjectContainer, Children, m_children, PropFlag::NoEdit );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( DisplayObjectContainer )

	DisplayObjectContainer::~DisplayObjectContainer()
	{
		onEndPlay();
	}

	void DisplayObjectContainer::addChild( DisplayObject::ptr displayObject )
	{
		addChildAt( displayObject, m_children.size() );
	}

	void DisplayObjectContainer::addChildAt( DisplayObject::ptr displayObject, size_t index )
	{
		REQUIRES( index <= numChildren() );
		REQUIRES( displayObject != this );
		REQUIRES( displayObject );
		REQUIRES( !displayObject->parent() );
		REQUIRES( getChildIndex( displayObject ) == size_t( -1 ));
		
		m_children.insert( m_children.begin() + index, displayObject );

		displayObject->parent( this );
		displayObject->propagateParentNotification();

		if( hasStage() )
		{
			displayObject->propagateStage( &stage() );
			displayObject->propagateStageNotification();
		}
		
		onAddedChild( displayObject );
		
		if( m_enableNewChildCatcher )
		{
			m_newChildCatcher.insert( displayObject );
		}
		
		PROMISES( getChildIndex( displayObject ) < numChildren() );
		PROMISES( displayObject->parent() == this );
	}

	void DisplayObjectContainer::removeChild( DisplayObject::ptr displayObject )
	{
		REQUIRES( displayObject );
		
		DisplayObject::cptr constDisplayObj = displayObject;
		
		auto iterDisplayObject = std::find( m_children.begin(), m_children.end(), constDisplayObj );
		REQUIRES( iterDisplayObject != m_children.end() );		// Didn't actually own this child.
		
		removeChildAt( iterDisplayObject - m_children.begin() );
		
		PROMISES( getChildIndex( displayObject ) == size_t( -1 ));
		PROMISES( !displayObject->parent() );
	}

	void DisplayObjectContainer::removeChildAt( size_t iChild )
	{
		REQUIRES( iChild < numChildren() );
		
		const DisplayObject::ptr displayObject = m_children[ iChild ];
		
		displayObject->onRemovingFromParent();
		
		m_children.erase( m_children.begin() + iChild );
		
		if( m_enableNewChildCatcher )
		{
			m_newChildCatcher.erase( displayObject );
		}
	}

	void DisplayObjectContainer::removeMarkedChildren()
	{
		auto firstToRemove = std::remove_if( m_children.begin(), m_children.end(),
											std::mem_fn( &DisplayObject::isMarkedForDeletion ));
		
		if( firstToRemove != m_children.end() )
		{
			m_children.erase( firstToRemove, m_children.end());
		}
	}

	DisplayObject::ptr DisplayObjectContainer::getChildAt( size_t iChild ) const
	{
		REQUIRES( iChild < numChildren() );	
		return m_children.at( iChild );
	}

	size_t DisplayObjectContainer::getChildIndex( DisplayObject::cptr displayObject ) const
	{
		auto iterDisplayObject = std::find( m_children.begin(), m_children.end(), displayObject );
		
		if( iterDisplayObject == m_children.end() )
		{
			return size_t( -1 );
		}
		else
		{
			return iterDisplayObject - m_children.begin();
		}
	}

	DisplayObject::ptr DisplayObjectContainer::getChildByName( NameRef name, NameSearchPolicy nameSearchPolicy ) const
	{
		for( auto child : m_children )
		{
			bool nameMatches = ( nameSearchPolicy == NameSearchPolicy::ExactMatch ) ?
					( child->name() == name ) :
					( name.empty() || child->name().find( name ) != std::string::npos );
			
			if( nameMatches )
			{
				return child;
			}
		}
		
		return nullptr;
	}

	void DisplayObjectContainer::swapChildren( DisplayObject::ptr childA, DisplayObject::ptr childB )
	{
		size_t iChildA = getChildIndex( childA );
		ASSERT( iChildA < numChildren());
		size_t iChildB = getChildIndex( childB );
		ASSERT( iChildB < numChildren());

		swapChildren( iChildA, iChildB );
	}

	void DisplayObjectContainer::swapChildren( size_t iChildA, size_t iChildB )
	{
		REQUIRES( iChildA < numChildren() );
		REQUIRES( iChildB < numChildren() );
		
		std::swap( m_children[ iChildA ], m_children[ iChildB ] );
	}

	bool DisplayObjectContainer::hasDescendant( DisplayObject::cptr displayObject ) const
	{
		if( displayObject == nullptr )
		{
			return false;
		}
		
		for( auto child : m_children )
		{
			if( child == displayObject )
			{
				return true;
			}
			else 
			{
				DisplayObjectContainer::cptr container = dynamic_freshptr_cast< DisplayObjectContainer::cptr >( child );
				if( container && container->hasDescendant( displayObject ))
				{
					return true;
				}
			}
		}
		
		return false;
	}

	rect DisplayObjectContainer::localBounds() const
	{
		return getChildrenBounds();
	}	

	rect DisplayObjectContainer::getChildrenBounds() const
	{
		rect bounds;
		bounds.setToInverseInfinity();
		
		for( auto child : m_children )
		{
			if( child->doesWantToRender() )
			{
				rect childBounds = child->localBounds();
				childBounds = child->localToParent( childBounds );
				bounds.growToEncompass( childBounds );
			}
		}
		
		return bounds;
	}

	size_t DisplayObjectContainer::disgorge()
	{
		REQUIRES( parent() );
		
		auto myParent = parent();
		
		const auto myDepth = myParent->getChildIndex( this );
		
		const size_t nOriginalChildren = numChildren();
		
		while( numChildren() > 0 )
		{
			auto child = getChildAt( numChildren() - 1 );
			removeChildAt( numChildren() - 1 );
			myParent->addChildAt( child, myDepth );
			
			child->position( localToParent( child->position() ));
			child->rotation( localToParent( child->rotation() ));
			child->scale( localToParentScale( child->scale() ));
		}
		
		myParent->removeChild( this );
		
		return nOriginalChildren;
	}
	
	vec2 DisplayObjectContainer::rectify()
	{
		// Find the minimum bounding circle that encompasses all child objects' bounding boxes.
		//
		std::vector< vec2 > boxPoints;
		
		for( auto child : *this )
		{
			ASSERT( child );
			const rect bounds = child->localBounds();
			
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
				
				corner = child->localToParent( corner );
				
				++iCorner;
			}
		}
		
		// Reposition self to sit at center of children.
		//
		vec2 circleCenter;
		
		if( numChildren() == 1 )
		{
			const auto& theObject = *getChildAt( 0 );
			
			circleCenter = theObject.localToParent( theObject.pivot() );
//			circleRadius = findMinimumBoundingCircleRadiusWithCenter( circleCenter, boxPoints.begin(), boxPoints.end() );
		}
		else
		{
			real circleRadius;
			findMinimumBoundingCircle( boxPoints.begin(), boxPoints.end(), /*out*/ circleCenter, /*out*/ circleRadius );
		}
		
		const auto& adjustment = circleCenter - position();
		
		position( circleCenter );
		
		// Reposition children relative to this new center.
		//
		for( auto child : *this )
		{
			child->position( child->position() - adjustment );
		}
		
		return adjustment;
	}
	
	void DisplayObjectContainer::update()
	{
		TIMER_AUTO( DisplayObjectContainer::update )
		if( !doUpdate() ) return;
				
		DisplayObject::update();
		updateChildren();
		
		removeMarkedChildren();
	}

	void DisplayObjectContainer::preRender( TimeType relativeFrameTime )
	{
		if( isMarkedForDeletion() ) return;
	
		DisplayObject::preRender( relativeFrameTime );
		
		std::for_each( m_children.begin(), m_children.end(), std::bind( &DisplayObject::preRender, std::placeholders::_1, relativeFrameTime ));
	}
	
	void DisplayObjectContainer::draw( TimeType relativeFrameTime, RenderInjector* injector )
	{
		TIMER_AUTO( DisplayObjectContainer::draw )
		if( isMarkedForDeletion() ) return;
		
		if( !injector || !injector->draw( relativeFrameTime, *this ))
		{
			DisplayObject::draw( relativeFrameTime, injector );
		}
		
		drawChildren( relativeFrameTime, injector );
	}

	void DisplayObjectContainer::postLoad()
	{
		Super::postLoad();
		
		// Kill any remaining null children.
		
		auto firstNullChild = std::remove( m_children.begin(), m_children.end(), nullptr );
		if( firstNullChild != m_children.end() )
		{
			dev_warning( toString() << " found null children in postLoad()." );
			
			m_children.erase( firstNullChild, m_children.end() );
		}
		
#if DEV_MODE
		// Ensure there are no duplicate children.
		//
		debugCleanupDuplicateChildren();
#endif
		
		// Make sure my children know who their parent is.
		//
		for( auto iter = m_children.begin(); iter != m_children.end(); /* iteration within */ )
		{
			if( !(*iter)->parent() )
			{
				(*iter)->parent( this );
			}
			
			// Is this child committed somewhere else?
			// Allow it to stay with the existing parent.
			//
			if( (*iter)->parent() != this )
			{
				iter = m_children.erase( iter );
			}
			else
			{
				++iter;
			}
		}
	}

	Object::ptr DisplayObjectContainer::createClone( NameRef name ) const
	{
		auto copy = dynamic_freshptr_cast< DisplayObjectContainer::ptr >( Super::createClone( name ));
		ASSERT( copy );
		
		// Clone all children.
		//
		decltype( m_children ) cloneChildren;
		
		for( size_t i = 0; i < copy->m_children.size(); ++i )
		{
			auto childCopy = copy->m_children[ i ]->createClone()->as< DisplayObject >();
			cloneChildren.push_back( childCopy );
		}
		
		copy->m_children.clear();
		
		std::for_each( cloneChildren.begin(), cloneChildren.end(), [&]( DisplayObject::ptr child )
					  {
						  copy->addChild( child );
					  } );
		
		return copy;
	}	

	bool DisplayObjectContainer::verifyTree() const
	{
#ifdef DEBUG
		if( isMarkedForDeletion() ) return true;
		
		bool correct = Super::verifyTree();
		
		for( auto child : m_children )
		{
			if( child->isMarkedForDeletion() ) continue;
			
			ASSERT( child->parent() == this );
			ASSERT(( !hasStage() && !child->hasStage() ) || ( &stage() == &child->stage() ));
			
			correct = child->verifyTree() && correct;
			
			ASSERT( correct );
		}
		
		return correct;
#else
		return true;
#endif
	}
	
	DisplayObjectContainer::Children DisplayObjectContainer::getChildrenUnderPoint( const vec2& location, HitTestFlags flags ) const
	{
		Children result;

		for( auto child : m_children )
		{
			if( child->isMarkedForDeletion() ) continue;

			vec2 localLocation = child->parentToLocal( location );
			
			if( child->hitTestPoint( localLocation, flags ))
			{
				result.push_back( child );
			}		
		}
		
		return result;
	}

	DisplayObjectContainer::Children DisplayObjectContainer::getDescendantsUnderPoint( const vec2& location, HitTestFlags flags ) const
	{
		Children result;
		
		for( auto child : m_children )
		{
			if( child->isMarkedForDeletion() ) continue;

			vec2 localLocation = child->parentToLocal( location );
			
			if( child->hitTestPoint( localLocation, flags ))
			{
				result.push_back( child );
				
				DisplayObjectContainer::cptr container = dynamic_freshptr_cast< DisplayObjectContainer::cptr >( child );
				
				if( container )
				{
					Children innerResult = container->getDescendantsUnderPoint( localLocation, flags );
					result.insert( result.end(), innerResult.begin(), innerResult.end() );
				}
			}
		}
		return result;
	}

	DisplayObject::ptr DisplayObjectContainer::getTopChildUnderPoint( const vec2& location, HitTestFlags flags ) const
	{
		// Go through children in reverse.
		//
		for( auto iterDisplayObject = m_children.rbegin();
			iterDisplayObject != m_children.rend();
			++iterDisplayObject )
		{
			if( (*iterDisplayObject)->isMarkedForDeletion() ) continue;

			vec2 localLocation = (*iterDisplayObject)->parentToLocal( location );
			
			if( (*iterDisplayObject)->hitTestPoint( localLocation, flags ))
			{
				return (*iterDisplayObject);
			}
		}
		
		return nullptr;
	}

	inline std::string tabs( int depth )
	{
		std::string tabs_;
		while( depth-- > 0 ) tabs_ += "\t";
		return tabs_;
	}
	
	DisplayObject::ptr DisplayObjectContainer::getTopDescendantUnderPoint( const vec2& location, HitTestFlags flags ) const
	{
#ifdef FRESH_DEBUG_HITTESTS
		static int iDepth = 0;
		trace_hittest( tabs( iDepth ) << toString() << ".DisplayObjectContainer::" << FRESH_CURRENT_FUNCTION << "(" << location << "," << flags << ")@depth=" << iDepth++ << ":" );
#endif

		DisplayObject::ptr result = nullptr;
		
		// Reverse iterate to find topmost descendant first.
		//
		for( auto iterDisplayObject = m_children.rbegin();
			iterDisplayObject != m_children.rend();
			++iterDisplayObject )
		{	
			auto child = *iterDisplayObject;
			
			if( child->isMarkedForDeletion() ) continue;
			
			const vec2 localLocation = child->parentToLocal( location );
			
			trace_hittest( tabs( iDepth ) << this << " considering child " << child->toString() << " with local loc=" << localLocation );
			
			if( child->hitTestPoint( localLocation, flags ))
			{
				trace_hittest( tabs( iDepth ) << "hit" );
				
				if( auto container = child->as< DisplayObjectContainer >() )
				{
					trace_hittest( tabs( iDepth ) << container->toString() << " is container. Recursing." );
					
					DisplayObject::ptr topDescendantOfChild = container->getTopDescendantUnderPoint( localLocation, flags );
					
					if( topDescendantOfChild )
					{
						trace_hittest( tabs( iDepth ) << container->toString() << " returned top descendant " << topDescendantOfChild->toString() );
						
						result = topDescendantOfChild;
						break;
					}
				}

				result = child;
				break;
			}
		}
		
		trace_hittest( tabs( --iDepth ) << toString() << ".DisplayObjectContainer::" << FRESH_CURRENT_FUNCTION << "(" << location << ")@depth=" << iDepth << " returning with result " << ( result ? result->toString() : "null" ) );
		
		return result;
	}
	
	void DisplayObjectContainer::recordPreviousState( bool recursive )
	{
		Super::recordPreviousState( recursive );
		
		if( recursive )
		{
			std::for_each( m_children.begin(), m_children.end(),
						  std::bind( &DisplayObject::recordPreviousState, std::placeholders::_1, recursive ));
		}
	}

	void DisplayObjectContainer::updateChildren()
	{
		auto vecDisplayObjectsCopy = m_children;

		std::for_each( vecDisplayObjectsCopy.begin(), 
					   vecDisplayObjectsCopy.end(),
					  []( const DisplayObject::ptr& child )
					  {
						  if( child->doUpdate() )
						  {
							  child->update();
						  }
					  } );
	}

	bool DisplayObjectContainer::hasDraggingDescendant() const
	{
		for( auto child : m_children )
		{
			if( child->isDragging() )
			{
				return true;
			}
			else
			{
				const auto container = child->as< const DisplayObjectContainer >();
				
				if( container && container->hasDraggingDescendant() )
				{
					return true;
				}
			}
		}
		return false;
	}
	
	bool DisplayObjectContainer::couldStartDrag() const
	{
		return Super::couldStartDrag() && !hasDraggingDescendant();
	}
	
	bool DisplayObjectContainer::isNewChildCatcherEnabled() const
	{
		return m_enableNewChildCatcher;
	}
	
	void DisplayObjectContainer::enableNewChildCatcher()
	{
		REQUIRES( !isNewChildCatcherEnabled() );
		m_enableNewChildCatcher = true;
	}
	
	void DisplayObjectContainer::disableNewChildCatcher()
	{
		REQUIRES( isNewChildCatcherEnabled() );
		m_enableNewChildCatcher = false;
	}
	
	void DisplayObjectContainer::clearNewChildCatcher()
	{
		m_newChildCatcher.clear();
	}

	void DisplayObjectContainer::onTouchCommonContainer( const EventTouch& event, const std::function< void( const EventTouch& ) >& superMethod, TouchFunction childMethod )
	{		
		ASSERT( hasStage() );

		if( !isTouchable() )
		{
			return;
		}
		
		if( event.target() == this )
		{
			EventTouch modifiedEvent( event, Event::AtTarget );
			superMethod( modifiedEvent );
		}
		else
		{
			// Report capturing phase.
			//
			{
				EventTouch modifiedEvent( event, Event::Capturing );
				superMethod( modifiedEvent );
				
				if( hasStage() )
				{
					propogateEventThroughLineageToTarget( childMethod, modifiedEvent );
				}
			}
			
			if( hasStage() )
			{
				// Report bubbling phase.
				//
				EventTouch modifiedEvent( event, Event::Bubbling );
				superMethod( modifiedEvent );
			}
		}
	}

	void DisplayObjectContainer::onTouchBegin( const EventTouch& event )
	{
		TIMER_AUTO( DisplayObjectContainer::onTouchBegin )
		onTouchCommonContainer( event, [this]( const EventTouch& aEvent ){ Super::onTouchBegin( aEvent ); }, &DisplayObject::onTouchBegin );
	}
	
	void DisplayObjectContainer::onTouchMove( const EventTouch& event )
	{
		TIMER_AUTO( DisplayObjectContainer::onTouchMove )
		onTouchCommonContainer( event, [this]( const EventTouch& aEvent ){ Super::onTouchMove( aEvent ); }, &DisplayObject::onTouchMove );
	}

	void DisplayObjectContainer::onTouchEnd( const EventTouch& event )
	{
		TIMER_AUTO( DisplayObjectContainer::onTouchEnd )
		onTouchCommonContainer( event, [this]( const EventTouch& aEvent ){ Super::onTouchEnd( aEvent ); }, &DisplayObject::onTouchEnd );
	}

	void DisplayObjectContainer::onWheelMove( const EventTouch& event )
	{
		TIMER_AUTO( DisplayObjectContainer::onWheelMove )
		onTouchCommonContainer( event, [this]( const EventTouch& aEvent ){ Super::onWheelMove( aEvent ); }, &DisplayObject::onWheelMove );
	}
	
	void DisplayObjectContainer::drawChildren( TimeType relativeFrameTime, RenderInjector* injector )
	{
		std::for_each( m_children.begin(), m_children.end(), [&]( const DisplayObject::ptr& child )
					  {
						  if( !child->isMarkedForDeletion() )
						  {
							  child->render( relativeFrameTime, injector );
						  }
					  } );
	}
	
	bool DisplayObjectContainer::hitTestPoint( const vec2& localLocation, HitTestFlags flags ) const
	{
		trace_hittest( toString() << ".DisplayObjectContainer::" << FRESH_CURRENT_FUNCTION << "(" << localLocation << "," << flags << "):" );
		
		if( DisplayObject::hitTestPoint( localLocation, flags ))
		{
			trace_hittest( "DisplayObjectContainer touched via DisplayObject" );
			return true;
		}

		if( rejectsOnTheBasisOfHitFlags( flags ))
		{
			trace_hittest( "DisplayObjectContainer rejecting because of flags." );
			return false;
		}
		
		if( !hitTestMask( localLocation, flags ))
		{
			return false;
		}
		
		// Check my children.
		//
		for( auto child : m_children )
		{
			if( child->isMarkedForDeletion() )
			{
				// This child is on its way out. Ignore it.
				continue;
			}
			
			const vec2 childLocation = child->parentToLocal( localLocation );
			
			trace_hittest( toString() << " considering child " << child->toString() << " with child location " << childLocation );
				  
			if( child->hitTestPoint( childLocation, flags ))
			{
				trace_hittest( toString() << " DisplayObjectContainer touched child " << child->toString() );
				
				return true;
			}
		}
		
		trace_hittest( toString() << " DisplayObjectContainer no touch." );
		return false;
	}

	void DisplayObjectContainer::propogateEventThroughoutHierarchy( TouchFunction fnMutator, const EventTouch& event )
	{
		if( !isTouchable() )
		{
			return;
		}
		
		auto vecDisplayObjectsCopy = m_children;
		
		for( auto child : vecDisplayObjectsCopy )
		{	
			vec2 localLocation = child->parentToLocal( event.location() );
			vec2 lastLocalLocation = child->parentToLocal( event.previousLocation() );
			
			if( child->hitTestPoint( localLocation, HTF_RequireTouchable ))
			{
				EventTouch transformedEvent( event, event.target(), localLocation, lastLocalLocation );
				
				(child.get()->*fnMutator)( transformedEvent );
			}
		}
	}


	void DisplayObjectContainer::propogateEventThroughLineageToTarget( TouchFunction fnMutator, const EventTouch& event )
	{
		ASSERT( hasStage() );
		
		if( !isTouchable() )
		{
			return;
		}
		
		DisplayObject::cptr target = dynamic_freshptr_cast< DisplayObject::cptr >( event.target() );
		if( target )
		{
			auto vecDisplayObjectsCopy = m_children;
			for( auto child : vecDisplayObjectsCopy )
			{
				// It not infrequently happens that event handlers (as called via fnMutator() below)
				// modify the display tree dramatically. When this happens, either my children, myself,
				// or my ancestors may get cut off from the stage or from each other.
				// That is a legal condition, but it requires that I avoid further event processing in the
				// modified part of the tree.
				
				if( !hasStage() ) return;
				if( !child->hasStage() ) continue;
				if( child->isMarkedForDeletion() ) continue;

				DisplayObjectContainer::ptr container = dynamic_freshptr_cast< DisplayObjectContainer::ptr >( child );
				
				if( child == target || ( container && container->hasDescendant( target )))
				{
					vec2 localLocation = child->parentToLocal( event.location() );
					vec2 lastLocalLocation = child->parentToLocal( event.previousLocation() );
					
					EventTouch transformedEvent( event, event.target(), localLocation, lastLocalLocation );			
					(child.get()->*fnMutator)( transformedEvent );
					
					break;
				}		   
			}
		}
	}
	
	void DisplayObjectContainer::onBeginPlay()
	{
		Super::onBeginPlay();
		
		decltype( m_children ) childrenNeedingCall( m_children );

		enableNewChildCatcher();
		
		while( !childrenNeedingCall.empty() )
		{
			std::for_each( childrenNeedingCall.begin(), childrenNeedingCall.end(), [&]( DisplayObject::ptr child )
						  {
							  if( !child->hasReceivedBeginPlayNotification() )
							  {
								  child->onBeginPlay();
							  }
						  } );
			
			childrenNeedingCall.assign( m_newChildCatcher.begin(), m_newChildCatcher.end() );
			clearNewChildCatcher();
		}
		
		disableNewChildCatcher();
	}
	
	void DisplayObjectContainer::onEndPlay()
	{
		Super::onEndPlay();
		
		// TODO make like onBeginPlay().
		std::for_each( m_children.begin(), m_children.end(), std::mem_fn( &DisplayObject::onEndPlay ));
	}
	
	void DisplayObjectContainer::debugCleanupDuplicateChildren()
	{
		std::set< DisplayObject::ptr > uniqueChildren;
		
		for( auto iterChild = m_children.begin(); iterChild != m_children.end(); ++iterChild )
		{
			if( uniqueChildren.find( *iterChild ) != uniqueChildren.end() )	// Have we already seen this child?
			{
				con_error( this << " found duplicate child " << *iterChild << " in postLoad(). Removing." );
				iterChild = m_children.erase( iterChild );
				--iterChild;	// Anticipating for loop's ++iterChild.
			}
			else
			{
				uniqueChildren.insert( *iterChild );
			}
		}
	}

	void DisplayObjectContainer::propagateStage( Stage::wptr stage )
	{
		std::for_each( m_children.begin(), m_children.end(), std::bind( &DisplayObject::propagateStage, std::placeholders::_1, stage ));
		
		Super::propagateStage( stage );
	}
	
	void DisplayObjectContainer::propagateStageNotification()
	{
		REQUIRES( !isMarkedForDeletion() );
		REQUIRES( hasStage() );
		
		Super::propagateStageNotification();

		auto childrenCopy = m_children;
		
		for( auto iterDisplayObject = childrenCopy.rbegin();
			iterDisplayObject != childrenCopy.rend();
			++iterDisplayObject )
		{
			if( (*iterDisplayObject)->isMarkedForDeletion() == false )
			{
				(*iterDisplayObject)->propagateStageNotification();
			}
		}
	}
	
	void DisplayObjectContainer::propagateParentNotification()
	{
		REQUIRES( !isMarkedForDeletion() );
		REQUIRES( parent() || ( hasStage() && &stage() == this ));

		auto childrenCopy = m_children;
		
		for( auto iterDisplayObject = childrenCopy.rbegin();
			iterDisplayObject != childrenCopy.rend();
			++iterDisplayObject )
		{
			if( (*iterDisplayObject)->isMarkedForDeletion() == false )
			{
				(*iterDisplayObject)->propagateParentNotification();
				onAddedChild( *iterDisplayObject );
			}
		}
		
		Super::propagateParentNotification();
	}
	
	void DisplayObjectContainer::propagateStageRemovalNotification()
	{
		if( hasStage() )
		{
			auto childrenCopy = m_children;
			
			for( auto iterDisplayObject = childrenCopy.rbegin();
				iterDisplayObject != childrenCopy.rend();
				++iterDisplayObject )
			{
				(*iterDisplayObject)->propagateStageRemovalNotification();
			}
			
			Super::propagateStageRemovalNotification();
		}
	}
	
}

