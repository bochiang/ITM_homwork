#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <memory.h>
#include "defines.h"
#include "global.h"

#define DEBUG_TEST 0

#if DEBUG_TEST
extern FILE* g_debug_fp;
#endif


/* ---------------------------------------------------------------------------
 * log level
 */
#define COM_LOG_NONE       (-1)
#define COM_LOG_ERROR      0
#define COM_LOG_WARNING    1
#define COM_LOG_INFO       2
#define COM_LOG_DEBUG      3

/* ---------------------------------------------------------------------------
 * date align
 */
#if defined(_WIN32)
#define DECLARE_ALIGNED(var, n) __declspec(align(n)) var
#else
#define DECLARE_ALIGNED(var, n) var __attribute__((aligned (n)))
#endif
#define ALIGNED_32(var)   DECLARE_ALIGNED(var, 32)
#define ALIGNED_16(var)   DECLARE_ALIGNED(var, 16)
#define ALIGNED_8(var)    DECLARE_ALIGNED(var, 8)
#define ALIGNED_4(var)    DECLARE_ALIGNED(var, 4)

#define ALIGN_BASIC 32 // for new generation CPU with 256-bits SIMD
#define ALIGN_MASK (ALIGN_BASIC - 1)
#define ALIGN_POINTER(x) (x + ALIGN_MASK - (((intptr_t)x + ALIGN_MASK) & ((intptr_t)ALIGN_MASK)))

/* ---------------------------------------------------------------------------
 * basic math operations
 */
#define COM_ABS(a)         ((a) < (0) ? (-(a)) : (a))
#define COM_MAX(a, b)      ((a) > (b) ? (a) : (b))
#define COM_MIN(a, b)      ((a) < (b) ? (a) : (b))
#define COM_CLIP3( min, max, val) (((val)<(min))? (min):(((val)>(max))? (max):(val)))
#define COM_ADD_MODE(v, mode) ((v) & (mode - 1))

/* ---------------------------------------------------------------------------
 * memory operations
 */
typedef union
{
    i16u_t    i;
    uchar_t   c[2];
} com_union16_t;

typedef union
{
    i32u_t    i;
    i16u_t    b[2];
    uchar_t   c[4];
} com_union32_t;

typedef union
{
    i64u_t    i;
    i32u_t    a[2];
    i16u_t    b[4];
    uchar_t   c[8];
} com_union64_t;

#define M16(src)                (((com_union16_t*)(src))->i)
#define M32(src)                (((com_union32_t*)(src))->i)
#define M64(src)                (((com_union64_t*)(src))->i)

#define CP16(dst,src)           M16(dst) = M16(src)
#define CP32(dst,src)           M32(dst) = M32(src)
#define CP64(dst,src)           M64(dst) = M64(src)

/* ---------------------------------------------------------------------------
 * SIMD/neon/.... declare
 */
#define SIMD_NAME(func) func##_sse128

/* ---------------------------------------------------------------------------
 * memory alloc & free
 */
void *com_malloc( int i_size );
void  com_free( void *p );

/* ---------------------------------------------------------------------------
 * functions handle
 */
enum IPFilterConf
{
    IPFILTER_H_4,
    IPFILTER_H_6,
    IPFILTER_H_10,
    IPFILTER_V_4,
    IPFILTER_V_6,
    IPFILTER_V_10,
    NUM_IPFILTER
};

enum IPFilterConf_Ext
{
    IPFILTER_EXT_4,
    IPFILTER_EXT_6,
    IPFILTER_EXT_10,
    NUM_IPFILTER_Ext
};


