TOXCORE_PORT_DIR := $(call select_from_ports,c-toxcore)

TOXCORE_SRC_DIR = $(TOXCORE_PORT_DIR)/src/lib/c-toxcore

#CC_C_OPT += -std=c99 -D_XOPEN_SOURCE

INC_DIR += $(TOXCORE_SRC_DIR)/toxav

TOXCORE_SRC_C := \
	toxav/rtp.c \
	toxav/msi.c  \
	toxav/groupav.c \
	toxav/audio.c \
	toxav/video.c \
	toxav/bwcontroller.c \
	toxav/ring_buffer.c \
	toxav/toxav.c \

vpath %.c  $(TOXCORE_SRC_DIR)

SRC_C  += $(TOXCORE_SRC_C)

LIBS += opus vpx libc

SHARED_LIB = yes
