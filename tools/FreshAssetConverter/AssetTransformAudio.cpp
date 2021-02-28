//
//  AssetTransformAudio.cpp
//  fac
//
//  Created by Jeff Wofford on 7/23/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "AssetTransformAudio.h"
#include "FreshDebug.h"
using namespace fr;

namespace
{
	using namespace fac;
	void writeIfNotDefault( std::ostream& out, const AssetTransformAudio& audio, PropertyNameRef propName )
	{
		const auto& transformClass = AssetTransformAudio::StaticGetClassInfo();
		
		auto property = transformClass.getPropertyByName( propName );
		ASSERT( property );
		
		if( !property->isEqual( transformClass.defaultObject(), &audio ))
		{
			out << "\t\t<" << propName << ">" << property->getValueByString( &audio ) << "</" << propName << ">\n";
		}
	}
}

namespace fac
{
	FRESH_DEFINE_CLASS( AssetTransformAudio )
	DEFINE_DVAR( AssetTransformAudio, std::string, m_destFileFormat );
	DEFINE_DVAR( AssetTransformAudio, std::string, m_destDataFormat );
	DEFINE_DVAR( AssetTransformAudio, int, m_bitRate );
	DEFINE_DVAR( AssetTransformAudio, int, m_channels );
	DEFINE_DVAR( AssetTransformAudio, Range< float >, m_rangeGain );
	DEFINE_DVAR( AssetTransformAudio, Range< float >, m_rangePitch );
	DEFINE_DVAR( AssetTransformAudio, size_t, m_maxSimultaneousSounds );
	DEFINE_DVAR( AssetTransformAudio, bool, m_doLoop );
	DEFINE_DVAR( AssetTransformAudio, bool, m_newSoundsTrumpOldSounds );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( AssetTransformAudio )
	
	bool AssetTransformAudio::apply()
	{
		Super::apply();
		
#if TARGET_OS_MAC
		
		if( m_destFileFormat != "MPG3" )
		{
			std::ostringstream command;
			command << "afconvert";
		
			// Set the file format.
			command << " --file " << m_destFileFormat;
			
			// Set the data format.
			command << " --data '" << m_destDataFormat << "'";
			
			// Set the bit rate, if requested.
			if( m_bitRate > 0 )
			{
				command << " --bitrate " << ( m_bitRate * 1000 );
			}
			
			// Set the number of channels, if requested.
			if( m_channels > 0 )
			{
				command << " --channels " << m_channels;
			}
			
			// Indicate the source and dest file paths.
			command << " \"" << fullPathSrc() << "\" \"" << fullPathDest() << "\"";
			
			// Execute the command.
			//
			int result = ::system( command.str().c_str() );

			return result == 0;
		}
		else
		{
			// Converting to .mp3's is hard:
			
			// (1) Convert the source file to a .wav. We have to do this because `lame` only supports input wavs.
			//
			const path tempWAVPath = fullPathDest() + "_tmp.wav";
			
			{
				std::ostringstream command;
				command << "afconvert --file WAVE --data LEI16";
				
				// Set the number of channels, if requested.
				if( m_channels > 0 )
				{
					command << " --channels " << m_channels;
				}
				
				// Indicate the source and dest file paths.
				command << " \"" << fullPathSrc() << "\" \"" << tempWAVPath << "\"";
				
				// Execute the command.
				//
				int result = ::system( command.str().c_str() );
				
				if( result != 0 )
				{
					dev_error( "Failed to convert mp3: failed to produce intermediate .wav." );
					return false;
				}
			}
			
			// (2) Convert the intermediate .wav file to an .mp3 using the `lame` utility.
			//
			{
				std::ostringstream command;
				command << "/opt/local/bin/lame";
				
				if( m_bitRate > 0 )
				{
					command << " --silent --preset cbr " << m_bitRate;
				}
				
				command << " \"" << tempWAVPath << "\" \"" << fullPathDest() << "\"";
				
				// Execute the command.
				//
				int result = ::system( command.str().c_str() );
				
				if( result != 0 )
				{
					dev_error( "Failed to convert mp3: LAME command '" << command.str() << "' failed." );
					return false;
				}
			}
			
			// (3) Kill the temp file.
			//
			if( exists( tempWAVPath ))
			{
				std::remove( tempWAVPath.c_str() );
			}
			
			return true;
		}
		
#else
		return false;
#endif
	}
	
	void AssetTransformAudio::writeAssetToDatabaseEnd( std::ostream& assetDatabaseXML ) const
	{
		writeIfNotDefault( assetDatabaseXML, *this, "rangeGain" );
		writeIfNotDefault( assetDatabaseXML, *this, "rangePitch" );
		writeIfNotDefault( assetDatabaseXML, *this, "maxSimultaneousSounds" );
		writeIfNotDefault( assetDatabaseXML, *this, "doLoop" );
		writeIfNotDefault( assetDatabaseXML, *this, "newSoundsTrumpOldSounds" );
				
		Super::writeAssetToDatabaseEnd( assetDatabaseXML );
	}
}

