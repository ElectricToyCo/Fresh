Android Ports
=============

# Useful links

Easy access to headers
http://mobilepearls.com/labs/native-android-api/

Brief outline of using openRawResourceFd() to get file access to uncompressed assets.
http://stackoverflow.com/questions/2651816/reading-resource-files-from-my-own-apk-in-android-native-environment/2663962#2663962

Useful options for saving and loading files.
http://stackoverflow.com/questions/11294487/android-writing-saving-files-from-native-code-only

Game Services

https://developers.google.com/games/services/

# Basic Process (2014-04-22)

From the Mac terminal:

    cd dig/Platforms/Android

Build the native cpp code:

    ndk-build -j8

Build the Android project in debug mode:

    ant debug

Run an emulator:

    android avd&

Then select the Galaxy device and Start... it. Wait for it to run completely, and unlock it.

Install the app (dig-debug.apk) onto the device:

    ant debug install

Any problems here usually indicates a need to restart the emulator. Android stuff is generally broken and unreliable--mere retries and restarts often resolve problems.

My assets directory is sent to the device but remains zipped (within the apk). As a temporary workaround, let's manually send the assets over:

    cd ../..
    adb push assets/ /data/data/co.electrictoy.dig/files/assets/

Now click over to the emulator and run "Doug dug."

While developing, makefile-style configuration changes live in Application.mk and Android.mk. See ndk docs for details on which file is which and how to do things generally. These .mk files serve the ndk-build native make system. "ant" is configured with build.xml, which seldom needs to be touched, but can be.

# Install the Android SDK

*	https://developer.android.com/sdk/installing/bundle.html

Try a first app:

*	https://developer.android.com/training/basics/firstapp/index.html

Install the Android NDK (for native code, i.e. C++)

*	See https://developer.android.com/tools/sdk/ndk/index.html#Installing


# The way things are:

You've got an Android Application complete with a manifest, java source for the essentials, etc.

You create a jni/ folder with native sources and a .mk makefile.

"android update project --name dig --target 1 --path ."

Build the native stuff using "ndk-build", which makes a library (lib*.so).

You then build the app itself, including java and whatnot, using "ant debug". This creates blah-debug.apk in your bin/ directory.

To run, "android avd&" and Start... a virtual machine. Wait a minute or so.

cd bin and adb install blah-debug.apk to install on the virtual machine. Will fail if the machine isn't ready yet.

Or: ant debug install.

Find the app on the emulated device and run it!



# Questions

## How will my main()/Application class/periodic draw call fit in with the Java underbelly?

Java Activity -> my main() (maybe called something else) -> constructs Application, sets up config, etc. 

Application::runMainLoop() either does nothing or somehow connects with Java. If does nothing, the java update loop calls over to Application::update(). I think the iOS version does something like this.

## Resources? i.e. assets/config.xml?

## Documents folder?

Input?


# State

There is a complex of problems:

*   Building using CMake and make rather than ndk-build and jni/blah.mk. See /Users/Jeff/Downloads/android-cmake-master/samples/hello-gl2
*   Using a pure-native implementation rather than Java. See /Users/Jeff/Projects/android/DroidBlaster


I have no basic aversion to using ndk-build as opposed to CMake, except that I'm weirded out by having to put everything into the jni/ directory, and I'll have to make a .mk file, which might be complex. CMake would be nicer, but it does add at least one layer of complexity to everything; and anyway it just isn't working right now.

Let's try the jni approach.


# CURRENT TODO

ndk-build approach is working pretty nicely. TODO:

*   Document my favorite console commands:
    
    Tab 1) adb push 'assets/' '/data/data/co.electrictoy.dig/files/assets/'

    Tab 2)  a) emulator @AVD_for_Galaxy_Nexus_by_Google &
            b) adb kill-server && adb start-server && adb shell logcat

    Tab 3) ndk-build -j8 && ant debug install

*   Implement OpenAL, or sound anyway.
*   Upload config-Android.xml.
*   Get window actually created and rendering.
*   Input, i.e. touch
*   Telemetry. Need a libcurl implementation on Android for this.
*   glESHelpers.* really needs to be somewhere else... not in FreshPlatform/Platforms/iOS. We're reaching across for Android.
*   Remove thumb support.
*   Remove redundant Android/src directory (ant debug complains).
*   ASSERT() is dysfunctional


License:
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAhI/QkhQQleecGhigBPZKSeDDk/nKNhHrqA8atA9alIuY1Zz3StFPHdrxu8GaMSpzZH8fyZiDpvZjPowilWHcRKLGWxsyRBIlsCK8mRT1nyjOcbXb9a+Vo3hrz7817WP3HwindyQWbxvN5QUZ9lMJRPm/Je2MitF05KoDOzZ6hAUjOQmCXNoPMZMQTg/T4HYAqWwsWE4AvWkTqA4bjhy6MroT79FGP/6B4Jk8LKXaxiA0Ss0iRBSpWmP6EZ1IT32CnfulOgkPAqTxN1K3NklJALqA1fpN2BDPgCKP8QmKTlZjMza0jdiOqTeqldfDAWoKPycsaFMa9W9TtQHiZpc3uQIDAQAB