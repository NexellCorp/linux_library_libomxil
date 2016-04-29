DIR		:= 
######################################################################
# Build options

# Codec
DIR += libnx_video_api

######################################################################
# Build
all:
	@for dir in $(DIR); do			\
	make -s -C $$dir || exit $?;	\
	done

install:
	@for dir in $(DIR); do			\
	make -C $$dir install || exit $?;	\
	done

clean:
	@for dir in $(DIR); do			\
	make -s -C $$dir clean || exit $?;	\
	done

distclean:
	@for dir in $(DIR); do			\
	make -C $$dir distclean || exit $?;	\
	done

