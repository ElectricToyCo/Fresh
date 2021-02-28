/*
 *  ObjectLinker.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 10/30/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_OBJECT_LINKER_H_INCLUDED
#define FRESH_OBJECT_LINKER_H_INCLUDED

#include "ObjectId.h"
#include "ObjectPtr.h"
#include "Singleton.h"
#include "Objects.h"
#include <vector>

namespace fr
{	
	class Object;
	
	class ObjectLinker : public Singleton< ObjectLinker >
	{
	public:
		
		ObjectLinker();
		~ObjectLinker();
		
		bool fixupObjectAddresses();
		// Fixes up pending fixups. Returns true if all pending fixups were fixed, else false.
		
		bool doFixupsRemain() const;
		void reportRemainingFixups() const;
		void clearRemainingFixups();
		
		void removeFixupsForOwner( void* pointerOwner );
		// REQUIRES( pointerOwner );

		template< typename ObjectT >
		bool getObjectAddressOrAddForFixup( const ObjectId& objectId, void* pointerOwner, SmartPtr< ObjectT >& outObject );
		// REQUIRES( objectId.IsValid() );
		// REQUIRES( !objectId.IsNull() );

		template< typename ObjectT >
		bool getObjectAddressOrAddForFixup( const ObjectId& objectId, void* pointerOwner, WeakPtr< ObjectT >& outObject );
		// REQUIRES( objectId.IsValid() );
		// REQUIRES( !objectId.IsNull() );

	private:

		struct PtrFixupGeneric
		{
			ObjectId objectId;
			void* pointerOwner;
			
			PtrFixupGeneric( const ObjectId& id, void* owner ) : objectId( id ), pointerOwner( owner ) {}
			virtual ~PtrFixupGeneric() {}
			virtual void assign( Object::ptr object ) = 0;
		};
		
		template< typename ObjectPtrT >
		struct PtrFixup : public PtrFixupGeneric
		{
			PtrFixup( const ObjectId& id, void* owner, ObjectPtrT* ppObject ) : PtrFixupGeneric( id, owner ), m_pointer( ppObject ) {}
			
			virtual void assign( Object::ptr object ) override;
			
		private:
			
			ObjectPtrT* m_pointer;
		};
		
		typedef std::vector< PtrFixupGeneric* > PtrFixups;
		PtrFixups m_ptrFixups;
		mutable bool m_isTraversingVector = false;
		
	};

	template< typename ObjectPtrT >
	void ObjectLinker::PtrFixup< ObjectPtrT >::assign( Object::ptr object )
	{
		ASSERT( object );
		*m_pointer = static_freshptr_cast< ObjectPtrT >( object );
	}
	
	template< typename ObjectT >
	inline bool ObjectLinker::getObjectAddressOrAddForFixup( const ObjectId& objectId, void* pointerOwner, SmartPtr< ObjectT >& outObject )
	{
		REQUIRES( objectId.isValid() );
		REQUIRES( !objectId.isNull() );
		
		outObject = getObject< ObjectT >( objectId );
		if( !outObject )
		{
			m_ptrFixups.push_back( new PtrFixup< SmartPtr< ObjectT > >( objectId, pointerOwner, &outObject ));
		}
		return (bool) outObject;
	}
	
	template< typename ObjectT >
	inline bool ObjectLinker::getObjectAddressOrAddForFixup( const ObjectId& objectId, void* pointerOwner, WeakPtr< ObjectT >& outObject )
	{
		REQUIRES( objectId.isValid() );
		REQUIRES( !objectId.isNull() );
		
		outObject = getObject< ObjectT >( objectId );
		if( !outObject )
		{
			m_ptrFixups.push_back( new PtrFixup< WeakPtr< ObjectT > >( objectId, pointerOwner, &outObject ));
		}
		return (bool) outObject;
	}

}

#endif
