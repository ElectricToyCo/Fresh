//
//  UIFrame.cpp
//  Fresh
//
//  Created by Jeff Wofford on 2/13/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "UIFrame.h"

namespace
{
	using namespace fr;
	
	struct Vertex
	{
		vec2 position;
		vec2 texCoord;
	};
}

namespace fr
{
	
	FRESH_DEFINE_CLASS( UIFrame )
	DEFINE_VAR( UIFrame, bool, m_creationReshapeToEncompassSiblings );
	DEFINE_VAR( UIFrame, bool, m_creationReshapeToEncompassChildren );
	DEFINE_VAR( UIFrame, rect, m_pixelCoordinatesInnerArea );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( UIFrame )
	
	UIFrame::UIFrame( const fr::ClassInfo& assignedClassInfo, fr::Object::NameRef objectName )
	:	Super( assignedClassInfo, objectName )
	{
		ignoreFrameAttachment( false );
	}

	rect UIFrame::frame() const
	{
		return Super::frame();
	}

	void UIFrame::frame( const rect& r )
	{
		reshape( r );
	}
	
	void UIFrame::reshape( const rect& innerArea )
	{
		if( texture() )
		{
			const vec2 textureDimensions = vector_cast< real >( texture()->getOriginalDimensions() );
			const rect texCoordsInnerArea( m_pixelCoordinatesInnerArea.ulCorner() / textureDimensions, m_pixelCoordinatesInnerArea.brCorner() / textureDimensions );
		
			// Create the frame mesh.
			//
			const int NUM_ROWS_COLS = 4;

			const vec2 positionsPerStep[ NUM_ROWS_COLS ] = { innerArea.ulCorner() - m_pixelCoordinatesInnerArea.ulCorner(),
															 innerArea.ulCorner(),
															 innerArea.brCorner(),
															 innerArea.brCorner() + ( textureDimensions - m_pixelCoordinatesInnerArea.brCorner() ) };
			const vec2 texCoordsPerStep[ NUM_ROWS_COLS ] = { vec2::ZERO, texCoordsInnerArea.ulCorner(), texCoordsInnerArea.brCorner(), vec2( 1.0f, 1.0f ) };
			
			// Calculate the raw points.
			//
			const size_t NUM_POINTS = NUM_ROWS_COLS * NUM_ROWS_COLS;
			Vertex points[ NUM_ROWS_COLS ][ NUM_ROWS_COLS ];
			
			for( int y = 0; y < NUM_ROWS_COLS; ++y )
			{
				for( int x = 0; x < NUM_ROWS_COLS; ++x )
				{
					Vertex& vertex = points[ x ][ y ];
					vertex.position.set( positionsPerStep[ x ].x, positionsPerStep[ y ].y );
					vertex.texCoord.set( texCoordsPerStep[ x ].x, texCoordsPerStep[ y ].y );
				}
			}
			
			// Tesselate the points into triangles, 3x3 quads, two per quad.
			//
			std::vector< Vertex > vertices;
			vertices.reserve( NUM_POINTS * 6 );
			
			for( int y = 0; y < NUM_ROWS_COLS - 1; ++y )
			{
				for( int x = 0; x < NUM_ROWS_COLS - 1; ++x )
				{
					vertices.push_back( points[ x     ][ y     ] );
					vertices.push_back( points[ x     ][ y + 1 ] );
					vertices.push_back( points[ x + 1 ][ y     ] );
					vertices.push_back( points[ x + 1 ][ y     ] );
					vertices.push_back( points[ x     ][ y + 1 ] );
					vertices.push_back( points[ x + 1 ][ y + 1 ] );
				}
			}
			
			SimpleMesh::ptr simpleMesh = createObject< SimpleMesh >();
			simpleMesh->create( Renderer::PrimitiveType::Triangles, vertices, Renderer::instance().createOrGetVertexStructure( "VS_Pos2TexCoord2" ) );
			
			const vec2* pointsBegin = &( points[ 0 ][ 0 ].position );
			const vec2* pointsEnd = pointsBegin + ( NUM_POINTS * 2 );	// * 2 because skipping texcoords.
			simpleMesh->calculateBounds( pointsBegin, pointsEnd, 2 );	// Skipping texcoords.
			
			mesh( simpleMesh );

			Super::frame( innerArea );
			
			m_lastReshapeFrame = Super::frame();
		}
	}
	
	void UIFrame::reshape( const vec2& innerDimensions )
	{
		reshape( rect( vec2::ZERO, innerDimensions ));
	}
	
	void UIFrame::reshapeToEncompassChildren()
	{
		mesh( nullptr );	// Get rid of any older mesh so that it doesn't interfere with my children's bounds calculations.
		reshape( localBounds() );
	}

	void UIFrame::reshapeToEncompassSiblings()
	{
		if( DisplayObjectContainer::ptr myParent = parent() )
		{
			mesh( nullptr );	// Get rid of any older mesh so that it doesn't interfere with my children's bounds calculations.
			reshape( parentToLocal( myParent->getChildrenBounds() ));
		}
	}
	
	void UIFrame::update()
	{
		Super::update();
		
		if( m_lastReshapeFrame != Super::frame() )
		{
			reshape( Super::frame() );
		}
	}
	
	void UIFrame::onAddedToStage()
	{
		Super::onAddedToStage();
		
		if( m_creationReshapeToEncompassChildren )
		{
			reshapeToEncompassChildren();
		}
		else if( m_creationReshapeToEncompassSiblings )
		{
			reshapeToEncompassSiblings();
		}
		else
		{
			reshape( Super::frame() );
		}
	}

}

