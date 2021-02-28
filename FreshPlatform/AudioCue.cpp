/*
 *  AudioCue.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 8/12/09.
 *  Copyright 2009 jeffwofford.com. All rights reserved.
 *
 */

#include "AudioCue.h"
#include "AudioSystem.h"
#include "Objects.h"
#include "FreshFile.h"
#include "FreshException.h"
#include "FreshOpenAL.h"
#include "CommandProcessor.h"
#include "FreshThread.h"
#include "Property.h"
#include "AudioSystemImpl.h"

#if TARGET_OS_MAC
#	define FRESH_DECODING_APPLE 1
#	include <AudioToolbox/ExtendedAudioFile.h>
#	include <AudioToolbox/AudioServices.h>
#	include <TargetConditionals.h>
#elif defined( _MSC_VER )
#	include "mpg123_win.h"
#	include <AL/alut.h>
#else
#	include <mpg123.h>
#	include <AL/alut.h>
#endif

#if TARGET_OS_MAC && !TARGET_IPHONE_SIMULATOR
#	define AUDIO_SUPPORTING_STATIC_BUFFERDATA 1
#endif

#if AUDIO_SUPPORTING_STATIC_BUFFERDATA
#	define FRESH_RETAIN_RAW_AUDIO_DATA 1
#endif

#if DEV_MODE && 0
#	define audio_trace(x) release_trace(x)
#else
#	define audio_trace(x)
#endif

namespace
{
	using namespace fr;
	
	// Delayed vector deletion to avoid OpenAL problems where a deleted buffer is still in use in the OpenAL thread.
	// See https://github.com/cocos2d/cocos2d-x/pull/17500
	//
	template< typename ContainerT >
	class DelayedDeletionContainer
	{
	public:
		using container_type = ContainerT;
		using value_type = typename ContainerT::value_type;

		explicit DelayedDeletionContainer()
		{}
		
		~DelayedDeletionContainer()
		{
			if( m_containerId > 0 )
			{
				const auto id = m_containerId;
				DelayedDeletionContainer::later( [=](){ s_containers.erase( id ); } );
			}
		}
		
		const container_type& container() const
		{
			ASSERT( m_containerId > 0 );
			return s_containers[ m_containerId ];
		}
		
		value_type* data() const
		{
			ASSERT( m_containerId > 0 );
			return s_containers[ m_containerId ].data();
		}

		bool empty() const
		{
			if( m_containerId > 0 )
			{
				return s_containers[ m_containerId ].empty();
			}
			else
			{
				return true;
			}
		}
		
		void clear()
		{
			const auto id = m_containerId;
			DelayedDeletionContainer::later( [=]()
			{
				DelayedDeletionContainer::onContainer( id, []( container_type& container )
				{
					container.clear();
				} );
			} );
		}

		void operator=( container_type&& container )
		{
			if( m_containerId != 0 )
			{
				const auto id = m_containerId;
				DelayedDeletionContainer::later( [=](){ s_containers.erase( id ); } );
			}
			
			m_containerId = s_nextUnusedContainerId++;
			
			ASSERT( m_containerId > 0 );
			s_containers[ m_containerId ] = std::move( container );
		}

	private:
		using ContainerId = size_t;
		ContainerId m_containerId = 0;
		
		static ContainerId s_nextUnusedContainerId;
		static std::unordered_map< ContainerId, container_type > s_containers;
		static dispatch::Duration s_deletionDelay;
		
		template< typename FnT >
		static void onContainer( ContainerId which, FnT&& fn )
		{
			auto iter = s_containers.find( which );
			if( iter != s_containers.end() )
			{
				fn( iter->second );
			}
		}

		template< typename FnT >
		static void later( FnT&& fn )
		{
			dispatch::mainQueue().asyncAfter( dispatch::Clock::now() + s_deletionDelay,
											 DISPATCH_BLOCK(
															{
																fn();
															}));
		}
	};
	
