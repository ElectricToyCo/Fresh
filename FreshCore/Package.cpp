
//
//  Package.cpp
//  Fresh
//
//  Created by Jeff Wofford on 2/12/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "Package.h"
#include "FreshFile.h"
#include "Archive.h"
#include "ObjectStreamFormatter.h"
#include "CommandProcessor.h"
#include "FreshXML.h"
#include "Constants.h"

#define TRACE_LOADING 0

#if DEV_MODE && TRACE_LOADING
#	define	trace_loads( x ) release_trace( x )
#else
#	define	trace_loads( x )
#endif

namespace
{
	using namespace fr;
	
	typedef std::vector< fr::Package::ptr > ActivePackageStack;
	ActivePackageStack& getActivePackageStack()
	{
		static ActivePackageStack stackActivePackages;
		return stackActivePackages;
	}

	typedef std::vector< fr::ObjectFixupCapturer* > Capturers;
	Capturers& getCapturers()
	{
		static Capturers capturers;
		return capturers;
	}
	
	size_t g_fixupSuppressionDepth = 0;
}

namespace fr
{
	Package::ptr activePackage()
	{
		ActivePackageStack& stack = getActivePackageStack();
		
		if( stack.empty() )
		{
			return nullptr;
		}
		else
		{
			return stack.back();
		}
	}
	
	void pushActivePackage( Package::ptr package )
	{
		getActivePackageStack().push_back( package );
	}
	
	Package::ptr popActivePackage()
	{
		ActivePackageStack& stack = getActivePackageStack();
		ASSERT( !stack.empty() );
		
		Package::ptr top = stack.back();
		stack.pop_back();
		return top;
	}
	
	//////////////////////////////////////////////
	
	ObjectFixupCapturer::ObjectFixupCapturer( const std::string& message, bool requireFixup )
	:	m_message( message )
	,	m_requireFixup( requireFixup )
	{
		if( g_fixupSuppressionDepth == 0 )	// Not suppressed
		{
			getCapturers().push_back( this );
		
			// If no package is active, create a temporary one to aid in object searching and such.
			//
			if( !activePackage() )
			{
				m_tempPackage = createPackage();
				pushActivePackage( m_tempPackage );
			}
		}
	}
	
	ObjectFixupCapturer::~ObjectFixupCapturer()
	{
		if( g_fixupSuppressionDepth == 0 )	// Not suppressed
		{
			ASSERT( getCapturers().back() == this );
			getCapturers().pop_back();
			
			if( m_requireFixup || getCapturers().empty() )	// Was I the last capturer in the chain?
			{
				// Actually do the fixup.
				//
				ObjectLinker& objectLinker = ObjectLinker::instance();
				objectLinker.fixupObjectAddresses();
				
				if( objectLinker.doFixupsRemain() )
				{
					dev_warning( m_message << "Found some objects referenced other objects that don't (yet?) exist." );
					objectLinker.reportRemainingFixups();
					objectLinker.clearRemainingFixups();
				}
			}
			
			if( m_tempPackage )
			{
				popActivePackage();
			}
		}
	}

	ObjectFixupSuppressor::ObjectFixupSuppressor()
	{
		++g_fixupSuppressionDepth;
	}
	
	ObjectFixupSuppressor::~ObjectFixupSuppressor()
	{
		ASSERT( g_fixupSuppressionDepth > 0 );
		--g_fixupSuppressionDepth;
	}

	//////////////////////////////////////////////
	
	FRESH_DEFINE_CLASS( Package )

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( Package )
	
	
	bool Package::empty() const
	{
		tidy();
		return m_members.empty();
	}
	
	size_t Package::size() const
	{
		tidy();
		return m_members.size();
	}
	
	void Package::save( const path& fullPath, bool forceSaveAllProperties ) const
	{
		// Create an in-memory stream to save to. This avoids corruption from aborts and other problems during save.
		//
		std::stringstream buffer;
		save( buffer, forceSaveAllProperties );

		// Now save the memory to disk.
		//
		std::ofstream out;
		out.exceptions( std::ios::badbit | std::ios::failbit );
		
		try
		{
			out.open( fullPath.c_str() );
			out << buffer.rdbuf();
		}
		catch( const std::exception& e )
		{
			con_error( "Exception saving package " << name() << " to " << fullPath << ": " << e.what() );
			throw;
		}
	}
	
	void Package::writeRootElement( std::ostream& out ) const
	{
		out << "<objects>\n";
	}
	
