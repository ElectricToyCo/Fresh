//
//  Classes.cpp
//  Fresh
//
//  Created by Jeff Wofford on 2/27/13.
//
//

#include "Classes.h"
#include "FreshException.h"
#include "Object.h"
#include "CommandProcessor.h"
#include <map>

namespace
{
	using namespace fr;

	bool g_isReflectionInitialized = false;
	
	typedef std::map< ClassInfo::Name, std::unique_ptr< ClassInfo >> Classes;
	
	Classes& getClasses()
	{
		static Classes classes;
		return classes;
	}
	
#if TABULAR_CLASS_IS_KIND_OF
	size_t numClasses()
	{
		return getClasses().size();
	}
	
	class ClassInheritanceMatrix
	{
	public:
		
		void addClass( const ClassInfo& classInfo )
		{
			const size_t nClasses = numClasses();
			const size_t ordinal = classInfo.ordinal();
			
			std::vector< int > depths( nClasses, -1 );
			
			ClassInfo::cptr super = &classInfo;
			int depth = 0;
			while( super )
			{
				const size_t superClassOrdinal = super->ordinal();
				ASSERT( superClassOrdinal < depths.size() );
				depths[ superClassOrdinal ] = depth;
				
				super = super->getSuperClass();
				++depth;
			}
			
			m_classInheritanceDepths.insert( m_classInheritanceDepths.begin() + ordinal, std::move( depths ));
		}
		
		int inheritanceDepth( const ClassInfo& maybeDerived, const ClassInfo& maybeBase ) const
		{
			// Returns:
			//		0				<=	identical class
			//		x where x < 0	<=	no inheritance relationship.
			//		x where x > 0	<=	maybeDerived is x steps of inheritance derived from maybeBase.
			
			const size_t derivedOrdinal = maybeDerived.ordinal();
			const size_t baseOrdinal = maybeBase.ordinal();
			
			ASSERT( derivedOrdinal < m_classInheritanceDepths.size() );
			const auto& depths = m_classInheritanceDepths[ derivedOrdinal ];
			
			if( baseOrdinal < depths.size() )
			{
				return depths[ baseOrdinal ];
			}
			else
			{
				// Can't be a base because it doesn't exist in this class's array.
				return -1;
			}
		}
		
	private:
		
		std::vector< std::vector< int >> m_classInheritanceDepths;
	};
	
	ClassInheritanceMatrix& inheritanceMatrix()
	{
		static ClassInheritanceMatrix matrix;
		return matrix;
	}
#endif
}

namespace fr
{
	ClassInfo::ptr generalizedCreateClass( ClassInfo::NameRef pseudoclassName, ClassInfo::NameRef baseClassName, const Manifest::Map& configuration, ClassInfo::Placeable isPlaceable )
	{
#ifdef FRESH_VERBOSE_REFLECTION
		dev_trace( "Creating class " << pseudoclassName << " extends " << baseClassName );
#endif
		
		if( pseudoclassName.empty())
		{
			FRESH_THROW( FreshException, "Pseudoclass deriving from base class " << baseClassName << " must have a name." );
		}
		
		if( pseudoclassName == baseClassName )
		{
			FRESH_THROW( FreshException, "Pseudoclass " << pseudoclassName << " cannot be its own base class." );
		}
		
		if( isClass( pseudoclassName ))
		{
			FRESH_THROW( FreshException, "Attempt to re-add pseudoclass " << pseudoclassName << " ignored." );
		}
		
		if( !isClass( baseClassName ))
		{
			FRESH_THROW( FreshException, "Pseudoclass " << pseudoclassName << " had unrecognized base class " << baseClassName );
		}
		
		ClassInfo::ptr base = getClass( baseClassName );
		ASSERT( base );
		
		if( base->isAbstract() )
		{
			FRESH_THROW( FreshException, "Pseudoclass " << pseudoclassName << " cannot extend abstract class " << baseClassName );
		}
		
		std::unique_ptr< ObjectFactoryBase > baseFactoryClone( base->factory().createFactoryClone() );
		
		// Create the new class.
		//
		ClassInfo::ptr newClass = new ClassInfo( base, pseudoclassName, std::move( baseFactoryClone ), &configuration, false /* non-native */, isPlaceable );
		ASSERT( !newClass->isNative() );
		ASSERT( !newClass->isAbstract() );
		
		return newClass;
	}

