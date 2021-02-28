//
//  SwitchPtr.inl.h
//  Fresh
//
//  Created by Jeff Wofford on 3/2/13.
//
//

#ifndef Fresh_SwitchPtr_inl_h
#define Fresh_SwitchPtr_inl_h


namespace fr
{
	
	template< typename T >
	ALWAYS_INLINE SwitchPtr<T>::SwitchPtr()
	{}
	
	template< typename T >
	ALWAYS_INLINE SwitchPtr<T>::SwitchPtr( const SmartPtr< T >& p )
	:	m_strong( p )
	,	m_isRetained( true )
	{}
	
	template< typename T >
	ALWAYS_INLINE SwitchPtr<T>::SwitchPtr( const SmartPtr< T >& p, bool retain )
	:	m_strong( retain ? p : nullptr )
	,	m_weak( retain ? nullptr : p )
	,	m_isRetained( retain )
	{}
	
	template< typename T >
	ALWAYS_INLINE bool SwitchPtr<T>::isRetained() const
	{
		return m_isRetained;
	}
	
	template< typename T >
	ALWAYS_INLINE void SwitchPtr<T>::retain()
	{
		if( !m_isRetained )
		{
			m_strong = m_weak;
			m_weak = nullptr;
			m_isRetained = true;
		}
	}
	
	template< typename T >
	ALWAYS_INLINE void SwitchPtr<T>::release()
	{
		if( m_isRetained )
		{
			m_weak = m_strong;
			m_strong = nullptr;
			m_isRetained = false;
		}
	}
	
	template< typename T >
	ALWAYS_INLINE T* SwitchPtr<T>::get() const
	{
		return m_isRetained ? m_strong.get() : m_weak.get();
	}
	
	template< typename T >
	ALWAYS_INLINE SwitchPtr<T>::operator bool() const
	{
		return m_isRetained ? bool( m_strong ) : bool( m_weak );
	}
	
	template< typename T >
	ALWAYS_INLINE SwitchPtr<T>::operator ptr() const
	{
		return m_isRetained ? ptr( m_strong ) : ptr( m_weak );
	}
	
	template< typename T >
	ALWAYS_INLINE typename SwitchPtr<T>::ReferentPtr SwitchPtr<T>::operator->() const
	{
		return m_isRetained ? m_strong.operator->() : m_weak.operator->();
	}
	
	template< typename T >
	ALWAYS_INLINE T& SwitchPtr<T>::operator*() const
	{
		return m_isRetained ? m_strong.operator*() : m_weak.operator*();
	}	

	template< typename T >
	ALWAYS_INLINE bool SwitchPtr<T>::operator==( const SwitchPtr& rVar ) const
	{
		return m_isRetained ? m_strong == rVar.get() : m_weak == rVar.get();
	}
	
	template< typename T >
	ALWAYS_INLINE bool SwitchPtr<T>::operator!=( const SwitchPtr& rVar ) const
	{
		return !operator==( rVar );
	}
	
	template< typename T >
	ALWAYS_INLINE bool SwitchPtr<T>::operator<( const SwitchPtr& rVar ) const
	{
		return m_isRetained ? m_strong < rVar.get() : m_weak < rVar.get();
	}
	
}

#endif
