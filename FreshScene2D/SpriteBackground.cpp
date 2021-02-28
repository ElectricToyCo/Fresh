/*
 *  SpriteBackground.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/24/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#include "SpriteBackground.h"
#include "Objects.h"
#include "Renderer.h"
#include "Stage.h"
#include "Texture.h"

namespace 
{
	using namespace fr;
	
	VertexBuffer::ptr g_spriteBackgroundVertexBuffer;

	vec2 parentToLocalScale_TimeInterpolated( const DisplayObject& localObject, const vec2& value, TimeType relativeFrameTime )
	{
		const auto& tweenedState = localObject.getTweenedState( relativeFrameTime );
		return value / tweenedState.scale();
	}
	
	angle parentToLocal_TimeInterpolated( const DisplayObject& localObject, angle value, TimeType relativeFrameTime )
	{
		const auto& tweenedState = localObject.getTweenedState( relativeFrameTime );
		return value - tweenedState.rotation();
	}
	
	vec2 parentToLocal_TimeInterpolated( const DisplayObject& localObject, const vec2& value, TimeType relativeFrameTime )
	{
		const auto& tweenedState = localObject.getTweenedState( relativeFrameTime );
		
		const DisplayObjectContainer* parent = localObject.parent().get();
		
		vec2 attachPoint;
		if( parent )
		{
			attachPoint = localObject.parentAttachOffset();
		}
		
		vec2 localLocation = value - ( tweenedState.position() + attachPoint );
		localLocation.rotate( -tweenedState.rotation() );
		localLocation /= tweenedState.scale();
		return localLocation + tweenedState.pivot();
	}
	
	template< typename T >
	T globalToLocal_TimeInterpolated( const DisplayObject& localObject, const T& value, TimeType relativeFrameTime )
	{
		const DisplayObjectContainer* parent = localObject.parent().get();
		
		if( !parent )
		{
			return value;
		}
		else
		{
			return parentToLocal_TimeInterpolated( localObject, globalToLocal_TimeInterpolated( *parent, value, relativeFrameTime ), relativeFrameTime );
		}
	}

	template< typename T >
	T globalToLocalScale_TimeInterpolated( const DisplayObject& localObject, const T& value, TimeType relativeFrameTime )
	{
		const DisplayObjectContainer* parent = localObject.parent().get();
		
		if( !parent )
		{
			return value;
		}
		else
		{
			return parentToLocalScale_TimeInterpolated( localObject, globalToLocalScale_TimeInterpolated( *parent, value, relativeFrameTime ), relativeFrameTime );
		}
	}
}

namespace fr
{
	FRESH_DEFINE_CLASS_UNPLACEABLE( SpriteBackground )
	
	DEFINE_VAR( SpriteBackground, DisplayObject::wptr, m_trackingObject );
	// The movement of this object determines the "slide" of the texture over the background sprite.
	// Typically you would assign this to a "world" display object that also tracks with a camera.
	
	DEFINE_VAR( SpriteBackground, vec2, m_velocity );
	DEFINE_VAR( SpriteBackground, vec2, m_trackingTranslationScale );
	DEFINE_VAR( SpriteBackground, vec2, m_stageDimensions );
	DEFINE_VAR( SpriteBackground, bool, m_fillStageX );
	DEFINE_VAR( SpriteBackground, bool, m_fillStageY );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( SpriteBackground )
	
	void SpriteBackground::texture( Texture::ptr texture_ )
	{
		Sprite::texture( texture_ );
	}

	void SpriteBackground::update()
	{
		Super::update();
		
		m_position += m_velocity * (real) stage().secondsPerFrame();
	}
	
	void SpriteBackground::draw( TimeType relativeFrameTime, RenderInjector* injector )
	{
		// This function has cost me countless hours and many headaches.
		// Since I now understand what the heck I have written here better than I usually do,
		// I will take the opportunity to document this function thoroughly.
		//
		// The overall approach is to render a single quad that covers the entire stage.
		// (The stage is usually the same as the screen.)
		// The background appears to move not because any triangles are moving but because 
		// the UVs of those triangles move.
		//
		// UV translation is based on the world-space camera position gleaned from m_trackingObject.
		// Scaling the UVs is complex because of both essential change-of-basis issues
		// as well as arbitrary scaling to allow for a texture of whatever size to be
		// larger than (or potentially smaller than) the screen.
		//
		// Here's how it all works.
		//
		
		if( !( !injector || !injector->draw( relativeFrameTime, *this )))
		{
			return;
		}
		
		if( !Super::texture() )
		{
			return;
		}		
		
		Renderer& renderer = Renderer::instance();
		
		// We use a static vertex buffer with two triangles formed as a strip.
		// This quad is 2x2 in dimensions, centered on the origin.
		// Its UV coordinates are also 2x2, so if you render this quad untransformed
		// you'll get a repeating pattern of 4 textures.
		//
		if( !g_spriteBackgroundVertexBuffer )
		{
			g_spriteBackgroundVertexBuffer = createObject< VertexBuffer >( getTransientPackage() );
			
			std::vector< Vector2f > points;
			
			points.reserve( 8 );
			
			points.push_back( Vector2f( -1.0f,  -1.0f ));
			points.push_back( Vector2f( -1.0f,  -1.0f ));
			points.push_back( Vector2f(  1.0f,  -1.0f ));
			points.push_back( Vector2f(  1.0f,  -1.0f ));
			points.push_back( Vector2f( -1.0f,   1.0f ));
			points.push_back( Vector2f( -1.0f,   1.0f ));
			points.push_back( Vector2f(  1.0f,   1.0f ));
			points.push_back( Vector2f(  1.0f,   1.0f ));
			
			g_spriteBackgroundVertexBuffer->associateWithVertexStructure( Stage::getPos2TexCoord2VertexStructure() );
			g_spriteBackgroundVertexBuffer->loadVertices( points.begin(), points.end() );
		}
		ASSERT( g_spriteBackgroundVertexBuffer );
		
		// Get ready with the background texture.
		//
		renderer.applyTexture( Super::texture() );
		renderer.setBlendMode( Renderer::getBlendModeForTextureAlphaUsage( Super::texture()->alphaUsage() ));
		
		const auto& dimensions = baseDimensions();
		const vec2 textureRatioScalar = dimensions / dimensions.majorAxisValue();
		
		// We set up the model view matrix to be totally unlike any other sprite.
		// First we discard the current matrix by reversing what our ancestors have done.
		// After this we'll scale the quad positions to fill the stage.
		//
		renderer.pushMatrix( Renderer::MAT_ModelView );
		
		auto normalizingTranslation = globalToLocal_TimeInterpolated( *parent(), vec2::ZERO, relativeFrameTime );
		auto normalizingRotation = globalToLocal_TimeInterpolated( *parent(), angle( 0 ), relativeFrameTime );
		auto normalizingScale = globalToLocalScale_TimeInterpolated( *parent(), vec2( 1.0f ), relativeFrameTime );
		
		// Now we figure out the size of the stage (aka screen aka view).
		// We divide it by 2 because both the quad positions and UVs are "overscaled"
		// If there were 0.5f's in the vertex data above, we wouldn't scale here.
		//
		vec2 stagedimensions = m_stageDimensions;
		if( stagedimensions.isZero() )
		{
			stagedimensions = stage().stageDimensions();
		}
		
		vec2 quadScale( stagedimensions * 0.5f );

		auto factor = [&]( int term )
		{
			normalizingTranslation[ term ] = normalizingTranslation[ term ] * ( 1.0f - m_trackingTranslationScale[ term ] ) + position()[ term ] * dimensions[ term ] * -0.5f;
			normalizingScale[ term ] = 1.0f;
			quadScale[ term ] = dimensions[ term ] * scale()[ term ];
		};
		
		if( !m_fillStageX )
		{
			factor( 0 );
		}
		if( !m_fillStageY )
		{
			factor( 1 );
		}
		
		renderer.translate( normalizingTranslation );
		renderer.rotate( normalizingRotation );
		renderer.scale( normalizingScale );
		
		renderer.scale( quadScale );

		
		// If we have a tracking object, using a texture transform to make the UVs move
		// as the tracking object does (inverted actually).
		//
		if( m_trackingObject )
		{			
			//
			// Prepare a texture transform.
			// This effort makes up the bulk of the function.
			//
			
			// First clear out the texture transform.
			//
			renderer.pushMatrix( Renderer::MAT_Texture );
			renderer.setMatrixToIdentity( Renderer::MAT_Texture );
			
			// In OpenGL, the last transform you apply is actually applied first and vice versa.
			// So read these two transforms (translate and scale) in backward order.
			// Although we *call* the translation first, then the scale, in effect we are 
			// scaling first, then translating.
			// The practical upshot of this is that the translation is affected by the scale.
			// So you can't really understand this translation code until you browse down
			// and read the scaling code.
			
			DisplayObjectState trackingObjectState = m_trackingObject->getTweenedState( relativeFrameTime );
			
			// TRANSLATE the UVs. This actually happens second, conceptually speaking. ******************************
			//
			// The UV translation comes essentially from the camera position. This is a world space position,
			// so transforming it into UV space is the big challenge here.
			// Note also that the translating we apply here is affected by the scale we've already applied (below).
			// This makes everything jolly confusing.
			//
			// Get the camera translation.
			//
			const vec2 viewTranslation = trackingObjectState.position() * m_trackingTranslationScale;
			
			// Also ask the camera how "zoomed in" we are. The viewScale describes the ratio between
			// world units and stage units.
			//
			const vec2 viewScale = trackingObjectState.scale();
			
			// To figure out the actual translation that we will apply to the stretched-quad UVs, we begin
			// by converting from the world space translation to a stage space translation.
			//
			vec2 adjustedTranslation( viewTranslation / viewScale );
			
			// Now further adjust the UV translation by converting it from stage space to UV space.
			// This is a little odd, because we have already (below) setup a scale that will
			// "stretch" UV coordinates onto the rectangular stage.
			// In effect, whatever translation we come up with here will be multiplied
			// by "adjustedTextureScale" below.
			// So where you might expect to see just stageDimensions, instead we use just the X coordinate.
			// That's because the X (U) coordinate will be automatically scaled back into proportion
			// with the Y (V) coordinate in the overall scale transform below.
			// 
			adjustedTranslation /= quadScale.x;
			
			// Since the whole texture is "blown up" by scale(), whenever we translate
			// the UVs we need to blow up the translation by this scale as well.
			//
			adjustedTranslation *= scale();
			
			adjustedTranslation /= textureRatioScalar;

			// By default, the UV origin corresponds to the world space origin.
			// But often we want a background image to be offset relative to the origin.
			// That's what this addition does. Note that it is effectively scaled by
			// adjustedTextureScale below.
			//
			adjustedTranslation += position();
			
			// Invert the translation so that we move against the camera rather than with it.
			//
			adjustedTranslation = -adjustedTranslation;

			
			// SCALE the UVs. This actually happens first, conceptually speaking. ******************************
			//
			// The quad's UV coordinates sit in UV space (obviously).
			// Before we do any other transformation, we need to setup two ideas.
			// First, we need to scale the texture based on an arbitrary "oversizing" scale as well as by the "zoom" scale.
			// Second, we need to adjust the UV coordinates to account for the fact that although the UVs
			// started out based on a square quad, the stage dimensions scale our UVs into a rectangular proportion.
			// That is, our modelview coordinates have gotten stretched in one dimension.
			// Our texture coordinates need to get stretched the same way.
			//
			// First, calculate the scale based on an arbitrary value. This just allows us to "blow up" textures to be
			// bigger than the screen.
			//
			vec2 adjustedTextureScale = scale();
			
			// Now adjust this overall scale by our camera "zoom" factor.
			//
			adjustedTextureScale /= viewScale;
			
			// Now scale the "V" axis so that it has the same ratio to the U axis as our stage's Y and X axes.
			//
			adjustedTextureScale.y = adjustedTextureScale.x * ( quadScale.y / quadScale.x );
			
			adjustedTextureScale /= textureRatioScalar;
			
			// Apply the translation and scale.
			//
			auto adjust = [&]( int term )
			{
				adjustedTranslation[ term ] = 0.5f;	// To shift UVs from [-1,1] to [0,2] range.
				adjustedTextureScale[ term ] = 0.5f;
			};
			if( !m_fillStageX )
			{
				adjust( 0 );
			}
			if( !m_fillStageY )
			{
				adjust( 1 );
			}
			
			renderer.translate( adjustedTranslation, Renderer::MAT_Texture );
			renderer.scale( adjustedTextureScale, Renderer::MAT_Texture );
		}
		
		// Now having set up all the transforms, render a simple quad all over the screen.
		//
		renderer.updateUniformsForCurrentShaderProgram( this );
		Renderer::instance().drawGeometry( Renderer::PrimitiveType::TriangleStrip, g_spriteBackgroundVertexBuffer, 4 );	// Quad	
		
		if( m_trackingObject )
		{
			// Restore matrix settings.
			//	
			renderer.popMatrix( Renderer::MAT_Texture );
		}
		
		renderer.popMatrix( Renderer::MAT_ModelView );
	}

	void SpriteBackground::setupTransforms( TimeType relativeFrameTime )
	{
		Renderer::instance().pushMatrix();
		m_didPushMatrixDuringPreDraw = true;
	}

}
