/**************************************************************************************
* Function: Macro definitions and global variables for fast integer pel motion
            estimation and fractional pel motion estimation
**************************************************************************************/
#ifndef _FAST_ME_H_
#define _FAST_ME_H_
#include "global.h"

#define EARLY_TERMINATION                                                \
    if(ref>0) {                                                          \
        if ((min_mcost-pred_SAD_ref)<pred_SAD_ref*betaThird)             \
           goto third_step;                                              \
        else if((min_mcost-pred_SAD_ref)<pred_SAD_ref*betaSec)           \
           goto sec_step;                                                \
    } else if(mode>1) {                                                  \
        if ((min_mcost-pred_SAD_uplayer)<pred_SAD_uplayer*betaThird) {   \
            goto third_step;                                             \
        } else if((min_mcost-pred_SAD_uplayer)<pred_SAD_uplayer*betaSec) \
            goto sec_step;                                               \
    } else {                                                             \
        if ((min_mcost-pred_SAD_space)<pred_SAD_space*betaThird) {       \
            goto third_step;                                             \
        } else if((min_mcost-pred_SAD_space)<pred_SAD_space*betaSec)     \
            goto sec_step;                                               \
    }


#define SEARCH_ONE_PIXEL                                                                                         \
    if(abs(cand_x - center_x) <=search_range && abs(cand_y - center_y)<= search_range) {                         \
        if(!McostState[cand_y-center_y+search_range][cand_x-center_x+search_range]) {                            \
            mcost = MV_COST (lambda_factor, mvshift, cand_x, cand_y, pred_x, pred_y);                            \
            if (ref != -1) {                                                                                     \
                mcost += REF_COST(lambda_factor, ref);                                                           \
            }                                                                                                    \
            mcost = PartCalMad(ref_pic, orig_val, stride, get_ref_line, mode, mcost, min_mcost, cand_x, cand_y); \
            McostState[cand_y-center_y+search_range][cand_x-center_x+search_range] = mcost;                      \
            if (mcost < min_mcost) {                                                                             \
                best_x = cand_x;                                                                                 \
                best_y = cand_y;                                                                                 \
                min_mcost = mcost;                                                                               \
            }                                                                                                    \
        }                                                                                                        \
    }

#define MHMC_SEARCH_ONE_PIXEL                                                                                                                                             \
    if(abs(cand_x - center_x) <=search_range && abs(cand_y - center_y)<= search_range) {                                                                                  \
        if(!McostState[cand_y-center_y+search_range][cand_x-center_x+search_range]) {                                                                                     \
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
        }                                                                                                                                                                 \
    }

#define SEARCH_ONE_PIXEL1(value_iAbort)                                                                          \
    if(abs(cand_x - center_x) <=search_range && abs(cand_y - center_y)<= search_range){                          \
        if(!McostState[cand_y-center_y+search_range][cand_x-center_x+search_range])                              \
        {                                                                                                        \
            mcost = MV_COST (lambda_factor, mvshift, cand_x, cand_y, pred_x, pred_y);                            \
            if (ref != -1) {                                                                                     \
                mcost += REF_COST(lambda_factor, ref);                                                           \
            }                                                                                                    \
            mcost = PartCalMad(ref_pic, orig_val, stride, get_ref_line, mode, mcost, min_mcost, cand_x, cand_y); \
            McostState[cand_y-center_y+search_range][cand_x-center_x+search_range] = mcost;                      \
            if (mcost < min_mcost) {                                                                             \
                best_x = cand_x;                                                                                 \
                best_y = cand_y;                                                                                 \
                min_mcost = mcost;                                                                               \
                iAbort = value_iAbort;                                                                           \
            }                                                                                                    \
        }                                                                                                        \
    }

