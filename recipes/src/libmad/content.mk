content: \
	lib/mk/libmad.mk \
	lib/import/import-libmad.mk \
	src/lib/mad \
	LICENSE \

PORT_DIR := $(call port_dir,$(REP_DIR)/ports/mad)

src/lib/mad:
	mkdir -p $@
	cp -r $(PORT_DIR)/src/lib/mad/* $@
	echo "LIBS = libmad" > $@/target.mk

lib/mk/%:
	$(mirror_from_rep_dir)
lib/import/%:
	$(mirror_from_rep_dir)

LICENSE:
	cp $(PORT_DIR)/src/lib/mad/COPYING $@
