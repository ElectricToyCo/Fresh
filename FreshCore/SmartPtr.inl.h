// SmartPtr.inl

namespace fr
{

	template< class T >
	ALWAYS_INLINE SmartPtr<T>::SmartPtr()
	:	m_var( nullptr )
	{}

	template< class T >
	ALWAYS_INLINE SmartPtr<T>::SmartPtr( std::nullptr_t )
	:	m_var( nullptr )
	{}

	template< class T >
	template< class Y, class >
	ALWAYS_INLINE SmartPtr<T>::SmartPtr( Y* p )
	:	m_var( p )
	{
		if( m_var )
		{
			m_var->addReference();
		}
	}

	template< class T >
	ALWAYS_INLINE SmartPtr<T>::SmartPtr( const SmartPtr& r )
	:	m_var( r.m_var )
	{
		if( m_var )
		{
			m_var->addReference();
		}
	}
	
	template< class T >
	ALWAYS_INLINE SmartPtr<T>::SmartPtr( SmartPtr< T >&& r )
	:	m_var( r.m_var )
	{
		r.m_var = nullptr;
	}

	template< class T >
	template< class Y >
	ALWAYS_INLINE SmartPtr<T>::SmartPtr( const SmartPtr< Y >& r, typename std::enable_if< std::is_convertible< Y*, T* >::value, NotAType >::type ) noexcept
	:	m_var( r.m_var )
	{
		if( m_var )
		{
			m_var->addReference();
		}
	}

	template< class T >
	template< class Y >
	ALWAYS_INLINE SmartPtr<T>::SmartPtr( SmartPtr< Y >&& r, typename std::enable_if< std::is_convertible< Y*, T* >::value, NotAType >::type ) noexcept
	:	m_var( r.m_var )
	{
		r.m_var = nullptr;
	}
	
	template< class T >
	template< class Y >
	ALWAYS_INLINE SmartPtr<T>::SmartPtr( const WeakPtr< Y >& r, typename std::enable_if< std::is_convertible< Y*, T* >::value, NotAType >::type ) noexcept
	:	m_var( r.cleanupProxy() )
	{
		if( m_var )
		{
			m_var->addReference();
		}
	}

	template< class T >
	ALWAYS_INLINE  SmartPtr<T>::~SmartPtr()
	{
		// Release the referent through operator=() in order to inherit the wisdom embedded in that code.
		operator=( nullptr );
	}

	template< class T >
	ALWAYS_INLINE  bool SmartPtr<T>::isNull() const
	{
		return !m_var;
	}

	template< class T >
	ALWAYS_INLINE  T* SmartPtr<T>::operator->() const
	{
		ASSERT( m_var );
		return m_var;
	}

	template< class T >
	ALWAYS_INLINE  T& SmartPtr<T>::operator*() const
	{
		ASSERT( m_var );
		return *m_var;
	}

	template< class T >
	ALWAYS_INLINE  SmartPtr<T>::operator ReferentPtr() const
	{
		return m_var;
	}

	template< class T >
	ALWAYS_INLINE  typename SmartPtr<T>::ReferentPtr SmartPtr<T>::get() const
	{
		return m_var;
	}

	template< class T >
	SmartPtr<T>& SmartPtr<T>::operator=( std::nullptr_t )
	{
		if( m_var )
		{
			// See note below in operator=( Y* ) for an explanation for why this isn't more straightforward.
			T* momentarilyRetained = m_var;
			m_var = nullptr;
			if( momentarilyRetained )
			{
				momentarilyRetained->release();
			}
		}
		return *this;
	}
	
	template< class T >
	template< class Y >
	ALWAYS_INLINE typename std::enable_if< std::is_convertible< Y*, T* >::value, SmartPtr<T>& >::type SmartPtr<T>::operator=( Y* p ) noexcept
	{
		if( m_var == p )
		{
			return *this;
		}
		
		// Since the object pointed to be m_var might be deleted
		// within the release() call, and yet may also make reference to this
		// smart pointer (as, for example, with an object pointing to itself),
		// We need to make sure that the smart pointer's address is assigned first,
		// before the deletion actually happens, so that if this function is called
		// re-entrantly, m_var will already have "moved on" and we won't call
		// release() in an infinite loop.
		//
		T* momentarilyRetained = m_var;
		m_var = p;
		if( momentarilyRetained )
		{
			momentarilyRetained->release();
		}
		
		if( m_var )
		{
			m_var->addReference();
		}
		return *this;
	}
	
	template< class T >
	ALWAYS_INLINE  SmartPtr< T >& SmartPtr<T>::operator=( const SmartPtr< T >& var )
	{
		// Use the raw pointer assignment operator.
		return operator=( var.m_var );
	}

	template< class T >
	ALWAYS_INLINE SmartPtr< T >& SmartPtr<T>::operator=( SmartPtr< T >&& var )
	{
		// Use the raw pointer assignment operator.
		operator=( var.m_var );
		var = nullptr;
		return *this;
	}

