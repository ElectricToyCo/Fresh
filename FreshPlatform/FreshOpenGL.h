#ifndef FRESH_OPENGL_H_
#define FRESH_OPENGL_H_

#include "FreshEssentials.h"

#	define	GL_VERTEX_ARRAY_OBJECTS_SUPPORTED 1

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR

#	include <OpenGLES/ES2/gl.h>
#	include <OpenGLES/ES2/glext.h>

#	define FRESH_USING_GL_ES 1

#	define FRESH_SUPPORTS_RENDERBUFFER_MULTISAMPLE 1
#	define FRESH_SUPPORTS_DISCARD_FRAME_BUFFER 1
#	define glGenVertexArrays glGenVertexArraysOES
#	define glDeleteVertexArrays glDeleteVertexArraysOES
#	define glBindVertexArray glBindVertexArrayOES
#	define GL_DEPTH_STENCIL GL_DEPTH_STENCIL_OES
#	define GL_UNSIGNED_INT_24_8 GL_UNSIGNED_INT_24_8_OES
#	define GL_DEPTH24_STENCIL8 GL_DEPTH24_STENCIL8_OES
#	define GL_HALF_FLOAT GL_HALF_FLOAT_OES
#	define GL_TEXTURE_MAX_LEVEL GL_TEXTURE_MAX_LEVEL_APPLE
#	define glRenderbufferStorageMultisample glRenderbufferStorageMultisampleAPPLE
#	define FRESH_GL_MULTISAMPLE_TYPE GL_RGBA8_OES
#	define GL_READ_FRAMEBUFFER GL_READ_FRAMEBUFFER_APPLE
#	define GL_DRAW_FRAMEBUFFER GL_DRAW_FRAMEBUFFER_APPLE

#elif TARGET_OS_MAC

#	include <OpenGL/gl.h>
#	include <OpenGL/glu.h>
#	include <OpenGL/glext.h>

#	define FRESH_SUPPORTS_RENDERBUFFER_MULTISAMPLE 1
#	define glGenVertexArrays glGenVertexArraysAPPLE
#	define glDeleteVertexArrays glDeleteVertexArraysAPPLE
#	define glBindVertexArray glBindVertexArrayAPPLE
#	define FRESH_GL_MULTISAMPLE_TYPE GL_RGBA8

#elif ANDROID

#	include <GLES2/gl2.h>
#	include <GLES2/gl2ext.h>
#	include <EGL/egl.h>

#	define FRESH_USING_GL_ES 1

#	undef	GL_VERTEX_ARRAY_OBJECTS_SUPPORTED
#	define	GL_VERTEX_ARRAY_OBJECTS_SUPPORTED 0
#	define FRESH_MANUAL_GL_BINDING 1
#	define FRESH_SUPPORTS_RENDERBUFFER_MULTISAMPLE 0

#	define glRenderbufferStorageMultisample glRenderbufferStorageMultisampleIMG
#	define glGenVertexArrays glGenVertexArraysOES
#	define glDeleteVertexArrays glDeleteVertexArraysOES
#	define glBindVertexArray glBindVertexArrayOES
#	define GL_DEPTH_STENCIL GL_DEPTH_STENCIL_OES
#	define GL_UNSIGNED_INT_24_8 GL_UNSIGNED_INT_24_8_OES
#	define GL_DEPTH24_STENCIL8 GL_DEPTH24_STENCIL8_OES
#	define GL_HALF_FLOAT GL_HALF_FLOAT_OES
#	define FRESH_GL_MULTISAMPLE_TYPE GL_RGBA8_OES

#elif RASPBIAN

#	include <GLES2/gl2.h>
#	include <GLES2/gl2ext.h>

#	undef	GL_VERTEX_ARRAY_OBJECTS_SUPPORTED
#	define	GL_VERTEX_ARRAY_OBJECTS_SUPPORTED 0
#	define FRESH_SUPPORTS_RENDERBUFFER_MULTISAMPLE 0

