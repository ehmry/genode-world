content: include lib/symbols/libmad LICENSE

PORT_DIR := $(call port_dir,$(REP_DIR)/ports/mad)

include:
	mkdir $@
	cp -r $(PORT_DIR)/include/mad/* $@/

lib/symbols/libmad:
	$(mirror_from_rep_dir)

LICENSE:
	cp $(PORT_DIR)/src/lib/mad/COPYING $@

