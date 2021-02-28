/*
 *  Property.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 11/18/08.
 *  Copyright 2008 jeffwofford.com. All rights reserved.
 *
 */

#ifndef PROPERTY_H_INCLUDED
#define PROPERTY_H_INCLUDED

#include "PropertyAbstract.h"
#include "FreshDebug.h"
#include "Archive.h"
#include "TypeTraits.h"
#include "SmartPtr.h"
#include "ObjectStreamFormatter.h"
#include "StructSerialization.h"

namespace fr
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Property Type Information
	
	template< typename elementType >
	struct PropertyTypeTraits
	{
		static PropertyAbstract::ControlType controlType() { return PropertyAbstract::ControlType::String; }
		static ClassInfo::cptr referencedClass() { return nullptr; }
		static bool deep() { return false; }
	};
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Property classes 
	
	template< typename elementType >
	class Property : public PropertyAbstract
	{
	public:
		Property( const ClassInfo& originatingClass, const std::string& szPropName, size_t offset, unsigned int flags )
		:	PropertyAbstract( originatingClass, szPropName, offset, flags | ( TypeTraits< elementType >::shouldLoadDefaults() ? PropFlag::LoadDefault : 0 ))
		{}
		
		virtual std::unique_ptr< PropertyAbstract > createOverride( const ClassInfo& originatingClass, unsigned int flags = PropFlag::None ) const override;

		void setValue( Object* object, const elementType& value ) const;
		virtual void setValueByManifestValue( Object* object, const Manifest::Value& value ) const override;

		virtual std::string getValueByString( const Object* object ) const override;
		
		const elementType& getValue( const Object* object ) const;
		const elementType& getDefaultValue() const;
		
		virtual void saveToFormatter( class ObjectStreamFormatter& formatter, const Object* object ) const override;
		
		virtual bool deep() const override;
		
		virtual bool doesObjectHaveDefaultValue( const Object* object ) const override;
		virtual bool isEqual( const Object* a, const Object* b ) const override;
		
		virtual ControlType getControlType() const override;
		virtual const ClassInfo* getReferencedClass() const override;
		
	private:
				
		virtual void setValueByString( Object* object, const std::string& strValue ) const override;
				
		FRESH_PREVENT_COPYING( Property )
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Helper functions
	
	template< typename elementType, typename PropertyType >
	const elementType& getPropertyRef( const PropertyType& property, const Object* object )
	{
		ASSERT( object );
		
		const char* byteAddress = reinterpret_cast< const char* >( object );
		
		return *( reinterpret_cast< const elementType* >( byteAddress + property.getByteOffset() ));
	}

	template< typename elementType, typename PropertyType >
	elementType& getPropertyRef( const PropertyType& property, Object* object )
	{
		ASSERT( object );
		
		char* byteAddress = reinterpret_cast< char* >( object );
		
		return *( reinterpret_cast< elementType* >( byteAddress + property.getByteOffset() ));
	}
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	template< typename elementType >
	std::unique_ptr< PropertyAbstract > Property< elementType >::createOverride( const ClassInfo& originatingClass, unsigned int flags ) const
	{
		if( flags == PropFlag::None )	// Inherit from me.
		{
			flags = ( isEditable() ? 0 : PropFlag::NoEdit ) |
					( shouldLoadDefaults() ? PropFlag::LoadDefault : 0 ) |
					( isTransient() ? PropFlag::Transient : 0 );
		}
		
		flags |= PropFlag::Override;
		
		return std::unique_ptr< PropertyAbstract >( new Property< elementType >( originatingClass,
																				propName(),
																				getByteOffset(),
																				flags ));
	}
	
	template <typename elementType>
	void Property< elementType >::setValue( Object* object, const elementType& value ) const
	{
		getPropertyRef< elementType >( *this, object ) = value;
	}
	
	
	template <typename elementType>
	void Property< elementType >::setValueByString( Object* object, const std::string& strValue ) const
	{
		try
		{
			Destringifier ss( strValue );
			ss.propertyOwner( object );
			elementType& property = getPropertyRef< elementType >( *this, object );
			ss >> property;
			ss.propertyOwner( nullptr );
		}
		catch( const std::exception& e )
		{
			dev_error( "While parsing property '" << propName() << "' for " << object->className() << "'" << object->name() << "' with value '" << strValue << "', exception with message '" << e.what() << "'." );
		}
	}

	template< typename elementType >
	void Property< elementType >::setValueByManifestValue( Object* object, const Manifest::Value& value ) const
	{
		elementType& propertyRef = getPropertyRef< elementType >( *this, object );
		try
		{
			assignObjectProperty( propertyRef, value );
		}
		catch( const std::exception& e )
		{
			dev_error( "While parsing property '" << propName() << "' for " << object->className() << "'" << object->name() << "': " << e.what() );
		}
	}

	template <typename elementType>
	std::string Property< elementType >::getValueByString( const Object* object ) const
	{
		std::ostringstream ss;
		Stringifier stringifier( ss );
		stringifier << getPropertyRef< elementType >( *this, object );
		return ss.str();
	}
	
	template <typename elementType>
	const elementType& Property< elementType >::getValue( const Object* object ) const
	{
		return getPropertyRef< elementType >( *this, object );
	}
	
	template <typename elementType>
	bool Property< elementType >::deep() const
	{
		return PropertyTypeTraits< elementType >::deep();
	}
	
	template <typename elementType>
	PropertyAbstract::ControlType Property< elementType >::getControlType() const
	{
		return PropertyTypeTraits< elementType >::controlType();
	}
	
	template <typename elementType>
	const ClassInfo* Property< elementType >::getReferencedClass() const
	{
		return PropertyTypeTraits< elementType >::referencedClass();
	}
	
	template <typename elementType>
	const elementType& Property< elementType >::getDefaultValue() const
	{
		auto object = defaultObject();		// Would be null if I'm a property of an abstract class.
		ASSERT( object );
		return getValue( object );
	}

	template< typename elementType >
	void Property< elementType >::saveToFormatter( class ObjectStreamFormatter& formatter, const Object* object ) const
	{
		ASSERT( object );
		formatter.beginProperty( propName() );
		formatter.getPropertyStringifier() << getPropertyRef< elementType >( *this, object );
		formatter.endProperty( propName() );
	}
	
	template< typename elementType >
	bool Property< elementType >::doesObjectHaveDefaultValue( const Object* object ) const
	{
		return getValue( object ) == getDefaultValue();
	}
	
	template< typename elementType >
	bool Property< elementType >::isEqual( const Object* a, const Object* b ) const
	{
		return getValue( a ) == getValue( b );
	}
}

