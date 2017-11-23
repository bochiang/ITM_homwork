#include "global.h"
#include "fast_me.h"
#include "defines.h"
#include "refbuf.h"
#include <math.h>
#include <stdint.h>

int*   mvbits;
int*   refbits;
int*   byte_abs;
static pel_t(*PelY_14) (pel_t*, int, int);
static pel_t(*Fw_PelY_14) (pel_t*, int, int);

#define MAKEDWORD(mx, my)     (((my) << 16) | ((mx) & 0xFFFF))
#define TZ_MIN(a, b)       ((a) < (b)? (a) : (b))
#define TZ_MAX(a, b)       ((a) > (b)? (a) : (b))

#define TZ_SEARCH_ONE_PIXEL_X4(dist, dir)                                                                                         \
    if(abs(cand_x - center_x) <=search_range && abs(cand_y - center_y)<= search_range) {                         \
        mcost = MV_COST (lambda_factor, mvshift, cand_x, cand_y, pred_x, pred_y);                            \
        if (ref != -1) {                                                                                     \
            mcost += REF_COST(lambda_factor, ref);                                                           \
        }                                                                                                    \
        mcost = PartCalMad(ref_pic, orig_val, stride, get_ref_line, mode, mcost, min_mcost, cand_x, cand_y); \
        if (mvinfo.bcost > mcost) {                                                                             \
            mvinfo.bdist = dist;                                                                                 \
            mvinfo.bdir = dir;                                                                                 \
            mvinfo.bcost = mcost;                                                                               \
            best_x = cand_x;                                                                                 \
            best_y = cand_y;                                                                                 \
        }                                                                                                    \
    }

#define TZ_SEARCH_ONE_PIXEL                                                                                         \
    if(abs(cand_x - center_x) <=search_range && abs(cand_y - center_y)<= search_range) {                         \
        mcost = MV_COST (lambda_factor, mvshift, cand_x, cand_y, pred_x, pred_y);                            \
        if (ref != -1) {                                                                                     \
            mcost += REF_COST(lambda_factor, ref);                                                           \
        }                                                                                                    \
        mcost = Tz_PartCalMad(ref_pic, orig_val, stride, get_ref_line, mode, mcost, min_mcost, cand_x, cand_y); \
        if (mcost < min_mcost) {                                                                             \
            best_x = cand_x;                                                                                 \
            best_y = cand_y;                                                                                 \
            min_mcost = mcost;                                                                               \
        }                                                                                                    \
    }

#define TZ_SEARCH_ONE_PIXEL1(value_iAbort)                                                                          \
    if(abs(cand_x - center_x) <=search_range && abs(cand_y - center_y)<= search_range){                          \
        mcost = MV_COST (lambda_factor, mvshift, cand_x, cand_y, pred_x, pred_y);                            \
        if (ref != -1) {                                                                                     \
            mcost += REF_COST(lambda_factor, ref);                                                           \
        }                                                                                                    \
        mcost = PartCalMad(ref_pic, orig_val, stride, get_ref_line, mode, mcost, min_mcost, cand_x, cand_y); \
        if (mcost < min_mcost) {                                                                             \
            best_x = cand_x;                                                                                 \
            best_y = cand_y;                                                                                 \
            min_mcost = mcost;                                                                               \
            iAbort = value_iAbort;                                                                           \
        }                                                                                                    \
    }

#define TZ_MHMC_SEARCH_ONE_PIXEL                                                                                                                                             \
    if(abs(cand_x - center_x) <=search_range && abs(cand_y - center_y)<= search_range) {                                                                                  \
        mcost = MV_COST (lambda_factor, mvshift, cand_x, cand_y, pred_x, pred_y);                                                                                     \
        if (ref != -1) {                                                                                                                                              \
            mcost += REF_COST(lambda_factor, ref);                                                                                                                    \
        }                                                                                                                                                             \
        mcost = MHMC_PartCalMad(ref_pic,fw_ref_pic, orig_val, stride, get_ref_line, mode, mcost, min_mcost, cand_x, cand_y, pic_pix_x, pic_pix_y, *fw_mv_x, *fw_mv_y);\
        McostState[cand_y-center_y+search_range][cand_x-center_x+search_range] = mcost;                                                                               \
        if (mcost < min_mcost) {                                                                                                                                      \
            best_x = cand_x;                                                                                                                                          \
            best_y = cand_y;                                                                                                                                          \
            min_mcost = mcost;                                                                                                                                        \
        }                                                                                                                                                             \
    }

