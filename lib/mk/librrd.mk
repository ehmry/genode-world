LIBS = libc

CC_DEF += -DNUMVERS=1.6999

RRD_SRC_DIR = $(call select_from_ports,rrdtool)/src/lib/rrdtool/src

SRC_C = \
	rrd_version.c	\
	rrd_last.c	\
	rrd_lastupdate.c	\
	rrd_first.c	\
	rrd_dump.c	\
	rrd_flushcached.c \
	rrd_fetch.c	\
	rrd_fetch_cb.c  \
	rrd_resize.c \
	rrd_tune.c	\
	rrd_list.c	\

vpath %.c $(RRD_SRC_DIR)
