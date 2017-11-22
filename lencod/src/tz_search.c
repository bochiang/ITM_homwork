#include "global.h"
#include "fast_me.h"

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
    int rFrameL, rFrameU, rFrameUR, rFrameUL;
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
    if((rFrameL != -1) && (rFrameU != -1) && (rFrameUR != -1))
    {
        for (hv = 0; hv < 2; hv++) {
            mv_a = block_available_left ? tmp_mv[pu_b8_y - 0][4 + pu_b8_x - 1][hv] : 0;
            mv_b = block_available_up ? tmp_mv[pu_b8_y - 1][4 + pu_b8_x][hv] : 0;
            mv_d = block_available_up_left ? tmp_mv[pu_b8_y - 1][4 + pu_b8_x - 1][hv] : 0;
            mv_c = block_available_up_right ? tmp_mv[pu_b8_y - 1][4 + pu_b8_x + (bsize_x >> 3)][hv] : mv_d;

            mv_c = block_available_up_right ? mv_c : mv_d;
            pmv[0][hv] = mid_value(mv_a, mv_b, mv_c);
        }
        imvc ++;
    }
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

}