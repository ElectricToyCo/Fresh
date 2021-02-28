#ifndef SMART_PTR_H_INCLUDED_
#define SMART_PTR_H_INCLUDED_

#include <cstddef>		// For std::nullptr_t
#include <type_traits>
#include "CoreCppCompatability.h"
#include "FreshDebug.h"
#include "FreshEssentials.h"	// For ALWAYS_INLINE

namespace fr
{

	template< class T > class WeakPtr;
	
	// SmartPtr
	//
	// Class SmartPtr is used to reference objects of type T.
	// T must be a reference-counted, self-deleting class having public member functions
	// addReference() and release().
	// An object of type T should always be accessed via SmartPtr< T >.
	// rather than T*. If you're not familiar with the concept of smart pointers, look them up.
	// This is a totally standard implementation of the smart pointer concept.
	//
	template < class T >
	class SmartPtr
	{
		struct NotAType { int forBool; };
		
	public:
		
		typedef T element_type;
		typedef T* ReferentPtr;

		SmartPtr();
		SmartPtr( std::nullptr_t );
		
		template< class Y, class = typename std::enable_if< std::is_convertible< Y*, element_type* >::value >::type >
        SmartPtr( Y* p );
		
		SmartPtr( const SmartPtr& r );
		SmartPtr( SmartPtr&& r );
		
		template< class Y >
		SmartPtr( const SmartPtr< Y >& r, typename std::enable_if< std::is_convertible< Y*, T* >::value, NotAType >::type = NotAType() ) noexcept;
		template< class Y >
		SmartPtr( SmartPtr< Y >&& r, typename std::enable_if< std::is_convertible< Y*, T* >::value, NotAType >::type = NotAType() ) noexcept;
		
		template< class Y >
		SmartPtr( const WeakPtr< Y >& r, typename std::enable_if< std::is_convertible< Y*, T* >::value, NotAType >::type = NotAType() ) noexcept;
		
		~SmartPtr();
		
		// ACCESS
		//
		explicit operator bool() const { return m_var != nullptr; }
		
		bool isNull() const;

		ReferentPtr operator->() const;
		T& operator*() const;

		explicit operator ReferentPtr() const;
		
		ReferentPtr get() const;
		
		// ASSIGNMENT
		//
		SmartPtr& operator=( std::nullptr_t );
		
		template< class Y >
		typename std::enable_if< std::is_convertible< Y*, element_type* >::value, SmartPtr& >::type operator=( Y* p ) noexcept;
		
		SmartPtr& operator=( const SmartPtr& var );
		SmartPtr& operator=( SmartPtr&& var );

		template< class Y >
		typename std::enable_if< std::is_convertible< Y*, element_type* >::value, SmartPtr& >::type operator=( const SmartPtr< Y >& r) noexcept;
		template< class Y >
		typename std::enable_if< std::is_convertible< Y*, element_type* >::value, SmartPtr& >::type operator=( SmartPtr< Y >&& r) noexcept;

	private:
		
		ReferentPtr m_var;
		
		template<class Y> friend class SmartPtr;
		template<class Y> friend class WeakPtr;
		
	};
	
	////////////////////////////////////////////////////
	
	class WeakPtrProxy
	{
	public:
		
		bool isNull() const			{ return !m_hasReferent; }
		
		void onReferentDeleted()	{ m_hasReferent = false; }
		
		void addReference()			{ ++m_nReferences; }
		void release();
		
	private:
		
		int m_nReferences = 0;
		bool m_hasReferent = true;
	};
	
	////////////////////////////////////////////////////
	
	template< typename T >
	class WeakPtr
	{
		struct NotAType { int forBool; };

	public:
		
		typedef T element_type;
		typedef T* ReferentPtr;
		
		WeakPtr();
		WeakPtr( std::nullptr_t );
		
		template< class Y, class = typename std::enable_if< std::is_convertible< Y*, element_type* >::value >::type >
        WeakPtr( Y* p );
		
		WeakPtr( const WeakPtr& r );
		
		template< class Y >
		WeakPtr( const WeakPtr< Y >& r, typename std::enable_if< std::is_convertible< Y*, T* >::value, NotAType >::type = NotAType() ) noexcept;
		
		template< class Y >
		WeakPtr( const SmartPtr< Y >& r, typename std::enable_if< std::is_convertible< Y*, T* >::value, NotAType >::type = NotAType() ) noexcept;
		
		bool isNull() const;
		explicit operator bool() const;
		
		ReferentPtr operator->() const;
		T& operator*() const;
		
		SmartPtr< T > lock() const;
		
		explicit operator ReferentPtr() const;
		
		ReferentPtr get() const;

		WeakPtr& operator=( ReferentPtr var );
		
		WeakPtr& operator=( const WeakPtr& ptr );
		
		WeakPtr& operator=( const SmartPtr< T >& ptr );
				
