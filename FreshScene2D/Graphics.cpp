 /*
 *  Graphics.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/24/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#include "Graphics.h"
#include "Renderer.h"
#include "ShaderProgram.h"
#include "Stage.h"
#include "Objects.h"
#include "VertexStructure.h"
#include <limits>

namespace
{
	const fr::Color COLOR_DONT_CARE = 0;
	const fr::vec2 VECTOR_DONT_CARE( std::numeric_limits< fr::real >::infinity(), std::numeric_limits< fr::real >::infinity() );
}

namespace fr
{

	FRESH_DEFINE_CLASS( Graphics )
	
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( Graphics )

	void Graphics::clear()
	{
		m_doesNeedVertexBufferReconstruction = m_doesNeedBoundsRecalculation = true;
		m_vecDrawInstructions.clear();
	}

	void Graphics::clearLineStyle()
	{
		m_doesNeedVertexBufferReconstruction = m_doesNeedBoundsRecalculation = true;
		m_vecDrawInstructions.push_back( DrawInstruction( &Graphics::reallySetLineStyle, VECTOR_DONT_CARE, Color::Invisible ));
	}

	void Graphics::lineStyle( Color color )
	{
		m_doesNeedVertexBufferReconstruction = m_doesNeedBoundsRecalculation = true;
		m_vecDrawInstructions.push_back( DrawInstruction( &Graphics::reallySetLineStyle, VECTOR_DONT_CARE, color ));
	}

	void Graphics::lineStyle( float r, float g, float b, float a /* = 1.0f */ )
	{
		lineStyle( Color( r, g, b, a ));
	}

	void Graphics::beginFill( Color color )
	{
		ASSERT( !m_isFillSet );
		
		m_doesNeedVertexBufferReconstruction = m_doesNeedBoundsRecalculation = true;
		m_vecDrawInstructions.push_back( DrawInstruction( &Graphics::reallySetFillColor, VECTOR_DONT_CARE, color ));
		m_isFillSet = true;
	}

	void Graphics::beginFill( float r, float g, float b, float a /* = 1.0f */ )
	{
		beginFill( Color( r, g, b, a ));
	}

	void Graphics::endFill()
	{
		ASSERT( m_isFillSet );
		
		m_doesNeedVertexBufferReconstruction = m_doesNeedBoundsRecalculation = true;
		m_vecDrawInstructions.push_back( DrawInstruction( &Graphics::reallySetFillColor, VECTOR_DONT_CARE, Color::Invisible ));
		m_isFillSet = false;
	}

	void Graphics::moveTo( const vec2& p )
	{
		m_doesNeedVertexBufferReconstruction = m_doesNeedBoundsRecalculation = true;
		m_vecDrawInstructions.push_back( DrawInstruction( &Graphics::reallyMoveTo, p, COLOR_DONT_CARE ));
	}

	void Graphics::moveTo( real x, real y )
	{
		moveTo( vec2( x, y ));
	}

	void Graphics::lineTo( const vec2& p )
	{
		m_doesNeedVertexBufferReconstruction = m_doesNeedBoundsRecalculation = true;
		m_vecDrawInstructions.push_back( DrawInstruction( &Graphics::reallyLineTo, p, COLOR_DONT_CARE ));
	}

	void Graphics::lineTo( real x, real y )
	{
		lineTo( vec2( x, y ));
	}

	void Graphics::drawRect( const rect& rect )
	{
		m_doesNeedVertexBufferReconstruction = m_doesNeedBoundsRecalculation = true;
		m_vecDrawInstructions.push_back( DrawInstruction( &Graphics::reallyMoveTo, rect.ulCorner(), COLOR_DONT_CARE ));
		m_vecDrawInstructions.push_back( DrawInstruction( &Graphics::reallyLineTo, rect.urCorner(), COLOR_DONT_CARE ));
		m_vecDrawInstructions.push_back( DrawInstruction( &Graphics::reallyLineTo, rect.brCorner(), COLOR_DONT_CARE ));
		m_vecDrawInstructions.push_back( DrawInstruction( &Graphics::reallyLineTo, rect.blCorner(), COLOR_DONT_CARE ));
		m_vecDrawInstructions.push_back( DrawInstruction( &Graphics::reallyLineTo, rect.ulCorner(), COLOR_DONT_CARE ));
	}

	void Graphics::drawRect( real left, real top, real right, real bottom )
	{
		drawRect( rect( left, top, right, bottom ));
	}

	void Graphics::drawCircle( const vec2& center, real outerRadius, real innerRadius, int nSubdivisions )
	{
		// TODO innerRadius unsupported
		
		FnDrawInstruction drawInstruction = &Graphics::reallyMoveTo;
		for( int i = 0; i < nSubdivisions + 1; ++i )		// + 1 so that we complete the circle by repeating the starting points.
		{
			real angleRadians = TWO_PI * i / nSubdivisions;
			vec2 normal( sin( angleRadians ), cos( angleRadians ));
			
			m_vecDrawInstructions.push_back( DrawInstruction( drawInstruction, center + normal * outerRadius, COLOR_DONT_CARE ));
			drawInstruction = &Graphics::reallyLineTo;
		}
		m_doesNeedVertexBufferReconstruction = m_doesNeedBoundsRecalculation = true;
	}

	void Graphics::drawTriangles( const std::vector< vec2 >& trianglePoints )
	{
		for( size_t iPoint = 0; iPoint < trianglePoints.size(); ++iPoint )
		{
			FnDrawInstruction drawInstruction = &Graphics::reallyLineTo;			
			if(( iPoint % 3 ) == 0 )
			{
				// Begin new triangle.
				drawInstruction = &Graphics::reallyMoveTo;
			}
			
			m_vecDrawInstructions.push_back( DrawInstruction( drawInstruction, trianglePoints[ iPoint ], COLOR_DONT_CARE ));			
		}
		m_doesNeedVertexBufferReconstruction = m_doesNeedBoundsRecalculation = true;
	}
	
	void Graphics::buildVertexBuffers()
	{
		// Construct the vertex buffers and other draw elements.
		//
		if( !m_vertexBufferFills )
		{
			ASSERT( !m_vertexBufferLines );
			
			m_vertexBufferFills = createObject< VertexBuffer >();
			m_vertexBufferLines = createObject< VertexBuffer >();

			VertexStructure::ptr vertexStructure = Renderer::instance().createOrGetVertexStructure( "VS_Pos2Col4" );
			ASSERT( vertexStructure->getVertexSizeInBytes() > 0 );			

			m_vertexBufferFills->associateWithVertexStructure( vertexStructure );
			m_vertexBufferLines->associateWithVertexStructure( vertexStructure );

			m_shaderProgram = Renderer::instance().createOrGetShaderProgram( "PlainVanillaUntextured" );
			ASSERT( m_shaderProgram );
		}
		
		m_nFillElements = 0;
		m_nLineElements = 0;

		if( !m_vecDrawInstructions.empty() )
		{
			m_vertexBufferFills->clear();
			m_vertexBufferLines->clear();
			
			// Draw the shapes as fills.
			//
			m_isDoingFillPhase = true;	
			m_verticesCurrent = &m_verticesFills;
			
			drawPoints();
			
			// Load the VBO.
			m_nFillElements = m_verticesFills.size();
			if( m_nFillElements >= 3 )
			{
				m_vertexBufferFills->loadVertices( m_verticesFills.begin(), m_verticesFills.end() );
			}
			
			
			// Draw the shapes as lines.
			//
			m_isDoingFillPhase = false;	
			m_verticesCurrent = &m_verticesLines;

			drawPoints();			

			// Load the VBO.
			m_nLineElements = m_verticesLines.size();
			if( m_nLineElements >= 2 )
			{
				m_vertexBufferLines->loadVertices( m_verticesLines.begin(), m_verticesLines.end() );
			}
		}
		else
		{
			// Destroy the vertex buffers.
			//
			m_vertexBufferFills = m_vertexBufferLines = nullptr;
			
			// Alternatively, m_vertexBufferFills->clear() might serve better if we want to cache these a bit.
		}
		
		m_doesNeedVertexBufferReconstruction = false;		
	}

	void Graphics::draw()
	{
		if( m_vecDrawInstructions.empty() )
		{
			return;
		}

		// Reconstruct draw geometry if needed.
		//
		if( m_doesNeedVertexBufferReconstruction )
		{
			buildVertexBuffers();
		}

		if( m_nFillElements > 2 || m_nLineElements > 1 )
		{
			// Prepare the shader program.
			//
			Renderer::instance().useShaderProgram( m_shaderProgram );
			m_shaderProgram->updateBoundUniforms( this );
		}
		
		if( m_nFillElements > 2 )
		{
			Renderer::instance().drawGeometry( Renderer::PrimitiveType::Triangles, m_vertexBufferFills, m_nFillElements );
		}
		
		if( m_nLineElements > 1 )
		{
			Renderer::instance().drawGeometry( Renderer::PrimitiveType::Lines, m_vertexBufferLines, m_nLineElements );
		}
	}
		
	rect Graphics::localBounds() const
	{
		if( m_doesNeedBoundsRecalculation )
		{
			m_cachedBounds.setToInverseInfinity();
			
			for( size_t i = 0; i < m_vecDrawInstructions.size(); ++i )
			{
				const DrawInstruction& instruction = m_vecDrawInstructions[ i ];

				if( instruction.point != VECTOR_DONT_CARE )
				{
					m_cachedBounds.growToEncompass( instruction.point );
				}
			}
			
			m_doesNeedBoundsRecalculation = false;
		}
		return m_cachedBounds;
	}

	bool Graphics::hitTestPoint( const vec2& localLocation ) const
	{
		return localBounds().doesEnclose( localLocation, true );
	}

	Renderer::BlendMode Graphics::calculatedBlendMode() const
	{
		// TODO could check actual imposed colors.
		
		return Renderer::BlendMode::AlphaPremultiplied;
	}
	
	void Graphics::drawPoints()
	{
		ASSERT( m_verticesCurrent );
		
		m_verticesCurrent->clear();
		
		m_hadBegunVertices = false;
		
		m_lineColor = Color::White;
		m_fillColor = Color::Invisible;

		for( size_t i = 0; i < m_vecDrawInstructions.size(); ++i )
		{
			const DrawInstruction& instruction = m_vecDrawInstructions[ i ];
			
			FnDrawInstruction fn = instruction.functionDrawInstruction;
			
			(*this.*fn)( instruction.point, instruction.color );
		}
		
		// End any dangling sequences (e.g. lineTo() with nothing following it.)
		//
		if( m_hadBegunVertices )
		{
			endSequence();
		}
	}

	void Graphics::reallySetLineStyle( const vec2& p, Color color )
	{
		if( m_isDoingFillPhase )
		{
			return;
		}
		
		if( m_lineColor == color )
		{
			return;
		}
		
		m_lineColor = color;
		
		if( m_hadBegunVertices )
		{
			endSequence();
		}
	}

	void Graphics::reallySetFillColor( const vec2& p, Color color )
	{
		if( !m_isDoingFillPhase )
		{
			return;
		}
		
		if( m_fillColor == color )
		{
			return;
		}

		m_fillColor = color;
		
		if( m_hadBegunVertices )
		{
			endSequence();
		}
	}

	void Graphics::reallyMoveTo( const vec2& p, Color color )
	{
		if( m_hadBegunVertices )
		{
			endSequence();
		}
		
		beginSequence();
		addVertex( p );
	}

	void Graphics::reallyLineTo( const vec2& p, Color color )
	{
		if( !m_hadBegunVertices )
		{
			// A lineTo() at the start of a sequence is really just a moveTo().
			//
			reallyMoveTo( p, color );
		}
		else
		{
			
			ASSERT( m_verticesCurrent );
			
			if( m_isDoingFillPhase )
			{
				// Triangle list needs original point and last point--forming a "fan" with this new one.
				//
				if(( m_verticesCurrent->size() - m_iSequenceStartFills ) >= 3 )
				{
					m_verticesCurrent->push_back( m_verticesCurrent->at( m_iSequenceStartFills ) );
					m_verticesCurrent->push_back( m_verticesCurrent->at( m_verticesCurrent->size() - 2 ) );
				}
			}
			else
			{
				// Line list needs to repeat old point to connect to this new one.			
				//
				if(( m_verticesCurrent->size() - m_iSequenceStartLines ) >= 2 )
				{
					m_verticesCurrent->push_back( m_verticesCurrent->back() );
				}
			}
			addVertex( p );
		}
	}

	void Graphics::beginSequence()
	{
		ASSERT( !m_hadBegunVertices );
		
		m_iSequenceStartFills = m_verticesFills.size();
		m_iSequenceStartLines = m_verticesLines.size();
		
		m_hadBegunVertices = true;	
	}

	void Graphics::addVertex( const vec2& p )
	{
		ASSERT( m_hadBegunVertices );
		
		Color desiredColor = m_isDoingFillPhase ? m_fillColor : m_lineColor;
		
		if( desiredColor.getA() > 0 )
		{
			m_verticesCurrent->push_back( Vertex( p, desiredColor ));
		}
	}

	void Graphics::endSequence()
	{
		ASSERT( m_hadBegunVertices );

		m_hadBegunVertices = false;
	}
}

