/*
 *  PropertyAbstract.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 10/30/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_PROPERTY_ABSTRACT_H_INCLUDED
#define FRESH_PROPERTY_ABSTRACT_H_INCLUDED

#include "FreshDebug.h"
#include "FreshEssentials.h"
#include "FreshManifest.h"
#include <type_traits>

namespace fr
{
	class Object;
	class ClassInfo;
	
	struct PropFlag
	{
		static const unsigned int None = 0;
		static const unsigned int NoEdit = 1 << 0;			// This property should not appear in editor property panels.
		static const unsigned int Override = 1 << 1;		// This property is an override of the same property in a base class.
		static const unsigned int LoadDefault = 1 << 2;		// Inert (default) objects *will* load this property from their configuration element(s).
		static const unsigned int Transient = 1 << 3;		// The property never saves or loads.
		static const unsigned int NormalNative = None;
	};
	
	class PropertyAbstract
	{
	public:
		
		enum class ControlType
		{
			String,
			Bool,
			Integer,
			Float,
			Color,
			Angle,
			ObjectRef,
			Container,
		};
				
		PropertyAbstract( const ClassInfo& originatingClass, const std::string& propName_, size_t offset, unsigned int flags )
		:	m_originatingClass( originatingClass )
		,	m_byteOffset( offset )
		,	m_flags( flags )
		{
			ASSERT( m_byteOffset >= 0 );
			
			// Get rid of "m_" prefix.
			//
			std::string modifiedPropName( propName_ );
			if( modifiedPropName.size() > 2 && modifiedPropName[ 0 ] == 'm' && modifiedPropName[ 1 ] == '_' )
			{
				modifiedPropName.erase( modifiedPropName.begin(), modifiedPropName.begin() + 2 );
			}
			
			m_propName = modifiedPropName;
		}
		virtual ~PropertyAbstract() {}
		
		virtual std::unique_ptr< PropertyAbstract > createOverride( const ClassInfo& originatingClass, unsigned int flags = PropFlag::None ) const = 0;
		// If flags == None, flags are preserved from original property, else they are overwritten. The Override flag is enforced regardless.
		
		unsigned int flags() const			{ return m_flags; }
		
		bool isEditable() const				{ return !( m_flags & PropFlag::NoEdit ); }
		bool isOverride() const				{ return  ( m_flags & PropFlag::Override ) != 0; }
		bool shouldLoadDefaults() const		{ return  ( m_flags & PropFlag::LoadDefault ) != 0; }
		bool isTransient() const			{ return  ( m_flags & PropFlag::Transient ) != 0; }
		virtual bool deep() const = 0;
		
		PropertyNameRef propName() const { return m_propName; }
		
		virtual void setValueByManifestValue( Object* object, const Manifest::Value& value ) const = 0;
		
		virtual std::string getValueByString( const Object* object ) const = 0;

		size_t getByteOffset() const { return m_byteOffset; }

		virtual void saveToFormatter( class ObjectStreamFormatter& formatter, const Object* object ) const = 0;

		void setDefaultValueByString( const std::string& strValue );
		std::string getDefaultValueByString() const;
		virtual bool doesObjectHaveDefaultValue( const Object* object ) const = 0;
		
		virtual bool isEqual( const Object* a, const Object* b ) const = 0;

		// Type metadata for use by editing and scripting contexts.
		//
		virtual ControlType getControlType() const = 0;
		virtual const ClassInfo* getReferencedClass() const = 0;
		
		friend class Object;

	protected:
		
		Object* defaultObject() const;
		
		virtual void setValueByString( Object* object, const std::string& strValue ) const = 0;
		
	private:
		
		const ClassInfo& m_originatingClass;
		PropertyName m_propName;
		size_t		m_byteOffset = -1;
		
		unsigned int m_flags = PropFlag::None;
		
		FRESH_PREVENT_COPYING( PropertyAbstract )
	};

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OBJECT PROPERTY DECLARATION
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define VAR( varType, varName )	\
\
varType varName;	\
static struct PropInit_##varName	\
{	\
	typedef varType property_t;		\
	PropInit_##varName( const property_t& propDefault = {}, unsigned int flags = fr::PropFlag::NormalNative );	\
} propInit_##varName##_;

// Declare member variable with default value.
#define DVAR( varType, varName, defaultValue )	\
\
varType varName = (defaultValue);	\
static struct PropInit_##varName	\
{	\
	typedef varType property_t;		\
	PropInit_##varName( const property_t& propDefault = (defaultValue), unsigned int flags = fr::PropFlag::NormalNative );	\
} propInit_##varName##_;

#endif
