//
//  VertexStructure.h
//  Fresh
//
//  Created by Jeff Wofford on 11/26/11.
//  Copyright (c) 2011 jeffwofford.com. All rights reserved.
//

#ifndef Fresh_VertexStructure_h
#define Fresh_VertexStructure_h

#include "Asset.h"
#include "StructSerialization.h"
#include "Archive.h"

namespace fr
{
	class ShaderProgram;
	class VertexStructure : public Asset
	{
	public:
		
		typedef unsigned char Raw;
		
		enum ComponentType
		{
			Float,
			UnsignedByte
		};
		
		enum AttributeUsage
		{
			Position,
			Normal,
			Color,
			TexCoord,
			Other,
			Ignore,
		};
		
		virtual void apply( size_t offsetInBytes = 0 ) const;
		virtual void bindProgramAttributes( ShaderProgram& program ) const;
		
		static size_t getMaxAttributes();
		
		size_t getVertexSizeInBytes() const;
		
		void addAttribute( const std::string& attributeName, int nComponents, ComponentType type, AttributeUsage usage, int usageIndex = 0 );
		
		void applyForArray( const Raw* vertexArray, size_t offsetInBytes = 0 ) const
		{
			apply( reinterpret_cast< size_t >( vertexArray ) + offsetInBytes );
		}
		
		static size_t getNumBytesForComponentType( ComponentType type )
		{
			switch( type )
			{
				case Float:
					return sizeof( float );
				case UnsignedByte:
					return sizeof( unsigned char );
				default:
					ASSERT( false );
					return 0;
			}
		}
		
		virtual bool operator==( const VertexStructure& other ) const;
		bool operator!=( const VertexStructure& other ) const
		{
			return !operator==( other );
		}
		
		// Inherited from Asset.
		//
		virtual size_t getMemorySize() const override { return m_attributes.size() * sizeof( Attribute ); }
		
	protected:
		
		struct Attribute : public SerializableStruct< Attribute >
		{
			int nComponents;
			ComponentType type;
			AttributeUsage usage;
			int usageIndex;
			std::string name;
			
			explicit Attribute( int nComponents_ = 0, ComponentType type_ = Float, AttributeUsage usage_ = Ignore, int usageIndex_ = 0, const std::string name_ = "" )
			:	nComponents( nComponents_ )
			,	type( type_ )
			,	usage( usage_ )
			,	usageIndex( usageIndex_ )
			,	name( name_ )
			{
				STRUCT_BEGIN_PROPERTIES
				STRUCT_ADD_PROPERTY( nComponents )
				STRUCT_ADD_PROPERTY( type )
				STRUCT_ADD_PROPERTY( usage )
				STRUCT_ADD_PROPERTY( usageIndex )
				STRUCT_ADD_PROPERTY( name )
				STRUCT_END_PROPERTIES
			}
			
			size_t getNumBytes() const { return getNumBytesForComponentType( type ) * nComponents; }
			
			bool operator==( const Attribute& other ) const
			{
				return nComponents == other.nComponents &&
				type == other.type &&
				( usage == other.usage || usage == Other || other.usage == Other ) &&	// Other is generic, so might stand in for anything.
				usageIndex == other.usageIndex &&		
				name == other.name;
			}
			bool operator !=( const Attribute& other ) const	{ return !operator==( other ); }
		};
		STRUCT_DECLARE_SERIALIZATION_OPERATORS( Attribute )

		VAR( std::vector< Attribute >, m_attributes );
		
		const Attribute* getAttribute( const std::string& attributeName ) const;
		
		FRESH_DECLARE_CLASS( VertexStructure, Asset )
	};
	
	
	FRESH_ENUM_STREAM_IN_BEGIN( VertexStructure, ComponentType )
	FRESH_ENUM_STREAM_IN_CASE( VertexStructure, Float )
	FRESH_ENUM_STREAM_IN_CASE( VertexStructure, UnsignedByte )
	FRESH_ENUM_STREAM_IN_END()
	
	
	FRESH_ENUM_STREAM_IN_BEGIN( VertexStructure, AttributeUsage )
	FRESH_ENUM_STREAM_IN_CASE( VertexStructure, Position )
	FRESH_ENUM_STREAM_IN_CASE( VertexStructure, Normal )
	FRESH_ENUM_STREAM_IN_CASE( VertexStructure, Color )
	FRESH_ENUM_STREAM_IN_CASE( VertexStructure, TexCoord )
	FRESH_ENUM_STREAM_IN_CASE( VertexStructure, Other )
	FRESH_ENUM_STREAM_IN_CASE( VertexStructure, Ignore )
	FRESH_ENUM_STREAM_IN_END()
	
}

#endif
