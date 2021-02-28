//
//  FreshThread.h
//  Fresh
//
//  Created by Jeff Wofford on 12/1/14.
//
//

#ifndef Fresh_FreshThread_h
#define Fresh_FreshThread_h

#include "Dispatch.h"

#if FRESH_ALLOW_THREADING
#	include <thread>
#	include <mutex>
#	include <atomic>
#endif

namespace fr
{
#if FRESH_ALLOW_THREADING
	const size_t DEFAULT_PARALLEL_FOR_THREADS = 4;
#else
	const size_t DEFAULT_PARALLEL_FOR_THREADS = 1;
#endif
	
	template< typename IterT, typename FunctionT >
	void parallelFor( IterT begin, const IterT end, FunctionT&& fnEach, size_t maxThreads = DEFAULT_PARALLEL_FOR_THREADS )
	{
#if FRESH_ALLOW_THREADING
		
		// The work function.
		//
		const auto work = [&]( IterT threadRangeBegin, const IterT threadRangeEnd )
		{
			for( ; threadRangeBegin != threadRangeEnd; ++threadRangeBegin )
			{
				fnEach( *threadRangeBegin );
			}
		};
		
		const size_t threadsToUseIncludingThisOne = maxThreads > 0 ? maxThreads : 1;
		std::vector< std::thread > threads( threadsToUseIncludingThisOne - 1 );
		
		const size_t perThreadSpanSize = std::distance( begin, end ) / threadsToUseIncludingThisOne;
		
		for( size_t i = 0; i < threads.size(); ++i )
		{
			const auto threadBegin = begin + i * perThreadSpanSize;
			const auto threadEnd = threadBegin + perThreadSpanSize;
			
			threads[ i ] = std::thread{ std::bind( work, threadBegin, threadEnd ) };
		}
		
		// Do some of the work on this thread.
		//
		work( begin + threads.size() * perThreadSpanSize, end );
		
		// Wait for the other threads to complete.
		//
		for( auto& thread : threads )
		{
			thread.join();
		}
		
#else
		for( ; begin != end; ++begin )
		{
			fnEach( *begin );
		}
#endif
	}
	
}

#endif		// Fresh_FreshThread_h
