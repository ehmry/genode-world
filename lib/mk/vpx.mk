VPX_PORT_DIR := $(call select_from_ports,libvpx)
VPX_SRC_DIR := $(VPX_PORT_DIR)/src/lib/libvpx

LIBS += libc

INC_DIR += $(REP_DIR)/src/lib/libvpx
INC_DIR += $(VPX_SRC_DIR)

VPX_SRC_C = svc_encodeframe.c vpx_codec.c vpx_decoder.c vpx_encoder.c vpx_image.c

SRC_C += $(VPX_SRC_C)

vpath %.c $(VPX_SRC_DIR)/vpx/src

SHARED_LIB = yes
