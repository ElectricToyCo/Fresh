//
//  Package.h
//  Fresh
//
//  Created by Jeff Wofford on 2/12/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_Package_h
#define Fresh_Package_h

#include "SwitchPtr.h"
#include "Object.h"
#include "FreshPath.h"
#include <list>

namespace fr
{
	
	class Package : public Object
	{
		FRESH_DECLARE_CLASS( Package, Object )
	public:
		
		void save( const path& fullPath, bool forceSaveAllProperties = false ) const;
		virtual void save( std::ostream& out, bool forceSaveAllProperties = false ) const;

		std::vector< Object::ptr > loadFile( const path& fullPath );
		// REQUIRES( exists( fullPath ));
		
		virtual std::vector< Object::ptr > loadFromManifest( const Manifest& manifest );

		bool empty() const;
		size_t size() const;

		//
		// OBJECT ACCESS
		//

		// has() returns true if object is in the package.
		//
		bool has( Object::cptr object ) const;
		// REQUIRES( object );

		void forEachMember( std::function< void( const Object::ptr& ) >&& fn ) const;

		template< typename member_t >
		void forEachMemberOfType( std::function< void( const SmartPtr< member_t >& ) >&& fn ) const;

		// FINDING
		//
		// find() returns the owned object that matches the given class and object name.
		//
		template< typename return_t = Object >
		SmartPtr< return_t > find( NameRef objectName ) const;
		// REQUIRES( objectName.empty() == false );
		// Returns nullptr iff !has( object );

		template< typename return_t = Object >
		SmartPtr< return_t > find( const  ClassInfo& classInfo, NameRef objectName ) const;
		// REQUIRES( objectName.empty() == false );
		// REQUIRES( classInfo.isKindOf( return_t::StaticGetClassInfo() );
		// Returns nullptr iff !has( object );

		template< typename return_t = Object >
		SmartPtr< return_t > find( const ObjectId& id ) const;
		// Returns nullptr iff !has( object );
		
		template< typename return_t = Object >
		std::vector< SmartPtr< return_t >> findFiltered( const ObjectId& objectFilter ) const;
		
		// REQUESTING
		//
		// request() works like find, but may optionally (at the package's discretion)
		// create and add the object if it is not found.
		//
		template< typename return_t = Object >
		SmartPtr< return_t > request( NameRef objectName );
		// REQUIRES( objectName.empty() == false );
		// Returns nullptr iff !has( object );
		
		template< typename return_t = Object >
		SmartPtr< return_t > request( const  ClassInfo& classInfo, NameRef objectName );
		// REQUIRES( objectName.empty() == false );
		// REQUIRES( classInfo.isKindOf( return_t::StaticGetClassInfo() );
		// Returns nullptr iff !has( object );
		
		template< typename return_t = Object >
		SmartPtr< return_t > request( const ObjectId& id );
		
		// ADDING
		//
		void add( Object::ptr object );
		// REQUIRES( object );
		// REQUIRES( !has( object ));
		// REQUIRES( object != this );
		// REQUIRES( !find( object->objectId() ));
		// PROMISES( hasObject( object ));
		
		// Adds a series of objects.
		//
		template< typename ForwardIteratorT >
		void add( ForwardIteratorT begin, ForwardIteratorT end );

		// MERGING
		//
		// merge() adds a new object to the package after first ensuring
		// that no other object with the same class and object name exists.
		// 
		enum class MergePolicy
		{
			IgnoreNew,
			ReplaceExisting,
			KeepBothRenamingNew,
		};
		void merge( Object::ptr object, MergePolicy policy );
		
		// Merges a series of objects.
		//
		template< typename ForwardIteratorT >
		void merge( ForwardIteratorT begin, ForwardIteratorT end, MergePolicy policy );
		
		// Merges all the members of another package.
		//
		void merge( const Package& other, MergePolicy policy );
		
		// REMOVING
		//
		void remove( Object::ptr object );
		// REQUIRES( object );
		// REQUIRES( hasObject( object ));
		// PROMISES( !hasObject( object ));

