ifndef	JOBS
JOBJS	:= 8
endif

######################################################################
DIR		:=
# Common Base Library
DIR += base

# OMX-IL Video Decoder Components
DIR += video/coda960_video_decoder

# OMX-IL Video Encoder Components
DIR += video/coda960_video_encoder
######################################################################

# Build Commands
all:
	@for dir in $(DIR); do			\
	make -C $$dir || exit $?;		\
	done

install:
	@for dir in $(DIR); do			\
	make -C $$dir install || exit $?;	\
	done

clean:
	@for dir in $(DIR); do			\
	make -C $$dir clean || exit $?;	\
	done

distclean:
	@for dir in $(DIR); do			\
	make -C $$dir distclean || exit $?;	\
	done
