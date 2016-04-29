LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false

NX_HW_TOP := $(TOP)/hardware/samsung_slsi/slsiap
NX_HW_INCLUDE := $(NX_HW_TOP)/include
NX_LINUX_TOP := $(TOP)/linux/platform/$(TARGET_CPU_VARIANT2)
NX_LINUX_INCLUDE := $(NX_LINUX_TOP)/library/include

ANDROID_VERSION_STR := $(subst ., ,$(PLATFORM_VERSION))
ANDROID_VERSION_MAJOR := $(firstword $(ANDROID_VERSION_STR))
ifeq "5" "$(ANDROID_VERSION_MAJOR)"
#@echo "This is LOLLIPOP!!!"
LOCAL_CFLAGS += -DLOLLIPOP=1
endif

ifeq ($(TARGET_ARCH),arm64)
LOCAL_CFLAGS += -DARM64=1
else
LOCAL_CFLAGS += -DARM64=0
endif

OMX_TOP := $(NX_HW_TOP)/omx
RATECONTROL_PATH := $(NX_LINUX_TOP)/library/lib/ratecontrol

LOCAL_SRC_FILES:= \
	NX_OMXVideoEncoder.c

LOCAL_C_INCLUDES += \
	$(TOP)/system/core/include \
	$(TOP)/hardware/libhardware/include \
	$(NX_HW_INCLUDE) \
	$(NX_HW_TOP)/nxutil

LOCAL_C_INCLUDES += \
	$(OMX_TOP)/include \
	$(OMX_TOP)/core/inc \
	$(OMX_TOP)/codec/video/coda960 \
	$(OMX_TOP)/components/base \
	$(NX_LINUX_INCLUDE)


LOCAL_SHARED_LIBRARIES := \
	libNX_OMX_Common \
	libNX_OMX_Base \
	libdl \
	liblog \
	libhardware \
	libnx_vpu \
	libion \
	libion-nexell \
	libnxutil

LOCAL_LDFLAGS_arm += -L$(NX_LINUX_TOP)/library/lib -lnxvidrc_android
LOCAL_LDFLAGS_arm64 += -L$(NX_LINUX_TOP)/library/lib/arm64 -lnxvidrc_android

LOCAL_CFLAGS += $(NX_OMX_CFLAGS)
LOCAL_CFLAGS += -DNX_DYNAMIC_COMPONENTS

LOCAL_MODULE := libNX_OMX_VIDEO_ENCODER

include $(BUILD_SHARED_LIBRARY)
