//
//  TelnetServer_Win.cpp
//  Fresh
//
//  Created by Jeff Wofford on 10/26/12.
//  Copyright (c) 2012 jeffwofford.com. All rights reserved.
//

#include "../../TelnetServer.h"
#include "../../Objects.h"
#include <winsock.h>
#include <map>

namespace
{
	DWORD WINAPI WinsockListenThread( LPVOID param );
}

namespace fr
{
	
	class TelnetServer_Win : public TelnetServer
	{
	public:
		
		virtual void beginListening( const Callback& callback, int port, const std::string& welcomeMessage = "Connected.\n" ) override
		{
			ASSERT( !getCallbackOnDataReceived() );
			ASSERT( callback );
			TelnetServer::beginListening( callback, port, welcomeMessage );

			WSADATA wsaData;
			VERIFY( ::WSAStartup( 0x101, &wsaData ) == 0 );

			sockaddr_in local;
			local.sin_family = AF_INET;
			local.sin_addr.s_addr = INADDR_ANY;
			local.sin_port = ::htons( static_cast< u_short >( port ));

			m_server = ::socket( AF_INET, SOCK_STREAM, 0 );
			ASSERT( m_server != INVALID_SOCKET );

			VERIFY( ::bind( m_server, reinterpret_cast< sockaddr* >( &local ), sizeof( local )) == 0 );

			VERIFY( ::listen( m_server, 10 /* TODO size of backlog */ ) == 0 );

			m_hThreadAcceptConnections = ::CreateThread( 0 , 0, WinsockListenThread, this, 0, 0 );
		}

		virtual bool isListening() const override
		{
			return m_server != INVALID_SOCKET;
		}		
		virtual bool isConnected() const override
		{
			return m_connections.empty();
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
		
		virtual ~TelnetServer_Win()
		{
			// Kill the listening thread.
			//
			if( m_hThreadAcceptConnections )
			{
				::TerminateThread( m_hThreadAcceptConnections, 0 );
				m_hThreadAcceptConnections = NULL;
			}

			for( const auto& pair : m_connections )
			{
				::closesocket( pair.first );
			}
			m_connections.clear();
			
			if( m_server != INVALID_SOCKET )
			{
				::closesocket( m_server );
				m_server = INVALID_SOCKET;
			}

			::WSACleanup();
		}		

		void acceptConnection( SOCKET client )
		{
			ASSERT( client != INVALID_SOCKET );

			Connection connection;
			connection.m_callback = getCallbackOnDataReceived();

			m_connections[ client ] = connection;

			// Send welcome message.
			//
			const auto& welcome = welcomeMessage();
			
			if( !welcome.empty() )
			{
				::send( client, welcome.data(), welcome.size(), 0 /* flags */ );
			}
		}

		void threadedListen()	// Called from secondary thread.
		{
			sockaddr_in from;
			int fromlen = sizeof( from );

			while( true )
			{
				// Synchronous. Stalls until a client connects.
				SOCKET client = ::accept( m_server, reinterpret_cast< struct sockaddr* >( &from ), &fromlen );

				dev_trace( "TelnetServer received socket connection from " << inet_ntoa( from.sin_addr ) );

				// Set client to non-blocking
				unsigned long nonBlocking = 1;
				VERIFY( ::ioctlsocket( client, FIONBIO, &nonBlocking ) == 0 );

				acceptConnection( client );

				::Sleep( 1000 );		// Don't kill the CPU spinlocking here.
			}
		}

		virtual void updateConnections() override	// Called from main thread
		{
			const size_t BUFFER_SIZE = 2048;
			std::vector< unsigned char > readBuffer;

			for( auto& pair : m_connections )
			{
				readBuffer.clear();
				readBuffer.resize( BUFFER_SIZE );
				const int nBytesReceived = ::recv( pair.first, reinterpret_cast< char* >( readBuffer.data()), readBuffer.size() * sizeof( readBuffer[ 0 ] ), 0 );

				if( nBytesReceived > 0 )
				{
					pair.second.onDataReceived( readBuffer );
				}
			}
		}

	private:
		
		HANDLE m_hThreadAcceptConnections = NULL;
		SOCKET m_server = INVALID_SOCKET;

		std::map< SOCKET, Connection > m_connections;

		FRESH_DECLARE_CLASS( TelnetServer_Win, TelnetServer );		
	};
	
	FRESH_DEFINE_CLASS( TelnetServer_Win );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( TelnetServer_Win );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( TelnetServer );
	
	TelnetServer::ptr TelnetServer::create( NameRef name )
	{
		return createObject< TelnetServer_Win >( name );
	}	

}

namespace
{
	DWORD WINAPI WinsockListenThread( LPVOID param )
	{
		fr::TelnetServer_Win* server = reinterpret_cast< fr::TelnetServer_Win* >( param );
		server->threadedListen();
		return 0;
	}
}
