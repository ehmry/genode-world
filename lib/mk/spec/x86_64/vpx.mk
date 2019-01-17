ifeq ($(shell which yasm),)
REQUIRES += installation_of_yasm
endif

VPX_PORT_DIR := $(call select_from_ports,libvpx)
VPX_SRC_DIR := $(VPX_PORT_DIR)/src/lib/libvpx

LIBS += libc

SHARED_LIB = yes

LD_OPT  += --version-script=libvpx.ver

CC_OPT +=  -msse3 -msse4
CC_OPT +=  -mavx512bw -mavx512dq

CC_OLEVEL = -O3

INC_DIR += $(REP_DIR)/src/lib/libvpx
INC_DIR += $(VPX_SRC_DIR)
INC_DIR += $(VPX_SRC_DIR)/third_party/libyuv/include

vpx.lib.so: libvpx.ver

libvpx.ver: $(REP_DIR)/lib/symbols/vpx
	$(MSG_CONVERT)$@
	$(VERBOSE)echo '{ global:' > $@
	$(VERBOSE)awk '{print $$1 ";"}' <$< >> $@
	$(VERBOSE)echo 'local: *; };' >> $@

vpx_config.asm: $(REP_DIR)/src/lib/libvpx/vpx_config.h
	$(MSG_CONVERT)$@
	$(VERBOSE)egrep "#define [A-Z0-9_]+ [01]" $< | awk '{print $$2 " equ " $$3}' > $@

## 
# File list generated from Linux build.
#
# ../configure  --target=x86_64-linux-gcc --enable-shared --disable-vp9
# make
# find . -name '*.o'

%.o: %.asm
	$(MSG_ASSEM)$@
	$(VERBOSE)yasm -f elf64 -I. -I$(VPX_SRC_DIR) -o $@ $<

vpath %.asm $(VPX_SRC_DIR)
vpath %.c $(VPX_SRC_DIR)

SRC_S += \
	vpx_ports/emms.asm \
	vp8/encoder/x86/encodeopt.asm \
	vp8/encoder/x86/fwalsh_sse2.asm \
	vp8/encoder/x86/dct_sse2.asm \
	vp8/encoder/x86/temporal_filter_apply_sse2.asm \
	vp8/common/x86/idctllm_sse2.asm \
	vp8/common/x86/idctllm_mmx.asm \
	vp8/common/x86/dequantize_mmx.asm \
	vp8/common/x86/loopfilter_sse2.asm \
	vp8/common/x86/recon_sse2.asm \
	vp8/common/x86/recon_mmx.asm \
	vp8/common/x86/iwalsh_sse2.asm \
	vp8/common/x86/subpixel_sse2.asm \
	vp8/common/x86/copy_sse2.asm \
	vp8/common/x86/copy_sse3.asm \
	vp8/common/x86/subpixel_ssse3.asm \
	vp8/common/x86/loopfilter_block_sse2_x86_64.asm \
	vp8/common/x86/subpixel_mmx.asm \
	vp8/common/x86/mfqe_sse2.asm \
	vpx_dsp/x86/deblock_sse2.asm \
	vpx_dsp/x86/sad_sse2.asm \
	vpx_dsp/x86/vpx_subpixel_8t_sse2.asm \
	vpx_dsp/x86/subpel_variance_sse2.asm \
	vpx_dsp/x86/sad_sse3.asm \
	vpx_dsp/x86/vpx_subpixel_bilinear_sse2.asm \
	vpx_dsp/x86/intrapred_sse2.asm \
	vpx_dsp/x86/ssim_opt_x86_64.asm \
	vpx_dsp/x86/add_noise_sse2.asm \
	vpx_dsp/x86/intrapred_ssse3.asm \
	vpx_dsp/x86/vpx_subpixel_bilinear_ssse3.asm \
	vpx_dsp/x86/sad_ssse3.asm \
	vpx_dsp/x86/sad4d_sse2.asm \
	vpx_dsp/x86/vpx_convolve_copy_sse2.asm \
	vpx_dsp/x86/subtract_sse2.asm \
	vpx_dsp/x86/sad_sse4.asm \
	vpx_dsp/x86/vpx_subpixel_8t_ssse3.asm \

