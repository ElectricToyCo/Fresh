//
//  ClassInfo.cpp
//  Fresh
//
//  Created by Jeff Wofford on 2/25/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "ClassInfo.h"
#include "Object.h"
#include "Classes.h"

namespace
{
	using namespace fr;

	bool hasConfigurationKey( const Manifest::Map& paring, const std::string& key )
	{
		return paring.find( key ) != paring.end();
	}
	
	// Produces a property map based on configuration such that
	// properties that are present in the pendingInitializationElement
	// are removed from the configuration element.
	// This has the effect of causing NoDefault properties of pseudoclasses to only get applied at all
	// if the currently-loading object of that pseudoclass completely ignores the property in question.
	//
	Manifest::Map pareConfiguration( const Manifest::Map& configuration, const Manifest::Map* pendingInitialization )
	{
		Manifest::Map pared = configuration;
		
		if( pendingInitialization )
		{
			for( auto iter = pared.begin(); iter != pared.end(); /* iteration within */ )
			{
				if( hasConfigurationKey( *pendingInitialization, iter->first ))
				{
					iter = pared.erase( iter );
				}
				else
				{
					++iter;
				}
			}
		}
		return pared;
	}

	template< typename init_t >
	void genericApplyConfiguration( Object& toObject, const Manifest::Map& configuration, const init_t& pendingInitialization )
	{
		// Apply the configuration element, if any.
		//
		auto pared = pareConfiguration( configuration, pendingInitialization );
		toObject.load( pared );
	}
}


namespace fr
{

	ClassInfo::PropertyIterator::PropertyIterator( ClassInfo::cptr hostClass )
	:	m_currentClass( hostClass )
	,	m_iterator( hostClass->m_properties.begin() )
	{
		REQUIRES( hostClass );
		
		// Make sure that the iterator is actually at the beginning of valid properties.
		// It might actually end up at the very end!
		//
		advanceToNextValidIteratorOrToFinalEnd();
	}
	
	ClassInfo::PropertyIterator::PropertyIterator( ClassInfo::cptr hostClass, PropertiesCI iterator )
	:	m_currentClass( hostClass )
	,	m_iterator( iterator )
	{
		REQUIRES( hostClass );
	}
	
	ClassInfo::PropertyIterator& ClassInfo::PropertyIterator::operator++()
	{
		ASSERT( m_currentClass );
		ASSERT( m_iterator != m_currentClass->m_properties.end() );
		
		++m_iterator;
		advanceToNextValidIteratorOrToFinalEnd();
		
		return *this;
	}
	
	PropertyAbstract& ClassInfo::PropertyIterator::operator*() const
	{
		return **m_iterator;
	}

	PropertyAbstract* ClassInfo::PropertyIterator::operator->() const
	{
		return (*m_iterator).get();
	}
	
	bool ClassInfo::PropertyIterator::operator==( const PropertyIterator& other ) const
	{
		return m_currentClass == other.m_currentClass && m_iterator == other.m_iterator;
	}
	
	void ClassInfo::PropertyIterator::establishIterator()
	{
		// If the iterator is non-end, advance it so long as it is overridden.
		//
		while( isOverridden( m_iterator ))
		{
			++m_iterator;
		}
		// The iterator is now either at an end or is not overridden.
		
		// If the iterator is non-end and is an override, list it as overridden.
		//
		if( m_iterator != m_currentClass->m_properties.end())
		{
			if( (*m_iterator)->isOverride() )
			{
				m_visitedOverrides.insert( (*m_iterator)->propName() );
			}
		}
	}
	
	void ClassInfo::PropertyIterator::advanceToNextValidIteratorOrToFinalEnd()
	{
		ASSERT( m_currentClass );
		
		establishIterator();
		
		// Is the iterator now at the end of the current class's container of properties?
		//
		while( m_iterator == m_currentClass->m_properties.end() )
		{
			// Move to the next Super class, if there is one.
			//
			ClassInfo::cptr superClass = m_currentClass->getSuperClass();
			
			if( superClass )
			{
				m_currentClass = superClass;				
				m_iterator = m_currentClass->m_properties.begin();
				establishIterator();
			}
			else
			{
				// We're at the end of the properties of the least derived class.
				// Stop here.
				break;
			}
		}
	}
	
	bool ClassInfo::PropertyIterator::isOverridden( PropertiesCI iter ) const
	{
		return m_iterator != m_currentClass->m_properties.end() &&
		( m_visitedOverrides.find( (*m_iterator)->propName() ) != m_visitedOverrides.end());
	}
	
