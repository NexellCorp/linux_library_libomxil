LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false

ANDROID_VERSION_STR := $(subst ., ,$(PLATFORM_VERSION))
ANDROID_VERSION_MAJOR := $(firstword $(ANDROID_VERSION_STR))
ifeq "5" "$(ANDROID_VERSION_MAJOR)"
# This is LOLLIPOP!!!
LOCAL_CFLAGS += -DLOLLIPOP=1
endif

ifeq ($(TARGET_ARCH),arm64)
LOCAL_CFLAGS += -DARM64=1
else
LOCAL_CFLAGS += -DARM64=0
endif

NX_HW_TOP := $(TOP)/hardware/samsung_slsi/slsiap
NX_HW_INCLUDE := $(NX_HW_TOP)/include
NX_LINUX_TOP := $(TOP)/linux/platform/$(TARGET_CPU_VARIANT2)
NX_LINUX_INCLUDE := $(NX_LINUX_TOP)/library/include

OMX_TOP := $(NX_HW_TOP)/omx

LOCAL_SRC_FILES:= \
	NX_AVCDecoder.c \
	NX_MPEG2Decoder.c \
	NX_MPEG4Decoder.c \
	NX_Div3Decoder.c \
	NX_RVDecoder.c \
	NX_VC1Decoder.c \
	NX_VP8Decoder.c \
	NX_DecoderUtil.c \
	NX_AVCUtil.c \
	NX_OMXVideoDecoder.c

LOCAL_C_INCLUDES += \
	$(TOP)/system/core/include \
	$(TOP)/hardware/libhardware/include \
	$(NX_HW_INCLUDE)

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
	libcutils \
	libnx_deinterlace
#	libnxgraphictools    

LOCAL_LDFLAGS_arm += -L$(NX_LINUX_TOP)/library/lib -lnxvidrc_android
LOCAL_LDFLAGS_arm64 += -L$(NX_LINUX_TOP)/library/lib/arm64 -lnxvidrc_android

LOCAL_CFLAGS += $(NX_OMX_CFLAGS)
LOCAL_CFLAGS += -DNX_DYNAMIC_COMPONENTS

LOCAL_MODULE:= libNX_OMX_VIDEO_DECODER

include $(BUILD_SHARED_LIBRARY)
