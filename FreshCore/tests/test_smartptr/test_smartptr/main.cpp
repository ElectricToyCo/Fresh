//
//  main.cpp
//  test_smartptr
//
//  Created by Jeff Wofford on 2/22/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#define SMARTPTR_SUPPORT_DOWNCAST
#include "../../../SmartPtr.h"
#include "../../../WeakPtr.h"
#include <list>
#include <iostream>
#include <memory>
using namespace Fresh;

struct SelfDeleting
{
	SelfDeleting() { s_selfDeletingObjects.push_back( this ); }
	~SelfDeleting() { s_selfDeletingObjects.remove( this ); }
	
	void addReference() { ++m_nReferences; }
	void release()
	{
		ASSERT( m_nReferences > 0 );
		--m_nReferences;
		
		if( m_nReferences == 0 )
		{
			delete this;
		}
	}
	
	size_t nReferences() const { return m_nReferences; }

	static size_t nObjects() { return s_selfDeletingObjects.size(); }
	
	
	WeakPtrProxy* getWeakPtrProxy() const { return m_weakPtrProxy; }
	
private:
	
	size_t m_nReferences = 0;
	SmartPtr< WeakPtrProxy > m_weakPtrProxy = new WeakPtrProxy;

	static std::list< SelfDeleting* > s_selfDeletingObjects;
};

struct DerivedA : public SelfDeleting {};
struct DerivedB : public SelfDeleting {};

std::list< SelfDeleting* > SelfDeleting::s_selfDeletingObjects;

void fooDowncast( SmartPtr< SelfDeleting > p )
{
	p = nullptr;	// Should have no outside effect.
}

void fooUpcast( SmartPtr< DerivedA > p )
{
	p = nullptr;	// Should have no outside effect.
}

void fooCrosscast( SmartPtr< DerivedB > p )
{
	p = nullptr;	// Should have no outside effect.
}

void fooWeak( WeakPtr< SelfDeleting > p )
{
	p = nullptr;
}


///////////////////////////////////////////////////////////////////////

void runControlledTests()
{
	assert( SelfDeleting::nObjects() == 0 );

	{
		SmartPtr< SelfDeleting > p;
		assert( !p );
		assert( SelfDeleting::nObjects() == 0 );
	}
	
	{
		SmartPtr< SelfDeleting > p = nullptr;
		assert( !p );
		assert( SelfDeleting::nObjects() == 0 );
	}
	
	{
		SmartPtr< SelfDeleting > p = new SelfDeleting();
		assert( p );
		assert( p->nReferences() == 1 );
		assert( SelfDeleting::nObjects() == 1 );
	}
	assert( SelfDeleting::nObjects() == 0 );
	
	{
		SmartPtr< SelfDeleting > p = new SelfDeleting();
		SmartPtr< SelfDeleting > q = p;
		assert( p );
		assert( q == p );
		assert( p->nReferences() == 2 );
		assert( SelfDeleting::nObjects() == 1 );
	}

	{
		SmartPtr< SelfDeleting > p = new SelfDeleting();
		SmartPtr< SelfDeleting > q = p;
		p = q;
		assert( p );
		assert( q == p );
		assert( p->nReferences() == 2 );
		assert( SelfDeleting::nObjects() == 1 );
	}

	{
		SmartPtr< SelfDeleting > p = new SelfDeleting();
		SmartPtr< SelfDeleting > q = p;
		p = nullptr;
		assert( 1 );
		assert( q->nReferences() == 1 );
		assert( SelfDeleting::nObjects() == 1 );
	}
	
	// Casting.
	//
	{
		SmartPtr< SelfDeleting > p = new DerivedA();
		assert( p );
		assert( p->nReferences() == 1 );
		assert( SelfDeleting::nObjects() == 1 );
	}

#if 0		// CORRECT COMPILER ERROR
	{
		SmartPtr< DerivedA > p = new SelfDeleting();
	}
#endif
	
	{
		SmartPtr< DerivedA > p = new DerivedA();
		fooDowncast( p );	// Should automatically downcast
		assert( p );
		assert( p->nReferences() == 1 );
		assert( SelfDeleting::nObjects() == 1 );
	}

#if 0		// CORRECT COMPILER ERROR
	{
		SmartPtr< SelfDeleting > p = new SelfDeleting();
		fooUpcast( p );
	}
#endif
	
#if 0		// CORRECT COMPILER ERROR
	{
		SmartPtr< DerivedA > p = new DerivedA();
		fooCrosscast( p );
	}
#endif
	
	{
		SmartPtr< SelfDeleting > p = new SelfDeleting();
		WeakPtr< SelfDeleting > w = p;
		
		fooDowncast( w );	// Should automatically convert weak to smart.
	}

	{
		SmartPtr< SelfDeleting > p = new SelfDeleting();
		
		fooWeak( p );	// Should automatically convert smart to weak.
	}
	

}


///////////////////////////////////////////////////////////////////////


int main( int argc, const char * argv[] )
{
	runControlledTests();
	
    return 0;
}

