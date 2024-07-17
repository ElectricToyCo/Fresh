/*
 *  FreshEssentials.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 6/29/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */


#include "FreshEssentials.h"
#include "FreshFile.h"
#include "AudioSession.h"
#include <cstring>
#include <sstream>
#include <iomanip>
#include <regex>
#include <functional>
#include <map>

#if TARGET_OS_IPHONE || TARGET_OS_MAC
#   include <sys/sysctl.h>
// For accessing the IP address of the machine.
#   include <ifaddrs.h>
#   include <arpa/inet.h>

#   include <CoreFoundation/CFPreferences.h>

#   if TARGET_OS_IPHONE
#       include <CoreFoundation/CFUUID.h>
#       include <AudioToolbox/AudioServices.h>
#   elif TARGET_OS_MAC
#       include <CoreFoundation/CFPreferences.h>
#       include <IOKit/IOKitLib.h>
#   endif
#endif


namespace
{
	bool CompareCharactersInsensitive( char a, char b )
	{
		return std::toupper( a, fr::getGeneralLocale() ) == std::toupper( b, fr::getGeneralLocale() );
	}

	// Adapted from http://stackoverflow.com/questions/3589936/c-urlencode-library-unicode-capable.
	//
	void urlEncodePerCharacter( std::string& str, std::string::value_type v )
	{
		if (isalnum(v))
		{
			str += v;
		}
		else
		{
			std::ostringstream enc;
			enc << '%' << std::setw( 2 ) << std::setfill( '0' ) << std::hex << std::uppercase << static_cast< int >( v );
			str += enc.str();
		}
	}

#if !TARGET_OS_MAC

	//
	// Windows/Linux "preferences" support.
	//

	typedef std::map< std::string, std::string > Preferences;
	Preferences g_preferences,			// Current in-memory preferences.
				g_basePreferences;		// Preferences at last sync.

	void savePreferences( const Preferences& preferences )
	{
		auto preferencesPath = fr::getDocumentPath( "preferences.txt" );

		create_directories( preferencesPath.parent_path() );

		std::ofstream preferencesFile( preferencesPath.string() );

		if( !preferencesFile )
		{
			release_error( "Could not create or write preferences file '" << preferencesPath << "'." );
			return;
		}

		for( const auto& preference : preferences )
		{
			preferencesFile << preference.first << "=" << preference.second << std::endl;
		}
	}

	Preferences loadPreferences()
	{
		auto preferencesPath = fr::getDocumentPath( "preferences.txt" );

		std::ifstream preferencesFile;
		preferencesFile.open( preferencesPath.string() );

		if( !preferencesFile )
		{
			release_error( "Could not open preferences file '" << preferencesPath << "'." );
			return Preferences{};
		}

		Preferences preferences;
		while( preferencesFile.good() )
		{
			// Read until =.
			std::string key;
			while( preferencesFile.good() && preferencesFile.peek() != '=' && preferencesFile.peek() != '\n' && preferencesFile.peek() != std::char_traits< char >::eof() )
			{
				key.push_back( preferencesFile.get() );
			}

			fr::trim( key );

			if( !preferencesFile.good() || preferencesFile.peek() != '=' )
			{
				if( !key.empty() )
				{
					dev_error( "Preference file '" << preferencesPath << "' had key '" << key << "' but stopped short of equals. Aborting read." );
				}
				break;
			}
			preferencesFile.get();

			std::string value;
			std::getline( preferencesFile, value );

			fr::trimLeft( value );

			preferences[ key ] = value;
		}

		return preferences;
	}

