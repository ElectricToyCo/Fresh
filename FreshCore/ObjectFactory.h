//
//  ObjectFactory.h
//  Fresh
//
//  Created by Jeff Wofford on 2/25/13.
//
//

#ifndef Fresh_ObjectFactory_h
#define Fresh_ObjectFactory_h

#include "FreshEssentials.h"
#include "SmartPtr.h"
#include "Profiler.h"

namespace fr
{
	
	class Object;
	class ClassInfo;
	
	// ObjectFactoryBase - the base class for all factories. It simply offers the createObject() interface.
	// This class is used exclusively by ObjectManager. There is no need for you to think about it.
	//
	class ObjectFactoryBase
	{
	public:
		virtual ~ObjectFactoryBase() {}
		
		virtual SmartPtr< Object > createObject( const ClassInfo& classInfo, ObjectNameRef objectName ) const = 0;
		virtual SmartPtr< Object > createInertObject( const ClassInfo& classInfo ) const = 0;

		virtual ObjectFactoryBase* createFactoryClone() const = 0;
	};
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	// ObjectFactory<T> - each specialization of this class knows how to create objects of type T, yet does so through a type-agnostic interface
	//		it has inherited from ObjectFactoryBase.
	// This class is used exclusively by ObjectManager. There is no need for you to think about it.
	//
	template< class T >
	class ObjectFactory : public ObjectFactoryBase
	{
	public:
		virtual SmartPtr< Object > createObject( const ClassInfo& classInfo, ObjectNameRef objectName ) const override;
		virtual SmartPtr< Object > createInertObject( const ClassInfo& classInfo ) const override;
		
		virtual ObjectFactoryBase* createFactoryClone() const override;
		
		SmartPtr< T > createObjectOfExactClass( const ClassInfo& classInfo, ObjectNameRef objectName ) const;

	};
		
	/////////////////////////////////////////////////////////////////////////////////
	
	template< class T >
	SmartPtr< Object > ObjectFactory<T>::createObject( const ClassInfo& classInfo, ObjectNameRef objectName ) const
	{
		return createObjectOfExactClass( classInfo, objectName );
	}

	template< class T >
	SmartPtr< Object > ObjectFactory<T>::createInertObject( const ClassInfo& classInfo ) const
	{
		SmartPtr< T > result = new T( CreateInertObject() );
		result->assignClassInfo( classInfo );
		return result;
	}	

	template< class T >
	SmartPtr< T > ObjectFactory< T >::createObjectOfExactClass( const ClassInfo& classInfo, ObjectNameRef objectName ) const
	{
		TIMER_AUTO( ObjectFactory< T >::createObjectOfExactClass )
		return new T( classInfo, objectName );
	}
	
	template< class T >
	ObjectFactoryBase* ObjectFactory<T>::createFactoryClone() const
	{
		return new ObjectFactory< T >();
	}

}

#endif
