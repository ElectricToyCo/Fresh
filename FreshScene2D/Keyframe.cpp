/*
 *  Keyframe.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/24/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#include "Keyframe.h"
#include "MovieClip.h"
#include "Texture.h"
#include "Renderer.h"
#include "ObjectStreamFormatter.h"

namespace fr
{

	Keyframe::Keyframe( MovieClip::wptr forMovieClip )
	:	m_clipTexture( nullptr )
	,	m_textureSlice( 0, 0, 1, 1 )
	,	m_isTextureSet( false )
	{
		if( forMovieClip )
		{
			texture( forMovieClip->texture() );
			
			for( size_t i = 0; i < forMovieClip->numChildren(); ++i )
			{
				DisplayObject::ptr movieClipChild = forMovieClip->getChildAt( i );
				ASSERT( movieClipChild );
				
				setChildState( *movieClipChild );
			}		
		}
	}

	Keyframe::Keyframe( Object::NameRef singleObjectName, const DisplayObjectState& state )
	:	m_clipTexture( nullptr )
	,	m_textureSlice( 0, 0, 1, 1 )
	,	m_isTextureSet( false )
	{
		setChildState( singleObjectName, state );
	}

	void Keyframe::texture( Texture::ptr texture, const rect& textureSlice )
	{
		m_clipTexture = texture;
		m_textureSlice = textureSlice;
		m_isTextureSet = true;
	}

	void Keyframe::setTextureByName( const std::string& strFileName, const rect& textureSlice )
	{
		texture( Renderer::instance().createTexture( strFileName ), textureSlice );
	}

	Texture::ptr Keyframe::texture() const
	{
		return m_clipTexture;
	}

	void Keyframe::ignoreClipTexture( bool ignore )
	{
		m_isTextureSet = !ignore;
	}

	bool Keyframe::ignoreClipTexture() const
	{
		return !m_isTextureSet;
	}

	void Keyframe::selfPosition( const vec2& pos )
	{
		m_selfPosition = pos;
		m_isSelfPositionSet = true;
	}
	
	void Keyframe::selfPivot( const vec2& pivot )
	{
		m_selfPivot = pivot;
		m_isSelfPivotSet = true;
	}
	
	void Keyframe::setChildState( Object::NameRef childName, const DisplayObjectState& state )
	{
		m_mapChildStates[ childName ] = state;
	}

	void Keyframe::setChildState( const DisplayObject& child )
	{
		m_mapChildStates[ child.name() ] = DisplayObjectState::create( child );
	}
	
	void Keyframe::clearChildState( Object::NameRef childName )
	{
		MapChildStatesI iter = m_mapChildStates.find( childName );
		ASSERT( iter != m_mapChildStates.end() );
		
		m_mapChildStates.erase( iter );
	}

	bool Keyframe::hasChildState( Object::NameRef childName ) const
	{
		return m_mapChildStates.find( childName ) != m_mapChildStates.end();
	}

	const DisplayObjectState& Keyframe::getChildState( Object::NameRef childName ) const
	{
		MapChildStatesCI iter = m_mapChildStates.find( childName );
		ASSERT( iter != m_mapChildStates.end() );
		
		return iter->second;
	}

	Keyframe Keyframe::getTweenedStateTo( const Keyframe& destination, real normalizedTime, const Tweener< DisplayObjectState >& tweener ) const
	{
		REQUIRES( 0 <= normalizedTime && normalizedTime <= 1.0 );
		
		Keyframe tweenedKeyframe;

		if( m_isTextureSet )
		{
			tweenedKeyframe.texture( m_clipTexture, m_textureSlice );		// Can't tween textures. Just use mine.
		}
		
		if( m_isSelfPositionSet )
		{
			if( destination.isSelfPositionSet() )
			{
				tweenedKeyframe.selfPosition( lerp( m_selfPosition, destination.m_selfPosition, normalizedTime ));
			}
			else
			{
				tweenedKeyframe.selfPosition( m_selfPosition );
			}
		}
		
		if( m_isSelfPivotSet )
		{
			if( destination.isSelfPositionSet() )
			{
				tweenedKeyframe.selfPivot( lerp( m_selfPivot, destination.m_selfPivot, normalizedTime ));
			}
			else
			{
				tweenedKeyframe.selfPivot( m_selfPivot );
			}
		}

		for( MapChildStatesCI iter = m_mapChildStates.begin(); iter != m_mapChildStates.end(); ++iter )
		{
			// Does the destination have this child?
			//
			if( destination.hasChildState( iter->first ))
			{
				// Yes, add the tweened state between us.
				const DisplayObjectState& destinationState = destination.getChildState( iter->first );
				
				tweenedKeyframe.setChildState( iter->first, iter->second.getTweenedStateTo( destinationState, normalizedTime, tweener, true ));	// Using rotation tweening (as opposed to angle tweening).
			}
			else
			{
				// No. Just add ours without tweening.
				//
				tweenedKeyframe.setChildState( iter->first, iter->second );
			}
		}
		
		return tweenedKeyframe;
	}

	void Keyframe::fixupSymbolicNames( WeakPtr< const MovieClip > object )
	{
		// Show any hidden children in the movie clip and set their states.
		//
		for( MapChildStatesI iter = m_mapChildStates.begin(); iter != m_mapChildStates.end();  )
		{
			// Parse symbolic child names.
			//
			if( isSymbolicName( iter->first ))
			{
				DisplayObject::cptr movieClipChild = getChildFromSymbol( object, iter->first );
				
				if( movieClipChild )
				{
					m_mapChildStates[ movieClipChild->name() ] = iter->second;
					
					iter = m_mapChildStates.erase( iter );
					continue;
				}
			}
			
			++iter;
		}
	}
	
	void Keyframe::applyStateTo( MovieClip::wptr object ) const
	{
		REQUIRES( object );
		
		if( m_isTextureSet )
		{
			object->texture( m_clipTexture );
			object->textureWindow( m_textureSlice );
		}
		
		if( m_isSelfPositionSet )
		{
			object->position( m_selfPosition );
		}
		
		if( m_isSelfPivotSet )
		{
			object->pivot( m_selfPivot );
		}
		
		if( m_isSelfPositionSet || m_isSelfPivotSet )
		{
			// Don't tween when self position or self pivot are involved.
			//
			object->recordPreviousState( false /* non-recursive */ );
		}
		
		// Show any hidden children in the movie clip and set their states.
		//
		for( MapChildStatesCI iter = m_mapChildStates.begin(); iter != m_mapChildStates.end(); ++iter )
		{
			const auto childName = iter->first;
			
			DisplayObject::ptr movieClipChild;
			if( isSymbolicName( childName ))
			{
				movieClipChild = getChildFromSymbol( object, iter->first );
			}
			else
			{
				movieClipChild = object->getChildByName( childName, DisplayObjectContainer::NameSearchPolicy::ExactMatch );
				
				if( !movieClipChild && object->animatesWholeTree() )
				{
					movieClipChild = object->getDescendantByName< DisplayObject >( childName, DisplayObjectContainer::NameSearchPolicy::ExactMatch );
				}
			}
			
			if( movieClipChild )
			{
				const bool justBeingShown = !movieClipChild->isKeyframeVisible();
				
				movieClipChild->setKeyframeVisible( true );
				iter->second.applyStateTo( *movieClipChild );
				
				// Disable state smoothing if we're just now showing the movie.
				//
				if( justBeingShown )
				{
					movieClipChild->recordPreviousState( false );
				}
				
				// Restart this child if it's a MovieClip and this behavior is requested.
				if( justBeingShown && m_replayShownClipChildren )
				{
					if( auto childClip = movieClipChild->as< MovieClip >() )
					{
						childClip->gotoAndPlay( 0 );
					}
				}
			}
			else
			{
				dev_warning( "Keyframe could not find child '" << iter->first << "' of MovieClip " << object->toString() );
			}
		}
			   
		// Hide children in the movie clip that are missing from this keyframe.
		//
		for( size_t i = 0; i < object->numChildren(); ++i )
		{
			DisplayObject::ptr movieClipChild = object->getChildAt( i );
			
			if( !hasChildState( movieClipChild->name() ))
			{
				movieClipChild->setKeyframeVisible( false );
			}
		}
		
		// If we have an action method, call this method on the owner.
		//
		if( !m_action.empty() )
		{
			std::istringstream methodExpression( m_action );
			std::string methodName;
			methodExpression >> methodName;
			trim( methodName );
			if( !methodName.empty() )
			{
				object->call( methodName, methodExpression );
			}
		}
	}
	
	bool Keyframe::isSymbolicName( Object::NameRef childName ) const
	{
		return childName[ 0 ] == '$' && ::isdigit( childName[ 1 ] );
	}
	
	void Keyframe::serialize( const MovieClip& host, class ObjectStreamFormatter& formatter ) const
	{
		if( m_isTextureSet )
		{
			formatter.beginProperty( "texture" );
			formatter.getPropertyStringifier() << m_clipTexture;
			formatter.endProperty( "texture" );

			formatter.beginProperty( "textureWindow" );
			formatter.getPropertyStringifier() << m_textureSlice;
			formatter.endProperty( "textureWindow" );
		}
		
		if( m_isSelfPositionSet )
		{
			formatter.beginProperty( "position" );
			formatter.getPropertyStringifier() << m_selfPosition;
			formatter.endProperty( "position" );
		}

		if( m_isSelfPivotSet )
		{
			formatter.beginProperty( "pivot" );
			formatter.getPropertyStringifier() << m_selfPivot;
			formatter.endProperty( "pivot" );
		}
		
		auto& out = formatter.getStringifier();
		
		for( const auto& childState : m_mapChildStates )
		{
			formatter.writeIndents();
			out << "<child name='";
			
			// Deduce and write symbolic name.
			//
			auto child = host.getChildByName( childState.first, DisplayObjectContainer::NameSearchPolicy::ExactMatch );
			if( child )
			{
				auto childIndex = host.getChildIndex( child );
				
				out << "$" << ( childIndex + 1 );
			}
			else
			{
				// Couldn't find the actual child by this name. Probably an error, but we'll do our best.
				// Just use the strict name as indicated.
				out << childState.first;
			}
			
			out << "'>\n";
			formatter.indent( 1 );

			out << childState.second;
			
			formatter.indent( -1 );
			formatter.writeIndents();
			out << "</child>\n";
		}
	}
	
	DisplayObject::wptr Keyframe::getChildFromSymbol( MovieClip::cwptr object, const std::string& symbol ) const
	{
		REQUIRES( object != 0 );
		
		// Symbolic.
		size_t iChild;
		std::istringstream name( symbol );
		name.get();	// Skip the $.
		name >> iChild;
		
		ASSERT( !name.fail() );
		
		--iChild;	// Symbolic numbers are indexed starting with 1 rather than 0.
		
		if( iChild >= object->numChildren() )
		{
			dev_warning( "Keyframe found that symbolic child '$" << ( iChild + 1 ) << "' of MovieClip " << object->toString() << " was out of range (num children=" << object->numChildren() << ")." );
			return nullptr;
		}
		else
		{
			return object->getChildAt( iChild );
		}
	}

}
