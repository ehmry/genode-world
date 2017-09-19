include $(REP_DIR)/lib/import/import-fdk-aac.mk

LIBS += libc

CC_OPT  += -fno-exceptions -fno-rtti
CC_WARN += -Wno-sign-compare

INC_DIR += \
	$(FDK_AAC_SRC_DIR)/libAACdec/include \
	$(FDK_AAC_SRC_DIR)/libAACenc/include \
	$(FDK_AAC_SRC_DIR)/libSBRdec/include \
	$(FDK_AAC_SRC_DIR)/libSBRenc/include \
	$(FDK_AAC_SRC_DIR)/libMpegTPDec/include \
	$(FDK_AAC_SRC_DIR)/libMpegTPEnc/include \
	$(FDK_AAC_SRC_DIR)/libSYS/include \
	$(FDK_AAC_SRC_DIR)/libFDK/include \
	$(FDK_AAC_SRC_DIR)/libPCMutils/include \

find_src = $(notdir $(wildcard $(FDK_AAC_SRC_DIR)/lib$(1)/src/*.cpp))
