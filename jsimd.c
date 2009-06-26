/*
 * jsimd.c
 *
 * Copyright 2009 Pierre Ossman <ossman@cendio.se> for Cendio AB
 * Copyright 2009 D. R. Commander
 * 
 * Based on the x86 SIMD extension for IJG JPEG library,
 * Copyright (C) 1999-2006, MIYASAKA Masaru.
 *
 * This file contains the interface between the "normal" portions
 * of the library and the SIMD implementations.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jsimd.h"
#include "jdct.h"
#include "jsimddct.h"
#include "simd/jsimd.h"

/*
 * In the PIC cases, we have no guarantee that constants will keep
 * their alignment. This macro allows us to verify it at runtime.
 */
#ifdef WITH_SIMD
#define IS_ALIGNED(ptr, order) (((unsigned)ptr & ((1 << order) - 1)) == 0)
#else
#define IS_ALIGNED(ptr, order) (0)
#endif

#define IS_ALIGNED_SSE(ptr) (IS_ALIGNED(ptr, 4)) /* 16 byte alignment */

static unsigned int simd_support = ~0;

/*
 * Check what SIMD accelerations are supported.
 *
 * FIXME: This code is racy under a multi-threaded environment.
 */
LOCAL(void)
init_simd (void)
{
#ifdef WITH_SIMD
  char *env = NULL;
#endif
  if (simd_support != ~0)
    return;

#ifdef WITH_SIMD
  simd_support = jpeg_simd_cpu_support();
  #ifndef __x86_64__
  if((env=getenv("JSIMD_FORCEMMX"))!=NULL && !strcmp(env, "1"))
    simd_support = JSIMD_MMX;
  else if((env=getenv("JSIMD_FORCESSE2"))!=NULL && !strcmp(env, "1"))
    simd_support = JSIMD_SSE2;
  #endif
#else
  simd_support = JSIMD_NONE;
#endif
}

GLOBAL(int)
jsimd_can_rgb_ycc (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if ((RGB_PIXELSIZE != 3) && (RGB_PIXELSIZE != 4))
    return 0;

  if ((simd_support & JSIMD_SSE2) &&
      IS_ALIGNED_SSE(jconst_rgb_ycc_convert_sse2))
    return 1;
  if (simd_support & JSIMD_MMX)
    return 1;

  return 0;
}

GLOBAL(int)
jsimd_can_ycc_rgb (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if ((RGB_PIXELSIZE != 3) && (RGB_PIXELSIZE != 4))
    return 0;

  if ((simd_support & JSIMD_SSE2) &&
      IS_ALIGNED_SSE(jconst_ycc_rgb_convert_sse2))
    return 1;
  if (simd_support & JSIMD_MMX)
    return 1;

  return 0;
}

GLOBAL(void)
jsimd_rgb_ycc_convert (j_compress_ptr cinfo,
                       JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
                       JDIMENSION output_row, int num_rows)
{
#ifdef WITH_SIMD
  void (*sse2fct)(JDIMENSION, JSAMPARRAY, JSAMPIMAGE, JDIMENSION, int);
  #ifndef __x86_64__
  void (*mmxfct)(JDIMENSION, JSAMPARRAY, JSAMPIMAGE, JDIMENSION, int);
  #endif
  switch(cinfo->in_color_space)
  {
    case JCS_EXT_RGB:
      sse2fct=jsimd_extrgb_ycc_convert_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_extrgb_ycc_convert_mmx;
      #endif
      break;
    case JCS_EXT_RGBX:
      sse2fct=jsimd_extrgbx_ycc_convert_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_extrgbx_ycc_convert_mmx;
      #endif
      break;
    case JCS_EXT_BGR:
      sse2fct=jsimd_extbgr_ycc_convert_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_extbgr_ycc_convert_mmx;
      #endif
      break;
    case JCS_EXT_BGRX:
      sse2fct=jsimd_extbgrx_ycc_convert_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_extbgrx_ycc_convert_mmx;
      #endif
      break;
    case JCS_EXT_XBGR:
      sse2fct=jsimd_extxbgr_ycc_convert_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_extxbgr_ycc_convert_mmx;
      #endif
      break;
    case JCS_EXT_XRGB:
      sse2fct=jsimd_extxrgb_ycc_convert_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_extxrgb_ycc_convert_mmx;
      #endif
      break;
    default:
      sse2fct=jsimd_rgb_ycc_convert_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_rgb_ycc_convert_mmx;
      #endif
      break;
  }
  if ((simd_support & JSIMD_SSE2) &&
      IS_ALIGNED_SSE(jconst_rgb_ycc_convert_sse2))
    sse2fct(cinfo->image_width, input_buf,
        output_buf, output_row, num_rows);
  #ifndef __x86_64__
  else if (simd_support & JSIMD_MMX)
    mmxfct(cinfo->image_width, input_buf,
        output_buf, output_row, num_rows);
  #endif
#endif
}