		// OTHER
		//
		// collect() adds objects that fit the filter that are referenced by the members
		// (including new members that are added during the collection process).
		//
		void collect( std::function< bool( const Object& ) >&& filter
					 = [] ( const Object& ) { return true; } );

		
		bool isLoading() const				{ return m_isLoading; }
		
		//
		// OBJECT RETENTION
		//
		
		bool areMembersRetained() const;
		
		// Set whether to operate in "retained" or "unretained" mode.
		//
		void retainMembers();
		// PROMISES( areObjectsRetained() );
		void releaseMembers();
		// PROMISES( !areObjectsRetained() );
		
		// In "retained" mode, releases objects that are unreferenced except by this class, so that at least
		// minBytesToReduce bytes of memory are recovered (if this can be measured).
		// If minBytesToReduce == 0, releases all unreferenced objects.
		virtual void releaseRetainedZombies( size_t minBytesToReduce = 0 );
		// REQUIRES( areObjectsRetained() );
		
		size_t numNullMembers() const;	// A measure of "slop" in the package. tidy() reduces this to 0.
		
		//
		// OTHER
		//
		ObjectName getUniqueName( const ClassInfo& classInfo, ObjectNameRef name ) const;
		// REQUIRES( name.empty() == false );
		
	protected:
		
		virtual Object::ptr findGeneric( const ClassInfo& classInfo, NameRef objectName ) const;
		// REQUIRES( objectName.empty() == false );
		
		virtual Object::ptr requestGeneric( const ClassInfo& classInfo, NameRef objectName );
		// REQUIRES( objectName.empty() == false );
		
		virtual void writeRootElement( std::ostream& out ) const;
		
		void tidy() const;	// Gets rid of null elements in m_objects, generally due to WeakPtrs being released.
		
		// Like tidy, but limits changes to a handful per call.
		// This is fast and separated so that with enough calls, quickTidy() is eventually equivalent to tidy().
		// Note: quickTidy() may be equivalent to tidy().
		//
		void quickTidy() const;
		
		virtual size_t quickTidySize() const;
		
		virtual void sortZombiesForReleasePriority( std::vector< Object::ptr >::iterator begin, std::vector< Object::ptr >::iterator end );
		virtual size_t getZombieSizeBytes( const Object& object ) const;
		
	private:
		
		bool m_isRetained = false;
		bool m_isLoading = false;
		
		std::list< SwitchPtr< Object > > m_members;
		
		mutable size_t m_nextQuickTidyBegin = 0;
	};

	///////////////////////////////////////////////////////////////
	// ACTIVE PACKAGE MANAGEMENT
	//
	Package::ptr activePackage();
	void pushActivePackage( Package::ptr package );
	Package::ptr popActivePackage();

	/////////////////////////////////////////
	// OBJECT FIXUP CAPTURER
	//
	
	// ObjectFixupCapturer is used to ensure that object pointers are fixed up
	// after all objects are done being created. Works in coordination with ObjectLinker.
	//
	struct ObjectFixupCapturer
	{
		explicit ObjectFixupCapturer( const std::string& message, bool requireFixup = false );
		~ObjectFixupCapturer();

	private:
		std::string m_message;
		Package::ptr m_tempPackage;
		bool m_requireFixup = false;
	};
	
	struct ObjectFixupSuppressor
	{
		ObjectFixupSuppressor();
		~ObjectFixupSuppressor();
	};
	
#	define CAPTURE_FIXUP_FOR_OBJECT( message ) std::ostringstream capturerMessage_; capturerMessage_ << message; ObjectFixupCapturer objectFixupCapturer_( capturerMessage_.str() );
#	define CAPTURE_FIXUP_FOR_PACKAGE( message ) std::ostringstream capturerMessage_; capturerMessage_ << message; ObjectFixupCapturer objectFixupCapturer_( capturerMessage_.str(), true );
#	define SUPPRESS_FIXUP_CAPTURE() ObjectFixupSuppressor suppressor_;
	
}

#include "Package.inl.h"

#endif
