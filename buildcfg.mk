#########################################################################
# Embedded Linux Build Enviornment:
#
#########################################################################
OBJTREE		:= $(if $(BUILD_DIR),$(BUILD_DIR),$(CURDIR))

ARCHNAME   	:= S5P6818
ifeq ($(NATIVE_BUILD), yes)
CROSSNAME	:=
else
CROSSNAME	:= aarch64-linux-gnu-
endif

ifneq ($(verbose),1)
	quiet	:= @
endif

INTERACTIVE := $(shell [ -t 0 ] && echo 1)
ifdef INTERACTIVE
# Light Color
	ColorRed=\033[0;91m
	ColorGreen=\033[0;92m
	ColorYellow=\033[0;93m
	ColorBlue=\033[0;93m
	ColorMagenta=\033[0;95m
	ColorCyan=\033[0;96m
	ColorEnd=\033[0m
# Dark Color
	# ColorRed=\033[0;31m
	# ColorGreen=\033[0;32m
	# ColorYellow=\033[0;33m
	# ColorBlue=\033[0;33m
	# ColorMagenta=\033[0;35m
	# ColorCyan=\033[0;36m
	# ColorEnd=\033[0m
else
	ColorRed=
	ColorGreen=
	ColorYellow=
	ColorBlue=
	ColorMagenta=
	ColorCyan=
	ColorEnd=
endif

#########################################################################
#	Toolchain.
#########################################################################
CROSS 	 	:= $(CROSSNAME)
CC 		 	:= $(CROSS)gcc
CPP		 	:= $(CROSS)g++
AR 		 	:= $(CROSS)ar
AS			:= $(CROSS)as
LD 		 	:= $(CROSS)ld
NM 		 	:= $(CROSS)nm
RANLIB 	 	:= $(CROSS)ranlib
OBJCOPY	 	:= $(CROSS)objcopy
STRIP	 	:= $(CROSS)strip

#########################################################################
#	Library & Header macro
#########################################################################
INCLUDE   	:=

#########################################################################
# 	Build Options
#########################################################################
OPTS		:= -Wall -O2 -Wextra -Wcast-align -Wno-unused-parameter -Wshadow -Wwrite-strings -Wcast-qual -fno-strict-aliasing -fstrict-overflow -fsigned-char -fno-omit-frame-pointer -fno-optimize-sibling-calls
COPTS 		:= $(OPTS)
CPPOPTS 	:= $(OPTS) -Wnon-virtual-dtor

CFLAGS 	 	:= $(COPTS)
CPPFLAGS 	:= $(CPPOPTS)
AFLAGS 		:=

ARFLAGS		:= crv
LDFLAGS  	:=
LIBRARY		:=

#########################################################################
# 	Generic Rules
#########################################################################
%.o: %.c
	@echo "Compiling : $(CC) $(ColorCyan)$(notdir $<)$(ColorEnd)"
	$(quiet)$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

%.o: %.s
	@echo "Compiling : $(AS) $(ColorCyan)$(notdir $<)$(ColorEnd)"
	$(quiet)$(AS) $(AFLAGS) $(INCLUDE) -c -o $@ $<

%.o: %.cpp
	@echo "Compiling : $(CPP) $(ColorCyan)$(notdir $<)$(ColorEnd)"
	$(quiet)$(CPP) $(CPPFLAGS) $(INCLUDE) -c -o $@ $<
