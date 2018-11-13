
SHARED_LIB = 1

LIBS += curl libssl xdiff zlib libc

include $(REP_DIR)/lib/import/import-libgit2.mk
LIBGIT_SRC_DIR = $(LIBGIT_PORT_DIR)/src/lib/libgit2/src

INC_DIR += $(LIBGIT_SRC_DIR)
INC_DIR += $(REP_DIR)/include/libgit2
INC_DIR += $(LIBGIT_SRC_DIR)/../deps/http-parser

LIBGIT_SRC_C := $(notdir $(wildcard $(LIBGIT_SRC_DIR)/*.c))
SRC_C += $(LIBGIT_SRC_C)
vpath %.c $(LIBGIT_SRC_DIR)

STREAMS_SRC_C := $(notdir $(wildcard $(LIBGIT_SRC_DIR)/streams/*.c))
STREAMS_SRC_FILTER = stransport.c
SRC_C += $(filter-out $(STREAMS_SRC_FILTER),$(STREAMS_SRC_C))
vpath %.c $(LIBGIT_SRC_DIR)/streams

TRANSPORTS_SRC_C := $(notdir $(wildcard $(LIBGIT_SRC_DIR)/transports/*.c))
SRC_C += $(TRANSPORTS_SRC_C)
vpath %.c $(LIBGIT_SRC_DIR)/transports

HASH_SHA1DC_SRC_C := $(notdir $(wildcard $(LIBGIT_SRC_DIR)/hash/sha1dc/*.c))
SRC_C += $(HASH_SHA1DC_SRC_C)
vpath %.c $(LIBGIT_SRC_DIR)/hash/sha1dc

SRC_C += http_parser.c
vpath %.c $(LIBGIT_SRC_DIR)/../deps/http-parser

SRC_C += realpath.c
vpath %.c $(LIBGIT_SRC_DIR)/unix