typedef struct
{
    // inter filter
    void( *ipcpy ) ( const pel_t *src, int i_src, pel_t *dst, int i_dst, int width, int height );
    void( *ipflt[NUM_IPFILTER] ) ( const pel_t *src, int i_src, pel_t *dst, int i_dst, int width, int height, const char_t *coeff, int max_val );
    //void(*ipflt_EXT_H[NUM_IPFILTER_Ext]) (const pel_t *src, int i_src, int *dst, int i_dst, int width, int height, char_t const *coeff);
    //void(*ipflt_EXT_V[NUM_IPFILTER_Ext]) (const int *src, int i_src, pel_t *dst, int i_dst, int width, int height, char_t const *coeff, int max_val);
    void( *ipflt_EXT[NUM_IPFILTER_Ext] ) ( const pel_t *src, int i_src, pel_t *dst, int i_dst, int width, int height, const char_t *coeff_h, const char_t *coeff_v, int max_val );
    void( *ipflt_chroma_subpix_EXT[NUM_IPFILTER_Ext] ) ( const pel_t *src, int i_src, pel_t *dst, int i_dst, const char_t *coeff_h, const char_t *coeff_v );


    void( *cpy ) ( const pel_t *src, int i_src, pel_t *dst, int i_dst, int width, int height );
    void( *cpy_pel_to_uchar )( const pel_t *src, int i_src, uchar_t *dst, int i_dst, int width, int height, int bit_size );
    void( *cpy_pel_I420_to_uchar_YUY2 )( const pel_t *srcy, const pel_t *srcu, const pel_t *srcv, int i_src, int i_srcc, uchar_t *dst, int i_dst, int width, int height, int bit_size );
    void( *padding_rows_lr )( pel_t *src, int i_src, int width, int height, int start, int rows, int pad );




    //void(*idct_sqt[5])(coef_t *blk, int shift, int clip);
    void( *idct_hor[3] )( coef_t *blk, int shift, int clip );
    void( *idct_ver[3] )( coef_t *blk, int shift, int clip );
    void( *inv_2nd_hor )( coef_t *blk, int i_blk, int shift, const i16s_t coef[4][4] );
    void( *inv_2nd_ver )( coef_t *blk, int i_blk, int shift, const i16s_t coef[4][4] );
    void( *inv_2nd )( coef_t *blk, int i_blk, int shift, int clip_depth, const i16s_t coef[4][4] );
    void( *inv_wavelet )( coef_t *blk );
    void( *inv_wavelet_hor )( coef_t *blk );
    void( *inv_wavelet_ver )( coef_t *blk );

    void( *alf_flt )( pel_t *imgRes, pel_t *imgPad, int stride, int isChroma, int yPos, int lcu_height, int xPos, int lcu_width, int *coef, int sample_bit_depth, int isAboveAvail, int isBelowAvail );


    // deblock
    void( *deblock_edge[2] )( uchar_t* SrcPtr, int istride, bool bChroma, int edge );


    // intra filter
    void( *intra_pred_luma_dc )( uchar_t *pSrc, pel_t *pDst, int i_dst, int iBlkWidth, int iBlkHeight, bool bAboveAvail, bool bLeftAvail );
    void( *intra_pred_chroma_dc )( uchar_t *pSrc, pel_t *pDst, int i_dst, int iBlkWidth, int iBlkHeight, bool bAboveAvail, bool bLeftAvail );
    void( *intra_pred_ver )( uchar_t *pSrc, pel_t *pDst, int i_dst, int iBlkWidth, int iBlkHeight );
    void( *intra_pred_hor )( uchar_t *pSrc, pel_t *pDst, int i_dst, int iBlkWidth, int iBlkHeight );
    void( *intra_pred_downright )( uchar_t *pSrc, pel_t *pDst, int i_dst, int iBlkWidth, int iBlkHeight );
    void( *intra_pred_downleft )( uchar_t *pSrc, pel_t *pDst, int i_dst, int iBlkWidth, int iBlkHeight );
    void( *intra_pred_chroma_plane )( uchar_t *pSrc, pel_t *pDst, int i_dst, int iBlkWidth, int iBlkHeight );

    // dct
    void( *dct_sqt[2] )( const int *curr_blk, int *coef_blk );
    void( *dct_sqt_16 )( const int *curr_blk, int *coef_blk, int bsize );

    // idct
    void( *idct_sqt[2] )( int *curr_blk );
    void( *idct_sqt_16 )( int *curr_blk, int bsize );

    // pixel
    void( *padding_rows )( pel_t *src, int i_src, int width, int height, int pad );

    void( *post_rdoquant )( const int bsize, const int *src, int *dst );
    void( *quant )( int qp, int isIntra, int *curr_blk, int bsize, int Q );
    void( *inv_quant )( int coef_num, int qp, int bsize, const int *IVC_SCAN, int *curr_blk, int shift, int QPI );
    void( *avg_pel )( pel_t *dst, int i_dst, pel_t *src1, int i_src1, pel_t *src2, int i_src2, int width, int height );
    void( *com_cpy )( const pel_t *src, int i_src, pel_t *dst, int i_dst, int width, int height );
} funs_handle_t;

extern funs_handle_t g_funs_handle;

/* ---------------------------------------------------------------------------
 * function handle init for c coding
 */
void com_funs_init_intra_pred();
void com_funs_init_ip_filter();
void com_funs_init_pixel_opt();
void com_funs_init_deblock_filter();
void com_funs_init_dct();
void com_funs_init_idct();
void com_funs_init_alf_filter();

void com_init_intrinsic_256();
void com_init_intrinsic();
void com_init_neon128();

void init_contexts( CSobj *cs_aec );

#if TRACE
void output_trace_info( char *notes, int val );
#endif

void getNeighbour(Macroblock *currMB, int curr_mb_nr, int xN, int yN, int luma, PixelPos *pix);
void getLuma8x8Neighbour(Macroblock *currMB, int curr_mb_nr, int b8, int rel_x, int rel_y, PixelPos *pix);

#endif // #ifndef __COMMON_H__