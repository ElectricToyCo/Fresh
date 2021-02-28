/*
 *  ObjectPtr.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 10/30/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_OBJECT_PTR_H_INCLUDED
#define FRESH_OBJECT_PTR_H_INCLUDED

#include "SmartPtr.h"
#include <type_traits>

#define FRESH_DECLARE_CLASS_POINTERS( class_ )	\
	typedef fr::SmartPtr< class_ > ptr;	\
	typedef fr::SmartPtr< const class_ > cptr;	\
	typedef fr::WeakPtr< class_ > wptr;	\
	typedef fr::WeakPtr< const class_ > cwptr;

namespace fr
{
	template< typename PtrT >
	struct getRawType
	{
		typedef typename std::decay< PtrT >::type type;
	};
	
	template< typename T >
	struct getRawType< SmartPtr< T >>
	{
		typedef typename std::decay< T* >::type type;
	};
	
	template< typename T >
	struct getRawType< WeakPtr< T >>
	{
		typedef typename std::decay< T* >::type type;
	};
	
	/////////////////////////////////////////
	
	template< typename T >
	T dynamic_freshptr_cast( T ptr )
	{
		return ptr;
	}
	
	template< typename DestT, typename SrcT >
	DestT dynamic_freshptr_cast( SrcT ptr, typename std::enable_if< !std::is_same< SrcT, DestT >::value, int >::type = int() )	// Hide this conversion if the two types are the same.
	{
		return DestT( dynamic_cast< typename getRawType< DestT >::type >( ptr.get() ));
	}
	
	template< typename T >
	T static_freshptr_cast( T ptr )
	{
		return ptr;
	}
	
	template< typename DestT, typename SrcT >
	DestT static_freshptr_cast( SrcT ptr, typename std::enable_if< !std::is_same< SrcT, DestT >::value, int >::type = int() )
	{
		return DestT( static_cast< typename getRawType< DestT >::type >( ptr.get() ));
	}
}

#endif
