/*
 *  Graphics.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/24/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_GRAPHICS_H_INCLUDED
#define FRESH_GRAPHICS_H_INCLUDED

#include "Object.h"
#include "Vector2.h"
#include "Color.h"
#include "Rectangle.h"
#include "VertexBuffer.h"
#include "Renderer.h"
#include <vector>

namespace fr
{
	
	class ShaderProgram;
	
	// A Graphics object is owned by each Sprite.
	// It is used for free-form drawing on the Sprite.
	// The Graphics is rendered behind all other elements of the Sprite.
	//
	// Loosely based on http://help.adobe.com/en_US/AS3LCR/Flash_10.0/flash/display/Graphics.html
	//
	class Graphics : public Object
	{
	public:

		void clear();
		
		void clearLineStyle();
		void lineStyle( Color color );
		void lineStyle( float r, float g, float b, float a = 1.0f );
		void beginFill( Color color );
		void beginFill( float r, float g, float b, float a = 1.0f );
		void endFill();

		void moveTo( const vec2& p );
		void moveTo( real x, real y );
		void lineTo( const vec2& p );
		void lineTo( real x, real y );
		void drawRect( const rect& rect );
		void drawRect( real left, real top, real right, real bottom );
	
		void drawCircle( const vec2& center, real outerRadius, real innerRadius = 0, int nSubdivisions = 16 );
		void drawTriangles( const std::vector< vec2 >& trianglePoints );		
		
		void draw();
		
		rect localBounds() const;
		
		bool hitTestPoint( const vec2& localLocation ) const;

		Renderer::BlendMode calculatedBlendMode() const;

	protected:
		
		typedef void (Graphics::*FnDrawInstruction)( const vec2& p, Color color );
		
		struct DrawInstruction
		{
			DrawInstruction( FnDrawInstruction functionDrawInstruction_, const vec2& point_, Color color_ )
			:	functionDrawInstruction( functionDrawInstruction_ )
			,	point( point_ )
			,	color( color_ )
			{}
			
			FnDrawInstruction functionDrawInstruction;
			vec2 point;
			Color color;
		};
		
		void buildVertexBuffers();
		
		void reallySetLineStyle( const vec2& p, Color color );
		void reallySetFillColor( const vec2& p, Color color );
		void reallyMoveTo( const vec2& p, Color color );
		void reallyLineTo( const vec2& p, Color color );
		
		void drawPoints();
		
		void beginSequence();
		void addVertex( const vec2& p );
		void endSequence();
		
	private:
		
		struct Vertex
		{
			vec2 position;
			vec4 color;
			
			Vertex( const vec2& p, const vec4& c ) : position( p ), color( c ) {}
		};
				
		Color m_lineColor = Color::Black;
		Color m_fillColor = Color::Invisible;
		
		bool m_isFillSet = false;
		bool m_isDoingFillPhase = true;
		bool m_hadBegunVertices = false;
		bool m_doesNeedVertexBufferReconstruction = false;
		
		mutable bool m_doesNeedBoundsRecalculation = false;
		mutable rect m_cachedBounds = rect::INVERSE_INFINITE;
		
		std::vector< DrawInstruction > m_vecDrawInstructions;
		
		std::vector< Vertex >* m_verticesCurrent = nullptr;
		
		std::vector< Vertex > m_verticesLines;
		std::vector< Vertex > m_verticesFills;
		
		size_t m_iSequenceStartLines = 0;
		size_t m_iSequenceStartFills = 0;
		
		size_t m_nLineElements = 0;
		size_t m_nFillElements = 0;
		
		VertexBuffer::ptr m_vertexBufferLines;
		VertexBuffer::ptr m_vertexBufferFills;
		
		SmartPtr< ShaderProgram > m_shaderProgram;
		
		FRESH_DECLARE_CLASS( Graphics, Object )
	};
	
}

#endif