	void Package::save( std::ostream& out, bool forceSaveAllProperties ) const
	{
		tidy();
		
		// TODO: SAVE FRESH FORMAT
		out << "<?xml version='1.0' encoding='UTF-8' ?>\n";
		
		writeRootElement( out );
		
		Stringifier stringifier( out );
		ObjectStreamFormatterXml formatter( stringifier, 1 );
		
		std::for_each( m_members.begin(), m_members.end(), [ & formatter, & forceSaveAllProperties ] ( const SwitchPtr< Object >& p )
		{
			p->serialize( formatter, forceSaveAllProperties );
		} );
		
		out << "</objects>" << std::endl;
		
		out.flush();
	}
	
	std::vector< Object::ptr > Package::loadFromManifest( const Manifest& manifest )
	{
		trace_loads( "Starting." );
		
		tidy();
		
		m_isLoading = true;
		
		const bool wasActiveToBeginWith = activePackage() == this;
		if( !wasActiveToBeginWith )
		{
			pushActivePackage( this );
		}
		
		std::vector< Object::ptr > objects;
		
		{
			CAPTURE_FIXUP_FOR_PACKAGE( "While loading package '" << name() << "': " );
			
			manifest.eachDirective( [&]( const Manifest::Directive& directive )
			{
				switch( directive.kind() )
				{
					case Manifest::Directive::Kind::Const:
					{
						const Manifest::Const* const directiveConst = directive.as< Manifest::Const >();
						ASSERT( directiveConst );
						
						const auto& constName = directive.name();
						const auto& constType = directiveConst->type;
						const auto& valueText = directiveConst->value;

						trace_loads( "Adding constant " << constType << " named " << constName );
						
						fr::constant::setValueString( constType, fr::toLower( constName ), valueText );
						break;
					}
						
					case Manifest::Directive::Kind::Object:
					{
						const Manifest::Object* const directiveObject = directive.as< Manifest::Object >();
						ASSERT( directiveObject );
						
						try
						{
							trace_loads( "Creating object with id: " << ObjectId{ *directiveObject } );
							
							Object::ptr object = createOrGetObject( *directiveObject, true /* apply element if found */ );
							trace_loads( "After create object." );
							ASSERT( object );
							
							trace_loads( "Created object with id: " << ObjectId{ *directiveObject } );
							
							objects.push_back( object );
							
							trace_loads( "Pushed it back." );
						}
						catch( FreshException& e )
						{
							con_error( "Package " << name() << " had exception while attempting to create " << ObjectId{ *directiveObject } << ": " << e.what() );
						}
							catch( ... )
						{
							con_error( "Package " << name() << " had unknown exception while attempting to create " << ObjectId{ *directiveObject });
						}
						break;
					}
						
					case Manifest::Directive::Kind::Class:
					{
						const Manifest::Class* const directiveClass = directive.as< Manifest::Class >();
						ASSERT( directiveClass );
						
						// Add a pseudoclass.
						//
						const auto& className = directiveClass->name();
						
						if( directiveClass->baseClassNames.size() != 1 )
						{
							dev_warning( "While loading package '" << name() << "', class " << className << " had "
										<< directiveClass->baseClassNames.size()
										<< " super classes. Needs exactly 1. Ignoring." );
						}
						else
						{
							const auto& baseClassName = directiveClass->baseClassNames.front();
							
							trace_loads( "Creating class " << className );
							
							ClassInfo::Placeable isPlaceable = ClassInfo::Placeable::Inherit;
							
							try
							{
								createClass( className, baseClassName, *directiveClass, isPlaceable );
							}
							catch( FreshException& e )
							{
								dev_warning( e.what() );
							}
						}
						break;
					}
				}
			} );
			
		}	// This closes the capture, which causes pointer fixup.
		
		
		// Post-load all loaded objects.
		// This gives objects a chance to complete their loading process *after*
		// having their pointers to other objects fixed up.
		//
		
		trace_loads( "Post loading..." );
		forEachMember( std::mem_fn( &Object::postLoad ));
		
		trace_loads( "Calling onAllLoaded() for each member..." );
		forEachMember( std::mem_fn( &Object::onAllLoaded ));
		
		m_isLoading = false;
		
		ASSERT( activePackage() == this );
		if( !wasActiveToBeginWith )
		{
			popActivePackage();
		}
		
		tidy();
		
		trace_loads( "Done." );
		
		return objects;
	}
	
	void Package::forEachMember( std::function< void( const SmartPtr< Object >& ) >&& fn ) const
	{
		tidy();
		const auto copy = m_members;
		std::for_each( copy.begin(), copy.end(), fn );
	}
	