	template< typename ContainerT > typename DelayedDeletionContainer< ContainerT >::ContainerId DelayedDeletionContainer< ContainerT >::s_nextUnusedContainerId = 1;
	template< typename ContainerT > std::unordered_map< typename DelayedDeletionContainer< ContainerT >::ContainerId, typename DelayedDeletionContainer< ContainerT >::container_type > DelayedDeletionContainer< ContainerT >::s_containers;
	template< typename ContainerT > dispatch::Duration DelayedDeletionContainer< ContainerT >::s_deletionDelay = std::chrono::milliseconds( 500 );



	//==================================================================================================
	//	Helper functions
	//==================================================================================================

#if !FRESH_NULL_AUDIO
	
	ALenum getALFormat( fr::AudioCue::Format format )
	{
		switch( format )
		{
			case fr::AudioCue::F_Mono8:
				return AL_FORMAT_MONO8;
				
			case fr::AudioCue::F_Mono16:
				return AL_FORMAT_MONO16;
				
			case fr::AudioCue::F_Stereo8:
				return AL_FORMAT_STEREO8;
				
			case fr::AudioCue::F_Stereo16:
				return AL_FORMAT_STEREO16;
				
			default:
				ASSERT( false );
				return 0;
		}
	}
	
#	if FRESH_DECODING_APPLE

	size_t readAudioByteCount( ExtAudioFileRef refAudioFile, void* const buffer, size_t maxBytes, UInt32* nFrames, int outputChannels )
	{
		ASSERT( nFrames );
		
		AudioBufferList audioBufferList;
		audioBufferList.mNumberBuffers = 1;
		audioBufferList.mBuffers[ 0 ].mDataByteSize = static_cast< unsigned int >( maxBytes );
		audioBufferList.mBuffers[ 0 ].mNumberChannels = outputChannels;
		audioBufferList.mBuffers[ 0 ].mData = buffer;
		if( !audioBufferList.mBuffers[ 0 ].mData )
		{
			return 0;
		}
		
		
		// Read the data into an AudioBufferList. The list's first buffer's mData parameter will have the uncompressed data.
		//
		OSStatus result = ExtAudioFileRead( refAudioFile, nFrames, &audioBufferList );
		if( result != noErr )
		{
			return 0;
		}
		
		// ExtAudioFileRead() may read fewer than the initially-reported number of frames and thus end up with a shorter file.
		// Adjust accordingly.
		//
		return audioBufferList.mBuffers[ 0 ].mDataByteSize;
	}

#	else	// FRESH_DECODING_APPLE
	
	fr::AudioCue::Format fromALFormat( ALenum format )
	{
		switch( format )
		{
			case AL_FORMAT_MONO8:
				return fr::AudioCue::F_Mono8;
				
			case AL_FORMAT_MONO16:
				return fr::AudioCue::F_Mono16;
				
			case AL_FORMAT_STEREO8:
				return fr::AudioCue::F_Stereo8;
				
			default:
				ASSERT( false );
			case AL_FORMAT_STEREO16:
				return fr::AudioCue::F_Stereo16;
		}
	}

#	endif	// FRESH_DECODING_APPLE


#	if AUDIO_SUPPORTING_STATIC_BUFFERDATA
	ALvoid FreshLoadBuffer( const ALint idBuffer, ALenum format, const ALvoid* data, ALsizei size, ALsizei freq )
	{
		typedef ALvoid AL_APIENTRY (*alBufferDataStaticProcPtr)( const ALint bid, ALenum format, const ALvoid* data, ALsizei size, ALsizei freq );
		static alBufferDataStaticProcPtr fn = reinterpret_cast< alBufferDataStaticProcPtr >( alcGetProcAddress( nullptr, "alBufferDataStatic" ));
		ASSERT( fn );
		
		ASSERT( data );
		ASSERT( size > 0 );
		
		fn( idBuffer, format, data, size, freq );
	}
#	else	// AUDIO_SUPPORTING_STATIC_BUFFERDATA
	ALvoid FreshLoadBuffer( const ALint idBuffer, ALenum format, const ALvoid* data, ALsizei size, ALsizei freq )
	{
		ASSERT( data );
		ASSERT( size > 0 );

		alBufferData( idBuffer, format, data, size, freq );
	}
#	endif	// AUDIO_SUPPORTING_STATIC_BUFFERDATA
	
#endif		// !FRESH_NULL_AUDIO
	
