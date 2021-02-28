//
//  StateMachine.h
//  Fresh
//
//  Created by Jeff Wofford on 9/3/12.
//  Copyright (c) 2012 jeffwofford.com. All rights reserved.
//

#ifndef Fresh_StateMachine_h
#define Fresh_StateMachine_h

#include "FreshDebug.h"
#include <stack>
#include <map>
#include <memory>

namespace fr
{

	template< typename StateIdT, class StateT, typename StatePtrT = std::shared_ptr< StateT > >
	class StateMachine
	{
	public:
		
		typedef const StateIdT& StateIdRef;
		
		bool hasRegisteredState( StateIdRef idState ) const
		{
			return m_registeredStates.find( idState ) != m_registeredStates.end();
		}
		
		StateT& registeredState( StateIdRef idState ) const
		{
			auto iter = m_registeredStates.find( idState );
			ASSERT( iter != m_registeredStates.end() );
			return *( iter->second );
		}
		
		void registerState( StateIdRef idState, StatePtrT state )
		{
			REQUIRES( !hasRegisteredState( idState ));
			m_registeredStates[ idState ] = state;
			PROMISES( hasRegisteredState( idState ));
		}
		
		void unregisterState( StateIdRef idState, StatePtrT state )
		{
			REQUIRES( hasRegisteredState( idState ));
			m_registeredStates.erase( state );
			PROMISES( !hasRegisteredState( idState ));
		}
		
		bool isInSomeState() const
		{
			return !m_stateStack.empty();
		}
		
		StatePtrT currentState()
		{
			return m_registeredStates[ currentStateId() ];
		}

		const StatePtrT currentState() const
		{
			return m_registeredStates[ currentStateId() ];
		}
		
		StateIdRef currentStateId() const
		{
			ASSERT( isInSomeState() );
			return m_stateStack.top();
		}
		
		void gotoState( StateIdRef state )
		{
			StatePtrT priorState = nullptr;
			
			if( m_stateStack.empty() )
			{
				m_stateStack.push( state );
			}
			else if( state != m_stateStack.top() )
			{
				// Leave current state, if any.
				//
				currentState()->endState();
				
				priorState = currentState();
				
				m_stateStack.top() = state;
			}
			
			ASSERT( isInSomeState() );
			
			// Begin the new state.
			//
			currentState()->beginState( priorState );
						
			PROMISES( isInSomeState() );
			PROMISES( currentStateId() == state );
		}
		
		void pushState( StateIdRef state )
		{
			StatePtrT priorState = nullptr;
			
			if( !m_stateStack.empty() && state != m_stateStack.back() )
			{
				// Leave current state, if any.
				//
				currentState().endState();
				
				priorState = &currentState();
			}
			
			m_stateStack.push( state );
			
			// Begin the new state.
			//
			currentState().beginState( priorState );
			
			PROMISES( isInSomeState() );
			PROMISES( currentStateId() == state );
		}
		
		StateT& popState()
		{
			REQUIRES( isInSomeState() );
			
			// Leave current state, if any.
			//
			currentState().endState();
			
			const StateIdT oldBack = currentStateId();
			
			m_stateStack.pop();
			
			// Begin the new state.
			//
			if( !m_stateStack.empty() )
			{
				currentState().beginState( m_registeredStates[ oldBack ] );
			}
			
			return *( m_registeredStates[ oldBack ] );
		}
		
	private:
		
		std::map< StateIdT, StatePtrT > m_registeredStates;
		std::stack< StateIdT > m_stateStack;
	};

}

#endif
