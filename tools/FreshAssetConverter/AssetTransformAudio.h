//
//  AssetTransformAudio.h
//  fac
//
//  Created by Jeff Wofford on 7/23/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef fac_AssetTransformAudio_h
#define fac_AssetTransformAudio_h

#include "AssetTransform.h"
#include "AudioSystem.h"

namespace fac
{
	
	using fr::Range;
	
	class AssetTransformAudio : public AssetTransform
	{
		FRESH_DECLARE_CLASS( AssetTransformAudio, AssetTransform );
	public:
		
		virtual bool apply() override;
		
		virtual std::string assetClassName() const override { return "AudioCue"; }
		
	protected:
		
		virtual void writeAssetToDatabaseEnd( std::ostream& assetDatabaseXML ) const override;
		
	private:
		
		DVAR( AssetTransformAudio, std::string, m_destFileFormat, "caff" );
		DVAR( AssetTransformAudio, std::string, m_destDataFormat, "aac" );
		DVAR( AssetTransformAudio, int, m_bitRate, 0 );			// 0 means "keep as source file"
		DVAR( AssetTransformAudio, int, m_channels, 0 );		// 0 means "keep as source file"
		DVAR( AssetTransformAudio, Range< float >, m_rangeGain, Range< float >( 1.0f, 1.0f ) );
		DVAR( AssetTransformAudio, Range< float >, m_rangePitch, Range< float >( 1.0f, 1.0f ) );
		DVAR( AssetTransformAudio, size_t, m_maxSimultaneousSounds, 0 );	// Zero means unlimited (or rather, limited brutally by OpenAL).
		DVAR( AssetTransformAudio, bool, m_doLoop, false );
		DVAR( AssetTransformAudio, bool, m_newSoundsTrumpOldSounds, true );
		
	};
	
}

#endif
