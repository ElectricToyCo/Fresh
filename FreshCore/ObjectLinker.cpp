/*
 *  ObjectLinker.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 10/30/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#include "ObjectLinker.h"
#include "Objects.h"
#include "Asset.h"
#include "CommandProcessor.h"

namespace fr
{
	
	ObjectLinker::ObjectLinker() 
	: Singleton< ObjectLinker >( this ) 
	{}
	
	ObjectLinker::~ObjectLinker()
	{
		clearRemainingFixups();
	}
	
	bool ObjectLinker::fixupObjectAddresses()
	{
		ASSERT( !m_isTraversingVector );
		m_isTraversingVector = true;
		
		for( size_t i = 0; i < m_ptrFixups.size(); ++i )
		{
			PtrFixupGeneric*& fixup = m_ptrFixups[ i ];
			ASSERT( fixup );
			
			const ObjectId& objectId = fixup->objectId;
			ASSERT( objectId.isValid() );
			ASSERT( !objectId.isNull() );
			
			Object::ptr object = getObject( objectId );
			
			if( object )
			{
				fixup->assign( object );
				delete fixup;
				fixup = nullptr;
			}
		}
		
		// Clear null fixups.
		m_ptrFixups.erase( std::remove( m_ptrFixups.begin(), m_ptrFixups.end(), nullptr ), m_ptrFixups.end() );
		
		m_isTraversingVector = false;
		
		return !doFixupsRemain();
	}

	bool ObjectLinker::doFixupsRemain() const
	{
		return !m_ptrFixups.empty();	
	}
	
	void ObjectLinker::reportRemainingFixups() const
	{
		if( doFixupsRemain() )
		{
			ASSERT( !m_isTraversingVector );
			m_isTraversingVector = true;
			trace( "Remaining Fixups: =============================" );
			for( auto iter = m_ptrFixups.begin(); iter != m_ptrFixups.end(); ++iter )
			{
				auto fixup = (*iter );
				
				std::ostringstream message;
				message << "\t" << fixup->objectId;
				
				if( fixup->pointerOwner )
				{
					message << " referenced by ";

					const Object* owner = reinterpret_cast< const Object* >( fixup->pointerOwner );
					
					if( isObject( owner ))	// Really an Object?
					{
						message << owner;
					}
					else
					{
						message << "unknown owner at address " << std::hex << std::showbase << fixup->pointerOwner;
					}
				}
				
				trace( message.str() );
			}
			trace( "===============================================" );
			m_isTraversingVector = false;
		}
	}

	void ObjectLinker::clearRemainingFixups()
	{
		ASSERT( !m_isTraversingVector );
		m_isTraversingVector = true;
		for( auto ptrFixup : m_ptrFixups )
		{
			delete ptrFixup;
		}
		m_ptrFixups.clear();
		m_isTraversingVector = false;
	}
	
	void ObjectLinker::removeFixupsForOwner( void* pointerOwner )
	{
		REQUIRES( pointerOwner );
		ASSERT( !m_isTraversingVector );
		
		auto iterFirstToRemove = std::find_if( m_ptrFixups.begin(), m_ptrFixups.end(), [pointerOwner] ( const PtrFixupGeneric* fixup )
		{
			return fixup->pointerOwner == pointerOwner;
		} );
		
		m_ptrFixups.erase( iterFirstToRemove, m_ptrFixups.end() );
	}
	
}
