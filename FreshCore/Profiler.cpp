// Copyright 2006 Jeff Wofford all rights reserved.

#include "Profiler.h"
#include "FreshEssentials.h"
#include "FreshTime.h"
#include <cstring>			// For strlen
#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <memory>
using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace fr
{

	class ProfilerImpl
	{
	public:
		ProfilerImpl();
		~ProfilerImpl();
		
		void timerBegin( const char* szScope );
		void timerEnd( const char* szScope );

		void clear();

		void dumpAsText( std::ostream& stream ) const;

	private:

		class Timer : public std::enable_shared_from_this< Timer >
		{
		public:
			typedef std::unordered_map< std::string, std::shared_ptr< Timer >> TimerMap;
			typedef TimerMap::iterator TimerMapI;
			typedef TimerMap::const_iterator TimerMapCI;

			Timer( std::shared_ptr< Timer > parent, const char* szScope, SystemClock clocks = 0, size_t nHits = 0 );
				//	REQUIRES( szScope != 0 && strlen( szScope ) > 0 );

			~Timer();

			const std::string& scope() const { return m_szScope; }

			bool isScope( const char* szScope )	const	{ return m_szScope == szScope; }
			std::shared_ptr< Timer > parent() const	{ return m_parentTimer.lock(); }
			std::shared_ptr< const Timer > getRoot() const	
			{ 
				if( parent() )
				{
					return parent()->getRoot();
				}
				else
				{
					return shared_from_this();
				}
			}

			void recordStartTime();
			void recordElapsed();

			std::shared_ptr< Timer > subTimerBegin( const char* szScope );
				//	REQUIRES( szScope != 0 && strlen( szScope ) > 0 );

			std::shared_ptr< Timer > findSubTimer( const char* szScope ) const;
				//	REQUIRES( szScope != 0 && strlen( szScope ) > 0 );
			std::shared_ptr< Timer > addSubTimer( const char* szScope );
				//	REQUIRES( szScope != 0 && strlen( szScope ) > 0 );

			SystemClock dumpAsText( std::ostream& stream, int iDepth = 0 ) const;

		private:
			const std::string m_szScope;
			SystemClock	m_elapsedClocks;
			size_t		m_nHits;

			SystemClock				m_startTime;

			std::weak_ptr< Timer > 	m_parentTimer;
			TimerMap				m_mapTimers;

			Timer( const Timer& );				// Invalidate copying
			Timer& operator=( const Timer& );
		};

		std::shared_ptr< Timer > m_rootTimer;
		std::shared_ptr< Timer > m_currentScope;
	};

	ProfilerImpl::ProfilerImpl() 
	{
		m_rootTimer = std::make_shared< Timer >( nullptr, "ROOT" );
		m_currentScope = m_rootTimer;

		clear();

		ASSERT( m_rootTimer );
		ASSERT( m_currentScope );
	}

	ProfilerImpl::~ProfilerImpl()
	{}

	void ProfilerImpl::timerBegin( const char* szScope )
	{
		REQUIRES( szScope && strlen( szScope ) > 0 );
		ASSERT( m_currentScope );
		m_currentScope = m_currentScope->subTimerBegin( szScope );
	}

	void ProfilerImpl::timerEnd( const char* szScope )
	{
		REQUIRES( szScope && strlen( szScope ) > 0 );
		ASSERT_MSG( m_currentScope->isScope( szScope ), "Profile::timerEnd(" << szScope 
			<< ") tried to end scope though the current scope is " 
			<< m_currentScope->scope() );

		m_currentScope->recordElapsed();
		m_currentScope = m_currentScope->parent();
		ASSERT( m_currentScope );						// If fails, found too many TIMER_END()s 
	}

	void ProfilerImpl::clear()
	{
		m_rootTimer->recordStartTime();
	}

	void ProfilerImpl::dumpAsText( ostream& stream ) const
	{
		ASSERT( m_currentScope );
		ASSERT( m_rootTimer );

		m_rootTimer->recordElapsed();

		stream << "<html><head><link rel='stylesheet' type='text/css' href='profile_style.css' /></head><body>\n";

		if( m_currentScope != m_rootTimer )
		{
			// Outstanding BEGINS are still hanging around, unmatched by ENDs.
			stream << "WARNING: More TIMER_BEGINs than TIMER_ENDs\n";
		}

		stream << "<table>\n";

		// Spew the header row.

		stream << "<tr>";

		stream << "<td>" << "SCOPE" << "</td>";
		stream << "<td>" << "MS" << "</td>";
		stream << "<td>" << "% TOTAL" << "</td>";
		stream << "<td>" << "% CALLER" << "</td>";
		stream << "<td>" << "CALLS" << "</td>";
		stream << "<td>" << "MS PER CALL" << "</td>";

		stream << "</tr>\n";

		m_rootTimer->dumpAsText( stream );

		stream << "</table>\n";
		stream << "</body></html>";

		m_rootTimer->recordStartTime();
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ProfilerImpl::Timer::Timer( std::shared_ptr< Timer > parent, const char* szScope, SystemClock clocks /* = 0 */, size_t nHits /* = 0 */ )
	:	m_szScope( szScope )
	,	m_elapsedClocks( clocks )
	,	m_nHits( nHits )
	,	m_parentTimer( parent )
	{
		REQUIRES( szScope && strlen( szScope ) > 0 );
		ASSERT( szScope && strlen( szScope ) > 0 );
	}

	ProfilerImpl::Timer::~Timer()
	{}

	void ProfilerImpl::Timer::recordStartTime()
	{
		++m_nHits;
		m_startTime = getAbsoluteTimeClocks();

		for( auto& pair : m_mapTimers )
		{
			pair.second->recordStartTime();
		}
	}

	void ProfilerImpl::Timer::recordElapsed()
	{
		m_elapsedClocks += getAbsoluteTimeClocks() - m_startTime;
	}

	std::shared_ptr< ProfilerImpl::Timer > ProfilerImpl::Timer::subTimerBegin( const char* szScope )
	{
		REQUIRES( szScope && strlen( szScope ) > 0 );
		std::shared_ptr< Timer > subTimer = findSubTimer( szScope );
		
		if( !subTimer )
		{
			subTimer = addSubTimer( szScope );
		}

		ASSERT( subTimer );

		subTimer->recordStartTime();

		return subTimer;
	}

	std::shared_ptr< ProfilerImpl::Timer > ProfilerImpl::Timer::findSubTimer( const char* szScope ) const
	{
		REQUIRES( szScope && strlen( szScope ) > 0 );
		TimerMapCI iter = m_mapTimers.find( szScope );

		if( iter != m_mapTimers.end() )
		{
			return iter->second;
		}
		else
		{
			return 0;
		}
	}

	std::shared_ptr< ProfilerImpl::Timer > ProfilerImpl::Timer::addSubTimer( const char* szScope )
	{
		REQUIRES( szScope && strlen( szScope ) > 0 );
		std::shared_ptr< Timer > subTimer = std::make_shared< Timer >( shared_from_this(), szScope );

		m_mapTimers[ szScope ] = subTimer;

		return subTimer;
	}

	SystemClock ProfilerImpl::Timer::dumpAsText( std::ostream& stream, int iDepth /*= 0*/ ) const
	{
		// Dump this timer's info.
		stream << "<tr>";

		stream << "<td>";

		int i = iDepth;
		while( i-- > 0 )
		{
			stream << "&nbsp;&nbsp;";
		}

		stream << m_szScope << "</td>";
		stream << "<td>" << std::fixed << std::setprecision( 3 ) << ( clocksToSeconds( m_elapsedClocks ) * 1000.0 ) << "</td>";
		
		// % of total
		stream << "<td>";
		stream << std::fixed << std::setprecision( 2 ) << ( m_elapsedClocks / static_cast< double >( getRoot()->m_elapsedClocks )) * 100.0;
		stream << "</td>";

		// % of parent ("caller")
		stream << "<td>";
		if( auto parent = m_parentTimer.lock() )
		{
			stream << std::fixed << std::setprecision( 2 ) << ( m_elapsedClocks / static_cast< double >( parent->m_elapsedClocks )) * 100.0;
		}
		else
		{
			stream << "-";
		}
		stream << "</td>";
				
		stream << "<td>" << m_nHits << "</td>";
		stream << "<td>" << std::fixed << std::setprecision( 2 ) << ( m_nHits ? ( clocksToSeconds( m_elapsedClocks ) / m_nHits * 1000.0 ) : 0 ) << "</td>";

		stream << "</tr>\n";

		// Dump all child timers' infos.
		SystemClock totalChildTime = 0;
		for( TimerMapCI iter = m_mapTimers.begin(); iter != m_mapTimers.end(); ++iter )
		{
			totalChildTime += iter->second->dumpAsText( stream, iDepth + 1 );
		}

		// Report (as if it were a child) how much time in this timer is not accounted for by children.
		if( m_mapTimers.size() > 0 )
		{
			stream << "<tr>";

			stream << "<td>";

			i = iDepth;
			while( i-- > 0 )
			{
				stream << "&nbsp;&nbsp;";
			}

			SystemClock remainingTime = m_elapsedClocks - totalChildTime;

			stream << "&nbsp;&nbsp;&lt;" << m_szScope << "&gt;</td>";
			stream << "<td>" << std::fixed << std::setprecision( 3 ) << ( clocksToSeconds( remainingTime ) * 1000.0 ) << "</td>";

			// % of total
			stream << "<td>";
			stream << std::fixed << std::setprecision( 2 ) << ( remainingTime / static_cast< double >( getRoot()->m_elapsedClocks )) * 100.0;
			stream << "</td>";

			// % of parent ("caller")
			stream << "<td>";
			if( auto parent = m_parentTimer.lock() )
			{
				stream << std::fixed << std::setprecision( 2 ) << ( remainingTime / static_cast< double >( m_elapsedClocks )) * 100.0;
			}
			else
			{
				stream << "-";
			}
			stream << "</td>";

			stream << "<td>" << m_nHits << "</td>";
			stream << "<td>" << ( m_nHits ? ( clocksToSeconds( remainingTime ) / m_nHits * 1000.0 ) : 0 ) << "</td>";

			stream << "</tr>\n";
		}

		return m_elapsedClocks;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	fr::Profiler::Profiler()
	:	Singleton< Profiler >( this )
	,	m_impl( new ProfilerImpl )
	{
		// Intentionally blank.
	}

	fr::Profiler::~Profiler()
	{}

	void Profiler::timerBegin( const char* szScope )
	{
		m_impl->timerBegin( szScope );
	}

	void Profiler::timerEnd( const char* szScope )
	{
		m_impl->timerEnd( szScope );
	}

	void Profiler::clear()
	{
		m_impl->clear();
	}

	void Profiler::dumpAsText( ostream& stream ) const
	{
		m_impl->dumpAsText( stream );
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Profiler::AutoTimer::AutoTimer( const char* szScope )
	: m_szScope( szScope )
	{
		REQUIRES( szScope && strlen( szScope ) > 0 );

		Profiler::instance().timerBegin( m_szScope.c_str() );
	}

	Profiler::AutoTimer::~AutoTimer()
	{
		Profiler::instance().timerEnd( m_szScope.c_str() );
	}

}		// END namespace fr