GLOBAL(void)
jsimd_ycc_rgb_convert (j_decompress_ptr cinfo,
                       JSAMPIMAGE input_buf, JDIMENSION input_row,
                       JSAMPARRAY output_buf, int num_rows)
{
#ifdef WITH_SIMD
  void (*sse2fct)(JDIMENSION, JSAMPIMAGE, JDIMENSION, JSAMPARRAY, int);
  #ifndef __x86_64__
  void (*mmxfct)(JDIMENSION, JSAMPIMAGE, JDIMENSION, JSAMPARRAY, int);
  #endif
  switch(cinfo->out_color_space)
  {
    case JCS_EXT_RGB:
      sse2fct=jsimd_ycc_extrgb_convert_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_ycc_extrgb_convert_mmx;
      #endif
      break;
    case JCS_EXT_RGBX:
      sse2fct=jsimd_ycc_extrgbx_convert_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_ycc_extrgbx_convert_mmx;
      #endif
      break;
    case JCS_EXT_BGR:
      sse2fct=jsimd_ycc_extbgr_convert_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_ycc_extbgr_convert_mmx;
      #endif
      break;
    case JCS_EXT_BGRX:
      sse2fct=jsimd_ycc_extbgrx_convert_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_ycc_extbgrx_convert_mmx;
      #endif
      break;
    case JCS_EXT_XBGR:
      sse2fct=jsimd_ycc_extxbgr_convert_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_ycc_extxbgr_convert_mmx;
      #endif
      break;
    case JCS_EXT_XRGB:
      sse2fct=jsimd_ycc_extxrgb_convert_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_ycc_extxrgb_convert_mmx;
      #endif
      break;
    default:
      sse2fct=jsimd_ycc_rgb_convert_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_ycc_rgb_convert_mmx;
      #endif
      break;
  }
  if ((simd_support & JSIMD_SSE2) &&
      IS_ALIGNED_SSE(jconst_ycc_rgb_convert_sse2))
    sse2fct(cinfo->output_width, input_buf,
        input_row, output_buf, num_rows);
  #ifndef __x86_64__
  else if (simd_support & JSIMD_MMX)
    mmxfct(cinfo->output_width, input_buf,
        input_row, output_buf, num_rows);
  #endif
#endif
}

GLOBAL(int)
jsimd_can_h2v2_downsample (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;

  if (simd_support & JSIMD_SSE2)
    return 1;
  #ifndef __x86_64__
  if (simd_support & JSIMD_MMX)
    return 1;
  #endif

  return 0;
}

GLOBAL(int)
jsimd_can_h2v1_downsample (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;

  if (simd_support & JSIMD_SSE2)
    return 1;
  #ifndef __x86_64__
  if (simd_support & JSIMD_MMX)
    return 1;
  #endif

  return 0;
}

