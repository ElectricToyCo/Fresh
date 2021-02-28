/*
 *  Keyframe.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/24/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_KEYFRAME_H_INCLUDED
#define FRESH_KEYFRAME_H_INCLUDED

#include "../FreshCore/Object.h"
#include "../FreshPlatform/Texture.h"
#include "DisplayObjectState.h"
#include "Tweener.h"
#include <unordered_map>

namespace fr
{
	class MovieClip;
	
	// A Keyframe holds the state of a MovieClip in a fashion reminiscent of how a keyframe works in the Timeline in Adobe Flash.
	// Primarily, the Keyframe holds information about the MovieClip's children; namely, whether they exist in this frame or not, and what their transforms and colors are.
	// Keyframes can automatically tween between each other, smoothing out movement of children over time.
	// A Keyframe can optionally specify the MovieClip's texture. Call SetTexture() if you want the Keyframe to determine this. Otherwise, just don't call it (not even with null, which
	// the Keyframe will see as the value it should set in the MovieClip), in which case the Keyframe will leave the MovieClip's texture alone.
	//
	// Keyframes hide and show children through a special "isKeyframeVisible()" function in DisplayObject. This function trumps the usual visible() call.
	// It's best to avoid use of visible() or addChild()/removeChild() where Keyframed children are concerned.
	//
	//
	class Keyframe
	{
	public:
		
		explicit Keyframe( WeakPtr< MovieClip > deriveFromMovieClip = nullptr );
		Keyframe( Object::NameRef singleObjectName, const DisplayObjectState& state );

			// You may specify a MovieClip here in order to construct the Keyframe to reflect that clip's current situation (including texture).
			// Otherwise, creates a default Keyframe.
		
		void texture( Texture::ptr texture, const rect& textureSlice = rect( 0, 0, 1, 1 ));
		void setTextureByName( const std::string& strFileName, const rect& textureSlice = rect( 0, 0, 1, 1 ) );
		Texture::ptr texture() const;
		
		void ignoreClipTexture( bool ignore );
		bool ignoreClipTexture() const;

		void selfPosition( const vec2& pos );
		SYNTHESIZE_GET( vec2, selfPosition );
		SYNTHESIZE( bool, isSelfPositionSet );

		void selfPivot( const vec2& pivot );
		SYNTHESIZE_GET( vec2, selfPivot );
		SYNTHESIZE( bool, isSelfPivotSet );
		
		SYNTHESIZE( bool, replayShownClipChildren );
		
		SYNTHESIZE( std::string, action );

		void setChildState( Object::NameRef childName, const DisplayObjectState& state );
			// Sets the desired state for the child that has the specified name/objectName.
			// Exactly one child *must* have the specified name.
		void setChildState( const DisplayObject& child );

		void clearChildState( Object::NameRef childName );

		bool hasChildState( Object::NameRef childName ) const;
		const DisplayObjectState& getChildState( Object::NameRef childName ) const;
		
		Keyframe getTweenedStateTo( const Keyframe& destination, real normalizedTime, const Tweener< DisplayObjectState >& tweener ) const;
			// REQUIRES( 0 <= normalizedTime && normalizedTime <= 1.0 );
		
		void fixupSymbolicNames( WeakPtr< const MovieClip > object );
		
		void applyStateTo( WeakPtr< MovieClip > object ) const;
			// REQUIRES( object );

		bool isSymbolicName( Object::NameRef childName ) const;
		
		void serialize( const MovieClip& host, class ObjectStreamFormatter& formatter ) const;
		
	protected:
		
		WeakPtr< DisplayObject > getChildFromSymbol( WeakPtr< const MovieClip > object, const std::string& symbol ) const;
		// REQUIRES( object );
		
	private:
		
		typedef std::unordered_map< Object::Name, DisplayObjectState > MapChildStates;
		typedef MapChildStates::iterator MapChildStatesI;
		typedef MapChildStates::const_iterator MapChildStatesCI;
		
		Texture::ptr 	m_clipTexture;
		rect			m_textureSlice;
		bool			m_isTextureSet = false;			// If not, this keyframe doesn't affect the texture at all.
		MapChildStates  m_mapChildStates;
		
		std::string		m_action;
		
		vec2			m_selfPosition;
		bool			m_isSelfPositionSet = false;

		vec2			m_selfPivot;
		bool			m_isSelfPivotSet = false;
		
		bool			m_replayShownClipChildren = false;
		
	};
	
}

#endif
