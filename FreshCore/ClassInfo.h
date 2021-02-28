//
//  ClassInfo.h
//  Fresh
//
//  Created by Jeff Wofford on 2/25/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_ClassInfo_h
#define Fresh_ClassInfo_h

#include "PropertyAbstract.h"
#include "ObjectMethod.h"
#include "SmartPtr.h"
#include "StringTabulated.h"
#include "ObjectFactory.h"
#include "FreshManifest.h"
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <map>

namespace fr
{
	// ClassInfo describes an Object-derived class's properties and class name.
	// You never need to create these yourself--just use the FRESH_DECLARE_CLASS() and FRESH_DEFINE_CLASS() macros above.
	//
	class ClassInfo
	{
	public:
		
		typedef RawPtr< ClassInfo > ptr;
		typedef RawPtr< const ClassInfo > cptr;
		
		typedef ClassName Name;
		typedef ClassNameRef NameRef;
		
		typedef std::vector< std::unique_ptr< PropertyAbstract >> Properties;
		typedef Properties::const_iterator PropertiesCI;
		
		class PropertyIterator
		{
		public:
			PropertyIterator( cptr hostClass );
			PropertyIterator( cptr hostClass, PropertiesCI iterator );
			// REQUIRES( hostClass );
			PropertyIterator& operator++();
			PropertyAbstract& operator*() const;
			PropertyAbstract* operator->() const;
			bool operator==( const PropertyIterator& other ) const;
			bool operator!=( const PropertyIterator& other ) const			{ return !operator==( other ); }
			
		protected:
			
			void establishIterator();
			
			void advanceToNextValidIteratorOrToFinalEnd();
			
		private:
			
			std::unordered_set< PropertyName > m_visitedOverrides;
			
			cptr m_currentClass;
			PropertiesCI m_iterator;
			
			bool isOverridden( PropertiesCI iter ) const;
		};
		
		friend class PropertyIterator;
		
		//////////////////////////////////////////////
		
		NameRef className() const;
		bool isNative() const													{ return m_isNative; }
		bool isAbstract() const													{ return !m_factory; }
		bool isPlaceable() const;
		// Returns true iff objects of this class may be placed in the editor.

		// Returns a trivial, unique, immutable number associated with this class.
		size_t ordinal() const													{ return m_ordinal; }
		
		ObjectFactoryBase& factory() const										{ ASSERT( m_factory ); return *m_factory; }
		
		//
		// CLASS RELATIONSHIPS AND COMPARISONS
		//
				
		ClassInfo::ptr getSuperClass() const										{ return m_superClass; }
		// May return result such that &result == nullptr.
		
		bool isKindOf( const std::string& classIdFilter, bool exactMatch = true ) const;
		bool isKindOf( const ClassInfo& classInfo ) const;
		int getSuperClassDepth( const ClassInfo& possibleSuperClass ) const;
		// Returns the number of steps from this class "down" (base-wise, aka super-wise) to the possibleSuperClass.
		// Returns < 0 if possibleSuperClass isn't a base class of this class.
		// Returns 0 if possibleSuperClass IS this class.
		
		const ClassInfo& getCommonBase( const ClassInfo& otherClass ) const;
		
		bool operator==( const ClassInfo& otherClass ) const;
		bool operator!=( const ClassInfo& otherClass ) const					{ return !operator==( otherClass ); }
		
		//
		// PROPERTIES AND DEFAULTS
		//
		
		void addProperty( std::unique_ptr< PropertyAbstract >&& prop );
		// REQUIRES( prop );
		// REQUIRES( !prop->propName().empty() );
		// REQUIRES( ( prop->isOverride() && getSuperClass()->getPropertyByName( prop->propName() )) || !getPropertyByName( prop->propName() ));
		// PROMISES( getPropertyByName( prop->propName() ));
		
		PropertyAbstract* overrideProperty( PropertyNameRef propName, unsigned int additionalFlags = 0 );
		
		PropertyAbstract* getPropertyByName( PropertyNameRef propName ) const;
		PropertyIterator getPropertyIteratorBegin() const;
		PropertyIterator getPropertyIteratorEnd() const;
		