	inline bool isUnusedSound( const Sound::ptr sound )
	{
		return !sound ||
		( sound->getReferenceCount() <= 2 &&			// Two because one for this local sound variable and one for the m_sounds container. If we're the only things referencing this sound, it's got a pretty sad life.
		 !sound->isPlaying()
		 );
	}
	
	using CompletionHandler = std::function< void( std::vector< unsigned char >&& fileBytes, 
												  size_t nBytes, 
												  size_t nBytesLoaded, 
												  AudioCue::Format format, 
												  double sampleRate ) >;
	void asyncLoad( path filePath, CompletionHandler&& completionHandler, AudioCue::ErrorHandler&& errorHandler );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
namespace fr
{
	FRESH_DEFINE_CLASS( AudioCueLoader )
	
	DEFINE_VAR( AudioCueLoader, Range< float >, m_rangeGain );
	DEFINE_VAR( AudioCueLoader, Range< float >, m_rangePitch );
	DEFINE_VAR( AudioCueLoader, size_t, m_maxSimultaneousSounds );
	DEFINE_VAR( AudioCueLoader, bool, m_doLoop );
	DEFINE_VAR( AudioCueLoader, bool, m_newSoundsTrumpOldSounds );
	DEFINE_VAR( AudioCueLoader, bool, m_allowAsyncLoad );
	
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( AudioCueLoader )

	AudioCueLoader::AudioCueLoader( const fr::ClassInfo& assignedClassInfo, fr::Object::NameRef objectName )
	:	Super( assignedClassInfo, objectName )
	{
		m_assetClass = getClass( "AudioCue" );
		
		doctorClass< AudioCueLoader >( [&]( ClassInfo& classInfo, AudioCueLoader& defaultObject )
							   {
								   DOCTOR_PROPERTY_ASSIGN( assetClass )
							   } );
	}
	
	///////////////////////////////////////////////////////////////////////////
	
	using AudioBuffer = DelayedDeletionContainer< std::vector< unsigned char >>;
	
	class AudioCueImpl : public std::enable_shared_from_this< AudioCueImpl >
	{
	public:
		explicit AudioCueImpl( AudioCue::wptr host ) : m_host( host ) {}
		~AudioCueImpl();
				
		void load( const path& filePath, AudioCue::ErrorHandler&& errorHandler );
		void abortLoad();
		
		size_t getMemorySize() const;
		
		void enqueueBuffersTo( Sound& sound ) const;
		
	protected:
		
		void generateBufferAndNotifySounds( const unsigned char* audio );
		void destroyBuffers();
		
	private:
		
		AudioCue::wptr m_host;
		
		size_t m_nBytes = 0;
		size_t m_nBytesLoaded = 0;
		std::vector< uint > m_bufferIDs;

		path m_filePath;
		AudioBuffer m_fileData;
	};

	///////////////////////////////////////////////////////////////////////////
	
	FRESH_DEFINE_CLASS( AudioCue )
	
	DEFINE_VAR( AudioCue, Range< float >, m_rangeGain );
	DEFINE_VAR( AudioCue, Range< float >, m_rangePitch );
	DEFINE_VAR( AudioCue, size_t, m_maxSimultaneousSounds );
	DEFINE_VAR( AudioCue, bool, m_doLoop );
	DEFINE_VAR( AudioCue, bool, m_newSoundsTrumpOldSounds );
	DEFINE_VAR( AudioCue, bool, m_allowAsyncLoad );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( AudioCue )
	
	FRESH_CUSTOM_STANDARD_CONSTRUCTOR_NAMING( AudioCue )
	,	m_impl( std::make_shared< AudioCueImpl >( this ))
	{}
	
