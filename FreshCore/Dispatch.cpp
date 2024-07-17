//
//  Dispatch.cpp
//  Dispatch
//
//  Created by Jeff Wofford on 3/8/16.
//  Copyright © 2016 Jeff Wofford. All rights reserved.
//

#include "Dispatch.h"
#include "Profiler.h"

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <cassert>

namespace fr
{
	namespace dispatch
	{
		class QueueImpl
		{
		public:

			explicit QueueImpl( const std::string& queueName )
			:	m_queueName( queueName )
			{}

			~QueueImpl()
			{
				stop();
			}

			void maxExpectedBlockDuration( Duration duration, Queue::OverDurationCallback&& onOverDuration )
			{
				m_maxExpectedBlockDuration = duration;
				m_overDurationCallback = std::move( onOverDuration );
			}

			void async( Block::ptr block )
			{
				TIMER_AUTO_FUNC
				asyncAfter( Clock::now(), block );
			}

			void asyncAfter( TimePoint when, Block::ptr block )
			{
				TIMER_AUTO_FUNC
				std::lock_guard< decltype( m_stateMutex ) > lock{ m_stateMutex };
				m_blockQueue.push( { when, block } );
				m_hasWork.notify_one();
			}

			void run()
			{
				m_continue = true;
				if( !m_thread.joinable() )
				{
					m_thread = std::thread{ std::bind( &QueueImpl::maintainQueue, this )};
				}
			}

			bool running() const
			{
				return m_continue;
			}

			void runSync()
			{
				m_continue = true;
				maintainQueue();
				m_continue = false;
			}

			void clear()
			{
				TIMER_AUTO_FUNC
				std::lock_guard< decltype( m_stateMutex ) > lock{ m_stateMutex };
				m_blockQueue = decltype( m_blockQueue ){};	// Clear the block queue.
			}

			bool usesCurrentThread() const
			{
				return m_threadId == std::this_thread::get_id();
			}

			void stop()
			{
				TIMER_AUTO_FUNC
				std::unique_lock< decltype( m_stateMutex ) > lock{ m_stateMutex };
				if( m_continue )
				{
					m_continue = false;
					m_hasWork.notify_one();

					lock.unlock();

					// Join if the thread is joinable and we're not calling stop() from the same thread.
					//
					if( m_thread.joinable() && std::this_thread::get_id() != m_thread.get_id() )
					{
						m_thread.join();
						m_thread = {};
						m_threadId = std::thread::id{};
					}
				}
			}

			// If you called run() or runSync() on this queue, you probably shouldn't call poll(); it won't break
			// anything catastrophically, but you're mixing architectural approaches to how this queue maintains
			// its work.
			//
			void poll()
			{
				TIMER_AUTO_FUNC
				m_threadId = std::this_thread::get_id();
				std::unique_lock< decltype( m_stateMutex ) > lock{ m_stateMutex };

				while( !m_blockQueue.empty() )
				{
					bool queueStillPending = executeNextBlock( lock );

					// Though the queue may not be empty, we're still waiting for the time of the earliest
					// block. Break out of the polling loop and wait for the next call to `poll()`.
					//
					if(queueStillPending)
					{
						break;
					}
				}
			}

		protected:

			void maintainQueue()
			{
				TIMER_AUTO_FUNC

				// Store the thread ID.
				//
				m_threadId = std::this_thread::get_id();

				// Main block execution loop.
				//
				std::unique_lock< decltype( m_stateMutex ) > lock{ m_stateMutex };
				while( m_continue )
				{
					const TimePoint nextScheduledEvent = m_blockQueue.empty()
					? ( Clock::now() + std::chrono::hours{ 24*7*52 /* about a year; basically infinite */ } )
					: m_blockQueue.top().when;

					// Wait until any one of the following is true:
					//
					//	1. m_continue has reset (i.e. we're being asked to stop altogether).
					//	2. The block queue has become non-empty.
					//	3. The time of the next event has arrived.
					//
					const auto workPredicate = [this, nextScheduledEvent]()
					{
						// Return false to continue waiting; true to stop waiting to that blocks can be processed,
						// the thread aborted, or the wait rescheduled.

						// Thread stopping? Stop waiting.
						if( !m_continue ) return true;

						// Queue empty? Continue waiting.
						if( m_blockQueue.empty() ) return false;

						const auto soonestBlockTime = m_blockQueue.top().when;

						// Time for the next block? Stop waiting.
						if( Clock::now() >= soonestBlockTime ) return true;

						// The scheduled wakeup time should move sooner? Stop waiting.
						if( soonestBlockTime < nextScheduledEvent ) return true;

						// Otherwise, continue waiting.
						return false;
					};

					if( m_hasWork.wait_until( lock, nextScheduledEvent, workPredicate ))
					{
						// We've stopped waiting, but is there actual work to be done?
						// (Else, probably we just need to reschedule our wait event at the top of this loop.)
						//
						if( m_continue && Clock::now() >= m_blockQueue.top().when )
						{
							executeNextBlock( lock );
						}
					}
				}
			}

