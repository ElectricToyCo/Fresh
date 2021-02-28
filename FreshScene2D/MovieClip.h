/*
 *  MovieClip.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 7/8/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_MOVIE_CLIP_H_INCLUDED
#define FRESH_MOVIE_CLIP_H_INCLUDED

#include "Sprite.h"
#include "Keyframe.h"
#include <map>

namespace fr
{
	class MovieClip : public Sprite
	{
	public:
		
		static const size_t INVALID_FRAME_INDEX = -1;
		static const char* TWEEN_FINISHED;		// Event type for receiving a callback whenever a tween completes. Even "instant" (duration==0) and "failed" (requested label doesn't exist) tweens dispatch this event.
		static const char* PLAYBACK_REACHED_END;
		
		typedef std::string Label;
		typedef const Label& LabelRef;

		
		//
		// Flash-style timeline functions.
		//
		
		bool isPlaying() const;
		virtual void play();
		virtual void stop();
		virtual void gotoAndPlay( size_t iFrame );
			// REQUIRES( iFrame <= getMaxKeyframeIndex() );
		virtual void gotoAndPlay( LabelRef label );
		virtual void gotoAndStop( size_t iFrame );
			// REQUIRES( iFrame <= getMaxKeyframeIndex() );
		virtual void gotoAndStop( LabelRef label );
		
		// Primarily for scriptable "methods"
		//
		void gotoAndPlayIndex( size_t iFrame )						{ gotoAndPlay( iFrame ); }
		void gotoAndPlayLabel( LabelRef label )						{ gotoAndPlay( label ); }
		void gotoAndStopIndex( size_t iFrame )						{ gotoAndStop( iFrame ); }
		void gotoAndStopLabel( LabelRef label )						{ gotoAndStop( label ); }
		
		SYNTHESIZE( TimeType, animFramesPerSecond )
		
		bool isLooping() const										{ return m_isLooping; }
		virtual void setLooping( bool loop )						{ m_isLooping = loop; }
		
		size_t maxKeyframeIndex() const;
			// Returns the index of the maximum keyframe index that has been set, or -1 if none set.
		
		SYNTHESIZE( bool, animatesWholeTree )
		
		//
		// Querying whether keyframes have been set.
		//
		
		bool hasKeyframe( size_t iFrame ) const;
		bool hasKeyframe( LabelRef label ) const;
			// REQUIRES( !label.empty() );
		
		//
		// Setting and getting keyframes by index and string label.
		//
		
		void createBlankKeyframe( size_t iFrame );
			// PROMISES( hasKeyframe( iFrame ));
		void setKeyframe( size_t iFrame, const Keyframe& keyFrame );
			// PROMISES( hasKeyframe( iFrame ));
		void setKeyframe( LabelRef label, const Keyframe& keyFrame );
			// REQUIRES( label.length() > 0 );
			// If no keyframe exists for this label, this function assigns the label to the next lowest unused keyframe index.
			// PROMISES( hasKeyframe( label ));
		const Keyframe& getKeyframe( size_t iFrame ) const;
		Keyframe& getKeyframe( size_t iFrame );
			// REQUIRES( hasKeyframe( iFrame ));
		const Keyframe& getKeyframe( LabelRef label ) const;
		Keyframe& getKeyframe( LabelRef label );
			// REQUIRES( hasKeyframe( label ));
		void clearKeyframe( size_t iFrame );
			// PROMISES( !hasKeyframe( iFrame ));
		void clearKeyframe( LabelRef label );
			// REQUIRES NOTHING
			// PROMISES( !hasKeyframe( label ));
		
		void clearKeyframes();
		
		//
		// Labeling frame numbers with string names.
		//
		
		Label getLabelForIndex( size_t iFrame ) const;
			// REQUIRES( HasKeyframe( iFrame ));
			// Returns "" if no such index, or the indexed frame has no label.
		size_t getIndexForLabel( LabelRef label ) const;
			// Returns -1 if no such label exists.
		void setLabelForIndex( LabelRef label, size_t iFrame );
			// REQUIRES( HasKeyframe( iFrame ));
			// If label == "", the frame label, if any, is cleared.
		
		template< typename FunctionT >
		void eachLabel( FunctionT&& fn ) const
		{
			for( const auto& pair : m_mapLabels )
			{
				fn( pair.first, pair.second );
			}
		}
		
		//
		// Keyframe-based tweens.
		//
		
		void createTween( size_t iFrame, const Tweener< DisplayObjectState >* tweener = &( DisplayObjectState::tweenerQuadEaseInOut ));
			// REQUIRES( HasKeyframe( iFrame ));
			// PROMISES( HasTween( iFrame ));
		void createTween( LabelRef label, const Tweener< DisplayObjectState >* tweener = &( DisplayObjectState::tweenerQuadEaseInOut ));
			// REQUIRES( HasKeyframe( label ));
			// PROMISES( HasTween( label ));
		void removeTween( size_t iFrame );
			// REQUIRES( HasKeyframe( iFrame ));
			// PROMISES( !HasTween( iFrame ));
		void removeTween( LabelRef label );
			// REQUIRES( HasKeyframe( label ));
			// PROMISES( !HasTween( label ));
		bool hasTween( size_t iFrame ) const;
			// REQUIRES( HasKeyframe( iFrame ));
		bool hasTween( LabelRef label ) const;
			// REQUIRES( HasKeyframe( label ));
		
		
		//
		// "Free" tweening between the current frame and an arbitrary destination keyframe.
		// This kind of tweening ignores the timeline--it can tween between any two keyframes over any interval.
		// But you should probably stop() frame-based playback while using this feature.
		//
		
		void tweenToKeyframe( size_t iFrame, TimeType tweenDuration, const Tweener< DisplayObjectState >* tweener = &( DisplayObjectState::tweenerQuadEaseInOut ), bool tweenFromCurrentState = false );
			// REQUIRES( tweenDuration >= 0 );
		void tweenToKeyframe( LabelRef label, TimeType tweenDuration, const Tweener< DisplayObjectState >* tweener = &( DisplayObjectState::tweenerQuadEaseInOut ), bool tweenFromCurrentState = false );
			// REQUIRES( label.length() > 0 );
			// REQUIRES( tweenDuration >= 0 );
		
		
		//
		// Jumping to arbitrary destination keyframes.
		//
		
		void gotoKeyframe( size_t iFrame )		 																	{ tweenToKeyframe( iFrame, 0 ); }
		void gotoKeyframe( LabelRef label ) 																		{ tweenToKeyframe( label, 0 ); }
			// REQUIRES( label.length() > 0 );
		
		// 
		// Queries regarding tweening and the current "playback head" state.
		//
		
		size_t currentFrame() const;
		Label currentLabel() const;
			// PROMISES NOTHING
		bool isTweening() const;
		size_t getTweenDestinationIndex() const;
			// PROMISES NOTHING
			// If no tween currently specified, returns INVALID_FRAME_INDEX.
		Label getTweenDestinationLabel() const;
			// PROMISES NOTHING

		
		virtual void update() override;

		virtual void load( const Manifest::Map& properties ) override;
		virtual void serialize( class ObjectStreamFormatter& formatter, bool doWriteEvenIfDefault = false ) const override;
		virtual void postLoad() override;
		virtual void onAddedToStage() override;
		
	protected:

		struct SimpleTextureAnimation : public SerializableStruct< SimpleTextureAnimation >
		{
			float sliceWidth = 0;
			int nFrames = 0;
			TimeType framesPerSecond = 10;
			
			SimpleTextureAnimation()
			{
				STRUCT_BEGIN_PROPERTIES
				STRUCT_ADD_PROPERTY( sliceWidth )
				STRUCT_ADD_PROPERTY( nFrames )
				STRUCT_ADD_PROPERTY( framesPerSecond )
				STRUCT_END_PROPERTIES
			}
			
			bool operator==( const SimpleTextureAnimation& other ) const
			{
				return sliceWidth == other.sliceWidth &&
						nFrames == other.nFrames &&
						framesPerSecond == other.framesPerSecond;
			}
		};
		
		STRUCT_DECLARE_SERIALIZATION_OPERATORS( SimpleTextureAnimation )
		
		virtual void onTweenComplete();
		virtual void onPlaybackReachedEnd();
		
		void deleteAnyExistingKeyframe( size_t iFrame );
		void ensureTimelineLongEnoughFor( size_t iFrame );
		void compactTimeline();
		
		virtual void setCurrentFrame( size_t iFrame );
		
		size_t getNextKeyframeAfter( size_t iFrame ) const;
			// If no keyframe after this one, returns -1;
		
		void fixupKeyframeNames();
		
		void setupSimpleTextureAnimationFrames( const SimpleTextureAnimation& simpleTextureAnimation );

		void writeKeyframe( class ObjectStreamFormatter& formatter, const Keyframe& keyframe, size_t iFrame ) const;
		
		void loadKeyframes( const Manifest::Array& keyframes );
		
	private:
		
		typedef std::map< Label, size_t > MapLabelsToFrameIndices;
		typedef std::map< size_t, const Tweener< DisplayObjectState >* > MapKeyframeTweens;
		
		VAR( MapLabelsToFrameIndices, m_mapLabels );
		
		DVAR( size_t,				m_currentFrame, 0 );
		DVAR( bool,					m_isPlaying, true );
		DVAR( bool,					m_isLooping, true );
		DVAR( bool,					m_animatesWholeTree, false );
		DVAR( TimeType,				m_tweenDuration, 0 );
		DVAR( real,					m_tweenEasing, 0 );
		VAR( SimpleTextureAnimation,	m_simpleTextureAnimation );
		DVAR( TimeType,				m_animFramesPerSecond, 0 );					 // If 0, matches the game frame rate.
		
		std::vector< std::unique_ptr< Keyframe >> m_vecFrames;
		MapKeyframeTweens	m_mapKeyframeTweens;

		TimeType			m_tweenStartTime = -1.0F;
		Keyframe			m_tweenStartFrame;

		size_t				m_nextLowestUnusedKeyframeIndex = 0;
		size_t				m_tweenEndFrameIndex = INVALID_FRAME_INDEX;
		
		TimeType			m_lastFrameStepTime = -1;
		size_t				m_lastAppliedFrame = -1;
		
		bool m_hasFixupCompleted = false;
		
		const Tweener< DisplayObjectState >*		m_tweener = &DisplayObjectState::tweenerQuadEaseInOut;
				
		FRESH_DECLARE_CLASS( MovieClip, Sprite )
	};
	
}

#endif