	void syncPreferencesInternal()
	{
		Preferences onDiskPreferences = loadPreferences();

		// Build a common preference map from the in-memory and disk preferences, preferring in-memory values when resolving conflicts.
		//
		for( const auto& preference : onDiskPreferences )
		{
			auto iterMemPref =  g_preferences.find( preference.first );
			auto iterBasePref = g_basePreferences.find( preference.first );

			// Don't set from disk iff we have a changed in-memory preference.
			const bool changedInMemory = iterMemPref != g_preferences.end() && ( iterBasePref == g_basePreferences.end() || *iterMemPref != *iterBasePref );

			if( !changedInMemory )
			{
				g_preferences[ preference.first ] = preference.second;
			}
		}

		savePreferences( g_preferences );

		// Record the common preference state.
		//
		g_basePreferences = g_preferences;
	}

	bool hasPreference( const std::string& key )
	{
		return g_preferences.find( key ) != g_preferences.end();
	}

	std::string preference( const std::string& key )
	{
		syncPreferencesInternal();

		auto iter = g_preferences.find( key );
		if( iter != g_preferences.end())
		{
			return iter->second;
		}
		else
		{
			return "";
		}
	}

	void preference( const std::string& key, const std::string& value )
	{
		g_preferences[ key ] = value;
		syncPreferencesInternal();
	}


#endif
}

namespace fr
{

	std::string getByteCountString( size_t nBytes )
	{
		size_t denomination = 0;

		double bytes = nBytes;
		while( bytes >= 1024.0 && denomination < 4 )
		{
			++denomination;
			bytes /= 1024.0;
		}

		std::ostringstream out;
		out << std::fixed << std::setprecision( 2 ) << bytes << " ";

		switch( denomination )
		{
			default:
			case 0:
				out << "B";
				break;

			case 1:
				out << "KiB";
				break;

			case 2:
				out << "MiB";
				break;

			case 3:
				out << "GiB";
				break;
		}

		return out.str();
	}


	std::locale& getGeneralLocale()
	{
		static std::locale s_locale( std::locale::classic() );
		return s_locale;
	}

	void savePreference( const std::string& key, const std::string& value )
	{
		REQUIRES( !key.empty() );
#if TARGET_OS_MAC

		CFStringRef cfKey = CFStringCreateWithCString( kCFAllocatorDefault, key.c_str(), kCFStringEncodingUTF8 );
		CFStringRef cfValue = CFStringCreateWithCString( kCFAllocatorDefault, value.c_str(), kCFStringEncodingUTF8 );

		CFPreferencesSetAppValue( cfKey, cfValue, kCFPreferencesCurrentApplication );

		CFRelease( cfKey );
		CFRelease( cfValue );

#else
		preference( key, value );
#endif
	}

	bool loadPreference( const std::string& key, std::string& outValue )
	{
		REQUIRES( !key.empty() );

#if TARGET_OS_MAC
		CFStringRef cfKey = CFStringCreateWithCString( kCFAllocatorDefault, key.c_str(), kCFStringEncodingUTF8 );
		CFStringRef value = (CFStringRef) CFPreferencesCopyAppValue( cfKey, kCFPreferencesCurrentApplication );
		CFRelease( cfKey );

		if( value )
		{
			outValue = stringFromCFString( value );
			CFRelease( value );
		}

		return !!value;
#else
		syncPreferences();
		if( !hasPreference( key ))
		{
			return false;
		}
		else
		{
			outValue = preference( key );
			return true;
		}
#endif
	}

	void syncPreferences()
	{
#if TARGET_OS_MAC
		CFPreferencesAppSynchronize( kCFPreferencesCurrentApplication );
#else
		syncPreferencesInternal();
#endif
	}

	bool isSystemMusicPlaying()
	{
		return fr::audiosession::isOtherAudioPlaying();
	}

	// Adapted From http://stackoverflow.com/questions/3589936/c-urlencode-library-unicode-capable
	std::string urlEncode( const std::string& url )
	{
		std::string result;
		std::for_each( url.begin(), url.end(), std::bind( urlEncodePerCharacter, std::ref( result ), std::placeholders::_1 ));
		return result;
	}

