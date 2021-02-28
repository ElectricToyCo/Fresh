//
//  SimpleMesh.cpp
//  Fresh
//
//  Created by Jeff Wofford on 12/7/11.
//  Copyright (c) 2011 jeffwofford.com. All rights reserved.
//

#include "SimpleMesh.h"
#include "Objects.h"
#include "CommandProcessor.h"

namespace fr
{
	FRESH_DEFINE_CLASS( SimpleMesh )
	
	DEFINE_VAR( SimpleMesh, VertexBuffer::ptr, m_buffer );
	DEFINE_VAR( SimpleMesh, Renderer::PrimitiveType, m_primitiveType );
	DEFINE_VAR( SimpleMesh, size_t, m_nElements );
	DEFINE_VAR( SimpleMesh, size_t, m_offset );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( SimpleMesh )
	
	//////////////////////////////////////////////////////////////////////////////////
	
	FRESH_DEFINE_CLASS( SimpleMeshLoader )
	
	DEFINE_VAR( SimpleMeshLoader, std::string, m_vertexStructure );
	DEFINE_VAR( SimpleMeshLoader, Floats, m_components );
	DEFINE_VAR( SimpleMeshLoader, Renderer::PrimitiveType, m_primitiveType );
	
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( SimpleMeshLoader )
	
	SimpleMeshLoader::SimpleMeshLoader( const fr::ClassInfo& assignedClassInfo, fr::Object::NameRef objectName )
	:	Super( assignedClassInfo, objectName )
	{
		m_assetClass = getClass( "SimpleMesh" );
		doctorClass< SimpleMeshLoader >( [&]( ClassInfo& classInfo, SimpleMeshLoader& defaultObject )
										   {
											   DOCTOR_PROPERTY_ASSIGN( assetClass )
										   } );
	}
	
	void SimpleMeshLoader::loadAsset( Asset::ptr asset )
	{
		Super::loadAsset( asset );

		SimpleMesh::ptr mesh = dynamic_freshptr_cast< SimpleMesh::ptr >( asset );
		ASSERT( mesh );
		
		if( !m_vertexStructure.empty() )
		{
			VertexStructure::ptr vertexStructure = Renderer::instance().createOrGetVertexStructure( m_vertexStructure );
			if( vertexStructure )
			{
				if( m_components.empty() )
				{
					dev_warning( "Could not create SimpleMesh asset " << name() << " without a float array." );
				}
				else
				{
					// This assumes that the vertex structure actually consists of floats, which is false in the case of e.g. 4-byte color attributes.
					//
					const size_t nAttributesPerElement = vertexStructure->getVertexSizeInBytes() / sizeof( float );

					// CREATE THE MESH
					//
					mesh->create( m_primitiveType, m_components, vertexStructure, nAttributesPerElement );
					mesh->calculateBounds( m_components, nAttributesPerElement );
				}
			}
			else
			{
				dev_warning( "Could not create SimpleMesh asset " << name() << " because I could not find the vertex structure named '" << m_vertexStructure << "'." );
			}
		}
		else
		{
			dev_warning( "Could not create SimpleMesh asset " << name() << " without a vertex structure name." );
		}
	}
		
}
