INC_DIR += $(call select_from_repositories,include/speex_resampler)
CC_OPT += -DSPX_RESAMPLE_EXPORT= -DRANDOM_PREFIX=genode -DOUTSIDE_SPEEX -DFLOATING_POINT
