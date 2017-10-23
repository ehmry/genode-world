SHARED_LIB = 1

LIBS += glib libc

CC_OPT += \
	-DPANGO_ENABLE_ENGINE -DPANGO_ENABLE_BACKEND \
	-DLIBDIR=\"/\" \
	-DSYSCONFDIR=\"/\" \

PANGO_PORT_DIR := $(call select_from_ports,pango)/src/lib/pango

INC_DIR += $(REP_DIR)/src/lib/pango $(PANGO_PORT_DIR) $(PANGO_PORT_DIR)/pango

PANGO_SRC_C = \
	break.c \
	ellipsize.c \
	fonts.c \
	glyphstring.c \
	modules.c \
	pango-attributes.c \
	pango-bidi-type.c \
	pango-color.c \
	pango-context.c \
	pango-coverage.c \
	pango-engine.c \
	pango-fontmap.c \
	pango-fontset.c \
	pango-glyph-item.c \
	pango-gravity.c \
	pango-item.c \
	pango-language.c \
	pango-layout.c \
	pango-markup.c \
	pango-matrix.c \
	pango-renderer.c \
	pango-script.c \
	pango-tabs.c \
	pango-utils.c \
	reorder-items.c \
	shape.c \
	pango-enum-types.c \

FILTER_OUT = 

SRC_C := $(filter-out $(FILTER_OUT),$(PANGO_SRC_C))

vpath %.c $(PANGO_PORT_DIR)/pango