	std::string getPlatform()
	{
		std::string platform;

#if TARGET_IPHONE_SIMULATOR
		platform = "iOS Simulator";
#elif TARGET_OS_IPHONE
		platform = "iOS ";
		platform += getPlatformModel();

		std::string submodel = getPlatformSubmodel();

		if( submodel.empty() == false )
		{
			platform += " (";
			platform += submodel;
			platform += ")";
		}
#elif TARGET_OS_MAC
		platform = "MacOSX";
#elif defined( _WIN32 )
		platform = "Win32";
#elif defined( __linux__ )
#	if ANDROID
		platform = "Android";
#	elif RASPBIAN
		platform = "Raspbian";
#	else
		platform = "Linux";		// TODO specialized from e.g. Ubuntu, Debian, Red Hat.
#	endif
#elif FRESH_EMSCRIPTEN
		platform = "Emscripten";
#endif

		return platform;
	}

	std::string getPlatformModel()
	{
#if TARGET_OS_MAC
		int mib[] = { CTL_HW, HW_MACHINE };

		size_t nBytes = 0;
		::sysctl( mib, 2, NULL, &nBytes, NULL, 0 );

		std::unique_ptr< char[] > szMachine( new char[ nBytes + 1 ] );

		::sysctl( mib, 2, szMachine.get(), &nBytes, NULL, 0 );

		if( 0 == strcmp( szMachine.get(), "iPhone1,1" ))    return "iPhone 1G";
		if( 0 == strcmp( szMachine.get(), "iPhone1,2" ))    return "iPhone 3G";
		if( 0 == strcmp( szMachine.get(), "iPhone2,1" ))    return "iPhone 3GS";
		if( 0 == strcmp( szMachine.get(), "iPhone3,1" ))    return "iPhone 4";			// AT&T
		if( 0 == strcmp( szMachine.get(), "iPhone3,2" ))    return "iPhone 4";			// Other carrier
		if( 0 == strcmp( szMachine.get(), "iPhone3,3" ))    return "iPhone 4";			// Verizon
		if( 0 == strcmp( szMachine.get(), "iPhone4,1" ))    return "iPhone 4S";			// AT&T
		if( 0 == strcmp( szMachine.get(), "iPhone4,2" ))    return "iPhone 4S";			// Verizon
		if( 0 == strcmp( szMachine.get(), "iPhone5,1" ))    return "iPhone 5";			// GSM
		if( 0 == strcmp( szMachine.get(), "iPhone5,2" ))    return "iPhone 5";			// GSM+CDMA
		if( 0 == strcmp( szMachine.get(), "iPod1,1" ))      return "iPod Touch (1st gen)";
		if( 0 == strcmp( szMachine.get(), "iPod2,1" ))      return "iPod Touch (2nd gen)";
		if( 0 == strcmp( szMachine.get(), "iPod3,1" ))      return "iPod Touch (3rd gen)";
		if( 0 == strcmp( szMachine.get(), "iPod4,1" ))      return "iPod Touch (4th gen)";
		if( 0 == strcmp( szMachine.get(), "iPod5,1" ))      return "iPod Touch (5th gen)";
		if( 0 == strcmp( szMachine.get(), "iPad1,1" ))      return "iPad 1";			// Wifi
		if( 0 == strcmp( szMachine.get(), "iPad1,2" ))      return "iPad 1";			// 3G
		if( 0 == strcmp( szMachine.get(), "iPad2,1" ))      return "iPad 2";			// Wifi
		if( 0 == strcmp( szMachine.get(), "iPad2,2" ))      return "iPad 2";			// GSM
		if( 0 == strcmp( szMachine.get(), "iPad2,3" ))      return "iPad 2";			// CDMA
		if( 0 == strcmp( szMachine.get(), "iPad2,4" ))      return "iPad 2";			// Wifi
		if( 0 == strcmp( szMachine.get(), "iPad2,5" ))      return "iPad Mini";			// Wifi
		if( 0 == strcmp( szMachine.get(), "iPad2,6" ))      return "iPad Mini";			// GSM
		if( 0 == strcmp( szMachine.get(), "iPad2,7" ))      return "iPad Mini";			// GSM+CDMA
		if( 0 == strcmp( szMachine.get(), "iPad3,1" ))      return "iPad 3";			// Wifi
		if( 0 == strcmp( szMachine.get(), "iPad3,2" ))      return "iPad 3";			// GSM+CDMA
		if( 0 == strcmp( szMachine.get(), "iPad3,3" ))      return "iPad 3";			// GSM
		if( 0 == strcmp( szMachine.get(), "iPad3,4" ))      return "iPad 4";			// Wifi
		if( 0 == strcmp( szMachine.get(), "iPad3,5" ))      return "iPad 4";			// GSM
		if( 0 == strcmp( szMachine.get(), "iPad3,6" ))      return "iPad 4";			// GSM+CDMA
		if( 0 == strcmp( szMachine.get(), "i386" ))         return "Simulator";
		if( 0 == strcmp( szMachine.get(), "x86_64" ))       return "Simulator";

		return std::string( szMachine.get() );
#else
		return "PC";
#endif
	}

