#ifndef _INTER_PREDICTION_H_
#define _INTER_PREDICTION_H_

#include <string.h>

#include "defines.h"
#include "global.h"
#include "common.h"

void get_chroma_block( pel_t *pDst, int i_dst, int mv_x, int mv_y, int posx, int posy, uchar_t *refpic, int i_refc );
void get_luma_block( ImgParams  *img, pel_t *pDst, int i_dst, int bsize, int x_pos, int y_pos, pel_t *ref_pic, int i_ref );
void SetSpatialMVPredictor( ImgParams  *img, Macroblock *currMB, int *pmv, int **refFrArr, int ***tmp_mv, int ref_frame, int block_x, int block_y, int blk_width, int ref );
void GetPskipMV( ImgParams *img, Macroblock *currMB, int bsize );
void GetBdirectMV( ImgParams *img, Macroblock *currMB, int bsize, int *fmv, int *bmv, int img_b8_x, int img_b8_y );
void get_pred_mv( ImgParams *img, Macroblock *currMB, int b8, int bframe, int img_b8_x, int img_b8_y, int *vec1_x, int *vec1_y, int *vec2_x, int *vec2_y, int *refframe, int *fw_refframe, int *bw_refframe );
void get_mv_ref( ImgParams *img, Macroblock *currMB, int b8, int bframe, int img_b8_x, int img_b8_y, int *vec1_x, int *vec1_y, int *vec2_x, int *vec2_y, int *refframe, int *fw_ref, int *bw_ref );

int calculate_distance( int blkref, int fw_bw );
int GenSymBackMV(int forward_mv);

void get_chroma_block_enc( pel_t *pDst, int i_dst, int bsize, int int_mv_x, int int_mv_y, int posx, int posy, pel_t *refpic );
pel_t get_chroma_subpix( int curx, int cury, int posx, int posy, pel_t *src, int i_src );
int com_if_filter_hor_chroma( pel_t *src, int i_src, const int COEF[8][4], int curx, int cury, int posx );
int com_if_filter_ver_chroma( pel_t *src, int i_src, const int COEF[8][4], int curx, int cury, int posy );
int com_if_filter_hor_ver_chroma( pel_t *src, int i_src, const int COEF[8][4], int curx, int cury, int posx, int posy );

#endif