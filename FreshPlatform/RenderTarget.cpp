//
//  RenderTarget.cpp
//  Fresh
//
//  Created by Jeff Wofford on 11/22/11.
//  Copyright (c) 2011 jeffwofford.com. All rights reserved.
//

#include "RenderTarget.h"
#include "Application.h"
#include "Objects.h"
#include "Renderer.h"
#include "FreshOpenGL.h"
#include "CommandProcessor.h"

namespace fr
{

	STRUCT_DEFINE_SERIALIZATION_OPERATORS( RenderTarget::BufferFormat )

	FRESH_DEFINE_CLASS( RenderTarget )
	
	DEFINE_VAR( RenderTarget, unsigned int, m_width );
	DEFINE_VAR( RenderTarget, unsigned int, m_height );
	DEFINE_VAR( RenderTarget, BufferFormat, m_colorBufferFormat );
	DEFINE_VAR( RenderTarget, BufferFormat, m_depthBufferFormat );
	DEFINE_VAR( RenderTarget, Color, m_clearColor );
	DEFINE_VAR( RenderTarget, bool, m_doInitialClearOnCapture );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( RenderTarget )

	RenderTarget::~RenderTarget()
	{
		if( isCapturing() )
		{
			endCapturing();
		}
		
		destroy();
	}

	void RenderTarget::create( unsigned int width_,
							   unsigned int height_,
							   const BufferFormat& colorBufferFormat,
							   const BufferFormat* depthBufferFormat )
	{
		REQUIRES( width_ > 0 && height_ > 0 );
		
		destroy();
		
		m_width = width_;
		m_height = height_;
		
		glGenFramebuffers( 1, &m_idFrameBuffer );
		
		saveFramebufferState();
		
		glBindFramebuffer( GL_FRAMEBUFFER, m_idFrameBuffer );

		auto associateBackingStore = [&]( const BufferFormat& bufferFormat,
										 GLenum attachment,
										 GLint internalTextureFormat,
										 GLenum pixelFormat,
										 GLenum pixelType,
										 GLenum renderBufferFormat,
										 Buffer buffer ) -> GLuint
		{
			if( bufferFormat.outputType == OutputType::Texture )
			{
				// Attach texture.
				//
				GLuint idAttachedTexture = 0;
				
				glGenTextures( 1, &idAttachedTexture );
				glBindTexture( GL_TEXTURE_2D, idAttachedTexture );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
				
				glTexImage2D( GL_TEXTURE_2D, 0, internalTextureFormat, m_width, m_height, 0, pixelFormat, pixelType, nullptr );
				// TODO generate mipmaps if requested and m_width, m_height are non-power-of-two.
				
				// Bind a Fresh texture object to this OpenGL texture object.
				//
				m_attachedTextures[ size_t( buffer )] = createObject< Texture >();
				m_attachedTextures[ size_t( buffer )]->assumeId( idAttachedTexture, Vector2ui( m_width, m_height ));
				
				glBindTexture( GL_TEXTURE_2D, 0 );	// Done talking to texture. Unbind.

				// Associate the texture with the framebuffer attachment.
				//
				glFramebufferTexture2D( GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, idAttachedTexture, 0 );
				
				return idAttachedTexture;
			}
			else if( bufferFormat.outputType != OutputType::None )
			{
				// Attach render buffer.
				//
				GLuint idRenderBuffer = 0;
				glGenRenderbuffers( 1, &idRenderBuffer );
				glBindRenderbuffer( GL_RENDERBUFFER, idRenderBuffer );
				
#if FRESH_SUPPORTS_RENDERBUFFER_MULTISAMPLE
				if(( isGLExtensionAvailable( "GL_EXT_framebuffer_multisample" ) ||
					 isGLExtensionAvailable( "GL_APPLE_framebuffer_multisample" ))
				    && bufferFormat.outputType == OutputType::RenderbufferMultisample )
				{
					// Attach multisample render buffer.
					//
					glRenderbufferStorageMultisample( GL_RENDERBUFFER, 4, renderBufferFormat, m_width, m_height );
				}
				else
#endif
				{
					glRenderbufferStorage( GL_RENDERBUFFER, renderBufferFormat, m_width, m_height );
				}

				
				glFramebufferRenderbuffer( GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, idRenderBuffer );
				
				HANDLE_GL_ERRORS();
				
				m_attachedRenderBufferIds[ size_t( buffer ) ] = idRenderBuffer;
				
				return idRenderBuffer;
			}
			else
			{
				return 0;
			}
		};
		
		// Generate render texture.
		//
		GLenum textureComponentType;
		switch( colorBufferFormat.colorComponentType )
		{
			default:
			case ColorComponentType::UnsignedByte:
				textureComponentType = GL_UNSIGNED_BYTE;
				break;
			case ColorComponentType::HalfFloat:
				textureComponentType = GL_HALF_FLOAT;
				break;
		}
		
		associateBackingStore( colorBufferFormat,
							  GL_COLOR_ATTACHMENT0,
							  GL_RGBA,
							  GL_RGBA,
							  textureComponentType,
							  FRESH_GL_MULTISAMPLE_TYPE,
							  Buffer::Color );
		
		// Generate depth buffer, if needed.
		//
		if( depthBufferFormat && depthBufferFormat->outputType != OutputType::None )
		{
			if( isGLExtensionAvailable( "GL_EXT_packed_depth_stencil" ) ||
			    isGLExtensionAvailable( "GL_OES_packed_depth_stencil" ))
			{
				const auto renderBuffer = associateBackingStore( *depthBufferFormat,
									  GL_DEPTH_ATTACHMENT,
									  GL_DEPTH_STENCIL,
									  GL_DEPTH_STENCIL,
									  GL_UNSIGNED_INT_24_8,
									  GL_DEPTH24_STENCIL8,
									  Buffer::DepthStencil );
				
				ASSERT( renderBuffer > 0 );
				
				// Attach the stencil attachment too.
				glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderBuffer );
			}
			else
			{
				// TODO Stencil support somehow? (WebGL)
				associateBackingStore( *depthBufferFormat,
									  GL_STENCIL_ATTACHMENT,
									  GL_DEPTH_STENCIL,
									  GL_DEPTH_STENCIL,
									  GL_DEPTH_COMPONENT16,
									  GL_STENCIL_INDEX8,
									  Buffer::DepthStencil );
			}
		}
		