	protected:
		
		ReferentPtr cleanupProxy() const;
		
	private:
		
		
		// This order is important. Proxies get cleaned up first, so the WeakPtr constructors
		// rely on cleaning up and assigning proxies, then assigning the raw pointer afterward.
		//
		mutable SmartPtr< WeakPtrProxy > m_proxy;
		mutable ReferentPtr m_var;
		
		bool isValid() const;					// For internal testing and debugging.
	
		template<class Y> friend class SmartPtr;
		template<class Y> friend class WeakPtr;
	};

	///////////////////////////////////////////////////////////////////////////
	// RawPtr<> is a simple wrapper for raw pointers that acts like a raw pointer
	// in every way except that it automatically initializes to null.
	//
	template< typename T >
	class RawPtr
	{
		struct NotAType { int forBool; };
		
	public:
		
		typedef T element_type;
		typedef T* ReferentPtr;

		RawPtr( ReferentPtr p = nullptr ) : m_ptr( p ) {}
		operator ReferentPtr() const { return m_ptr; }
		ReferentPtr operator->() const { ASSERT( m_ptr ); return m_ptr; }
		T& operator*() const { ASSERT( m_ptr ); return *m_ptr; }
		T& operator[]( ptrdiff_t i ) const { ASSERT( m_ptr ); return m_ptr[ i ]; }
		
		ReferentPtr get() const { return m_ptr; }
		
		// Conversion from e.g. non-const to const.
		//
		template< class Y, class = typename std::enable_if< std::is_convertible< Y*, element_type* >::value >::type >
        RawPtr( Y* p ) { m_ptr = p; }
		
		template< class Y >
		RawPtr( const RawPtr< Y >& r, typename std::enable_if< std::is_convertible< Y*, T* >::value, NotAType >::type = NotAType() ) noexcept { m_ptr = r.get(); }
		
		template< class Y >
		typename std::enable_if< std::is_convertible< Y*, element_type* >::value, RawPtr& >::type operator=( Y* p ) noexcept { m_ptr = p; return *this; }

		template< class Y >
		typename std::enable_if< std::is_convertible< Y*, element_type* >::value, RawPtr& >::type operator=( const RawPtr< Y >& r ) noexcept { m_ptr = r.get(); return *this; }

	private:
		
		ReferentPtr m_ptr;
	};
	

	///////////////////////////////////////////////////////////////////////////
	// SMARTPTR COMPARISON OPERATORS
	//
	
