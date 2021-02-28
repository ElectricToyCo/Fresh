/*
 *  glESHelpers.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 5/29/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#include "glESHelpers.h"
#include "FreshDebug.h"
#include <cmath>

namespace
{
	template< typename Real >
	void gluMultMatrixVec( const Real matrix[16], const Real in[4], Real out[4] )
	{
		for( int i = 0; i < 4; ++i )
		{
			out[i] =
				in[0] * matrix[0*4+i] +
				in[1] * matrix[1*4+i] +
				in[2] * matrix[2*4+i] +
				in[3] * matrix[3*4+i];
		}
	}
	
	template< typename Real >
	void gluMultMatrices( const Real a[16], const Real b[16], Real r[16] )
	{
		for( int i = 0; i < 4; ++i )
		{
			for( int j = 0; j < 4; ++j )
			{
				r[i*4+j] =
					a[i*4+0]*b[0*4+j] +
					a[i*4+1]*b[1*4+j] +
					a[i*4+2]*b[2*4+j] +
					a[i*4+3]*b[3*4+j];
			}
		}
	}
	
	template< typename Real >
	void gluMakeIdentity( Real m[16] )
	{
		m[0+4*0] = 1; m[0+4*1] = 0; m[0+4*2] = 0; m[0+4*3] = 0;
		m[1+4*0] = 0; m[1+4*1] = 1; m[1+4*2] = 0; m[1+4*3] = 0;
		m[2+4*0] = 0; m[2+4*1] = 0; m[2+4*2] = 1; m[2+4*3] = 0;
		m[3+4*0] = 0; m[3+4*1] = 0; m[3+4*2] = 0; m[3+4*3] = 1;
	}
	
	template< typename Real >
	int gluInvertMatrix( const Real src[16], Real inverse[16] )
	{
		int i, j, k, swap;
		Real t;
		Real temp[4][4];
		
		for (i=0; i<4; i++) {
			for (j=0; j<4; j++) {
				temp[i][j] = src[i*4+j];
			}
		}
		gluMakeIdentity(inverse);
		
		for (i = 0; i < 4; i++) {
			/*
			 ** Look for largest element in column
			 */
			swap = i;
			for (j = i + 1; j < 4; j++) {
				if (fabs(temp[j][i]) > fabs(temp[i][i])) {
					swap = j;
				}
			}
			
			if (swap != i) {
				/*
				 ** Swap rows.
				 */
				for (k = 0; k < 4; k++) {
					t = temp[i][k];
					temp[i][k] = temp[swap][k];
					temp[swap][k] = t;
					
					t = inverse[i*4+k];
					inverse[i*4+k] = inverse[swap*4+k];
					inverse[swap*4+k] = t;
				}
			}
			
			if (temp[i][i] == 0) {
				/*
				 ** No non-zero pivot.  The matrix is singular, which shouldn't
				 ** happen.  This means the user gave us a bad matrix.
				 */
				return GL_FALSE;
			}
			
			t = temp[i][i];
			for (k = 0; k < 4; k++) {
				temp[i][k] /= t;
				inverse[i*4+k] /= t;
			}
			for (j = 0; j < 4; j++) {
				if (j != i) {
					t = temp[j][i];
					for (k = 0; k < 4; k++) {
						temp[j][k] -= temp[i][k]*t;
						inverse[j*4+k] -= inverse[i*4+k]*t;
					}
				}
			}
		}
		return GL_TRUE;
	}
	
	
	template< typename Real >
	GLint gluProject_Generic(Real objx, Real objy, Real objz,
			   const Real modelMatrix[16],
			   const Real projMatrix[16],
			   const GLint viewport[4],
					 Real *winx, Real *winy, Real *winz)
	{
		Real in[4];
		Real out[4];
		
		in[0]=objx;
		in[1]=objy;
		in[2]=objz;
		in[3]=1.0;
		gluMultMatrixVec(modelMatrix, in, out);
		gluMultMatrixVec(projMatrix, out, in);
		if (in[3] == 0.0)
		{
			return GL_FALSE;
		}
		
		in[0] /= in[3];
		in[1] /= in[3];
		in[2] /= in[3];
		/* Map x, y and z to range 0-1 */
		in[0] = in[0] * 0.5 + 0.5;
		in[1] = in[1] * 0.5 + 0.5;
		in[2] = in[2] * 0.5 + 0.5;
		
		/* Map x,y to viewport */
		in[0] = in[0] * viewport[2] + viewport[0];
		in[1] = in[1] * viewport[3] + viewport[1];
		
		*winx=in[0];
		*winy=in[1];
		*winz=in[2];
		
		return GL_TRUE;
	}

	
	template< typename Real >
	GLint gluUnProject_Generic(Real winx, Real winy, Real winz,
				 const Real modelMatrix[16],
				 const Real projMatrix[16],
				 const GLint viewport[4],
					   Real *objx, Real *objy, Real *objz)
	{
		Real finalMatrix[16];
		Real in[4];
		Real out[4];
		
		gluMultMatrices(modelMatrix, projMatrix, finalMatrix);
		if (!gluInvertMatrix(finalMatrix, finalMatrix))
		{
			return GL_FALSE;
		}
		
		in[0]=winx;
		in[1]=winy;
		in[2]=winz;
		in[3]=1.0;
		
		/* Map x and y from window coordinates */
		in[0] = (in[0] - viewport[0]) / viewport[2];
		in[1] = (in[1] - viewport[1]) / viewport[3];
		
		/* Map to range -1 to 1 */
		
		in[0] = in[0] * 2 - 1;
		in[1] = in[1] * 2 - 1;
		in[2] = in[2] * 2 - 1;
		
		gluMultMatrixVec(finalMatrix, in, out);
		if (out[3] == 0.0)
		{
			return GL_FALSE;
		}
		
		out[0] /= out[3];
		out[1] /= out[3];
		out[2] /= out[3];
		*objx = out[0];
		*objy = out[1];
		*objz = out[2];
		return GL_TRUE;
	}

}

