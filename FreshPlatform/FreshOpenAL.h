#ifndef FRESH_OPENAL_H_
#define FRESH_OPENAL_H_

#include "CommandProcessor.h"

#if !FRESH_NULL_AUDIO

#if _WIN32 || ANDROID || FRESH_EMSCRIPTEN || RASPBIAN
#	include <AL/al.h>
#	include <AL/alc.h>
#	undef NO_PRIORITY
#	undef MIN_PRIORITY
#elif __linux__
#	include <al.h>
#	include <alc.h>
#else
#	include <OpenAL/al.h>
#	include <OpenAL/alc.h>
#endif

namespace fr
{
#if defined( DEBUG )
	inline void handleALErrors( const char* sourceCodeFileName, int sourceCodeLine )
	{
		ALenum err = alGetError();
		
		if( err != AL_NO_ERROR )
		{
			const char* szErrString = "(Unknown.)";
			
			switch( err )
			{
				case AL_INVALID_NAME:
					szErrString = "Invalid Name parameter passed to AL call.";
					break;
					
				case AL_INVALID_ENUM:
					szErrString = "Invalid parameter passed to AL call.";
					break;
					
				case AL_INVALID_VALUE:
					szErrString = "Invalid enum parameter value.";
					break;
					
				case AL_INVALID_OPERATION:
					szErrString = "Illegal call.";
					break;
					
				case AL_OUT_OF_MEMORY:
					szErrString = "No mojo. (Out of memory.)";
					break;
					
				case AL_INVALID:
					szErrString = "invalid (deprecated).";
					
				default:
					break;
			}
			
			con_error( sourceCodeFileName << "(" << sourceCodeLine << "): OpenAL reported error " << std::hex << std::showbase << std::setw( 4 ) << std::setfill( '0' ) << err << ": '" << szErrString << "'" );
		}
	}
#	define HANDLE_AL_ERRORS() handleALErrors( __FILE__, __LINE__ )
#else
#	define HANDLE_AL_ERRORS()
#endif
}

#endif
#endif
