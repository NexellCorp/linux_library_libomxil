#
#	libNX_OMX_VIDEO_DECODER.so
#

######################################################################

#
#	Target Information
#
LIBNAME := libNX_OMX_VIDEO_DECODER
TARGET  := $(LIBNAME).so

#	Include Common Build Enviroments
include ../../../buildcfg.mk

#	Install Path
INSTALL_PATH := ../../../lib

#	Sources
COBJS  	:= \
		NX_OMXVideoDecoder.o	\
		NX_DecoderUtil.o		\
		NX_AVCDecoder.o			\
		NX_AVCUtil.o			\
		NX_MPEG4Decoder.o		\
		NX_MPEG2Decoder.o		\
		NX_Div3Decoder.o		\
		NX_RVDecoder.o			\
		NX_VC1Decoder.o			\
		NX_VP8Decoder.o

CPPOBJS	:=
OBJS	:= $(COBJS) $(CPPOBJS)

#	Include Path
INCLUDE += \
		-I./					\
		-I../../../include		\
		-I../../../core/inc		\
		-I../../../codec/libnx_video_api			\
		-I../../../codec/libnx_video_api/nx_video_alloc	\
		-I../../base

#	Add dependent libraries
LIBRARY += \
		-L$(INSTALL_PATH) -lNX_OMX_Common -lNX_OMX_Base -lnx_video_api

#	Compile Options
CFLAGS	+= -fPIC

all: $(TARGET) install

$(TARGET):	depend $(OBJS)
	$(quiet)$(CC) $(LDFLAGS) -shared -Wl,-soname,$(SONAME) -o $@ $(OBJS) $(LIBRARY)

install:
	install -m 755 -d $(INSTALL_PATH)
	install -m 644 $(TARGET) $(INSTALL_PATH)

clean:
	@echo "$(ColorMagenta)[[[ Clean $(LIBNAME) ]]]$(ColorEnd)"
	rm -f $(COBJS) $(CPPOBJS) $(TARGET) .depend

distclean: clean
	rm -f $(INSTALL_PATH)/$(TARGET)

#########################################################################
# Dependency
ifeq (.depend,$(wildcard .depend))
include .depend
endif

SRCS := $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp)
INCS := $(INCLUDE)
depend dep:
	@echo "$(ColorMagenta)[[[ Build $(LIBNAME) ]]]$(ColorEnd)"
	$(quiet)$(CC) -M $(CFLAGS) $(INCS) $(SRCS) > .depend