GLOBAL(void)
jsimd_h2v2_downsample (j_compress_ptr cinfo, jpeg_component_info * compptr,
                       JSAMPARRAY input_data, JSAMPARRAY output_data)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_SSE2)
    jsimd_h2v2_downsample_sse2(cinfo->image_width, cinfo->max_v_samp_factor,
        compptr->v_samp_factor, compptr->width_in_blocks,
        input_data, output_data);
  #ifndef __x86_64__
  else if (simd_support & JSIMD_MMX)
    jsimd_h2v2_downsample_mmx(cinfo->image_width, cinfo->max_v_samp_factor,
        compptr->v_samp_factor, compptr->width_in_blocks,
        input_data, output_data);
  #endif
#endif
}

GLOBAL(void)
jsimd_h2v1_downsample (j_compress_ptr cinfo, jpeg_component_info * compptr,
                       JSAMPARRAY input_data, JSAMPARRAY output_data)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_SSE2)
    jsimd_h2v1_downsample_sse2(cinfo->image_width, cinfo->max_v_samp_factor,
        compptr->v_samp_factor, compptr->width_in_blocks,
        input_data, output_data);
  #ifndef __x86_64__
  else if (simd_support & JSIMD_MMX)
    jsimd_h2v1_downsample_mmx(cinfo->image_width, cinfo->max_v_samp_factor,
        compptr->v_samp_factor, compptr->width_in_blocks,
        input_data, output_data);
  #endif
#endif
}

GLOBAL(int)
jsimd_can_h2v2_upsample (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;

  if (simd_support & JSIMD_SSE2)
    return 1;
  #ifndef __x86_64__
  if (simd_support & JSIMD_MMX)
    return 1;
  #endif

  return 0;
}

GLOBAL(int)
jsimd_can_h2v1_upsample (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;

  if (simd_support & JSIMD_SSE2)
    return 1;
  #ifndef __x86_64__
  if (simd_support & JSIMD_MMX)
    return 1;
  #endif

  return 0;
}

GLOBAL(void)
jsimd_h2v2_upsample (j_decompress_ptr cinfo,
                     jpeg_component_info * compptr, 
                     JSAMPARRAY input_data,
                     JSAMPARRAY * output_data_ptr)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_SSE2)
    jsimd_h2v2_upsample_sse2(cinfo->max_v_samp_factor,
        cinfo->output_width, input_data, output_data_ptr);
  #ifndef __x86_64__
  else if (simd_support & JSIMD_MMX)
    jsimd_h2v2_upsample_mmx(cinfo->max_v_samp_factor,
        cinfo->output_width, input_data, output_data_ptr);
  #endif
#endif
}

GLOBAL(void)
jsimd_h2v1_upsample (j_decompress_ptr cinfo,
                     jpeg_component_info * compptr, 
                     JSAMPARRAY input_data,
                     JSAMPARRAY * output_data_ptr)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_SSE2)
    jsimd_h2v1_upsample_sse2(cinfo->max_v_samp_factor,
        cinfo->output_width, input_data, output_data_ptr);
  #ifndef __x86_64__
  else if (simd_support & JSIMD_MMX)
    jsimd_h2v1_upsample_mmx(cinfo->max_v_samp_factor,
        cinfo->output_width, input_data, output_data_ptr);
  #endif
#endif
}

GLOBAL(int)
jsimd_can_h2v2_fancy_upsample (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;

  if ((simd_support & JSIMD_SSE2) &&
      IS_ALIGNED_SSE(jconst_fancy_upsample_sse2))
    return 1;
  #ifndef __x86_64__
  if (simd_support & JSIMD_MMX)
    return 1;
  #endif

  return 0;
}

GLOBAL(int)
jsimd_can_h2v1_fancy_upsample (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;

  if ((simd_support & JSIMD_SSE2) &&
      IS_ALIGNED_SSE(jconst_fancy_upsample_sse2))
    return 1;
  #ifndef __x86_64__
  if (simd_support & JSIMD_MMX)
    return 1;
  #endif

  return 0;
}