	inline void concludeCreateClass( ClassInfo::ptr newClass )
	{
		// If we've already concluded reflection initialization, conclude it for this class now.
		//
		if( g_isReflectionInitialized )
		{
			newClass->concludeInitialization();
			ASSERT( newClass->defaultObject() );
		}
		
		// Register the class.
		//
		registerClass_implementation( newClass );
	}

	void initReflection()
	{
#ifdef FRESH_VERBOSE_REFLECTION
		dev_trace( "initReflection()" );
#endif
		
		if( !g_isReflectionInitialized )
		{
			Classes& classes = getClasses();
			
			// Ask each class to finish its initialization.
			//
			for( const auto& pair : classes )
			{
				pair.second->concludeInitialization();
			}
			
			g_isReflectionInitialized = true;
		}
	}
	
	bool isClass( ClassInfo::NameRef className )
	{
		return getClasses().find( className ) != getClasses().end();
	}
	
	ClassInfo::ptr getClass( ClassInfo::NameRef className )
	{
		Classes& classes = getClasses();
		auto iter = classes.find( className );
		if( iter != classes.end() )
		{
			return iter->second.get();
		}
		else
		{
			return nullptr;
		}
	}

	void forEachClass( std::function< void( ClassInfo& ) >&& fnPerClass, ClassInfo::cptr baseClass )
	{
		const Classes& classes = getClasses();
		
		for( const auto& pair : classes )
		{
			if( !baseClass || pair.second->isKindOf( *baseClass ))
			{
				fnPerClass( *pair.second );
			}
		}
	}	

	ClassInfo::ptr registerClass_implementation( ClassInfo::ptr classInfo )
	{
		REQUIRES( classInfo );
		REQUIRES( !isClass( classInfo->className() ));
		
#ifdef FRESH_VERBOSE_REFLECTION
		dev_trace( "Registering class " << classInfo->className() );
#endif
		
		auto& classes = getClasses();
		
		classInfo->m_ordinal = classes.size();
		classes[ classInfo->className() ].reset( classInfo );
		
#if TABULAR_CLASS_IS_KIND_OF
		inheritanceMatrix().addClass( *classInfo );
#endif
		
		PROMISES( isClass( classInfo->className() ));
		
		return classInfo;
	}
	
	ClassInfo::ptr createClass( ClassInfo::NameRef pseudoclassName, ClassInfo::NameRef baseClassName, const Manifest::Class& classDirective, ClassInfo::Placeable isPlaceable )
	{
		ASSERT( classDirective.map );
		const Manifest::Map& propertyMap = *classDirective.map;
		
		auto newClass = generalizedCreateClass( pseudoclassName, baseClassName, propertyMap, isPlaceable );
		
		// Mark properties that are specified by the configuration as "overrides".
		//
		for( const auto& mapping : propertyMap )
		{
			const std::string& propertyName = mapping.first;
			if( propertyName != "passthrough" )
			{
				unsigned int flags = PropFlag::None;

				// Evaluate property attributes.
				//
				const auto& attributes = mapping.second.second;
				for( const auto& attribute : attributes )
				{
					if( attribute == "transient" )
					{
						flags |= PropFlag::Transient;
					}
					else if( attribute == "noedit" )
					{
						flags |= PropFlag::NoEdit;
					}
					else
					{
						con_warning( "Unrecognized attribute '" << attribute << "' for property '" << propertyName << "' for new class " << pseudoclassName << "." );
					}
				}
				
				auto property = newClass->getPropertyByName( propertyName );
				if( !property )
				{
					con_error( "Unrecognized property '" << propertyName << "' for new class " << pseudoclassName << " extending " << baseClassName << "." );
				}
				else
				{
					newClass->addProperty( property->createOverride( *newClass, flags ) );
				}
			}
		}

		concludeCreateClass( newClass );
		
		PROMISES( isClass( pseudoclassName ));
		
		return newClass;
	}

	bool isClassKindOf( const ClassInfo& maybeDerived, const ClassInfo& maybeBase )
	{
#if TABULAR_CLASS_IS_KIND_OF
		return inheritanceMatrix().inheritanceDepth( maybeDerived, maybeBase ) >= 0;
#else
		ASSERT( false );	// You must #define TABULAR_CLASS_IS_KIND_OF to call this function.
		return 0;
#endif
	}
}