#define FRESH_OFFSET_OF( structure, member ) ((const char*) &( static_cast< const structure* >( nullptr )->member ) - static_cast< const char* >( nullptr ))

#define DEFINE_VAR( varHost, varType, varName )	struct varHost::PropInit_##varName varHost::propInit_##varName##_;	\
	varHost::PropInit_##varName::PropInit_##varName( const varType& propDefault, unsigned int flags )	\
	{	\
		static_assert( std::is_same< varType, property_t >::value, "VAR/DEFINE_VAR mismatch" );	\
		const size_t byteOffset = FRESH_OFFSET_OF( varHost, varName );	\
		varHost::StaticGetClassInfo().addProperty( std::unique_ptr< fr::PropertyAbstract >( new fr::Property< varType >( varHost::StaticGetClassInfo(), #varName, byteOffset, flags )));	\
	}

#define DEFINE_VAR_FLAG( varHost, varType, varName, userFlags )	struct varHost::PropInit_##varName varHost::propInit_##varName##_;	\
	varHost::PropInit_##varName::PropInit_##varName( const varType& propDefault, unsigned int flags )	\
	{	\
		static_assert( std::is_same< varType, property_t >::value, "VAR/DEFINE_VAR mismatch" );	\
		const size_t byteOffset = FRESH_OFFSET_OF( varHost, varName );	\
		varHost::StaticGetClassInfo().addProperty( std::unique_ptr< fr::PropertyAbstract >( new fr::Property< varType >( varHost::StaticGetClassInfo(), #varName, byteOffset, (userFlags) )));	\
	}

#endif