GLOBAL(void)
jsimd_h2v2_fancy_upsample (j_decompress_ptr cinfo,
                           jpeg_component_info * compptr, 
                           JSAMPARRAY input_data,
                           JSAMPARRAY * output_data_ptr)
{
#ifdef WITH_SIMD
  if ((simd_support & JSIMD_SSE2) &&
      IS_ALIGNED_SSE(jconst_fancy_upsample_sse2))
    jsimd_h2v1_fancy_upsample_sse2(cinfo->max_v_samp_factor,
        compptr->downsampled_width, input_data, output_data_ptr);
  #ifndef __x86_64__
  else if (simd_support & JSIMD_MMX)
    jsimd_h2v2_fancy_upsample_mmx(cinfo->max_v_samp_factor,
        compptr->downsampled_width, input_data, output_data_ptr);
  #endif
#endif
}

GLOBAL(void)
jsimd_h2v1_fancy_upsample (j_decompress_ptr cinfo,
                           jpeg_component_info * compptr, 
                           JSAMPARRAY input_data,
                           JSAMPARRAY * output_data_ptr)
{
#ifdef WITH_SIMD
  if ((simd_support & JSIMD_SSE2) &&
      IS_ALIGNED_SSE(jconst_fancy_upsample_sse2))
    jsimd_h2v1_fancy_upsample_sse2(cinfo->max_v_samp_factor,
        compptr->downsampled_width, input_data, output_data_ptr);
  #ifndef __x86_64__
  else if (simd_support & JSIMD_MMX)
    jsimd_h2v1_fancy_upsample_mmx(cinfo->max_v_samp_factor,
        compptr->downsampled_width, input_data, output_data_ptr);
  #endif
#endif
}

GLOBAL(int)
jsimd_can_h2v2_merged_upsample (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;

  if ((simd_support & JSIMD_SSE2) &&
      IS_ALIGNED_SSE(jconst_merged_upsample_sse2))
    return 1;
  #ifndef __x86_64__
  if (simd_support & JSIMD_MMX)
    return 1;
  #endif

  return 0;
}

GLOBAL(int)
jsimd_can_h2v1_merged_upsample (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;

  if ((simd_support & JSIMD_SSE2) &&
      IS_ALIGNED_SSE(jconst_merged_upsample_sse2))
    return 1;
  #ifndef __x86_64__
  if (simd_support & JSIMD_MMX)
    return 1;
  #endif

  return 0;
}

GLOBAL(void)
jsimd_h2v2_merged_upsample (j_decompress_ptr cinfo,
                            JSAMPIMAGE input_buf,
                            JDIMENSION in_row_group_ctr,
                            JSAMPARRAY output_buf)
{
#ifdef WITH_SIMD
  void (*sse2fct)(JDIMENSION, JSAMPIMAGE, JDIMENSION, JSAMPARRAY);
  #ifndef __x86_64__
  void (*mmxfct)(JDIMENSION, JSAMPIMAGE, JDIMENSION, JSAMPARRAY);
  #endif
  switch(cinfo->out_color_space)
  {
    case JCS_EXT_RGB:
      sse2fct=jsimd_h2v2_extrgb_merged_upsample_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_h2v2_extrgb_merged_upsample_mmx;
      #endif
      break;
    case JCS_EXT_RGBX:
      sse2fct=jsimd_h2v2_extrgbx_merged_upsample_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_h2v2_extrgbx_merged_upsample_mmx;
      #endif
      break;
    case JCS_EXT_BGR:
      sse2fct=jsimd_h2v2_extbgr_merged_upsample_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_h2v2_extbgr_merged_upsample_mmx;
      #endif
      break;
    case JCS_EXT_BGRX:
      sse2fct=jsimd_h2v2_extbgrx_merged_upsample_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_h2v2_extbgrx_merged_upsample_mmx;
      #endif
      break;
    case JCS_EXT_XBGR:
      sse2fct=jsimd_h2v2_extxbgr_merged_upsample_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_h2v2_extxbgr_merged_upsample_mmx;
      #endif
      break;
    case JCS_EXT_XRGB:
      sse2fct=jsimd_h2v2_extxrgb_merged_upsample_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_h2v2_extxrgb_merged_upsample_mmx;
      #endif
      break;
    default:
      sse2fct=jsimd_h2v2_merged_upsample_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_h2v2_merged_upsample_mmx;
      #endif
      break;
  }
  if ((simd_support & JSIMD_SSE2) &&
      IS_ALIGNED_SSE(jconst_merged_upsample_sse2))
    sse2fct(cinfo->output_width, input_buf,
        in_row_group_ctr, output_buf);
  #ifndef __x86_64__
  else if (simd_support & JSIMD_MMX)
    mmxfct(cinfo->output_width, input_buf,
        in_row_group_ctr, output_buf);
  #endif
#endif
}

