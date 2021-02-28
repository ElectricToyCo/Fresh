/*
 *  ChangeHistory.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 1/29/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#ifndef _FRESH_CHANGE_HISTORY_H_INCLUDED_
#define _FRESH_CHANGE_HISTORY_H_INCLUDED_

#include <list>
#include <functional>

namespace fr
{

	template< typename HistoryStateT >
	class ChangeHistory
	{
	public:
		typedef HistoryStateT history_state_t;
		
		explicit ChangeHistory( const std::function< void( const history_state_t& ) >& fnEnactState ):	m_fnEnactState( fnEnactState )
		{
			assert( m_fnEnactState );
			m_currentState = m_historyStates.end();
		}
		
		virtual ~ChangeHistory()
		{}
		
		void clear()
		{
			m_historyStates.clear();
			m_currentState = m_historyStates.end();
		}
		
		void addState( const history_state_t&& state )
		{
			if( m_currentState != m_historyStates.end() )
			{
				auto next = m_currentState;
				++next;
				m_historyStates.erase( next, m_historyStates.end() );
			}
			m_historyStates.emplace_back( state );
			m_currentState = m_historyStates.end();
			--m_currentState;
		}
		
		bool canUndo() const
		{
			return m_currentState != m_historyStates.begin() && m_currentState != m_historyStates.end();
		}
		
		bool canRedo() const
		{
			auto next = m_currentState;
			++next;
			return m_currentState != m_historyStates.end() && next != m_historyStates.end();
		}
		
		void undo()
		{
			assert( canUndo() );
			--m_currentState;
			m_fnEnactState( *m_currentState );
		}
				
		void redo()
		{
			assert( canRedo() );
			++m_currentState;
			m_fnEnactState( *m_currentState );
		}
		
		size_t size() const
		{
			return m_historyStates.size();
		}
		
		size_t currentIndex() const
		{
			if( m_currentState != m_historyStates.end() )
			{
				return std::distance( m_historyStates.begin(), static_cast< typename std::list< history_state_t >::const_iterator >( m_currentState ));
			}
			else
			{
				return -1;
			}
		}
		
		template< typename fn_t >
		void forEachState( fn_t&& fn ) const
		{
			size_t index = 0;
			for( const auto& state : m_historyStates )
			{
				fn( state, index++ );
			}
		}

	private:
		
		std::list< history_state_t > m_historyStates;
		typename std::list< history_state_t >::iterator m_currentState;
		
		std::function< void( const history_state_t& ) > m_fnEnactState;
	};
	
	
}

#endif
