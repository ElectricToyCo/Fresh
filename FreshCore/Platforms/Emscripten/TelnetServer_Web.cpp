//
//  TelnetServer_Web.cpp
//  Fresh
//
//  Created by Jeff Wofford on 2/8/12.
//  Implementation guided by http://homepages.ius.edu/rwisman/C490/html/tcp.htm.
//  Copyright (c) 2012 jeffwofford.com. All rights reserved.
//

#include "FreshDebug.h"
#include "TelnetServer.h"
#include "Objects.h"
#include <map>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>

namespace
{
#ifndef SO_NOSIGPIPE
#	define SO_NOSIGPIPE MSG_NOSIGNAL
#endif
	
	void setToNonBlock( int socket )
	{
		// Set to non-blocking mode.
		//
		int flags = ::fcntl( socket, F_GETFL );		// Get the current flags.
		if( flags == -1 )
		{
			// Server is failing. Bail out.
			dev_warning( "TelnetServer failed fcntl() call. Aborting listen." );
			return;
		}
		
		if( ::fcntl( socket, F_SETFL, flags | O_NONBLOCK ) == -1 )
		{
			dev_warning( "TelnetServer failed to set non-blocking mode. Telnet will block." );
		}
	}
}

namespace fr
{
	
	class TelnetServer_bsd : public TelnetServer
	{
	public:
		
		virtual void beginListening( const Callback& callback, int port, const std::string& welcomeMessage = "Connected.\n" ) override
		{
			ASSERT( !getCallbackOnDataReceived() );
			ASSERT( callback );
			
			TelnetServer::beginListening( callback, port, welcomeMessage );
			
			// Attach the SIGPIPE handler to SIG_IGN to avoid exceptions when writing to a disconnected client.
			//
			::signal( SIGPIPE, SIG_IGN );
			
			m_socket = ::socket( PF_INET, SOCK_STREAM, IPPROTO_TCP );

			if( m_socket < 0 )
			{
				dev_warning( "TelnetServer failed socket() call. Aborting listen." );
				return;
			}

			setToNonBlock( m_socket );
			
			struct sockaddr_in socketAddress;
			::memset( &socketAddress, 0, sizeof( socketAddress ));
			
			socketAddress.sin_family = AF_INET;
			socketAddress.sin_port = htons( portNumber() );
			
			{
				int socketOption = 1;		// A pigeon boolean: int === bool, 1 === true.
				::setsockopt( m_socket, 
							SOL_SOCKET, 
							SO_REUSEADDR,
							&socketOption, sizeof( socketOption ));
			}
			
			{
				int socketOption = 1;
				::setsockopt( m_socket,
							 SOL_SOCKET,
							 SO_NOSIGPIPE,
							 &socketOption, sizeof( socketOption ));
			}
			
			::bind( m_socket, reinterpret_cast< sockaddr* >( &socketAddress ), sizeof( socketAddress ));
			::listen( m_socket, 5 /* maximum connections */ );			
		}
		
		virtual bool isListening() const override
		{
			return m_socket >= 0;
		}
	
		virtual bool isConnected() const override
		{
			return !m_connections.empty();
		}
		
		virtual void broadcast( const std::string& text ) override
		{
			// Send to all connections.
			//
			for( const auto& pair : m_connections )
			{
				::send( pair.first, text.data(), text.size(), 0 /* flags */ );
			}
		}
		
		virtual ~TelnetServer_bsd()
		{
			::close( m_socket );
		}

		virtual void updateConnections() override
		{
			if( m_socket >= 0 )
			{
				// Check for new connections.
				//
				int connectedSocket = ::accept( m_socket, nullptr, nullptr );
				if( connectedSocket >= 0 )
				{
					// Found a connection. Set up data transmission.
					//
					setToNonBlock( connectedSocket );
					
					Connection connection;
					connection.m_callback = getCallbackOnDataReceived();
					m_connections[ connectedSocket ] = connection;
			
					// Send welcome message.
					//
					const auto& welcome = welcomeMessage();
					
					if( !welcome.empty() )
					{
						::send( connectedSocket, welcome.data(), welcome.size(), 0 /* flags */ );
					}
				}

				// Update ongoing connections.
				//
				std::vector< unsigned char > bytes;

				for( auto iter = m_connections.begin(); iter != m_connections.end();  )
				{
					const auto& connection = *iter;
					
					const size_t BUF_LEN = 256;
					bytes.resize( BUF_LEN );

					const auto nBytes = ::recv( connection.first, bytes.data(), BUF_LEN, 0 );
					
					if( nBytes == 0 || nBytes == ssize_t( -1 ))
					{
						// Error or just nothing to read (non-blocking)?
						//
						if( errno != EWOULDBLOCK )
						{
							dev_warning( "TelnetServer received error " << errno << ". Closing connection " << connection.first <<  "." );

							// Trouble. Close the connection.
							//
							::close( connection.first );
							iter = m_connections.erase( iter );
							continue;
						}
					}
					else
					{
						// Send the buffer data over.
						//
						bytes.resize( nBytes );
						onDataReceived( connection.first, bytes );
					}

					++iter;
				}
			}
		}

		void onDataReceived( int connectedSocket, const std::vector< unsigned char >& bytes )
		{
			// Which connection?
			//			
			const auto iter = m_connections.find( connectedSocket );
			ASSERT( iter != m_connections.end() );
			
			iter->second.onDataReceived( bytes );			
		}

	private:

		std::map< int, Connection > m_connections;

		int m_socket = -1;
		
		FRESH_DECLARE_CLASS( TelnetServer_bsd, TelnetServer );
		
	};
	
	FRESH_DEFINE_CLASS( TelnetServer_bsd );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( TelnetServer_bsd );
	
	
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( TelnetServer );
	
	TelnetServer::ptr TelnetServer::create( NameRef name )
	{
		return createObject< TelnetServer_bsd >( name );
	}	

}