GLOBAL(void)
jsimd_h2v1_merged_upsample (j_decompress_ptr cinfo,
                            JSAMPIMAGE input_buf,
                            JDIMENSION in_row_group_ctr,
                            JSAMPARRAY output_buf)
{
#ifdef WITH_SIMD
  void (*sse2fct)(JDIMENSION, JSAMPIMAGE, JDIMENSION, JSAMPARRAY);
  #ifndef __x86_64__
  void (*mmxfct)(JDIMENSION, JSAMPIMAGE, JDIMENSION, JSAMPARRAY);
  #endif
  switch(cinfo->out_color_space)
  {
    case JCS_EXT_RGB:
      sse2fct=jsimd_h2v1_extrgb_merged_upsample_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_h2v1_extrgb_merged_upsample_mmx;
      #endif
      break;
    case JCS_EXT_RGBX:
      sse2fct=jsimd_h2v1_extrgbx_merged_upsample_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_h2v1_extrgbx_merged_upsample_mmx;
      #endif
      break;
    case JCS_EXT_BGR:
      sse2fct=jsimd_h2v1_extbgr_merged_upsample_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_h2v1_extbgr_merged_upsample_mmx;
      #endif
      break;
    case JCS_EXT_BGRX:
      sse2fct=jsimd_h2v1_extbgrx_merged_upsample_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_h2v1_extbgrx_merged_upsample_mmx;
      #endif
      break;
    case JCS_EXT_XBGR:
      sse2fct=jsimd_h2v1_extxbgr_merged_upsample_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_h2v1_extxbgr_merged_upsample_mmx;
      #endif
      break;
    case JCS_EXT_XRGB:
      sse2fct=jsimd_h2v1_extxrgb_merged_upsample_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_h2v1_extxrgb_merged_upsample_mmx;
      #endif
      break;
    default:
      sse2fct=jsimd_h2v1_merged_upsample_sse2;
      #ifndef __x86_64__
      mmxfct=jsimd_h2v1_merged_upsample_mmx;
      #endif
      break;
  }
  if ((simd_support & JSIMD_SSE2) &&
      IS_ALIGNED_SSE(jconst_merged_upsample_sse2))
    sse2fct(cinfo->output_width, input_buf,
        in_row_group_ctr, output_buf);
  #ifndef __x86_64__
  else if (simd_support & JSIMD_MMX)
    mmxfct(cinfo->output_width, input_buf,
        in_row_group_ctr, output_buf);
  #endif
#endif
}

GLOBAL(int)
jsimd_can_convsamp (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (DCTSIZE != 8)
    return 0;
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if (sizeof(DCTELEM) != 2)
    return 0;

  if (simd_support & JSIMD_SSE2)
    return 1;
  #ifndef __x86_64__
  if (simd_support & JSIMD_MMX)
    return 1;
  #endif

  return 0;
}

GLOBAL(int)
jsimd_can_convsamp_float (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (DCTSIZE != 8)
    return 0;
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if (sizeof(FAST_FLOAT) != 4)
    return 0;

  if (simd_support & JSIMD_SSE2)
    return 1;
  if (simd_support & JSIMD_SSE)
    return 1;
  #ifndef __x86_64__
  if (simd_support & JSIMD_3DNOW)
    return 1;
  #endif

  return 0;
}

