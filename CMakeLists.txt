cmake_minimum_required( VERSION 3.1 )
project( Fresh )

include( CMakeBaseSettings.txt )

#### Find general standard system packages

if( IOS )
	message( "Building for iOS." )
	find_library( OpenGLES2 OpenGLES2 REQUIRED )
	include_directories( ${OPENGLES2_INCLUDE_DIR} )
elseif( ANDROID )
	# TODO Android support incomplete
	message( "Building for Android." )
	find_library( OpenGLES2 OpenGLES2 REQUIRED )
	include_directories( ${OPENGLES2_INCLUDE_DIR} )
elseif( RASPBIAN )
	message( "Building for Raspbian." )
	find_library( OpenGLES2 OpenGLES2 REQUIRED )
	include_directories( ${OPENGLES2_INCLUDE_DIR} )
	include_directories(
	    ${CMAKE_FIND_ROOT_PATH}/opt/vc/include
	    ${CMAKE_FIND_ROOT_PATH}/opt/vc/include/interface/vcos/pthreads
	    ${CMAKE_FIND_ROOT_PATH}/opt/vc/include/interface/vmcs_host/linux
	)
	link_directories( ${CMAKE_FIND_ROOT_PATH}/opt/vc/lib )
elseif( EMSCRIPTEN )
	message( "Building for emscripten." )
else()
	find_package( OpenGL REQUIRED )
	include_directories( ${OPENGL_INCLUDE_DIR} )
endif()

### Other library dependencies.

if( WIN32 OR EMSCRIPTEN OR RASPBIAN )
	# ALUT on Windows (for loading audio)
	include_directories( lib/freealut/include )
endif()

if( WIN32 )

	# OpenAL on Windows
	include_directories( lib/Win/OpenAL/include )

	# GLEW on Windows
	include_directories( ${PROJECT_SOURCE_DIR}/lib/Win/glew/include )

	# Threading on Windows (Basic C++ threading is omited on MinGW. We re-add it with a library.)
	# https://github.com/meganz/mingw-std-threads
	#
	option(MINGW_STDTHREADS_GENERATE_STDHEADERS "" ON)
	add_subdirectory( ${PROJECT_SOURCE_DIR}/lib/mingw-std-threads )
	set( MINGW_STDTHREADS "mingw_stdthreads" )

else()		# All other platforms

	find_package( OpenAL )
	if( OPENAL_FOUND )
		if( RASPBIAN )
			set( OPENAL_INCLUDE_DIR "${OPENAL_INCLUDE_DIR}/.." )
		endif()

		message( "Found OpenAL with include directory " ${OPENAL_INCLUDE_DIR} )

		include_directories( ${OPENAL_INCLUDE_DIR} )
	else()
		message( "Could not find OpenAL." )
	endif()
endif()

# Finish setting up cURL

if( CURL_FOUND )
	include_directories( ${CURL_INCLUDE_DIRS} )
else()
	message( "Could not find cURL library." )
endif()

#### Create TinyXML library

include_directories( lib/tinyxml_src )
add_library( tinyxml
	lib/tinyxml_src/tinystr.cpp
	lib/tinyxml_src/tinyxml.cpp
	lib/tinyxml_src/tinyxmlerror.cpp
	lib/tinyxml_src/tinyxmlparser.cpp
)

#### Create LUA library

include_directories( lib/lua )
add_library( lua
	lib/lua/lua/lapi.c
	lib/lua/lua/lauxlib.c
	lib/lua/lua/lbaselib.c
	lib/lua/lua/lbitlib.c
	lib/lua/lua/lcode.c
	lib/lua/lua/lcorolib.c
	lib/lua/lua/lctype.c
	lib/lua/lua/ldblib.c
	lib/lua/lua/ldebug.c
	lib/lua/lua/ldo.c
	lib/lua/lua/ldump.c
	lib/lua/lua/lfunc.c
	lib/lua/lua/lgc.c
	lib/lua/lua/linit.c
	lib/lua/lua/liolib.c
	lib/lua/lua/llex.c
	lib/lua/lua/lmathlib.c
	lib/lua/lua/lmem.c
	lib/lua/lua/loadlib.c
	lib/lua/lua/lobject.c
	lib/lua/lua/lopcodes.c
	lib/lua/lua/loslib.c
	lib/lua/lua/lparser.c
	lib/lua/lua/lstate.c
	lib/lua/lua/lstring.c
	lib/lua/lua/lstrlib.c
	lib/lua/lua/ltable.c
	lib/lua/lua/ltablib.c
	lib/lua/lua/ltm.c
	lib/lua/lua/lundump.c
	lib/lua/lua/lutf8lib.c
	lib/lua/lua/lvm.c
	lib/lua/lua/lzio.c
)