	std::string getPlatformSubmodel()
	{
#if TARGET_OS_MAC
		int mib[] = { CTL_HW, HW_MACHINE };

		size_t nBytes = 0;
		sysctl( mib, 2, NULL, &nBytes, NULL, 0 );

		std::unique_ptr< char[] > szMachine( new char[ nBytes + 1 ] );

		sysctl( mib, 2, szMachine.get(), &nBytes, NULL, 0 );

		if( 0 == strcmp( szMachine.get(), "iPhone3,1" ))    return "AT&T";
		if( 0 == strcmp( szMachine.get(), "iPhone3,2" ))    return "Other Carrier";
		if( 0 == strcmp( szMachine.get(), "iPhone3,3" ))    return "Verizon";
		if( 0 == strcmp( szMachine.get(), "iPhone4,1" ))    return "AT&T";
		if( 0 == strcmp( szMachine.get(), "iPhone4,2" ))    return "Verizon";
		if( 0 == strcmp( szMachine.get(), "iPhone5,1" ))    return "GSM";
		if( 0 == strcmp( szMachine.get(), "iPhone5,2" ))    return "GSM+CDMA";
		if( 0 == strcmp( szMachine.get(), "iPad1,1" ))      return "Wifi";
		if( 0 == strcmp( szMachine.get(), "iPad1,2" ))      return "3G";
		if( 0 == strcmp( szMachine.get(), "iPad2,1" ))      return "Wifi";
		if( 0 == strcmp( szMachine.get(), "iPad2,2" ))      return "GSM";
		if( 0 == strcmp( szMachine.get(), "iPad2,3" ))      return "CDMA";
		if( 0 == strcmp( szMachine.get(), "iPad2,4" ))      return "Wifi";
		if( 0 == strcmp( szMachine.get(), "iPad2,5" ))      return "Wifi";
		if( 0 == strcmp( szMachine.get(), "iPad2,6" ))      return "GSM";
		if( 0 == strcmp( szMachine.get(), "iPad2,7" ))      return "GSM+CDMA";
		if( 0 == strcmp( szMachine.get(), "iPad3,1" ))      return "Wifi";
		if( 0 == strcmp( szMachine.get(), "iPad3,2" ))      return "GSM+CDMA";
		if( 0 == strcmp( szMachine.get(), "iPad3,3" ))      return "GSM";
		if( 0 == strcmp( szMachine.get(), "iPad3,4" ))      return "Wifi";
		if( 0 == strcmp( szMachine.get(), "iPad3,5" ))      return "GSM";
		if( 0 == strcmp( szMachine.get(), "iPad3,6" ))      return "GSM+CDMA";

		return "";
#else
		return "";
#endif
	}

#if !TARGET_OS_MAC
	std::string getOSVersion()
	{
		return "(Unknown OS version)";	// TODO
	}
#endif

