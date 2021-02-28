/*
 *  TypeTraits.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 11/3/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_TYPE_TRAITS_H_INCLUDED
#define FRESH_TYPE_TRAITS_H_INCLUDED

#include <limits>
#include "Objects.h"
#include "Archive.h"

namespace fr
{
	
	template< typename PropertyT >
	void assignObjectProperty( PropertyT& property, const Manifest::Value& value )
	{
		if( value.is< std::string >() )
		{
			Destringifier destringifier( value.get< std::string >() );
			destringifier >> property;
		}
		else
		{
			FRESH_THROW( FreshException, "Can't assign non-string value to this property." );
		}
	}

	template< typename PointerT >
	inline void assignObjectPointer( PointerT& pointer, const Manifest::Value& value )
	{
		if( value.is< Manifest::Object >() )
		{
			const Manifest::Object& manifestObject = value.get< Manifest::Object >();
			pointer = createOrGetObject< typename PointerT::element_type >( manifestObject );
		}
		else if( value.is< std::string >() )
		{
			Destringifier destringifier( value.get< std::string >() );
			destringifier >> pointer;
		}
		else
		{
			FRESH_THROW( FreshException, "Can't assign non-string/non-object value to this object pointer." );
		}
	}
	
	template< typename ObjectT >
	inline void assignObjectProperty( SmartPtr< ObjectT >& property, const Manifest::Value& value )
	{
		assignObjectPointer( property, value );
	}
	
	template< typename ObjectT >
	inline void assignObjectProperty( WeakPtr< ObjectT >& property, const Manifest::Value& value )
	{
		assignObjectPointer( property, value );
	}
	
	template< typename ElementT >
	inline void assignObjectProperty( std::vector< ElementT >& property, const Manifest::Value& value )
	{
		if( value.is< Manifest::Array >() )
		{
			const Manifest::Array& array = value.get< Manifest::Array >();
			
			property.clear();
			property.resize( array.size() );
			
			for( size_t i = 0; i < array.size(); ++i )
			{
				const Manifest::Value& subValue = *array[ i ];
				
				try
				{
					assignObjectProperty( property[ i ], subValue );
				}
				catch( const std::exception& e )
				{
					FRESH_THROW( FreshException, "with array, exception with message '" << e.what() << "' for index " << i << "." );
				}
			}
		}
		else if( value.is< std::string >() )
		{
			Destringifier destringifier( value.get< std::string >() );
			destringifier >> property;
		}
		else if( value.is< Manifest::Object >() )
		{
			property.clear();
			property.resize( 1 );
			
			try
			{
				assignObjectProperty( property[ 0 ], value );
			}
			catch( const std::exception& e )
			{
				FRESH_THROW( FreshException, "with array, assigning a single object, exception with message '" << e.what() << "'." );
			}
		}
		else
		{
			FRESH_THROW( FreshException, "Can't assign non-array, non-string, non-object value to this vector property." );
		}
	}
	
	template< typename KeyT, typename ValueT >
	inline void assignObjectProperty( std::map< KeyT, ValueT >& property, const Manifest::Value& value )
	{
		if( value.is< Manifest::Map >() )
		{
			property.clear();
			
			const Manifest::Map& map = value.get< Manifest::Map >();
			
			for( const auto& pair : map )
			{
				try
				{
					Destringifier destringifier( pair.first );
					KeyT mapKey;
					destringifier >> mapKey;
					
					auto& mapValue = property[ mapKey ];
					assignObjectProperty( mapValue, *pair.second.first );
				}
				catch( const std::exception& e )
				{
					FRESH_THROW( FreshException, "with map, exception with message '" << e.what() << "' for key " << pair.first << "." );
				}
			}
		}
		else if( value.is< std::string >() )
		{
			Destringifier destringifier( value.get< std::string >() );
			destringifier >> property;
		}
		else
		{
			FRESH_THROW( FreshException, "Can't assign non-map, non-string value to this map property." );
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template< typename T, typename RefT = const T&, typename DefaultT = T >
	struct TypeTraits
	{
		typedef RefT RefType;
		typedef DefaultT DefaultType;
		
		static DefaultType defaultValue()
		{
			return DefaultType();
		}
		
		static bool shouldLoadDefaults()
		{
			return true;
		}
	};
	
	// SmartPtr
	template<typename ObjectT >
	struct TypeTraits< SmartPtr< ObjectT > >
	{
		static bool shouldLoadDefaults() { return false; }
	};
	
	// WeakPtr
	template<typename ObjectT >
	struct TypeTraits< WeakPtr< ObjectT > >
	{
		FRESH_DEFINE_EXCEPTION_TYPE( InvalidAssignment );
		
		static bool shouldLoadDefaults() { return false; }
	};
	
	// vector of SmartPtr
	template< typename ObjectT >
	struct TypeTraits< std::vector< SmartPtr< ObjectT > > >
	{
		typedef std::vector< SmartPtr< ObjectT > > vec;
		
		static bool shouldLoadDefaults() { return false; }
	};
	
	// vector of WeakPtr
	template< typename ObjectT >
	struct TypeTraits< std::vector< WeakPtr< ObjectT > > >
	{
		typedef std::vector< WeakPtr< ObjectT > > vec;
		
		static bool shouldLoadDefaults() { return false; }
	};
}


#endif