#define MHMC_SEARCH_ONE_PIXEL1(value_iAbort)                                                                                                                               \
    if(abs(cand_x - center_x) <=search_range && abs(cand_y - center_y)<= search_range){                                                                                    \
        if(!McostState[cand_y-center_y+search_range][cand_x-center_x+search_range]) {                                                                                      \
            mcost = MV_COST (lambda_factor, mvshift, cand_x, cand_y, pred_x, pred_y);                                                                                      \
            if (ref != -1) {                                                                                                                                               \
                mcost += REF_COST(lambda_factor, ref);                                                                                                                     \
            }                                                                                                                                                              \
            mcost = MHMC_PartCalMad(ref_pic, fw_ref_pic, orig_val, stride, get_ref_line, mode, mcost, min_mcost, cand_x, cand_y, pic_pix_x, pic_pix_y, *fw_mv_x, *fw_mv_y);\
            McostState[cand_y-center_y+search_range][cand_x-center_x+search_range] = mcost;                                                                                \
            if (mcost < min_mcost) {                                                                                                                                       \
                best_x = cand_x;                                                                                                                                           \
                best_y = cand_y;                                                                                                                                           \
                min_mcost = mcost;                                                                                                                                         \
                iAbort = value_iAbort;                                                                                                                                     \
            }                                                                                                                                                              \
        }                                                                                                                                                                  \
    }

int **McostState; //state for integer pel search

int *****all_bwmincost;//store for backward prediction
int pred_SAD_space, pred_SAD_ref, pred_SAD_uplayer;//SAD prediction
int pred_MV_time[2], pred_MV_ref[2], pred_MV_uplayer[2];//pred motion vector by space or temporal correlation,Median is provided

//for half_stop
float Bsize[8];
float AlphaSec[8];
float AlphaThird[8];
int flag_intra[625];
int flag_intra_SAD;


pel_t **SearchState; //state for fractional pel search
void DefineThreshold();
void DefineThresholdMB();
int get_mem_mincost( int ** ** **mv );
int get_mem_bwmincost( int ** ** **mv );
int get_mem_FME();
void free_mem_mincost( int ** *** mv );
void free_mem_bwmincost( int ** *** mv );
void free_mem_FME();
void decide_intrabk_SAD();

void reset_mincost( ImgParams *img, int best_mode );

int                                     //  ==> minimum motion cost after search
FastIntegerMVSearch  ( pel_t*   orig_val,    // <--  not used
                                   int       stride,
                                   int       ref,          // <--  reference frame (0... or -1 (backward))
                                   int       pic_pix_x,    // <--  absolute x-coordinate of regarded AxB block
                                   int       pic_pix_y,    // <--  absolute y-coordinate of regarded AxB block
                                   int       mode,         // <--  block type (1-16x16 ... 7-4x4)
                                   int       pred_mv_x,    // <--  motion vector predictor (x) in sub-pel units
                                   int       pred_mv_y,    // <--  motion vector predictor (y) in sub-pel units
                                   int      *mv_x,         //  --> motion vector (x) - in pel units
                                   int      *mv_y,         //  --> motion vector (y) - in pel units
                                   int       min_mcost );  // <--  minimum motion cost (cost for center or huge value)

int                                     //  ==> minimum motion cost after search
FastIntegerMVSearchMhp  ( pel_t*   orig_val,    // <--  not used
                                       int       stride,
                                       int       ref,          // <--  reference frame (0... or -1 (backward))
                                       int       pic_pix_x,    // <--  absolute x-coordinate of regarded AxB block
                                       int       pic_pix_y,    // <--  absolute y-coordinate of regarded AxB block
                                       int       mode,         // <--  block type (1-16x16 ... 7-4x4)
                                       int       pred_mv_x,    // <--  motion vector predictor (x) in sub-pel units
                                       int       pred_mv_y,    // <--  motion vector predictor (y) in sub-pel units
                                       int      *mv_x,         //  --> motion vector (x) - in pel units
                                       int      *mv_y,         //  --> motion vector (y) - in pel units
                                       int      *fw_mv_x,      //  --> forward motion vector (x) - in pel units
                                       int      *fw_mv_y,      //  --> forward motion vector (y) - in pel units
                                       int       min_mcost );  // <--  minimum motion cost (cost for center or huge value)

int CalculateSADCost( int pic_pix_x, int pic_pix_y, int mode,
                     int cand_mv_x,int cand_mv_y, pel_t *ref_pic, pel_t* orig_val, int stride, int mv_mcost );

