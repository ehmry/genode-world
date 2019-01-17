MIRROR_FROM_REP_DIR := \
	lib/symbols/vpx \
	lib/import/import-vpx.mk \

content: $(MIRROR_FROM_REP_DIR) include LICENSE

$(MIRROR_FROM_REP_DIR):
	$(mirror_from_rep_dir)

PORT_DIR := $(call port_dir,$(REP_DIR)/ports/libvpx)

include:
	mkdir -p $@
	cp -r $(PORT_DIR)/include/libvpx/* $@/

LICENSE:
	cp $(PORT_DIR)/src/lib/libvpx/LICENSE $@
