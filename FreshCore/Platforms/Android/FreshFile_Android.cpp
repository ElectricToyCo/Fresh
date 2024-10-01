/*
 *  FreshFile_Android.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 2014/04/23.
 *  Copyright 2014 jeffwofford.com. All rights reserved.
 *
 */

#include "FreshFile.h"
#include "FreshDebug.h"
#include "FreshException.h"
#include "CommandProcessor.h"
#include <fstream>
#include <cstdlib>
#include <utime.h>
#include <pwd.h>
#include <sys/stat.h>
#include <jni.h>
#include <android_native_app_glue.h>
#include <android/asset_manager.h>

using namespace fr;

extern struct android_app* g_androidApp;

namespace
{
	path getActivityPath( const std::string& queryFunctionName, JNIEnv* const initialEnvironment = nullptr )
	{
		// See http://stackoverflow.com/questions/10683119/android-app-activity-internaldatapath-still-null-in-2-3-8-ndk-r8

		ASSERT( g_androidApp );

		path thePath;

		JNIEnv* jni = initialEnvironment;

		if( !jni )
		{
			// Get the JNI.
			g_androidApp->activity->vm->AttachCurrentThread( &jni, NULL );
		}

		// Get the file.
		//
		jclass activityClass = jni->GetObjectClass( g_androidApp->activity->clazz );
		jmethodID method = jni->GetMethodID( activityClass, queryFunctionName.c_str(), "()Ljava/io/File;" );
		jobject fileObject = jni->CallObjectMethod( g_androidApp->activity->clazz, method );

		if( !fileObject )
		{
			release_error( "Android: file query '" << queryFunctionName << "' returned null." );
		}
		else
		{
			// Get the file path.
			//
			jclass fileClass = jni->GetObjectClass( fileObject );
			jmethodID getAbsolutePath = jni->GetMethodID( fileClass, "getAbsolutePath", "()Ljava/lang/String;" );
			jstring pathObject = (jstring) jni->CallObjectMethod( fileObject, getAbsolutePath );

			const char* pathChars = jni->GetStringUTFChars( pathObject, NULL );
			ASSERT( pathChars );
			thePath = pathChars;

			jni->ReleaseStringUTFChars( pathObject, pathChars );
			jni->DeleteLocalRef( pathObject );
			jni->DeleteLocalRef( fileClass );
			jni->DeleteLocalRef( fileObject );
		}

		jni->DeleteLocalRef( activityClass );

		if( !initialEnvironment )
		{
			g_androidApp->activity->vm->DetachCurrentThread();
		}

		return thePath;
	}

	path unpackResource( const path& resourcePath )
	{
		ASSERT( g_androidApp );

		// For Android, remove the "asset" subfolder at the beginning of all resources.
		path amendedPath;

		auto pathIter = resourcePath.begin();
		ASSERT( pathIter != resourcePath.end() );
		ASSERT( *pathIter == "assets" );

		++pathIter;
		ASSERT( pathIter != resourcePath.end() );

		amendedPath = *pathIter;
		++pathIter;

		for( ; pathIter != resourcePath.end(); ++pathIter )
		{
			amendedPath /= *pathIter;
		}

		// Assumes that the current working directory is writable.

		// Attach to the virtual machine thread so that the threads are coordinated.

		JNIEnv* env = nullptr;
		g_androidApp->activity->vm->AttachCurrentThread( &env, NULL );

		// Get the asset manager.
		//
		AAssetManager* const assetManager = g_androidApp->activity->assetManager;

		AAsset* const asset = AAssetManager_open( assetManager, amendedPath.c_str(), AASSET_MODE_STREAMING );

		if( !asset )
		{
			FRESH_THROW( FreshException, "Failed to find resource '" << amendedPath << "'." );
		}

		// TODO Avoid the overwrite if the files are identical.

		create_directories( resourcePath.parent_path() );
		FILE* out = std::fopen( resourcePath.c_str(), "w" );

		char buffer[ BUFSIZ ];

		int bytesRead = 0;
		while(( bytesRead = AAsset_read( asset, buffer, BUFSIZ )) > 0 )
		{
			std::fwrite( buffer, bytesRead, 1, out );
		}

		std::fclose( out );
		AAsset_close( asset );

		g_androidApp->activity->vm->DetachCurrentThread();

		return resourcePath;
	}

