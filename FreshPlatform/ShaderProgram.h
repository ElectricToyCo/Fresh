/*
 *  ShaderProgram.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 12/22/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_SHADER_PROGRAM_GL_H_INCLUDED_
#define FRESH_SHADER_PROGRAM_GL_H_INCLUDED_

#include "Objects.h"
#include "Property.h"
#include "Asset.h"
#include "Color.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4.h"
#include "Shader.h"
#include "ShaderUniformUpdater.h"

namespace fr
{
	/*
	 
	 A Note on Uniform Binding.
	 
	 One of the major types of input to a shader program is "uniforms"--global variables
	 that feed directly from the host CPU application into the vertex and fragment shaders.
	 Uniforms are essential to the functioning of a shader program but are also entirely
	 idiosyncratic. Therefore user code for a given shader program must have an intimate
	 knowledge of uniforms in order to correctly feed a value for each uniform prior to
	 the execution of the program for each Renderer::drawGeometry() call.
	 
	 One way for user code to do this is to explicitly call setUniform() for each 
	 uniform in question. This is a perfectly sensible way to set uniform values.
	 
	 Calling setUniform() from user code, however, requires that the user code be hand-
	 coded. That is, this is a code-driven, not data-driven solution. A data-driven 
	 solution would allow a data file to determine what the value of each uniform is.
	 
	 To do so in a static way is trivial. That is, the data file can very easily provide
	 literal values that the uniforms are set to. This is inflexible, however.
	 
	 A superior system is to allow the data file to determine not only the literal value of 
	 the uniform, but instead a function call or object property to read the value
	 from immediately before each Renderer::drawGeometry() call (or at some other
	 interval).
	 
	 In other words, I want a data-driven way of "binding" a uniform variable to
	 something else in the Fresh system: either an object property or an object
	 method call.
	 
	 This is accomplished through the bindUniformTo*() methods. 
	 
	 bindUniformToObjectProperty() binds a uniform of a given type (indicated statically
	 via template parameter) to a named property within a specific named object. So long
	 as the object exists (it is stored as a weak pointer), the property of the given
	 name will be queried for the value that is to be assigned to the uniform. If the 
	 property's and uniform's type are mismatched, this results in a runtime exception
	 (currently an ASSERT).
	 
	 bindUniformToObjectMethod() binds a uniform of a given type to a named simple 
	 accessor within a given object. A simple accessor is a const method that takes
	 no arguments and returns a result of a given type. The uniform type and accessor
	 return type must match.	 
	 
	 */
	
	class VertexStructure;
	
	class ShaderProgram : public Asset
	{
		FRESH_DECLARE_CLASS( ShaderProgram, Asset )
	public:
		
		ShaderProgram( const ClassInfo& assignedClassInfo, Shader* vertexShader, Shader* fragmentShader, NameRef objectName = DEFAULT_OBJECT_NAME );
		
		virtual ~ShaderProgram();
		
		virtual Shader::ptr createShader();
		// PROMISES( result );
		virtual Shader::ptr createShader( const std::string& sourceCodePath, Shader::Type type );
		// REQUIRES( !sourceCodePath.empty() );
		// PROMISES( result );
		// Be sure to check result->isLoaded() and, if false result->getCompileErrors(true).
		
		virtual bool isLinked() const { return m_hasProgramLinked; }
		
		virtual size_t getNumAttachedShaders() const { return m_shaders.size(); }
		virtual void attachShader( Shader::ptr shader );
		// REQUIRES( GetNumAttachedShaders() < 2 );
		// REQUIRES( !IsLinked() );
		// REQUIRES( shader && shader->IsLoaded() );
		
		virtual size_t getMaxAttributes() const;
		virtual void bindAttribute( size_t iAttribute, const std::string& attributeName );
		// REQUIRES( !attributeName.empty() );
		// REQUIRES( iAttribute <= GetMaxAttributes() );
		// REQUIRES( !IsLinked() );
		
		virtual size_t getNumExpectedAttributes() const;
		// REQUIRES( isLinked() );
		virtual SmartPtr< VertexStructure > getExpectedVertexStructure( NameRef vertexStructureName = DEFAULT_OBJECT_NAME ) const;
		// REQUIRES( isLinked() );
		
		virtual bool link();
		// Returns false iff link fails. Call GetErrorString() for details.
		// REQUIRES( !IsLinked() );
		// PROMISES( !result || IsLinked() );
		
		virtual std::string getErrorString( bool clearErrors = false );
		// Returns an error message associated with the latest errorful call to
		// Link() or Use().
		// Returns empty string if no errors.
		
		virtual int getUniformId( const std::string& uniformName ) const;
		// REQUIRES( !uniformName.empty() );
		// REQUIRES( IsLinked() );
		// Returns < 0 if uniformName is unrecognized.
	
		virtual void setUniform( int idUniform, const int value ) const;
		virtual void setUniform( int idUniform, const float value ) const;
		virtual void setUniform( int idUniform, const Color value ) const;
		virtual void setUniform( int idUniform, const vec2& value ) const;
		virtual void setUniform( int idUniform, const vec3& value ) const;
		virtual void setUniform( int idUniform, const vec4& value ) const;
		virtual void setUniform( int idUniform, const mat4& value ) const;
		
		virtual bool use();
		// REQUIRES( IsLinked() );
		// Returns true iff no errors. If errors, call GetErrorString() for details.
		
		virtual void createAndLinkWithShaders( const std::string& vertexShaderSourceCodePath, const std::string& fragmentShaderSourceCodePath );
		
		virtual void associateSamplerWithTextureUnit( const std::string& samplerName, const int iTextureUnit );
		// REQUIRES( !uniformName.empty() );
		// REQUIRES( IsLinked() );
		
		template< typename PropValueType >
		void bindUniformToObjectProperty( const std::string& programUniformName,
										 const ObjectId& objectId,
										 const std::string& objectPropName );
		// REQUIRES( !programUniformName.empty() );
		// REQUIRES( IsLinked() );
		// REQUIRES( getUniformId( programUniformName ) >= 0 );
		// REQUIRES( objectId.IsValid() && !objectId.IsNull() );
		// REQUIRES( !objectPropName.empty() );
		
		template< typename MethodReturnValueType >
		void bindUniformToObjectMethod( const std::string& programUniformName,
									   const ObjectId& objectId,
									   const std::string& objectMethodName );
		// REQUIRES( !programUniformName.empty() );
		// REQUIRES( IsLinked() );
		// REQUIRES( getUniformId( programUniformName ) >= 0 );
		// REQUIRES( objectId.IsValid() && !objectId.IsNull() );
		// REQUIRES( !objectMethodName.empty() );
		
		virtual void updateBoundUniforms( Object::cptr host = nullptr );
		
		// Inherited from Asset.
		//
		virtual size_t getMemorySize() const override;
		
		void claimBoundUpdaters();
		// Necessary after a load, because updaters will be complete except that they 
		// might not know which program to use.
		
		static ShaderProgram::ptr createFromSource( const std::string& vertexShaderSource,
												    const std::string& fragmentShaderSource,
												    SmartPtr< VertexStructure > vertexStructure );
		// REQUIRES( vertexShaderSource.empty() == false );
		// REQUIRES( fragmentShaderSource.empty() == false );

	protected:
		
		static unsigned int getCurrentProgramId();
		
		void traceProgramLog( const std::string& preambleMessage );
		
		
	private:
		
		VAR( std::vector< Shader::ptr >, m_shaders );
		VAR( std::vector< int >, m_samplerIdsPerTextureUnit );		

		VAR( std::vector< ShaderUniformUpdater::ptr >, m_uniformUpdaters );
		
		std::string m_strErrorMessages;
		
		unsigned int m_idProgram = 0;
		size_t	m_nBoundAttributes = 0;
		bool	m_hasProgramLinked = false;
		bool	m_hasClaimedUpdaters = false;
	};
	
	////////////////////////////////////////////////////////////////////////////////////////
	
	class ShaderProgramLoader : public AssetLoader_Default
	{
	public:
		
		virtual void loadAsset( Asset::ptr asset ) override;
		
	private:
		
		VAR( std::string, m_vertexStructure );
		VAR( std::string, m_vertexShaderSourcePath );
		VAR( std::string, m_fragmentShaderSourcePath );
		VAR( std::string, m_vertexShaderSourceText );
		VAR( std::string, m_fragmentShaderSourceText );
		
		FRESH_DECLARE_CLASS( ShaderProgramLoader, AssetLoader_Default )
	};
	


	////////////////////////////////////////////////////////////////////////////////////////

	template< typename PropValueType >
	void ShaderProgram::bindUniformToObjectProperty( 
													const std::string& programUniformName,
													const ObjectId& objectId,
													const std::string& objectPropName )
	{
		REQUIRES( !programUniformName.empty() );
		REQUIRES( isLinked() );		
		REQUIRES( objectId.isValid() && !objectId.isNull() );
		
		if( getUniformId( programUniformName ) < 0 )
		{
			dev_warning( "Could not find shader program uniform named " << programUniformName << ". Bind failing." );
			return;
		}
		
		ClassInfo::NameRef className = ShaderUniformUpdater::GetConcreteUpdaterTypeName< PropValueType >();
		
		ShaderUniformUpdater::ptr updater = createObject< ShaderUniformUpdater >( className );
		ASSERT( updater );
		
		updater->bindUniformToObjectProperty( *this, programUniformName, objectId, objectPropName );
		
		m_uniformUpdaters.push_back( updater );
	}

	template< typename MethodReturnValueType >
	void ShaderProgram::bindUniformToObjectMethod( const std::string& programUniformName,
												  const ObjectId& objectId,
												  const std::string& objectMethodName )
	{
		REQUIRES( !programUniformName.empty() );
		REQUIRES( isLinked() );
		REQUIRES( objectId.isValid() && !objectId.isNull() );
		REQUIRES( !objectMethodName.empty() );
		
		if( getUniformId( programUniformName ) < 0 )
		{
			dev_warning( "Could not find shader program uniform named " << programUniformName << ". Bind failing." );
			return;
		}
		
		ClassInfo::NameRef className = ShaderUniformUpdater::GetConcreteUpdaterTypeName< MethodReturnValueType >();
		ClassInfo::cptr classInfo = getClass( className );
		ASSERT( classInfo );
		
		ShaderUniformUpdater::ptr updater = createObject< ShaderUniformUpdater >( *classInfo );
		ASSERT( updater );
		
		updater->bindUniformToObjectMethod( *this, programUniformName, objectId, objectMethodName );
		
		m_uniformUpdaters.push_back( updater );
	}
}

#endif

