//
//  main.cpp
//  TestShaders
//
//  Created by Jeff Wofford on 12/2/10.
//  Copyright 2010 jeffwofford.com. All rights reserved.
//

#include "Application.h"
#include "Package.h"
#include "FreshTime.h"
#include "Asset.h"
#include "Renderer.h"
#include "Texture.h"
#include "Renderer.h"
#include "ShaderProgram.h"
#include "FreshGraphicsUtil.h"

using namespace fr;


namespace
{
	const Vector3i GRID_SIZE( 50, 50, 50 );
	const vec3 GRID_SIZE_FLOAT( GRID_SIZE.x, GRID_SIZE.y, GRID_SIZE.z );
	
	const real FOV = 90.0f;
	
	const real TRANSLATION_DRAG = 0.1f;
	const real ROTATION_DRAG = 0.1f;
	
	const real TAP_TRANSLATION_BURST_SPEED = 2.0f;

}

class TestApp : public fr::Application
{
public:
	
	TestApp()
	:	Application( "appConfig.xml" )
	,	m_translation( GRID_SIZE_FLOAT * 0.5f )
	,	m_lastTranslation( m_translation )
	,	m_rotation( vec2::ZERO )
	,	m_lastRotation( vec2::ZERO )
	,	m_nTouchesDown( 0 )
	{}
	
	virtual void onPreFirstUpdate()
	{
		Renderer& renderer = Renderer::instance();
		
		renderer.setClearColor( Color( 0.3f, 0.3f, 0.4f ));
		
		m_shaderPlainVanilla3d = renderer.createOrGetShaderProgram( "PlainVanilla3d" );
		if( !m_shaderPlainVanilla3d->isLinked() )
		{
			dev_trace( m_shaderPlainVanilla3d->getErrorString() );
		}
		else
		{
			// TODO move into assets.xml
			m_shaderPlainVanilla3d->bindUniformToObjectMethod< mat4 >( "projectionMatrix", renderer.objectId(), "getProjectionMatrix" );
			m_shaderPlainVanilla3d->bindUniformToObjectMethod< mat4 >( "modelViewMatrix", renderer.objectId(), "getModelViewMatrix" );
			m_shaderPlainVanilla3d->bindUniformToObjectMethod< mat4 >( "textureMatrix", renderer.objectId(), "getTextureMatrix" );
			m_shaderPlainVanilla3d->bindUniformToObjectMethod< Color >( "material_color", renderer.objectId(), "getColorMultiply" );
			
			m_shaderPlainVanilla3d->associateSamplerWithTextureUnit( "diffuseTexture", 0 );
		}
		
		m_vertexStructurePos3 = renderer.createOrGetVertexStructure( "VS_Pos3TexCoord2Col4" );
		
		ASSERT( *m_vertexStructurePos3 == *( m_shaderPlainVanilla3d->getExpectedVertexStructure() ));
		
		m_defaultTexture = renderer.createTexture( "default" );

		// Create a simple white texture for use by PlainVanilla3d.
		//
		m_whiteTexture = createObject< Texture >( "White" );
		const unsigned int color( Color( Color::White ).getABGR() );
		m_whiteTexture->loadFromPixelData( reinterpret_cast< const unsigned char* >( &color ), Vector2ui( 1, 1 ));
	}

	void applyImpulse( const vec3& impulse )
	{
		m_translation += impulse;
	}

	void applyImpulseWorldSpace( const vec3& impulse )
	{
		mat4 matrix;
		matrix.fromEulerAnglesXYZ( degreesToRadians( -m_rotation.x ), degreesToRadians( -m_rotation.y ), 0 );		
		applyImpulse( matrix * impulse );
	}
	