	AudioCue::~AudioCue()
	{
		FRESH_AUDIO_SYNCHRONIZE
		audio_trace( "Destroying " << this );
		
		if( m_impl )		// Otherwise inert.
		{
			// Stop all dependant sounds.
			//
			for( auto sound : m_sounds )
			{
				if( sound )
				{
					sound->stop();
				}
			}
			
			m_impl->abortLoad();
		}
	}
	
	double AudioCue::getLengthSeconds() const
	{
		FRESH_AUDIO_SYNCHRONIZE
		ASSERT( m_sampleRate > 0 );
		
		return getLengthSamples() / m_sampleRate;
	}
	
	size_t AudioCue::getBytesPerSample() const
	{
		FRESH_AUDIO_SYNCHRONIZE
		switch( m_format )
		{
			case AudioCue::F_Mono8:
				return 1;
				
			case AudioCue::F_Mono16:
				return 2;
				
			case AudioCue::F_Stereo8:
				return 2;
				
			case AudioCue::F_Stereo16:
				return 4;
				
			default:
				ASSERT( false );
				return 0;
		}
	}
	
	size_t AudioCue::getLengthSamples() const
	{
		FRESH_AUDIO_SYNCHRONIZE
		size_t bytesPerSample = getBytesPerSample();
		ASSERT( bytesPerSample > 0 );
		
		return m_impl->getMemorySize() / bytesPerSample;
	}
	
	size_t AudioCue::secondsToBytes( double seconds ) const
	{
		FRESH_AUDIO_SYNCHRONIZE
		size_t bytes = static_cast< size_t >( seconds * m_sampleRate * getBytesPerSample());
		
		// Truncate to start of nearest sample.
		//
		bytes -= bytes % getBytesPerSample();
		
		return bytes;
	}
	
	Sound::ptr AudioCue::createSound()
	{
		FRESH_AUDIO_SYNCHRONIZE
		if( !AudioSystem::ready() ) return nullptr;

		purgeUnusedSounds();
		
		Sound::ptr sound;
		
		// Do we have space for a new sound?
		//
		if( m_maxSimultaneousSounds == 0 || m_sounds.size() < m_maxSimultaneousSounds )
		{
			// Yes we do. Create one.
			//
			sound = createObject< Sound >( getTransientPackage() );
			
			if( !sound )
			{
				return nullptr;
			}

			m_sounds.push_back( sound );
			sound->cue( this );
			
			enqueueBuffersTo( *sound );
		}
		else
		{
			// No we don't. Ransack an existing sound for this.
			//
			if( m_newSoundsTrumpOldSounds )
			{
				// Reuse the oldest sound by moving it to the back of the list.
				//
				m_sounds.push_back( m_sounds.front() );
				m_sounds.pop_front();
			}
			sound = m_sounds.back();
			
			// Remove the sound from its parent group, if any.
			//
			if( SoundGroup::wptr parent = sound->parentGroup() )
			{
				parent->removeChild( sound );
			}
			
			ASSERT( sound->cue() == this );
			sound->stop();
		}
		
		auto gain = randInRange( m_rangeGain );
		auto pitch = randInRange( m_rangePitch );
		
		audio_trace( this << " has " << m_sounds.size() << " sounds." );
		audio_trace( this << " created sound " << sound << " with gain " << gain << " and pitch " << pitch );
		
		sound->gain( gain );
		sound->pitchScalar( pitch );
		sound->looping( m_doLoop );
		
		return sound;
	}	

	void AudioCue::purgeUnusedSounds()
	{
		FRESH_AUDIO_SYNCHRONIZE
		m_sounds.remove_if( &isUnusedSound );
	}
	
	size_t AudioCue::reportDevStatistics() const
	{
		FRESH_AUDIO_SYNCHRONIZE
		size_t nActiveSounds = 0;
		for( auto sound : m_sounds )
		{
			if( !sound )
			{
				trace( "\tnull sound" );
			}
			else
			{
				++nActiveSounds;
				trace( "\t" << sound << " refs: " << ( sound->getReferenceCount() - 1 ) << " using cue " << sound->cue() << ( sound->isPlaying() ? " playing" : " stopped" ));	// -1 on ref count because sound itself is referencing it.
			}
		}
		
		return nActiveSounds;
	}
	
