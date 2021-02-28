//
//  ApiAudio.cpp
//  Fresh
//
//  Created by Jeff Wofford on 6/26/18.
//  Copyright (c) 2018 Jeff Wofford. All rights reserved.
//

#include "FantasyConsole.h"
#include "FreshVector.h"
#include "AudioCue.h"
#include "AudioSound.h"

namespace fr
{		
	LUA_FUNCTION( sfx, 1 )
	void FantasyConsole::sfx( std::string name, real gain )
	{
		SANITIZE( gain, 1.0f, 0.0f, 1.0f );
		
		const auto iter = m_audioCues.find( name );
		if( iter == m_audioCues.end() )
		{
			console_trace( "No sfx named '" << name << "'." );
			return;
		}
		
		const auto sound = iter->second->createSound();
		sound->gain( gain );
		sound->play();
	}	
	
	LUA_FUNCTION( music, 0 )
	void FantasyConsole::music( std::string name, real gain )
	{
		DEFAULT( name, "" );
		SANITIZE( gain, 1.0f, 0.0f, 1.0f );
		
		// Stop any pre-existing sound.
		//
		if( m_musicSound )
		{
			m_musicSound->stop();
		}
		m_musicSound = nullptr;
		
		if( name.empty() )
		{
			// Stopping music only.
			return;
		}
		
		const auto iter = m_audioCues.find( name );
		if( iter == m_audioCues.end() )
		{
			console_trace( "No music named '" << name << "'." );
			return;
		}
		
		m_musicSound = iter->second->createSound();
		m_musicSound->gain( gain );
		m_musicSound->looping( true );
		m_musicSound->play();		
	}
}

