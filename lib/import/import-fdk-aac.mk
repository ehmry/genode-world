FDK_AAC_PORT_DIR := $(call select_from_ports,fdk-aac)
FDK_AAC_SRC_DIR  := $(FDK_AAC_PORT_DIR)/src/lib/fdk-aac

INC_DIR += $(FDK_AAC_PORT_DIR)/include/fdk-aac