	void AudioCue::enqueueBuffersTo( Sound& sound ) const
	{
		FRESH_AUDIO_SYNCHRONIZE
		m_impl->enqueueBuffersTo( sound );
	}

	void AudioCue::loadFile( const path& filePath, ErrorHandler&& errorHandler )
	{
		FRESH_AUDIO_SYNCHRONIZE
		ASSERT( dispatch::onMainQueue() );

		REQUIRES( !filePath.empty() );

		if( !AudioSystem::ready() ) return;
		
		// Load the data.
		//
		m_impl->load( filePath, std::move( errorHandler ));
	}
	
	size_t AudioCue::getMemorySize() const
	{
		FRESH_AUDIO_SYNCHRONIZE
		return m_impl->getMemorySize();
	}
	
	//////////////////////////////////////////////////////////////////////////////
	// AudioCueImpl
	
	void AudioCueImpl::enqueueBuffersTo( Sound& sound ) const
	{
		FRESH_AUDIO_SYNCHRONIZE
		ASSERT( dispatch::onMainQueue() );

#if !FRESH_NULL_AUDIO
		if( !m_bufferIDs.empty() )
		{
			sound.enqueueBuffers( m_bufferIDs.data(), m_bufferIDs.size() );
		}
#endif
	}

	size_t AudioCueImpl::getMemorySize() const
	{
		FRESH_AUDIO_SYNCHRONIZE
		return m_nBytes;
	}
	
	AudioCueImpl::~AudioCueImpl()
	{
		FRESH_AUDIO_SYNCHRONIZE
		ASSERT( dispatch::onMainQueue() );
		destroyBuffers();
	}
	

	void AudioCueImpl::generateBufferAndNotifySounds( const unsigned char* audio )
	{
		FRESH_AUDIO_SYNCHRONIZE
		ASSERT( dispatch::onMainQueue() );

		ASSERT( audio );
		
		ASSERT( m_bufferIDs.empty() );
		m_bufferIDs.resize( 1 );
		alGenBuffers( static_cast< ALsizei >( m_bufferIDs.size() ), m_bufferIDs.data() );
		ASSERT( m_bufferIDs.front() );
		
		FreshLoadBuffer( m_bufferIDs.front(),
						getALFormat( m_host->m_format ),
						static_cast< const ALvoid* >( audio ),
						static_cast< ALsizei >( m_nBytesLoaded ),
						static_cast< ALsizei >( m_host->m_sampleRate ) );
		
		HANDLE_AL_ERRORS();
		
		for( const auto& sound : m_host->m_sounds )
		{
			if( sound )
			{
				sound->enqueueBuffer( m_bufferIDs.front() );
			}
		}
	}

	void AudioCueImpl::load( const path& filePath, AudioCue::ErrorHandler&& errorHandler )
	{
		FRESH_AUDIO_SYNCHRONIZE
		ASSERT( dispatch::onMainQueue() );

		m_filePath = filePath;
		
		std::weak_ptr< AudioCueImpl > weakThis = shared_from_this();
		
		auto loadCompletionCallback = [weakThis]( std::vector< unsigned char >&& fileBytes, size_t nBytes, size_t nBytesLoaded, AudioCue::Format format, double sampleRate )
		{
			// Verify that this and the host cue still exist.
			//
			if( const auto strongThis = weakThis.lock() )
			{
				if( const auto host = strongThis->m_host.lock() )
				{			
					strongThis->m_nBytes = nBytes;
					strongThis->m_nBytesLoaded = nBytesLoaded;
					host->m_format = format;
					host->m_sampleRate = sampleRate;
					
					ASSERT( strongThis->m_fileData.empty() );
					strongThis->m_fileData = std::move( fileBytes );
					
					strongThis->generateBufferAndNotifySounds( strongThis->m_fileData.data() );

#if !FRESH_RETAIN_RAW_AUDIO_DATA
					strongThis->m_fileData.clear();
#endif					
				}
			}
		};
		
		const auto path = m_filePath;
		
#if FRESH_ALLOW_THREADING
		if( m_host->m_allowAsyncLoad )
		{
			dispatch::globalQueue().async( std::make_shared< fr::dispatch::Block >( [=, errorHandler = std::move( errorHandler ) ]() mutable
			{
				asyncLoad( path, std::move( loadCompletionCallback ), std::move( errorHandler ));
			} ));
		}
		else
#endif
		{
			asyncLoad( path, std::move( loadCompletionCallback ), std::move( errorHandler ));
		}
	}
	