#define TZ_MHMC_SEARCH_ONE_PIXEL_X4(dist, dir)                                                                                         \
    if(abs(cand_x - center_x) <=search_range && abs(cand_y - center_y)<= search_range) {                         \
        mcost = MV_COST (lambda_factor, mvshift, cand_x, cand_y, pred_x, pred_y);                            \
        if (ref != -1) {                                                                                     \
            mcost += REF_COST(lambda_factor, ref);                                                           \
        }                                                                                                    \
        mcost = MHMC_PartCalMad(ref_pic,fw_ref_pic, orig_val, stride, get_ref_line, mode, mcost, min_mcost, cand_x, cand_y, pic_pix_x, pic_pix_y, *fw_mv_x, *fw_mv_y); \
        if (mvinfo.bcost > mcost) {                                                                             \
            mvinfo.bdist = dist;                                                                                 \
            mvinfo.bdir = dir;                                                                                 \
            mvinfo.bcost = mcost;                                                                               \
            best_x = cand_x;                                                                                 \
            best_y = cand_y;                                                                                 \
        }                                                                                                    \
    }

#define TZ_MHMC_SEARCH_ONE_PIXEL1(value_iAbort)                                                                          \
    if(abs(cand_x - center_x) <=search_range && abs(cand_y - center_y)<= search_range){                          \
        mcost = MV_COST (lambda_factor, mvshift, cand_x, cand_y, pred_x, pred_y);                            \
        if (ref != -1) {                                                                                     \
            mcost += REF_COST(lambda_factor, ref);                                                           \
        }                                                                                                    \
        mcost = MHMC_PartCalMad(ref_pic,fw_ref_pic, orig_val, stride, get_ref_line, mode, mcost, min_mcost, cand_x, cand_y, pic_pix_x, pic_pix_y, *fw_mv_x, *fw_mv_y); \
        if (mcost < min_mcost) {                                                                             \
            best_x = cand_x;                                                                                 \
            best_y = cand_y;                                                                                 \
            min_mcost = mcost;                                                                               \
            iAbort = value_iAbort;                                                                           \
        }                                                                                                    \
    }

typedef struct mv_info {
    int     bdir;               /* best direction */
    int64_t  bcost;              /* best cost      */
    int     bdist;              /* best distance  */
} mv_info;

