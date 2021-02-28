/*
 *  Shader.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 12/22/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_SHADERGL_H_INCLUDED
#define FRESH_SHADERGL_H_INCLUDED

#include "Object.h"
#include "Property.h"

namespace fr
{
	
	class Shader : public Object
	{
	public:
		
		enum Type
		{
			VertexShader,
			FragmentShader
		};
		
		;
		virtual ~Shader();
		
		bool hasSourceCode() const 							{ return !m_programSourceCode.empty(); }
		const std::string& getSourceCode() const			{ return m_programSourceCode; }
		Type getType() const								{ return m_type; }
		virtual bool isLoaded() const;
		
		virtual void loadSourceCodeFromText( const std::string& sourceCode, Type type );
		// REQUIRES( !sourceCode.empty() );
		// PROMISES( hasSourceCode() );
		virtual void loadSourceCodeFromFile( const std::string& filePath, Type type );
		// PROMISES( hasSourceCode() );
		
		virtual std::string getCompileErrors( bool clearErrors = false );
		// Returns empty string iff no errors.
		// Else returns a string describing errors.
		
		unsigned int getGLShaderId() const					{ return m_idShader; }

	protected:
		
		virtual void buildErrorMessages();		
		virtual void compileLoadedSourceCode();
		// REQUIRES( HasSourceCode() );
		// PROMISES( IsLoaded() || !GetCompileErrors().empty() );

	private:
		
		DVAR( Type, m_type, VertexShader );
		VAR( std::string, m_programSourceCode );
		
		unsigned int m_idShader = 0;
		bool m_isCompiled = false;
		std::string m_strErrorMessages;
		
		FRESH_DECLARE_CLASS( Shader, Object )
	};
	
	FRESH_ENUM_STREAM_IN_BEGIN( Shader, Type )
	FRESH_ENUM_STREAM_IN_CASE( Shader, VertexShader )
	FRESH_ENUM_STREAM_IN_CASE( Shader, FragmentShader )
	FRESH_ENUM_STREAM_IN_END()
	
	FRESH_ENUM_STREAM_OUT_BEGIN( Shader, Type )
	FRESH_ENUM_STREAM_OUT_CASE( Shader, VertexShader )
	FRESH_ENUM_STREAM_OUT_CASE( Shader, FragmentShader )
	FRESH_ENUM_STREAM_OUT_END()

}


#endif
