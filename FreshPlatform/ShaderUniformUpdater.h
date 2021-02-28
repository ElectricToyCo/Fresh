/*
 *  ShaderUniformUpdater.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 12/22/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_SHADER_UNIFORM_UPDATER_H_INCLUDED
#define FRESH_SHADER_UNIFORM_UPDATER_H_INCLUDED

#include "Object.h"
#include "Objects.h"
#include "Property.h"
#include "Color.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4.h"

namespace fr
{
	class ShaderProgram;
	
	class ShaderUniformUpdater : public Object
	{
	public:
		
		bool isBoundToObject() const;
		// Returns true iff the updater has information about the object and property or method
		// from which to pull information from.
		
		bool isBoundToProgram( SmartPtr< const ShaderProgram > program = nullptr ) const;
		// Returns true iff the updater is bound to program, where program != 0.
		// If !program, returns true iff the updater is bound to *any* program.
		
		bool isBound( SmartPtr< const ShaderProgram > = nullptr ) const;
		// Returns true if isBoundToObject() and isBoundToProgram( program ).
		
		void bindUniformToObjectProperty( 
												 ShaderProgram& program, 
												 const std::string& programUniformName,
												 const ObjectId& objectId,
												 const std::string& objectPropName );
		// REQUIRES( !programUniformName.empty() );
		// REQUIRES( program.isLinked() );
		// REQUIRES( program.getUniformId( programUniformName ) >= 0 );
		// REQUIRES( objectId.IsValid() && !objectId.IsNull() );
		// REQUIRES( !objectPropName.empty() );
		// PROMISES( isBound( &program ) );

		void bindUniformToObjectMethod( 
												 ShaderProgram& program, 
												 const std::string& programUniformName,
												 const ObjectId& objectId,
												 const std::string& objectMethodName );
		// REQUIRES( !programUniformName.empty() );
		// REQUIRES( program.IsLinked() );
		// REQUIRES( program.getUniformId( programUniformName ) >= 0 );
		// REQUIRES( objectId.IsValid() && !objectId.IsNull() );
		// REQUIRES( !objectMethodName.empty() );
		// PROMISES( isBound( &program ) );
		
		virtual void updateUniform( Object::cptr host = nullptr ) = 0;
		// REQUIRES( isBoundToProgram() );
		
		void bindToProgram( ShaderProgram& program );
		// PROMISES( isBoundToProgram( &program ) );

		template< typename PropValueType >
		static ClassInfo::Name GetConcreteUpdaterTypeName();

		SYNTHESIZE_GET( std::string, uniformName );
		
	protected:
		
		VAR( std::string, m_uniformName );
		VAR( Object::wptr, m_boundObject );
		VAR( std::string, m_objectMemberName );
		DVAR( bool, m_isObjectMemberProperty, false );
		VAR( fr::WeakPtr< ShaderProgram >, m_shaderProgram );
		VAR( std::string, m_uniformValue );						// Only used if no property is specified
		
		int m_uniformId = -1;
		
		virtual void clearCachedValues() {}
		// Called when the bound object is changed.

		FRESH_DECLARE_CLASS_ABSTRACT( ShaderUniformUpdater, Object )
	};

	//////////////////////////////////////////////////////////////////////////////////////
	
	template< typename PropValueType >
	ClassInfo::Name ShaderUniformUpdater::GetConcreteUpdaterTypeName()
	{
		ASSERT( false );
		return "ShaderUniformUpdaterConcrete";
	}
	
#define DECLARE_SHADER_UNIFORM_UPDATER_TYPENAME( type )	\
	template<>	\
	inline ClassInfo::Name ShaderUniformUpdater::GetConcreteUpdaterTypeName< type >()	{ return STRINGIFY( ShaderUniformUpdaterConcrete##_##type ); }

	DECLARE_SHADER_UNIFORM_UPDATER_TYPENAME( float )
	DECLARE_SHADER_UNIFORM_UPDATER_TYPENAME( Color )
	DECLARE_SHADER_UNIFORM_UPDATER_TYPENAME( vec2 )
	DECLARE_SHADER_UNIFORM_UPDATER_TYPENAME( vec3 )
	DECLARE_SHADER_UNIFORM_UPDATER_TYPENAME( vec4 )
	DECLARE_SHADER_UNIFORM_UPDATER_TYPENAME( mat4 )

#undef DECLARE_SHADER_UNIFORM_UPDATER_TYPENAME
	
}

#endif