GLOBAL(void)
jsimd_convsamp (JSAMPARRAY sample_data, JDIMENSION start_col,
                DCTELEM * workspace)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_SSE2)
    jsimd_convsamp_sse2(sample_data, start_col, workspace);
  #ifndef __x86_64__
  else if (simd_support & JSIMD_MMX)
    jsimd_convsamp_mmx(sample_data, start_col, workspace);
  #endif
#endif
}

GLOBAL(void)
jsimd_convsamp_float (JSAMPARRAY sample_data, JDIMENSION start_col,
                      FAST_FLOAT * workspace)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_SSE2)
    jsimd_convsamp_float_sse2(sample_data, start_col, workspace);
  #ifndef __x86_64__
  else if (simd_support & JSIMD_SSE)
    jsimd_convsamp_float_sse(sample_data, start_col, workspace);
  else if (simd_support & JSIMD_3DNOW)
    jsimd_convsamp_float_3dnow(sample_data, start_col, workspace);
  #endif
#endif
}

GLOBAL(int)
jsimd_can_fdct_islow (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (DCTSIZE != 8)
    return 0;
  if (sizeof(DCTELEM) != 2)
    return 0;

  if ((simd_support & JSIMD_SSE2) && IS_ALIGNED_SSE(jconst_fdct_islow_sse2))
    return 1;
  #ifndef __x86_64__
  if (simd_support & JSIMD_MMX)
    return 1;
  #endif

  return 0;
}

GLOBAL(int)
jsimd_can_fdct_ifast (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (DCTSIZE != 8)
    return 0;
  if (sizeof(DCTELEM) != 2)
    return 0;

  if ((simd_support & JSIMD_SSE2) && IS_ALIGNED_SSE(jconst_fdct_ifast_sse2))
    return 1;
  #ifndef __x86_64__
  if (simd_support & JSIMD_MMX)
    return 1;
  #endif

  return 0;
}

GLOBAL(int)
jsimd_can_fdct_float (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (DCTSIZE != 8)
    return 0;
  if (sizeof(FAST_FLOAT) != 4)
    return 0;

  if ((simd_support & JSIMD_SSE) && IS_ALIGNED_SSE(jconst_fdct_float_sse))
    return 1;
  #ifndef __x86_64__
  if (simd_support & JSIMD_3DNOW)
    return 1;
  #endif

  return 0;
}

GLOBAL(void)
jsimd_fdct_islow (DCTELEM * data)
{
#ifdef WITH_SIMD
  if ((simd_support & JSIMD_SSE2) && IS_ALIGNED_SSE(jconst_fdct_islow_sse2))
    jsimd_fdct_islow_sse2(data);
  #ifndef __x86_64__
  else if (simd_support & JSIMD_MMX)
    jsimd_fdct_islow_mmx(data);
  #endif
#endif
}

GLOBAL(void)
jsimd_fdct_ifast (DCTELEM * data)
{
#ifdef WITH_SIMD
  if ((simd_support & JSIMD_SSE2) && IS_ALIGNED_SSE(jconst_fdct_islow_sse2))
    jsimd_fdct_ifast_sse2(data);
  #ifndef __x86_64__
  else if (simd_support & JSIMD_MMX)
    jsimd_fdct_ifast_mmx(data);
  #endif
#endif
}

GLOBAL(void)
jsimd_fdct_float (FAST_FLOAT * data)
{
#ifdef WITH_SIMD
  if ((simd_support & JSIMD_SSE) && IS_ALIGNED_SSE(jconst_fdct_float_sse))
    jsimd_fdct_float_sse(data);
  #ifndef __x86_64__
  else if (simd_support & JSIMD_3DNOW)
    jsimd_fdct_float_3dnow(data);
  #endif
#endif
}

GLOBAL(int)
jsimd_can_quantize (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (DCTSIZE != 8)
    return 0;
  if (sizeof(JCOEF) != 2)
    return 0;
  if (sizeof(DCTELEM) != 2)
    return 0;

  if (simd_support & JSIMD_SSE2)
    return 1;
  #ifndef __x86_64__
  if (simd_support & JSIMD_MMX)
    return 1;
  #endif

  return 0;
}