		Object* defaultObject() const											{ return m_defaultObject.get(); }
		// May return nullptr.
		bool isDefaultObjectDoctored() const									{ return m_isDefaultObjectDoctored; }
		void markDefaultObjectDoctored()										{ ASSERT( !m_isDefaultObjectDoctored ); m_isDefaultObjectDoctored = true; }
				
		void applyConfiguration( Object& toObject, const Manifest::Map* pendingInitialization = nullptr ) const;
		
		//
		// METHODS AND ACCESSORS
		//
		
		void addMethod( const std::string& name, std::unique_ptr< StreamedMethodAbstract >&& method );
		void forEachMethod( std::function< void( const std::string&, const StreamedMethodAbstract& ) >&& fn ) const;
		
		void addAccessor( const std::string& name, std::unique_ptr< SimpleAccessorAbstract >&& accessor );
		// REQUIRES( accessor );
		// REQUIRES( !getAccessorByName( name ) );
		
		// Return null if unfound:
		const StreamedMethodAbstract* getMethodByName( const std::string& name ) const;
		const SimpleAccessorAbstract* getAccessorByName( const std::string& name ) const;
		
		//////////////////////////////////////////////////////////////////////////////////////////
		// INTERNAL USE ONLY.
		// Don't delete objects directly. They will delete themselves as needed.
		//
		~ClassInfo();

		enum class Placeable
		{
			False,
			True,
			Inherit
		};
		
	protected:
		
		void overrideAbstractSuperClassProps();

	private:
		
		Name m_className;
		ClassInfo::ptr m_superClass;
		size_t m_ordinal = -1;
		
		std::unique_ptr< ObjectFactoryBase > m_factory;
		SmartPtr< Object > m_defaultObject;
		Manifest::Map m_configuration;
		
		Properties m_properties;
		std::unordered_map< std::string, std::unique_ptr< SimpleAccessorAbstract >> m_accessors;
		std::unordered_map< std::string, std::unique_ptr< StreamedMethodAbstract >> m_methods;

#if !TABULAR_CLASS_IS_KIND_OF
		mutable std::map< const ClassInfo*, bool > m_superClassCache;	// Speeds up isKindOf().
#endif
		bool m_isDefaultObjectDoctored = false;
		bool m_isNative = true;
		
		Placeable m_isPlaceable = Placeable::True;
		
		ClassInfo( ClassInfo::ptr superClass, NameRef szClassName, std::unique_ptr< ObjectFactoryBase >&& factory, const Manifest::Map* configuration, bool isNative = true, Placeable isPlaceable = Placeable::True );
		
		void concludeInitialization();
		
		bool isPlaceableInPrinciple() const;
		
		bool isKindOf_thorough( const ClassInfo& otherClass ) const;
		
		// defaultObject is null iff the class is abstract.
		
		template< typename class_t > friend ClassInfo::ptr createNativeClass( ClassInfo::ptr base, ClassInfo::NameRef className, Placeable isPlaceable );
		template< typename class_t > friend ClassInfo::ptr createNativeClassAbstract( ClassInfo::ptr base, ClassInfo::NameRef className );
		
		friend ClassInfo::ptr generalizedCreateClass( ClassInfo::NameRef pseudoclassName, ClassInfo::NameRef baseClassName, const Manifest::Map& configuration, ClassInfo::Placeable isPlaceable );
		friend void concludeCreateClass( ClassInfo::ptr );
		friend void initReflection();
		friend ClassInfo::ptr registerClass_implementation( ClassInfo::ptr classInfo );
		
		FRESH_PREVENT_COPYING( ClassInfo )
	};
	
	//////////////////////////////////////////////////////////////////
	// INLINES
	
	template< typename ObjectT >
	void doctorClass( std::function< void( ClassInfo& classInfo, ObjectT& defaultObject ) >&& doctoringFunction )
	{
		ClassInfo& myClassInfo = ObjectT::StaticGetClassInfo();
		if( !myClassInfo.isDefaultObjectDoctored())
		{
			doctoringFunction( myClassInfo, ObjectT::StaticGetDefaultObject() );
			myClassInfo.markDefaultObjectDoctored();
		}
	}
	
}

