LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
	NX_OMXDebugMsg.c \
	NX_OMXQueue.c \
	NX_OMXSemaphore.c

LOCAL_C_INCLUDES += \
	$(TOP)/system/core/include \
	$(TOP)/hardware/samsung_slsi/slsiap/omx/include \
	$(NX_OMX_INCLUDES)

LOCAL_SHARED_LIBRARIES := \
	libdl \
	libutils \
	liblog
	
LOCAL_CFLAGS := $(NX_OMX_CFLAGS)

LOCAL_CFLAGS += -DNO_OPENCORE

LOCAL_MODULE:= libNX_OMX_Common

include $(BUILD_SHARED_LIBRARY)