	unsigned int getDeviceIPAddress()
	{
#if TARGET_OS_MAC
		// See http://stackoverflow.com/questions/6807788/how-to-get-ip-address-of-iphone-programatically

		// Retrieve the current interfaces - returns 0 on success
		ifaddrs* interfaces = NULL;
		int success = getifaddrs( &interfaces );

		if( success == 0 )
		{
			unsigned int address = 0;

			// Loop through linked list of interfaces
			ifaddrs* interface = interfaces;
			while( interface != NULL )
			{
				if( interface->ifa_addr->sa_family == AF_INET)
				{
					// Check if interface is en0 which is the wifi connection on the iPhone
					if( 0 == strcmp( interface->ifa_name, "en0" ))
					{
						address = reinterpret_cast< sockaddr_in* >( interface->ifa_addr )->sin_addr.s_addr;
						break;
					}
				}

				interface = interface->ifa_next;
			}

			// Free memory
			freeifaddrs( interfaces );
			return address;
		}

		return 0;
#else
		return 0;	// TODO
#endif
	}

	std::string getStringIPAddress( unsigned int address )
	{
		std::ostringstream stream;
		for( unsigned long i = 0; i < sizeof( address ); ++i )
		{
			if( i > 0 )
			{
				stream << ".";
			}

			stream << ( address & 0xFF );
			address >>= 8;	// Next byte.
		}
		return stream.str();
	}

	std::string getDeviceId()
	{
		std::string device;

#if TARGET_OS_IPHONE

		// Try to read the per-device, per-application key that I have previously created.
		//
		const std::string key = "UniqueID";
		bool found = loadPreference( key, device );
		if( !found )
		{
			// Haven't previously created it. So create it now and store it.
			//
			CFUUIDRef uuidRef = CFUUIDCreate( NULL );
			CFStringRef ident = CFUUIDCreateString( NULL, uuidRef );
			CFRelease( uuidRef );

			savePreference( key, stringFromCFString( ident ).c_str() );
			syncPreferences();

			device = stringFromCFString( ident );

			CFRelease( ident );
		}


#elif TARGET_OS_MAC

		io_registry_entry_t ioRegistryRoot = IORegistryEntryFromPath( kIOMasterPortDefault, "IOService:/");
		CFStringRef uuidCf = (CFStringRef) IORegistryEntryCreateCFProperty( ioRegistryRoot, CFSTR( kIOPlatformUUIDKey ), kCFAllocatorDefault, 0 );
		IOObjectRelease( ioRegistryRoot );

		device = stringFromCFString( uuidCf );
		CFRelease( uuidCf );

#elif defined( _WIN32 )

		// Get the MAC address for this system.
		//
		// TODO from http://wxwidgets.info/cross-platform-way-of-obtaining-mac-address-of-your-machine/

#if 0	// Requires special link library. Too much trouble for now.
		UUID uuid;
		if( ::UuidCreateSequential( &uuid ) != RPC_S_UUID_NO_ADDRESS )
		{
			std::copy( uuid.Data4 + 2, uuid.Data4 + 8, std::back_inserter( device ));
		}
		else
		{
			device = "<unknown MAC>";
		}
#endif

		// Add the computer network name.
		//
		const DWORD lenName = MAX_COMPUTERNAME_LENGTH + 1;
		TCHAR szComputerName[ lenName ];

		DWORD outLen = lenName;
		bool success = ::GetComputerName( szComputerName, &outLen );

		if( success )
		{
			device += szComputerName;
		}
		else
		{
			device = "<unknown Windows device>";
		}
#elif defined( __linux__ )
		// TODO
		device = "<unknown Linux device>";
#endif

		return device;
	}

