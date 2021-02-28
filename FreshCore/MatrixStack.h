/*
 *  MatrixStack.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 5/28/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_MATRIX_STACK_H_INCLUDED
#define FRESH_MATRIX_STACK_H_INCLUDED

#include "Matrix4.h"
#include <vector>

namespace fr
{

	class MatrixStack
	{
	public:
		
		typedef float Real;
		
		MatrixStack();
		
		const Matrix4< Real >& top() const;
		
		void load( const Matrix4< Real >& matrix );
		void loadIdentity();
		void perspective( float radianFOV, float aspectRatio, float nearPlaneDist, float farPlaneDist );
		void ortho( Real left, Real right, Real bottom, Real top, Real near, Real far );
		void push();
		void pop();
		void translate( float x, float y, float z );
		void rotate( float angleRadians, float x, float y, float z );
		void scale( float x, float y, float z );
		void shearX( float angleRadians );
		void shearY( float angleRadians );
		void translateLocal( float x, float y, float z );
		void rotateLocal( float angleRadians, float x, float y, float z );
		void scaleLocal( float x, float y, float z );
		void shearXLocal( float angleRadians );
		void shearYLocal( float angleRadians );
		
	private:
		
		std::vector< Matrix4< Real > > m_matrixStack;
	};
	
}

#endif