	virtual void update()
	{
		vec3 savedTranslation( m_translation );
		vec2 savedRotation( m_rotation );
		
		if( m_nTouchesDown == 0 )
		{
			// No touches down. Update physics.
			//
			m_translation += ( m_translation - m_lastTranslation ) * ( 1.0f - TRANSLATION_DRAG );
			m_rotation += ( m_rotation - m_lastRotation ) * ( 1.0f - ROTATION_DRAG		);
		}
		
		Renderer& renderer = Renderer::instance();
		renderer.clear();

		renderer.setPerspectiveProjection( FOV * 0.5f, getWindowAspectRatio(), 1.0f, 10000.0f );

		renderer.setMatrixToIdentity();
		renderer.rotate( -m_rotation.x, vec3( 1, 0, 0 ));
		renderer.rotate( -m_rotation.y, vec3( 0, 1, 0 ));
		renderer.translate( -m_translation );

		renderer.useShaderProgram( m_shaderPlainVanilla3d );
		
		renderer.applyTexture( m_whiteTexture );
		
		drawOrigin( 100.0f );
		drawWorldGrid( GRID_SIZE );
		
		renderer.applyTexture( m_defaultTexture );
		renderer.scale( GRID_SIZE.x, GRID_SIZE.y );
		drawQuad2D();
		
		m_lastTranslation = savedTranslation;
		m_lastRotation = savedRotation;
	}
	
	virtual void onTouchesBegin( TouchIter begin, TouchIter end ) 
	{
		Application::onTouchesBegin( begin, end );
		m_nTouchesDown += end - begin;
	}
	
	virtual void onTouchesMove( TouchIter begin, TouchIter end ) 
	{
		const float TRANSLATION_PER_TOUCH_UNIT = 0.04f;
		
		Application::onTouchesMove( begin, end );
		
		vec2 touchPositions[ 2 ];
		vec2 touchLastPositions[ 2 ];
		vec2 touchAverage( vec2::ZERO ), touchAverageLast( vec2::ZERO );
		int i = 0;
		for( ; i < 2 && begin != end; ++begin, ++i )
		{		
			const float DEGREES_PER_PIXEL = FOV / getWindowDimensions().y * 0.5f;
			
			vec2 delta = begin->position - begin->lastPosition;
			
			if( begin->nTouches == 1 )
			{
				// Single touch. Immediately rotate pitch and yaw.
				//
				m_rotation += vec2( -delta.y, delta.x ) * DEGREES_PER_PIXEL;
			}
			else	// Record data for multiple touch response.
			{
				touchPositions[ i ] = begin->position;
				touchLastPositions[ i ] = begin->lastPosition;
				touchAverage += begin->position;
				touchAverageLast += begin->lastPosition;
			}
		}
		
		
		if( i == 2 )
		{
			// Handle translation.
			//
			touchAverage *= 0.5f;
			touchAverageLast *= 0.5f;
			
			vec2 averageDelta = touchAverage - touchAverageLast;
			
			// Average delta defines how to move in the XY plane.
			//
			vec3 impulse( vec3::ZERO );
			
			impulse.x += averageDelta.x * TRANSLATION_PER_TOUCH_UNIT;
			impulse.y += averageDelta.y * TRANSLATION_PER_TOUCH_UNIT;
			
			// Change in distance defines how to move in the Z plane.
			//
			float dist = ( touchPositions[ 0 ] - touchPositions[ 1 ] ).getMagnitude();
			float distLast = ( touchLastPositions[ 0 ] - touchLastPositions[ 1 ] ).getMagnitude();
			
			float distDelta = dist - distLast;
			
			impulse.z += distDelta * TRANSLATION_PER_TOUCH_UNIT;			
			
			applyImpulseWorldSpace( -impulse );
		}		
	}
	
	virtual void onTouchesEnd( TouchIter begin, TouchIter end ) 
	{
		m_nTouchesDown -= end - begin;
		assert( m_nTouchesDown >= 0 );
		
		if( begin->nTaps == 2 )		// Double tap to fly in.
		{
			vec3 impulse = vec3( 0, 0, -TAP_TRANSLATION_BURST_SPEED );
			if( begin->iTouch == 0 && begin->nTouches == 2 )
			{
				impulse = -impulse;
			}
			applyImpulseWorldSpace( impulse );
		}
		
		Application::onTouchesEnd( begin, end );
	}
	
private:
	
	vec3 m_translation;
	vec3 m_lastTranslation;
	vec2 m_rotation;	// Pitch and yaw in degrees.
	vec2 m_lastRotation;	// Pitch and yaw in degrees.
	int m_nTouchesDown;
	
	ShaderProgram::ptr m_shaderPlainVanilla3d;
	
	Texture::ptr m_whiteTexture;
	Texture::ptr m_defaultTexture;
	
	VertexStructure::ptr m_vertexStructurePos3;	
};

int main( int argc, char* argv[] )
{
	TestApp app;

	int retVal = app.runMainLoop( argc, argv );
	
    return retVal;
}
