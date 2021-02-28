//
//  VertexBuffer.h
//  Fresh
//
//  Created by Jeff Wofford on 11/26/11.
//  Copyright (c) 2011 jeffwofford.com. All rights reserved.
//

#ifndef Fresh_VertexBuffer_h
#define Fresh_VertexBuffer_h

#include "Asset.h"
#include "VertexStructure.h"

namespace fr
{
	
	class VertexBuffer : public Asset
	{
	public:
		
		virtual ~VertexBuffer();
		
		void associateWithVertexStructure( VertexStructure::ptr vertexStructure );
		
		// If no vertex structure has yet been associated, associates one.
		// Either way, this function then calls the corresponding function in VertexStructure.
		//
		void addAttribute( const std::string& attributeName, 
						  int nComponents, 
						  VertexStructure::ComponentType type, 
						  VertexStructure::AttributeUsage usage, 
						  int usageIndex = 0 );
		
		void clear();
		
		template< typename VertexIter >
		void loadVertices( const VertexIter begin, const VertexIter end );
		
		void loadRaw( const unsigned char* begin, size_t nBytes );

		virtual void applyForRendering();
			// REQUIRES( isLoaded() );
		
		// Inherited from Asset
		//
		virtual size_t getMemorySize() const override;
		virtual bool isLoaded() const override;
		
	private:
		
		unsigned int m_idVertexArrayObject = 0;
		unsigned int m_idVertexBufferObject = 0;
		
		VAR( VertexStructure::ptr, m_vertexStructure );
		
		size_t m_nBytesLoaded = 0;
		
		FRESH_DECLARE_CLASS( VertexBuffer, Asset )
	};
	
	//////////////////////////////////////////////////////////////////////////////////
	
	template< typename VertexIter >
	void VertexBuffer::loadVertices( const VertexIter begin, const VertexIter end )
	{
		const size_t nBytes = ( end - begin ) * sizeof( *begin );
		
		loadRaw( reinterpret_cast< const unsigned char* >( &*begin ), nBytes );
	}
	
}

#endif
