/*
 *  AudioSound.h
 *
 *  Created by Jeff Wofford on 8/12/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_AUDIO_SOUND_H_INCLUDED_
#define FRESH_AUDIO_SOUND_H_INCLUDED_

#include "Singleton.h"
#include "Object.h"
#include "Asset.h"
#include "FreshVector.h"

namespace fr
{

	class AudioCue;
	
	class SoundGroup : public Object
	{
		FRESH_DECLARE_CLASS( SoundGroup, Object )
	public:
		
		typedef std::list< WeakPtr< SoundGroup > > Children;
		
		SYNTHESIZE( WeakPtr< SoundGroup >, parentGroup );
		
		SYNTHESIZE_GET( float, gain );
		void gain( float gain_ );
		
		SYNTHESIZE_GET( float, gainScalar );
		
		bool hasChild( SoundGroup::cptr child ) const;
		// REQUIRES( child );
		void addChild( SoundGroup::ptr child );
		// REQUIRES( child );
		// REQUIRES( !hasChild( child ));
		// PROMISES( hasChild( child ));
		void removeChild( SoundGroup::ptr child );
		// REQUIRES( child );
		// REQUIRES( hasChild( child ));
		// PROMISES( !hasChild( child ));

		Children::iterator begin();
		Children::iterator end();
		
	protected:
		
		virtual void applyGain();
		void cleanupChildren();
		void gainScalar( float gainScalar_ );
		
	private:
		
		DVAR( float, m_gain, 1.0f );
		DVAR( float, m_gainScalar, 1.0f );
		Children m_children;	
		WeakPtr< SoundGroup > m_parentGroup;				
	};
	
	////////////////////////////////////////////////////////////////////////////////////////////
	
	class Sound : public SoundGroup
	{
	public:
		
		virtual ~Sound();

		SmartPtr< AudioCue > cue() const;
		
		void play();
		void pause();
		void stop();
		bool isPlaying() const;
		
		TimeType getPlayHeadSeconds() const;
		
		void looping( bool doLoop );
		bool looping() const;
		
		void pitchScalar( float scale );
		// REQUIRES( scale > 0 );
		float pitchScalar() const;
		// PROMISES( result > 0 );
		
		// Spatial audio.
		//
		bool usesListenerRelativePosition() const;
		void usesListenerRelativePosition( bool uses );
		
		vec3 position() const;
		void position( const vec3& pos );
		// PROMISES( position.isZero() || usesListenerRelativePosition() == false );
		// PROMISES( position() == pos );
		void position( const vec2& pos )					{ position( vec3( pos.x, pos.y, 0.0f )); }
		
		float maxAudibleDistance() const;
		void maxAudibleDistance( float dist ) const;
		// If dist < 0, uses the system-wide default.
		
		float maxUnattenuatedDistance() const;
		void maxUnattenuatedDistance( float dist );
		// If dist < 0, uses the system-wide default.
		
	private:
		
		bool m_pendingPlay = false;
		bool m_hasBuffers = false;
		uint m_idALSource = 0;
		SmartPtr< AudioCue > m_cue;
		
		virtual void applyGain() override;

		friend class AudioCue;		// For access to:
		friend class AudioCueImpl;	// For access to:
		void cue( SmartPtr< AudioCue > cue_ );
		void enqueueBuffer( uint cueBuffer );
		void enqueueBuffers( const uint* cueBuffers, size_t nBuffers );
		
		FRESH_DECLARE_CLASS( Sound, SoundGroup )
	};

}

#endif
