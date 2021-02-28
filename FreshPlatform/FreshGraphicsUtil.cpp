//
//  FreshGraphicsUtil.cpp
//  Fresh
//
//  Created by Jeff Wofford on 11/29/11.
//  Copyright (c) 2011 jeffwofford.com. All rights reserved.
//

#include "FreshGraphicsUtil.h"
#include "Renderer.h"

namespace fr
{
	void drawQuad2D()
	{
		Renderer& renderer = Renderer::instance();
		
		static VertexBuffer::ptr geometry;
		
		if( !geometry )
		{
			geometry = createOrGetObject< VertexBuffer >( "VB_NormalQuad" );
			
			ASSERT( geometry );
			if( !geometry->isLoaded() )
			{
				std::vector< VertexPos3Col > points;
				
				points.resize( 4 );
				for( size_t i = 0; i < points.size(); ++i )
				{
					points[ i ].pos[ 0 ] = ( i & 1 ) ? 1.0f : 0;
					points[ i ].pos[ 1 ] = (( i >> 1 ) & 1 ) ? 1.0f : 0;
					points[ i ].pos[ 2 ] = 0;
					
					points[ i ].texCoord[ 0 ] = points[ i ].pos[ 0 ];
					points[ i ].texCoord[ 1 ] = points[ i ].pos[ 1 ];
					
					points[ i ].color.set( 1.0f, 1.0f, 1.0f, 1.0f );
				}
				
				geometry->associateWithVertexStructure( renderer.createOrGetVertexStructure( "VS_Pos3TexCoord2Col4" ) );
				geometry->loadVertices( points.begin(), points.end() );
			}
		}
		
		renderer.updateUniformsForCurrentShaderProgram();
		renderer.drawGeometry( Renderer::PrimitiveType::TriangleStrip, geometry, 4 );		// Quad
	}

	void drawOrigin( real length )
	{
		Renderer& renderer = Renderer::instance();
		
		// TODO stinky implementation can't really change length after the first time.
		static VertexBuffer::ptr geometry;
		
		if( !geometry )
		{
			geometry = createOrGetObject< VertexBuffer >( "VB_Origin" );		
			ASSERT( geometry );
			
			if( !geometry->isLoaded() )
			{
				std::vector< VertexPos3Col > points;
				
				points.resize( 6 );
				for( size_t i = 0; i < points.size(); ++i )
				{
					points[ i ].pos.setToZero();
					points[ i ].texCoord.setToZero();
					
					if( i & 1 )
					{
						points[ i ].pos[ i >> 1 ] = length;
					}
					
					points[ i ].color.setToZero();
					points[ i ].color[ i >> 1 ] = 1.0f;
				}
				
				geometry->associateWithVertexStructure( renderer.createOrGetVertexStructure( "VS_Pos3TexCoord2Col4" ) );
				geometry->loadVertices( points.begin(), points.end() );
			}
		}
		
		renderer.updateUniformsForCurrentShaderProgram();
		renderer.drawGeometry( Renderer::PrimitiveType::Lines, geometry, 6 );		// 6 = endpoints of 3 line segments
	}

	void drawWorldGrid( const Vector3i& gridSize )
	{
		Renderer& renderer = Renderer::instance();
		
		// TODO stinky implementation can't really change size after the first time.
		static VertexBuffer::ptr geometry;
		
		if( !geometry )
		{
			geometry = createOrGetObject< VertexBuffer >( "VB_WorlGrid" );
			
			ASSERT( geometry );
			if( !geometry->isLoaded() )
			{
				std::vector< VertexPos3Col > points;
				
				points.resize( gridSize.x * gridSize.y * gridSize.z );
				for( int z = 0; z < gridSize.z; ++z )
				{
					for( int y = 0; y < gridSize.y; ++y )
					{
						for( int x = 0; x < gridSize.x; ++x )
						{
							VertexPos3Col& vertex = points[ x + y * gridSize.x + z * gridSize.x * gridSize.y ];
							
							vertex.pos.set( (float)x, (float)y, (float)z );
							vertex.color.set( 1, 1, 1, 1 );// = vertex.pos / ( GRID_SIZE_FLOAT - 1.0f );
						}
					}
				}
				
				geometry->associateWithVertexStructure( renderer.createOrGetVertexStructure( "VS_Pos3TexCoord2Col4" ) );
				geometry->loadVertices( points.begin(), points.end() );
			}
		}
		
		renderer.updateUniformsForCurrentShaderProgram();
		renderer.drawGeometry( Renderer::PrimitiveType::Points, geometry, gridSize.x * gridSize.y * gridSize.z );
	}
}
