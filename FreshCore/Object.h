#ifndef FRESH_OBJECT_H_INCLUDED_
#define FRESH_OBJECT_H_INCLUDED_

//
// Copyright 2005-2009 Jeff Wofford
//
// The Object class is the heart of the Fresh engine.
// Objects know how to participate in automatic reference-counted memory cleanup.
// Objects have string names, useful for debugging, searching, and other purposes.
// Objects know how to produce a friendly string version of themselves (using toString()).
// Objects know how to serialize and deserialize themselves (though the deserialization side of this feature in in progress).
// You can also use ObjectManager::createObject() to create an instance of any Object-derived class simply by a string class identifier. This is handy for scripting and automatic deserialization features.

#include "ObjectPtr.h"
#include "ObjectId.h"
#include "FreshManifest.h"

// For each non-abstract Object-derived class (derived directly or indirectly), invoke this macro at the bottom of the class's declaration.
// You must also invoke the FRESH_DEFINE_CLASS() macro. See below.
// The macro provides:
//		* handy typedefs for the unique strong and weak pointer types for the class (::ptr and ::wptr).
// 		* the className() function for the class, which returns the string name of the class this object instantiates.
// The macro also guarantees construction and registration of the class factory, such that instances of this class may be created using
// ObjectManager's createObject() function.
//
#define FRESH_DECLARE_CLASS( class_, superClass )	\
public:	\
	typedef superClass Super;	\
	typedef class_ ConcreteSuper;	\
	FRESH_DECLARE_CLASS_POINTERS( class_ )	\
	static class_& StaticGetDefaultObject();	\
	static fr::ClassInfo& StaticGetClassInfo();	\
protected:	\
	explicit class_( fr::CreateInertObject c );	\
	explicit class_( const fr::ClassInfo& assignedClassInfo, NameRef objectName = fr::DEFAULT_OBJECT_NAME );	\
private:	\
	static const fr::ClassInitializer< class_ > s_classInit_##class_;	\
	FRESH_PREVENT_COPYING( class_ )	\
	friend class fr::ObjectFactory< class_ >;