	std::vector< Object::ptr > Package::loadFile( const path& fullPath )
	{
		trace_loads( "Package loading from '" << fullPath << "'." );
		
		auto extension = fr::toLower( fullPath.extension() );
		
		Manifest manifest;
		if( extension == ".xml" )
		{
			// Load the package.
			//
			XmlDocument doc;
			const XmlElement* rootElement = loadXmlDocument( fullPath.c_str(), doc );
			
			if( !rootElement )
			{
				FRESH_THROW( FreshException, "Unable to load package " << name() << " from " << fullPath << "." );
			}
			if( rootElement->ValueStr() != "objects" )
			{
				dev_warning( "Package " << name() << " loading file '" << fullPath << "' did not have an <objects> root node (root node is <" << rootElement->Value() << ">)." );
			}

			manifest.load( *rootElement );
		}
		else if( extension == ".fresh" )
		{
			try
			{
				manifest.load( fullPath.string() );
			}
			catch( const std::exception& e )
			{
				FRESH_THROW( FreshException, "Unable to load package " << name() << " manifest: " << e.what() );
			}
		}
		else
		{
			FRESH_THROW( FreshException, "Package " << name() << " cannot load from file '" << fullPath << "' because the extension '" << extension << "' is not supported." );
		}
		return loadFromManifest( manifest );
	}
	
	//
	// PACKAGE OBJECT MANAGEMENT INTERFACE
	//
	
	Object::ptr Package::findGeneric( const ClassInfo& classInfo, NameRef objectName ) const
	{
		REQUIRES( objectName.empty() == false );
		auto iter = std::find_if( m_members.begin(), m_members.end(), [&classInfo, &objectName] ( const SwitchPtr< Object >& p )
								 {
									 SmartPtr< Object > q = p;	// Convert to SmartPtr<> to avoid repeated, expensive WeakPtr null checks.
									 return q && q->hasName( objectName ) && q->isA( classInfo );
								 });
		
		if( iter != m_members.end() )
		{
			return *iter;
		}
		else
		{
			return nullptr;
		}
	}
	
	Object::ptr Package::requestGeneric( const ClassInfo& classInfo, NameRef objectName )
	{
		// Overridden in subclasses.
		return findGeneric( classInfo, objectName );
	}
	
	bool Package::has( Object::cptr object ) const
	{
		TIMER_AUTO( Package::has )
		
		REQUIRES( object );
		return m_members.end() != std::find( m_members.begin(), m_members.end(), SwitchPtr< Object >( const_cast< Object* >( object.get() )));	// TODO bit of a hack
	}
	
	void Package::add( Object::ptr object )
	{
		TIMER_AUTO( Package::add )
		
		REQUIRES( object );
		REQUIRES( !has( object ));
		REQUIRES( object != this );
		REQUIRES( !find( object->objectId() ));

		quickTidy();
		
		m_members.push_back( SwitchPtr< Object >( object, m_isRetained ));

		PROMISES( has( object ));
	}

	ObjectName Package::getUniqueName( const ClassInfo& classInfo, ObjectNameRef name ) const
	{
		REQUIRES( name.empty() == false );
		
		const char* const MANGLING_TOKEN = "~x";
		
		// Does the object already have something that looks like a "unique mangler" (following "~x")?
		// If so, removing the mangling.
		//
		auto manglerPos = name.find( MANGLING_TOKEN );
		ObjectName demangledName = name.substr( 0, manglerPos );
		
		std::ostringstream newName( name );			// For the first check, just check the original name itself.
		
		while( find( classInfo, newName.str() ))	// So long as this name is not unique among objects of this class...
		{
			const int ordinal = rand() & 0xFFFF;	// A random ordinal is chosen in order to decrease the chance of name collisions both here and in other packages.
			
			newName.str( "" );
			newName << demangledName << MANGLING_TOKEN << std::hex << ordinal;
		}
			
		PROMISES( !newName.str().empty() );
		
		return newName.str();
	}
	
	void Package::merge( Object::ptr object, MergePolicy policy )
	{
		REQUIRES( object );
		REQUIRES( !has( object ));
		REQUIRES( object != this );

		if( auto existing = find( object->classInfo(), object->name() ) )
		{
			// Has similar object.
			//
			switch( policy )
			{
				default:
					ASSERT( false );
				case MergePolicy::IgnoreNew:
					// Do nothing.
					return;
					
				case MergePolicy::ReplaceExisting:
					remove( existing );
					break;
					
				case MergePolicy::KeepBothRenamingNew:
				{
					// This is a precarious operation. The system assumes that every object of a given class in a package is named uniquely.
					// That's the whole point of the getUniqueName() function.
					// And yet renaming the object for this package may cause it to have a non-unique name in other packages.
					// When this operation goes wrong, it could cause bugs such as objects that are clearly in a package not being found
					// due to having the same name as an "earlier" (in the member container sequence) object.
					//
					object->rename( getUniqueName( object->classInfo(), object->name() ));
					
					break;
				}
			}
		}

		// Having prepared for its addition above, add the new object.
		//
		add( object );
	}
	
