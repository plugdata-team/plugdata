LOCAL_PATH := $(call my-dir)/../..
include $(CLEAR_VARS)

LOCAL_MODULE     := fluidlite
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include $(LOCAL_PATH)/android/include
LOCAL_ARM_MODE   := arm
LOCAL_STATIC_LIBRARIES := vorbis

LOCAL_SRC_FILES := \
	src/fluid_chan.c \
	src/fluid_chorus.c \
	src/fluid_conv.c \
	src/fluid_defsfont.c \
	src/fluid_dsp_float.c \
	src/fluid_gen.c \
	src/fluid_hash.c \
	src/fluid_list.c \
	src/fluid_mod.c \
	src/fluid_ramsfont.c \
	src/fluid_rev.c \
	src/fluid_settings.c \
	src/fluid_synth.c \
	src/fluid_sys.c \
	src/fluid_tuning.c \
	src/fluid_voice.c

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE     := vorbis
LOCAL_C_INCLUDES := $(LOCAL_PATH)/libvorbis-1.3.5/include $(LOCAL_PATH)/android/include
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/libvorbis-1.3.5/include
LOCAL_STATIC_LIBRARIES := ogg
LOCAL_ARM_MODE   := arm
LOCAL_SRC_FILES  := \
	libvorbis-1.3.5/lib/analysis.c \
	libvorbis-1.3.5/lib/bitrate.c \
	libvorbis-1.3.5/lib/block.c \
	libvorbis-1.3.5/lib/codebook.c \
	libvorbis-1.3.5/lib/envelope.c \
	libvorbis-1.3.5/lib/floor0.c \
	libvorbis-1.3.5/lib/floor1.c \
	libvorbis-1.3.5/lib/info.c \
	libvorbis-1.3.5/lib/lookup.c \
	libvorbis-1.3.5/lib/lpc.c \
	libvorbis-1.3.5/lib/lsp.c \
	libvorbis-1.3.5/lib/mapping0.c \
	libvorbis-1.3.5/lib/mdct.c \
	libvorbis-1.3.5/lib/psy.c \
	libvorbis-1.3.5/lib/registry.c \
	libvorbis-1.3.5/lib/res0.c \
	libvorbis-1.3.5/lib/sharedbook.c \
	libvorbis-1.3.5/lib/smallft.c \
	libvorbis-1.3.5/lib/synthesis.c \
	libvorbis-1.3.5/lib/tone.c \
	libvorbis-1.3.5/lib/vorbisfile.c \
	libvorbis-1.3.5/lib/window.c
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE     := ogg
LOCAL_C_INCLUDES := $(LOCAL_PATH)/libogg-1.3.2/include $(LOCAL_PATH)/android/include
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/libogg-1.3.2/include
LOCAL_ARM_MODE   := arm
LOCAL_SRC_FILES  := \
	libogg-1.3.2/src/bitwise.c \
	libogg-1.3.2/src/framing.c
include $(BUILD_STATIC_LIBRARY)
