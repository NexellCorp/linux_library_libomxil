#
#	libnx_video_api.so
#

######################################################################

#
#	Target Information
#
LIBNAME	:= libnx_video_api
TARGET  := libnx_video_api.so

#	Include Common Build Enviroments
include ../../buildcfg.mk

#	Install Path
INSTALL_PATH	:=	../../lib

#	Sources
COBJS	:= \
		nx_video_alloc/nx_video_alloc.o	\
		nx_video_api.o
CPPOBJS	:=  
OBJS	:= $(COBJS) $(CPPOBJS)

#	Include Path
INCLUDE += -I./ -I../../include -I./nx_video_alloc

#	Add dependent libraries
LIBRARY += \
		-L$(INSTALL_PATH) -lstdc++

#	Compile Options
CFLAGS	+= -fPIC

all: $(TARGET) install

$(TARGET):	depend $(OBJS)
	$(quiet)$(AR) $(ARFLAGS) $(LIBNAME).a $(OBJS)
	#$(quiet)$(CC) $(LDFLAGS) -shared -Wl,-soname,$(TARGET) -o $@ $(OBJS) $(LIBRARY)

install:
	@echo "$(ColorMagenta)[[[ Install $(LIBNAME) ]]]$(ColorEnd)"
	install -m 755 -d $(INSTALL_PATH)
	install -m 644 $(LIBNAME).a $(INSTALL_PATH)
	#install -m 644 $(TARGET) $(INSTALL_PATH)

clean:
	@echo "$(ColorMagenta)[[[ Clean $(LIBNAME) ]]]$(ColorEnd)"
	rm -f $(OBJS) $(TARGET) $(LIBNAME).a .depend

distclean: clean
	rm -f $(INSTALL_PATH)/$(LIBNAME).a
	#rm -f $(INSTALL_PATH)/$(TARGET)

#########################################################################
# Dependency
#
ifeq (.depend,$(wildcard .depend))
include .depend
endif

SRCS := $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp)
INCS := $(INCLUDE)
depend dep:
	@echo "$(ColorMagenta)[[[ Build $(LIBNAME) ]]]$(ColorEnd)"
	$(quiet)$(CC) -M $(CFLAGS) $(INCS) $(SRCS) > .depend