GLOBAL(int)
jsimd_can_quantize_float (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (DCTSIZE != 8)
    return 0;
  if (sizeof(JCOEF) != 2)
    return 0;
  if (sizeof(FAST_FLOAT) != 4)
    return 0;

  if (simd_support & JSIMD_SSE2)
    return 1;
  if (simd_support & JSIMD_SSE)
    return 1;
  #ifndef __x86_64__
  if (simd_support & JSIMD_3DNOW)
    return 1;
  #endif

  return 0;
}

GLOBAL(void)
jsimd_quantize (JCOEFPTR coef_block, DCTELEM * divisors,
                DCTELEM * workspace)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_SSE2)
    jsimd_quantize_sse2(coef_block, divisors, workspace);
  #ifndef __x86_64__
  else if (simd_support & JSIMD_MMX)
    jsimd_quantize_mmx(coef_block, divisors, workspace);
  #endif
#endif
}

GLOBAL(void)
jsimd_quantize_float (JCOEFPTR coef_block, FAST_FLOAT * divisors,
                      FAST_FLOAT * workspace)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_SSE2)
    jsimd_quantize_float_sse2(coef_block, divisors, workspace);
  #ifndef __x86_64__
  else if (simd_support & JSIMD_SSE)
    jsimd_quantize_float_sse(coef_block, divisors, workspace);
  else if (simd_support & JSIMD_3DNOW)
    jsimd_quantize_float_3dnow(coef_block, divisors, workspace);
  #endif
#endif
}

GLOBAL(int)
jsimd_can_idct_2x2 (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (DCTSIZE != 8)
    return 0;
  if (sizeof(JCOEF) != 2)
    return 0;
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if (sizeof(ISLOW_MULT_TYPE) != 2)
    return 0;

  if ((simd_support & JSIMD_SSE2) && IS_ALIGNED_SSE(jconst_idct_red_sse2))
    return 1;
  #ifndef __x86_64__
  if (simd_support & JSIMD_MMX)
    return 1;
  #endif

  return 0;
}

GLOBAL(int)
jsimd_can_idct_4x4 (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (DCTSIZE != 8)
    return 0;
  if (sizeof(JCOEF) != 2)
    return 0;
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if (sizeof(ISLOW_MULT_TYPE) != 2)
    return 0;

  if ((simd_support & JSIMD_SSE2) && IS_ALIGNED_SSE(jconst_idct_red_sse2))
    return 1;
  #ifndef __x86_64__
  if (simd_support & JSIMD_MMX)
    return 1;
  #endif

  return 0;
}

GLOBAL(void)
jsimd_idct_2x2 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
                JCOEFPTR coef_block, JSAMPARRAY output_buf,
                JDIMENSION output_col)
{
#if WITH_SIMD
  if ((simd_support & JSIMD_SSE2) && IS_ALIGNED_SSE(jconst_idct_red_sse2))
    jsimd_idct_2x2_sse2(compptr->dct_table, coef_block, output_buf, output_col);
  #ifndef __x86_64__
  else if (simd_support & JSIMD_MMX)
    jsimd_idct_2x2_mmx(compptr->dct_table, coef_block, output_buf, output_col);
  #endif
#endif
}

GLOBAL(void)
jsimd_idct_4x4 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
                JCOEFPTR coef_block, JSAMPARRAY output_buf,
                JDIMENSION output_col)
{
#if WITH_SIMD
  if ((simd_support & JSIMD_SSE2) && IS_ALIGNED_SSE(jconst_idct_red_sse2))
    jsimd_idct_4x4_sse2(compptr->dct_table, coef_block, output_buf, output_col);
  #ifndef __x86_64__
  else if (simd_support & JSIMD_MMX)
    jsimd_idct_4x4_mmx(compptr->dct_table, coef_block, output_buf, output_col);
  #endif
#endif
}

