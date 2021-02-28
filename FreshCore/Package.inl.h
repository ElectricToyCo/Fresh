//
//  Package.inl.h
//  Fresh
//
//  Created by Jeff Wofford on 2/27/13.
//
//

#ifndef Fresh_Package_inl_h
#define Fresh_Package_inl_h

#include "Classes.h"
#include "FreshException.h"

namespace fr
{
	template< typename member_t >
	void Package::forEachMemberOfType( std::function< void( const SmartPtr< member_t >& ) >&& fn ) const
	{
		forEachMember( [&fn] ( const Object::ptr& member )
								{
									SmartPtr< member_t > cast = member->as< member_t >();
									if( cast )
									{
										fn( cast );
									}
								} );
	}
	
	//////////////////////////////////////////////////////////////////////////////////
	// FIND

	template< typename return_t >
	SmartPtr< return_t > Package::find( Object::NameRef objectName ) const
	{
		REQUIRES( objectName.empty() == false );
		return find< return_t >( return_t::StaticGetClassInfo(), objectName );
	}

	template< typename return_t >
	SmartPtr< return_t > Package::find( const ClassInfo& classInfo, Object::NameRef objectName ) const
	{
		REQUIRES( objectName.empty() == false );
		REQUIRES( classInfo.isKindOf( return_t::StaticGetClassInfo() ));
		return dynamic_freshptr_cast< SmartPtr< return_t > >( findGeneric( classInfo, objectName ));	// Calling the virtual, untemplated version.
	}
	
	template< typename return_t >
	SmartPtr< return_t > Package::find( const ObjectId& id ) const
	{
		ASSERT( id.packageName().empty() || id.packageName() == name() );		// Better specify this same package or none at all.
		ClassInfo::ptr classInfo = getClass( id.className() );
		if( !classInfo )
		{
			FRESH_THROW( FreshException, "Unrecognized class " << id.className() << "." );
		}
		return find< return_t >( *classInfo, id.objectName() );
	}

	template< typename return_t >
	std::vector< SmartPtr< return_t >> Package::findFiltered( const ObjectId& objectFilter ) const
	{
		std::vector< SmartPtr< return_t >> results;
		std::copy_if( m_members.begin(), m_members.end(), std::back_inserter( results ), [ & objectFilter ]
					 ( const typename decltype( m_members )::value_type& member )
					 {
						 return member && member->matchesFilters( objectFilter.className(), objectFilter.objectName() );
					 } );
		return results;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// REQUEST
	
	template< typename return_t >
	SmartPtr< return_t > Package::request( Object::NameRef objectName )
	{
		REQUIRES( objectName.empty() == false );
		return request< return_t >( return_t::StaticGetClassInfo(), objectName );
	}
	
	template< typename return_t >
	SmartPtr< return_t > Package::request( const ClassInfo& classInfo, Object::NameRef objectName )
	{
		REQUIRES( objectName.empty() == false );
		REQUIRES( classInfo.isKindOf( return_t::StaticGetClassInfo() ));
		return dynamic_freshptr_cast< SmartPtr< return_t > >( requestGeneric( classInfo, objectName ));	// Calling the virtual, untemplated version.
	}
	
	template< typename return_t >
	SmartPtr< return_t > Package::request( const ObjectId& id )
	{
		ASSERT( id.packageName().empty() || id.packageName() == name() );		// Better specify this same package or none at all.
		ClassInfo::ptr classInfo = getClass( id.className() );
		if( !classInfo )
		{
			FRESH_THROW( FreshException, "Unrecognized class " << id.className() << "." );
		}
		return request< return_t >( *classInfo, id.objectName() );
	}
	

	//////////////////////////////////////////////////////////////////////////////////
	// ADD
	
	template< typename ForwardIteratorT >
	void Package::add( ForwardIteratorT begin, ForwardIteratorT end )
	{
		std::for_each( begin, end, [this] ( Object::ptr p ) { add( p ); } );
		tidy();
	}	

	//////////////////////////////////////////////////////////////////////////////////
	// MERGE
	
	template< typename ForwardIteratorT >
	void Package::merge( ForwardIteratorT begin, ForwardIteratorT end, MergePolicy policy )
	{
		std::for_each( begin, end, [this, &policy] ( Object::ptr p ) { merge( p, policy ); } );
		tidy();
	}
}

#endif
