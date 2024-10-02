//
//  RenderTarget.h
//  Fresh
//
//  Created by Jeff Wofford on 11/22/11.
//  Copyright (c) 2011 jeffwofford.com. All rights reserved.
//

#pragma once

#include "Object.h"
#include "Property.h"
#include "FreshVector.h"
#include "Color.h"
#include "StructSerialization.h"

namespace fr
{
	class Texture;

	class RenderTarget : public Object
	{
	public:

		enum class Buffer
		{
			Color,
			DepthStencil,
			NUM_BUFFERS
		};

		enum class ColorComponentType
		{
			UnsignedByte,
			HalfFloat,
		};

		enum class OutputType
		{
			None,
			Texture,
			Renderbuffer,
			RenderbufferMultisample,
		};

		virtual ~RenderTarget();

		bool isCreated() const							{ return m_idFrameBuffer != 0; }

		struct BufferFormat : public SerializableStruct< BufferFormat >
		{
			ColorComponentType colorComponentType;
			OutputType outputType;

			BufferFormat() : BufferFormat( ColorComponentType::UnsignedByte, OutputType::None ) {}

			explicit BufferFormat( ColorComponentType type, OutputType outputType_ = OutputType::None )
			:	colorComponentType( type )
			,	outputType( outputType_ )
			{
				STRUCT_BEGIN_PROPERTIES
				STRUCT_ADD_PROPERTY( colorComponentType )
				STRUCT_ADD_PROPERTY( outputType )
				STRUCT_END_PROPERTIES
			}

			bool operator==( const BufferFormat& other ) const
			{
				return colorComponentType == other.colorComponentType &&
						outputType == other.outputType;
			}
		};

		STRUCT_DECLARE_SERIALIZATION_OPERATORS( BufferFormat )

		void create( unsigned int width_,
					 unsigned int height_,
					 const BufferFormat& colorBufferFormat,
					 const BufferFormat* depthBufferFormat = nullptr );
		// REQUIRES( width_ > 0 && height_ > 0 );
		// PROMISES( isCreated() );

		SYNTHESIZE( Color, clearColor )
		SYNTHESIZE( bool, doInitialClearOnCapture )

		bool usingDepthStencil() const;

		void destroy();
		// PROMISES( !isCreated() );

		bool isCapturing() const						{ return m_isCapturing; }
		void beginCapturing();
		// REQUIRES( isCreated() );
		// REQUIRES( !isCapturing() );
		// PROMISES( isCapturing() );
		void endCapturing();
		// REQUIRES( isCreated() );
		// REQUIRES( isCapturing() );
		// PROMISES( !isCapturing() );

		void beginServing();

		SmartPtr< Texture > getCapturedTexture( Buffer buffer = Buffer::Color ) const;
		// REQUIRES( isCreated() );

        SYNTHESIZE_GET( BufferFormat, colorBufferFormat );
        SYNTHESIZE_GET( BufferFormat, depthBufferFormat );

		unsigned int width() const						{ return m_width; }
		unsigned int height() const						{ return m_height; }

		virtual void load( const Manifest::Map& properties ) override;

	protected:

		void saveFramebufferState();
		void restoreFramebufferState();

	private:

		unsigned int m_idFrameBuffer = 0;

		DVAR( unsigned int, m_width, 0 );
		DVAR( unsigned int, m_height, 0 );
		DVAR( BufferFormat, m_colorBufferFormat, BufferFormat( ColorComponentType::UnsignedByte, OutputType::Texture ));
		VAR( BufferFormat, m_depthBufferFormat );
		DVAR( Color, m_clearColor, 0xFF111122 );
		DVAR( bool, m_doInitialClearOnCapture, false );

		SmartPtr< Texture > m_attachedTextures[ size_t( Buffer::NUM_BUFFERS ) ];
		unsigned int m_attachedRenderBufferIds[ size_t( Buffer::NUM_BUFFERS ) ];

		int m_savedViewport[4];				// Rectangle edges.
		int m_savedPriorFramebufferObject = 0;

		bool m_isCapturing = false;

		FRESH_DECLARE_CLASS( RenderTarget, Object )
	};

	FRESH_ENUM_STREAM_IN_BEGIN( RenderTarget, ColorComponentType )
	FRESH_ENUM_STREAM_IN_CASE( RenderTarget::ColorComponentType, UnsignedByte )
	FRESH_ENUM_STREAM_IN_CASE( RenderTarget::ColorComponentType, HalfFloat )
	FRESH_ENUM_STREAM_IN_END()

	FRESH_ENUM_STREAM_OUT_BEGIN( RenderTarget, ColorComponentType )
	FRESH_ENUM_STREAM_OUT_CASE( RenderTarget::ColorComponentType, UnsignedByte )
	FRESH_ENUM_STREAM_OUT_CASE( RenderTarget::ColorComponentType, HalfFloat )
	FRESH_ENUM_STREAM_OUT_END()

	FRESH_ENUM_STREAM_IN_BEGIN( RenderTarget, OutputType )
	FRESH_ENUM_STREAM_IN_CASE( RenderTarget::OutputType, None )
	FRESH_ENUM_STREAM_IN_CASE( RenderTarget::OutputType, Texture )
	FRESH_ENUM_STREAM_IN_CASE( RenderTarget::OutputType, Renderbuffer )
	FRESH_ENUM_STREAM_IN_CASE( RenderTarget::OutputType, RenderbufferMultisample )
	FRESH_ENUM_STREAM_IN_END()

	FRESH_ENUM_STREAM_OUT_BEGIN( RenderTarget, OutputType )
	FRESH_ENUM_STREAM_OUT_CASE( RenderTarget::OutputType, None )
	FRESH_ENUM_STREAM_OUT_CASE( RenderTarget::OutputType, Texture )
	FRESH_ENUM_STREAM_OUT_CASE( RenderTarget::OutputType, Renderbuffer )
	FRESH_ENUM_STREAM_OUT_CASE( RenderTarget::OutputType, RenderbufferMultisample )
	FRESH_ENUM_STREAM_OUT_END()

}

