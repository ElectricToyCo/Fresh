/*
 *  Object.inl.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/30/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#include <sstream>
#include <algorithm>

namespace fr
{
	template< typename class_t >
	inline ClassInitializer< class_t >::ClassInitializer()
	{
#ifdef FRESH_VERBOSE_REFLECTION
		dev_trace( "Initializing class " << class_t::StaticGetClassInfo().className() );
#endif
		
		// Invoke StaticGetClassInfo() for this class so that the class info and associated
		// factory and default object are created.
		//
		class_t::StaticGetClassInfo();
	}

	/////////////////////////////////////////////
	
	ALWAYS_INLINE void Object::addReference() const
	{
		++m_nReferences;
		ASSERT( m_nReferences > 0 );
	}

	inline std::ostream& operator<<( std::ostream& out, const Object* obj )
	{
		if( obj )
		{
			out << obj->toString();
		}
		else
		{
			out << "'null'";
		}
		return out;
	}

	template< class ObjectT >
	inline std::ostream& operator<<( std::ostream& out, const SmartPtr< ObjectT >& obj )
	{
		return operator<<( out, static_cast< const Object* >( obj.get() ));
	}
	
	template< class ObjectT >
	inline std::ostream& operator<<( std::ostream& out, const WeakPtr< ObjectT >& obj )
	{
		return operator<<( out, static_cast< const Object* >( obj.get() ));
	}
	
	template< typename DerivedT >
	inline SmartPtr< DerivedT > Object::as()
	{
		if( isA( DerivedT::StaticGetClassInfo() ))
		{
			return SmartPtr< DerivedT >( static_cast< DerivedT* >( this ));
		}
		else
		{
			return nullptr;
		}
	}
	
	template< typename DerivedT >
	inline SmartPtr< const DerivedT > Object::as() const
	{
		if( isA( DerivedT::StaticGetClassInfo() ))
		{
			return SmartPtr< const DerivedT >( static_cast< const DerivedT* >( this ));
		}
		else
		{
			return nullptr;
		}
	}
	
}