	template< class T >
	template< class Y >
	ALWAYS_INLINE typename std::enable_if< std::is_convertible< Y*, T* >::value, SmartPtr<T>& >::type SmartPtr<T>::operator=( const SmartPtr< Y >& r ) noexcept
	{
		// Use the raw pointer assignment operator.
		return operator=( r.m_var );
	}

	template< class T >
	template< class Y >
	ALWAYS_INLINE typename std::enable_if< std::is_convertible< Y*, T* >::value, SmartPtr<T>& >::type SmartPtr<T>::operator=( SmartPtr< Y >&& r ) noexcept
	{
		m_var = r.m_var;
		r.m_var = nullptr;
		return *this;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	ALWAYS_INLINE void WeakPtrProxy::release()
	{
		--m_nReferences;
		ASSERT( m_nReferences >= 0 );
		if( m_nReferences == 0 ) delete this;
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template< class T >
	ALWAYS_INLINE WeakPtr<T>::WeakPtr()
	:	m_proxy( nullptr )
	,	m_var( nullptr )
	{}
	
	template< class T >
	ALWAYS_INLINE WeakPtr<T>::WeakPtr( std::nullptr_t p )
	:	m_proxy( nullptr )
	,	m_var( nullptr )
	{}
	
	template< class T >
	template< class Y, class >
	ALWAYS_INLINE WeakPtr<T>::WeakPtr( Y* p )
	:	m_proxy( p ? p->getWeakPtrProxy() : nullptr )
	,	m_var( p )
	{}
	
	template< class T >
	ALWAYS_INLINE WeakPtr<T>::WeakPtr( const WeakPtr& r )
	:	m_proxy( r.m_proxy )
	,	m_var( r.m_var )
	{}
	
	template< class T >
	template< class Y >
	ALWAYS_INLINE WeakPtr<T>::WeakPtr( const WeakPtr< Y >& r, typename std::enable_if< std::is_convertible< Y*, T* >::value, NotAType >::type ) noexcept
	:	m_proxy( r.m_proxy )
	,	m_var( r.m_var )
	{}
	
	template< class T >
	template< class Y >
	ALWAYS_INLINE WeakPtr<T>::WeakPtr( const SmartPtr< Y >& r, typename std::enable_if< std::is_convertible< Y*, T* >::value, NotAType >::type ) noexcept
	:	m_proxy( r.isNull() ? nullptr : r->getWeakPtrProxy() )
	,	m_var( r.get() )
	{}
	
	template< class T >
	ALWAYS_INLINE bool WeakPtr<T>::isNull() const
	{
		return !cleanupProxy();
	}

	template< class T >
	ALWAYS_INLINE WeakPtr<T>::operator bool() const
	{
		return !isNull();
	}
	
	template< class T >
	ALWAYS_INLINE typename WeakPtr<T>::ReferentPtr WeakPtr<T>::operator->() const
	{
		const auto var = cleanupProxy();
		ASSERT( var );
		return var;
	}
	
	template< class T >
	SmartPtr< T > WeakPtr< T >::lock() const
	{
		return SmartPtr< T >{ cleanupProxy() };
	}
	
	template< class T >
	ALWAYS_INLINE T& WeakPtr<T>::operator*() const
	{
		const auto var = cleanupProxy();
		ASSERT( var );
		return *var;
	}
	
	template< class T >
	ALWAYS_INLINE WeakPtr<T>::operator ReferentPtr() const
	{
		return cleanupProxy();
	}
	
	template< class T >
	ALWAYS_INLINE typename WeakPtr<T>::ReferentPtr WeakPtr<T>::get() const
	{
		return cleanupProxy();
	}
	
	template< class T >
	ALWAYS_INLINE WeakPtr<T>& WeakPtr<T>::operator=( ReferentPtr var )
	{
		m_proxy = var ? var->getWeakPtrProxy() : nullptr;
		m_var = var;
		return *this;
	}
	
	template< class T >
	ALWAYS_INLINE WeakPtr<T>& WeakPtr<T>::operator=( const WeakPtr& ptr )
	{
		m_proxy = ptr.isNull() ? nullptr : ptr.m_proxy;
		m_var = ptr.m_var;
		return *this;
	}
	
	template< class T >
	ALWAYS_INLINE WeakPtr<T>& WeakPtr<T>::operator=( const SmartPtr< T >& ptr )
	{
		m_proxy = ptr.isNull() ? nullptr : ptr->getWeakPtrProxy();
		m_var = ptr.get();
		return *this;
	}
	
	template< class T >
	ALWAYS_INLINE typename WeakPtr<T>::ReferentPtr WeakPtr<T>::cleanupProxy() const
	{
		if( m_proxy )
		{
			if( m_proxy->isNull() )
			{
				m_proxy = nullptr;
				m_var = nullptr;
			}
		}
		return m_var;
	}
	
	template< class T >
	ALWAYS_INLINE bool WeakPtr<T>::isValid() const
	{
		return !m_var || ( m_var && m_proxy );
	}

}
