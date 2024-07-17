/*
 *  ShaderProgram.cpp
 *  TestShaders
 *
 *  Created by Jeff Wofford on 12/22/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#include "ShaderProgram.h"
#include "Objects.h"
#include "Renderer.h"
#include "FreshOpenGL.h"
#include "CommandProcessor.h"

namespace
{
	using namespace fr;
	
	inline Shader::ptr createAndAttachShaderFromTextOrFile( ShaderProgram::ptr shaderProgram,
														   const std::string& text,
														   const std::string& file,
														   Shader::Type type )
	{
		ASSERT( shaderProgram );
		Shader::ptr shader = shaderProgram->createShader();
		
		if( !text.empty() )
		{
			shader->loadSourceCodeFromText( text, type );
		}
		else if( !file.empty() )
		{
			shader->loadSourceCodeFromFile( file, type );
		}
		else
		{
			return 0;
		}
		
		if( !shader->isLoaded() )
		{
			trace( type << " for file '" << ( file.empty() ? "<inline src>" : file ) << "' had errors: " );
			trace( "======================================================" );
			trace( shader->getCompileErrors( true ) );
			trace( "======================================================" );
			
			// Release.
			shader = 0;
		}
		else
		{
			shaderProgram->attachShader( shader );
		}
		
		return shader;
	}
}

namespace fr
{
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( ShaderProgramLoader )
	
	ShaderProgramLoader::ShaderProgramLoader( const fr::ClassInfo& assignedClassInfo, fr::Object::NameRef objectName )
	:	Super( assignedClassInfo, objectName )
	{
		m_assetClass = getClass( "ShaderProgram" );
		doctorClass< ShaderProgramLoader >( [&]( ClassInfo& classInfo, ShaderProgramLoader& defaultObject )
							{
								DOCTOR_PROPERTY_ASSIGN( assetClass )
							} );

	}
	

	FRESH_DEFINE_CLASS( ShaderProgram )
	
	DEFINE_VAR( ShaderProgram, std::vector< Shader::ptr >, m_shaders );
	DEFINE_VAR( ShaderProgram, std::vector< int >, m_samplerIdsPerTextureUnit );
	DEFINE_VAR( ShaderProgram, std::vector< ShaderUniformUpdater::ptr >, m_uniformUpdaters );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( ShaderProgram )
	
	ShaderProgram::ShaderProgram( const ClassInfo& assignedClassInfo, NameRef objectName /* = DEFAULT_OBJECT_NAME */ )
	:	Super( assignedClassInfo, objectName )
	{
		m_shaders.reserve( 2 );
		m_idProgram = glCreateProgram();
	}
	
	ShaderProgram::ShaderProgram( const ClassInfo& assignedClassInfo, Shader* vertexShader, Shader* fragmentShader, NameRef objectName /* = DEFAULT_OBJECT_NAME */ )
	:	Super( assignedClassInfo, objectName )
	,	m_idProgram( 0 )
	,	m_nBoundAttributes( 0 )
	,	m_hasProgramLinked( false )
	{
		REQUIRES( vertexShader && vertexShader->isLoaded() );
		REQUIRES( fragmentShader && fragmentShader->isLoaded() );
		
		m_shaders.reserve( 2 );
		
		m_idProgram = glCreateProgram();
		ASSERT( m_idProgram );
		
		attachShader( vertexShader );
		attachShader( fragmentShader );
	}
	
	ShaderProgram::~ShaderProgram()
	{
		// Detach and clear shaders.
		//
		for( auto iter = m_shaders.begin(); iter != m_shaders.end(); ++iter )
		{
			ASSERT( *iter );
			glDetachShader( m_idProgram, (*iter)->getGLShaderId() );
		}
		m_shaders.clear();
		
		if( m_idProgram )
		{			
			// Cleanup program.
			//
			glDeleteProgram( m_idProgram );
			m_idProgram = 0;
		}
	}
	
	Shader::ptr ShaderProgram::createShader()
	{
		Shader::ptr result = createObject< Shader >();
		PROMISES( result );
		return result;
	}
	
	Shader::ptr ShaderProgram::createShader( const std::string& sourceCodePath, Shader::Type type )
	{
		REQUIRES( !sourceCodePath.empty() );

		Shader::ptr result = createObject< Shader >();
		result->loadSourceCodeFromFile( sourceCodePath, type );

		PROMISES( result );
		return result;
	}
	
	void ShaderProgram::attachShader( Shader::ptr shader )
	{
		REQUIRES( getNumAttachedShaders() < 2 );
		REQUIRES( !isLinked() );
		REQUIRES( shader && shader->isLoaded() );
		
		ASSERT( m_idProgram );
		
		glAttachShader( m_idProgram, shader->getGLShaderId() );
		HANDLE_GL_ERRORS();

		m_shaders.push_back( shader );
	}

	size_t ShaderProgram::getMaxAttributes() const
	{
		GLint maxAttributes = 0;
		glGetIntegerv( GL_MAX_VERTEX_ATTRIBS, &maxAttributes );
		return maxAttributes;
	}
	
	void ShaderProgram::bindAttribute( size_t iAttribute, const std::string& attributeName )
	{
		REQUIRES( !attributeName.empty() );
		REQUIRES( iAttribute <= getMaxAttributes() );
		REQUIRES( !isLinked() );
		
		ASSERT( m_idProgram );
		
		glBindAttribLocation( m_idProgram, static_cast< GLuint >( iAttribute ), attributeName.c_str() );
		
		HANDLE_GL_ERRORS();			
		
		++m_nBoundAttributes;
	}
	
	size_t ShaderProgram::getNumExpectedAttributes() const
	{
		ASSERT( m_idProgram );		
		
		GLint i;
		glGetProgramiv( m_idProgram, GL_ACTIVE_ATTRIBUTES, &i );
		
		return static_cast< size_t >( i );
	}
	
	VertexStructure::ptr ShaderProgram::getExpectedVertexStructure( NameRef vertexStructureName ) const
	{
		REQUIRES( isLinked() );
		ASSERT( m_idProgram );
		
		Name modifiedVertexStructureName( vertexStructureName );
		
		if( modifiedVertexStructureName.empty() )
		{
			// Invent a name.
			modifiedVertexStructureName = "VS_";
			modifiedVertexStructureName += name();
		}
		
		
		ASSERT( !Renderer::instance().getVertexStructure( modifiedVertexStructureName ) );		// Should be unkown.
		VertexStructure::ptr vertexStructure = Renderer::instance().createOrGetVertexStructure( modifiedVertexStructureName );
		
		const size_t nAttributes = getNumExpectedAttributes();
		for( size_t i = 0; i < nAttributes; ++i )
		{
			const GLsizei LEN = 256;
			GLchar attribName[ LEN ];
			GLint nComponents;
			GLenum type;
			glGetActiveAttrib( m_idProgram, static_cast< GLuint >( i ), LEN, 0, &nComponents, &type, attribName );
			
			// Translate compound types into simple types.
			//
			switch( type )
			{
				case GL_FLOAT_VEC2:
					nComponents = 2;
					type = GL_FLOAT;
					break;
				case GL_FLOAT_VEC3:
					nComponents = 3;
					type = GL_FLOAT;
					break;
				case GL_FLOAT_VEC4:
					nComponents = 4;
					type = GL_FLOAT;
					break;
				case GL_FLOAT_MAT2:
					nComponents = 2*2;
					type = GL_FLOAT;
					break;
				case GL_FLOAT_MAT3:
					nComponents = 3*3;
					type = GL_FLOAT;
					break;
				case GL_FLOAT_MAT4:
					nComponents = 4*4;
					type = GL_FLOAT;
					break;
			}
			
			vertexStructure->addAttribute( attribName, nComponents, Renderer::getTypeForGLType( type ), VertexStructure::Other );
		}
		
		return vertexStructure;
	}
	
	bool ShaderProgram::link()
	{
		REQUIRES( !isLinked() );
		ASSERT( m_idProgram );
		
#if DEV_MODE && 1
		// Warn about failure to bind attributes.
		if( m_nBoundAttributes == 0 )
		{
			trace( "WARNING: ShaderProgram::link() finds no attributes have been bound.\nAre you prepared to serve this program the vertex structure it expects?" );
		}
#endif
				  
		glLinkProgram( m_idProgram );
		
		traceProgramLog( "Program linking had log: " );
		
		GLint status;
		glGetProgramiv( m_idProgram, GL_LINK_STATUS, &status );
		m_hasProgramLinked = ( status != 0 );
		
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
		glReleaseShaderCompiler();
#endif
		
		HANDLE_GL_ERRORS();

		if( m_hasProgramLinked )
		{			
			// Validate the program.
			//			
			glValidateProgram( m_idProgram );				
			
			traceProgramLog( "Program validation had log: " );

			HANDLE_GL_ERRORS();
		}
		
		PROMISES( !m_hasProgramLinked || isLinked() );
		
		return m_hasProgramLinked;
	}
	
	std::string ShaderProgram::getErrorString( bool clearErrors /* = false */ )
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
	
	int ShaderProgram::getUniformId( const std::string& uniformName ) const
	{
		REQUIRES( !uniformName.empty() );
		REQUIRES( isLinked() );
		ASSERT( m_idProgram );
		
		// TODO cache these values rather than calling glGetUniformLocation(). For speed.
		
		return glGetUniformLocation( m_idProgram, uniformName.c_str() );
	}
	
	bool ShaderProgram::use()
	{
		REQUIRES( isLinked() );
		ASSERT( m_idProgram );
		
		if( m_idProgram != getCurrentProgramId() )
		{
			glUseProgram( m_idProgram );
		}
		
		HANDLE_GL_ERRORS();
		
		return true;
	}
	
	void ShaderProgram::traceProgramLog( const std::string& preambleMessage )
	{
		ASSERT( m_idProgram );

		GLint lenLog;
		glGetProgramiv( m_idProgram, GL_INFO_LOG_LENGTH, &lenLog );
		
		if( lenLog > 0 )
		{
			// Report errors or warnings.
			//
			std::unique_ptr< char[] > szLogText( new char[ lenLog + 1 ] );
			
			glGetProgramInfoLog( m_idProgram, lenLog, &lenLog, szLogText.get() );
			
			m_strErrorMessages += preambleMessage;
			m_strErrorMessages += szLogText.get();
		}			
	}
	
	void ShaderProgram::claimBoundUpdaters()
	{	
		// Make sure that contained updaters are actually bound to me.
		//
		std::for_each( m_uniformUpdaters.begin(), m_uniformUpdaters.end(), 
					   std::bind( &ShaderUniformUpdater::bindToProgram, std::placeholders::_1, std::ref( *this )));
		
		m_hasClaimedUpdaters = true;
	}
	
	unsigned int ShaderProgram::getCurrentProgramId()
	{
		GLint idCurrentProgram;
		glGetIntegerv( GL_CURRENT_PROGRAM, &idCurrentProgram ); 
		return (unsigned int) idCurrentProgram;
	}

	void ShaderProgram::setUniform( int idUniform, const int value ) const
	{
		glUniform1i( idUniform, value );
		HANDLE_GL_ERRORS();
	}
	
	void ShaderProgram::setUniform( int idUniform, const float value ) const
	{
		glUniform1f( idUniform, value );
		HANDLE_GL_ERRORS();
	}
	
	void ShaderProgram::setUniform( int idUniform, const Color value ) const
	{
		float r, g, b, a;
		value.getComponentsAsFloats( r, g, b, a );
		glUniform4f( idUniform, r, g, b, a );
		HANDLE_GL_ERRORS();
	}
	
	void ShaderProgram::setUniform( int idUniform, const vec2& value ) const
	{
		glUniform2f( idUniform, value.x, value.y );
		HANDLE_GL_ERRORS();
	}
	
	void ShaderProgram::setUniform( int idUniform, const vec3& value ) const
	{
		glUniform3f( idUniform, value.x, value.y, value.z );
		HANDLE_GL_ERRORS();
	}
	
	void ShaderProgram::setUniform( int idUniform, const vec4& value ) const
	{
		glUniform4f( idUniform, value.x, value.y, value.z, value.w );
		HANDLE_GL_ERRORS();
	}
	
	void ShaderProgram::setUniform( int idUniform, const mat4& value ) const
	{
		glUniformMatrix4fv( idUniform, 1, GL_FALSE, static_cast< const GLfloat* >( value ));
		HANDLE_GL_ERRORS();
	}
	
	/////////////////////////////////////////////////////////////////////////////////////
	
	FRESH_DEFINE_CLASS( ShaderProgramLoader )
	DEFINE_VAR( ShaderProgramLoader, std::string, m_vertexStructure );
	DEFINE_VAR( ShaderProgramLoader, std::string, m_vertexShaderSourcePath );
	DEFINE_VAR( ShaderProgramLoader, std::string, m_fragmentShaderSourcePath );
	DEFINE_VAR( ShaderProgramLoader, std::string, m_vertexShaderSourceText );
	DEFINE_VAR( ShaderProgramLoader, std::string, m_fragmentShaderSourceText );


	void ShaderProgramLoader::loadAsset( Asset::ptr asset )
	{
		Super::loadAsset( asset );
		
		// Actually have the shader program load its shaders.
		
		ShaderProgram::ptr shaderProgram = dynamic_freshptr_cast< ShaderProgram::ptr >( asset );
		ASSERT( shaderProgram );
		
		// Bind to attributes, if specified.
		//
		if( !m_vertexStructure.empty() )
		{
			VertexStructure::ptr vertexStructure = Renderer::instance().createOrGetVertexStructure( m_vertexStructure );
			ASSERT( vertexStructure );
			
			vertexStructure->bindProgramAttributes( *shaderProgram );
		}
		
		// Load source code files if requested.
		//
		createAndAttachShaderFromTextOrFile( shaderProgram, m_vertexShaderSourceText, m_vertexShaderSourcePath, Shader::VertexShader );
		createAndAttachShaderFromTextOrFile( shaderProgram, m_fragmentShaderSourceText, m_fragmentShaderSourcePath, Shader::FragmentShader );
		
		// Link if both shaders have loaded.
		//
		if( shaderProgram->getNumAttachedShaders() == 2 )
		{
			shaderProgram->link();

			if( !shaderProgram->isLinked() )
			{
				// Report error.
				trace( shaderProgram->toString() << " reported linking errors: " << shaderProgram->getErrorString() );
			}
		}
		
		HANDLE_GL_ERRORS();
	}
	
	void ShaderProgram::createAndLinkWithShaders( const std::string& vertexShaderSourceCodePath, const std::string& fragmentShaderSourceCodePath )
	{
		REQUIRES( !vertexShaderSourceCodePath.empty() );
		REQUIRES( !fragmentShaderSourceCodePath.empty() );
		
		attachShader( createShader( vertexShaderSourceCodePath, Shader::VertexShader ));
		attachShader( createShader( fragmentShaderSourceCodePath, Shader::FragmentShader ));
		
		link();		
	}
	
	void ShaderProgram::associateSamplerWithTextureUnit( const std::string& samplerName, const int iTextureUnit )
	{
		int iUniformLocation = getUniformId( samplerName );
		if( iUniformLocation < 0 )
		{
			trace( "ShaderProgram::associateSamplerWithTextureUnit( '" << samplerName << "', " << iTextureUnit << " ): couldn't find samplerName. Failing." );
			return;
		}
		
		m_samplerIdsPerTextureUnit.resize( std::max( m_samplerIdsPerTextureUnit.size(), static_cast< size_t >( iTextureUnit + 1 )));
		m_samplerIdsPerTextureUnit[ iTextureUnit ] = iUniformLocation;		
	}
		
	void ShaderProgram::updateBoundUniforms( Object::cptr host )
	{
		// Make sure updaters are ready.
		//
		if( !m_hasClaimedUpdaters )
		{
			claimBoundUpdaters();
		}
		
		for( size_t i = 0; i < m_uniformUpdaters.size(); ++i )
		{
			ASSERT( m_uniformUpdaters[ i ]->isBoundToProgram( this ) );
			
			m_uniformUpdaters[ i ]->updateUniform( host );
		}
		
		const int nSamplerIds = static_cast< int >( m_samplerIdsPerTextureUnit.size() );
		for( int i = 0; i < nSamplerIds; ++i )
		{
			setUniform( m_samplerIdsPerTextureUnit[ i ], i );
		}
	}
	
	size_t ShaderProgram::getMemorySize() const
	{
		return sizeof( *this );		// Not accurate, but that's okay. Just needs to be > 0.
	}
	
	ShaderProgram::ptr ShaderProgram::createFromSource( const std::string& vertexShaderSource, const std::string& fragmentShaderSource, VertexStructure::ptr vertexStructure )
	{
		REQUIRES( vertexShaderSource.empty() == false );
		REQUIRES( fragmentShaderSource.empty() == false );
		
		auto shaderProgram = createObject< ShaderProgram >();
		
		createAndAttachShaderFromTextOrFile( shaderProgram, vertexShaderSource, "", Shader::Type::VertexShader );
		createAndAttachShaderFromTextOrFile( shaderProgram, fragmentShaderSource, "", Shader::Type::FragmentShader );
		
		// Link if both shaders have loaded.
		//
		if( shaderProgram->getNumAttachedShaders() == 2 )
		{
			vertexStructure->bindProgramAttributes( *shaderProgram );
			
			shaderProgram->link();
			
			if( !shaderProgram->isLinked() )
			{
				// Report error.
				trace( shaderProgram->toString() << " reported linking errors: " << shaderProgram->getErrorString() );
			}
		}
		
		return shaderProgram;
	}
}