int CalculateSADCostMhp( int pic_pix_x, int pic_pix_y, int mode,
                         int cand_mv_x,int cand_mv_y, int fw_mv_x, int fw_mv_y, pel_t *ref_pic, pel_t* orig_val, int stride, int mv_mcost );

int CalculateSADCostSym( int pic_pix_x,int pic_pix_y,int mode,
                         int cand_mv_x,int cand_mv_y, pel_t *ref_pic, pel_t *ref_pic_bid, pel_t *orig_val, int stride,
                         int Mvmcost);

int                                                   //  ==> minimum motion cost after search
CrossFractionalMVSearchSAD ( pel_t*   orig_val,     // <--  original pixel values for the AxB block
                              int       stride,
                              int       ref,           // <--  reference frame (0... or -1 (backward))
                              int       pic_pix_x,     // <--  absolute x-coordinate of regarded AxB block
                              int       pic_pix_y,     // <--  absolute y-coordinate of regarded AxB block
                              int       mode,          // <--  block type (1-16x16 ... 7-4x4)
                              int       pred_mv_x,     // <--  motion vector predictor (x) in sub-pel units
                              int       pred_mv_y,     // <--  motion vector predictor (y) in sub-pel units
                              int      *mv_x,          // <--> in: search center (x) / out: motion vector (x) - in pel units
                              int      *mv_y,          // <--> in: search center (y) / out: motion vector (y) - in pel units
                              int       min_mcost );   // <--  minimum motion cost (cost for center or huge value)


int CrossFractionalMVSearchSADMhp ( pel_t*   orig_val,     // <--  original pixel values for the AxB block
                                      int       stride,
                                      int       ref,           // <--  reference frame (0... or -1 (backward))
                                      int       pic_pix_x,     // <--  absolute x-coordinate of regarded AxB block
                                      int       pic_pix_y,     // <--  absolute y-coordinate of regarded AxB block
                                      int       mode,     // <--  block type (1-16x16 ... 7-4x4)
                                      int       pred_mv_x,     // <--  motion vector predictor (x) in sub-pel units
                                      int       pred_mv_y,     // <--  motion vector predictor (y) in sub-pel units
                                      int      *mv_x,          // <--> in: search center (x) / out: motion vector (x) - in pel units
                                      int      *mv_y,          // <--> in: search center (y) / out: motion vector (y) - in pel units
                                      int      *fw_mv_x,       //  --> forward motion vector (x) - in pel units
                                      int      *fw_mv_y,       //  --> forward motion vector (y) - in pel units
                                      int       min_mcost );   // <--  minimum motion cost (cost for center or huge value)

int SquareFractionalMVSearchSATD ( pel_t*   orig_val,     // <--  original pixel values for the AxB block
                              int       stride,
                              int       ref,           // <--  reference frame (0... or -1 (backward))
                              int       pic_pix_x,     // <--  absolute x-coordinate of regarded AxB block
                              int       pic_pix_y,     // <--  absolute y-coordinate of regarded AxB block
                              int       mode,          // <--  block type (1-16x16 ... 7-4x4)
                              int       pred_mv_x,     // <--  motion vector predictor (x) in sub-pel units
                              int       pred_mv_y,     // <--  motion vector predictor (y) in sub-pel units
                              int      *mv_x,          // <--> in: search center (x) / out: motion vector (x) - in pel units
                              int      *mv_y,          // <--> in: search center (y) / out: motion vector (y) - in pel units
                              int       search_pos2,   // <--  search positions for    half-pel search  (default: 9)
                              int       search_pos4,   // <--  search positions for quarter-pel search  (default: 9)
                              int       min_mcost );     // <--  minimum motion cost (cost for center or huge value)


