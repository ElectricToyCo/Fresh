cmake_minimum_required( VERSION 2.8 )
project( fac )

#### Use Clang compiler on Apple systems

if( APPLE )
	set( CMAKE_CXX_COMPILER_ID		"Clang" )
	set( CMAKE_C_COMPILER           "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang" )
	set( CMAKE_CXX_COMPILER         "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++" )
	set( CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD c++14 )
	set( CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY libc++ )
endif ()

#### Functions

if( "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" )
	message( "Clang all load" )
	set( WHOLE_ARCHIVE_ON "-Wl,-all_load" )
	set( WHOLE_ARCHIVE_OFF "" )	# TODO
else()
	set( WHOLE_ARCHIVE_ON "-Wl,-whole-archive" )
	set( WHOLE_ARCHIVE_OFF "-Wl,-no-whole-archive" )
endif()

function( my_link_whole_library target libraryName )
	target_link_libraries( ${target} ${WHOLE_ARCHIVE_ON} ${libraryName} ${WHOLE_ARCHIVE_OFF} )
endfunction( my_link_whole_library )

#### END Functions


#### Initialize CXXFLAGS.

set( CMAKE_CXX_FLAGS                "-Wall -std=c++17")
set( CMAKE_CXX_FLAGS_DEBUG          "-O0 -g -DDEBUG -DDEV_MODE=1 -DFRESH_PROFILER_ENABLED=1")	#TODO Profiling
set( CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
set( CMAKE_CXX_FLAGS_RELEASE        "-O3 -DNDEBUG -DDEV_MODE=1 -D")
set( CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g" )

#### Compiler-specific C++17 activation.

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
    if( NOT( GCC_VERSION VERSION_GREATER 4.9 OR GCC_VERSION VERSION_EQUAL 4.9 ))
        message( FATAL_ERROR "${PROJECT_NAME} requires g++ 4.9 or greater." )
    endif ()
elseif( "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" )
	
else ()
	message("Your C++ compiler does not support C++17! I'll do my best...")
endif ()

#### Create TINYXML library

include_directories( ../../lib/tinyxml_src )
add_library( tinyxml
	../../lib/tinyxml_src/tinystr.cpp
	../../lib/tinyxml_src/tinyxml.cpp
	../../lib/tinyxml_src/tinyxmlerror.cpp
	../../lib/tinyxml_src/tinyxmlparser.cpp
)

################ Add Fresh sub-libraries as sources

#### Add FreshCore library sources

include_directories( ../../FreshCore )
file( GLOB FreshCoreSources ../../FreshCore/*.cpp )
file( GLOB FreshCoreHeaders ../../FreshCore/*.h )

# Identify FreshCore platform-specific source code.
if( APPLE )

	file( GLOB FreshCoreAppleSources ../../FreshCore/Platforms/Apple/*.h ../../FreshCore/Platforms/Apple/*.cpp ../../FreshCore/Platforms/Apple/*.m ../../FreshCore/Platforms/Apple/*.mm )
	file( GLOB FreshCoreMacSources ../../FreshCore/Platforms/MacOSX/*.h ../../FreshCore/Platforms/MacOSX/*.cpp ../../FreshCore/Platforms/MacOSX/*.m ../../FreshCore/Platforms/MacOSX/*.mm )

	set( FreshCorePlatformSpecificSources ${FreshCoreAppleSources} ${FreshCoreMacSources} )

elseif( WIN32 )

	file( GLOB FreshCorePlatformSpecificSources ../../FreshCore/Platforms/Win/*.h ../../FreshCore/Platforms/Win/*.cpp )

endif( APPLE)

add_library( FreshCore ${FreshCoreSources} ${FreshCoreHeaders} ${FreshCorePlatformSpecificSources} )

#### Add FreshPlatform library

include_directories( ../../FreshPlatform )
file( GLOB FreshPlatformSources ../../FreshPlatform/*.cpp )
file( GLOB FreshPlatformHeaders ../../FreshPlatform/*.h )

# Identify FreshPlatform platform-specific source code.
if( APPLE )
	file( GLOB FreshPlatformPlatformSpecificSources ../../FreshPlatform/Platforms/MacOSX/*.h ../../FreshPlatform/Platforms/MacOSX/*.cpp ../../FreshPlatform/Platforms/MacOSX/*.m ../../FreshPlatform/Platforms/MacOSX/*.mm )

	#TODO set(CMAKE_OSX_SYSROOT iphoneos2.2.1)
	#set(CMAKE_OSX_ARCHITECTURES $(ARCHS_STANDARD_32_BIT))

elseif( WIN32 )

	file( GLOB FreshPlatformPlatformSpecificSources ../../FreshPlatform/Platforms/Win/*.h ../../FreshPlatform/Platforms/Win/*.cpp )

endif( APPLE)

add_library( FreshPlatform ../../lib/stb_image.cpp ${FreshPlatformSources} ${FreshPlatformHeaders} ${FreshPlatformPlatformSpecificSources} )

find_package( OpenGL )
if( OPENGL_FOUND )
	include_directories( ${OPENGL_INCLUDE_DIR} )
else( OPENGL_FOUND )
    message("Could not find OpenGL.")
endif (OPENGL_FOUND) 

if( WIN32 )

	# OpenAL on Windows
	include_directories( ../../lib/Win/OpenAL/include )
	set( OPENAL_LIBRARY ${PROJECT_SOURCE_DIR}/../../lib/Win/OpenAL/OpenAL32.dll )

	# GLEW on Windows
	include_directories( ${PROJECT_SOURCE_DIR}/../../lib/Win/glew/include )
	set( GLEW ${PROJECT_SOURCE_DIR}/../../lib/Win/glew/glew32.dll )

	# Winsock
	find_library( WINSOCK_LIB ws2_32 )
	if( NOT WINSOCK_LIB )
    	message( "Could not locate the winsock library (ws2_32)")
  	endif()

else( WIN32 )
	find_package( OpenAL )
	if( OPENAL_FOUND )
		include_directories( ${OPENAL_INCLUDE_DIR} )
	else( OPENAL_FOUND )
		message("Could not find OpenAL.")
	endif (OPENAL_FOUND)
endif( WIN32 )

#### Enable TCLAP library

include_directories( ../../lib/tclap )

file( GLOB facSourceFiles *.cpp *.h )
add_executable( fac ${facSourceFiles} )

#### Platform-specific link libraries

my_link_whole_library( fac FreshCore FreshPlatform )
my_link_whole_library( fac FreshPlatform )
target_link_libraries( fac tinyxml ${GLEW} ${WINSOCK_LIB} ${OPENGL_LIBRARIES} ${OPENAL_LIBRARY} )

