/*
 *  EventDispatcher.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 7/2/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_EVENT_DISPATCHER_H_INCLUDED
#define FRESH_EVENT_DISPATCHER_H_INCLUDED

#include "Object.h"
#include "Event.h"
#include "Profiler.h"
#include "Property.h"
#include <list>


// Handy macro for (slightly) simpler declaration of CallbackFunction methods and their associated objects. 
// The objects will still need to be initialized in the constructor and the method actually defined.
//
#define FRESH_DECLARE_CALLBACK( ownerClass, fnName, eventType );	\
	typedef fr::EventDispatcher::CallbackFunction< ownerClass, eventType > Callback##fnName; \
	Callback##fnName::ptr m_callback##fnName = new Callback##fnName( Callback##fnName::StaticGetClassInfo(), this, &ownerClass::fnName, #fnName );	\
	virtual void fnName( const eventType& event )

#define FRESH_DEFINE_CALLBACK( ownerClass, fnName, eventType )	void ownerClass::fnName( const eventType& event )

// Use this macro to reference a previously declared and initialized callback from within the class
// that owns it.
//
#define FRESH_CALLBACK( fnName ) m_callback##fnName



namespace fr
{

		
	// An EventDispatcher knows how to allow registration and de-registration of "listeners" and to send events to those listeners' member functions.
	// It is the base class for DisplayObject.
	//
	// This system is based loosely on Adobe Flash's Event/EventDispatcher model. For more information see http://help.adobe.com/en_US/AS3LCR/Flash_10.0/flash/events/EventDispatcher.html
	//
	class EventDispatcher : public Object
	{
	public:
		
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//
		// CallbackFunctionAbstract and CallbackFunction are classes that facilitate EventDispatchers being able to call
		// an arbitrary member function (which returns void and takes an Event-derived parameter) on a specific object.
		//
		// To create your own callback function and ask an EventDispatcher to call it when a specific event happens, do the following:
		//
		// In your listener class (e.g. Foo), declare and define the member function you want to actually handle the event.
		// This function must return void and can take any Event-derived class as a parameter.
		//
		//		void TheFunctionToActuallyCallBack( const EventWhatever& event );
		//
		// In your listener class, define a CallbackFunction member datum:
		//
		//		CallbackFunction< Foo, fr::EventWhatever, &Foo::TheFunctionToActuallyCallBack > m_callback;
		//
		// In your listener class constructor, construct the CallbackFunction datum by passing *this* into its constructor:
		//
		//		Foo::Foo() : m_callback( this ) {}
		//
		// Finally, register this callback object with the object you want to listen to. You can do this at construction or whenever it makes sense.
		// (For example, you might not want to listen for TouchEnd events until you've actually received a TouchBegin event.)
		//
		//		Foo::Foo( DisplayObject* eventDispatcher ) : m_callback( this ) { eventDispatcher->addEventListener( EventWhatever::NOW, m_callback ); }
		//
		// Now the callback function TheFunctionToActuallyCallBack() will be called whenever the EventWhatever::NOW event is dispatched.
		//
		
		class CallbackFunctionAbstract : public Object		// Derived from Object for weak ptr support
		{
		public:
			
			virtual void operator()( const Event* ) = 0;
			virtual void setHoldOwnerStrongly( bool hold ) = 0;
			virtual Object::wptr owner() const = 0;
			
			FRESH_DECLARE_CLASS_ABSTRACT( CallbackFunctionAbstract, Object )
		};

		/////////////

		template< typename ListenerType, typename EventType >
		class CallbackFunction : public CallbackFunctionAbstract
		{
		public:
			CallbackFunction( const ClassInfo& assignedClassInfo, ListenerType* owner, void (ListenerType::*Method)( const EventType& ), NameRef objectName = DEFAULT_OBJECT_NAME )
			:	CallbackFunctionAbstract( assignedClassInfo, objectName )
			,	m_owner( owner )
			,	m_ownerStronglyHeld( nullptr )
			,	m_fnMethod( Method )
			{
				REQUIRES( m_owner );
			}
			
			virtual void operator()( const Event* e ) override
			{
#ifdef FRESH_PROFILER_ENABLED
				fr::Profiler::AutoTimer autoTimer_(( name() + " CALLBACK" ).c_str());
#endif

				ASSERT( m_owner );
				ASSERT( m_fnMethod );
				
				// Ensure that this object is retained at least for the duration of this call.
				//
				SmartPtr< ListenerType > strong( m_owner );	// Same intent as std::weak_ptr<>::lock().
				(strong.get()->*m_fnMethod)( *static_cast< const EventType* >( e ));					// TODO dynamic_cast<> might be wise.
			}
			
			void setHoldOwnerStrongly( bool hold ) override
			{
				if( hold )
				{
					m_ownerStronglyHeld = m_owner;
				}
				else
				{
					m_ownerStronglyHeld = nullptr;
				}
			}
			
			virtual Object::wptr owner() const override
			{
				return m_owner;
			}
			
		private:
			
			WeakPtr< ListenerType > m_owner;
			SmartPtr< ListenerType > m_ownerStronglyHeld;
			void (ListenerType::*m_fnMethod)( const EventType& );
		};

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		void addEventListener( Event::TypeRef type, CallbackFunctionAbstract::ptr fnCallback, bool strongReference = false );
		void removeEventListener( Event::TypeRef type, CallbackFunctionAbstract::ptr fnCallback );
		bool hasEventListener( Event::TypeRef type, CallbackFunctionAbstract::cptr fnCallback ) const;
		void clearEventListeners();
		void clearEventListeners( Event::TypeRef type );
		
		void dispatchEvent( const Event* event );
		
		virtual void postLoad() override;
		
		void debugPrintListeners() const;
		
	protected:
		
		void clearNullEventListeners();
		
	private:
		
		typedef std::pair< Event::Type, CallbackFunctionAbstract::wptr > WeakListenerRecord;
		typedef std::list< WeakListenerRecord > ListWeakListeners;
		typedef ListWeakListeners::iterator ListWeakListenersI;
		typedef ListWeakListeners::const_iterator ListWeakListenersCI;

		typedef std::pair< Event::Type, CallbackFunctionAbstract::ptr > StrongListenerRecord;
		typedef std::list< StrongListenerRecord > ListStrongListeners;
		typedef ListStrongListeners::iterator ListStrongListenersI;
		typedef ListStrongListeners::const_iterator ListStrongListenersCI;
		
		ListWeakListeners m_listWeakListeners;
		ListStrongListeners m_listStrongListeners;
		
		FRESH_DECLARE_CLASS( EventDispatcher, Object )
		
	};


	inline EventDispatcher::CallbackFunctionAbstract::CallbackFunctionAbstract( const ClassInfo& assignedClassInfo, NameRef objectName )
	: Super( assignedClassInfo, objectName )
	{}
	
	
}

#endif