	void AudioCueImpl::abortLoad()
	{
		FRESH_AUDIO_SYNCHRONIZE
		ASSERT( dispatch::onMainQueue() );

		// Invalidate all data.
		//
		destroyBuffers();
		m_fileData.clear();
		m_filePath.clear();
		m_nBytes = m_nBytesLoaded = 0;
	}
	
	void AudioCueImpl::destroyBuffers()
	{
		FRESH_AUDIO_SYNCHRONIZE
		ASSERT( dispatch::onMainQueue() );
			   
#if !FRESH_NULL_AUDIO
		if( !m_bufferIDs.empty() )
		{
			alDeleteBuffers( (ALsizei) m_bufferIDs.size(), m_bufferIDs.data() );
			m_bufferIDs.clear();
		}
#endif
	}
}

namespace
{
#	define	RETURN_ERROR( msg ) if( errorHandler ) \
{ 	dispatch::mainQueue().async( std::make_shared< fr::dispatch::Block >( [=, errorHandler = std::move( errorHandler ), message = createString( msg )]()	\
	{  errorHandler( message );	\
	}, DISPATCH_CODE_LOCATION ));	\
} return;
	

#if !FRESH_DECODING_APPLE
	
	// Non-Apple loading.
	
	void asyncLoad( path filePath, CompletionHandler&& completionHandler, AudioCue::ErrorHandler&& errorHandler )
	{
		ASSERT( !filePath.empty() );
		
		ALenum format;
		ALsizei nBytes;
		ALfloat sampleRate;
		std::vector< unsigned char > fileData;
		
		// Is this an mp3 or something else?
		//
		auto fileExtension = filePath.extension();
		if( toLower( fileExtension ) == ".mp3" )
		{
			// Use libMpg123 to load mp3 file.
			
			int err = mpg123_init();
			if( err != MPG123_OK )
			{
				FRESH_THROW( FreshException, "Could not init libMpg123: " << mpg123_plain_strerror( err ));
			}
			
			mpg123_handle* mpgLoader = mpg123_new( NULL, &err );
			if( !mpgLoader )
			{
				FRESH_THROW( FreshException, "Could not create libMpg123 handle: " << mpg123_plain_strerror( err ));
			}
			
			// Open the file.
			err = mpg123_open( mpgLoader, filePath.c_str() );
			if( err != MPG123_OK )
			{
				FRESH_THROW( FreshException, "Could not create open mp3 file: " << mpg123_plain_strerror( err ));
			}
			
			ASSERT( mpgLoader );
			
			// Peek into track and get first output format.
			//
			int encoding = 0;
			int nChannels;
			long longSampleRate;
			err = mpg123_getformat( mpgLoader, &longSampleRate, &nChannels, &encoding );
			
			if( err != MPG123_OK )
			{
				FRESH_THROW( FreshException, "Could not get mp3 file encoding: " << mpg123_strerror( mpgLoader ));
			}
			
			ASSERT( encoding == MPG123_ENC_SIGNED_16 );
			
			// Set the format.
			//
			mpg123_format_none( mpgLoader );
			mpg123_format( mpgLoader, longSampleRate, nChannels, encoding );
			
			// How many bytes should we expect during each pass?
			//
			const size_t bufferSize = mpg123_outblock( mpgLoader );	// Not the size of the total output, but just of the rolling buffer.
			
			nBytes = 0;
			do
			{
				// Make space for the incoming bytes.
				//
				fileData.resize( nBytes + bufferSize );
				
				size_t bytesRead;
				err = mpg123_read( mpgLoader, fileData.data() + nBytes, bufferSize, &bytesRead );
				
				ASSERT( bytesRead <= bufferSize );
				
				nBytes += bytesRead;
				
			} while( err == MPG123_OK );
			
			if( err != MPG123_DONE )
			{
				release_error( "Decoding ended prematurely because: " << ( err == MPG123_ERR ? mpg123_strerror( mpgLoader ) : mpg123_plain_strerror( err )) );
			}
			
			mpg123_close( mpgLoader );
			mpg123_delete( mpgLoader );
			mpg123_exit();
			
			fileData.resize( nBytes );
			
			sampleRate = static_cast< ALfloat >( longSampleRate );
			
			ASSERT( nChannels == 1 || nChannels == 2 );
			format = nChannels > 1 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
		}
		else
		{
			// Use ALUT for wavs and whatever else it supports.
			//
			
			// Load the data.
			//
			const auto rawData = reinterpret_cast< unsigned char* >( alutLoadMemoryFromFile( filePath.c_str(), &format, &nBytes, &sampleRate ));
			
			// Check for errors.
			//
			const ALenum err = alutGetError();
			if( !rawData || err != ALUT_ERROR_NO_ERROR )
			{
				FRESH_THROW( FreshException, "ALUT failed to load audio from '" << filePath << "' with error: " << alutGetErrorString( err ) );
			}
			
			// Copy the memory.
			//
			fileData.assign( rawData, rawData + nBytes );
			
			// Delete the ALUT-provided memory.
			//
			std::free( rawData );
		}
		
		if( fileData.empty() == false )
		{
			const auto properFormat = fromALFormat( format );
			
			dispatch::mainQueue().async( std::make_shared< fr::dispatch::Block >( [=, completionHandler = std::move( completionHandler ), fileData = std::move( fileData )]() mutable
																				 {
																					 completionHandler( std::move( fileData ), nBytes, nBytes, properFormat, sampleRate );
																				 }, DISPATCH_CODE_LOCATION ));
		}
	}
	
#else	// Begin Mac/iOS
	
