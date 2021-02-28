//
//  TelnetServer.h
//  Fresh
//
//  Created by Jeff Wofford on 2/8/12.
//  Copyright (c) 2012 jeffwofford.com. All rights reserved.
//

#ifndef Fresh_TelnetServer_h
#define Fresh_TelnetServer_h

#include "Objects.h"
#include "Classes.h"
#include <vector>
#include <iterator>

namespace fr
{
	
	class TelnetServer : public Object
	{
	public:
		
		static SmartPtr< TelnetServer > create( NameRef name = DEFAULT_OBJECT_NAME );
		
		typedef std::function< void ( const std::string& ) > Callback;
		
		virtual void beginListening( const Callback& callback, int port = 47125, const std::string& welcomeMessage = "Connected.\n" )
		{
			m_callback = callback;
			m_port = port;
			m_welcomeMessage = welcomeMessage;
		}
		
		virtual bool isListening() const = 0;
		
		virtual bool isConnected() const = 0;
		
		virtual void broadcast( const std::string& text ) = 0;
		
		virtual void updateConnections() = 0;
		
		virtual int portNumber() const											  { return m_port; }
		
		SYNTHESIZE( std::string, welcomeMessage )
		
	protected:
		
		std::string m_welcomeMessage;

		struct Connection
		{
			Callback m_callback;
			std::string m_buffer;
			bool m_seekingWholeEnd = false;
			
			void onDataReceived( const std::vector< unsigned char >& bytes )
			{
				ASSERT( m_callback );
				
				const char WHOLE_TAG[] = "<<WHOLE>>";
				static size_t LEN_WHOLE_TAG = strlen( WHOLE_TAG );
				const char END_WHOLE_TAG[] = "<<END-WHOLE>>";
				static size_t LEN_END_WHOLE_TAG = strlen( END_WHOLE_TAG );
				
				std::string newBytes;
				
				std::copy( bytes.begin(), bytes.end(), std::back_inserter( newBytes ));
				
				// Look for a "WHOLE" tag that might indicate that more data is to come.
				//
				auto posTag = newBytes.find( WHOLE_TAG );
				if( posTag != std::string::npos )
				{
					// Strip the whole tag from the input.
					//
					newBytes.erase( posTag, LEN_WHOLE_TAG );
					
					if( m_seekingWholeEnd )
					{
						// Woops. Found a new whole tag while looking for the end of another.
						// Must have missed an END-WHOLE somewhere.
						// Pretend we saw one.
						// This could very well lead to erroneous results.
						//
						m_callback( m_buffer );
						m_buffer.clear();
					}
					
					m_seekingWholeEnd = true;
				}
				
				m_buffer.append( newBytes );
				
				// Look for END-WHOLE if one is expected.
				// We search the buffer rather than the incoming bytes because the incoming bytes
				// may very well be truncated, obscuring the tag.
				//
				if( m_seekingWholeEnd )
				{
					posTag = m_buffer.find( END_WHOLE_TAG );
					if( posTag != std::string::npos )
					{
						// Strip end tag.
						//
						m_buffer.erase( posTag, LEN_END_WHOLE_TAG );
						
						m_seekingWholeEnd = false;
					}
				}
				
				if( !m_seekingWholeEnd && m_callback )
				{
					m_callback( m_buffer );
					m_buffer.clear();
				}
			}
		};
		
		Callback& getCallbackOnDataReceived() { return m_callback; }
		

	private:
		
		Callback m_callback;
		int m_port = 0;
		
		FRESH_DECLARE_CLASS_ABSTRACT( TelnetServer, Object )
		
	};
	
}

#endif
