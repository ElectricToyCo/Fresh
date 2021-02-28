/*
 *  MovieClip.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 7/8/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#include "MovieClip.h"
#include "Objects.h"
#include "Stage.h"
#include "Application.h"
#include "Profiler.h"
#include "CommandProcessor.h"

namespace
{
	using namespace fr;
	
	const size_t MAX_REASONABLE_FRAME_INDEX = 100000;
	
	std::string propertyText( const Manifest::Map::const_iterator& iterator )
	{
		return iterator->second.first->get< std::string >();
	}
	
	template< typename PropertyT >
	inline void assignStateProperty( const Manifest::Object& childObject,
									DisplayObjectState& state,
#ifndef _MSC_VER
									typename fr::passing< PropertyT >::type (DisplayObjectState::*assignment)( typename fr::passing< PropertyT >::type value ),
#else
									PropertyT (DisplayObjectState::*assignment)( PropertyT value ),
#endif
									const char* childPropertyName )
	{
		ASSERT( childObject.map );
		const auto& properties = *childObject.map;
		const auto property = properties.find( childPropertyName );
		
		if( property != properties.end() )
		{
			Destringifier ss( propertyText( property ));
			
			PropertyT propertyValue;
			ss >> propertyValue;
			
			(state.*assignment)( propertyValue );
		}
	}
	

}

namespace fr
{
	STRUCT_DEFINE_SERIALIZATION_OPERATORS( MovieClip::SimpleTextureAnimation )
	
	FRESH_DEFINE_CLASS( MovieClip )
	
	DEFINE_METHOD( MovieClip, gotoAndPlayIndex )
	DEFINE_METHOD( MovieClip, gotoAndPlayLabel )
	DEFINE_METHOD( MovieClip, gotoAndStopIndex )
	DEFINE_METHOD( MovieClip, gotoAndStopLabel )
	
	DEFINE_VAR( MovieClip, MapLabelsToFrameIndices, m_mapLabels );
	DEFINE_VAR( MovieClip, size_t,				m_currentFrame );
	DEFINE_VAR( MovieClip, bool,				m_isPlaying );
	DEFINE_VAR( MovieClip, bool,				m_isLooping );
	DEFINE_VAR( MovieClip, bool,				m_animatesWholeTree );
	DEFINE_VAR( MovieClip, TimeType,			m_tweenDuration );
	DEFINE_VAR( MovieClip, real,				m_tweenEasing );
	DEFINE_VAR( MovieClip, SimpleTextureAnimation,	m_simpleTextureAnimation );
	DEFINE_VAR( MovieClip, TimeType,				m_animFramesPerSecond );

	DEFINE_METHOD( MovieClip, play )
	DEFINE_METHOD( MovieClip, stop )

	const char* MovieClip::TWEEN_FINISHED = "TweenFinished";
	const char* MovieClip::PLAYBACK_REACHED_END = "PlaybackReachedEnd";

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( MovieClip )

	MovieClip::MovieClip( const ClassInfo& assignedClassInfo, NameRef objectName /* = DEFAULT_OBJECT_NAME */ )
	:	Sprite( assignedClassInfo, objectName )
	,	m_tweener( & DisplayObjectState::tweenerQuadEaseInOut )
	{}

	bool MovieClip::isPlaying() const
	{
		return m_isPlaying;
	}

	void MovieClip::play()
	{
		m_isPlaying = true;
	}

	void MovieClip::stop()
	{
		m_isPlaying = false;
		
		// Stop any ongoing tween.
		//
		m_tweenStartTime = -1;
	}

	void MovieClip::gotoAndPlay( size_t iFrame )
	{
		if( iFrame <= maxKeyframeIndex() )
		{
			m_isPlaying = true;
			m_lastFrameStepTime = -1;
			setCurrentFrame( iFrame );
		}
		else
		{
			dev_warning( this << " requested out-of-range frame #" << iFrame << "." );
		}
	}

	void MovieClip::gotoAndPlay( LabelRef label )
	{
		auto index = getIndexForLabel( label );
		if( index <= maxKeyframeIndex() )
		{
			gotoAndPlay( index );
		}
		else
		{
			dev_warning( this << " requested unknown frame label '" << label << "'." );
		}
	}
	
	void MovieClip::gotoAndStop( size_t iFrame )
	{
		if( iFrame != INVALID_FRAME_INDEX && iFrame <= maxKeyframeIndex() )
		{
			m_isPlaying = false;
			
			// Stop any ongoing tween.
			//
			m_tweenStartTime = -1;
			
			m_lastFrameStepTime = -1;
			setCurrentFrame( iFrame );
		}
		else
		{
			dev_warning( this << " requested out-of-range frame #" << iFrame << "." );
		}
	}

	void MovieClip::gotoAndStop( LabelRef label )
	{
		gotoAndStop( getIndexForLabel( label ));
	}
	
	size_t MovieClip::maxKeyframeIndex() const
	{
		return m_vecFrames.size() - 1;
	}

	MovieClip::Label MovieClip::getLabelForIndex( size_t iFrame ) const
	{
		REQUIRES( iFrame != INVALID_FRAME_INDEX );

		// Return the closest label that is at or before this frame number.
		//
		size_t closestDist = size_t( -1 );
		Label closestEarlierLabel;
		
		for( auto iter = m_mapLabels.begin(); iter != m_mapLabels.end(); ++iter )
		{
			size_t dist = iFrame - iter->second;
			
			if( dist < closestDist )
			{
				closestDist = dist;
				closestEarlierLabel = iter->first;
				
				if( dist == 0 )
				{
					// Can't get any better. Stop looking.
					break;
				}
			}
		}
		return closestEarlierLabel;
	}

	size_t MovieClip::getIndexForLabel( LabelRef label ) const
	{
		const auto iter = m_mapLabels.find( label );
		if( iter != m_mapLabels.end() )
		{
			return iter->second;
		}
		else
		{
			return INVALID_FRAME_INDEX;
		}
	}

	void MovieClip::setLabelForIndex( LabelRef label, size_t iFrame )
	{
		REQUIRES( hasKeyframe( iFrame ));
		
		if( label.empty() )
		{
			// Clear the frame label for this index, if any.
			//
			for( auto iter = m_mapLabels.begin(); iter != m_mapLabels.end(); ++iter )
			{
				if( iter->second == iFrame )
				{
					m_mapLabels.erase( iter );
					break;
				}
			}
		}
		else
		{
			m_mapLabels[ label ] = iFrame;
		}
	}

	bool MovieClip::hasKeyframe( size_t iFrame ) const
	{
		return iFrame < m_vecFrames.size() && m_vecFrames[ iFrame ] != nullptr;
	}

	bool MovieClip::hasKeyframe( LabelRef label ) const
	{
		REQUIRES( label.length() > 0 );
		
		size_t iFrame = getIndexForLabel( label );
		
		if( iFrame != INVALID_FRAME_INDEX )
		{
			return hasKeyframe( iFrame );
		}
		else
		{
			return false;
		}
	}

	void MovieClip::createBlankKeyframe( size_t iFrame )
	{
		deleteAnyExistingKeyframe( iFrame );
		ensureTimelineLongEnoughFor( iFrame );
		
		m_vecFrames[ iFrame ].reset( new Keyframe() );
		
		m_nextLowestUnusedKeyframeIndex = std::max( m_nextLowestUnusedKeyframeIndex, iFrame + 1 );
		
		PROMISES( hasKeyframe( iFrame ));
	}

	void MovieClip::setKeyframe( size_t iFrame, const Keyframe& keyFrame )
	{
		REQUIRES( iFrame != INVALID_FRAME_INDEX );
		
		deleteAnyExistingKeyframe( iFrame );
		ensureTimelineLongEnoughFor( iFrame );
		
		m_vecFrames[ iFrame ].reset( new Keyframe( keyFrame ));

		m_nextLowestUnusedKeyframeIndex = std::max( m_nextLowestUnusedKeyframeIndex, iFrame + 1 );
		
		// If the frame we're currently on has changed, move with it. Only after completed loaded.
		//
		if( m_hasFixupCompleted && currentFrame() == iFrame )
		{
			setCurrentFrame( iFrame );
		}
		
		PROMISES( hasKeyframe( iFrame ));
	}

	void MovieClip::setKeyframe( LabelRef label, const Keyframe& keyFrame )
	{
		REQUIRES( label.length() > 0 );

		size_t iFrame = getIndexForLabel( label );

		if( iFrame == INVALID_FRAME_INDEX )
		{
			iFrame = m_nextLowestUnusedKeyframeIndex;
			++m_nextLowestUnusedKeyframeIndex;
			
			createBlankKeyframe( iFrame );
			setLabelForIndex( label, iFrame );
		}
		
		setKeyframe( iFrame, keyFrame );
		
		PROMISES( hasKeyframe( label ));
	}

	const Keyframe& MovieClip::getKeyframe( size_t iFrame ) const
	{
		REQUIRES( hasKeyframe( iFrame ));
		return *m_vecFrames[ iFrame ];
	}

	Keyframe& MovieClip::getKeyframe( size_t iFrame )
	{
		REQUIRES( hasKeyframe( iFrame ));
		return *m_vecFrames[ iFrame ];
	}
	
	const Keyframe& MovieClip::getKeyframe( LabelRef label ) const
	{
		REQUIRES( hasKeyframe( label ));		
		return *m_vecFrames[ getIndexForLabel( label ) ];
	}

	Keyframe& MovieClip::getKeyframe( LabelRef label )
	{
		REQUIRES( hasKeyframe( label ));		
		return *m_vecFrames[ getIndexForLabel( label ) ];
	}
	
	void MovieClip::clearKeyframe( size_t iFrame )
	{
		REQUIRES( iFrame != INVALID_FRAME_INDEX);

		if( iFrame < m_vecFrames.size() )
		{
			m_vecFrames[ iFrame ] = nullptr;
			compactTimeline();
		}
		
		PROMISES( !hasKeyframe( iFrame ));
	}

	void MovieClip::clearKeyframe( LabelRef label )
	{
		size_t iFrame = getIndexForLabel( label );
		
		if( iFrame != INVALID_FRAME_INDEX )
		{
			clearKeyframe( iFrame );
		}
		
		PROMISES( !hasKeyframe( label ));
	}

	void MovieClip::clearKeyframes()
	{
		m_vecFrames.clear();
		m_mapKeyframeTweens.clear();
		m_mapLabels.clear();
	}
	
	void MovieClip::createTween( size_t iFrame, const Tweener< DisplayObjectState >* tweener /* = &( DisplayObjectState::tweenerQuadEaseInOut ) */ )
	{
		REQUIRES( hasKeyframe( iFrame ));
		
		m_mapKeyframeTweens[ iFrame ] = tweener;
		
		PROMISES( hasTween( iFrame ));
	}

	void MovieClip::createTween( LabelRef label, const Tweener< DisplayObjectState >* tweener /* = &( DisplayObjectState::tweenerQuadEaseInOut ) */ )
	{
		REQUIRES( hasKeyframe( label ));
		
		createTween( getIndexForLabel( label ), tweener );
		
		PROMISES( hasTween( label ));
	}

	void MovieClip::removeTween( size_t iFrame )
	{
		const auto iter = m_mapKeyframeTweens.find( iFrame );
		
		REQUIRES( iter != m_mapKeyframeTweens.end() );
		
		m_mapKeyframeTweens.erase( iter );
		
		PROMISES( !hasTween( iFrame ));
	}

	void MovieClip::removeTween( LabelRef label )
	{
		REQUIRES( hasKeyframe( label ));
		
		removeTween( getIndexForLabel( label ));
		
		PROMISES( !hasTween( label ));
	}

	bool MovieClip::hasTween( size_t iFrame ) const
	{
		REQUIRES( hasKeyframe( iFrame ));
		
		return m_mapKeyframeTweens.find( iFrame ) != m_mapKeyframeTweens.end();
	}

	bool MovieClip::hasTween( LabelRef label ) const
	{
		REQUIRES( hasKeyframe( label ));
		
		return hasTween( getIndexForLabel( label ));
	}

	void MovieClip::tweenToKeyframe( size_t iFrame, TimeType tweenDuration, const Tweener< DisplayObjectState >* tweener /* = &( DisplayObjectState::tweenerQuadEaseInOut ) */, bool tweenFromCurrentState )
	{
		TIMER_AUTO( MovieClip::tweenToKeyframe with iFrame )

		REQUIRES( iFrame != INVALID_FRAME_INDEX );
		REQUIRES( tweenDuration >= 0 );
		if( !tweener )
		{
			tweener = &( DisplayObjectState::tweenerQuadEaseInOut );
		}
		
		if( !hasKeyframe( iFrame ))
		{
			// Ignore this condition (as opposed to ASSERTing it). It helps with movieclips that may or may not have a keyframe and that's okay--like SimpleButtons.
			//
			TIMER_AUTO( Calling OnTweenComplete )
			onTweenComplete();
			return;
		}
		
		m_tweenEndFrameIndex = iFrame;
		m_tweener = tweener;
		
		if( tweenDuration <= std::numeric_limits< real >::epsilon() )
		{
			// Instant.
			//
			m_tweenStartTime = -1;
			
			setCurrentFrame( m_tweenEndFrameIndex );
			m_tweenEndFrameIndex = INVALID_FRAME_INDEX;
			
			onTweenComplete();
		}
		else
		{
			m_tweenStartTime = stage().time();
			m_tweenDuration = tweenDuration;
			
			// If we have a keyframe for the current frame, use it directly.
			// Otherwise, use the current state of the MovieClip itself.
			// The big difference in behavior here is that keyframes may store rotations as floats, for example,
			// whereas MovieClips (like other DisplayObjects) store them as angles.
			// Therefore reading the keyframe from the object itself may truncate out-of-range angle numbers that are actually correct.
			//
			if( !tweenFromCurrentState && hasKeyframe( m_currentFrame ))
			{
				m_tweenStartFrame = getKeyframe( m_currentFrame );
			}
			else
			{
				m_tweenStartFrame = Keyframe( this );			// Get the current state.
			}
			
			// Make sure that the whole tween either manipulates the clip's texture or not.
			//
			const Keyframe& keyframe = getKeyframe( m_tweenEndFrameIndex );
			m_tweenStartFrame.ignoreClipTexture( keyframe.ignoreClipTexture() );
		}
	}

	void MovieClip::tweenToKeyframe( LabelRef label, TimeType tweenDuration, const Tweener< DisplayObjectState >* tweener, bool tweenFromCurrentState )
	{
		TIMER_AUTO( MovieClip::tweenToKeyframe with label )

		size_t iFrame = getIndexForLabel( label );
		
		if( iFrame == INVALID_FRAME_INDEX )
		{
			// Ignore this condition (as opposed to ASSERTing it). It helps with movieclips that may or may not have a keyframe and that's okay--like SimpleButtons.
			//
			onTweenComplete();
			return;
		}
		
		tweenToKeyframe( iFrame, tweenDuration, tweener, tweenFromCurrentState );
	}

	size_t MovieClip::currentFrame() const
	{
		PROMISES( m_currentFrame >= 0 );
		return m_currentFrame;
	}

	MovieClip::Label MovieClip::currentLabel() const
	{
		return getLabelForIndex( m_currentFrame );
	}

	bool MovieClip::isTweening() const
	{
		return m_tweenStartTime >= 0;
	}

	size_t MovieClip::getTweenDestinationIndex() const
	{
		return m_tweenEndFrameIndex;
	}

	MovieClip::Label MovieClip::getTweenDestinationLabel() const
	{
		if( m_tweenEndFrameIndex != INVALID_FRAME_INDEX )
		{
			return getLabelForIndex( m_tweenEndFrameIndex );
		}
		else
		{
			return "";
		}
	}

	void MovieClip::update()
	{
		// If the currentFrame has magically changed somehow (e.g. the editor or console), apply it.
		//
		if( m_lastAppliedFrame != m_currentFrame )
		{
			setCurrentFrame( m_currentFrame );
		}
		
		// Update tweening.
		//
		if( m_tweenStartTime >= 0 )
		{
			TimeType currentTime = stage().time();
			TimeType actualDuration = currentTime - m_tweenStartTime;
			TimeType normalizedDuration = clamp( actualDuration / m_tweenDuration, (TimeType)0.0, (TimeType)1.0 );
			
			if(( actualDuration - m_tweenDuration ) >= -std::numeric_limits< TimeType >::epsilon() )
			{
				// Tween completed.
				//
				size_t newCurrentFrame = m_tweenEndFrameIndex;
				
				m_tweenEndFrameIndex = INVALID_FRAME_INDEX;
				m_tweenStartTime = -1;			
				
				setCurrentFrame( newCurrentFrame );
				
				onTweenComplete();
			}
			else
			{
				const Keyframe& tweenEndFrame = getKeyframe( m_tweenEndFrameIndex );			
				Keyframe tweenedFrame = 
					m_tweenStartFrame.getTweenedStateTo( tweenEndFrame, static_cast< fr::real >( normalizedDuration ), *m_tweener );
				tweenedFrame.applyStateTo( this );				
			}
		}

		// Update "playhead" playback.
		//	
		if( m_isPlaying )
		{
			// Time for the next step?
			//
			ASSERT( hasStage() );
			auto now = stage().time();
			
			if( m_animFramesPerSecond == 0 || ( now >= m_lastFrameStepTime + ( 1.0 / m_animFramesPerSecond )))
			{
				m_lastFrameStepTime = now;

				size_t iNextFrame = m_currentFrame + 1;
				
				// Stop or loop if we reach the end of the timeline.
				//
				if( iNextFrame >= m_vecFrames.size() )
				{
					if( m_isLooping )
					{
						setCurrentFrame( 0 );
						
						if( iNextFrame > 1 )
						{
							// TODO recursion is overkill. Should recordPreviousState() for children only.
							recordPreviousState( true );	// Don't attempt to tween between render frames from the last keyframe and the current one.
						}
					}
					else
					{
						stop();

						onPlaybackReachedEnd();
					}
				}
				else
				{
					// Just move to the next index.
					//
					setCurrentFrame( iNextFrame );
				}
			}
		}
		
		Sprite::update();
	}

	void MovieClip::loadKeyframes( const Manifest::Array& keyframes )
	{
		// Specially load children via keyframe elements.
		//
		size_t lastFrameIndex = 0;
		for( const auto& keyframeInfo : keyframes )
		{
			const auto& keyframeObject = keyframeInfo->get< Manifest::Object >();
			ASSERT( keyframeObject.className == "keyframe" );
			ASSERT( keyframeObject.map );
			
			const auto& keyframeProperties = *keyframeObject.map;
			
			Keyframe keyframe;
			
			// Does this keyframe specify a texture?
			//
			const auto textureProperty = keyframeProperties.find( "texture" );
			if( textureProperty != keyframeProperties.end() )
			{
				// Yes. Set it.
				//
				Texture::ptr frameTexture;
				{
					const auto& textureName = propertyText( textureProperty );
					Destringifier destringifier( textureName );
					destringifier >> frameTexture;
				}
				
				// Does this keyframe specify a texture window?
				//
				rect textureWindow( 0, 0, 1, 1 );
				
				const auto textureWindowProperty = keyframeProperties.find( "textureWindow" );
				if( textureWindowProperty != keyframeProperties.end() )
				{
					// Yes. Set it.
					//
					Destringifier destringifier( propertyText( textureWindowProperty ) );
					destringifier >> textureWindow;
				}
				
				keyframe.texture( frameTexture, textureWindow );
			}
			
			const auto selfPositionProperty = keyframeProperties.find( "position" );
			if( selfPositionProperty != keyframeProperties.end() )
			{
				vec2 selfPosition;
				const auto positionText = propertyText( selfPositionProperty );
				Destringifier destringifier( positionText );
				destringifier >> selfPosition;
				keyframe.selfPosition( selfPosition );
			}
			
			const auto selfPivotProperty = keyframeProperties.find( "pivot" );
			if( selfPivotProperty != keyframeProperties.end() )
			{
				vec2 selfPivot;
				auto pivotText = propertyText( selfPivotProperty );
				Destringifier destringifier( pivotText );
				destringifier >> selfPivot;
				keyframe.selfPivot( selfPivot );
			}
			
			// What about child elements?
			//
			const auto childrenProperty = keyframeProperties.find( "children" );
			if( childrenProperty != keyframeProperties.end() )
			{
				const auto& children = childrenProperty->second.first->get< Manifest::Array >();
				
				for( const auto& childValue : children )
				{
					if( !childValue->is< Manifest::Object >())
					{
						dev_error( "Keyframe child value was not an object." );
						for( DisplayObjectContainer::ptr parent = this; parent; parent = parent->parent() )
						{
							dev_error( "\t" << parent );
						}
						continue;
					}
					
					const auto& child = childValue->get< Manifest::Object >();
					const auto& childName = child.name();
					ASSERT( child.className == "child" );
					ASSERT( !childName.empty() );
					
					const Object::Name parsedChildName = parseObjectName( childName );
					
					DisplayObjectState state;
					
					assignStateProperty< vec2 >( child, state, &DisplayObjectState::position, "position" );
					assignStateProperty< vec2 >( child, state, &DisplayObjectState::scale, "scale" );
					assignStateProperty< real >( child, state, &DisplayObjectState::rotation, "rotation" );
					assignStateProperty< Color >( child, state, &DisplayObjectState::color, "color" );
					assignStateProperty< Color >( child, state, &DisplayObjectState::colorAdditive, "colorAdditive" );
					assignStateProperty< vec2 >( child, state, &DisplayObjectState::pivot, "pivot" );
					assignStateProperty< rect >( child, state, &DisplayObjectState::frame, "frame" );
					
					if( keyframe.hasChildState( parsedChildName ))
					{
						dev_warning( "While loading, " << toString() << " found a keyframe with more than one element for child '" << parsedChildName << "'. Overwriting earlier information." );
					}
					
					keyframe.setChildState( parsedChildName, state );
				}
			}
			
			// Get the label and/or frame index (t or s).
			//
			const auto defaultedPropertyText = []( const Manifest::Map& properties, const std::string& propertyName )
			{
				const auto property = properties.find( propertyName );
				if( property != properties.end() )
				{
					return propertyText( property );
				}
				else
				{
					return std::string{};
				}
			};
			
			const auto szIndex = defaultedPropertyText( keyframeProperties, "t" );
			const auto szSeconds = defaultedPropertyText( keyframeProperties, "s" );
			const auto szRelativeIndex = defaultedPropertyText( keyframeProperties, "rel_t" );
			const auto szRelativeSeconds = defaultedPropertyText( keyframeProperties, "rel_s" );
			
			const auto timeIndicators = {
				std::make_tuple( szIndex, false, false ),
				std::make_tuple( szSeconds, false, true ),
				std::make_tuple( szRelativeIndex, true, false ),
				std::make_tuple( szRelativeSeconds, true, true ) };
			
			typedef std::tuple< const std::string&, bool, bool > Indicator;
			
			const auto actualIndicator = []( const Indicator& indicator ) { return !std::get< 0 >( indicator ).empty(); };
			
			const auto nTimeIndicatorsSet = std::count_if( timeIndicators.begin(), timeIndicators.end(), actualIndicator );
			
			size_t iFrame = INVALID_FRAME_INDEX;
			
			// Did the element specify an index?
			//
			if( nTimeIndicatorsSet == 0 )
			{
				// No index found. Just use the next available index.
				//
				iFrame = maxKeyframeIndex() + 1;
			}
			else
			{
				if( nTimeIndicatorsSet > 1 )
				{
					dev_warning( toString() << " found more than one keyframe time indicator." );
				}
				
				auto iter = std::find_if( timeIndicators.begin(), timeIndicators.end(), actualIndicator );
				ASSERT( iter != timeIndicators.end() );
				const Indicator& indicatorToUse = *iter;
				
				Destringifier ss( std::get< 0 >( indicatorToUse ));
				
				size_t frameStep = -1;
				
				if( std::get< 2 >( indicatorToUse ) )		// Seconds (as opposed to frame indices)
				{
					TimeType seconds;
					ss >> seconds;
					
					frameStep = static_cast< size_t >( fr::round( seconds * Application::instance().desiredFramesPerSecond() ));
				}
				else
				{
					ss >> frameStep;
				}
				
				
				if( std::get< 1 >( indicatorToUse ))		// Relative (as opposed to absolute).
				{
					// Step at least one frame.
					iFrame = lastFrameIndex + std::max( static_cast< decltype( frameStep ) >( 1 ), frameStep );
				}
				else
				{
					iFrame = frameStep;
				}
			}
			
			if( iFrame != INVALID_FRAME_INDEX && iFrame > MAX_REASONABLE_FRAME_INDEX )
			{
				dev_warning( "While loading, " << toString() << " found a huge keyframe index (" << iFrame << "). Ignoring this index." );
				iFrame = INVALID_FRAME_INDEX;
			}
			
			// If we've determined a sensible index for the keyframe, add it at that index.
			//
			auto label = defaultedPropertyText( keyframeProperties, "label" );
			
			if( iFrame != INVALID_FRAME_INDEX )
			{
				if( hasKeyframe( iFrame ))
				{
					dev_warning( "While loading, " << toString() << " had a keyframe that overwrote an existing keyframe at index " << iFrame );
				}
				
				// Does this keyframe want to replay children whenever the keyframe is played?
				//
				const auto replayChildrenProperty = keyframeProperties.find( "replay_children" );
				if( replayChildrenProperty != keyframeProperties.end() )
				{
					bool replayClipChildren = false;
					
					Destringifier ss( propertyText( replayChildrenProperty ));
					ss >> replayClipChildren;
					
					keyframe.replayShownClipChildren( replayClipChildren );
				}
				
				// Do we like an action with this keyframe?
				//
				const auto actionProperty = keyframeProperties.find( "action" );
				if( actionProperty != keyframeProperties.end() )
				{
					keyframe.action( propertyText( actionProperty ));
				}
				
				// Set the keyframe on the MovieClip.
				//
				setKeyframe( iFrame, std::move( keyframe ));
				
				// If the element specifies a label, attach the keyframe to that label.
				//
				if( iFrame != INVALID_FRAME_INDEX && !label.empty() )
				{
					trim( label );
					
					if( !label.empty() )
					{
						setLabelForIndex( label, iFrame );
					}
					else
					{
						dev_warning( "While loading, " << toString() << " found blank keyframe label." );
					}
				}
				
				lastFrameIndex = iFrame;
				
				// Do we like a tween with this keyframe?
				//
				const auto tweenProperty = keyframeProperties.find( "tween" );
				if( iFrame != INVALID_FRAME_INDEX && tweenProperty != keyframeProperties.end() )
				{
					// Yes. What kind?
					
					const auto& tweenObject = tweenProperty->second.first->get< Manifest::Object >();
					
					auto tweenType = tweenObject.className;
					if( tweenType.empty() )
					{
						tweenType = "QuadEaseInOut";			// The default tween type.
					}
					
					const Tweener< DisplayObjectState >* tweener = DisplayObjectState::getTweenerByName( tweenType );
					
					if( !tweener )
					{
						dev_warning( toString() << ": Unrecognized tween type " << tweenType << " at keyframe " << iFrame );
						tweener = &( DisplayObjectState::tweenerQuadEaseInOut );
					}
					
					ASSERT( tweener );
					createTween( iFrame, tweener );
				}
				
			}
			else
			{
				dev_warning( "While loading, " << toString() << " did not find a valid index for a frame '" << ( label.empty() ? "(unlabeled)" : label ) << "'. Ignored." );
			}
		}
	}

	void MovieClip::load( const Manifest::Map& properties )
	{
		m_hasFixupCompleted = false;
		
		Super::load( properties );
		
		if( !isInert() )
		{
			pushCurrentLoadingObject( this );
			
			// Get the passthrough element, which hosts shortcut keyframe goodies.
			//
			auto passthroughElement = properties.find( "passthrough" );
			if( passthroughElement != properties.end() )
			{
				std::shared_ptr< Manifest::Value > passthroughValue = passthroughElement->second.first;
				ASSERT( passthroughValue );
				
				const auto& passthroughArray = passthroughValue->get< Manifest::Array >();
				
				loadKeyframes( passthroughArray );
			}
		
			popCurrentLoadingObject();
		}
	}
	
	void MovieClip::serialize( class ObjectStreamFormatter& formatter, bool doWriteEvenIfDefault ) const
	{
		if( /* DISABLES CODE */ (true) || m_vecFrames.empty() )		// TODO disabled for Megadventure.
		{
			Super::serialize( formatter, doWriteEvenIfDefault );
		}
		else
		{
			
			// Adapted from Object::serialize()
			
			const ClassInfo& myClassInfo = classInfo();
			
			formatter.beginObject( this );
			
			const ClassInfo::PropertyIterator endIter = myClassInfo.getPropertyIteratorEnd();
			for( ClassInfo::PropertyIterator iter = myClassInfo.getPropertyIteratorBegin();
				iter != endIter;
				++iter )
			{
				const PropertyAbstract& prop = *iter;
				
				// Only write out this property if it is different from the default for this class.
				//
				if( !prop.isTransient() &&																		// We don't save transient properties.
				   ( !shallow() || !prop.deep() ) &&															// We don't save pointers (and other "deep" properties) if we're shallow.
				   ( doWriteEvenIfDefault || !prop.doesObjectHaveDefaultValue( this )))							// We don't save properties that still have the default value, unless it's forced.
				{
					prop.saveToFormatter( formatter, this );
				}
			}
			
			
			// Write passthrough element.
			//
			formatter.writeIndents();
			formatter.getStringifier() << "<passthrough>\n";
			formatter.indent( 1 );

			for( size_t i = 0; i < m_vecFrames.size(); ++i )
			{
				const auto& keyframe = m_vecFrames[ i ];
				
				if( keyframe )
				{
					writeKeyframe( formatter, *keyframe, i );
				}
			}

			formatter.indent( -1 );
			formatter.writeIndents();
			formatter.getStringifier() << "</passthrough>\n";
			
			formatter.endObject( this );			
		}
	}
	
	void MovieClip::writeKeyframe( class ObjectStreamFormatter& formatter, const Keyframe& keyframe, size_t iFrame ) const
	{
		auto& out = formatter.getStringifier();
		
		formatter.writeIndents();
		out << "<keyframe t='" << iFrame << "'";
		
		const auto& label = getLabelForIndex( iFrame );
		if( !label.empty() )
		{
			out << " name='" << label << "'";
		}
		
		out << ">\n";
		
		formatter.indent( 1 );
		
		keyframe.serialize( *this, formatter );
		
		// Write tweens.
		//
		auto iter = m_mapKeyframeTweens.find( iFrame );
		if( iter != m_mapKeyframeTweens.end() )
		{
			auto tweener = iter->second;
			
			if( tweener )
			{
				formatter.writeIndents();
				
				out << "<tween type='" << DisplayObjectState::getTweenerName( *tweener ) << "' />\n";
			}
		}
		
		formatter.indent( -1 );
		
		formatter.writeIndents();
		out << "</keyframe>\n";
	}
	
	void MovieClip::postLoad()
	{
		Super::postLoad();
		
		fixupKeyframeNames();		
	}
	
	void MovieClip::onAddedToStage()
	{
		Super::onAddedToStage();
		
		// Setup texture animation keyframes as requested by simpleTextureAnimation.
		//
		if( m_simpleTextureAnimation.nFrames > 0 )
		{
			setupSimpleTextureAnimationFrames( m_simpleTextureAnimation );
		}

		// Actually apply the initial keyframe.
		//
		m_lastAppliedFrame = -1;
		
		setCurrentFrame( currentFrame() );
	}

	void MovieClip::setupSimpleTextureAnimationFrames( const SimpleTextureAnimation& simpleTextureAnimation )
	{
		// Calculate the frame rate in terms of skipped MovieClip frames per Stage update.
		//		
		const int skipsPerKeyframe = static_cast< int >( stage().frameRate() / simpleTextureAnimation.framesPerSecond );
		
		for( int iFrame = 0; iFrame < simpleTextureAnimation.nFrames; ++iFrame )
		{
			const int frameTime = iFrame * skipsPerKeyframe;
			
			// Do we already have a keyframe here?
			//
			if( !hasKeyframe( frameTime ))
			{
				createBlankKeyframe( frameTime );
				ASSERT( hasKeyframe( frameTime ));
			}
			
			Keyframe& keyframe = getKeyframe( frameTime );
			
			rect textureSlice( iFrame * simpleTextureAnimation.sliceWidth, 0, ( iFrame + 1 ) * simpleTextureAnimation.sliceWidth, 1.0f );
			
			keyframe.texture( texture(), textureSlice );
		}
		
		// Insert one additional empty frame to lengthen the last frame, if necessary.
		//
		const int theoreticalLoopedFrameTime = simpleTextureAnimation.nFrames * skipsPerKeyframe;
		if( !hasKeyframe( theoreticalLoopedFrameTime - 1 ))
		{
			createBlankKeyframe( theoreticalLoopedFrameTime - 1 );
		}
	}

	void MovieClip::onTweenComplete()
	{
		TIMER_AUTO( MovieClip::OnTweenComplete )

		// Dispatch an event to listeners interested in knowing when tweens complete.
		//
		Event tweenFinishedEvent( TWEEN_FINISHED, this );			
		dispatchEvent( &tweenFinishedEvent );
	}

	void MovieClip::onPlaybackReachedEnd()
	{
		Event event( PLAYBACK_REACHED_END, this );
		dispatchEvent( &event );
	}
	
	void MovieClip::deleteAnyExistingKeyframe( size_t iFrame )
	{
		if( iFrame < m_vecFrames.size() && m_vecFrames[ iFrame ] != 0 )
		{
			m_vecFrames[ iFrame ] = nullptr;
		}
	}

	void MovieClip::ensureTimelineLongEnoughFor( size_t iFrame )
	{
		if( iFrame >= m_vecFrames.size() )
		{
			m_vecFrames.resize( iFrame + 1 );
		}
	}

	void MovieClip::compactTimeline()
	{
		// Remove null values from the back of the timeline vector.
		//
		while( !m_vecFrames.empty() && m_vecFrames.back() == nullptr )
		{
			m_vecFrames.pop_back();
		}
	}

	void MovieClip::setCurrentFrame( size_t iFrame )
	{
		TIMER_AUTO( MovieClip::setCurrentFrame )

		m_lastAppliedFrame = m_currentFrame = iFrame;
		
		if( hasKeyframe( m_currentFrame ))
		{
			Keyframe& keyframe = getKeyframe( m_currentFrame );
			keyframe.applyStateTo( this );
			
			// If we're playing and this keyframe corresponds to a tween, apply it.
			//
			if( m_isPlaying )
			{			
				const auto iter = m_mapKeyframeTweens.find( iFrame );
				
				if( iter != m_mapKeyframeTweens.end() )
				{
					size_t iNextKeyframe = getNextKeyframeAfter( m_currentFrame );
					
					if( iNextKeyframe != INVALID_FRAME_INDEX )
					{
						ASSERT( iNextKeyframe > m_currentFrame );
						
						TimeType nSecondsBetweenKeyframes = ( iNextKeyframe - m_currentFrame ) * stage().secondsPerFrame();
						
						tweenToKeyframe( iNextKeyframe, nSecondsBetweenKeyframes, iter->second );
					}
				}
			}
		}
	}

	size_t MovieClip::getNextKeyframeAfter( size_t iFrame ) const
	{
		while( ++iFrame < m_vecFrames.size() )
		{
			if( m_vecFrames[ iFrame ] != 0 )
			{
				return iFrame;
			}
		}
		
		return INVALID_FRAME_INDEX;
	}

	void MovieClip::fixupKeyframeNames()
	{
		for( const auto& keyframe : m_vecFrames )
		{
			if( keyframe )
			{
				keyframe->fixupSymbolicNames( this );
			}
		}
		
		m_hasFixupCompleted = true;
	}

}

