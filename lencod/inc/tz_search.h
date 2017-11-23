#ifndef TZ_SEARCH_H
#define TZ_SEARCH_H

#define USING_TZ_SEARCH     1

#include "global.h"

int Get_mvc(ImgParams *img, Macroblock *currMB, int(*pmv)[2], int **refFrArr, int ***tmp_mv,
    int ref_frame,
    int pix_x_in_mb,
    int pix_y_in_mb,
    int bsize_x,
    int mode,
    int ref);

int                                     //  ==> minimum motion cost after search
Tz_Search(pel_t*   orig_val,    // <--  not used
    int       stride,
    int       ref,          // <--  reference frame (0... or -1 (backward))
    int       pic_pix_x,    // <--  absolute x-coordinate of regarded AxB block
    int       pic_pix_y,    // <--  absolute y-coordinate of regarded AxB block
    int       mode,         // <--  block type (1-16x16 ... 7-4x4)
    int      (*mvc)[2],    // <--  motion vector predictor in sub-pel units
    int       i_mvc,      // <--  number of motion vector predictors
    int      *mv_x,         //  --> motion vector (x) - in pel units
    int      *mv_y,         //  --> motion vector (y) - in pel units
    int       min_mcost);   // <--  minimum motion cost (cost for center or huge value)

int                                     //  ==> minimum motion cost after search
Tz_SearchMhp(pel_t*   orig_val,    // <--  not used
    int       stride,
    int       ref,          // <--  reference frame (0... or -1 (backward))
    int       pic_pix_x,    // <--  absolute x-coordinate of regarded AxB block
    int       pic_pix_y,    // <--  absolute y-coordinate of regarded AxB block
    int       mode,         // <--  block type (1-16x16 ... 7-4x4)
    int      (*mvc)[2],    // <--  motion vector predictor in sub-pel units
    int       i_mvc,      // <--  number of motion vector predictors
    int      *mv_x,         //  --> motion vector (x) - in pel units
    int      *mv_y,         //  --> motion vector (y) - in pel units
    int      *fw_mv_x,      //  --> forward motion vector (x) - in pel units
    int      *fw_mv_y,      //  --> forward motion vector (y) - in pel units
    int       min_mcost);   // <--  minimum motion cost (cost for center or huge value)
#endif // !TZ_SEARCH_H

