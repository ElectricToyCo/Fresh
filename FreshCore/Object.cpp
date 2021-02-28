#include "FreshDebug.h"
#include "Object.h"
#include "Classes.h"
#include "CommandProcessor.h"
#include "ObjectLinker.h"
#include "ObjectStreamFormatter.h"
#include "Property.h"
#include <ctime>

#define TRACE_LOADING 0

#if TRACE_LOADING && DEV_MODE
#	define trace_loading release_trace
#else
#	define trace_loading( x )
#endif

namespace
{
	using namespace fr;
	
	std::vector< Object::ptr >& getLoadingObjects()
	{
		static std::vector< Object::ptr > stackLoadingObjects;
		return stackLoadingObjects;
	}
}

namespace fr
{
	size_t loadStackDepth()
	{
		return getLoadingObjects().size();
	}
	
	Object::ptr loadStackTop()
	{
		return loadStackFromTop( 0 );
	}
	
	Object::ptr loadStackFromTop( size_t depth )
	{
		REQUIRES( depth < loadStackDepth() );
		
		std::vector< Object::ptr >& objects = getLoadingObjects();
		
		return objects.at( objects.size() - 1 - depth );
	}
	
	void pushCurrentLoadingObject( Object::ptr object )
	{
		getLoadingObjects().push_back( object );
	}
	
	void popCurrentLoadingObject()
	{
		getLoadingObjects().pop_back();
	}
	
	Object::Name parseObjectName( Object::NameRef name )
	{
		// Go through the name looking for the '$^' substring, replacing it with the current loading object's name.
		
		// Find dollar signs in the name.
		//
		Object::Name amendedName( name );
		size_t dollarPos = 0;
		while( true )
		{
			// Is the up-reference symbol to be found?
			//
			dollarPos = amendedName.find( "$^", dollarPos );
			if( dollarPos == std::string::npos )
			{
				// No. done.
				//
				break;
			}
			
			size_t depth = 0;
			size_t symbolEndPos = dollarPos + 2;
			
			const size_t maxStackDepth = loadStackDepth();
			
			if( maxStackDepth == 0 )
			{
				dev_warning( "Found a symbolic reference ($^) when we had no loading objects." );
				++dollarPos;	// Move on so we don't find the same symbol again.
			}
			else
			{
				// How "deep" is the reference?
				//
				while( symbolEndPos < amendedName.size() && depth < maxStackDepth && amendedName[ symbolEndPos ] == '^' )
				{
					++symbolEndPos;
					++depth;
				}
				
				if( depth >= maxStackDepth )
				{
					dev_warning( "Symbolic reference (" << amendedName.substr( dollarPos, symbolEndPos - dollarPos ) <<  ") referenced beyond the front of the loading object stack (stack depth is " << loadStackDepth() << ")." );
					++dollarPos;	// Move on so we don't find the same symbol again.
				}
				else
				{
					// Insert current object name.
					//
					Object::ptr loadingObject = loadStackFromTop( depth );
					ASSERT( loadingObject );
					Object::NameRef parentName = loadingObject->name();
					
					// Erase the "$^..." substring
					//
					amendedName.erase( dollarPos, symbolEndPos - dollarPos );
					amendedName.insert( dollarPos, parentName );
					dollarPos += parentName.size();
				}
			}
		}
		
		return amendedName;
	}
	

	////////////////////////////////////////////////////////////////////////////////////
	
	size_t Object::s_nObjectsCreated = 0;
	bool Object::s_useTimeCodedDefaultNames = true;
		