	void Package::merge( const Package& other, MergePolicy policy )
	{
		if( this != &other )
		{
			other.forEachMember( std::bind( ( void (Package::*)( Object::ptr, MergePolicy )) &Package::merge, this, std::placeholders::_1, policy ));
		}
	}
	
	void Package::remove( Object::ptr object )
	{
		REQUIRES( object );
		REQUIRES( has( object ));
		
		auto iter = std::find( m_members.begin(), m_members.end(), object );
		REQUIRES( iter != m_members.end() );

		m_members.erase( iter );

		PROMISES( !has( object ));
	}
	
	void Package::collect( std::function< bool( const Object& ) >&& filter )
	{
		std::ostringstream out;	// A null stream would be better.
		
		StringifierObjectGraph objectGraph;
		Stringifier stringifier( out, &objectGraph );
		ObjectStreamFormatterXml formatter( stringifier );
		
		StringifierObjectGraph::ObjectSet savedObjects;
		StringifierObjectGraph::ObjectSet pendingObjects;
		
		std::for_each( m_members.begin(), m_members.end(), [&pendingObjects] ( const SwitchPtr< Object >& p )
					  {
						  if( p ) pendingObjects.insert( SmartPtr< Object >( p ));
					  } );
		
		while( !pendingObjects.empty() )
		{
			Object::cptr object = *pendingObjects.begin();
			ASSERT( object );	// Refuse null pointers.
			
			if( filter( *object ))
			{
				// This object passes the test. Make sure it's in the package.
				//
				if( !has( object ))
				{
					// The stringifier's pending objects container insists on const objects
					// because that's in the nature of (output) stringification. We need
					// to change the object, however, and since Stringifier's constness
					// is just a byproduct of that particular feature (not the situation
					// in general), it should be safe to do so.
					//
					Object::ptr mutableObject( const_cast< Object* >( object.get() ));
					add( mutableObject );
				}
				
				// Now carry on looking for objects that this object references.
				//
				object->serialize( formatter );
				
				savedObjects.insert( object );
				
				// Append new found objects to the pending objects set.
				//
				const StringifierObjectGraph::ObjectSet& foundObjects = objectGraph.getFoundObjects();
				pendingObjects.insert( foundObjects.begin(), foundObjects.end() );
				
				objectGraph.clear();
				
				// Remove saved objects from pending objects so that only the unsaved pending objects remain.
				//
				std::vector< Object::cptr > remainingObjects;
				std::set_difference( pendingObjects.begin(), pendingObjects.end(), savedObjects.begin(), savedObjects.end(), std::back_inserter( remainingObjects ));
				pendingObjects.clear();
				pendingObjects.insert( remainingObjects.begin(), remainingObjects.end() );
			}
			else
			{
				pendingObjects.erase( pendingObjects.begin() );
			}
		}
	}
				
	bool Package::areMembersRetained() const
	{
		return m_isRetained;
	}
	
	void Package::retainMembers()
	{
		if( !m_isRetained )
		{
			std::for_each( m_members.begin(), m_members.end(), std::mem_fn( &SwitchPtr< Object >::retain ));
			m_isRetained = true;
		}
		PROMISES( areMembersRetained() );
	}
	
	
	void Package::releaseMembers()
	{
		if( m_isRetained )
		{
			std::for_each( m_members.begin(), m_members.end(), std::mem_fn( &SwitchPtr< Object >::release ));
			m_isRetained = false;
			tidy();
		}
		PROMISES( !areMembersRetained() );
	}
	
