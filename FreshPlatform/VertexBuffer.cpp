//
//  VertexBuffer.cpp
//  Fresh
//
//  Created by Jeff Wofford on 11/28/11.
//  Copyright (c) 2011 jeffwofford.com. All rights reserved.
//

#include "VertexBuffer.h"
#include "Objects.h"
#include "FreshOpenGL.h"
#include "Renderer.h"

namespace
{
#if !GL_VERTEX_ARRAY_OBJECTS_SUPPORTED
	struct VirtualVAO
	{
		GLuint m_idVBO;
		fr::VertexStructure::ptr m_vertexStructure;
	};
	
	std::vector< std::unique_ptr< VirtualVAO >> g_vaos;
	VirtualVAO* g_boundVAO = nullptr;
	
	void bindVertexArray( GLuint iVAO )
	{
		if( iVAO > 0 )
		{
			--iVAO;
			
			ASSERT( iVAO < g_vaos.size() );
			const auto& vao = g_vaos[ iVAO ];
			ASSERT( vao );
			glBindBuffer( GL_ARRAY_BUFFER, vao->m_idVBO );

			if( vao->m_vertexStructure )
			{
				vao->m_vertexStructure->apply();
			}
			
			g_boundVAO = vao.get();
		}
		else
		{
			g_boundVAO = nullptr;
			glBindBuffer( GL_ARRAY_BUFFER, 0 );
		}
	}

	void genVertexArray( GLuint* vao )
	{
		// Find an available VAO slot.
		//
		auto iter = std::find_if( g_vaos.begin(), g_vaos.end(), []( const std::unique_ptr< VirtualVAO >& innerVAO )
				  {
					  return !innerVAO;
				  } );
		
		if( g_vaos.end() == iter )
		{
			g_vaos.emplace_back( new VirtualVAO{ 0, nullptr } );
			*vao = g_vaos.size();		// Not -1, because they're 1-indexed.
		}
		else
		{
			(*iter).reset( new VirtualVAO{ 0, nullptr } );
			*vao = ( iter - g_vaos.begin() ) + 1;
		}
	}
	
	void deleteVertexArrays( GLsizei n, GLuint* vaoArray )
	{
		if( g_vaos.empty() )
		{
			// Global already destructed. Never mind.
			return;
		}

		for( GLsizei i = 0; i < n; ++i )
		{
			auto index = vaoArray[ i ] - 1;
			ASSERT( index < g_vaos.size() );
			auto& vao = g_vaos[ index ];
			ASSERT( vao );
			
			if( vao.get() == g_boundVAO )
			{
				g_boundVAO = nullptr;
			}
			
			vao.reset();
		}
	}
	
	void bindBuffer( GLenum type, GLuint buffer )
	{
		glBindBuffer( type, buffer );
		
		if( g_boundVAO )
		{
			g_boundVAO->m_idVBO = buffer;
		}
	}
	
	inline void applyVertexStructure( fr::VertexStructure::ptr vertexStructure )
	{
		ASSERT( vertexStructure );
		if( g_boundVAO )
		{
			g_boundVAO->m_vertexStructure = vertexStructure;
			vertexStructure->apply();
		}
	}
	
#else
	
#	define bindVertexArray glBindVertexArray
#	define deleteVertexArrays glDeleteVertexArrays
#	define bindBuffer glBindBuffer

	inline void genVertexArray( GLuint* vao )
	{
		glGenVertexArrays( 1, vao );
	}
	
	inline void applyVertexStructure( fr::VertexStructure::ptr vertexStructure )
	{
		ASSERT( vertexStructure );
		vertexStructure->apply();
	}

#endif
	
	class VertexArrayObjectState
	{
	public:
		
		static GLuint currentVAO()
		{
			return s_lastBoundVertexArrayObject;
		}
		
		static void bindVAO( GLuint vao )
		{
			if( vao != s_lastBoundVertexArrayObject )
			{
				bindVertexArray( vao );
				s_lastBoundVertexArrayObject = vao;
			}
		}
		