// For each ABSTRACT Object-derived class (derived directly or indirectly), invoke this macro at the bottom of the class's declaration.
// If you use this DECLARE macro, you should *not* invoke the FRESH_DEFINE_CLASS() macro.
// Abstract classes are not constructable, therefore they do not have factories.
// If you use FRESH_DECLARE_CLASS() on an abstract class, you'll get a linker error.
//
#define FRESH_DECLARE_CLASS_ABSTRACT( class_, superClass )	\
public:	\
	typedef superClass Super;	\
	typedef superClass::ConcreteSuper ConcreteSuper;	\
	FRESH_DECLARE_CLASS_POINTERS( class_ )	\
	inline static fr::ClassInfo& StaticGetClassInfo()	\
	{	\
		static fr::ClassInfo::ptr myClass = fr::createNativeClassAbstract< class_ >( &Super::StaticGetClassInfo(), #class_ );	\
		return *myClass;	\
	}	\
protected:	\
	explicit class_( fr::CreateInertObject c );	\
	explicit class_( const fr::ClassInfo& assignedClassInfo, NameRef objectName = fr::DEFAULT_OBJECT_NAME );	\
private:	\
	FRESH_PREVENT_COPYING( class_ )	\
	friend class fr::ObjectFactory< class_ >;

//////////////////////////////////////////////////////////////////
// Constructor implementation help

#define FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( class_ )	\
class_::class_( fr::CreateInertObject c ) : Super( c ) { ASSERT( isInert() ); }	\

#define FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_NAMING( class_ )	\
class_::class_( const fr::ClassInfo& assignedClassInfo, fr::Object::NameRef objectName )	\
:	Super( assignedClassInfo, objectName )	\
{}

#define FRESH_CUSTOM_STANDARD_CONSTRUCTOR_NAMING( class_ )	\
class_::class_( const fr::ClassInfo& assignedClassInfo, fr::Object::NameRef objectName )	\
:	Super( assignedClassInfo, objectName )

#define FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( class_ )	\
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( class_ )	\
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_NAMING( class_ )

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace fr
{

	// Forward declaration for FRESH_DECLARE_CLASS's benefit.
	template< typename T >
    class ObjectFactory;
	
	template< typename class_t >
	struct ClassInitializer
	{
		ClassInitializer();
	};
		
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Class Object, the core of the system.
	//
	// Generally speaking, any class that has interesting lifecycle needs should be derived from Object.
	// This enables instances of that class to be automatically deleted based on reference counted smart pointers.
	//
	// Objects also support weak pointers. These are automatically nullified when the object they point to is deleted.
	//
	// DO NOT CREATE OBJECT-DERIVED OBJECTS ON THE STACK OR IN GLOBAL MEMORY.
	// That is, don't do this: 		Object a; 
	// The Object class firmly expects to be constructed on the heap (using *new*), not on the stack or as global data.
	//
	// Also, it is possible, but inadvisable, to *delete* an Object directly. Rather, use SmartPtrs to reference Objects
	// and they will be deleted automatically when they are no longer needed.
	//
	class ClassInfo;
		
	class Object
	{
	public:
		
		FRESH_DECLARE_CLASS_POINTERS( Object )	// Define Object::ptr, Object::wptr, etc.
		
		// ConcreteSuper is the class you should create if you want to create an instance of this class.
		// (Abstract classes finagle this typedef to indicate their first-non-abstract superclass.
		//
		typedef Object ConcreteSuper;

		//
		// OBJECT NAME
		//
		typedef ObjectName Name;
		typedef ObjectNameRef NameRef;
		
		ALWAYS_INLINE NameRef name() const						{ return m_name; }
		ALWAYS_INLINE bool hasName( NameRef name ) const		{ return m_name == name; }
		
		// Renames the object. Do not call this lightly. In fact, stay away from it.
		// The system makes various assumptions about the relationship of object names to each other
		// and the persistence of an object's name over time. Don't call rename() unless you are
		// REALLY confident that you are honoring these assumptions.
		void rename( NameRef newName );
		// REQUIRES( newName != DEFAULT_OBJECT_NAME );

		//
		// OBJECT CLASS and CLASS METADATA
		//
		static Object& StaticGetDefaultObject();
		static ClassInfo& StaticGetClassInfo();
		
		const ClassInfo& classInfo() const						{ ASSERT( m_classInfo ); return *m_classInfo; }
		ClassNameRef className() const;

		bool isA( ClassNameRef classIdFilter, bool exactMatch = true ) const;
		bool isA( const ClassInfo& aClass ) const;
			// REQUIRES( class );
		
		template< typename DerivedT >
		SmartPtr< DerivedT > as();
		
		template< typename DerivedT >
		SmartPtr< const DerivedT > as() const;
		
		// Calls the indicated method on this object, reading method parameter values from *in*.
		std::string call( const std::string& methodName, std::istream& in );
		
		virtual bool setPropertyValue( const std::string& propertyName, const std::string& strValue );	// Returns true iff the property exists and the assignment succeeded.
		virtual std::string getPropertyValue( const std::string& propertyName ) const;
		
		//
		// OBJECT ID and SEARCH SUPPORT
		//
		virtual std::string toString() const;	
		ObjectId objectId() const;
		
		bool matchesFilters( ClassNameRef classIdFilter, NameRef objectIdFilter ) const;

		//
		// REFERENCE COUNTING
		//
		ALWAYS_INLINE void addReference() const;
		int release() const;
		ALWAYS_INLINE int getReferenceCount() const				{ return m_nReferences; }

		//
		// SERIALIZATION
		//
		SYNTHESIZE( bool, shallow )		// Shallow objects do not save pointers to objects.
		virtual void load( const Manifest::Map& properties );
		virtual void serialize( class ObjectStreamFormatter& formatter, bool doWriteEvenIfDefault = false ) const;

		virtual Object::ptr createClone( NameRef objectName = fr::DEFAULT_OBJECT_NAME ) const;
		
		//
		// INTERNALS. You really shouldn't care.
		//
		static void createDefaultName( std::string& outName );
		static void useTimeCodedDefaultNames( bool use )		{ s_useTimeCodedDefaultNames = use; }
		
		SmartPtr< WeakPtrProxy > getWeakPtrProxy()	const		{ ASSERT( m_weakPtrProxy ); return m_weakPtrProxy; }
		
		bool isInert() const									{ return m_isInert; }

		//
		// OVERRIDES for SUBCLASSES
		//
		
		// postLoad() is called during manifest loading or individual object creation.
		// It is guaranteed to be called after pointer fixup.
		// The order of postLoad() calls to various objects is undefined.
		// You can assume that the current object is fully constructed apart from whatever postLoad() does,
		// but not that any other object is constructed. For that, use onAllLoaded().
		virtual void postLoad();
		
		// Called after all objects in a manifest have been loaded and post-loaded, or as the last step of
		// initializing a single created object.
		virtual void onAllLoaded() {}
		
		
	protected:

		explicit Object( const ClassInfo& assignedClassInfo, NameRef objectName = DEFAULT_OBJECT_NAME );
		explicit Object( CreateInertObject );

		virtual ~Object();
		
		void assignClassInfo( const ClassInfo& ci )				{ ASSERT( !m_classInfo ); m_classInfo = &ci; }

	private:

		Name m_name;
		const ClassInfo* m_classInfo = nullptr;
		
		bool m_isInert = false;
		bool m_shallow = false;

		mutable int m_nReferences = 0;
		
		SmartPtr< WeakPtrProxy > m_weakPtrProxy = new WeakPtrProxy();
		
		static size_t s_nObjectsCreated;
		static bool s_useTimeCodedDefaultNames;

		static const ClassInitializer< Object > s_classInit_Object;
		
		FRESH_PREVENT_COPYING( Object )

		friend class ClassInfo;
		template< typename T > friend class ObjectFactory;
	};
	
	////////////////////////////////////////////////////////
	
	std::ostream& operator<<( std::ostream& out, const Object* obj );

	template< class ObjectT >
	std::ostream& operator<<( std::ostream& out, const SmartPtr< ObjectT >& obj );
	
	template< class ObjectT >
	std::ostream& operator<<( std::ostream& out, const WeakPtr< ObjectT >& obj );
	
	/////////////////////////////////////////////////////////////////////////////////
	// "LOAD STACK" SUPPORT
	
	size_t loadStackDepth();
	Object::ptr loadStackTop();
	Object::ptr loadStackFromTop( size_t depth );
		// REQUIRES( depth < loadStackDepth() );

	void pushCurrentLoadingObject( Object::ptr object );
	void popCurrentLoadingObject();

	Object::Name parseObjectName( Object::NameRef name );
	
} //	END namespace fr

////////////////////////////////////////////////////////////

#include "Object.inl.h"

#endif
