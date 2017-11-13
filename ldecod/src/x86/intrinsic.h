/*****************************************************************************
* Authors: Ronggang Wang <rgwang@pkusz.edu.cn>
*          Zhenyu Wang <wangzhenyu@pkusz.edu.cn>
*          Kui Fan <kuifan@pku.edu.cn>
*          Shenghao Zhang <1219759986@qq.com>
* Affiliate: Peking University Shenzhen Graduate School
*****************************************************************************/

#ifndef __INTRINSIC_H__
#define __INTRINSIC_H__

#include "../../../common/common.h"

#include <mmintrin.h>
#include <emmIntrin.h>
#include <tmmIntrin.h>
#include <smmIntrin.h>

#define FILTER_HORORVER_OFFSET   32
#define FILTER_HORORVER_SHIFT    6
#define FILTER_HORANDVER_OFFSET  1<<11
#define FILTER_HORANDVER_SHIFT   12
#define INIT_ZERO                0

#define FILTER_TAPNUM_4             4
#define FILTER_TAPNUM_6             6
#define FILTER_TAPNUM_10           10

#define HOR_VER_4_TMPWIDTH       4*8
#define HOR_VER_6_TMPWIDTH       6*8
#define HOR_VER_10_TMPWIDTH      10*8

// ----------- intra -------------
#define INTRA_DC_CONSTVALUE      128
#define INIT_POS_ZERO            0


ALIGNED_16( extern char intrinsic_mask[15][16] );

void com_if_filter_cpy_sse128( const pel_t *src, int i_src, pel_t *dst, int i_dst, int width, int height );

// ------------------  inter hor filter --------------------
void com_if_filter_hor_4_sse128( const pel_t *src,
                                 int i_src,
                                 pel_t *dst,
                                 int i_dst,
                                 int width,
                                 int height,
                                 char_t const *coeff,
                                 int max_val );
void com_if_filter_hor_6_sse128( const pel_t *src,
                                 int i_src,
                                 pel_t *dst,
                                 int i_dst,
                                 int width,
                                 int height,
                                 char_t const *coeff,
                                 int max_val );
void com_if_filter_hor_10_sse128( const pel_t *src,
                                  int i_src,
                                  pel_t *dst,
                                  int i_dst,
                                  int width,
                                  int height,
                                  char_t const *coeff,
                                  int max_val );

// ------------------  inter ver filter --------------------
void com_if_filter_ver_4_sse128( const pel_t *src,
                                 int i_src,
                                 pel_t *dst,
                                 int i_dst,
                                 int width,
                                 int height,
                                 char_t const *coeff,
                                 int max_val );
void com_if_filter_ver_6_sse128( const pel_t *src,
                                 int i_src,
                                 pel_t *dst,
                                 int i_dst,
                                 int width,
                                 int height,
                                 char_t const *coeff,
                                 int max_val );
void com_if_filter_ver_10_sse128( const pel_t *src,
                                  int i_src,
                                  pel_t *dst,
                                  int i_dst,
                                  int width,
                                  int height,
                                  char_t const *coeff,
                                  int max_val );

// ------------------  inter ver & hor filter--------------------
void com_if_filter_hor_ver_4_sse128( const pel_t *src,
                                     int i_src,
                                     pel_t *dst,
                                     int i_dst,
                                     int width,
                                     int height,
                                     const char_t *coeff_h,
                                     const char_t *coeff_v,
                                     int max_val );
void com_if_filter_hor_ver_6_sse128( const pel_t *src,
                                     int i_src,
                                     pel_t *dst,
                                     int i_dst,
                                     int width,
                                     int height,
                                     const char_t *coeff_h,
                                     const char_t *coeff_v,
                                     int max_val );
void com_if_filter_hor_ver_10_sse128( const pel_t *src,
                                      int i_src,
                                      pel_t *dst,
                                      int i_dst,
                                      int width,
                                      int height,
                                      const char_t *coeff_h,
                                      const char_t *coeff_v,
                                      int max_val );
void get_chroma_subpix_Ext_sse128( const pel_t *pSrc,
                                   int i_ref,
                                   pel_t *pDst,
                                   int i_dst,
                                   const char_t *COEF_HOR,
                                   const char_t *COEF_VER );

// ------------------  intra  --------------------
void xPredIntraLumaDC_sse128( uchar_t *pSrc,
                              pel_t *pDst,
                              int i_dst,
                              int iBlkWidth,
                              int iBlkHeight,
                              bool bAboveAvail,
                              bool bLeftAvail );
void xPredIntraChromaDC_sse128( uchar_t *pSrc,
                                pel_t *pDst,
                                int i_dst,
                                int iBlkWidth,
                                int iBlkHeight,
                                bool bAboveAvail,
                                bool bLeftAvail );
void xPredIntraVer_sse128( uchar_t *pSrc, pel_t *pDst, int i_dst, int iBlkWidth, int iBlkHeight );
void xPredIntraHor_sse128( uchar_t *pSrc, pel_t *pDst, int i_dst, int iBlkWidth, int iBlkHeight );
void xPredIntraDownRight_sse128( uchar_t *pSrc, pel_t *pDst, int i_dst, int iBlkWidth, int iBlkHeight );
void xPredIntraDownLeft_sse128( uchar_t *pSrc, pel_t *pDst, int i_dst, int iBlkWidth, int iBlkHeight );
void xPredIntraChromaPlane_sse128( uchar_t *pSrc, pel_t *pDst, int i_dst, int iBlkWidth, int iBlkHeight );

// ------------------  deblock  --------------------
void EdgeLoopVer_sse128( uchar_t* SrcPtr, int istride, bool bChroma, int edge );
void EdgeLoopHor_sse128( uchar_t* SrcPtr, int istride, bool bChroma, int edge );

// ------------------  idct  --------------------
void idct_4x4_sse128( int *curr_blk );
void idct_8x8_sse128( int *curr_blk );
void idct_16x16_sse128( int *curr_blk, int bsize );

// ------------------  pixel  --------------------
void add_pel_clip_sse128( int b8, int b4, int* curr_blk, int bsize, \
                          pel_t *ppredblk, int( *pm7 )[16], int ipix_y, \
                          int iStride, int ipix_x, int ipix_c_y, int ipix_c_x, int iStrideC );
void avg_pel_sse128( pel_t *dst, int i_dst, pel_t *src1, int i_src1, pel_t *src2, int i_src2, int width, int height );
void padding_rows_sse128( pel_t *src, int i_src, int width, int height, int pad );
void com_cpy_sse128( const pel_t *src, int i_src, pel_t *dst, int i_dst, int width, int height );

/*
void com_if_filter_hor_ver_4_H_sse128(const pel_t *src,
                                      int i_src,
                                      int *dst,
                                      int i_dst,
                                      int width,
                                      int height,
                                      char_t const *coef_hor);
void com_if_filter_hor_ver_4_V_sse128(const int *src,
                                      int i_src,
                                      pel_t *dst,
                                      int i_dst,
                                      int width,
                                      int height,
                                      char_t const *coef_ver,
                                      int max_val);
*/
#endif  // #ifndef __INTRINSIC_H__