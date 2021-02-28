//
//  SimpleMesh.h
//  Fresh
//
//  Created by Jeff Wofford on 12/7/11.
//  Copyright (c) 2011 jeffwofford.com. All rights reserved.
//

#ifndef HouseOfShadows_SimpleMesh_h
#define HouseOfShadows_SimpleMesh_h

#include "Renderer.h"
#include "VertexBuffer.h"
#include "Asset.h"

namespace fr
{
	// A SimpleMesh is a convenience structure. 
	// It bundles up the closely related (but not mutually inclusive) concepts
	// of a VertexBuffer with its embedded vertex structure, a primitive type, and a 
	// span of elements (i.e. vertices or indices).
	// In a nutshell, use of a SimpleMesh simplifies the task of 
	// storing and drawing of vertex buffers.
	//
	class SimpleMesh : public Asset
	{
	public:
		
		template< typename AttributeIter >
		inline void create( Renderer::PrimitiveType primitiveType_, 
						   AttributeIter begin, 
						   AttributeIter end, 
						   VertexStructure::ptr vertexStructure,
						   size_t nAttributesPerElement = 1 )
		{
			REQUIRES( begin != end );
			REQUIRES( vertexStructure );
			REQUIRES( nAttributesPerElement > 0 );
			
			m_primitiveType = primitiveType_;
			
			if( !m_buffer )
			{
				m_buffer = createObject< VertexBuffer >();
			}
			
			m_buffer->associateWithVertexStructure( vertexStructure );
			m_buffer->loadVertices( begin, end );
			m_nElements = ( end - begin ) / nAttributesPerElement;
			m_offset = 0;
		}
		
		template< typename AttributeT >
		inline void create( Renderer::PrimitiveType primitiveType_, 
						   const std::vector< AttributeT >& vertices, 
						   VertexStructure::ptr vertexStructure,
						   size_t nAttributesPerElement = 1 )
		{
			REQUIRES( !vertices.empty() );
			REQUIRES( vertexStructure );
			REQUIRES( nAttributesPerElement > 0 );
			
			create( primitiveType_, vertices.begin(), vertices.end(), vertexStructure, nAttributesPerElement );
		}

		template< typename PositionIter >
		void calculateBounds( PositionIter begin, 
							  PositionIter end,
							  ptrdiff_t stride = 1 )
		{
			m_bounds.setToInverseInfinity();
			while( begin != end )
			{
				m_bounds.growToEncompass( *begin );
				
				begin += stride;
			}
			
			ASSERT( m_bounds.isWellFormed() );
		}

		template< typename PositionIter >
		void calculateBounds( const std::vector< PositionIter >& vertices,
							 ptrdiff_t stride = 1 )
		{
			calculateBounds( vertices.begin(), vertices.end(), stride );
		}
		
		inline bool isReadyToDraw() const
		{
			return m_buffer && m_buffer->isLoaded() &&
					m_nElements > 0 &&
					m_offset < m_nElements;
		}
		
		inline void draw()
		{
			REQUIRES( isReadyToDraw() );
			
			Renderer::instance().drawGeometry( m_primitiveType,
											  m_buffer,
											  m_nElements - m_offset,
											  m_offset );
		}
		
		const rect& bounds() const
		{
			return m_bounds;
		}

		// Inherited from Asset.
		// These are stubs--don't rely on them.
		//
		virtual size_t getMemorySize() const override { return sizeof( *this ); } // Not accurate, but that's okay. Just needs to be > 0. 

	private:
		
		VAR( VertexBuffer::ptr, m_buffer );
		DVAR( Renderer::PrimitiveType, m_primitiveType, Renderer::PrimitiveType::Triangles );
		DVAR( size_t, m_nElements, 0 );
		DVAR( size_t, m_offset, 0 );
		
		rect m_bounds = rect::INVERSE_INFINITE;
		
		FRESH_DECLARE_CLASS( SimpleMesh, Asset )
	};


	template<>
	inline void SimpleMesh::calculateBounds( std::vector< float >::const_iterator begin, std::vector< float >::const_iterator end,
									 ptrdiff_t stride )
	{
		m_bounds.setToInverseInfinity();
		while( begin != end )
		{
			vec2 point;
			point.x = *begin;
			if( ++begin == end )
			{
				break;
			}
			
			point.y = *begin;
			
			m_bounds.growToEncompass( point );
			
			begin += stride - 1;
		}
	}
	
	template<>
	inline void SimpleMesh::calculateBounds( const std::vector< float >& floats,
									 ptrdiff_t stride )
	{
		calculateBounds( floats.begin(), floats.end(), stride );
	}
	
	
	//////////////////////////////////////////////////////////////////////////////////
	
	class SimpleMeshLoader : public AssetLoader
	{
	public:
		
		virtual void loadAsset( Asset::ptr asset ) override;
		
	private:
		
		typedef std::vector< float > Floats;
		
		VAR( std::string, m_vertexStructure );
		VAR( Floats, m_components );
		DVAR( Renderer::PrimitiveType, m_primitiveType, Renderer::PrimitiveType::Triangles );
		
		FRESH_DECLARE_CLASS( SimpleMeshLoader, AssetLoader )
	};
	

}

#endif