// Use within a doctorClass() doctoring function, *after* setting local property values. Only supports properties with setters and getters.
#define DOCTOR_PROPERTY( propName )	\
	defaultObject.propName( propName() );	\
	classInfo.overrideProperty( #propName );

// Use within a doctorClass() doctoring function, *after* setting local property values. Uses assignment of member variables.
#define DOCTOR_PROPERTY_ASSIGN( propName )	\
	defaultObject.m_##propName = m_##propName;	\
	classInfo.overrideProperty( #propName );

//////////////////////////////////////////////////////////////////
// FRESH DEFINE CLASS

// For each non-abstract, non-template Object-derived class (direct or indirect), invoke this macro somewhere in the global scope of the class's .cpp file.
//
#define FRESH_DEFINE_CLASS( class_ )	\
	class_& class_::StaticGetDefaultObject() { auto defaultObject = StaticGetClassInfo().defaultObject(); ASSERT( defaultObject ); return static_cast< class_& >( *defaultObject ); }	\
	fr::ClassInfo& class_::StaticGetClassInfo()	\
	{	\
		static fr::ClassInfo::ptr const myClass = fr::createNativeClass< class_ >( &Super::StaticGetClassInfo(), #class_, fr::ClassInfo::Placeable::Inherit );	\
		return *myClass;	\
	}	\
	const fr::ClassInitializer< class_ > class_::s_classInit_##class_;

#define FRESH_DEFINE_CLASS_PLACEABLE( class_ )	\
	class_& class_::StaticGetDefaultObject() { auto defaultObject = StaticGetClassInfo().defaultObject(); ASSERT( defaultObject ); return static_cast< class_& >( *defaultObject ); }	\
	fr::ClassInfo& class_::StaticGetClassInfo()	\
	{	\
		static fr::ClassInfo::ptr const myClass = fr::createNativeClass< class_ >( &Super::StaticGetClassInfo(), #class_, fr::ClassInfo::Placeable::True );	\
		return *myClass;	\
	}	\
	const fr::ClassInitializer< class_ > class_::s_classInit_##class_;

#define FRESH_DEFINE_CLASS_UNPLACEABLE( class_ )	\
	class_& class_::StaticGetDefaultObject() { auto defaultObject = StaticGetClassInfo().defaultObject(); ASSERT( defaultObject ); return static_cast< class_& >( *defaultObject ); }	\
	fr::ClassInfo& class_::StaticGetClassInfo()	\
	{	\
		static fr::ClassInfo::ptr const myClass = fr::createNativeClass< class_ >( &Super::StaticGetClassInfo(), #class_, fr::ClassInfo::Placeable::False );	\
		return *myClass;	\
	}	\
	const fr::ClassInitializer< class_ > class_::s_classInit_##class_;

// For each non-abstract, *templated* Object-derived class (direct or indirect) with one template parameter, invoke this macro somewhere in the global scope of the class's .cpp file.
//
#define FRESH_GET_CLASS_NAME_TEMPLATE_1( className, templateType1 )	STRINGIFY( className##_##templateType1 )

#define FRESH_DEFINE_CLASS_TEMPLATE_1( class_, templateType1 )	\
	template<>	\
	fr::ClassInfo& class_< templateType1 >::StaticGetClassInfo()	\
	{	\
		static fr::ClassInfo::ptr const myClass = fr::createNativeClass< class_< templateType1 > >( &Super::StaticGetClassInfo(), FRESH_GET_CLASS_NAME_TEMPLATE_1( class_, templateType1 ), fr::ClassInfo::Placeable::Inherit );	\
		return *myClass;	\
	}	\
	template<>	\
	class_< templateType1 >& class_< templateType1 >::StaticGetDefaultObject() { auto defaultObject = StaticGetClassInfo().defaultObject(); ASSERT( defaultObject ); return static_cast< class_< templateType1 >& >( *defaultObject ); }	\
	template<>	\
	const fr::ClassInitializer< class_< templateType1 > > class_< templateType1 >::s_classInit_##class_;	\
	const fr::ClassInitializer< class_< templateType1 > > g_classInit_##class_##_##templateType1;
//
// NOTE: Templates need an explicit instantiation; hence the g_classInit_... variable above.

#endif