SRC_C += \
	md5_utils.c \
	vpx_config.c \
	y4menc.c \
	vpxstats.c \
	vpx_mem/vpx_mem.c \
	ivfenc.c \
	tools_common.c \
	y4minput.c \
	tools/tiny_ssim.c \
	video_writer.c \
	vpx_scale/vpx_scale_rtcd.c \
	vpx_scale/generic/yv12config.c \
	vpx_scale/generic/vpx_scale.c \
	vpx_scale/generic/yv12extend.c \
	vpx_scale/generic/gen_scalers.c \
	vpx/src/vpx_image.c \
	vpx/src/vpx_decoder.c \
	vpx/src/vpx_encoder.c \
	vpx/src/vpx_codec.c \
	ivfdec.c \
	warnings.c \
	video_reader.c \
	vp8/encoder/pickinter.c \
	vp8/encoder/denoising.c \
	vp8/encoder/bitstream.c \
	vp8/encoder/modecosts.c \
	vp8/encoder/encodeframe.c \
	vp8/encoder/mcomp.c \
	vp8/encoder/temporal_filter.c \
	vp8/encoder/onyx_if.c \
	vp8/encoder/encodemv.c \
	vp8/encoder/picklpf.c \
	vp8/encoder/segmentation.c \
	vp8/encoder/boolhuff.c \
	vp8/encoder/tokenize.c \
	vp8/encoder/ethreading.c \
	vp8/encoder/encodeintra.c \
	vp8/encoder/lookahead.c \
	vp8/encoder/firstpass.c \
	vp8/encoder/dct.c \
	vp8/encoder/encodemb.c \
	vp8/encoder/vp8_quantize.c \
	vp8/encoder/treewriter.c \
	vp8/encoder/x86/vp8_quantize_ssse3.c \
	vp8/encoder/x86/denoising_sse2.c \
	vp8/encoder/x86/vp8_quantize_sse2.c \
	vp8/encoder/x86/vp8_enc_stubs_sse2.c \
	vp8/encoder/x86/quantize_sse4.c \
	vp8/encoder/rdopt.c \
	vp8/encoder/ratectrl.c \
	vp8/vp8_dx_iface.c \
	vp8/common/reconintra4x4.c \
	vp8/common/alloccommon.c \
	vp8/common/blockd.c \
	vp8/common/rtcd.c \
	vp8/common/entropymode.c \
	vp8/common/findnearmv.c \
	vp8/common/mfqe.c \
	vp8/common/copy_c.c \
	vp8/common/reconintra.c \
	vp8/common/filter.c \
	vp8/common/entropy.c \
	vp8/common/vp8_loopfilter.c \
	vp8/common/dequantize.c \
	vp8/common/treecoder.c \
	vp8/common/swapyv12buffer.c \
	vp8/common/modecont.c \
	vp8/common/mbpitch.c \
	vp8/common/extend.c \
	vp8/common/idct_blk.c \
	vp8/common/quant_common.c \
	vp8/common/idctllm.c \
	vp8/common/vp8_skin_detection.c \
	vp8/common/entropymv.c \
	vp8/common/x86/filter_x86.c \
	vp8/common/x86/loopfilter_x86.c \
	vp8/common/x86/vp8_asm_stubs.c \
	vp8/common/x86/idct_blk_sse2.c \
	vp8/common/x86/idct_blk_mmx.c \
	vp8/common/generic/systemdependent.c \
	vp8/common/postproc.c \
	vp8/common/loopfilter_filters.c \
	vp8/common/setupintrarecon.c \
	vp8/common/reconinter.c \
	vp8/decoder/decodemv.c \
	vp8/decoder/threading.c \
	vp8/decoder/dboolhuff.c \
	vp8/decoder/decodeframe.c \
	vp8/decoder/detokenize.c \
	vp8/decoder/onyxd_if.c \
	vp8/vp8_cx_iface.c \
	rate_hist.c \
	vpx_util/vpx_thread.c \
	vpx_util/vpx_write_yuv_frame.c \
	vpx_dsp/sad.c \
	vpx_dsp/prob.c \
	vpx_dsp/loopfilter.c \
	vpx_dsp/sum_squares.c \
	vpx_dsp/variance.c \
	vpx_dsp/subtract.c \
	vpx_dsp/bitreader_buffer.c \
	vpx_dsp/add_noise.c \
	vpx_dsp/bitwriter_buffer.c \
	vpx_dsp/deblock.c \
	vpx_dsp/vpx_convolve.c \
	vpx_dsp/skin_detection.c \
	vpx_dsp/psnr.c \
	vpx_dsp/bitwriter.c \
	vpx_dsp/x86/vpx_asm_stubs.c \
	vpx_dsp/x86/vpx_subpixel_8t_intrin_ssse3.c \
	vpx_dsp/x86/vpx_subpixel_8t_intrin_avx2.c \
	vpx_dsp/x86/variance_avx2.c \
	vpx_dsp/x86/sad4d_avx2.c \
	vpx_dsp/x86/loopfilter_avx2.c \
	vpx_dsp/x86/avg_pred_sse2.c \
	vpx_dsp/x86/variance_sse2.c \
	vpx_dsp/x86/loopfilter_sse2.c \
	vpx_dsp/x86/sum_squares_sse2.c \
	vpx_dsp/x86/sad_avx2.c \
	vpx_dsp/x86/sad4d_avx512.c \
	vpx_dsp/vpx_dsp_rtcd.c \
	vpx_dsp/bitreader.c \
	vpx_dsp/intrapred.c \

$(SRC_S): vpx_config.asm