	////////////////////////////////////////////////////////////////////////////////////
	
	ClassInfo::ClassInfo( ClassInfo::ptr superClass, NameRef szClassName, std::unique_ptr< ObjectFactoryBase >&& factory, const Manifest::Map* configuration, bool isNative, Placeable isPlaceable )
	:	m_className( szClassName )
	,	m_superClass( superClass )
	,	m_ordinal( -1 )
	,	m_factory( std::move( factory ))
	,	m_configuration( configuration ? *configuration : Manifest::Map{} )
	,	m_isNative( isNative )
	,	m_isPlaceable( isPlaceable )
	{
		REQUIRES( !m_className.empty() );
		REQUIRES( m_superClass || szClassName == "Object" );
	}

	void ClassInfo::concludeInitialization()
	{
#ifdef FRESH_VERBOSE_REFLECTION
		dev_trace( "Class " << m_className << " concluding initialization" );
#endif
		if( !isAbstract() )
		{
			overrideAbstractSuperClassProps();

			m_defaultObject = m_factory->createInertObject( *this );
			applyConfiguration( *m_defaultObject );
		}
	}
	
	ClassInfo::~ClassInfo()
	{
#ifdef FRESH_VERBOSE_REFLECTION
		dev_trace( "Class " << m_className << " destroyed" );
#endif
	}
	
	ClassInfo::NameRef ClassInfo::className() const
	{
		return m_className;
	}
	
	bool ClassInfo::isPlaceable() const
	{
		return !isAbstract() && isPlaceableInPrinciple();
	}
	
	bool ClassInfo::isPlaceableInPrinciple() const
	{
		if( m_isPlaceable == Placeable::Inherit )	// Derive our placeability from our super class.
		{
			if( m_superClass )
			{
				return m_superClass->isPlaceableInPrinciple();
			}
			else
			{
				// For class Object: never placeable.
				return false;
			}
		}
		else
		{
			return m_isPlaceable == Placeable::True;
		}
	}
	
	bool ClassInfo::isKindOf( const std::string& classIdFilter, bool exactMatch ) const
	{
		if( !exactMatch && classIdFilter.empty() )
		{
			return true;
		}
		
		return ( exactMatch ? m_className == classIdFilter :
				 matchesFilter( m_className, classIdFilter )) || ( m_superClass && m_superClass->isKindOf( classIdFilter, exactMatch ));
	}
	
	bool ClassInfo::isKindOf( const ClassInfo& otherClass ) const
	{
#if TABULAR_CLASS_IS_KIND_OF
		return isClassKindOf( *this, otherClass );
#else
		const ClassInfo* const pOtherClass = &otherClass;
		const auto iter = m_superClassCache.find( pOtherClass );
		if( iter == m_superClassCache.end() )
		{
			const bool isOtherClass = isKindOf_thorough( otherClass );
			m_superClassCache[ pOtherClass ] = isOtherClass;
			return isOtherClass;
		}
		else
		{
			return iter->second;
		}
#endif
	}
	
	bool ClassInfo::isKindOf_thorough( const ClassInfo& otherClass ) const
	{
		return &otherClass == this || ( m_superClass && m_superClass->isKindOf( otherClass ));
	}
	
	int ClassInfo::getSuperClassDepth( const ClassInfo& possibleSuperClass ) const
	{
		if( this == &possibleSuperClass )
		{
			return 0;
		}
		else if( m_superClass )
		{
			int myBaseClassDepth = m_superClass->getSuperClassDepth( possibleSuperClass );
			if( myBaseClassDepth >= 0 )
			{
				return myBaseClassDepth + 1;
			}
			else
			{
				return -1;
			}
		}
		else
		{
			return -1;
		}
	}
	
	void ClassInfo::addProperty( std::unique_ptr< PropertyAbstract >&& prop )
	{
		REQUIRES( prop );
		REQUIRES( !prop->propName().empty() );
		REQUIRES( ( prop->isOverride() && m_superClass && m_superClass->getPropertyByName( prop->propName() )) || !getPropertyByName( prop->propName() ));
		
#ifdef DEBUG
		const std::string propName = prop->propName();
#endif
		
#if FRESH_VERBOSE_REFLECTION
		dev_trace( "Class " << m_className << " adding property " << prop->propName() << ( prop->isOverride() ? " (override) " : "" ) <<  "." );
#endif

		
		m_properties.emplace_back( std::move( prop ));
		
		PROMISES( getPropertyByName( propName ));
	}
	
