# Include this file from game-specific Android.mk files, with FRESH_PATH, BASE_PATH, LOCAL_PATH configured.

LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES += $(FRESH_PATH)/lib/tinyxml_src
LOCAL_C_INCLUDES += $(FRESH_PATH)/FreshCore
LOCAL_C_INCLUDES += $(FRESH_PATH)/FreshPlatform
LOCAL_C_INCLUDES += $(FRESH_PATH)/FreshPlatform/Platforms
LOCAL_C_INCLUDES += $(FRESH_PATH)/FreshScene2D
LOCAL_C_INCLUDES += $(FRESH_PATH)/FreshGame
LOCAL_C_INCLUDES += $(FRESH_PATH)/FreshScript
LOCAL_C_INCLUDES += $(BASE_PATH)/src
LOCAL_C_INCLUDES += $(FRESH_PATH)/lib/freealut/include
LOCAL_C_INCLUDES += $(FRESH_PATH)/lib/lodepng
LOCAL_C_INCLUDES += $(FRESH_PATH)/lib/lua

FILE_LIST := $(wildcard $(FRESH_PATH)/lib/tinyxml_src/*.cpp)
FILE_LIST += $(wildcard $(FRESH_PATH)/FreshCore/*.cpp)
FILE_LIST += $(wildcard $(FRESH_PATH)/FreshCore/Platforms/Android/*.cpp)
FILE_LIST += $(FRESH_PATH)/FreshCore/Platforms/Unix/TelnetServer_Unix.cpp
FILE_LIST += $(wildcard $(FRESH_PATH)/FreshPlatform/*.cpp)
FILE_LIST += $(wildcard $(FRESH_PATH)/FreshPlatform/Platforms/Android/*.cpp)
FILE_LIST += $(FRESH_PATH)/FreshPlatform/Platforms/glESHelpers.cpp
FILE_LIST += $(FRESH_PATH)/FreshPlatform/Platforms/FreshGameCenter_Null.cpp
FILE_LIST += $(FRESH_PATH)/FreshPlatform/Platforms/FreshSocial_Null.cpp
FILE_LIST += $(FRESH_PATH)/FreshPlatform/Platforms/FreshLicensing_Null.cpp
FILE_LIST += $(FRESH_PATH)/FreshPlatform/Platforms/FreshAnalytics_Null.cpp
FILE_LIST += $(FRESH_PATH)/FreshPlatform/Platforms/AudioSession_Null.cpp
FILE_LIST += $(FRESH_PATH)/FreshPlatform/Platforms/Gamepad_Null.cpp
FILE_LIST += $(FRESH_PATH)/FreshPlatform/Platforms/ImageLoader_Generic.cpp
FILE_LIST += $(wildcard $(FRESH_PATH)/FreshScene2D/*.cpp)
FILE_LIST += $(wildcard $(FRESH_PATH)/FreshGame/*.cpp)
FILE_LIST += $(wildcard $(FRESH_PATH)/FreshScript/*.cpp)
FILE_LIST += $(wildcard $(FRESH_PATH)/lib/freealut/src/*.cpp)
FILE_LIST += $(wildcard $(BASE_PATH)/src/*.cpp)
FILE_LIST += $(wildcard $(FRESH_PATH)/lib/lua/lua/*.c)
FILE_LIST += $(FRESH_PATH)/lib/lodepng/lodepng.cpp

# OpenAL directly embedded

OPENAL_BASE := $(FRESH_PATH)/lib/OpenAL

LOCAL_CONLYFLAGS += -std=c99
LOCAL_CFLAGS     += -DAL_BUILD_LIBRARY -DAL_ALEXT_PROTOTYPES

LOCAL_C_INCLUDES += $(OPENAL_BASE)/OpenAL32 $(OPENAL_BASE)/OpenAL32/Include $(OPENAL_BASE)/include $(OPENAL_BASE)/Alc
FILE_LIST  += 	\
		$(OPENAL_BASE)/Alc/ALc.c 	\
		$(OPENAL_BASE)/Alc/ALu.c 	\
		$(OPENAL_BASE)/Alc/alcConfig.c 	\
		$(OPENAL_BASE)/Alc/alcRing.c 	\
		$(OPENAL_BASE)/Alc/bs2b.c 	\
		$(OPENAL_BASE)/Alc/helpers.c 	\
		$(OPENAL_BASE)/Alc/hrtf.c 	\
		$(OPENAL_BASE)/Alc/mixer.c 	\
		$(OPENAL_BASE)/Alc/mixer_c.c 	\
		$(OPENAL_BASE)/Alc/mixer_neon.c 	\
		$(OPENAL_BASE)/Alc/panning.c 	\
		$(OPENAL_BASE)/OpenAL32/alAuxEffectSlot.c 	\
		$(OPENAL_BASE)/OpenAL32/alBuffer.c 	\
		$(OPENAL_BASE)/OpenAL32/alEffect.c 	\
		$(OPENAL_BASE)/OpenAL32/alError.c 	\
		$(OPENAL_BASE)/OpenAL32/alExtension.c 	\
		$(OPENAL_BASE)/OpenAL32/alFilter.c 	\
		$(OPENAL_BASE)/OpenAL32/alFontsound.c 	\
		$(OPENAL_BASE)/OpenAL32/alListener.c 	\
		$(OPENAL_BASE)/OpenAL32/alMidi.c 	\
		$(OPENAL_BASE)/OpenAL32/alPreset.c 	\
		$(OPENAL_BASE)/OpenAL32/alSoundfont.c 	\
		$(OPENAL_BASE)/OpenAL32/alSource.c 	\
		$(OPENAL_BASE)/OpenAL32/alState.c 	\
		$(OPENAL_BASE)/OpenAL32/alThunk.c 	\
		$(OPENAL_BASE)/OpenAL32/sample_cvt.c 	\
		$(OPENAL_BASE)/Alc/backends/base.c 	\
		$(OPENAL_BASE)/Alc/backends/loopback.c 	\
		$(OPENAL_BASE)/Alc/backends/null.c 	\
		$(OPENAL_BASE)/Alc/backends/opensl.c 	\
		$(wildcard $(OPENAL_BASE)/common/*.c) 	\
		$(wildcard $(OPENAL_BASE)/Alc/effects/*.c) 	\
		$(wildcard $(OPENAL_BASE)/Alc/midi/*.c) 	\

# libmpg123 directly embedded

LIBMPG123_BASE := $(FRESH_PATH)/lib/mpg123

LOCAL_C_INCLUDES 	+= $(LIBMPG123_BASE)

FILE_LIST += 	$(LIBMPG123_BASE)/compat.c \
	$(LIBMPG123_BASE)/parse.c \
	$(LIBMPG123_BASE)/frame.c \
	$(LIBMPG123_BASE)/format.c \
	$(LIBMPG123_BASE)/dct64.c \
	$(LIBMPG123_BASE)/equalizer.c \
	$(LIBMPG123_BASE)/id3.c \
	$(LIBMPG123_BASE)/icy.c \
	$(LIBMPG123_BASE)/icy2utf8.c \
	$(LIBMPG123_BASE)/optimize.c \
	$(LIBMPG123_BASE)/readers.c \
	$(LIBMPG123_BASE)/tabinit.c \
	$(LIBMPG123_BASE)/libmpg123.c \
	$(LIBMPG123_BASE)/index.c \
	$(LIBMPG123_BASE)/layer1.c \
	$(LIBMPG123_BASE)/layer2.c \
	$(LIBMPG123_BASE)/layer3.c \
	$(LIBMPG123_BASE)/dither.c \
	$(LIBMPG123_BASE)/feature.c \
	$(LIBMPG123_BASE)/synth.c \
	$(LIBMPG123_BASE)/ntom.c \
	$(LIBMPG123_BASE)/synth_8bit.c \
	$(LIBMPG123_BASE)/stringbuf.c

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
	LOCAL_CFLAGS += -mfloat-abi=softfp -mfpu=neon -DOPT_NEON -DREAL_IS_FLOAT
	FILE_LIST += 	$(LIBMPG123_BASE)/synth_neon.S \
						$(LIBMPG123_BASE)/synth_stereo_neon.S \
						$(LIBMPG123_BASE)/dct64_neon.S
else
	LOCAL_CFLAGS += -DOPT_ARM -DREAL_IS_FIXED
	FILE_LIST += $(LIBMPG123_BASE)/synth_arm.S
endif 

# Collect all sources and conclude building.

LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)
LOCAL_LDLIBS    := -lm -llog -landroid -lGLESv2 -lEGL -lOpenSLES
LOCAL_STATIC_LIBRARIES := android_native_app_glue

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
