SHARED_LIB = 1

LIBS += libc glib libm pthread cairo pango libpng zlib

CC_DEF += -DNUMVERS=1.6999 -DDISABLE_FLOCK

RRD_SRC_DIR = $(call select_from_ports,rrdtool)/src/lib/rrdtool/src

RRD_SRC_C := $(notdir $(wildcard $(RRD_SRC_DIR)/*.c))

FILTER_OUT = \
	rrd_cgi.c \
	rrd_daemon.c \
	rrd_fetch_libdbi.c \
	rrd_rados.c \
	rrd_restore.c \
	rrd_thread_safe_nt.c \
	rrd_tool.c \
	win32comp.c \

SRC_C := $(filter-out $(FILTER_OUT),$(RRD_SRC_C))

vpath %.c $(RRD_SRC_DIR)