	Object::Object( const ClassInfo& assignedClassInfo, NameRef objectName /* = DEFAULT_OBJECT_NAME */ )
	:	m_name( objectName )
	,	m_classInfo( &assignedClassInfo )
	,	m_nReferences( 0 )
	{
		// Can't use the doctorClass() technique because Object is generally constructed before its default object even exists.
		if( !StaticGetClassInfo().isDefaultObjectDoctored() )
		{
			auto& classInfo = StaticGetClassInfo();
		
			// Add the "shallow" property. Normally this kind of thing is done via the VAR/DEFINE_VAR directives, but Object.h doesn't have access to those.
			//
			const Object* host = nullptr;
			size_t byteOffset = reinterpret_cast< size_t >( &( host->m_shallow ));
			classInfo.addProperty( std::unique_ptr< fr::PropertyAbstract >( new Property< bool >( classInfo, "shallow", byteOffset, fr::PropFlag::NormalNative )));
			
			classInfo.markDefaultObjectDoctored();
		}
		
		if( m_name == DEFAULT_OBJECT_NAME )
		{
			createDefaultName( m_name );
		}
		
		++s_nObjectsCreated;
	}

	Object::Object( CreateInertObject c )
	:	m_isInert( true )
	{}

	Object::~Object()
	{
		ASSERT( m_nReferences == 0 );

		// Tell the fixup system to nevermind.
		//
		if( ObjectLinker::doesExist() )
		{
			ObjectLinker::instance().removeFixupsForOwner( this );
		}
		
		if( m_weakPtrProxy )
		{
			m_weakPtrProxy->onReferentDeleted();
			m_weakPtrProxy = nullptr;
		}
	}

	void Object::rename( NameRef newName )
	{
		m_name = newName;
		if( m_name == DEFAULT_OBJECT_NAME )
		{
			createDefaultName( m_name );
		}
	}
	
	ClassNameRef Object::className() const
	{
		return classInfo().className();
	}
	
	Object& Object::StaticGetDefaultObject()
	{
		auto theDefaultObject = StaticGetClassInfo().defaultObject();
		ASSERT( theDefaultObject );
		return static_cast< Object& >( *theDefaultObject );
	}
	
	fr::ClassInfo& Object::StaticGetClassInfo()
	{
		static ClassInfo::ptr myClass = createNativeClass< Object >( nullptr, "Object", ClassInfo::Placeable::False );
		return *myClass;
	}
	
	bool Object::setPropertyValue( const std::string& propertyName, const std::string& strValue )
	{
		// Find the property.
		//
		auto property = classInfo().getPropertyByName( propertyName );
		if( property )
		{
			property->setValueByString( this, strValue );
		}
		else
		{
			con_warning( this << ": attempt to assign unknown property '" << propertyName << "'." );
		}
		
		return property;
	}
	
	std::string Object::getPropertyValue( const std::string& propertyName ) const
	{
		// Find the property.
		//
		auto property = classInfo().getPropertyByName( propertyName );
		if( property )
		{
			return property->getValueByString( this );
		}
		else
		{
			con_warning( this << ": attempt to read unknown property '" << propertyName << "'." );
			return "";
		}
	}
	
	std::string Object::toString() const
	{
		return objectId().toString();
	}

	ObjectId Object::objectId() const
	{
		const auto self = this;
		if( self )
		{
			return ObjectId( className(), name() );
		}
		else
		{
			return ObjectId::NULL_OBJECT;
		}
	}
	
	bool Object::isA( ClassInfo::NameRef classIdFilter, bool exactMatch ) const
	{
		return classIdFilter.empty() || classInfo().isKindOf( classIdFilter, exactMatch );
	}

	bool Object::isA( const ClassInfo& aClass ) const
	{
		return classInfo().isKindOf( aClass );
	}

	bool Object::matchesFilters( ClassInfo::NameRef classIdFilter, NameRef objectIdFilter ) const
	{
		return isA( classIdFilter ) &&		// Class matches.
			   ( objectIdFilter.empty() || matchesFilter( name(), objectIdFilter ));	// Object name matches
	}

	int Object::release() const
	{
		--m_nReferences;
		
		ASSERT_MSG( m_nReferences >= 0, "For object: " << toString() << "..." );
		
		if( m_nReferences == 0 )
		{
			// Time to delete me.
			//
			::delete this;
			
			return 0;
		}
		
		return m_nReferences;
	}

