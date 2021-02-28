//
//  IndentingStream.h
//  fr_test
//
//  Created by Jeff Wofford on 4/12/15.
//  Copyright (c) 2015 The Electric Toy Company. All rights reserved.
//

#ifndef __fr_test__IndentingStream__
#define __fr_test__IndentingStream__

#include <ostream>
#include <cassert>

namespace fr
{
	class IndentingOStreambuf : public std::streambuf
	{
	public:

		explicit IndentingOStreambuf( std::ostream& dest, const std::string& indentText = "\t" )
		:	m_streamBuf( dest.rdbuf() )
		,	m_indentText( indentText )
		,	m_ownerStream( dest )
		{
			m_ownerStream.rdbuf( this );
		}
		
		virtual ~IndentingOStreambuf()
		{
			m_ownerStream.rdbuf( m_streamBuf );
		}
		
		void indent() { ++m_indent; }
		void unindent() { assert( m_indent > 0 ); --m_indent; }
		
		void setIndent( int indent ) { assert( indent >= 0 ); m_indent = indent; }
		int indents() const { return m_indent; }
		
	protected:
		
		virtual int overflow( int ch )
		{
			if( m_atStartOfLine && ch != '\n' )
			{
				for( int i = 0; i < m_indent; ++i )
				{
					m_streamBuf->sputn( m_indentText.data(), m_indentText.size() );
				}
			}
			m_atStartOfLine = ch == '\n';
			
			return m_streamBuf->sputc( ch );
		}

	private:
		
		std::streambuf*     m_streamBuf;
		std::string         m_indentText;
		std::ostream&       m_ownerStream;
		bool                m_atStartOfLine = true;
		int					m_indent = 0;
	};
}

#endif /* defined(__fr_test__IndentingStream__) */
