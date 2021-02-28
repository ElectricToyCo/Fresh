//
//  VertexStructure.cpp
//  Fresh
//
//  Created by Jeff Wofford on 11/26/11.
//  Copyright (c) 2011 jeffwofford.com. All rights reserved.
//

#include "VertexStructure.h"
#include "FreshOpenGL.h"
#include "Objects.h"
#include "Renderer.h"

namespace
{
	size_t g_maxObservedAttributes = 0;
}

namespace fr
{
	FRESH_DEFINE_CLASS( VertexStructure )

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( VertexStructure )	

	DEFINE_VAR( VertexStructure, std::vector< Attribute >, m_attributes );
	
	size_t VertexStructure::getVertexSizeInBytes() const
	{
		size_t size = 0;
		for( auto iter = m_attributes.begin(); iter != m_attributes.end(); ++iter )
		{
			size += iter->getNumBytes();
		}
		return size;
	}
	
	bool VertexStructure::operator==( const VertexStructure& other ) const
	{
		// Must be same size.
		if( other.m_attributes.size() != m_attributes.size() )
		{
			return false;
		}
		
		// Must have the same attributes by name, not index.
		for( size_t i = 0; i < m_attributes.size(); ++i )
		{
			const Attribute* attribute = other.getAttribute( m_attributes[ i ].name );
			if( !attribute || ( *attribute != m_attributes[ i ] ))
			{
				return false;
			}
		}
		return true;
	}
	
	void VertexStructure::addAttribute( const std::string& attributeName, int nComponents, ComponentType type, AttributeUsage usage, int usageIndex /* = 0 */ )
	{
		m_attributes.push_back( Attribute( nComponents, type, usage, usageIndex, attributeName ) );
	}
	
	const VertexStructure::Attribute* VertexStructure::getAttribute( const std::string& attributeName ) const
	{
		for( auto iter = m_attributes.begin(); iter != m_attributes.end(); ++iter )
		{
			if( iter->name == attributeName )
			{
				return &( *iter );
			}
		}
		
		return 0;
	}
	
	STRUCT_DEFINE_SERIALIZATION_OPERATORS( VertexStructure::Attribute )
	

	void VertexStructure::apply( size_t offsetInBytes /* = 0 */ ) const
	{
		GLint maxAttributes = 0;
		glGetIntegerv( GL_MAX_VERTEX_ATTRIBS, &maxAttributes );
		
		// Disable all attributes.
		//
		for( size_t iAttribute = 0; iAttribute < g_maxObservedAttributes; ++iAttribute )
		{
			glDisableVertexAttribArray( static_cast< GLuint >( iAttribute ));
		}
		
		const size_t nVertexBytes = getVertexSizeInBytes();
		
		// Enable and configure valid attributes
		//
		
#if DEV_MODE
		if( m_attributes.empty() )
		{
			dev_warning( this << " is being applied but actually has no attributes." );
		}
#endif
		
		size_t attributeOffset = 0;
		for( size_t iAttribute = 0; iAttribute < m_attributes.size(); ++iAttribute )
		{
			const VertexStructure::Attribute& attribute = m_attributes[ iAttribute ];
			if( attribute.usage != VertexStructure::Ignore )
			{
				glEnableVertexAttribArray( static_cast< GLuint >( iAttribute ) );
				glVertexAttribPointer( static_cast< GLuint >( iAttribute ), 
									  attribute.nComponents, 
									  Renderer::getGLTypeForType( attribute.type ), 
									  GL_FALSE, 
									  static_cast< GLsizei >( nVertexBytes ), 
									  reinterpret_cast< const GLvoid* >( offsetInBytes + attributeOffset )); 
			}
			
			attributeOffset += attribute.getNumBytes();
		}

		g_maxObservedAttributes = std::max( g_maxObservedAttributes, m_attributes.size() );
		
		HANDLE_GL_ERRORS();
	}
	
	void VertexStructure::bindProgramAttributes( ShaderProgram& program ) const
	{
		for( size_t iAttribute = 0; iAttribute < m_attributes.size(); ++iAttribute )
		{
			program.bindAttribute( iAttribute, m_attributes[ iAttribute ].name );
		}
	}

}
