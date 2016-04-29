#
#	libNX_OMX_Base.so
#

######################################################################

#
#	Target Information
#
LIBNAME := libNX_OMX_Base
TARGET  := $(LIBNAME).so

#	Include Common Build Enviroments
include ../../buildcfg.mk

#	Install Path
INSTALL_PATH := ../../lib

#	Sources
COBJS  	:= \
		NX_OMXBaseComponent.o	\
		NX_OMXBasePort.o		\
		NX_OMXCommon.o
CPPOBJS	:=
OBJS	:= $(COBJS) $(CPPOBJS)

#	Include Path
INCLUDE += -I./ -I../../include -I../../core/inc

#	Add dependent libraries
LIBRARY += \
		-L$(INSTALL_PATH) -l NX_OMX_Common

#	Compile Options
CFLAGS	+= -fPIC

all: $(TARGET) install

$(TARGET):	depend $(OBJS)
	$(quiet)$(CC) $(LDFLAGS) -shared -Wl,-soname,$(SONAME) -o $@ $(OBJS) $(LIBRARY)

install:
	@echo "$(ColorMagenta)[[[ Install $(LIBNAME) ]]]$(ColorEnd)"
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
