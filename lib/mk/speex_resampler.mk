include $(REP_DIR)/lib/import/import-speex_resampler.mk

LIBS += libc libm
CC_DEF += -DSPX_RESAMPLE_EXPORT= -DRANDOM_PREFIX=genode -DOUTSIDE_SPEEX -DFLOATING_POINT
SRC_C = resample.c

vpath %.c  $(REP_DIR)/src/lib/speex_resampler
vpath %.cc $(REP_DIR)/src/lib/speex_resampler
