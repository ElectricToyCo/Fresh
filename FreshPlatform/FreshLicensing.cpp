//
//  FreshLicensing.cpp
//  Fresh
//
//  Created by Jeff Wofford on 12/3/16.
//
//

#include "FreshLicensing.h"
#include "FreshEssentials.h"
#include "FreshDebug.h"
#include <limits>
#include <algorithm>
#include <set>

namespace
{
    using DelegateSet = std::set< fr::licensing::Delegate* >;
    fr::Global< DelegateSet > g_delegates;
}

namespace fr
{
	namespace licensing
	{
		Version::Version()
		{}
		
		Version::Version( const std::string& versionString )
		:	m_versionString( versionString )
		{
			std::istringstream stream{ m_versionString };
			
			// Split version string into substrings.
			//
			std::vector< std::string > versionComponents;
			
			std::string token;
			while( std::getline( stream, token, '.' ))
			{
				versionComponents.push_back( std::move( token ));
			}
			
			// Convert substrings to numbers.
			//
			bool succeeded = true;
			std::transform( versionComponents.begin(),
						   versionComponents.end(),
						   std::back_inserter( m_versionNumbers ),
						   [&succeeded]( const std::string& component )
						   {
							   char* end;
							   long value = std::strtol( component.c_str(), &end, 10 );
							   
							   // Read whole string?
							   //
							   if( *end != '\0' )
							   {
								   succeeded = false;
							   }
							   
							   // Out of range?
							   //
							   if( value < 0 || value > std::numeric_limits< long >::max() )
							   {
								   succeeded = false;
							   }
							   
							   return value;
						   } );
			
			if( !succeeded )
			{
				m_versionNumbers.clear();
			}
		}
		
		bool Version::comparable() const
		{
			return m_versionNumbers.empty() == false;
		}
		
		const std::string& Version::string() const
		{
			return m_versionString;
		}
		
		bool Version::operator==( const Version& other ) const
		{
			REQUIRES( comparable() && other.comparable() );
			
			return	m_versionNumbers.size() == other.m_versionNumbers.size() &&
			std::equal( m_versionNumbers.begin(), m_versionNumbers.end(), other.m_versionNumbers.begin() );
		}
		
		bool Version::operator!=( const Version& other ) const
		{
			return !operator==( other );
		}
		
		bool Version::operator<( const Version& other ) const
		{
			// Examples:
			//		1.0		<	1.1
			//		1.0.2	<	1.1
			//		1.1		!<	1.1
			//		1.1		<	1.1.0
			//		1.0		<	1.0.0
			//		1.1		!<	1.0.0
			//		1.0		<	2.0
			//		1.3		<	2.0
			//		1.100	<	2.0
			//		1.2.3	<	2.0
			
			REQUIRES( comparable() && other.comparable() );
			for( size_t i = 0; i < m_versionNumbers.size(); ++i )
			{
				// Are we equivalent up to this point but shorter (fewer components)?
				if( i >= other.m_versionNumbers.size() )
				{
					return true;
				}
				
				const auto me = m_versionNumbers[ i ];
				const auto him = other.m_versionNumbers[ i ];
				
				// Are we smaller at this point?
				//
				if( me < him )
				{
					return true;
				}
				
				// Are we larger at this point?
				//
				if( me > him )
				{
					return false;
				}
				
				// Otherwise we're the same, so keep looking.
			}
			
			// We were the same until we hit the end.
			// If the other version has more components still, we're smaller, else we're the same.
			return m_versionNumbers.size() < other.m_versionNumbers.size();
		}
		
		bool Version::operator<=( const Version& other ) const
		{
			return operator==( other ) || operator<( other );
		}
		
		bool Version::operator>( const Version& other ) const
		{
			return !operator<=( other );
		}
		
		bool Version::operator>=( const Version& other ) const
		{
			return !operator<( other );
		}
		
        //////////////////////////////////////////////////////////////////////////////////////////////////
        
        void addDelegate( Delegate* delegate )
        {
            REQUIRES( delegate );
            g_delegates.with( [&]( DelegateSet& delegates ) { delegates.insert( delegate ); } );
            addDelegate_platform( delegate );
        }
        
        void removeDelegate( Delegate* delegate )
        {
            REQUIRES( delegate );
            removeDelegate_platform( delegate );
            
            g_delegates.with( [&]( DelegateSet& delegates )
                             {
                                 const auto iter = delegates.find( delegate );
                                 if( iter != delegates.end() )
                                 {
                                     delegates.erase( iter );
                                 }
                             } );
        }
        
        void eachDelegate( std::function< void( Delegate& ) >&& fn )
        {
            g_delegates.with( [&]( DelegateSet& delegates )
                             {
                                 for( const auto delegate : delegates )
                                 {
									 ASSERT( delegate );
                                     fn( *delegate );
                                 }
                             } );
        }
        
		//////////////////////////////////////////////////////////////////////////////////////////////////

		Error::Error()
		:	m_code{ 0 }
		,	m_message{}
		{}
		
		Error::Error( const std::string& message, int code )
		:	m_code{ code }
		,	m_message{ message }
		{}
		
		bool Error::isError() const
		{
			return m_message.empty() == false;
		}
		
		//////////////////////////////////////////////////////////////////////////////////////////////////
		
		Delegate::~Delegate()
		{
			removeDelegate( this );
		}
	}
}
