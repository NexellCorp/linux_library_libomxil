LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
	NX_OMXCore.c

LOCAL_C_INCLUDES += \
	$(TOP)/system/core/include \
	$(TOP)/hardware/samsung_slsi/slsiap/omx/include \
	$(TOP)/hardware/samsung_slsi/slsiap/omx/core/inc \
	$(TOP)/frameworks/base/include \
	$(NX_OMX_INCLUDES)

LOCAL_SHARED_LIBRARIES := \
	libNX_OMX_Common\
	libutils \
	libdl \
	liblog

LOCAL_CFLAGS := $(NX_OMX_CFLAGS)

LOCAL_CFLAGS += -DNO_OPENCORE

ifeq ($(EN_FFMPEG_AUDIO_DEC),true)
LOCAL_CFLAGS += -DUSE_AUDIO_COMPONENT=1
endif

LOCAL_MODULE:= libNX_OMX_Core

include $(BUILD_SHARED_LIBRARY)
