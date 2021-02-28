/*
 *  Renderer.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 5/28/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_RENDERER_GL_ES2_H_INCLUDED_
#define FRESH_RENDERER_GL_ES2_H_INCLUDED_

#include "Asset.h"
#include "Archive.h"
#include "Vector2.h"
#include "Angle.h"
#include "Matrix4.h"
#include "Color.h"
#include "Rectangle.h"
#include "Singleton.h"
#include "Texture.h"
#include "ObjectMethod.h"
#include "MatrixStack.h"
#include "VertexBuffer.h"
#include "VertexStructure.h"
#include "ShaderProgram.h"

namespace fr
{
	class Renderer : public Object, public fr::ObjectSingleton< Renderer >
	{
	public:
		
		enum class PrimitiveType			// Mirrors OpenGL ES Values.
		{
			Points			= 0x0000,
			Lines			= 0x0001,
			LineLoop		= 0x0002,
			LineStrip		= 0x0003,
			Triangles		= 0x0004,
			TriangleStrip	= 0x0005,
			TriangleFan		= 0x0006
		};
		
		enum MatrixIdentifier
		{
			MAT_ModelView,
			MAT_Projection,
			MAT_Texture
		};
		
		enum class BlendMode
		{
			None,
			Alpha,
			AlphaPremultiplied,
			Multiply,
			Add
		};
		
		enum class StencilMode
		{
			Ignore,				// Draw to the color buffer, ignoring the stencil.
			DrawToStencil,		// Draw to the stencil buffer.
			MaskInclusive,		// Draw to the color buffer, allowing painting of pixels only where stencil has been "touched".
			MaskExclusive,		// Draw to the color buffer, allowing painting of pixels only where stencil has NOT been "touched".
		};
		
		virtual ~Renderer();
		
		ShaderProgram::ptr createShaderProgram();
		// PROMISES( result );
		// The returned object is the responsibility of the caller and should be referenced using a ShaderProgram::ptr.
		ShaderProgram::ptr getShaderProgram( NameRef objectName ) const;
		ShaderProgram::ptr createOrGetShaderProgram( NameRef objectName );		

		bool useShaderProgram( ShaderProgram::ptr shaderProgram );
		ShaderProgram::wptr getCurrentShaderProgram() const															{ return m_currentShaderProgram; }
		
		VertexStructure::ptr createOrGetVertexStructure( NameRef objectName );
		// PROMISES( result );
		// The returned object is retained by the system. It should be referenced by smart pointer.
		
		VertexStructure::ptr getVertexStructure( NameRef objectName ) const;
		// PROMISES NOTHING.
		// The returned object is retained by the system. It should be referenced by smart pointer.
		
		// Matrix functions
		//
		void setPerspectiveProjection( real halfFOVDegrees, real aspectRatio, real zNear, real zFar );
		void setOrthoProjection( real left, real right, real top, real bottom, real zNear = -1.0f, real zFar = 1.0f  );
		
		void setMatrixToIdentity( MatrixIdentifier whichMatrix = MAT_ModelView );
		void setMatrix( const mat4& value, MatrixIdentifier whichMatrix = MAT_ModelView );
		void pushMatrix( MatrixIdentifier whichMatrix = MAT_ModelView );
		void popMatrix( MatrixIdentifier whichMatrix = MAT_ModelView );
		
		void rotate( angle a, const vec3& axis, MatrixIdentifier whichMatrix = MAT_ModelView );		
		void rotate( angle a, MatrixIdentifier whichMatrix = MAT_ModelView )										{ rotate( a, vec3( 0, 0, 1 ), whichMatrix ); }
		void rotate( angle a, real x, real y, real z, MatrixIdentifier whichMatrix = MAT_ModelView )				{ rotate( a, vec3( x, y, z ), whichMatrix ); }
		
		void translate( const vec3& translation, MatrixIdentifier whichMatrix = MAT_ModelView );
		void translate( const vec2& translation, MatrixIdentifier whichMatrix = MAT_ModelView )						{ translate( vec3( translation.x, translation.y, 0 ), whichMatrix ); }
		void translate( real x, real y, MatrixIdentifier whichMatrix = MAT_ModelView )								{ translate( vec2( x, y ), whichMatrix ); }
		void translate( real x, real y, real z, MatrixIdentifier whichMatrix = MAT_ModelView )						{ translate( vec3( x, y, z ), whichMatrix ); }
		
		void scale( const vec3& scale, MatrixIdentifier whichMatrix = MAT_ModelView );
		void scale( real x, real y, MatrixIdentifier whichMatrix = MAT_ModelView )									{ scale( vec2( x, y ), whichMatrix ); }
		void scale( const vec2& scale_, MatrixIdentifier whichMatrix = MAT_ModelView )								{ scale( vec3( scale_.x, scale_.y, 1 ), whichMatrix ); }
		void scale( real x, real y, real z, MatrixIdentifier whichMatrix = MAT_ModelView )							{ scale( vec3( x, y, z ), whichMatrix ); }

		void shearX( angle a, MatrixIdentifier whichMatrix = MAT_ModelView );
		void shearY( angle a, MatrixIdentifier whichMatrix = MAT_ModelView );

		const mat4& getProjectionModelViewMatrix() const;
		const mat4& getProjectionMatrix() const;
		const mat4& getModelViewMatrix() const;
		const mat4& getTextureMatrix() const;
				
		vec2 screenToWorld2D( const vec2& screenCoords ) const;
		vec2 world2DToScreen( const vec2& worldCoords ) const;

		void bindTextureId( unsigned int idTexture, size_t iSampler = 0 );
		// REQUIRES( iSampler < getNumTextureSamplers() );

		unsigned int boundTextureId( size_t iSampler = 0 ) const;

		Rectanglei getViewport() const;
		void setViewport( const Rectanglei& viewport );
		
		SmartPtr< Texture > createTexture( const std::string& strFilename );
		// REQUIRES( !strFilename.empty() );
		// PROMISES( result != 0 );
		void applyTexture( Texture::ptr texture, size_t iSampler = 0 );
		// REQUIRES( iSampler < getNumTextureSamplers() );
		// Use null to turn off texturing.

		void updateUniformsForCurrentShaderProgram( Object::cptr host = nullptr );
		// REQUIRES( getCurrentShaderProgram() );

		void drawGeometry( PrimitiveType primitiveType, VertexBuffer::ptr vertices, size_t nVertices, size_t offset = 0 );
		// REQUIRES( vertices && vertices->isReadyForRendering() );
		
		void setBlendMode( BlendMode blendMode );
		void setStencilMode( StencilMode stencilMode, bool beginNewStencil = true );
	
		// Material color transform functions
		//
		void pushColor();
		void popColor();
		void color( Color multiplyColor, Color additiveColor = Color::Invisible );
		const Color& getColorMultiply() const;
		const Color& getColorAdditive() const;
		void multiplyColor( Color color );
		void addColor( Color color );
		
		void clearColor( Color color );
		Color clearColor() const;
		void clear();


		static VertexStructure::ComponentType getTypeForGLType( int glType );
		static int getGLTypeForType( VertexStructure::ComponentType type );
		
		static size_t getNumTextureSamplers();
		
		static BlendMode getBlendModeForTextureAlphaUsage( Texture::AlphaUsage alphaUsage );
		
		// Debugging.
		//
		void reportErrors();
		void pushDebugMarker( const std::string& label );
		void popDebugMarker();
		void insertDebugEvent( const std::string& label );
		
		bool wireframeMode() const;
		void wireframeMode( bool wireframe );
		void toggleWireframeMode();
		
	protected:
		
		MatrixStack& getMatrix( MatrixIdentifier whichMatrix );

		inline ClassInfo::Name getShaderProgramClassName() const
		{
			return "ShaderProgram";
		}
		
		inline ClassInfo::Name getVertexStructureClassName() const
		{
			return "VertexStructure";
		}
		
		void enableStencil( bool enable );
		
	private:
	
		mutable mat4 m_matrixProjectionModelView;
		
		MatrixStack m_matrixStackProjection;
		MatrixStack m_matrixStackModelView;
		MatrixStack m_matrixStackTexture;
		
		ShaderProgram::wptr m_currentShaderProgram;
		
		Rectanglei m_viewportArea;
		
		int m_nStencilId = 0;
		
		struct RenderState
		{
			static const size_t MAX_TEXTURE_UNITS = 8;	// TODO Might be more on certain hardware, but don't want to dynamically resize currentTextureId.
			
			unsigned int currentTextureId[ MAX_TEXTURE_UNITS ];
			BlendMode blendMode;
			BlendMode lastEnabledBlendMode;
			StencilMode stencilMode;
			Color clearColor;
			bool isStencilEnabled;
			bool inWireframeMode = false;
		};
		
		RenderState m_currentRenderState;

		typedef std::pair< Color, Color > ColorState;
		std::vector< ColorState > m_stackColor;
		
		DECLARE_ACCESSOR( Renderer, const mat4&, getProjectionModelViewMatrix );
		DECLARE_ACCESSOR( Renderer, const mat4&, getProjectionMatrix );
		DECLARE_ACCESSOR( Renderer, const mat4&, getModelViewMatrix );
		DECLARE_ACCESSOR( Renderer, const mat4&, getTextureMatrix );
		DECLARE_ACCESSOR( Renderer, const Color&, getColorMultiply );
		DECLARE_ACCESSOR( Renderer, const Color&, getColorAdditive );
				
		FRESH_DECLARE_CLASS( Renderer, Object )
	};

	///////////////////////////////////////////////////////////////////////////
	
	FRESH_ENUM_STREAM_IN_BEGIN( Renderer, PrimitiveType )
	FRESH_ENUM_STREAM_IN_CASE( Renderer::PrimitiveType, Points )
	FRESH_ENUM_STREAM_IN_CASE( Renderer::PrimitiveType, Lines )
	FRESH_ENUM_STREAM_IN_CASE( Renderer::PrimitiveType, LineLoop )
	FRESH_ENUM_STREAM_IN_CASE( Renderer::PrimitiveType, LineStrip )
	FRESH_ENUM_STREAM_IN_CASE( Renderer::PrimitiveType, Triangles )
	FRESH_ENUM_STREAM_IN_CASE( Renderer::PrimitiveType, TriangleStrip )
	FRESH_ENUM_STREAM_IN_CASE( Renderer::PrimitiveType, TriangleFan )
	FRESH_ENUM_STREAM_IN_END()
	
	FRESH_ENUM_STREAM_OUT_BEGIN( Renderer, PrimitiveType )
	FRESH_ENUM_STREAM_OUT_CASE( Renderer::PrimitiveType, Points )
	FRESH_ENUM_STREAM_OUT_CASE( Renderer::PrimitiveType, Lines )
	FRESH_ENUM_STREAM_OUT_CASE( Renderer::PrimitiveType, LineLoop )
	FRESH_ENUM_STREAM_OUT_CASE( Renderer::PrimitiveType, LineStrip )
	FRESH_ENUM_STREAM_OUT_CASE( Renderer::PrimitiveType, Triangles )
	FRESH_ENUM_STREAM_OUT_CASE( Renderer::PrimitiveType, TriangleStrip )
	FRESH_ENUM_STREAM_OUT_CASE( Renderer::PrimitiveType, TriangleFan )
	FRESH_ENUM_STREAM_OUT_END()

	///////////////////////////////////////////////////////////////////////////
	
	FRESH_ENUM_STREAM_IN_BEGIN( Renderer, BlendMode )
	FRESH_ENUM_STREAM_IN_CASE( Renderer::BlendMode, None )
	FRESH_ENUM_STREAM_IN_CASE( Renderer::BlendMode, Alpha )
	FRESH_ENUM_STREAM_IN_CASE( Renderer::BlendMode, AlphaPremultiplied )
	FRESH_ENUM_STREAM_IN_CASE( Renderer::BlendMode, Multiply )
	FRESH_ENUM_STREAM_IN_CASE( Renderer::BlendMode, Add )
	FRESH_ENUM_STREAM_IN_END()
	
	FRESH_ENUM_STREAM_OUT_BEGIN( Renderer, BlendMode )
	FRESH_ENUM_STREAM_OUT_CASE( Renderer::BlendMode, None )
	FRESH_ENUM_STREAM_OUT_CASE( Renderer::BlendMode, Alpha )
	FRESH_ENUM_STREAM_OUT_CASE( Renderer::BlendMode, AlphaPremultiplied )
	FRESH_ENUM_STREAM_OUT_CASE( Renderer::BlendMode, Multiply )
	FRESH_ENUM_STREAM_OUT_CASE( Renderer::BlendMode, Add )
	FRESH_ENUM_STREAM_OUT_END()

	///////////////////////////////////////////////////////////////////////////
	
	FRESH_ENUM_STREAM_IN_BEGIN( Renderer, StencilMode )
	FRESH_ENUM_STREAM_IN_CASE( Renderer::StencilMode, Ignore )
	FRESH_ENUM_STREAM_IN_CASE( Renderer::StencilMode, DrawToStencil )
	FRESH_ENUM_STREAM_IN_CASE( Renderer::StencilMode, MaskInclusive )
	FRESH_ENUM_STREAM_IN_CASE( Renderer::StencilMode, MaskExclusive )
	FRESH_ENUM_STREAM_IN_END()
	
	FRESH_ENUM_STREAM_OUT_BEGIN( Renderer, StencilMode )
	FRESH_ENUM_STREAM_OUT_CASE( Renderer::StencilMode, Ignore )
	FRESH_ENUM_STREAM_OUT_CASE( Renderer::StencilMode, DrawToStencil )
	FRESH_ENUM_STREAM_OUT_CASE( Renderer::StencilMode, MaskInclusive )
	FRESH_ENUM_STREAM_OUT_CASE( Renderer::StencilMode, MaskExclusive )
	FRESH_ENUM_STREAM_OUT_END()
	
	///////////////////////////////////////////////////////////////////////////
	
	inline Renderer::BlendMode Renderer::getBlendModeForTextureAlphaUsage( Texture::AlphaUsage alphaUsage )
	{
		switch( alphaUsage )
		{
			default:
			case Texture::AlphaUsage::None:
				return BlendMode::None;
				break;
			case Texture::AlphaUsage::Alpha:
				return BlendMode::Alpha;
				break;
			case Texture::AlphaUsage::AlphaPremultiplied:
				return BlendMode::AlphaPremultiplied;
				break;					
		}
	}
		
}		// END namespace


#endif
