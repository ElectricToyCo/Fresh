/*
 *  glESHelpers.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 5/29/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 * Adds projection and unprojection of a point.
 *
 */

#ifndef GLESHELPERS_H_INCLUDED
#define GLESHELPERS_H_INCLUDED

#include "FreshOpenGL.h"
#include "FreshEssentials.h"

namespace fr
{
	extern GLint gluProject(GLfloat objx, GLfloat objy, GLfloat objz,
							const GLfloat modelMatrix[16], 
							const GLfloat projMatrix[16],
							const GLint viewport[4],
							GLfloat *winx, GLfloat *winy, GLfloat *winz);

	extern GLint gluProject(GLdouble objx, GLdouble objy, GLdouble objz,
							const GLdouble modelMatrix[16],
							const GLdouble projMatrix[16],
							const GLint viewport[4],
							GLdouble *winx, GLdouble *winy, GLdouble *winz);

	extern GLint gluUnProject(GLfloat winx, GLfloat winy, GLfloat winz,
							  const GLfloat modelMatrix[16], 
							  const GLfloat projMatrix[16],
							  const GLint viewport[4],
							  GLfloat *objx, GLfloat *objy, GLfloat *objz);

	extern GLint gluUnProject(GLdouble winx, GLdouble winy, GLdouble winz,
							  const GLdouble modelMatrix[16],
							  const GLdouble projMatrix[16],
							  const GLint viewport[4],
							  GLdouble *objx, GLdouble *objy, GLdouble *objz);
}

#endif

