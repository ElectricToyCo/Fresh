//
//  Dispatch.h
//  Dispatch
//
//  Created by Jeff Wofford on 3/8/16.
//  Copyright © 2016 Jeff Wofford. All rights reserved.
//

#ifndef FR_DISPATCH_H_INCLUDED_
#define FR_DISPATCH_H_INCLUDED_

#include <memory>
#include <string>
#include <functional>
#include <chrono>

namespace fr
{
	namespace dispatch
	{
		using Clock = std::chrono::system_clock;
		using TimePoint = Clock::time_point;
		using Duration = Clock::duration;
		
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		class Block
		{
		public:
			
			using Function = std::function< void() >;
			using ptr = std::shared_ptr< Block >;
			
			Block( Function&& fn, std::string&& name = {} )
			:	m_function( std::move( fn ))
			,	m_name( std::move( name ))
			{}

			virtual ~Block() {}
			
			virtual void call()
			{
				m_function();
			}
			
			const std::string& name() const			{ return m_name; }
			
		private:
			
			Function m_function;
			std::string m_name;
		};
		
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		template< typename HostT >
		class BoundBlock : public Block
		{
		public:
			using host_t = HostT;
			using shared_ptr = std::shared_ptr< host_t >;
			using weak_ptr = std::weak_ptr< host_t >;
			
			BoundBlock( const shared_ptr& host, Function&& fn, bool weak = true, std::string&& name = {} )
			:	Block( std::move( fn ), std::move( name ))
			,	m_strongHost( weak ? nullptr : host )
			,	m_weakHost(   weak ? host : nullptr )
			{}
			
			virtual void call() override
			{
				if( auto host_ = host() )
				{
					Block::call();
				}
				
				// Block executed. Release the host.
				//
				m_strongHost.reset();
			}
			
			shared_ptr host() const
			{
				shared_ptr host_ = m_strongHost;
				if( !host_ )
				{
					host_ = m_weakHost.lock();
				}
				return host_;
			}
			
		private:
			
			shared_ptr m_strongHost;
			weak_ptr m_weakHost;
		};
		
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		class Queue
		{
		public:
			
			explicit Queue( const std::string& queueName = {} );
			~Queue();
			
			using OverDurationCallback = std::function< void( const std::string& blockName, Duration totalBlockDuration ) >;
			void maxExpectedBlockDuration( Duration duration, OverDurationCallback&& onOverDuration );
			
			void async( Block::ptr block );
			void asyncAfter( TimePoint when, Block::ptr block );
			
			void run();
			bool running() const;
			void stop();
			void poll();
			
			void runSync();
			
			// Removes all pending blocks (added via `async*()`) from the queue.
			void clear();
			
			// Useful for debugging. Don't rely on this for real functionality.
			// Only valid when `running()` or when `poll()` has been called when not `running()`.
			//
			bool usesCurrentThread() const;
			
		private:
			
			std::unique_ptr< class QueueImpl > m_impl;
		};
		
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		Queue& mainQueue();			// The queue associated with the main thread.
		Queue& globalQueue();		// A utility queue for background work.
		
		// For debugging. Don't rely on this functionality.
		
		// Returns true iff the caller's thread is the mainQueue's thread.
		//
		bool onMainQueue();
	}
}

#define DISPATCH_CODE_LOCATION ( std::string{ __FILE__ } + ":" + std::to_string( __LINE__ ))
#define DISPATCH_BLOCK( fn_scope ) std::make_shared< fr::dispatch::Block >( [=]() fn_scope, DISPATCH_CODE_LOCATION )
#define DISPATCH_BOUND( host_ptr, fn_scope ) std::make_shared< fr::dispatch::BoundBlock< decltype( host_ptr )::element_type >>( (host_ptr), [=]() fn_scope, true, DISPATCH_CODE_LOCATION )


#endif	// FR_DISPATCH_H_INCLUDED_

