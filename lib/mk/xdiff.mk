LIBS += libc

XDIFF_DIR := $(call select_from_ports,libgit2)/src/lib/libgit2/src/xdiff

INC_DIR += $(XDIFF_DIR)
INC_DIR += $(XDIFF_DIR)/..
INC_DIR += $(XDIFF_DIR)/../../include
INC_DIR += $(REP_DIR)/include/libgit2

SRC_C += $(notdir $(wildcard $(XDIFF_DIR)/*.c))
vpath %.c $(XDIFF_DIR)