#### Create lodepng library

include_directories( lib/lodepng )
add_library( lodepng lib/lodepng/lodepng.cpp )

#### Create mpg123 library

if( WIN32 OR LINUX OR EMSCRIPTEN OR RASPBIAN )
	message( "mpg123 on Windows/Linux" )
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOPT_GENERIC -DREAL_IS_FLOAT" )
endif()

include_directories( lib/mpg123 )
add_library( mpg123
	lib/mpg123/compat.c
	lib/mpg123/parse.c
	lib/mpg123/frame.c
	lib/mpg123/format.c
	lib/mpg123/dct64.c
	lib/mpg123/equalizer.c
	lib/mpg123/id3.c
	lib/mpg123/icy.c
	lib/mpg123/icy2utf8.c
	lib/mpg123/optimize.c
	lib/mpg123/readers.c
	lib/mpg123/tabinit.c
	lib/mpg123/libmpg123.c
	lib/mpg123/index.c
	lib/mpg123/layer1.c
	lib/mpg123/layer2.c
	lib/mpg123/layer3.c
	lib/mpg123/dither.c
	lib/mpg123/feature.c
	lib/mpg123/synth.c
	lib/mpg123/ntom.c
	lib/mpg123/synth_8bit.c
	lib/mpg123/stringbuf.c )

################ Add Fresh sub-libraries as sources

#### Add FreshCore library sources

