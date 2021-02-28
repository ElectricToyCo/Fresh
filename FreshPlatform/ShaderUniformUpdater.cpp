/*
 *  ShaderUniformUpdater.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 12/22/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#include "ShaderUniformUpdater.h"
#include "Objects.h"
#include "ShaderProgram.h"

namespace fr
{

	DEFINE_VAR( ShaderUniformUpdater, std::string, m_uniformName );
	DEFINE_VAR( ShaderUniformUpdater, Object::wptr, m_boundObject );
	DEFINE_VAR( ShaderUniformUpdater, std::string, m_objectMemberName );
	DEFINE_VAR( ShaderUniformUpdater, bool, m_isObjectMemberProperty );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( ShaderUniformUpdater )
	
	bool ShaderUniformUpdater::isBoundToObject() const
	{
		return m_boundObject && !m_objectMemberName.empty() && !m_uniformName.empty();
	}
	
	bool ShaderUniformUpdater::isBoundToProgram( ShaderProgram::cptr program ) const
	{
		return ( !program && m_shaderProgram ) || ( program && m_shaderProgram == program );
	}
	
	bool ShaderUniformUpdater::isBound( ShaderProgram::cptr program ) const
	{
		return isBoundToObject() && isBoundToProgram( program );
	}
	
	void ShaderUniformUpdater::bindUniformToObjectProperty( 
											 ShaderProgram& program, 
											 const std::string& programUniformName,
											 const ObjectId& objectId,
											 const std::string& objectPropName )
	{
		REQUIRES( !programUniformName.empty() );
		REQUIRES( program.isLinked() );
		REQUIRES( objectId.isValid() && !objectId.isNull() );
		REQUIRES( !objectPropName.empty() );

		m_uniformName = programUniformName;
		
		m_boundObject = getObject( objectId );
		ASSERT( m_boundObject );
		
		m_objectMemberName = objectPropName;
		m_isObjectMemberProperty = true;
		
		bindToProgram( program );
		
		clearCachedValues();
		
		PROMISES( isBound( &program ) );
	}

	void ShaderUniformUpdater::bindUniformToObjectMethod( 
														   ShaderProgram& program, 
														   const std::string& programUniformName,
														   const ObjectId& objectId,
														   const std::string& objectMethodName )
	{
		REQUIRES( !programUniformName.empty() );
		REQUIRES( program.isLinked() );
		REQUIRES( objectId.isValid() && !objectId.isNull() );
		REQUIRES( !objectMethodName.empty() );
		
		m_uniformName = programUniformName;
		m_boundObject = getObject( objectId );
		ASSERT( m_boundObject );
		
		m_objectMemberName = objectMethodName;
		m_isObjectMemberProperty = false;
		
		bindToProgram( program );
		
		clearCachedValues();
		
		PROMISES( isBound( &program ) );
	}
	
	void ShaderUniformUpdater::bindToProgram( ShaderProgram& program )
	{
		m_uniformId = program.getUniformId( m_uniformName );
		if( m_uniformId < 0 )
		{
			dev_warning( this << " had unrecognized uniform '" << m_uniformName << "' for program " << &program );
		}
		
		m_shaderProgram = &program;
		
		PROMISES( isBoundToProgram( &program ) );
	}

}
