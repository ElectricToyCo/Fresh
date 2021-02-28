/*
 *  SubjectObserver.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 7/2/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_SUBJECT_OBSERVER_H_INCLUDED
#define FRESH_SUBJECT_OBSERVER_H_INCLUDED

#include <list>
#include <algorithm>

namespace fr
{
	
	template< typename SubjectT, typename DerivedT >
	class Observer
	{
	public:
		
		Observer()
		{
			SubjectT::instance().addObserver( static_cast< DerivedT* >( this ));
		}
		
		virtual ~Observer()
		{
			SubjectT::instance().removeObserver( static_cast< DerivedT* >( this ));
		}
	};
	
	template< typename ObserverT >
	class Subject
	{
	public:
		
		virtual ~Subject() {}
		
		virtual void addObserver( ObserverT* observer )
		{
			REQUIRES( observer );
			REQUIRES( !hasObserver( observer ));

			m_observers.push_back( observer );
			
			PROMISES( hasObserver( observer ));
		}
		
		virtual void removeObserver( ObserverT* observer )
		{
			ObserversI iter = findObserver( observer );
			REQUIRES( iter != m_observers.end() );
			
			m_observers.erase( iter );
			
			PROMISES( !hasObserver( observer ));
		}
		
		bool hasObserver( ObserverT* observer ) const
		{
			return findObserver( observer ) != m_observers.end();
		}
		
	protected:
		
		typedef std::list< ObserverT* > Observers;
		typedef typename Observers::iterator ObserversI;
		typedef typename Observers::const_iterator ObserversCI;
		
		ObserversI observers_begin()			{ return m_observers.begin(); }
		ObserversCI observers_begin()	const	{ return m_observers.begin(); }
		ObserversI observers_end()			{ return m_observers.end(); }
		ObserversCI observers_end() const		{ return m_observers.end(); }
		
		ObserversI findObserver( ObserverT* observer )
		{
			return std::find( m_observers.begin(), m_observers.end(), observer );
		}
		ObserversCI findObserver( ObserverT* observer ) const
		{
			return std::find( m_observers.begin(), m_observers.end(), observer );
		}
		
	private:
		
		Observers m_observers;
		
	};
	
}


#endif
