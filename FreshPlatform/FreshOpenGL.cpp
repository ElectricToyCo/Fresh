//
//  FreshOpenGL.cpp
//  Fresh
//
//  Created by Jeff Wofford on 04/21/2014.
//
//

#include "FreshOpenGL.h"
#include "FreshDebug.h"
#include <map>

#if FRESH_MANUAL_GL_BINDING

#	if	GL_VERTEX_ARRAY_OBJECTS_SUPPORTED
	PFNGLGENVERTEXARRAYSOESPROC glGenVertexArraysOES = nullptr;
	PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOES = nullptr;
	PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOES = nullptr;
	PFNGLISVERTEXARRAYOESPROC glIsVertexArrayOES = nullptr;
#	endif

#	if FRESH_SUPPORTS_RENDERBUFFER_MULTISAMPLE
	PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG glRenderbufferStorageMultisampleIMG = nullptr;
#	endif 

#	if FRESH_SUPPORTS_DISCARD_FRAME_BUFFER
	PFNGLDISCARDFRAMEBUFFEREXTPROC glDiscardFramebufferEXT = nullptr;
#	endif

#elif !GL_ES_VERSION_2_0 && ( defined( _WIN32 ) || defined( __linux__ ))

#	define FRESH_USE_GLEW 1
#	if defined( _MSC_VER ) && !defined( GLEW_STATIC )
#		define GLEW_STATIC 1
#	endif

#	include <GL/glew.h>
#	include <GL/gl.h>

#endif

namespace fr
{

	void initGLExtensions()
	{
		const GLubyte* szVersion = glGetString( GL_VERSION );
		release_trace( "Initializing extensions in OpenGL version " << szVersion );


#if FRESH_MANUAL_GL_BINDING
#	define BIND_GL_FUNCTION( fn )	fn = reinterpret_cast< decltype( fn ) >( eglGetProcAddress( #fn )); ASSERT( fn );
		
		// Bind extension functions. On some platforms we get GLEW to do this for us.
		//
#	if	GL_VERTEX_ARRAY_OBJECTS_SUPPORTED
		BIND_GL_FUNCTION( glGenVertexArraysOES )
		BIND_GL_FUNCTION( glBindVertexArrayOES )
		BIND_GL_FUNCTION( glDeleteVertexArraysOES )
		BIND_GL_FUNCTION( glIsVertexArrayOES )
#	endif
		
#	if FRESH_SUPPORTS_RENDERBUFFER_MULTISAMPLE
		BIND_GL_FUNCTION( glRenderbufferStorageMultisampleIMG )
#	endif
		
#	if FRESH_SUPPORTS_DISCARD_FRAME_BUFFER
		BIND_GL_FUNCTION( glDiscardFramebufferEXT )
#	endif

#else	// Non-manual binding.

#	if FRESH_USE_GLEW
		
		// Initialize GLEW on Windows and Linux.
		//
		GLenum err = glewInit();
		if( GLEW_OK != err )
		{
			dev_trace( "ERROR: GLEW initialization failed. Message: '" << glewGetErrorString( err ) << "'" );
		}

		ASSERT( glewIsSupported( "glCreateProgram" ) );
		
#	endif
		
#endif

	}

	bool isGLExtensionAvailable( const char* extensionName )
	{
		static std::map< std::string, bool > cachedResults;
		
		auto iter = cachedResults.find( extensionName );
		if( iter != cachedResults.end() )
		{
			return iter->second;
		}
		else
		{
			// Is it available?
			//
			const GLubyte* const szExtensions = glGetString( GL_EXTENSIONS );
			const std::string extensions( reinterpret_cast< const char* >( szExtensions ));
			
			const bool available = extensions.find( extensionName ) != std::string::npos;
			
			// Cache the result for the efficiency of future checks.
			//
			cachedResults[ extensionName ] = available;
			
			dev_trace( "OpenGL extension " << extensionName << " is " << ( available ? "available" : "unavailable" ));
			
			return available;
		}
	}
	
	
	const char* getGLErrorDescription( GLenum errCode )
	{
		switch( errCode )
		{
			case GL_NO_ERROR: return "GL_NO_ERROR - No error has been recorded. The value of this symbolic constant is guaranteed to be 0.";
			case GL_INVALID_ENUM: return "GL_INVALID_ENUM - An unacceptable value is specified for an enumerated argument. The offending command is ignored, and has no other side effect than to set the error flag.";
			case GL_INVALID_VALUE: return " GL_INVALID_VALUE - A numeric argument is out of range. The offending command is ignored, and has no other side effect than to set the error flag.";
			case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION - The specified operation is not allowed in the current state. The offending command is ignored, and has no other side effect than to set the error flag.";
			case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY - There is not enough memory left to execute the command. The state of the GL is undefined, except for the state of the error flags, after this error is recorded.";
#if !GL_ES_VERSION_2_0
			case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW - This command would cause a stack overflow. The offending command is ignored, and has no other side effect than to set the error flag.";
			case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW - This command would cause a stack underflow. The offending command is ignored, and has no other side effect than to set the error flag.";
#endif
			default: return "UNKNOWN ERROR";
		}
	}
}
