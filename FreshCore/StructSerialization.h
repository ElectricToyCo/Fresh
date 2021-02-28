//
//  StructSerialization.h
//  Fresh
//
//  Created by Jeff Wofford on 11/16/12.
//
//

#ifndef Fresh_StructSerialization_h
#define Fresh_StructSerialization_h

#include <map>
#include <string>
#include <memory>
#include <iostream>
#include <iomanip>
#include <cassert>
#include "Archive.h"

namespace fr
{
	template< class struct_t, typename ostream_t = fr::Stringifier, typename istream_t = fr::Destringifier >
	class SerializableStruct
	{
	public:
		
		bool hasProperties() const { return getPropertyRegistry().hasProperties(); }
		
		void write( ostream_t& out ) const
		{
			getPropertyRegistry().write( out, this );
		}
		
		void read( istream_t& in )
		{
			getPropertyRegistry().read( in, this );
		}
		
	protected:
		
		template< typename prop_t >
		void addProperty( const char* propName, ptrdiff_t propOffset ) { getPropertyRegistry().addProperty( propName, std::make_shared< StructProperty< prop_t > >( propOffset )); };

	private:
		
		class StructPropertyAbstract
		{
		public:
			StructPropertyAbstract( ptrdiff_t propOffset ) : m_offset( propOffset ) {}
			virtual ~StructPropertyAbstract() {}
			
			virtual void write( ostream_t& out, const void* pStruct ) const = 0;
			virtual void read( istream_t& in, void* pStruct ) const = 0;
			
		protected:
			
			ptrdiff_t offset() const { return m_offset; }
			
		private:
			ptrdiff_t m_offset;
		};
		
		template< typename prop_t >
		class StructProperty : public StructPropertyAbstract
		{
		public:
			StructProperty( ptrdiff_t propOffset ) : StructPropertyAbstract( propOffset ) {}
			
			virtual void write( ostream_t& out, const void* pStruct ) const override
			{
				assert( pStruct );
				const prop_t* pProp = reinterpret_cast< const prop_t* >( reinterpret_cast< const char* >( pStruct ) + this->offset() );
				out << *pProp;
			}
			
			virtual void read( istream_t& in, void* pStruct ) const override
			{
				assert( pStruct );
				prop_t* pProp = reinterpret_cast< prop_t* >( reinterpret_cast< char* >( pStruct ) + this->offset() );
				in >> *pProp;
			}
		};
		
		class PropertyRegistry
		{
		public:
			
			bool hasProperties() const { return !m_properties.empty(); }
			
			void addProperty( const char* propName, std::shared_ptr< StructPropertyAbstract > prop ) { ASSERT( prop ); m_properties[ propName ] = prop; }
			
			std::shared_ptr< const StructPropertyAbstract > getProperty( const char* propName ) const
			{
				auto iter = m_properties.find( propName );
				if( iter == m_properties.end() )
				{
					dev_error( "Could not find property '" << propName << "'." );
					return nullptr;
				}
				return iter->second;
			}
			
			template< typename OStream >
			void write( OStream& out, const void* pStruct ) const
			{
				out << "{";
				std::for_each( m_properties.begin(), m_properties.end(), [&] ( const std::pair< std::string, std::shared_ptr< StructPropertyAbstract > >& propPair )
							  {
								  out << propPair.first << "=";
								  propPair.second->write( out, pStruct );
								  out << " , ";
							  } );
				out << "}";
			}
			
			template< typename IStream >
			void read( IStream& in, void* pStruct ) const
			{
				in.gobbleStrings( false );
				in.stream() >> std::ws;
				if( in.peek() != '{' )
				{
					dev_error( "Struct lacked leading {." );
				}
				else
				{
					in.get();
				}
				
				while( in && !in.eof() )
				{
					std::string propName;
					std::getline( in.stream(), propName, '=' );
					trim( propName );
					auto property = getProperty( propName.c_str() );
					if( !property )
					{
						dev_error( "Struct lacked indicated property '" << propName << "'." );
					}
					else
					{
						property->read( in, pStruct );
					}

					in.stream() >> std::ws;
					if( in.peek() == ',' || in.peek() == '|' )
					{
						in.get();
						in.stream() >> std::ws;
					}
					
					if( in.peek() == '}' )
					{
						break;
					}
				}
				in.stream() >> std::ws;
				if( in.peek() != '}' )
				{
					dev_error( "Struct lacked trailing }." );
				}
				else
				{
					in.get();
				}
			}
			
		private:
			
			std::map< std::string, std::shared_ptr< StructPropertyAbstract > > m_properties;
		};
		
		static PropertyRegistry& getPropertyRegistry()
		{
			static PropertyRegistry propertyRegistry;
			return propertyRegistry;
		}
	};

#define STRUCT_BEGIN_PROPERTIES if( !hasProperties() ) {
#define STRUCT_ADD_PROPERTY( propName ) addProperty< decltype( propName ) >( #propName, reinterpret_cast< char* >( &( this->propName )) - reinterpret_cast< char* >( this ) );;
#define STRUCT_END_PROPERTIES }
#define STRUCT_DECLARE_SERIALIZATION_OPERATORS( structType ) \
	friend fr::Stringifier& operator<<( fr::Stringifier& out, const structType& instance );	\
	friend fr::Destringifier& operator>>( fr::Destringifier& in, structType& instance );
#define STRUCT_DEFINE_SERIALIZATION_OPERATORS( structType )	\
	fr::Stringifier& operator<<( fr::Stringifier& out, const structType& instance ) { instance.write( out ); return out; }	\
	fr::Destringifier& operator>>( fr::Destringifier& in, structType& instance ) { instance.read( in ); return in; }

	
#if 0
	// USE CASE
	
	struct AttractorRepulsor : public SerializableStruct< AttractorRepulsor >
	{
		std::string	name = "purpulescence of fright";
		float		power = 0;
		bool		doesUseInverseDistance = false;
		float		lateralDisplacement = 0;
		
		AttractorRepulsor()
		{
			STRUCT_BEGIN_PROPERTIES
			STRUCT_ADD_PROPERTY( location );
			STRUCT_ADD_PROPERTY( power );
			STRUCT_ADD_PROPERTY( doesUseInverseDistance );
			STRUCT_ADD_PROPERTY( lateralDisplacement );
			STRUCT_END_PROPERTIES
		}
	};
	STRUCT_DECLARE_SERIALIZATION_OPERATORS( AttractorRepulsor )

	// ...
	
	STRUCT_DEFINE_SERIALIZATION_OPERATORS( AttractorRepulsor )
	
	void main()
	{
		// Vague test: suggestive rather than complete or correct.
		
		std::istringstream in( "{ name=bugger , power=4.56, doesUseInverseDistance=true, lateralDisplacement = 5.1 }" );
		AttractorRepulsor ar;
		in >> ar;

		assert( ar.name == "bugger" );
		assert( ar.power == 4.56f );				// Approximately so, anyway.
		assert( ar.doesUseInverseDistance );
		assert( ar.lateralDisplacement == 5.1f );	// Approximately so, anyway.
		
		std::ostringstream out;
		out << ar;
		assert( out.str() == "{name=bugger , power=4.56 , doesUseInverseDistance=true , lateralDisplacement = 5.1 , }" );
	}
	
#endif
	
}

#endif