	// In "retained" mode, releases objects that are unreferenced except by this class, so that at least
	// minBytesToReduce bytes of memory are recovered (if this can be measured).
	// If minBytesToReduce == 0, releases all unreferenced objects.
	void Package::releaseRetainedZombies( size_t minBytesToReduce )
	{
		REQUIRES( areMembersRetained() );
				
		tidy();
		
		std::vector< Object::ptr > zombies;
		
		// Identify all members that are only alive by virtue of being referenced by this package.
		//
		std::copy_if( m_members.begin(), m_members.end(), std::back_inserter( zombies ), []( const Object::ptr& p )
		{
			return p->getReferenceCount() <= 2;
		} );
		
		if( !zombies.empty() )
		{
			if( minBytesToReduce > 0 )
			{
				// Sort the unused objects by size, to get rid of big ones first.
				//
				sortZombiesForReleasePriority( zombies.begin(), zombies.end() );
			}
			
			// Remove unused objects until we've reduced the memory at least as much as the target.
			// If the target is 0, get rid of all unused textures.
			//
			size_t totalBytesFreed = 0;
			
			for( auto iter = zombies.begin(); iter != zombies.end() && ( minBytesToReduce == 0 || totalBytesFreed < minBytesToReduce ); ++iter )
			{
				const Object::ptr& zombie = *iter;
				
				const size_t nZombieBytes = getZombieSizeBytes( *zombie );
				totalBytesFreed += nZombieBytes;
				
#if DEV_MODE && 0		// Report zombies being cached out.
				trace( "deleting zombie " << zombie << " with bytes=" << nZombieBytes );
#endif
				
				m_members.erase( std::find( m_members.begin(), m_members.end(), zombie ));
			}
			
#if DEV_MODE && 1
			trace( "freed " << totalBytesFreed << " bytes." );
#endif
			
			// Since released zombies may reduce the reference count on other objects (e.g. Fonts referencing Textures),
			// Recursively call this function again until nothing is released.
			// Only recurse if we actually released something just now, and we're not already done releasing enough bytes.
			//
			if( totalBytesFreed > 0 && ( minBytesToReduce == 0 || totalBytesFreed < minBytesToReduce ))
			{
				if( totalBytesFreed < minBytesToReduce )
				{
					minBytesToReduce -= totalBytesFreed;
				}
				
				releaseRetainedZombies( minBytesToReduce );
			}
		}
	}
	
	void Package::sortZombiesForReleasePriority( std::vector< Object::ptr >::iterator begin, std::vector< Object::ptr >::iterator end )
	{
		// Ignore. Subclasses can override.
	}

	size_t Package::getZombieSizeBytes( const Object& zombie ) const
	{
		return 0;	// Subclasses override.
	}
	
	size_t Package::numNullMembers() const
	{
		return std::count_if( m_members.begin(), m_members.end(), []( const SwitchPtr< Object >& p ) { return !p; } );
	}
	
	void Package::tidy() const
	{
		TIMER_AUTO( Package::tidy )
		
		// A bit evil. tidy() pretends to be a const function, but it's anything but.
		// The reason: I don't want to make m_members mutable. But I need to call tidy
		// from lots of const functions.
		
		if( !areMembersRetained() )
		{
			Package* nonConstMe = const_cast< Package* >( this );
			nonConstMe->m_members.remove_if( []( const SwitchPtr< Object >& p )
											{
												return !p;
											} );
		}
		
#if DEV_MODE && 0
		// Verify that no two objects have the same class and name.
		//
		std::set< ObjectId > uniqueIds;
		std::transform( m_members.begin(), m_members.end(), std::inserter( uniqueIds, uniqueIds.begin() ), []( const decltype( m_members )::value_type& member )
					   {
						   return member->objectId();
					   } );
		
		ASSERT( uniqueIds.size() == m_members.size() );		
#endif
	}
	
	void Package::quickTidy() const
	{
		TIMER_AUTO( Package::quickTidy )

		const auto nItemsToTidy = quickTidySize();
		if( nItemsToTidy == 0 ) { return; }
		
		const auto nMembers = m_members.size();
		
		const auto begin = m_nextQuickTidyBegin >= nMembers ? 0 : m_nextQuickTidyBegin;
		ASSERT( nMembers >= begin );
		
		const auto count = std::min( nItemsToTidy, nMembers - begin );

		Package* nonConstMe = const_cast< Package* >( this );
		auto& members = nonConstMe->m_members;
		auto beginIter = members.begin();
		std::advance( beginIter, begin );

		auto endIter   = beginIter;
		std::advance( endIter, count );
		
		while( beginIter != endIter )
		{
			const auto& p = *beginIter;
			
			if( !p )
			{
				beginIter = members.erase( beginIter );
			}
			else
			{
				++beginIter;
			}
		}

		m_nextQuickTidyBegin += count;
	}
	
	size_t Package::quickTidySize() const
	{
		return m_members.empty() ? 0 : std::max( static_cast< size_t >( 1 ), m_members.size() / 10 );
	}
}

