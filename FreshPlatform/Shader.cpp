/*
 *  Shader.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 12/22/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#include "Shader.h"
#include "Objects.h"
#include "FreshFile.h"

#include "FreshOpenGL.h"

namespace
{
	GLenum getGLShaderTypeFromType( fr::Shader::Type type )
	{
		switch( type )
		{
			case fr::Shader::VertexShader:
				return GL_VERTEX_SHADER;
			case fr::Shader::FragmentShader:
				return GL_FRAGMENT_SHADER;
			default:
				ASSERT( false );		// Invalid shader type.
				return 0;
		}
	}
}

namespace fr
{
	
	FRESH_DEFINE_CLASS( Shader )
	
	DEFINE_VAR( Shader, Type, m_type );
	DEFINE_VAR( Shader, std::string, m_programSourceCode );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( Shader )
	
	Shader::~Shader()
	{
		if( m_idShader )
		{
			glDeleteShader( m_idShader );
		}
	}
	
	bool Shader::isLoaded() const
	{
		return m_idShader != 0 && m_isCompiled;
	}

	void Shader::loadSourceCodeFromText( const std::string& sourceCode, Type type )
	{
		REQUIRES( !sourceCode.empty() );
		
		m_programSourceCode = sourceCode;
		m_type = type;
		compileLoadedSourceCode();
		
		PROMISES( hasSourceCode() );
		PROMISES( isLoaded() || !getCompileErrors(false).empty() );
	}
	
	void Shader::loadSourceCodeFromFile( const std::string& filePath, Type type )
	{
		path exactPath = getResourcePath( filePath );
		ASSERT( !exactPath.empty() );
		
		m_programSourceCode = getFileText( exactPath );
		
		m_type = type;
		compileLoadedSourceCode();
		
		PROMISES( hasSourceCode() );
		PROMISES( isLoaded() || !getCompileErrors(false).empty() );
	}
	
	std::string Shader::getCompileErrors( bool clearErrors /* = false */ )
	{
		if( clearErrors )
		{
			std::string backupMessage = m_strErrorMessages;
			m_strErrorMessages.clear();
			return backupMessage;
		}
		else
		{
			return m_strErrorMessages;
		}
	}
	
	void Shader::compileLoadedSourceCode()
	{
		REQUIRES( hasSourceCode() );
		
		if( m_idShader )
		{
			glDeleteShader( m_idShader );
			m_idShader = 0;
		}
		
		m_idShader = glCreateShader( getGLShaderTypeFromType( getType() ));
		
		std::string adjustedSourceText;
		
		// Prepend shader statements to make non-GL ES targets comfortable with our GLSL code.
		//
#if GL_ES_VERSION_2_0
		
		adjustedSourceText = "#define FRESH_TARGET_OPENGL_ES 1\n"
							 "precision mediump float;";
		
#else

		adjustedSourceText = "#define FRESH_TARGET_OPENGL_ES 0\n"
		                     "#	define lowp \n"
							 "#	define mediump \n" 
							 "#	define highp \n";

#endif
		
		adjustedSourceText += getSourceCode();
		
		const char* szShaderText = adjustedSourceText.c_str();
		glShaderSource( m_idShader, 1, &szShaderText, NULL );
		glCompileShader( m_idShader );
		
		GLint compileStatus;
		glGetShaderiv( m_idShader, GL_COMPILE_STATUS, &compileStatus );
		m_isCompiled = ( compileStatus != 0 );
		
		buildErrorMessages();

		HANDLE_GL_ERRORS();
		
		PROMISES( isLoaded() || !getCompileErrors().empty() );
	}
	
	void Shader::buildErrorMessages()
	{
		GLint lenLog;
		glGetShaderiv( m_idShader, GL_INFO_LOG_LENGTH, &lenLog );
		
		if( lenLog > 0 )
		{
			m_strErrorMessages.clear();
			
			// Report errors or warnings.
			//
			std::unique_ptr< char[] > szLogText( new char[ lenLog + 1 ] );
			
			glGetShaderInfoLog( m_idShader, lenLog, &lenLog, szLogText.get() );
			m_strErrorMessages += szLogText.get();
		}			
	}
	
}