	path androidInternalDataPath()
	{
		ASSERT( g_androidApp );

		const char* szPath = g_androidApp->activity->internalDataPath;
		if( !szPath )
		{
			return getActivityPath( "getFilesDir" );
		}
		else
		{
			return path{ szPath };
		}
	}

	path androidExternalDataPath()
	{
		ASSERT( g_androidApp );

		const char* szPath = g_androidApp->activity->externalDataPath;
		if( !szPath )
		{
			return getActivityPath( "getExternalFilesDir" );
		}
		else
		{
			return path{ szPath };
		}
	}

	path androidCachePath()
	{
		return getActivityPath( "getCacheDir" );
	}
}

namespace fr
{
	void processWorkingDirectoryRedirection()
	{
		const auto assetPath = androidCachePath();

		// Nuance the working directory so that we can find assets.
		//
		release_trace( "Android: Setting working directory to " << assetPath );
		::chdir( assetPath.c_str() );

	}

	path getDocumentBasePath()
	{
		path basePath = androidInternalDataPath();

		basePath /= documentSubfolderPath();

		if( !exists( basePath ))
		{
			dev_trace( "Android: Creating document base path " << basePath );
			create_directory( basePath );
		}

		return basePath;
	}

    void explorePath( const path& path )
    {
//        debug_trace( "Platform ignoring `explorePath( " << path << " )`." );
        // Ignored.
    }

	path getResourcePath( const path& resourcePath )
	{
		return unpackResource( resourcePath );
	}

	void openURLInBrowser( const std::string& url )
	{
		// Get the JNI.
		//
		ASSERT( g_androidApp );
		JNIEnv* jni = nullptr;
		g_androidApp->activity->vm->AttachCurrentThread( &jni, NULL );
		ASSERT( jni );

		// Convert the string to Java.
		//
		jstring jstrUrl = jni->NewStringUTF( url.c_str() );
		ASSERT( jstrUrl );

		// Find the method.
		//
		jclass activityClass = jni->GetObjectClass( g_androidApp->activity->clazz );
		ASSERT( activityClass );

		jmethodID method = jni->GetMethodID( activityClass, "visitURL", "(Ljava/lang/String;)V" );
		ASSERT( method );

		// Call the method.
		//
		jni->CallVoidMethod( g_androidApp->activity->clazz /* actually the Java activity itself */ , method, jstrUrl );

		// Release the thread.
		//
		g_androidApp->activity->vm->DetachCurrentThread();
	}

	path getTempDirectoryPath()
	{
		return "/tmp";
	}

	path createTempFile( std::ofstream& outTempFile )
	{
		// Find where to put it.
		//
		path tempPath = getTempDirectoryPath();		// Includes trailing /
		ASSERT( tempPath.empty() == false );
		tempPath /= "freshXXXXXX";

		// Make tempBasePath writable so that mkstemp can jack with it.
		//
		const char* szPath = tempPath.c_str();
		const size_t lenPath = tempPath.string().length();

		std::vector< char > tempBasePathBytes( szPath, szPath + lenPath );
		tempBasePathBytes.push_back( '\0' );		// Need null terminator though.

		const int fileDescriptor = ::mkstemp( tempBasePathBytes.data() );
		ASSERT( fileDescriptor != -1 );

		// Convert the modified path back to a string.
		//
		std::string tempBasePathString( tempBasePathBytes.begin(), tempBasePathBytes.end() - 1 );

		tempPath = tempBasePathString;

		outTempFile.open( tempPath.c_str(), std::ios_base::trunc | std::ios_base::out );
		close( fileDescriptor );

		return tempPath;
	}

	std::time_t getFileLastModifiedTime( const path& filePath )
	{
		struct stat attrib;			// create a file attribute structure
		int result = ::stat( filePath.c_str(), &attrib );		// get the attributes of afile.txt
		if( result != -1 )
		{
			return attrib.st_mtime;
		}
		else
		{
			return time_t( 0 );
		}
	}

	void setFileLastModifiedTime( const path& filePath, std::time_t time )
	{
		utimbuf modificationAndAccessTimes;
		modificationAndAccessTimes.actime = time;
		modificationAndAccessTimes.modtime = time;
		::utime( filePath.c_str(), &modificationAndAccessTimes );
	}

	void copyToPasteboard( const std::string& string )
	{
		// TODO
	}

	std::string pasteFromPasteboard()
	{
		// TODO
		return "";
 	}
}
