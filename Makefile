ifndef	JOBS
JOBJS	:= 8
endif

include buildcfg.mk

INSTALL_DIR = /home/hcjun/devel/nfs/rootfs-omx/usr/lib/
SRC_FILE	= ../linux-artik7/include/uapi/linux/videodev2_nxp_media.h
DST_FILE	= ./include/linux/videodev2_nxp_media.h


DIR		:=
######################################################################
# Build options

# Codec
DIR += codec

# Common Base Library
DIR += common

# OMX-IL Core
DIR += core

# OMX-IL Components
DIR += components

######################################################################
# Build
all: compare
	@echo "$(ColorMagenta)###############################################$(ColorEnd)"
	@echo "$(ColorMagenta)#                                             #$(ColorEnd)"
	@echo "$(ColorMagenta)#        Build Nexell OMX-IL Libraries        #$(ColorEnd)"
	@echo "$(ColorMagenta)#                                             #$(ColorEnd)"
	@echo "$(ColorMagenta)###############################################$(ColorEnd)"
	@for dir in $(DIR); do			\
	make -s -C $$dir || exit $?;	\
	done

install:
	@for dir in $(DIR); do			\
	make -C $$dir install || exit $?;	\
	done

clean:
	@echo "$(ColorMagenta)###############################################$(ColorEnd)"
	@echo "$(ColorMagenta)#                                             #$(ColorEnd)"
	@echo "$(ColorMagenta)#        Clean Nexell OMX-IL Libraries        #$(ColorEnd)"
	@echo "$(ColorMagenta)#                                             #$(ColorEnd)"
	@echo "$(ColorMagenta)###############################################$(ColorEnd)"
	@for dir in $(DIR); do			\
	make -s -C $$dir clean || exit $?;	\
	done

distclean:
	@for dir in $(DIR); do			\
	make -C $$dir distclean || exit $?;	\
	done

copy:
	@echo "$(ColorMagenta)[[[     Copy Bineries to $(INSTALL_DIR)     ]]]""$(ColorCyan)"
	cp -av lib/*.so $(INSTALL_DIR)
	@echo "$(ColorEnd)"

compare:
	@echo "$(ColorMagenta)###############################################$(ColorEnd)"
	@echo "$(ColorMagenta)#                                             #$(ColorEnd)"
	@echo "$(ColorMagenta)#        Compare Kernel Private Header        #$(ColorEnd)"
	@echo "$(ColorMagenta)#                                             #$(ColorEnd)"
	@echo "$(ColorMagenta)###############################################$(ColorEnd)"
	@cmp -s $(SRC_FILE) $(DST_FILE);		\
	RETVAL=$$?;								\
	if [ $$RETVAL -eq 0 ]; then				\
		echo "Matched.";						\
	else									\
		echo "Update Private Header"; 		\
		cp -av $(SRC_FILE) $(DST_FILE);		\
	fi
	@echo "$(ColorEnd)"
