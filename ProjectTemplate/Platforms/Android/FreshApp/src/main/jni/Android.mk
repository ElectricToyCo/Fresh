LOCAL_PATH := $(realpath $(call my-dir))
BASE_PATH := $(LOCAL_PATH)/../../../../../..
FRESH_PATH := $(BASE_PATH)/Fresh

include $(CLEAR_VARS)

LOCAL_MODULE    := freshshortname

include $(FRESH_PATH)/Platforms/Android/jni/Android.mk