int SquareFractionalMVSearchSATDMhp ( pel_t*   orig_val,     // <--  original pixel values for the AxB block
                                  int       stride,
                                  int       ref,           // <--  reference frame (0... or -1 (backward))
                                  int       pic_pix_x,     // <--  absolute x-coordinate of regarded AxB block
                                  int       pic_pix_y,     // <--  absolute y-coordinate of regarded AxB block
                                  int       mode,          // <--  block type (1-16x16 ... 7-4x4)
                                  int       pred_mv_x,     // <--  motion vector predictor (x) in sub-pel units
                                  int       pred_mv_y,     // <--  motion vector predictor (y) in sub-pel units
                                  int      *mv_x,          // <--> in: search center (x) / out: motion vector (x) - in pel units
                                  int      *mv_y,          // <--> in: search center (y) / out: motion vector (y) - in pel units
                                  int      *fw_mv_x,       //  --> forward motion vector (x) - in pel units
                                  int      *fw_mv_y,       //  --> forward motion vector (y) - in pel units
                                  int       search_pos2,   // <--  search positions for    half-pel search  (default: 9)
                                  int       search_pos4,   // <--  search positions for quarter-pel search  (default: 9)
                                  int       min_mcost );    // <--  minimum motion cost (cost for center or huge value)

int                                               //  ==> minimum motion cost after search
SquareFractionalMVSearchSATDSym ( pel_t*   orig_val,     // <--  original pixel values for the AxB block
                              int       stride,
                              int       ref,           // <--  reference frame (0... or -1 (backward))
                              int       pic_pix_x,     // <--  absolute x-coordinate of regarded AxB block
                              int       pic_pix_y,     // <--  absolute y-coordinate of regarded AxB block
                              int       mode,          // <--  block type (1-16x16 ... 7-4x4)
                              int       pred_mv_x,     // <--  motion vector predictor (x) in sub-pel units
                              int       pred_mv_y,     // <--  motion vector predictor (y) in sub-pel units
                              int      *mv_x,          // <--> in: search center (x) / out: motion vector (x) - in pel units
                              int      *mv_y,          // <--> in: search center (y) / out: motion vector (y) - in pel units
                              int       search_pos2,   // <--  search positions for    half-pel search  (default: 9)
                              int       search_pos4,   // <--  search positions for quarter-pel search  (default: 9)
                              int       min_mcost );    // <--  minimum motion cost (cost for center or huge value)

void SetMotionVectorPredictorME ( ImgParams *img, Macroblock *currMB,
                                     int  pmv[2],
                                     int  **refFrArr,
                                     int  ***tmp_mv,
                                     int  ref_frame,
                                     int  mb_x,
                                     int  mb_y,
                                     int  blockshape_x,
                                     int  mode,
                                     int  ref );

int FastMVSeach ( Macroblock *currMB,
                             int       ref,           // <--  reference frame (0... or -1 (backward))
                             int       pic_pix_x,     // <--  absolute x-coordinate of regarded AxB block
                             int       pic_pix_y,     // <--  absolute y-coordinate of regarded AxB block
                             int       mode );   // <--  block type (1-16x16 ... 7-4x4)


int FastMotionSearchMhp( Macroblock *currMB, int ref, int pu_b8_x, int pu_b8_y, int mode );

int FastMotionSearchSym( Macroblock *currMB, int ref, int pu_b8_x, int pu_b8_y, int mode );

int                                                   //  ==> minimum motion cost after search
CrossFractionalMVSearchSADSym ( pel_t*   orig_val,     // <--  original pixel values for the AxB block
                                  int       stride,
                                  int       ref,           // <--  reference frame (0... or -1 (backward))
                                  int       pic_pix_x,     // <--  absolute x-coordinate of regarded AxB block
                                  int       pic_pix_y,     // <--  absolute y-coordinate of regarded AxB block
                                  int       mode,          // <--  block type (1-16x16 ... 7-4x4)
                                  int       pred_mv_x,     // <--  motion vector predictor (x) in sub-pel units
                                  int       pred_mv_y,     // <--  motion vector predictor (y) in sub-pel units
                                  int      *mv_x,          // <--> in: search center (x) / out: motion vector (x) - in pel units
                                  int      *mv_y,          // <--> in: search center (y) / out: motion vector (y) - in pel units
                                  int       min_mcost );   // <--  minimum motion cost (cost for center or huge value)

#endif