#ifndef _INTRA_PREDICTION_H_
#define _INTRA_PREDICTION_H_

#include "defines.h"
#include "global.h"
#include "common.h"

void xPrepareIntraPattern( ImgParams *img, Macroblock *currMB, pel_t *pedge, int img_x, int img_y, int bsize, bool *bAboveAvail, bool *bLeftAvail );
void xPrepareIntraPatternC( ImgParams *img, Macroblock *currMB, pel_t *pedge, int uv, int bsize, bool *bAboveAvail, bool *bLeftAvail );

void intra_pred_luma( uchar_t *pSrc, pel_t *pDst, int i_dst, int predmode, int uhBlkWidth, bool bAboveAvail, bool bLeftAvail );
void intra_pred_chroma( uchar_t *pSrc, pel_t *pDst, int i_dst, int predmode, int bsize, bool bAboveAvail, bool bLeftAvail );

void xIntraPredFilter( pel_t *pSrc, pel_t *pDst, int bsize );

#endif