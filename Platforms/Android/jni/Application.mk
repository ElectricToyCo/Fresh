APP_PLATFORM := android-16

APP_ABI := armeabi-v7a
APP_STL := c++_static

APP_CFLAGS += -DANDROID=1

APP_CPPFLAGS := -frtti
APP_CPPFLAGS += -fexceptions
APP_CPPFLAGS += -std=c++14

NDK_TOOLCHAIN_VERSION := clang

ifeq ($(APP_OPTIM),debug)

	## Debug

	APP_CFLAGS += -Og
	APP_CFLAGS += -DDEBUG=1
	APP_CFLAGS += -DDEV_MODE=1

else

	## Release

	APP_CFLAGS += -DNDEBUG -ffast-math -O3 -fomit-frame-pointer -funroll-all-loops -finline-functions

endif