	void asyncLoad( path filePath, CompletionHandler&& completionHandler, AudioCue::ErrorHandler&& errorHandler )
	{
		const auto& pathString = filePath.string();
		ASSERT( !pathString.empty() );
		
		// Loads compressed or uncompressed formats of various descriptions.
		
		OSStatus result = noErr;
		
		// Get the file URL.
		//
		CFURLRef fileURL = CFURLCreateFromFileSystemRepresentation( kCFAllocatorDefault, (UInt8*) pathString.c_str(), pathString.size(), false );
		if( !fileURL )
		{
			// Load failed.
			RETURN_ERROR( "Could not open file '" << pathString << "'." );
		}
		
		// Open the file with extended audio services.
		//
		ExtAudioFileRef audioFile = nullptr;
		result = ExtAudioFileOpenURL( fileURL, &audioFile );
		CFRelease( fileURL );

		if( result != noErr )
		{
			RETURN_ERROR( "Loading audio file '" << pathString << "' gave OSStatus " << result << "." );
		}
		if( !audioFile )
		{
			RETURN_ERROR( "Loading audio file '" << pathString << "' returned a null audio file." );
		}
		
		
		// Get the audio data format.
		//
		AudioStreamBasicDescription inputFileFormat;
		UInt32 propertyBytes = sizeof( inputFileFormat );
		result = ExtAudioFileGetProperty( audioFile, kExtAudioFileProperty_FileDataFormat, &propertyBytes, &inputFileFormat );

		if( result != noErr )
		{
			ExtAudioFileDispose( audioFile );
			RETURN_ERROR( "Getting audio file format property for '" << pathString << "' gave OSStatus " << result << "." );
		}

		if( inputFileFormat.mChannelsPerFrame > 2 )
		{
			ExtAudioFileDispose( audioFile );
			RETURN_ERROR( "Audio file '" << pathString << "' has too many channels per frame (" << inputFileFormat.mChannelsPerFrame << "; expected max: 2)." );
		}
		
		// Set the output format to 16 bit signed integer (native-endian) data
		// Maintain the channel count and sample rate of the original source format
		//
		AudioStreamBasicDescription outputFileFormat = inputFileFormat;
		
		outputFileFormat.mFormatID = kAudioFormatLinearPCM;		// Force output to be linear PCM.
		
		if( inputFileFormat.mFormatID != kAudioFormatLinearPCM )
		{
			// If going from compressed to uncompressed, use 16 bits on the receiving end. (Otherwise just leave it the same as the input file.)
			//
			outputFileFormat.mBitsPerChannel = 16;
		}
		size_t bytesPerChannel = outputFileFormat.mBitsPerChannel / 8;
		
		outputFileFormat.mFramesPerPacket = 1;
		outputFileFormat.mBytesPerPacket = static_cast< unsigned int >( bytesPerChannel * outputFileFormat.mChannelsPerFrame );
		outputFileFormat.mBytesPerFrame = static_cast< unsigned int >( bytesPerChannel * outputFileFormat.mChannelsPerFrame );
		
		outputFileFormat.mFormatFlags = kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked | kAudioFormatFlagIsSignedInteger;
		
		// Set the desired generated (output) data format
		//
		result = ExtAudioFileSetProperty( audioFile, kExtAudioFileProperty_ClientDataFormat, sizeof( outputFileFormat ), &outputFileFormat );
		
		if( result != noErr )
		{
			ExtAudioFileDispose( audioFile );
			RETURN_ERROR( "Error setting client data format for audio file '" << pathString << "'." );
		}
		
		if( outputFileFormat.mBitsPerChannel != 16 )
		{
			ExtAudioFileDispose( audioFile );
			RETURN_ERROR( "Audio file '" << pathString << "' had " << outputFileFormat.mBitsPerChannel << " bits per channel. 16 bits required." );
		}
		
		// Get the total frame count
		//
		SInt64 nLargeFramesInFile = 0;
		propertyBytes = sizeof( nLargeFramesInFile );
		result = ExtAudioFileGetProperty( audioFile, kExtAudioFileProperty_FileLengthFrames, &propertyBytes, &nLargeFramesInFile );
		if( result != noErr )
		{
			ExtAudioFileDispose( audioFile );
			RETURN_ERROR( "Error getting file length for audio file '" << pathString << "'." );
		}
		
		UInt32 nFramesInFile = static_cast< UInt32 >( nLargeFramesInFile );
		
		// Allocate enough space for all the sound data.
		//
		auto nBytes = static_cast< size_t >( nFramesInFile * outputFileFormat.mBytesPerFrame );
		std::vector< unsigned char > fileData( nBytes );
		
		const auto nBytesLoaded = readAudioByteCount( audioFile, fileData.data(), nBytes, &nFramesInFile, outputFileFormat.mChannelsPerFrame );
		
		// ExtAudioFileRead() may read fewer than the initially-reported number of frames, even when all are requested, and thus end up with a shorter buffer.
		// Adjust accordingly.
		//
		if( nBytes != nBytesLoaded )
		{
			nBytes = nBytesLoaded;
			fileData.resize( nBytes );
			fileData.shrink_to_fit();
		}
		
		const auto format = ( outputFileFormat.mChannelsPerFrame > 1 ) ? fr::AudioCue::F_Stereo16 : fr::AudioCue::F_Mono16;
		const auto sampleRate = outputFileFormat.mSampleRate;
		
		dispatch::mainQueue().async( std::make_shared< fr::dispatch::Block >( [=, completionHandler = std::move( completionHandler ), fileData = std::move( fileData )]() mutable
																			 {
																				 completionHandler( std::move( fileData ), nBytes, nBytesLoaded, format, sampleRate );
																			 }, DISPATCH_CODE_LOCATION ));
		
		// Dispose of the audio file.
		//
		ASSERT( audioFile );
		ExtAudioFileDispose( audioFile );
		audioFile = nullptr;
	}
	
#endif
}