include_directories( FreshCore )
file( GLOB FreshCoreSources FreshCore/*.cpp )
file( GLOB FreshCoreHeaders FreshCore/*.h )

# Identify FreshCore platform-specific source code.
if( APPLE )

	file( GLOB FreshCoreAppleSources FreshCore/Platforms/Apple/*.h FreshCore/Platforms/Apple/*.cpp FreshCore/Platforms/Apple/*.m FreshCore/Platforms/Apple/*.mm )
	file( GLOB FreshCoreMacSources FreshCore/Platforms/MacOSX/*.h FreshCore/Platforms/MacOSX/*.cpp FreshCore/Platforms/MacOSX/*.m FreshCore/Platforms/MacOSX/*.mm )

	set( FreshCorePlatformSpecificSources ${FreshCoreAppleSources} ${FreshCoreMacSources} )

elseif( WIN32 )

	file( GLOB FreshCorePlatformSpecificSources FreshCore/Platforms/Win/*.h FreshCore/Platforms/Win/*.cpp )

elseif( ANDROID )

	# TODO	

elseif( EMSCRIPTEN )

	file( GLOB FreshCorePlatformSpecificSources FreshCore/Platforms/Emscripten/*.h FreshCore/Platforms/Emscripten/*.cpp )
	
elseif( UNIX )

	message( "Compiling for UNIX." )
	file( GLOB FreshCorePlatformSpecificSources FreshCore/Platforms/Unix/*.h FreshCore/Platforms/Unix/*.cpp )

endif()

add_library( FreshCore ${FreshCoreSources} ${FreshCoreHeaders} ${FreshCorePlatformSpecificSources} )

#### Add FreshPlatform library

include_directories( FreshPlatform FreshPlatform/Platforms )
file( GLOB FreshPlatformSources FreshPlatform/*.cpp )
file( GLOB FreshPlatformHeaders FreshPlatform/*.h )

# Identify FreshPlatform platform-specific source code.
if( APPLE )

	file( GLOB FreshPlatformPlatformSpecificSources FreshPlatform/Platforms/MacOSX/*.h FreshPlatform/Platforms/MacOSX/*.cpp FreshPlatform/Platforms/MacOSX/*.m FreshPlatform/Platforms/MacOSX/*.mm )

elseif( WIN32 )

	file( GLOB FreshPlatformPlatformSpecificSources FreshPlatform/Platforms/Win/*.h FreshPlatform/Platforms/Win/*.cpp )
	file( GLOB FreshPlatformPlatformSpecificSources ${FreshPlatformPlatformSpecificSources} FreshPlatform/Platforms/Null_Platform/AudioSession_Null.cpp FreshPlatform/Platforms/Null_Platform/FreshAnalytics_Null.cpp )

elseif( ANDROID )

# TODO	

elseif( EMSCRIPTEN )

		file( GLOB FreshPlatformPlatformSpecificSources FreshPlatform/Platforms/glESHelpers.cpp FreshPlatform/Platforms/Emscripten/*.h FreshPlatform/Platforms/Emscripten/*.cpp )

		file( GLOB FreshPlatformPlatformSpecificSources ${FreshPlatformPlatformSpecificSources} FreshPlatform/Platforms/Null_Platform/Gamepad_Null.cpp FreshPlatform/Platforms/Null_Platform/AudioSession_Null.cpp FreshPlatform/Platforms/Null_Platform/FreshAnalytics_Null.cpp )

elseif( UNIX )

	if( RASPBIAN )
		file( GLOB FreshPlatformPlatformSpecificSources FreshPlatform/Platforms/glESHelpers.cpp FreshPlatform/Platforms/RPi/*.h FreshPlatform/Platforms/RPi/*.cpp )

		file( GLOB FreshPlatformPlatformSpecificSources ${FreshPlatformPlatformSpecificSources} FreshPlatform/Platforms/Null_Platform/Gamepad_Null.cpp FreshPlatform/Platforms/Null_Platform/AudioSession_Null.cpp FreshPlatform/Platforms/Null_Platform/FreshAnalytics_Null.cpp )

	else()
		file( GLOB FreshPlatformPlatformSpecificSources FreshPlatform/Platforms/Unix/*.h FreshPlatform/Platforms/Unix/*.cpp FreshPlatform/Platforms/Null_Platform/AudioSession_Null.cpp FreshPlatform/Platforms/Null_Platform/FreshAnalytics_Null.cpp)
	endif()

endif()

add_library( FreshPlatform FreshPlatform/Platforms/Null_Platform/FreshGameCenter_Null.cpp FreshPlatform/Platforms/Null_Platform/FreshSocial_Null.cpp FreshPlatform/Platforms/Null_Platform/FreshLicensing_Null.cpp FreshPlatform/Platforms/ImageLoader_Generic.cpp FreshPlatform/Platforms/glESHelpers.cpp ${FreshPlatformSources} ${FreshPlatformHeaders} ${FreshPlatformPlatformSpecificSources} )

#### Add FreshScene2D library

include_directories( FreshScene2D )
file( GLOB FreshScene2DSources FreshScene2D/*.cpp )
file( GLOB FreshScene2DHeaders FreshScene2D/*.h )

add_library( FreshScene2D ${FreshScene2DSources} ${FreshScene2DHeaders} )

#### Add FreshEditor library

# include_directories( FreshEditor )
# file( GLOB FreshScene2DSources FreshEditor/*.cpp )
# file( GLOB FreshScene2DHeaders FreshEditor/*.h )

# add_library( FreshEditor ${FreshScene2DSources} ${FreshScene2DHeaders} )

#### Add FreshGame library

include_directories( FreshGame )
file( GLOB FreshGameSources FreshGame/*.cpp )
file( GLOB FreshGameHeaders FreshGame/*.h )

add_library( FreshGame ${FreshGameSources} ${FreshGameHeaders} )

#### Add FreshScript library

include_directories( FreshScript )
file( GLOB FreshScriptSources FreshScript/*.cpp )
file( GLOB FreshScriptHeaders FreshScript/*.h FreshScript/*.hpp )

add_library( FreshScript ${FreshScriptSources} ${FreshScriptHeaders} )