int mid_value(int a, int b, int c)
{
    if ((b <= a && a <= c) || (c <= a && a <= b)) // a在b，c之间， b<a<c 或者c<a<b
        return a;
    else if ((a <= b && b <= c) || (c <= b && b <= a)) //b在a，c之间。a<b<c 或者c<b<a
        return b;
    else if ((a <= c && c <= b) || (b <= c && c <= a)) //c在a，b 之间。a<c<b 或者 b<c<a
        return c;
}
int Get_mvc(ImgParams *img, Macroblock *currMB, int(*pmv)[2], int **refFrArr, int ***tmp_mv,
    int ref_frame,
    int pix_x_in_mb,
    int pix_y_in_mb,
    int bsize_x,
    int mode,
    int ref)
{
    int pu_b8_x = img->mb_b8_x + (pix_x_in_mb >> 3);
    int pu_b8_y = img->mb_b8_y + (pix_y_in_mb >> 3);
    int pu_b4_x = img->mb_b4_x + (pix_x_in_mb >> 2);
    int pu_b4_y = img->mb_b4_y + (pix_y_in_mb >> 2);
    int mb_nr = img->current_mb_nr;
    int mb_width = img->PicWidthInMbs;
    int mb_available_up = (img->mb_y == 0) ? 0 : (img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - mb_width].slice_nr);
    int mb_available_left = (img->mb_x == 0) ? 0 : (img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - 1].slice_nr);
    int mb_available_up_right = (img->mb_x >= mb_width - 1 || img->mb_y == 0) ? 0 : (img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - mb_width + 1].slice_nr);
    int block_available_up, block_available_left, block_available_up_right, block_available_up_left;
    int mv_a, mv_b, mv_c, mv_d, pred_vec = 0;
    int mvPredType, rFrameL, rFrameU, rFrameUR, rFrameUL;
    int hv, diff_a, diff_b, diff_c;

    int SAD_a, SAD_b, SAD_c, SAD_d;
    int temp_pred_SAD[2];
    int imvc;
    /* D B C */
    /* A X   */

    /* 1 A, B, D are set to 0 if unavailable       */
    /* 2 If C is not available it is replaced by D */

    block_available_up = mb_available_up || (pix_y_in_mb > 0);
    block_available_left = mb_available_left || (pix_x_in_mb > 0);
    block_available_up_left = block_available_up && block_available_left;

    if (pix_y_in_mb > 0) {
        if (pix_x_in_mb < 8) // first column of 8x8 blocks
        {
            if (pix_y_in_mb == 8) {
                block_available_up_right = (bsize_x != 16);
            }
            else {
                block_available_up_right = (pix_x_in_mb + bsize_x != 8);
            }
        }
        else {
            block_available_up_right = (pix_x_in_mb + bsize_x != 16);
        }
    }
    else if (pix_x_in_mb + bsize_x != MB_SIZE) {
        block_available_up_right = block_available_up;
    }
    else {
        block_available_up_right = mb_available_up_right;
    }

    //FAST MOTION ESTIMATION. ZHIBO CHEN 2003.3
    SAD_a = block_available_left ? ((ref == -1) ? all_bwmincost[pu_b4_x - 1][pu_b4_y][0][mode][0] : all_mincost[pu_b4_x - 1][pu_b4_y][ref_frame][mode][0]) : 0;
    SAD_b = block_available_up ? ((ref == -1) ? all_bwmincost[pu_b4_x][pu_b4_y - 1][0][mode][0] : all_mincost[pu_b4_x][pu_b4_y - 1][ref_frame][mode][0]) : 0;
    SAD_d = block_available_up_left ? ((ref == -1) ? all_bwmincost[pu_b4_x - 1][pu_b4_y - 1][0][mode][0] : all_mincost[pu_b4_x - 1][pu_b4_y - 1][ref_frame][mode][0]) : 0;
    SAD_c = block_available_up_right ? ((ref == -1) ? all_bwmincost[pu_b4_x + 1][pu_b4_y - 1][0][mode][0] : all_mincost[pu_b4_x + 1][pu_b4_y - 1][ref_frame][mode][0]) : SAD_d;

    rFrameL = block_available_left ? refFrArr[pu_b8_y][pu_b8_x - 1] : -1;
    rFrameU = block_available_up ? refFrArr[pu_b8_y - 1][pu_b8_x] : -1;
    rFrameUR = block_available_up_right ? refFrArr[pu_b8_y - 1][pu_b8_x + (bsize_x >> 3)] :
        block_available_up_left ? refFrArr[pu_b8_y - 1][pu_b8_x - 1] : -1;
    rFrameUL = block_available_up_left ? refFrArr[pu_b8_y - 1][pu_b8_x - 1] : -1;

    imvc = 0;
    mvPredType = MVPRED_xy_MIN;
    if ((rFrameL != -1) && (rFrameU == -1) && (rFrameUR == -1)) {
        mvPredType = MVPRED_L;
    }
    else if ((rFrameL == -1) && (rFrameU != -1) && (rFrameUR == -1)) {
        mvPredType = MVPRED_U;
    }
    else if ((rFrameL == -1) && (rFrameU == -1) && (rFrameUR != -1)) {
        mvPredType = MVPRED_UR;
    }

    for (hv = 0; hv < 2; hv++) {
        mv_a = block_available_left ? tmp_mv[pu_b8_y - 0][4 + pu_b8_x - 1][hv] : 0;
        mv_b = block_available_up ? tmp_mv[pu_b8_y - 1][4 + pu_b8_x][hv] : 0;
        mv_d = block_available_up_left ? tmp_mv[pu_b8_y - 1][4 + pu_b8_x - 1][hv] : 0;
        mv_c = block_available_up_right ? tmp_mv[pu_b8_y - 1][4 + pu_b8_x + (bsize_x >> 3)][hv] : mv_d;

        switch (mvPredType) {
        case MVPRED_xy_MIN:
            mv_c = block_available_up_right ? mv_c : mv_d;
            pmv[imvc][hv] = mid_value(mv_a, mv_b, mv_c);
            break;
        case MVPRED_L:
            pmv[imvc][hv] = mv_a;
            break;
        case MVPRED_U:
            pmv[imvc][hv] = mv_b;
            break;
        case MVPRED_UR:
            pmv[imvc][hv] = mv_c;
            break;
        default:
            break;
        }
    }
    imvc++;
    if(rFrameL != -1)
    {
        pmv[imvc][0] = block_available_left ? tmp_mv[pu_b8_y - 0][4 + pu_b8_x - 1][0] : 0;
        pmv[imvc][1] = block_available_left ? tmp_mv[pu_b8_y - 0][4 + pu_b8_x - 1][1] : 0;
        imvc++;
    }
    if (rFrameU != -1) {
        pmv[imvc][0] = block_available_up ? tmp_mv[pu_b8_y - 1][4 + pu_b8_x][0] : 0;
        pmv[imvc][1] = block_available_up ? tmp_mv[pu_b8_y - 1][4 + pu_b8_x][1] : 0;
        imvc++;
    }
    if (rFrameUR != -1) {
        for (hv = 0; hv < 2; hv++) {
            mv_d = block_available_up_left ? tmp_mv[pu_b8_y - 1][4 + pu_b8_x - 1][hv] : 0;
            mv_c = block_available_up_right ? tmp_mv[pu_b8_y - 1][4 + pu_b8_x + (bsize_x >> 3)][hv] : mv_d;

            pmv[imvc][hv] = mv_c;
        }
        imvc++;
    }
    pmv[imvc][0] = 0;
    pmv[imvc][1] = 0;
    imvc++;

    return imvc;
}

