MAD_SRC_DIR := $(call select_from_ports,mad)/src/lib/mad

LIBS += libc

# TODO: spec and select optimized math
CC_DEF += -DFPM_DEFAULT

SRC_C = \
	version.c \
	fixed.c \
	bit.c \
	timer.c \
	stream.c \
	frame.c  \
	synth.c \
	decoder.c \
	layer12.c \
	layer3.c \
	huffman.c \

vpath %.c $(MAD_SRC_DIR)

SHARED_LIB = yes
