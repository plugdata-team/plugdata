LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := fluidlite_shared
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/libfluidlite.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../include
TARGET_PLATFORM = android-10

include $(PREBUILT_SHARED_LIBRARY)