	void Object::createDefaultName( std::string& outName )
	{
		// Choose a unique name for this object.
		//
		
		std::ostringstream ss;
		
		ss << s_nObjectsCreated;
		outName = ss.str();
		
		if( s_useTimeCodedDefaultNames )
		{
			time_t theTime = std::time( nullptr );
			
			while( theTime > 0 )
			{
				outName += 'a' + ( theTime & 0xF );		// Code in the least significant 4 bits.
				theTime >>= 4;							// Chop off the least significant 4 bits.
			}
		}
		
	}
	
	void Object::load( const Manifest::Map& properties )
	{
		TIMER_AUTO( Object::load( Manifest::Map ))
		
		trace_loading( "Loading " << this );
		
		checkDebugBreakpoint( "load", *this )
		
		// For each property in the object, look for the property in the element and if found, assign it.
		
		const ClassInfo& myClassInfo = classInfo();
		
		pushCurrentLoadingObject( this );

		for( const auto& mapping : properties )
		{
			const std::string& childName = mapping.first;
			
			// Load the property.
			//
			if( childName != "passthrough" )
			{
				auto property = myClassInfo.getPropertyByName( childName );
				
				if( !property )
				{
					dev_warning( "While loading " << objectId() << ", found an unrecognized property '" << childName << "'." );
				}
				else
				{
					if(( !isInert() || property->shouldLoadDefaults() ))
					{
						property->setValueByManifestValue( this, *( mapping.second.first ));
						trace_loading( "Assigned property '" << childName << "' to " << property->getValueByString( this ));
					}
				}
			}
		}
		popCurrentLoadingObject();
		
		trace_loading( "Done loading " << this );
	}

	void Object::serialize( ObjectStreamFormatter& formatter, bool doWriteEvenIfDefault ) const
	{
		const ClassInfo& myClassInfo = classInfo();

		formatter.beginObject( this );
		
		const ClassInfo::PropertyIterator endIter = myClassInfo.getPropertyIteratorEnd();
		for( ClassInfo::PropertyIterator iter = myClassInfo.getPropertyIteratorBegin();
			iter != endIter;
			++iter )
		{
			const PropertyAbstract& prop = *iter;
			
			// Only write out this property if it is different from the default for this class.
			//
			if( !prop.isTransient() &&																		// We don't save transient properties.
			   ( !shallow() || !prop.deep() ) &&															// We don't save pointers (and other "deep" properties) if we're shallow.
			   ( doWriteEvenIfDefault || !prop.doesObjectHaveDefaultValue( this )))							// We don't save properties that still have the default value, unless it's forced.
			{
				prop.saveToFormatter( formatter, this );
			}
		}
		
		formatter.endObject( this );
	}
	
	Object::ptr Object::createClone( NameRef objectName ) const
	{
#if	0		// TODO: Reinstate eventually...?
		
		// Serialize properties.
		//
		std::ostringstream out;
		Stringifier stringifier( out );
		ObjectStreamFormatterXml formatter( stringifier );
		serialize( formatter );
		
		std::unique_ptr< XmlElement > xmlElement = stringToXmlElement( out.str() );
		
		// Create a copy.
		//
		auto copy = createObject( *xmlElement );
		ASSERT( copy );
		
		// Rename it.
		//
		copy->rename( objectName );

		return copy;
#endif
		return nullptr;
	}
	
	void Object::postLoad()
	{
		checkDebugBreakpoint( "postLoad", *this )		
	}
	
	std::string Object::call( const std::string& methodName, std::istream& in )
	{
		const ClassInfo& myClassInfo = classInfo();
		auto function = myClassInfo.getMethodByName( methodName );
		if( function )
		{
			try
			{
				return (*function)( this, in );
			}
			catch( std::exception& e )
			{
				trace( "Call to " << this << "." << methodName << "() failed with exception: " << e.what() );
			}
		}
		else
		{
			trace( "Unrecognized method `" << methodName << "` for " << this );
		}
		return std::string();
	}
}