	private:

		static GLuint s_lastBoundVertexArrayObject;
	};
	
	GLuint VertexArrayObjectState::s_lastBoundVertexArrayObject = 0;
	
}

namespace fr
{

	FRESH_DEFINE_CLASS( VertexBuffer )

	DEFINE_VAR( VertexBuffer, VertexStructure::ptr, m_vertexStructure );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( VertexBuffer )
		
	VertexBuffer::~VertexBuffer()
	{
		clear();
	}
	
	void VertexBuffer::associateWithVertexStructure( VertexStructure::ptr vertexStructure )
	{
		m_vertexStructure = vertexStructure;
	}
	
	// If no vertex structure has yet been associated, associates one.
	// Either way, this function then calls the corresponding function in VertexStructure.
	//
	void VertexBuffer::addAttribute( const std::string& attributeName, 
					  int nComponents, 
					  VertexStructure::ComponentType type, 
					  VertexStructure::AttributeUsage usage, 
					  int usageIndex )
	{
		if( !m_vertexStructure )
		{
			m_vertexStructure = createObject< VertexStructure >();
		}
		
		m_vertexStructure->addAttribute( attributeName,
										 nComponents,
										 type,
										 usage,
										 usageIndex );
	}
	
	void VertexBuffer::clear()
	{
		if( m_idVertexArrayObject )
		{
			// If deleting the currently bound VAO, unbind it.
			//
			if( VertexArrayObjectState::currentVAO() == m_idVertexArrayObject )
			{
				VertexArrayObjectState::bindVAO( 0 );
			}
			
			deleteVertexArrays( 1, &m_idVertexArrayObject );
			m_idVertexArrayObject = 0;
		}
		
		if( m_idVertexBufferObject )
		{
			glDeleteBuffers( 1, &m_idVertexBufferObject );
			m_idVertexBufferObject = 0;
		}
	}
	
	void VertexBuffer::loadRaw( const unsigned char* begin, size_t nBytes )
	{
		REQUIRES( begin );
		REQUIRES( nBytes > 0 );
		REQUIRES( m_vertexStructure );
		
		// Prepare the vertex array object to "record" vertex data information.
		//
		// Create the VAO if it is yet uncreated.
		//
		if( !m_idVertexArrayObject )
		{
			genVertexArray( &m_idVertexArrayObject );
			ASSERT( m_idVertexArrayObject );			
		}
		VertexArrayObjectState::bindVAO( m_idVertexArrayObject );
		
		// Fill the VBO (after creating it if needed).
		//
		if( !m_idVertexBufferObject )
		{
			glGenBuffers( 1, &m_idVertexBufferObject );
			ASSERT( m_idVertexBufferObject );
		}
		
		bindBuffer( GL_ARRAY_BUFFER, m_idVertexBufferObject );
		glBufferData( GL_ARRAY_BUFFER, 
					 nBytes,
					 reinterpret_cast< const GLvoid* >( begin ),
					 GL_STATIC_DRAW );
		
		HANDLE_GL_ERRORS();

		// Setup structure information, if available.
		//
		applyVertexStructure( m_vertexStructure );
		
		// Finish by restoring default state.
		//
		VertexArrayObjectState::bindVAO( 0 );
		bindBuffer( GL_ARRAY_BUFFER, 0 );
		
		m_nBytesLoaded = nBytes;
	}
	
	bool VertexBuffer::isLoaded() const
	{
		return m_idVertexArrayObject && m_idVertexBufferObject;
	}
	
	void VertexBuffer::applyForRendering()
	{
		ASSERT( isLoaded() );
		
		VertexArrayObjectState::bindVAO( m_idVertexArrayObject );
	}
	
	// Inherited from Asset
	//
	size_t VertexBuffer::getMemorySize() const
	{
		return m_nBytesLoaded;
	}
	
}

