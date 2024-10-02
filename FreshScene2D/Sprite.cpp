/*
 *  Sprite.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/18/08.
 *  Copyright 2008 jeffwofford.com. All rights reserved.
 *
 */

#include "Sprite.h"
#include "Graphics.h"
#include "Renderer.h"
#include "Stage.h"
#include "Objects.h"
#include "Texture.h"
#include "SimpleMesh.h"
#include "Application.h"

#ifdef FRESH_DEBUG_HITTESTS
#	define trace_hittest( expr ) trace( expr )
#else
#	define trace_hittest( expr )
#endif

namespace fr
{
	
	SmartPtr< SimpleMesh > Sprite::s_standardMesh;
	
	FRESH_DEFINE_CLASS( Sprite )

	Sprite::Sprite( CreateInertObject c )
	:	Super( c )
	{}
	
	Sprite::Sprite( const ClassInfo& assignedClassInfo, NameRef objectName /*= DEFAULT_OBJECT_NAME*/ )
	:	Super( assignedClassInfo, objectName )
	{
		if( !s_standardMesh )
		{
			s_standardMesh = getObject< SimpleMesh >( "SM_ZeroCenteredDiameter1" );
			ASSERT( s_standardMesh );
		}

		mesh( s_standardMesh );
		
		doctorClass< Sprite >( [&]( ClassInfo& classInfo, Sprite& defaultObject )
									{
										DOCTOR_PROPERTY( mesh )
									} );
	}
	
	Graphics& Sprite::graphics() const
	{
		if( !m_graphics )
		{
			m_graphics = createObject< Graphics >();
		}
		PROMISES( m_graphics );
		return *m_graphics;
	}

	void Sprite::draw( TimeType relativeFrameTime, RenderInjector* injector )
	{
		if( !injector || !injector->draw( relativeFrameTime, *this ))
		{
			drawSprite( relativeFrameTime );
		}
		DisplayObjectContainer::draw( relativeFrameTime, injector );						// Bypass DisplayObjectWithMesh's implementation.
	}

	void Sprite::drawSprite( TimeType relativeFrameTime )
	{
		if( m_graphics )
		{
			m_graphics->draw();
		}
		
		if( effectiveTexture() )
		{
			Renderer& renderer = Renderer::instance();
			renderer.pushMatrix();
			
			// Scale the image in terms of the original dimensions before they were forced to be power-of-2.
			//
			renderer.scale( baseDimensions() );	
			
			drawMesh( relativeFrameTime );
			
			renderer.popMatrix();
		}
	}

	rect Sprite::localBounds() const
	{
		rect bounds = DisplayObjectContainer::localBounds();
		
		const vec2 halfDimensions = baseDimensions() * 0.5f;
		const rect meshTextureBounds( -halfDimensions, halfDimensions );
		bounds.growToEncompass( meshTextureBounds );
		
		if( m_graphics )
		{
			const rect graphicsBounds = m_graphics->localBounds();
			bounds.growToEncompass( graphicsBounds );
		}
		
		return bounds;
	}
	
	Renderer::BlendMode Sprite::calculatedBlendMode() const
	{
		Renderer::BlendMode blendMode = Super::calculatedBlendMode();
		
		// Impose alpha blending if the graphics requests it.
		//
		if( blendMode == Renderer::BlendMode::None && m_graphics )
		{
			return m_graphics->calculatedBlendMode();
		}
		return blendMode;
	}

	vec2 Sprite::baseDimensions() const
	{
        if( effectiveTexture() )
		{
			const Vector2ui texDimensions = effectiveTexture()->getOriginalDimensions();
			
			rect uvSpaceTexWindow = textureWindow();
			uvSpaceTexWindow.left( uvSpaceTexWindow.left() * texDimensions.x );
			uvSpaceTexWindow.right( uvSpaceTexWindow.right() * texDimensions.x );
			uvSpaceTexWindow.top( uvSpaceTexWindow.top() * texDimensions.y );
			uvSpaceTexWindow.bottom( uvSpaceTexWindow.bottom() * texDimensions.y );
			
			return uvSpaceTexWindow.dimensions();
		}
		else
		{
			return vec2( 0, 0 );
		}
	}

	vec2 Sprite::getScaledDimensions() const
	{
		return fr::abs( baseDimensions() * m_scale );
	}

	bool Sprite::hitTestPoint( const vec2& localLocation, HitTestFlags flags ) const
	{
		trace_hittest( toString() << ".Sprite::" << FRESH_CURRENT_FUNCTION << "(" << localLocation << "," << flags << "):" );
		
		if( rejectsOnTheBasisOfHitFlags( flags ))
		{
			trace_hittest( "Sprite rejecting on the basis of flags" );
			return false;
		}

		if( !hitTestMask( localLocation, flags ))
		{
			return false;
		}
		
		if( !baseDimensions().isZero() )
		{
			// Scale the local location down from texture space into mesh space.
			//
			const vec2 meshLocation = localLocation / baseDimensions();
			
			if( hitTestPointAgainstMesh( meshLocation ))
			{
				trace_hittest( "Sprite touched texture where tex dims are " << originalTextureDimensions << " and transformed point is therefore " << meshLocation );
				return true;
			}
		}
		
		if( DisplayObjectContainer::hitTestPoint( localLocation, flags ))		// Bypass DisplayObjectWithMesh check in this case, because with no texture our mesh is irrelevant
		{
			trace_hittest( "Textureless Sprite touched by virtual of DisplayObjectContainer::hitTestPoint()" );
			return true;
		}
		
		// Does our graphics object touch the location?
		//
		if( m_graphics && m_graphics->hitTestPoint( localLocation ))
		{
			trace_hittest( "Sprite touched by virtue of graphics." );
			return true;
		}
		
		trace_hittest( toString() << " Sprite found no touch." );
		return false;
	}
}