#	define glGenVertexArrays glGenVertexArraysOES
#	define glDeleteVertexArrays glDeleteVertexArraysOES
#	define glBindVertexArray glBindVertexArrayOES
#	define GL_DEPTH_STENCIL GL_DEPTH_STENCIL_OES
#	define GL_UNSIGNED_INT_24_8 GL_UNSIGNED_INT_24_8_OES
#	define GL_DEPTH24_STENCIL8 GL_DEPTH24_STENCIL8_OES
#	define GL_HALF_FLOAT GL_HALF_FLOAT_OES
#	define GL_TEXTURE_MAX_LEVEL GL_TEXTURE_MAX_LEVEL_APPLE
#	define glRenderbufferStorageMultisample glRenderbufferStorageMultisampleAPPLE
#	define FRESH_GL_MULTISAMPLE_TYPE GL_RGBA8_OES

#elif EMSCRIPTEN

#	include <GLES2/gl2.h>
#	include <GLES2/gl2ext.h>
#	include <EGL/egl.h>

#	define FRESH_USING_GL_ES 1

#	undef	GL_VERTEX_ARRAY_OBJECTS_SUPPORTED
#	define	GL_VERTEX_ARRAY_OBJECTS_SUPPORTED 0
#	define FRESH_MANUAL_GL_BINDING 1
#	define FRESH_SUPPORTS_RENDERBUFFER_MULTISAMPLE 0

#	define glRenderbufferStorageMultisample glRenderbufferStorageMultisampleIMG
#	define glGenVertexArrays glGenVertexArraysOES
#	define glDeleteVertexArrays glDeleteVertexArraysOES
#	define glBindVertexArray glBindVertexArrayOES
#	define GL_DEPTH_STENCIL GL_DEPTH_STENCIL_OES
#	define GL_UNSIGNED_INT_24_8 GL_UNSIGNED_INT_24_8_OES
#	define GL_DEPTH24_STENCIL8 GL_DEPTH24_STENCIL8_OES
#	define GL_HALF_FLOAT GL_HALF_FLOAT_OES
#	define FRESH_GL_MULTISAMPLE_TYPE GL_RGBA8_OES

#else

#if defined( _MSC_VER ) && !defined( GLEW_STATIC )
#		define GLEW_STATIC 1
#endif
#	include <GL/glew.h>

#	define FRESH_GL_MULTISAMPLE_TYPE GL_RGBA8
#	define FRESH_SUPPORTS_RENDERBUFFER_MULTISAMPLE 1

#	undef	GL_VERTEX_ARRAY_OBJECTS_SUPPORTED
#	define	GL_VERTEX_ARRAY_OBJECTS_SUPPORTED 0

#endif


#if FRESH_MANUAL_GL_BINDING

#	if	GL_VERTEX_ARRAY_OBJECTS_SUPPORTED
	extern PFNGLGENVERTEXARRAYSOESPROC glGenVertexArraysOES;
	extern PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOES;
	extern PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOES;
	extern PFNGLISVERTEXARRAYOESPROC glIsVertexArrayOES;
#	endif

#	if FRESH_SUPPORTS_RENDERBUFFER_MULTISAMPLE
	extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG glRenderbufferStorageMultisampleIMG;
#	endif

#	if FRESH_SUPPORTS_DISCARD_FRAME_BUFFER
	extern PFNGLDISCARDFRAMEBUFFEREXTPROC glDiscardFramebufferEXT;
#	endif

#endif

namespace fr
{
#ifdef DEBUG
	// Implemented as a macro to keep error reporting "lined" at the calling location.
#	define HANDLE_GL_ERRORS()	{ GLenum errCode = glGetError(); if( errCode != GL_NO_ERROR ) { dev_trace( "ERROR: glGetError() reported error (" << std::showbase << std::hex << errCode << "): " << fr::getGLErrorDescription( errCode )); } }
#else
#	define HANDLE_GL_ERRORS()
#endif

	void initGLExtensions();
	const char* getGLErrorDescription( GLenum errCode );

	bool isGLExtensionAvailable( const char* extensionName );
}

#if FRESH_USING_GL_ES
	using GLdouble = double;
#endif

#endif
