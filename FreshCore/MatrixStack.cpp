/*
 *  MatrixStack.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 5/28/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#include "MatrixStack.h"


namespace fr
{

	MatrixStack::MatrixStack()
	{
		m_matrixStack.emplace_back( Matrix4< Real >() );
	}

	const Matrix4< MatrixStack::Real >& MatrixStack::top() const
	{
		return m_matrixStack.back();
	}
	
	void MatrixStack::load( const Matrix4< Real >& matrix )
	{
		ASSERT( m_matrixStack.size() > 0 );
		m_matrixStack.back() = matrix;
	}

	void MatrixStack::loadIdentity()
	{
		ASSERT( m_matrixStack.size() > 0 );
		m_matrixStack.back().makeIdentity();
	}
	
	void MatrixStack::perspective( float radianFOV, float aspectRatio, float nearPlaneDist, float farPlaneDist )
	{
		Matrix4f projection;
		projection.makePerspectiveProjection( radianFOV, aspectRatio, nearPlaneDist, farPlaneDist );
		m_matrixStack.back() = projection * m_matrixStack.back();
	}

	void MatrixStack::ortho( Real left_, Real right_, Real bottom_, Real top_, Real near_, Real far_ )
	{
		Matrix4f orthoProjection;
		orthoProjection.makeOrthoProjection( left_, right_, bottom_, top_, near_, far_ );
		m_matrixStack.back() = orthoProjection * m_matrixStack.back();
	}
	
	void MatrixStack::push()
	{
		ASSERT( m_matrixStack.size() > 0 );
		m_matrixStack.emplace_back( m_matrixStack.back() );
	}

	void MatrixStack::pop()
	{
		ASSERT( m_matrixStack.size() > 0 );
		m_matrixStack.pop_back();
		ASSERT( m_matrixStack.size() > 0 );
	}

	void MatrixStack::translate( float x, float y, float z )
	{
		ASSERT( m_matrixStack.size() > 0 );
		
		Matrix4< Real > translation;
		translation.makeTranslation( Vector3< Real >( x, y, z ));
		
		m_matrixStack.back() *= translation;
	}

	void MatrixStack::rotate( float angleRadians, float x, float y, float z )
	{
		ASSERT( m_matrixStack.size() > 0 );
		
		Matrix4< Real > rotation;
		rotation.makeRotation( angleRadians, Vector3f( x, y, z ));
		
		m_matrixStack.back() *= rotation;			
	}

	void MatrixStack::scale( float x, float y, float z )
	{
		ASSERT( m_matrixStack.size() > 0 );
		
		Matrix4< Real > scale_;
		scale_.makeScale( Vector3< Real >( x, y, z ));
		
		m_matrixStack.back() *= scale_;						
	}

	void MatrixStack::shearX( float angleRadians )
	{
		ASSERT( m_matrixStack.size() > 0 );
		
		Matrix4< Real > m;
		m.makeShearX( angleRadians );
		
		m_matrixStack.back() *= m;
	}
	
	void MatrixStack::shearY( float angleRadians )
	{
		ASSERT( m_matrixStack.size() > 0 );
		
		Matrix4< Real > m;
		m.makeShearY( angleRadians );
		
		m_matrixStack.back() *= m;
	}
	
	void MatrixStack::translateLocal( float x, float y, float z )
	{
		ASSERT( m_matrixStack.size() > 0 );
		
		Matrix4< Real > translation;
		translation.makeTranslation( Vector3< Real >( x, y, z ));
		
		m_matrixStack.back() = translation * m_matrixStack.back();
	}
	
	void MatrixStack::rotateLocal( float angleRadians, float x, float y, float z )
	{
		ASSERT( m_matrixStack.size() > 0 );
		
		Matrix4< Real > rotation;
		rotation.makeRotation( angleRadians, Vector3f( x, y, z ));
		
		m_matrixStack.back() = rotation * m_matrixStack.back();
	}
	
	void MatrixStack::scaleLocal( float x, float y, float z )
	{
		ASSERT( m_matrixStack.size() > 0 );
		
		Matrix4< Real > scale_;
		scale_.makeScale( Vector3< Real >( x, y, z ));
		
		m_matrixStack.back() = scale_ * m_matrixStack.back();
	}
	
	void MatrixStack::shearXLocal( float angleRadians )
	{
		ASSERT( m_matrixStack.size() > 0 );
		
		Matrix4< Real > m;
		m.makeShearX( angleRadians );
		
		m_matrixStack.back() = m * m_matrixStack.back();
	}

	void MatrixStack::shearYLocal( float angleRadians )
	{
		ASSERT( m_matrixStack.size() > 0 );
		
		Matrix4< Real > m;
		m.makeShearY( angleRadians );
		
		m_matrixStack.back() = m * m_matrixStack.back();
	}

}