_inline int Tz_PartCalMad( pel_t *ref_pic,pel_t* orig_val, int stride, pel_t *( *get_ref_line )( int, pel_t*, int, int ), int mode, int mcost, int min_mcost, int cand_x, int cand_y )
{
    int y, x4, index_pos = 0;
    int blocksize_y   = ( blc_size[mode][1] << 3 );
    int blocksize_x   = ( blc_size[mode][0] << 3 );
    int blocksize_x4 = blocksize_x >> 2;
    pel_t *orig_line, *ref_line;
    for ( y = 0; y < blocksize_y; y++ )
    {
        ref_line  = get_ref_line( blocksize_x, ref_pic, cand_y + y, cand_x );
        index_pos = y * stride;
        orig_line = orig_val + index_pos;

        for ( x4 = 0; x4 < blocksize_x4; x4++ )
        {
            mcost += byte_abs[ *orig_line++ - *ref_line++ ];
            mcost += byte_abs[ *orig_line++ - *ref_line++ ];
            mcost += byte_abs[ *orig_line++ - *ref_line++ ];
            mcost += byte_abs[ *orig_line++ - *ref_line++ ];
        }
        if ( mcost >= min_mcost )
        {
            break;
        }
    }
    return mcost;
}

_inline int TZ_MHMC_PartCalMad(pel_t *ref_pic, pel_t* fw_ref_pic, pel_t* orig_val, int stride, pel_t *(*get_ref_line)(int, pel_t*, int, int), int mode, int mcost, int min_mcost, int cand_x, int cand_y, int pic_pix_x, int pic_pix_y, int fw_mv_x, int fw_mv_y)
{
    int y, x4;
    int ry, rx;
    int index_pos = 0;
    int blocksize_x = (blc_size[mode][0] << 3);
    int blocksize_y = (blc_size[mode][1] << 3);
    int blocksize_x4 = blocksize_x >> 2;
    pel_t *orig_line, *ref_line;
    for (y = 0; y < blocksize_y; y++) {
        ry = ((pic_pix_y + y) << 2) + fw_mv_y;
        ref_line = get_ref_line(blocksize_x, ref_pic, cand_y + y, cand_x);
        index_pos = y * stride;
        orig_line = orig_val + index_pos;

        for (x4 = 0; x4 < blocksize_x4; x4++) {
            rx = ((pic_pix_x + x4 * 4) << 2) + fw_mv_x;
            mcost += byte_abs[*orig_line++ - ((*ref_line++) + Fw_PelY_14(fw_ref_pic, ry, rx)) / 2];
            mcost += byte_abs[*orig_line++ - ((*ref_line++) + Fw_PelY_14(fw_ref_pic, ry, rx + 4)) / 2];
            mcost += byte_abs[*orig_line++ - ((*ref_line++) + Fw_PelY_14(fw_ref_pic, ry, rx + 8)) / 2];
            mcost += byte_abs[*orig_line++ - ((*ref_line++) + Fw_PelY_14(fw_ref_pic, ry, rx + 12)) / 2];
        }
        if (mcost >= min_mcost) {
            break;
        }
    }
    return mcost;
}

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
    int       min_mcost)   // <--  minimum motion cost (cost for center or huge value)
{
    static int cross_points_x[4] = { 0, 1, 0, -1 };
    static int cross_points_y[4] = { -1, 0, 1, 0 };
    static int square_points_x[4] = { -1, 1, 1, -1 };
    static int square_points_y[4] = { -1, -1, 1, 1 };

    int   pos, cand_x, cand_y, mcost;
    int   search_range = input->search_range;
    pel_t *(*get_ref_line)(int, pel_t *, int, int);
    pel_t  *ref_pic = img->type == B_IMG ? Refbuf11[ref + 1] : Refbuf11[ref];
    int   lambda_factor = LAMBDA_FACTOR(sqrt(img->lambda));               // factor for determining lagragian motion cost
    int   mvshift = 2;                  // motion vector shift for getting sub-pel units
    int   pred_x = (pic_pix_x << mvshift) + mvc[0][0];     // position x (in sub-pel units)
    int   pred_y = (pic_pix_y << mvshift) + mvc[0][1];     // position y (in sub-pel units)
    int   center_x = pic_pix_x + mvc[0][0] / 4;                        // center position x (in pel units)
    int   center_y = pic_pix_y + mvc[0][1] / 4;                        // center position y (in pel units)
    int    best_x = 0, best_y = 0;
    int   search_step, iYMinNow, iXMinNow;
    int   i, j, m;
    int   height = img->height;
    /* mvc[0][] is the MVP */
    // set function for getting reference picture lines
    if ((center_x > search_range) && (center_x < img->width - 1 - search_range - (blc_size[mode][0] << 3)) &&
        (center_y > search_range) && (center_y < height - 1 - search_range - (blc_size[mode][1] << 3))) {
        get_ref_line = FastLineX;
    }
    else {
        get_ref_line = UMVLineX;
    }
    cand_x = center_x;
    cand_y = center_y;
    mcost = MV_COST(lambda_factor, mvshift, cand_x, cand_y, pred_x, pred_y);
    if (ref != -1) {
        mcost += REF_COST(lambda_factor, ref);
    }
    int i_mvc_start = 1;
    mcost = Tz_PartCalMad(ref_pic, orig_val, stride, get_ref_line, mode, mcost, min_mcost, cand_x, cand_y);
    McostState[search_range][search_range] = mcost;
    if (mcost < min_mcost) {
        min_mcost = mcost;
        best_x = cand_x;
        best_y = cand_y;
    }

    iXMinNow = best_x;
    iYMinNow = best_y;
    for (m = 0; m < 4; m++) {
        cand_x = iXMinNow + cross_points_x[m];
        cand_y = iYMinNow + cross_points_y[m];
        TZ_SEARCH_ONE_PIXEL
    }

    if (center_x != pic_pix_x || center_y != pic_pix_y) {
        cand_x = pic_pix_x;
        cand_y = pic_pix_y;
        TZ_SEARCH_ONE_PIXEL

        iXMinNow = cand_x;
        iYMinNow = cand_y;
        for (m = 0; m < 4; m++) {
            cand_x = iXMinNow + cross_points_x[m];
            cand_y = iYMinNow + cross_points_y[m];
            TZ_SEARCH_ONE_PIXEL
        }
    }

    for (i = i_mvc_start; i < i_mvc; i++) {
        cand_x = pic_pix_x + mvc[i][0] / 4;
        cand_y = pic_pix_y + mvc[i][1] / 4;
        TZ_SEARCH_ONE_PIXEL
    }
    /* 当前最优MV不是 MVP，搜索其周围一个小窗口 */
    if (MAKEDWORD(center_x - pic_pix_x, center_y - pic_pix_y) != MAKEDWORD(best_x - pic_pix_x, best_y - pic_pix_y)) {
        iXMinNow = best_x;
        iYMinNow = best_y;
        for (m = 0; m < 4; m++) {
            cand_x = iXMinNow + cross_points_x[m];
            cand_y = iYMinNow + cross_points_y[m];
            TZ_SEARCH_ONE_PIXEL
        }
    }

    /* TZ search step2 */
    const int RasterDistance = 16;
    const int EarlyExitIters = 3;
    int bdist, dir;
    mv_info mvinfo;
    int tz_iters = 0;
    int i_dist;

    mvinfo.bcost = min_mcost;
    mvinfo.bdist = 0;
    mvinfo.bdir = 0;

    /* direction */
    /*     2     */
    /*   4 * 5   */
    /*     7     */

    iXMinNow = best_x;
    iYMinNow = best_y;
    for (m = 0; m < 4; m++) {
        cand_x = iXMinNow + cross_points_x[m];
        cand_y = iYMinNow + cross_points_y[m];
        TZ_SEARCH_ONE_PIXEL_X4(i_dist, 1)
    }
    if (mvinfo.bcost < min_mcost) {
        tz_iters = 0;
    }
    else {
        ++tz_iters;
    }
    for (i_dist = 2; i_dist <= search_range; i_dist <<= 1) {
        /*          2           points 2, 4, 5, 7 are i_dist
        *        1   3         points 1, 3, 6, 8 are i_dist/2
        *      4   *   5
        *        6   8
        *          7           */
        int i_dist2 = i_dist >> 1;
        iXMinNow = best_x;
        iYMinNow = best_y;
        for (m = 0; m < 4; m++) {
            cand_x = iXMinNow + square_points_x[m] * i_dist2;
            cand_y = iYMinNow + square_points_y[m] * i_dist2;
            TZ_SEARCH_ONE_PIXEL_X4(i_dist, m + 1)
            cand_x = iXMinNow + cross_points_x[m] * i_dist;
            cand_y = iYMinNow + cross_points_y[m] * i_dist;
            TZ_SEARCH_ONE_PIXEL_X4(i_dist, m + 2)
        }
        if (mvinfo.bcost < min_mcost) {
            tz_iters = 0;
        }
        else if (++tz_iters >= EarlyExitIters) {
            break;
        }
    }
    min_mcost = mvinfo.bcost;
    bdist = mvinfo.bdist;
    dir = mvinfo.bdir;

    if (bdist == 1) {
        goto step_3;
    }
    /* raster search refinement if original search distance was too big */
    if (bdist > RasterDistance) {
        const int iRasterDist = RasterDistance >> 1;
        const int iRasterDist2 = RasterDistance >> 2;
        int rmv_y_min = best_y - RasterDistance + 2;
        int rmv_y_max = best_y + RasterDistance - 2;
        int rmv_x_min = best_x - RasterDistance + 2;
        int rmv_x_max = best_x + RasterDistance - 2;
        for (j = rmv_y_min; j < rmv_y_max; j += iRasterDist2) {
            for (i = rmv_x_min; i < rmv_x_max; i += iRasterDist2) {
                cand_x = i;
                cand_y = j;
                TZ_SEARCH_ONE_PIXEL
            }
        }
        bdist = iRasterDist;
    }
    else {
        bdist = mvinfo.bdist;
    }

    mvinfo.bcost = min_mcost;
    mvinfo.bdist = 0;
    mvinfo.bdir = 0;

    /* raster refinement */
    for (i_dist = bdist >> 1; i_dist > 1; i_dist >>= 1) {
        int i_dist2 = i_dist >> 1;
        iXMinNow = best_x;
        iYMinNow = best_y;
        for (m = 0; m < 4; m++) {
            cand_x = iXMinNow + square_points_x[m] * i_dist2;
            cand_y = iYMinNow + square_points_y[m] * i_dist2;
            TZ_SEARCH_ONE_PIXEL_X4(i_dist, m + 1)
            cand_x = iXMinNow + cross_points_x[m] * i_dist;
            cand_y = iYMinNow + cross_points_y[m] * i_dist;
            TZ_SEARCH_ONE_PIXEL_X4(i_dist, m + 2)
        }
    }
    iXMinNow = best_x;
    iYMinNow = best_y;
    for (m = 0; m < 4; m++) {
        cand_x = iXMinNow + cross_points_x[m];
        cand_y = iYMinNow + cross_points_y[m];
        TZ_SEARCH_ONE_PIXEL_X4(1, m + 2)
    }

    /* star refinement */
    if (mvinfo.bdist > 0) {
        iXMinNow = best_x;
        iYMinNow = best_y;
        for (m = 0; m < 4; m++) {
            cand_x = iXMinNow + cross_points_x[m];
            cand_y = iYMinNow + cross_points_y[m];
            TZ_SEARCH_ONE_PIXEL_X4(1, m + 2)
        }
        for (i_dist = 2; i_dist <= 8; i_dist <<= 1) {
            int i_dist2 = i_dist >> 1;
            iXMinNow = best_x;
            iYMinNow = best_y;
            for (m = 0; m < 4; m++) {
                cand_x = iXMinNow + square_points_x[m] * i_dist2;
                cand_y = iYMinNow + square_points_y[m] * i_dist2;
                TZ_SEARCH_ONE_PIXEL_X4(i_dist, m + 1)
                cand_x = iXMinNow + cross_points_x[m] * i_dist;
                cand_y = iYMinNow + cross_points_y[m] * i_dist;
                TZ_SEARCH_ONE_PIXEL_X4(i_dist, m + 2)
            }
        }
    }

    min_mcost = mvinfo.bcost;
    bdist = mvinfo.bdist;
    dir = mvinfo.bdir;

    int iAbort = 0;
step_3: //the third step with a small search pattern
    iXMinNow = best_x;
    iYMinNow = best_y;
    for (i = 0; i < search_range; i++) {
        iAbort = 1;
        for (m = 0; m < 4; m++) {
            cand_x = iXMinNow + cross_points_x[m];
            cand_y = iYMinNow + cross_points_y[m];
            TZ_SEARCH_ONE_PIXEL1(0)
        }
        if (iAbort) {
            break;
        }
        iXMinNow = best_x;
        iYMinNow = best_y;
    }

    *mv_x = best_x - pic_pix_x;
    *mv_y = best_y - pic_pix_y;
    return min_mcost;
}

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
    int       min_mcost)   // <--  minimum motion cost (cost for center or huge value)
{
    static int cross_points_x[4] = { 0, 1, 0, -1 };
    static int cross_points_y[4] = { -1, 0, 1, 0 };
    static int square_points_x[4] = { -1, 1, 1, -1 };
    static int square_points_y[4] = { -1, -1, 1, 1 };

    int   pos, cand_x, cand_y, mcost;
    int   search_range = input->search_range;
    pel_t *(*get_ref_line)(int, pel_t *, int, int);
    pel_t  *ref_pic = img->type == B_IMG ? Refbuf11[ref + 1] : Refbuf11[ref];
    pel_t*  fw_ref_pic = mref[0];
    int   lambda_factor = LAMBDA_FACTOR(sqrt(img->lambda));               // factor for determining lagragian motion cost
    int   mvshift = 2;                  // motion vector shift for getting sub-pel units
    int   blocksize_y = (blc_size[mode][1] << 3);          // vertical block size
    int   blocksize_x = (blc_size[mode][0] << 3);          // horizontal block size
    int   pred_x = (pic_pix_x << mvshift) + mvc[0][0];     // position x (in sub-pel units)
    int   pred_y = (pic_pix_y << mvshift) + mvc[0][1];     // position y (in sub-pel units)
    int   center_x = pic_pix_x + mvc[0][0] / 4;                        // center position x (in pel units)
    int   center_y = pic_pix_y + mvc[0][1] / 4;                        // center position y (in pel units)
    int    best_x = 0, best_y = 0;
    int   search_step, iYMinNow, iXMinNow;
    int   i, j, m;
    int   pic4_pix_x = (pic_pix_x << 2);
    int   pic4_pix_y = (pic_pix_y << 2);
    int   max_pos_x4 = ((img->width - blocksize_x + 1) << 2);
    int   max_pos_y4 = ((img->height - blocksize_y + 1) << 2);
    int   height = img->height;
    /* mvc[0][] is the MVP */
    // set function for getting reference picture lines
    if ((center_x > search_range) && (center_x < img->width - 1 - search_range - blocksize_x) &&
        (center_y > search_range) && (center_y < height - 1 - search_range - blocksize_y)){
        get_ref_line = FastLineX;
    }
    else {
        get_ref_line = UMVLineX;
    }
    if ((pic4_pix_x + *fw_mv_x > 1) && (pic4_pix_x + *fw_mv_x < max_pos_x4 - 2) &&
        (pic4_pix_y + *fw_mv_y > 1) && (pic4_pix_y + *fw_mv_y < max_pos_y4 - 2)) {
        Fw_PelY_14 = FastPelY_14;
    }
    else {
        Fw_PelY_14 = UMVPelY_14;
    }
    cand_x = center_x;
    cand_y = center_y;
    mcost = MV_COST(lambda_factor, mvshift, cand_x, cand_y, pred_x, pred_y);
    if (ref != -1) {
        mcost += REF_COST(lambda_factor, ref);
    }
    int i_mvc_start = 1;
    mcost = TZ_MHMC_PartCalMad(ref_pic, fw_ref_pic, orig_val, stride, get_ref_line, mode, mcost, min_mcost, cand_x, cand_y, pic_pix_x, pic_pix_y, *fw_mv_x, *fw_mv_y);
    McostState[search_range][search_range] = mcost;
    if (mcost < min_mcost) {
        min_mcost = mcost;
        best_x = cand_x;
        best_y = cand_y;
    }

    iXMinNow = best_x;
    iYMinNow = best_y;
    for (m = 0; m < 4; m++) {
        cand_x = iXMinNow + cross_points_x[m];
        cand_y = iYMinNow + cross_points_y[m];
        TZ_MHMC_SEARCH_ONE_PIXEL
    }

    if (center_x != pic_pix_x || center_y != pic_pix_y) {
        cand_x = pic_pix_x;
        cand_y = pic_pix_y;
        TZ_MHMC_SEARCH_ONE_PIXEL

            iXMinNow = cand_x;
        iYMinNow = cand_y;
        for (m = 0; m < 4; m++) {
            cand_x = iXMinNow + cross_points_x[m];
            cand_y = iYMinNow + cross_points_y[m];
            TZ_MHMC_SEARCH_ONE_PIXEL
        }
    }

    for (i = i_mvc_start; i < i_mvc; i++) {
        cand_x = pic_pix_x + mvc[i][0] / 4;
        cand_y = pic_pix_y + mvc[i][1] / 4;
        TZ_MHMC_SEARCH_ONE_PIXEL
    }
    /* 当前最优MV不是 MVP，搜索其周围一个小窗口 */
    if (MAKEDWORD(center_x - pic_pix_x, center_y - pic_pix_y) != MAKEDWORD(best_x - pic_pix_x, best_y - pic_pix_y)) {
        iXMinNow = best_x;
        iYMinNow = best_y;
        for (m = 0; m < 4; m++) {
            cand_x = iXMinNow + cross_points_x[m];
            cand_y = iYMinNow + cross_points_y[m];
            TZ_MHMC_SEARCH_ONE_PIXEL
        }
    }

    /* TZ search step2 */
    const int RasterDistance = 16;
    const int EarlyExitIters = 3;
    int bdist, dir;
    mv_info mvinfo;
    int tz_iters = 0;
    int i_dist;

    mvinfo.bcost = min_mcost;
    mvinfo.bdist = 0;
    mvinfo.bdir = 0;

    /* direction */
    /*     2     */
    /*   4 * 5   */
    /*     7     */

    iXMinNow = best_x;
    iYMinNow = best_y;
    for (m = 0; m < 4; m++) {
        cand_x = iXMinNow + cross_points_x[m];
        cand_y = iYMinNow + cross_points_y[m];
        TZ_MHMC_SEARCH_ONE_PIXEL_X4(i_dist, 1)
    }
    if (mvinfo.bcost < min_mcost) {
        tz_iters = 0;
    }
    else {
        ++tz_iters;
    }
    for (i_dist = 2; i_dist <= search_range; i_dist <<= 1) {
        /*          2           points 2, 4, 5, 7 are i_dist
        *        1   3         points 1, 3, 6, 8 are i_dist/2
        *      4   *   5
        *        6   8
        *          7           */
        int i_dist2 = i_dist >> 1;
        iXMinNow = best_x;
        iYMinNow = best_y;
        for (m = 0; m < 4; m++) {
            cand_x = iXMinNow + square_points_x[m] * i_dist2;
            cand_y = iYMinNow + square_points_y[m] * i_dist2;
            TZ_MHMC_SEARCH_ONE_PIXEL_X4(i_dist, m + 1)
                cand_x = iXMinNow + cross_points_x[m] * i_dist;
            cand_y = iYMinNow + cross_points_y[m] * i_dist;
            TZ_MHMC_SEARCH_ONE_PIXEL_X4(i_dist, m + 2)
        }
        if (mvinfo.bcost < min_mcost) {
            tz_iters = 0;
        }
        else if (++tz_iters >= EarlyExitIters) {
            break;
        }
    }
    min_mcost = mvinfo.bcost;
    bdist = mvinfo.bdist;
    dir = mvinfo.bdir;

    if (bdist == 1) {
        goto step_3;
    }
    /* raster search refinement if original search distance was too big */
    if (bdist > RasterDistance) {
        const int iRasterDist = RasterDistance >> 1;
        const int iRasterDist2 = RasterDistance >> 2;
        int rmv_y_min = best_y - RasterDistance + 2;
        int rmv_y_max = best_y + RasterDistance - 2;
        int rmv_x_min = best_x - RasterDistance + 2;
        int rmv_x_max = best_x + RasterDistance - 2;
        for (j = rmv_y_min; j < rmv_y_max; j += iRasterDist2) {
            for (i = rmv_x_min; i < rmv_x_max; i += iRasterDist2) {
                cand_x = i;
                cand_y = j;
                TZ_MHMC_SEARCH_ONE_PIXEL
            }
        }
        bdist = iRasterDist;
    }
    else {
        bdist = mvinfo.bdist;
    }

    mvinfo.bcost = min_mcost;
    mvinfo.bdist = 0;
    mvinfo.bdir = 0;

    /* raster refinement */
    for (i_dist = bdist >> 1; i_dist > 1; i_dist >>= 1) {
        int i_dist2 = i_dist >> 1;
        iXMinNow = best_x;
        iYMinNow = best_y;
        for (m = 0; m < 4; m++) {
            cand_x = iXMinNow + square_points_x[m] * i_dist2;
            cand_y = iYMinNow + square_points_y[m] * i_dist2;
            TZ_MHMC_SEARCH_ONE_PIXEL_X4(i_dist, m + 1)
                cand_x = iXMinNow + cross_points_x[m] * i_dist;
            cand_y = iYMinNow + cross_points_y[m] * i_dist;
            TZ_MHMC_SEARCH_ONE_PIXEL_X4(i_dist, m + 2)
        }
    }
    iXMinNow = best_x;
    iYMinNow = best_y;
    for (m = 0; m < 4; m++) {
        cand_x = iXMinNow + cross_points_x[m];
        cand_y = iYMinNow + cross_points_y[m];
        TZ_MHMC_SEARCH_ONE_PIXEL_X4(1, m + 2)
    }

    /* star refinement */
    if (mvinfo.bdist > 0) {
        iXMinNow = best_x;
        iYMinNow = best_y;
        for (m = 0; m < 4; m++) {
            cand_x = iXMinNow + cross_points_x[m];
            cand_y = iYMinNow + cross_points_y[m];
            TZ_MHMC_SEARCH_ONE_PIXEL_X4(1, m + 2)
        }
        for (i_dist = 2; i_dist <= 8; i_dist <<= 1) {
            int i_dist2 = i_dist >> 1;
            iXMinNow = best_x;
            iYMinNow = best_y;
            for (m = 0; m < 4; m++) {
                cand_x = iXMinNow + square_points_x[m] * i_dist2;
                cand_y = iYMinNow + square_points_y[m] * i_dist2;
                TZ_MHMC_SEARCH_ONE_PIXEL_X4(i_dist, m + 1)
                    cand_x = iXMinNow + cross_points_x[m] * i_dist;
                cand_y = iYMinNow + cross_points_y[m] * i_dist;
                TZ_MHMC_SEARCH_ONE_PIXEL_X4(i_dist, m + 2)
            }
        }
    }

    min_mcost = mvinfo.bcost;
    bdist = mvinfo.bdist;
    dir = mvinfo.bdir;

    int iAbort = 0;
step_3: //the third step with a small search pattern
    iXMinNow = best_x;
    iYMinNow = best_y;
    for (i = 0; i < search_range; i++) {
        iAbort = 1;
        for (m = 0; m < 4; m++) {
            cand_x = iXMinNow + cross_points_x[m];
            cand_y = iYMinNow + cross_points_y[m];
            TZ_MHMC_SEARCH_ONE_PIXEL1(0)
        }
        if (iAbort) {
            break;
        }
        iXMinNow = best_x;
        iYMinNow = best_y;
    }

    *mv_x = best_x - pic_pix_x;
    *mv_y = best_y - pic_pix_y;
    return min_mcost;
}