		GLenum status = glCheckFramebufferStatus( GL_FRAMEBUFFER );
		if( status != GL_FRAMEBUFFER_COMPLETE )
		{
			dev_warning( this << " had incomplete frame buffer." );
		}
		
		restoreFramebufferState();
		
		HANDLE_GL_ERRORS();

		PROMISES( isCreated() );
	}
	
	bool RenderTarget::usingDepthStencil() const
	{
		return m_attachedRenderBufferIds[ size_t( Buffer::DepthStencil ) ] || m_attachedTextures[ size_t( Buffer::DepthStencil ) ];
	}

	void RenderTarget::destroy()
	{
		for( size_t i = 0; i < size_t( Buffer::NUM_BUFFERS ); ++i )
		{
			m_attachedTextures[ i ] = nullptr;		// Deletes by virtue of smart ptr nullification.
			m_attachedRenderBufferIds[ i ] = 0;
		}
		
		m_width = m_height = 0;
		
		if( m_idFrameBuffer )
		{
			glDeleteFramebuffers( 1, &m_idFrameBuffer );
			m_idFrameBuffer = 0;
		}
		
		PROMISES( !isCreated() );
	}

	void RenderTarget::beginCapturing()
	{
		REQUIRES( isCreated() );
		REQUIRES( !isCapturing() );
		
		m_isCapturing = true;
		
		saveFramebufferState();

		GLenum whichFramebuffer;
#ifdef GL_DRAW_FRAMEBUFFER
		whichFramebuffer = GL_DRAW_FRAMEBUFFER;
#else
		whichFramebuffer = GL_FRAMEBUFFER;
#endif
		
		glBindFramebuffer( whichFramebuffer, m_idFrameBuffer );
		
		auto& renderer = Renderer::instance();
		
		renderer.setViewport( Rectanglei( 0, 0, m_width, m_height ));
		
		// Initial clear.
		//
		if( m_doInitialClearOnCapture )
		{
			const auto oldClearColor = renderer.clearColor();

			renderer.clearColor( m_clearColor );
			renderer.clear();	// TODO depth/stencil too.
			
//TODO			glClear( GL_COLOR_BUFFER_BIT | ( usingDepthStencil() ? GL_DEPTH_BUFFER_BIT : 0 ));		// TODO integrate with renderer clearcolor and clear
			
			renderer.clearColor( oldClearColor );
		}

		HANDLE_GL_ERRORS();
	}

	void RenderTarget::endCapturing()
	{
		REQUIRES( isCreated() );
		REQUIRES( isCapturing() );

		restoreFramebufferState();
		
		// Let the renderer know that any textures that it *thought* were bound aren't.
		//
		Renderer& renderer = Renderer::instance();
		renderer.bindTextureId( 0 );
		
		// More renderer finagling. TODO centralize this stuff.
		//
		renderer.setBlendMode( Renderer::BlendMode::Multiply );		// TODO trying to trick Renderer into reseting blendMode

		m_isCapturing = false;
		
#ifdef GL_READ_FRAMEBUFFER
		beginServing();
#endif
		
		HANDLE_GL_ERRORS();
	}
	
	void RenderTarget::beginServing()
	{
		REQUIRES( isCreated() );
		
#ifdef GL_READ_FRAMEBUFFER
		glBindFramebuffer( GL_READ_FRAMEBUFFER, m_idFrameBuffer );
#else
		ASSERT( false );		// read frame buffer serving not supported.
#endif
	}

	Texture::ptr RenderTarget::getCapturedTexture( Buffer buffer ) const
	{
		REQUIRES( isCreated() );
		return m_attachedTextures[ static_cast< size_t >( buffer ) ];
	}

	void RenderTarget::saveFramebufferState()
	{
		ASSERT( !m_savedPriorFramebufferObject );
		
		glGetIntegerv( GL_VIEWPORT, m_savedViewport );
		glGetIntegerv( GL_FRAMEBUFFER_BINDING, &m_savedPriorFramebufferObject );
	}
	
	void RenderTarget::restoreFramebufferState()
	{
		ASSERT( m_savedPriorFramebufferObject || m_savedViewport[ 2 ] > 0 );
		glBindFramebuffer( GL_FRAMEBUFFER, m_savedPriorFramebufferObject );
		HANDLE_GL_ERRORS();

		m_savedPriorFramebufferObject = 0;
		
		Renderer::instance().setViewport( Rectanglei( m_savedViewport[ 0 ], m_savedViewport[ 1 ], m_savedViewport[ 2 ], m_savedViewport[ 3 ] ));
	}

	void RenderTarget::load( const Manifest::Map& properties )
	{
		Super::load( properties );
		
		if( !isInert() )
		{
			if( m_width == 0 )
			{
				m_width = Application::instance().getWindowDimensions().x;
			}
			if( m_height == 0 )
			{
				m_height = Application::instance().getWindowDimensions().y;
			}
			
			create( m_width, m_height, m_colorBufferFormat, &m_depthBufferFormat );
		}
	}
	
}
