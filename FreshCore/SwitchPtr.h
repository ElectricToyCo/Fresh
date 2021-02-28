//
//  SwitchPtr.h
//  Fresh
//
//  Created by Jeff Wofford on 3/2/13.
//
//

#ifndef Fresh_SwitchPtr_h
#define Fresh_SwitchPtr_h

#include "SmartPtr.h"

namespace fr
{
	
	template< typename T >
	class SwitchPtr
	{
	public:
		
		typedef SmartPtr< T > ptr;
		typedef typename ptr::ReferentPtr ReferentPtr;
		
		SwitchPtr();
		SwitchPtr( const SmartPtr< T >& p );	// Implicit
		SwitchPtr( const SmartPtr< T >& p, bool retain );
		
		bool isRetained() const;
		void retain();
		void release();
		
		T* get() const;
		
		explicit operator bool() const;
		
		operator ptr() const;

		ReferentPtr operator->() const;
		T& operator*() const;

		bool operator==( const SwitchPtr& rVar ) const;
		bool operator!=( const SwitchPtr& rVar ) const;
		bool operator<( const SwitchPtr& rVar ) const;

	private:
		
		typedef WeakPtr< T > wptr;
		
		ptr m_strong;
		wptr m_weak;
		
		bool m_isRetained = false;
		
	};
	
}

#include "SwitchPtr.inl.h"


#endif
