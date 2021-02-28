// Copyright 2006 Jeff Wofford all rights reserved.

#ifndef FRESH_SINGLETON_H_INCLUDED
#define FRESH_SINGLETON_H_INCLUDED

#include "FreshEssentials.h"
#include "FreshDebug.h"

namespace fr
{
    
    template< class T, typename PointerT = T* >
    class Singleton
    {
    public:
        static ALWAYS_INLINE T& instance();
            // REQUIRES( doesExist() );
        static ALWAYS_INLINE bool doesExist();

		template< typename... Args >
		static void create( Args... args );
			// REQUIRES( !doesExist() );
			// PROMISES( doesExist() );
		
        static void destroy();
            // REQUIRES( DoesExist() );
            // PROMISES( !DoesExist() );
        
    protected:
        ALWAYS_INLINE Singleton( T* instance );
            // REQUIRES( !s_instance );
            // REQUIRES( instance );
        ALWAYS_INLINE ~Singleton();

        static PointerT s_instance;
        
    private:
        
        Singleton( Singleton& );
        void operator=( Singleton& );
    };


    template< class T, typename PointerT >
    T& Singleton<T, PointerT>::instance()
    {
        REQUIRES( doesExist() );
        return *s_instance;
    }

    template< class T, typename PointerT >
    bool Singleton<T, PointerT>::doesExist()
    {
        return !!s_instance;
    }

    template< class T, typename PointerT >
	template< typename... Args >
    void Singleton<T, PointerT>::create( Args... args )
    {
        REQUIRES( !doesExist() );
		
        s_instance = new T( args... );
		
        PROMISES( doesExist() );
    }

    template< class T, typename PointerT >
    void Singleton<T, PointerT>::destroy()
    {
        REQUIRES( doesExist() );

        delete s_instance;
        s_instance = nullptr;

        PROMISES( !doesExist() );
    }
    template< class T, typename PointerT >
    Singleton<T, PointerT>::Singleton( T* instance )
    {
        REQUIRES( !s_instance );
//        REQUIRES( instance );	// No longer required. Inert objects pass nullptr for this.
        s_instance = instance;
    }

    template< class T, typename PointerT >
    Singleton<T, PointerT>::~Singleton()
    {
        s_instance = nullptr;
    }

	template< class T, typename PointerT >	PointerT fr::Singleton< T, PointerT >::s_instance = nullptr;
	
}		// end namespace fr

#endif