namespace fr
{

	GLint gluProject(GLfloat objx, GLfloat objy, GLfloat objz, 
			   const GLfloat modelMatrix[16], 
			   const GLfloat projMatrix[16],
			   const GLint viewport[4],
			   GLfloat *winx, GLfloat *winy, GLfloat *winz)
	{
		return gluProject_Generic( objx, objy, objz, modelMatrix, projMatrix, viewport, winx, winy, winz );
	}

	GLint gluProject(GLdouble objx, GLdouble objy, GLdouble objz,
			   const GLdouble modelMatrix[16],
			   const GLdouble projMatrix[16],
			   const GLint viewport[4],
					 GLdouble *winx, GLdouble *winy, GLdouble *winz)
	{
		return gluProject_Generic( objx, objy, objz, modelMatrix, projMatrix, viewport, winx, winy, winz );
	}


	GLint gluUnProject(GLfloat winx, GLfloat winy, GLfloat winz,
							  const GLfloat modelMatrix[16],
							  const GLfloat projMatrix[16],
							  const GLint viewport[4],
							  GLfloat *objx, GLfloat *objy, GLfloat *objz)
	{
		return gluUnProject_Generic( winx, winy, winz, modelMatrix, projMatrix, viewport, objx, objy, objz );
	}

	GLint gluUnProject(GLdouble winx, GLdouble winy, GLdouble winz,
							  const GLdouble modelMatrix[16],
							  const GLdouble projMatrix[16],
							  const GLint viewport[4],
							  GLdouble *objx, GLdouble *objy, GLdouble *objz)
	{
		return gluUnProject_Generic( winx, winy, winz, modelMatrix, projMatrix, viewport, objx, objy, objz );
	}

}
