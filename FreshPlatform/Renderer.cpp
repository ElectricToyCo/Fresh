/*
 *  Renderer.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 5/28/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#include "Renderer.h"

#include "Objects.h"
#include "Texture.h"
#include "FreshTime.h"
#include "FreshFile.h"
#include "ShaderProgram.h"
#include "ShaderUniformUpdaterConcrete.h"
#include "CommandProcessor.h"
#include "Application.h"					// For access to AssetPackage

#include "FreshOpenGL.h"
#include "glESHelpers.h"

namespace
{	

#if DEV_MODE && ANDROID
#	define REPORT_GL_CAPS 1
#endif
	
#if REPORT_GL_CAPS
	inline void reportGLValue( GLenum var, const char* name )
	{
		GLint value = -1;
		glGetIntegerv( var, &value );
		
		trace( name << "=" << value );
	}
#	define REPORT_GL_VALUE( var ) reportGLValue( var, #var )
#endif
}

namespace fr
{
	
	FRESH_DEFINE_CLASS( Renderer )
	
	DEFINE_METHOD( Renderer, toggleWireframeMode );

	Renderer::Renderer( CreateInertObject c )
	:	Super( c )
	,	ObjectSingleton< Renderer >( nullptr )
	{}
	
	Renderer::Renderer( const ClassInfo& assignedClassInfo, NameRef objectName /* = DEFAULT_OBJECT_NAME */ )
	:	Object( assignedClassInfo, objectName )
	,	ObjectSingleton< Renderer >( this )
	{
		fr::initGLExtensions();
		
		// Set default colors.
		//
		m_stackColor.push_back( std::make_pair( Color::White, Color::Invisible ));
	
#if REPORT_GL_CAPS
		// Report interesting general capabilities.
		//
		REPORT_GL_VALUE( GL_MAX_VERTEX_UNIFORM_VECTORS );
		REPORT_GL_VALUE( GL_MAX_VARYING_VECTORS );
#endif
		
		// Record initial render state values.
		//
		m_currentRenderState.blendMode = m_currentRenderState.lastEnabledBlendMode = BlendMode::None;	// Default OpenGL state
		m_currentRenderState.stencilMode = StencilMode::Ignore;
		m_currentRenderState.isStencilEnabled = false;
		
		memset( m_currentRenderState.currentTextureId, 0, RenderState::MAX_TEXTURE_UNITS * sizeof( m_currentRenderState.currentTextureId[ 0 ] ));
		
		m_currentRenderState.clearColor = Color::White;
		clearColor( Color::Black );

		// Textures sent into glTexImage2D() are packed to 1-byte alignment, even when RGB.
		//
		glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );	
		
		HANDLE_GL_ERRORS();	
	}

	Renderer::~Renderer()
	{}

	void Renderer::setPerspectiveProjection( real halfFOVDegrees, real aspectRatio, real zNear, real zFar )
	{
		ASSERT( 0 < halfFOVDegrees && halfFOVDegrees <= 180 );
		ASSERT( 0 < aspectRatio );
		ASSERT( zNear != zFar );
		
		m_matrixStackProjection.loadIdentity();
		m_matrixStackProjection.perspective( degreesToRadians( halfFOVDegrees ), aspectRatio, zNear, zFar );
	}
	
	void Renderer::setOrthoProjection( real left, real right, real top, real bottom, real zNear, real zFar )
	{
		ASSERT( left != right );
		ASSERT( bottom != top );
		ASSERT( zNear != zFar );
		
		m_matrixStackProjection.loadIdentity();
		m_matrixStackProjection.ortho( left, right, bottom, top, zNear, zFar );
	}

	void Renderer::setMatrixToIdentity( MatrixIdentifier whichMatrix /* = MAT_ModelView */ )
	{
		getMatrix( whichMatrix ).loadIdentity();
	}

	void Renderer::setMatrix( const mat4& value, MatrixIdentifier whichMatrix )
	{
		getMatrix( whichMatrix ).load( value );
	}
	
	void Renderer::pushMatrix( MatrixIdentifier whichMatrix /* = MAT_ModelView */ )
	{
		getMatrix( whichMatrix ).push();
	}

	void Renderer::popMatrix( MatrixIdentifier whichMatrix /* = MAT_ModelView */ )
	{
		getMatrix( whichMatrix ).pop();
	}

	MatrixStack& Renderer::getMatrix( MatrixIdentifier whichMatrix )
	{
		switch( whichMatrix )
		{
			default:
			case MAT_ModelView:		return m_matrixStackModelView;
			case MAT_Projection:	return m_matrixStackProjection;
			case MAT_Texture:		return m_matrixStackTexture;
		}
	}

	void Renderer::enableStencil( bool enable )
	{
		if( m_currentRenderState.isStencilEnabled != enable )
		{
			if( enable )
			{
				glEnable( GL_STENCIL_TEST );
			}
			else
			{
				glDisable( GL_STENCIL_TEST );					
			}
			m_currentRenderState.isStencilEnabled = enable;
			
			HANDLE_GL_ERRORS();
		}
	}

	void Renderer::rotate( angle a, const vec3& axis, MatrixIdentifier whichMatrix /* = MAT_ModelView */ )
	{
		real angleRadians = a.toRadians< float >();
		
		getMatrix( whichMatrix ).rotateLocal( -angleRadians, axis.x, axis.y, axis.z );
	}

	void Renderer::translate( const vec3& translation, MatrixIdentifier whichMatrix /* = MAT_ModelView */ )
	{
		getMatrix( whichMatrix ).translateLocal( translation.x, translation.y, translation.z );
	}

	void Renderer::scale( const vec3& scale, MatrixIdentifier whichMatrix /* = MAT_ModelView */ )
	{
		getMatrix( whichMatrix ).scaleLocal( scale.x, scale.y, scale.z );
	}

	void Renderer::shearX( angle a, MatrixIdentifier whichMatrix /* = MAT_ModelView */ )
	{
		getMatrix( whichMatrix ).shearXLocal( -a.toRadians< float >() );
	}
	
	void Renderer::shearY( angle a, MatrixIdentifier whichMatrix /* = MAT_ModelView */ )
	{
		getMatrix( whichMatrix ).shearYLocal( -a.toRadians< float >() );
	}
	
	DEFINE_ACCESSOR( Renderer, const mat4&, getProjectionModelViewMatrix );
	DEFINE_ACCESSOR( Renderer, const mat4&, getProjectionMatrix );
	DEFINE_ACCESSOR( Renderer, const mat4&, getModelViewMatrix );
	DEFINE_ACCESSOR( Renderer, const mat4&, getTextureMatrix );
	DEFINE_ACCESSOR( Renderer, const Color&, getColorMultiply );
	DEFINE_ACCESSOR( Renderer, const Color&, getColorAdditive );
	
	const mat4& Renderer::getProjectionModelViewMatrix() const
	{
		m_matrixProjectionModelView = getModelViewMatrix() * getProjectionMatrix();
		return m_matrixProjectionModelView;
	}
	
	const mat4& Renderer::getProjectionMatrix() const
	{
		return m_matrixStackProjection.top();
	}
	
	const mat4& Renderer::getModelViewMatrix() const
	{
		return m_matrixStackModelView.top();
	}
	
	const mat4& Renderer::getTextureMatrix() const
	{
		return m_matrixStackTexture.top();
	}
	
	vec2 Renderer::screenToWorld2D( const vec2& screenCoords ) const
	{
        GLint view[4] = { m_viewportArea.left(), m_viewportArea.top(), m_viewportArea.width(), m_viewportArea.height() };
        
#if !GL_ES_VERSION_2_0
        GLdouble mx, my, mz;
        
        const Matrix4f& topf( m_matrixStackProjection.top() );
        const float* pTopf = static_cast< const float* >( topf );
        
        Matrix4d topd;
        double* pTopd = static_cast< double* >( topd );
        
        for( size_t i = 0; i < 16; ++i )
        {
            pTopd[ i ] = static_cast< double >( pTopf[ i ] );
        }
        fr::gluUnProject( screenCoords.x, screenCoords.y, 0.0, static_cast< const GLdouble* >( Matrix4d() ), topd, view, &mx, &my, &mz);
#else
        GLfloat mx, my, mz;
		fr::gluUnProject( screenCoords.x, screenCoords.y, 0.0f, static_cast< const GLfloat* >( Matrix4f() ), m_matrixStackProjection.top(), view, &mx, &my, &mz);
#endif
        
        return vec2( static_cast< real >( mx ), static_cast< real >( my ));
	}
	
	vec2 Renderer::world2DToScreen( const vec2& worldCoords ) const
	{
        GLint view[4] = { m_viewportArea.left(), m_viewportArea.top(), m_viewportArea.width(), m_viewportArea.height() };
        
#if !GL_ES_VERSION_2_0
        GLdouble mx, my, mz;

        const Matrix4f& topf( m_matrixStackProjection.top() );
        const float* pTopf = static_cast< const float* >( topf );
        
        Matrix4d topd;
        double* pTopd = static_cast< double* >( topd );
        
        for( size_t i = 0; i < 16; ++i )
        {
            pTopd[ i ] = static_cast< double >( pTopf[ i ] );
        }
        fr::gluProject( worldCoords.x, worldCoords.y, 0.0f, static_cast< const GLdouble* >( Matrix4d() ), topd, view, &mx, &my, &mz );
#else
        GLfloat mx, my, mz;
		fr::gluProject( worldCoords.x, worldCoords.y, 0.0f, static_cast< const GLfloat* >( Matrix4f() ), m_matrixStackProjection.top(), view, &mx, &my, &mz );
#endif
        
        return vec2( static_cast< real >( mx ), static_cast< real >( my ));
	}

	unsigned int Renderer::boundTextureId( size_t iSampler ) const
	{
		return m_currentRenderState.currentTextureId[ iSampler ];
	}

	void Renderer::bindTextureId( unsigned int idTexture, size_t iSampler )
	{
		REQUIRES( iSampler < getNumTextureSamplers() );
		
		if( idTexture != m_currentRenderState.currentTextureId[ iSampler ] )
		{	
			if( iSampler != 0 )
			{
				glActiveTexture( static_cast< GLenum >( GL_TEXTURE0 + iSampler ));
			}
			m_currentRenderState.currentTextureId[ iSampler ] = idTexture;
			
			glBindTexture( GL_TEXTURE_2D, idTexture );
			
			// Go back to unit 0 so that future calls with iSampler==0 (the most common case) can avoid calling glActiveTexture().
			//
			if( iSampler != 0 )
			{
				glActiveTexture( GL_TEXTURE0 );
			}
		}
		HANDLE_GL_ERRORS();
	}

	ShaderProgram::ptr Renderer::createShaderProgram()
	{
		ShaderProgram::ptr result = createObject< ShaderProgram >();
		PROMISES( result );
		return result;
	}
	
	bool Renderer::useShaderProgram( ShaderProgram::ptr shaderProgram )
	{
		if( m_currentShaderProgram != shaderProgram )
		{
			m_currentShaderProgram = shaderProgram;
			
			if( shaderProgram && shaderProgram->isLinked() )
			{
				return m_currentShaderProgram->use();
			}
			else
			{
				return true;
			}
		}
		else
		{
			return true;
		}
	}

	Rectanglei Renderer::getViewport() const
	{
		return m_viewportArea;
	}
	
	void Renderer::pushColor()
	{		
		ASSERT( !m_stackColor.empty() );
		m_stackColor.push_back( m_stackColor.back() );
	}
	
	void Renderer::popColor()
	{
		ASSERT( m_stackColor.size() > 1 );	// Always leave at least one element, the default color.
		m_stackColor.pop_back();
	}

	void Renderer::color( Color multiplyColor, Color additiveColor )
	{
		ASSERT( !m_stackColor.empty() );
		m_stackColor.back().first = multiplyColor;
		m_stackColor.back().second = additiveColor;
	}
	
	const Color& Renderer::getColorMultiply() const
	{
		ASSERT( !m_stackColor.empty() );
		return m_stackColor.back().first;
	}
	
	const Color& Renderer::getColorAdditive() const
	{
		ASSERT( !m_stackColor.empty() );
		return m_stackColor.back().second;
	}
	
	void Renderer::multiplyColor( Color color )
	{
		ASSERT( !m_stackColor.empty() );
		if( color != Color::White )
		{
			m_stackColor.back().first = m_stackColor.back().first.modulate( color );
		}
	}
	
	void Renderer::addColor( Color color )
	{
		ASSERT( !m_stackColor.empty() );
		if( color != Color::Invisible )
		{
			m_stackColor.back().second += color;
		}
	}

	void Renderer::updateUniformsForCurrentShaderProgram( Object::cptr host )
	{
		REQUIRES( m_currentShaderProgram );
		m_currentShaderProgram->updateBoundUniforms( host );
	}

	void Renderer::drawGeometry( PrimitiveType primitiveType, VertexBuffer::ptr vertices, size_t nVertices, size_t offset )
	{
		REQUIRES( vertices );
		
		vertices->applyForRendering();
		
#if GL_ES_VERSION_2_0		// Roundabout wireframe support on OpenGL ES implementations.
		if( m_currentRenderState.inWireframeMode )
		{
			primitiveType = PrimitiveType::LineLoop;
		}
#endif
		
		glDrawArrays( static_cast< GLenum >( primitiveType ), static_cast< GLint >( offset ), static_cast< GLsizei >( nVertices ));
		
		HANDLE_GL_ERRORS();
	}
	
	void Renderer::reportErrors()
	{
		HANDLE_GL_ERRORS();
	}
	
	void Renderer::pushDebugMarker( const std::string& label )
	{
#if DEV_MODE && TARGET_OS_IPHONE
		glPushGroupMarkerEXT( 0, label.c_str() );
#endif
	}
	
	void Renderer::popDebugMarker()
	{
#if DEV_MODE && TARGET_OS_IPHONE
		glPopGroupMarkerEXT();
#endif
	}
	
	void Renderer::insertDebugEvent( const std::string& label )
	{
#if DEV_MODE && TARGET_OS_IPHONE
		glInsertEventMarkerEXT( 0, label.c_str() );		
#endif
	}
	
	void Renderer::setViewport( const Rectanglei& viewport )
	{
		if( m_viewportArea != viewport )
		{
			m_viewportArea = viewport;
			glViewport( m_viewportArea.left(), m_viewportArea.top(), m_viewportArea.width(), m_viewportArea.height() );
			HANDLE_GL_ERRORS();
		}
	}
	
	void Renderer::clearColor( Color color )
	{
		if( m_currentRenderState.clearColor != color )
		{
			m_currentRenderState.clearColor = color;

			float r, g, b, a;
			m_currentRenderState.clearColor.getComponentsAsFloats( r, g, b, a );
			glClearColor( r, g, b, a );
			HANDLE_GL_ERRORS();
		}
	}
	
	Color Renderer::clearColor() const
	{
		return m_currentRenderState.clearColor;
	}

	void Renderer::clear()
	{
#if FRESH_SUPPORTS_DISCARD_FRAME_BUFFER
		
		if( isGLExtensionAvailable( "EXT_discard_framebuffer" ))
		{
			// Hint to OpenGL that frame buffer contents can be discarded (don't need to be cached),
			// because we're about to clear() anyway.
			//
			const GLenum discards[]  = { GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT };
			glDiscardFramebufferEXT( GL_FRAMEBUFFER, GET_NUM_STATIC_ARRAY_ELEMENTS( discards ), discards );
			HANDLE_GL_ERRORS();
		}
#endif
		
		glClear( GL_COLOR_BUFFER_BIT );								// TODO depth/stencil buffers too?
		HANDLE_GL_ERRORS();
	}
	
	Texture::ptr Renderer::createTexture( const std::string& strFilename )
	{
		REQUIRES( !strFilename.empty() );
		
		AssetPackage& assets = Application::instance().assetPackage();
		
		Texture::ptr result = assets.request< Texture >( strFilename );
		
		if( !result )
		{
			FRESH_THROW( FreshException, "Could not load texture " << strFilename );
		}
		return result;
	}
	
	void Renderer::applyTexture( Texture::ptr texture, size_t iSampler )
	{
		// REQUIRES( iSampler < getNumTextureSamplers() ); // enforced within...
		
		REQUIRES( !texture || texture->getTextureId() > 0 );	// If not a null texture, better have a non-zero id.
		
		bindTextureId( texture ? texture->getTextureId() : 0, iSampler );
	}
	
	void Renderer::setBlendMode( BlendMode blendMode )
	{
		if( m_currentRenderState.blendMode != blendMode )
		{
			if( m_currentRenderState.blendMode == BlendMode::None )
			{
				// Previously none. Restore blending.
				//
				glEnable( GL_BLEND );
			}	
			
			m_currentRenderState.blendMode = blendMode;
			
			if( m_currentRenderState.blendMode != BlendMode::None )
			{
				if( m_currentRenderState.lastEnabledBlendMode == BlendMode::None || m_currentRenderState.lastEnabledBlendMode != blendMode )
				{
					GLenum blendTermSrc, blendTermDest;
					
					switch( m_currentRenderState.blendMode )
					{
						default:
						case BlendMode::Alpha:
							blendTermSrc = GL_SRC_ALPHA;
							blendTermDest = GL_ONE_MINUS_SRC_ALPHA;
							break;
							
						case BlendMode::AlphaPremultiplied:
							blendTermSrc = GL_ONE;
							blendTermDest = GL_ONE_MINUS_SRC_ALPHA;
							break;
							
						case BlendMode::Multiply:
							blendTermSrc = GL_ZERO;
							blendTermDest = GL_SRC_COLOR;
							break;
							
						case BlendMode::Add:
							blendTermSrc = GL_SRC_ALPHA;
							blendTermDest = GL_ONE;
							break;
					}
					
					glBlendFunc( blendTermSrc, blendTermDest );
				}
				
				m_currentRenderState.lastEnabledBlendMode = blendMode;
			}
			else
			{
				glDisable( GL_BLEND );
			}
			HANDLE_GL_ERRORS();
		}
	}
	
	void Renderer::setStencilMode( StencilMode stencilMode, bool beginNewStencil )
	{
		if( m_currentRenderState.stencilMode != stencilMode || ( stencilMode == StencilMode::DrawToStencil && beginNewStencil ))
		{
			m_currentRenderState.stencilMode = stencilMode;
			
			switch( stencilMode )
			{
				case StencilMode::Ignore:
					enableStencil( false );
					break;
					
				case StencilMode::DrawToStencil:
					enableStencil( true );
					
					if( beginNewStencil )
					{
						glClear( GL_STENCIL_BUFFER_BIT );
						HANDLE_GL_ERRORS();

						m_nStencilId = ( m_nStencilId + 1 ) & 0xFF;

						// Skip the '0' index--it's the clear value and not used as a proper stencil id.
						if( m_nStencilId == 0 )
						{
							m_nStencilId = 1;
						}
					}

					glStencilFunc( GL_NEVER, m_nStencilId, 0xFF );
					glStencilOp( GL_REPLACE, GL_REPLACE, GL_REPLACE );					
					break;
					
				case StencilMode::MaskInclusive:
					enableStencil( true );
					glStencilFunc( GL_EQUAL, m_nStencilId, 0xFF );
					glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
					break;
					
				case StencilMode::MaskExclusive:
					enableStencil( true );
					glStencilFunc( GL_NOTEQUAL, m_nStencilId, 0xFF );
					glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
					break;
					
				default:
					ASSERT( false );
					break;
			}
			
			HANDLE_GL_ERRORS();
		}
	}
	
	VertexStructure::ComponentType Renderer::getTypeForGLType( int glType )
	{
		switch( glType )
		{
			case GL_FLOAT: return VertexStructure::Float;
			case GL_UNSIGNED_BYTE: return VertexStructure::UnsignedByte;
			default: ASSERT( false ); return VertexStructure::Float;
		}
	}
	
	int Renderer::getGLTypeForType( VertexStructure::ComponentType type )
	{
		static GLenum GLComponentTypes[] = { GL_FLOAT, GL_UNSIGNED_BYTE };		
		return GLComponentTypes[ type ];
	}
	
	ShaderProgram::ptr Renderer::createOrGetShaderProgram( NameRef objectName )
	{
		return createOrGetObject< ShaderProgram >( ObjectId( getShaderProgramClassName(), objectName ));
	}
	
	ShaderProgram::ptr Renderer::getShaderProgram( NameRef objectName ) const
	{
		return getObject< ShaderProgram >( ObjectId( getShaderProgramClassName(), objectName ));
	}
	
	VertexStructure::ptr Renderer::createOrGetVertexStructure( NameRef objectName )
	{
		return createOrGetObject< VertexStructure >( ObjectId( getVertexStructureClassName(), objectName ));
	}
	
	VertexStructure::ptr Renderer::getVertexStructure( NameRef objectName ) const
	{
		return getObject< VertexStructure >( ObjectId( getVertexStructureClassName(), objectName ));
	}

	size_t Renderer::getNumTextureSamplers() 
	{
		// Determine the maximum permitted iSampler value.
		//
		static int maxTextureImageUnits = 0;	// Static because we just want to call this once.
		
		if( maxTextureImageUnits <= 0 )
		{
			// Could use GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS here, but that would tell me all units
			// across all shader stages. In practice I'm normally just interested in accessing samplers
			// in fragment shaders only, which GL_MAX_TEXTURE_IMAGE_UNITS gives us.
			//
			glGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureImageUnits );
			ASSERT( maxTextureImageUnits > 0 );
		}
		
		return (size_t) maxTextureImageUnits;
	}

	bool Renderer::wireframeMode() const
	{
		return m_currentRenderState.inWireframeMode;
	}

	void Renderer::wireframeMode( bool wireframe )
	{
#if GL_VERSION_1_1
		glPolygonMode( GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL );
#endif
		m_currentRenderState.inWireframeMode = wireframe;
	}
	
	void Renderer::toggleWireframeMode()
	{
		wireframeMode( !wireframeMode() );
	}

}
