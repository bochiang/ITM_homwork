#ifndef INTRA_ADD_H
#define INTRA_ADD_H
#include <stdint.h>

typedef uint8_t  uchar_t;
typedef uint8_t  pel_t;


#define USING_INTRA_5_9               1


#define MAX_CU_SIZE                   16

int  g_log2size[MAX_CU_SIZE + 1];

#define NUM_INTRA_PMODE               33        //!< # luma intra prediction modes

#define Clip3(min,max,val) (((val)<(min))?(min):(((val)>(max))?(max):(val)))

void xPredIntraPlaneAdi(uchar_t *pSrc, pel_t *pDst, int i_dst, int iWidth, int iHeight, int sample_bit_depth);
void xPredIntraBiAdi(uchar_t *pSrc, pel_t *pDst, int i_dst, int iWidth, int iHeight, int sample_bit_depth);
void intra_pred_ang_30_c(pel_t *src, pel_t *dst, int i_dst, int bsx, int bsy);
void intra_pred_ang_20_c(pel_t *src, pel_t *dst, int i_dst, int bsx, int bsy);
void intra_init();

#endif