GLOBAL(int)
jsimd_can_idct_islow (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (DCTSIZE != 8)
    return 0;
  if (sizeof(JCOEF) != 2)
    return 0;
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if (sizeof(ISLOW_MULT_TYPE) != 2)
    return 0;

  if ((simd_support & JSIMD_SSE2) && IS_ALIGNED_SSE(jconst_idct_islow_sse2))
    return 1;
  #ifndef __x86_64__
  if (simd_support & JSIMD_MMX)
    return 1;
  #endif

  return 0;
}

GLOBAL(int)
jsimd_can_idct_ifast (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (DCTSIZE != 8)
    return 0;
  if (sizeof(JCOEF) != 2)
    return 0;
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if (sizeof(IFAST_MULT_TYPE) != 2)
    return 0;
  if (IFAST_SCALE_BITS != 2)
    return 0;

  if ((simd_support & JSIMD_SSE2) && IS_ALIGNED_SSE(jconst_idct_ifast_sse2))
    return 1;
  #ifndef __x86_64__
  if (simd_support & JSIMD_MMX)
    return 1;
  #endif

  return 0;
}

GLOBAL(int)
jsimd_can_idct_float (void)
{
  init_simd();

  if (DCTSIZE != 8)
    return 0;
  if (sizeof(JCOEF) != 2)
    return 0;
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if (sizeof(FAST_FLOAT) != 4)
    return 0;
  if (sizeof(FLOAT_MULT_TYPE) != 4)
    return 0;

  if ((simd_support & JSIMD_SSE2) && IS_ALIGNED_SSE(jconst_idct_float_sse2))
    return 1;
  #ifndef __x86_64__
  if ((simd_support & JSIMD_SSE) && IS_ALIGNED_SSE(jconst_idct_float_sse))
    return 1;
  if (simd_support & JSIMD_3DNOW)
    return 1;
  #endif

  return 0;
}

GLOBAL(void)
jsimd_idct_islow (j_decompress_ptr cinfo, jpeg_component_info * compptr,
                JCOEFPTR coef_block, JSAMPARRAY output_buf,
                JDIMENSION output_col)
{
#if WITH_SIMD
  if ((simd_support & JSIMD_SSE2) && IS_ALIGNED_SSE(jconst_idct_islow_sse2))
    jsimd_idct_islow_sse2(compptr->dct_table, coef_block, output_buf, output_col);
  #ifndef __x86_64__
  else if (simd_support & JSIMD_MMX)
    jsimd_idct_islow_mmx(compptr->dct_table, coef_block, output_buf, output_col);
  #endif
#endif
}

GLOBAL(void)
jsimd_idct_ifast (j_decompress_ptr cinfo, jpeg_component_info * compptr,
                JCOEFPTR coef_block, JSAMPARRAY output_buf,
                JDIMENSION output_col)
{
#if WITH_SIMD
  if ((simd_support & JSIMD_SSE2) && IS_ALIGNED_SSE(jconst_idct_ifast_sse2))
    jsimd_idct_ifast_sse2(compptr->dct_table, coef_block, output_buf, output_col);
  #ifndef __x86_64__
  else if (simd_support & JSIMD_MMX)
    jsimd_idct_ifast_mmx(compptr->dct_table, coef_block, output_buf, output_col);
  #endif
#endif
}

GLOBAL(void)
jsimd_idct_float (j_decompress_ptr cinfo, jpeg_component_info * compptr,
                JCOEFPTR coef_block, JSAMPARRAY output_buf,
                JDIMENSION output_col)
{
#if WITH_SIMD
  if ((simd_support & JSIMD_SSE2) && IS_ALIGNED_SSE(jconst_idct_float_sse2))
    jsimd_idct_float_sse2(compptr->dct_table, coef_block,
        output_buf, output_col);
  #ifndef __x86_64__
  else if ((simd_support & JSIMD_SSE) && IS_ALIGNED_SSE(jconst_idct_float_sse))
    jsimd_idct_float_sse(compptr->dct_table, coef_block,
        output_buf, output_col);
  else if (simd_support & JSIMD_3DNOW)
    jsimd_idct_float_3dnow(compptr->dct_table, coef_block,
        output_buf, output_col);
  #endif
#endif
}