	PropertyAbstract* ClassInfo::overrideProperty( PropertyNameRef propName, unsigned int additionalFlags )
	{
		auto prop = getPropertyByName( propName );
		ASSERT( prop );
		
		addProperty( prop->createOverride( *this, prop->flags() | additionalFlags ));
		
		return getPropertyByName( propName );
	}
	
	PropertyAbstract* ClassInfo::getPropertyByName( PropertyNameRef propName ) const
	{
		for( PropertiesCI iter = m_properties.begin(); iter != m_properties.end(); ++iter )
		{
			if( stringCaseInsensitiveCompare( (*iter)->propName(), propName ))
			{
				return iter->get();
			}
		}
		
		if( m_superClass )
		{
			return m_superClass->getPropertyByName( propName );
		}
		else
		{
			return nullptr;
		}
	}
	
	ClassInfo::PropertyIterator ClassInfo::getPropertyIteratorBegin() const
	{
		return PropertyIterator( this );
	}
	
	ClassInfo::PropertyIterator ClassInfo::getPropertyIteratorEnd() const
	{
		ClassInfo::cptr leastDerivedClass = this;
		for( ClassInfo::cptr classInfo = this; classInfo; classInfo = classInfo->getSuperClass() )
		{
			leastDerivedClass = classInfo;
		}
		
		return PropertyIterator( leastDerivedClass, leastDerivedClass->m_properties.end() );
	}
	
	void ClassInfo::applyConfiguration( Object& toObject, const Manifest::Map* pendingInitialization ) const
	{
		TIMER_AUTO( ClassInfo::applyConfiguration( Manifest::Map ) )
		
		// Propagate the call up the class chain recursively.
		//
		if( m_superClass )
		{
			m_superClass->applyConfiguration( toObject, pendingInitialization );
		}

		toObject.load( pareConfiguration( m_configuration, pendingInitialization ) );
	}

	void ClassInfo::addMethod( const std::string& name, std::unique_ptr< StreamedMethodAbstract >&& streamedMethod )
	{
		REQUIRES( streamedMethod );
		REQUIRES( !getMethodByName( name ) );
		m_methods.emplace( std::make_pair( name, std::move( streamedMethod )));
	}
	
	void ClassInfo::forEachMethod( std::function< void( const std::string&, const StreamedMethodAbstract& ) >&& fn ) const
	{
		for( const auto& pair : m_methods )
		{
			fn( pair.first, *pair.second );
		}
		
		if( m_superClass )
		{
			m_superClass->forEachMethod( std::move( fn ));
		}
	}
	
	void ClassInfo::addAccessor( const std::string& name, std::unique_ptr< SimpleAccessorAbstract >&& accessor )
	{
		REQUIRES( accessor );
		REQUIRES( !getAccessorByName( name ) );
		
		m_accessors[ name ] = std::move( accessor );
	}
	
	const StreamedMethodAbstract* ClassInfo::getMethodByName( const std::string& name ) const
	{
		auto iter = m_methods.find( name );
		
		if( iter != m_methods.end() )
		{
			return iter->second.get();
		}
		else if( m_superClass )
		{
			return m_superClass->getMethodByName( name );
		}
		else
		{
			return nullptr;
		}
	}
	
	const SimpleAccessorAbstract* ClassInfo::getAccessorByName( const std::string& name ) const
	{
		auto iter = m_accessors.find( name );
		
		if( iter != m_accessors.end() )
		{
			return iter->second.get();
		}
		else if( m_superClass )
		{
			return m_superClass->getAccessorByName( name );
		}
		else
		{
			return nullptr;
		}
	}
	
	const ClassInfo& ClassInfo::getCommonBase( const ClassInfo& otherClass ) const
	{
		if( isKindOf( otherClass ))				// He's my common base.
		{
			return otherClass;
		}
		else if( otherClass.isKindOf( *this ))	// I'm his common base.
		{
			return *this;
		}
		else									// Let's ask our parents.
		{
			return m_superClass->getCommonBase( *otherClass.m_superClass );
		}
	}
	
	bool ClassInfo::operator==( const ClassInfo& otherClass ) const
	{
		return this == &otherClass;
	}

	void ClassInfo::overrideAbstractSuperClassProps()
	{
		// If my super is abstract--and recursively on down so long as supers are abstract,
		// override their originated properties.
		// This is so that users can ask the default value of those properties.
		
		ClassInfo::cptr superClass = m_superClass;
		
		while( superClass && superClass->isAbstract() )
		{
			for( const auto& property : superClass->m_properties )
			{
				addProperty( property->createOverride( *this ));
			}
			
			superClass = superClass->m_superClass;
		}
	}
	
}

