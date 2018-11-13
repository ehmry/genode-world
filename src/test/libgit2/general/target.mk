TARGET = libgit2-examples-general
$(TARGET): testrepo.git.tar

LIBS += libgit2 posix

include $(REP_DIR)/lib/import/import-libgit2.mk
EXAMPLES_DIR := $(LIBGIT_PORT_DIR)/src/lib/libgit2/examples

INC_DIR += $(EXAMPLES_DIR)

SRC_C += general.c
vpath %.c $(EXAMPLES_DIR)

testrepo.git.tar: $(LIBGIT_PORT_DIR)/src/lib/libgit2/tests/resources/testrepo.git
	$(MSG_ASSEM)$@
	$(VERBOSE) tar cf $@ -C $< .