	void getline( std::istream& stream, std::string& out, const std::string& delimiters )
	{
		while( stream )
		{
			const auto c = stream.peek();
			if( c == std::char_traits< char >::eof() || delimiters.find( c ) != std::string::npos )
			{
				break;
			}
			else
			{
				stream.get();
				if( c == '\n' )
				{
					break;
				}
				out += c;
			}
		}
	}

	bool matchesFilter( const std::string& str, const std::string& filter )
	{
		return std::regex_match( str, std::regex( filter ) );
	}

	std::string toLower( const std::string& s )
	{
		std::string result;
		std::transform( s.begin(), s.end(), std::back_inserter( result ), &::tolower );
		return result;
	}

	std::string toUpper( const std::string& s )
	{
		std::string result;
		std::transform( s.begin(), s.end(), std::back_inserter( result ), &::toupper );
		return result;
	}

	void replaceAll( std::string& within, const std::string& replaced, const std::string& with )
	{
		const auto replacedSize = replaced.size();
		const auto withSize = with.size();

		size_t start = 0;

		while( start < within.size() )
		{
			const auto pos = within.find( replaced, start );

			if( pos != std::string::npos )
			{
				within.replace( pos, replacedSize, with );
				start = pos + withSize;
			}
			else
			{
				break;
			}
		}
	}

	bool stringCaseInsensitiveCompare( const char* str1, const char* str2 )
	{
#if defined( _WIN32 )
		return _stricmp( str1, str2 ) == 0;
#else
		return strcasecmp( str1, str2 ) == 0;
#endif
	}

	bool stringCaseInsensitiveCompare( const std::string& str1, const std::string& str2 )
	{
		return stringCaseInsensitiveCompare( str1.c_str(), str2.c_str() );
	}

	size_t stringCaseInsensitiveFind( const std::string& str1, const std::string& str2 )
	{
		auto pos = std::search( str1.begin(), str1.end(), str2.begin(), str2.end(), CompareCharactersInsensitive );

		if( pos == str1.end() )
		{
			return std::string::npos;
		}
		else
		{
			return pos - str1.begin();
		}
	}

	std::vector< std::string > split( const std::string& str, const std::string& delims )
	{
		std::vector< std::string > container;

		std::size_t current, previous = 0;
		current = str.find_first_of( delims );
		while( current != std::string::npos )
		{
			container.push_back( str.substr(previous, current - previous ));
			previous = current + 1;
			current = str.find_first_of(delims, previous);
		}
		container.push_back( str.substr( previous, current - previous ));

		return container;
	}

	std::string toTraditionalLetters( int thisIndex )
	{
		assert( thisIndex >= 0 );

		std::string label;
		int places = 0;							// 677
		do
		{
			if( ++places > 1 )
			{
				--thisIndex;					// 25
			}

			int thisDigit = thisIndex % 26;		// 25 % 26 == 25

			thisIndex /= 26;					// 0

			std::string t;
			t = 'A' + thisDigit;
			label = t + label;			// 'BZ'
		} while( thisIndex > 0 );

		return label;
	}

#if TARGET_OS_MAC
	std::string stringFromCFString( CFStringRef cfStr )
	{
		std::string value;

		if( cfStr )
		{

			// Try to get the buffer the fast and cheap way.
			//
			const char* szValue = CFStringGetCStringPtr( cfStr, kCFStringEncodingUTF8 );

			if( szValue )
			{
				value = szValue;
			}
			else
			{
				// Didn't work. Copy it the slow and tedious way.
				//
				const CFIndex length = CFStringGetLength( cfStr );
				const CFIndex maxSize = CFStringGetMaximumSizeForEncoding( length, kCFStringEncodingUTF8 );

				std::unique_ptr< char[] > buf( new char[ maxSize ]);

				VERIFY( CFStringGetCString( cfStr, buf.get(), maxSize, kCFStringEncodingUTF8 ));

				value = buf.get();
			}
		}

		return value;
	}
#endif

}