	// To smart pointers. /////////////////////////////////
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator==( const SmartPtr<ObjectT_A>& __x, const SmartPtr<ObjectT_B>& __y) noexcept
	{
		return __x.get() == __y.get();
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator!=( const SmartPtr<ObjectT_A>& __x, const SmartPtr<ObjectT_B>& __y) noexcept
	{
		return !(__x == __y);
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator<( const SmartPtr<ObjectT_A>& __x, const SmartPtr<ObjectT_B>& __y) noexcept
	{
		typedef typename std::common_type<ObjectT_A*, ObjectT_B*>::type CommonType;
		return static_cast< CommonType >( __x.get() ) < static_cast< CommonType >( __y.get() );
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator>( const SmartPtr<ObjectT_A>& __x, const SmartPtr<ObjectT_B>& __y) noexcept
	{
		return __y < __x;
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator<=( const SmartPtr<ObjectT_A>& __x, const SmartPtr<ObjectT_B>& __y) noexcept
	{
		return !(__y < __x);
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator>=( const SmartPtr<ObjectT_A>& __x, const SmartPtr<ObjectT_B>& __y) noexcept
	{
		return !(__x < __y);
	}
	
	// To raw pointers. /////////////////////////////////
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator==( const SmartPtr<ObjectT_A>& __x, ObjectT_B* const __y ) noexcept
	{
		return __x.get() == __y;
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator==( ObjectT_B* const __y, const SmartPtr<ObjectT_A>& __x) noexcept
	{
		return __x.get() == __y;
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator!=( const SmartPtr<ObjectT_A>& __x, ObjectT_B* const __y) noexcept
	{
		return __x.get() != __y;
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator!=( ObjectT_B* const __y, const SmartPtr<ObjectT_A>& __x) noexcept
	{
		return __x.get() != __y;
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator<( const SmartPtr<ObjectT_A>& __x, ObjectT_B* const __y) noexcept
	{
		return __x.get() < __y;
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator<( ObjectT_B* const __y, const SmartPtr<ObjectT_A>& __x) noexcept
	{
		return __y < __x.get();
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator>( const SmartPtr<ObjectT_A>& __x, ObjectT_B* const __y) noexcept
	{
		return __y < __x;
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator>( ObjectT_B* const __y, const SmartPtr<ObjectT_A>& __x) noexcept
	{
		return __x < __y;
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator<=( const SmartPtr<ObjectT_A>& __x, ObjectT_B* const __y) noexcept
	{
		return !(__y < __x);
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator<=( ObjectT_B* const __y, const SmartPtr<ObjectT_A>& __x) noexcept
	{
		return !(__x < __y);
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator>=( const SmartPtr<ObjectT_A>& __x, ObjectT_B* const __y) noexcept
	{
		return !(__x < __y);
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator>=( ObjectT_B* const __y, const SmartPtr<ObjectT_A>& __x) noexcept
	{
		return !(__y < __x);
	}
	
	// To nullptr. /////////////////////////////////
	
	template<class ObjectT_A>
	inline bool operator==( const SmartPtr<ObjectT_A>& __x, std::nullptr_t) noexcept
	{
		return !__x;
	}
	
	template<class ObjectT_A>
	inline bool operator==( std::nullptr_t, const SmartPtr<ObjectT_A>& __x) noexcept
	{
		return !__x;
	}
	
	template<class ObjectT_A>
	inline bool operator!=( const SmartPtr<ObjectT_A>& __x, std::nullptr_t) noexcept
	{
		return static_cast<bool>(__x);
	}
	
	template<class ObjectT_A>
	inline bool operator!=( std::nullptr_t, const SmartPtr<ObjectT_A>& __x) noexcept
	{
		return static_cast<bool>(__x);
	}
	
	template<class ObjectT_A>
	inline bool operator<( const SmartPtr<ObjectT_A>& __x, std::nullptr_t) noexcept
	{
		return __x.get() < nullptr;
	}
	
	template<class ObjectT_A>
	inline bool operator<( std::nullptr_t, const SmartPtr<ObjectT_A>& __x) noexcept
	{
		return nullptr < __x.get();
	}
	
	template<class ObjectT_A>
	inline bool operator>( const SmartPtr<ObjectT_A>& __x, std::nullptr_t) noexcept
	{
		return nullptr < __x;
	}
	
	template<class ObjectT_A>
	inline bool operator>( std::nullptr_t, const SmartPtr<ObjectT_A>& __x) noexcept
	{
		return __x < nullptr;
	}
	
	template<class ObjectT_A>
	inline bool operator<=( const SmartPtr<ObjectT_A>& __x, std::nullptr_t) noexcept
	{
		return !(nullptr < __x);
	}
	
	template<class ObjectT_A>
	inline bool operator<=( std::nullptr_t, const SmartPtr<ObjectT_A>& __x) noexcept
	{
		return !(__x < nullptr);
	}
	
	template<class ObjectT_A>
	inline bool operator>=( const SmartPtr<ObjectT_A>& __x, std::nullptr_t) noexcept
	{
		return !(__x < nullptr);
	}
	
	template<class ObjectT_A>
	inline bool operator>=( std::nullptr_t, const SmartPtr<ObjectT_A>& __x) noexcept
	{
		return !(nullptr < __x);
	}

	///////////////////////////////////////////////////////////////////////////
	// SMARTPTR-to-WEAKPTR COMPARISON OPERATORS
	//
	
	template< typename ObjectT_A, typename ObjectT_B >
	inline bool operator==( SmartPtr< ObjectT_A > sp, WeakPtr< ObjectT_B > wp )
	{
		return sp.get() == wp.get();
	}
	
	template< typename ObjectT_A, typename ObjectT_B >
	inline bool operator!=( SmartPtr< ObjectT_A > sp, WeakPtr< ObjectT_B > wp )
	{
		return sp.get() != wp.get();
	}
	
	template< typename ObjectT_A, typename ObjectT_B >
	inline bool operator==( WeakPtr< ObjectT_A > wp, SmartPtr< ObjectT_B > sp )
	{
		return sp.get() == wp.get();
	}
	
	template< typename ObjectT_A, typename ObjectT_B >
	inline bool operator!=( WeakPtr< ObjectT_A > wp, SmartPtr< ObjectT_B > sp )
	{
		return sp.get() != wp.get();
	}
	
	///////////////////////////////////////////////////////////////////////////
	// WEAKPTR COMPARISON OPERATORS
	//
	
	// To weak pointers. /////////////////////////////////
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator==( const WeakPtr<ObjectT_A>& __x, const WeakPtr<ObjectT_B>& __y) noexcept
	{
		return __x.get() == __y.get();
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator!=( const WeakPtr<ObjectT_A>& __x, const WeakPtr<ObjectT_B>& __y) noexcept
	{
		return !(__x == __y);
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator<( const WeakPtr<ObjectT_A>& __x, const WeakPtr<ObjectT_B>& __y) noexcept
	{
		typedef typename std::common_type<ObjectT_A*, ObjectT_B*>::type CommonType;
		return static_cast< CommonType >( __x.get() ) < static_cast< CommonType >( __y.get() );
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator>( const WeakPtr<ObjectT_A>& __x, const WeakPtr<ObjectT_B>& __y) noexcept
	{
		return __y < __x;
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator<=( const WeakPtr<ObjectT_A>& __x, const WeakPtr<ObjectT_B>& __y) noexcept
	{
		return !(__y < __x);
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator>=( const WeakPtr<ObjectT_A>& __x, const WeakPtr<ObjectT_B>& __y) noexcept
	{
		return !(__x < __y);
	}
	
	// To raw pointers. /////////////////////////////////
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator==( const WeakPtr<ObjectT_A>& __x, ObjectT_B* const __y ) noexcept
	{
		return __x.get() == __y;
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator==( ObjectT_B* const __y, const WeakPtr<ObjectT_A>& __x) noexcept
	{
		return __x.get() == __y;
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator!=( const WeakPtr<ObjectT_A>& __x, ObjectT_B* const __y) noexcept
	{
		return __x.get() != __y;
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator!=( ObjectT_B* const __y, const WeakPtr<ObjectT_A>& __x) noexcept
	{
		return __x.get() != __y;
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator<( const WeakPtr<ObjectT_A>& __x, ObjectT_B* const __y) noexcept
	{
		return __x.get() < __y;
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator<( ObjectT_B* const __y, const WeakPtr<ObjectT_A>& __x) noexcept
	{
		return __y < __x.get();
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator>( const WeakPtr<ObjectT_A>& __x, ObjectT_B* const __y) noexcept
	{
		return __y < __x;
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator>( ObjectT_B* const __y, const WeakPtr<ObjectT_A>& __x) noexcept
	{
		return __x < __y;
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator<=( const WeakPtr<ObjectT_A>& __x, ObjectT_B* const __y) noexcept
	{
		return !(__y < __x);
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator<=( ObjectT_B* const __y, const WeakPtr<ObjectT_A>& __x) noexcept
	{
		return !(__x < __y);
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator>=( const WeakPtr<ObjectT_A>& __x, ObjectT_B* const __y) noexcept
	{
		return !(__x < __y);
	}
	
	template<class ObjectT_A, class ObjectT_B>
	inline bool operator>=( ObjectT_B* const __y, const WeakPtr<ObjectT_A>& __x) noexcept
	{
		return !(__y < __x);
	}
	
	// To nullptr. /////////////////////////////////
	
	template<class ObjectT_A>
	inline bool operator==( const WeakPtr<ObjectT_A>& __x, std::nullptr_t) noexcept
	{
		return !__x;
	}
	
	template<class ObjectT_A>
	inline bool operator==( std::nullptr_t, const WeakPtr<ObjectT_A>& __x) noexcept
	{
		return !__x;
	}
	
	template<class ObjectT_A>
	inline bool operator!=( const WeakPtr<ObjectT_A>& __x, std::nullptr_t) noexcept
	{
		return static_cast<bool>(__x);
	}
	
	template<class ObjectT_A>
	inline bool operator!=( std::nullptr_t, const WeakPtr<ObjectT_A>& __x) noexcept
	{
		return static_cast<bool>(__x);
	}
	
	template<class ObjectT_A>
	inline bool operator<( const WeakPtr<ObjectT_A>& __x, std::nullptr_t) noexcept
	{
		return __x.get() < nullptr;
	}
	
	template<class ObjectT_A>
	inline bool operator<( std::nullptr_t, const WeakPtr<ObjectT_A>& __x) noexcept
	{
		return nullptr < __x.get();
	}
	
	template<class ObjectT_A>
	inline bool operator>( const WeakPtr<ObjectT_A>& __x, std::nullptr_t) noexcept
	{
		return nullptr < __x;
	}
	
	template<class ObjectT_A>
	inline bool operator>( std::nullptr_t, const WeakPtr<ObjectT_A>& __x) noexcept
	{
		return __x < nullptr;
	}
	
	template<class ObjectT_A>
	inline bool operator<=( const WeakPtr<ObjectT_A>& __x, std::nullptr_t) noexcept
	{
		return !(nullptr < __x);
	}
	
	template<class ObjectT_A>
	inline bool operator<=( std::nullptr_t, const WeakPtr<ObjectT_A>& __x) noexcept
	{
		return !(__x < nullptr);
	}
	
	template<class ObjectT_A>
	inline bool operator>=( const WeakPtr<ObjectT_A>& __x, std::nullptr_t) noexcept
	{
		return !(__x < nullptr);
	}
	
	template<class ObjectT_A>
	inline bool operator>=( std::nullptr_t, const WeakPtr<ObjectT_A>& __x) noexcept
	{
		return !(nullptr < __x);
	}

}	// END namespace fr

#include "SmartPtr.inl.h"

#endif