			// Returns true iff the queue should not undergo further processing at this time--
			// primarily because although it may be non-empty, the top (and thus earlest) block
			// isn't due yet.
			//
			bool executeNextBlock( std::unique_lock< std::mutex >& lock )
			{
				TIMER_AUTO_FUNC

				if( !m_blockQueue.empty() )
				{
					const auto scheduledBlock = m_blockQueue.top();		// Copy.

					// Time for this block to fire?
					//
					if( Clock::now() >= scheduledBlock.when )
					{
						// Pop and call the block.

						m_blockQueue.pop();
						lock.unlock();

						const auto startTime = Clock::now();		// Record how long the block takes.

						scheduledBlock.block->call();

						const auto endTime = Clock::now();

						// If the user has a requested a callback when blocks take too long and this block did so,
						// call the callback.
						//
						if( m_overDurationCallback )
						{
							const auto duration = endTime - startTime;
							if( duration > m_maxExpectedBlockDuration )
							{
								m_overDurationCallback( scheduledBlock.block->name(), duration );
							}
						}

						lock.lock();
						return false;		// `false` means the larger queue management loop may need to continue.
					}
				}
				return true;	// `true` means the larger queue management loop should not continue: there's no work here.
			}

		private:

			mutable std::mutex m_stateMutex;	// Used to make all internal accesses and changes atomically.

			struct ScheduledBlock
			{
				TimePoint when;
				Block::ptr block;

				bool operator<( const ScheduledBlock& other ) const
				{
					// This is kinda confusing: The earlier time (<) is higher priority (>).
					return when > other.when;
				}
			};

			std::priority_queue< ScheduledBlock > m_blockQueue;

			std::atomic_bool m_continue{ false };
			std::condition_variable m_hasWork;
			std::thread m_thread;
			std::atomic< std::thread::id > m_threadId{};

			Duration m_maxExpectedBlockDuration{ std::chrono::minutes{ 10 }};		// The default value is ridiculously long.
			Queue::OverDurationCallback m_overDurationCallback;

			const std::string m_queueName;
		};

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////

		Queue::Queue( const std::string& queueName )
		:	m_impl( new QueueImpl( queueName ))
		{}

		Queue::~Queue()
		{}

		void Queue::maxExpectedBlockDuration( Duration duration, OverDurationCallback&& onOverDuration )
		{
			assert( m_impl );
			m_impl->maxExpectedBlockDuration( duration, std::move( onOverDuration ));
		}

		void Queue::async( Block::ptr block )
		{
			assert( m_impl );
			m_impl->async( block );
		}

		void Queue::asyncAfter( TimePoint when, Block::ptr block )
		{
			assert( m_impl );
			m_impl->asyncAfter( when, block );
		}

		void Queue::run()
		{
			assert( m_impl );
			m_impl->run();
		}

		bool Queue::running() const
		{
			assert( m_impl );
			return m_impl->running();
		}

		void Queue::stop()
		{
			assert( m_impl );
			m_impl->stop();
		}

		void Queue::poll()
		{
			assert( m_impl );
			m_impl->poll();
		}

		void Queue::runSync()
		{
			assert( m_impl );
			m_impl->runSync();
		}

		void Queue::clear()
		{
			assert( m_impl );
			return m_impl->clear();
		}

		bool Queue::usesCurrentThread() const
		{
			assert( m_impl );
			return m_impl->usesCurrentThread();
		}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////

		Queue& mainQueue()
		{
			static Queue queue{ "fr_main_queue" };
			return queue;
		}

		Queue& globalQueue()
		{
			static Queue queue{ "fr_global_queue" };

			if( !queue.running() )
			{
				queue.run();
			}

			return queue;
		}

		bool onMainQueue()
		{
			return mainQueue().usesCurrentThread();
		}
	}
}