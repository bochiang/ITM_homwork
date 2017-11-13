#include <math.h>
#include <assert.h>
#include <string.h>

#include "defines.h"
#include "global.h"
#include "elements.h"
#include "macroblock.h"
#include "vlc.h"
#include "AEC.h"
#include "block.h"
#include "transform.h"
#include "biaridecod.h"
#include "../../common/pixel.h"
#include "../../common/intraPrediction.h"
#include "../../common/interPrediction.h"

extern  short IQ_SHIFT[64];
extern  unsigned  short IQ_TAB[64];
extern int DCT_Pairs;

/**************************************************************************
* Function:Checks the availability of neighboring macroblocks of
  the current macroblock for prediction and context determination;
  marks the unavailable MBs for intra prediction in the
  past of the current MB are checked.
* Input:
* Output:
* Return:
* Attention:
**************************************************************************/
void CheckAvailabilityOfNeighbors( ImgParams *img, Macroblock *currMB )
{
    int mb_width = img->PicWidthInMbs;
    int mb_nr = img->current_mb_nr;
    int pix_x   = img->mb_pix_x;
    int pix_y   = img->mb_pix_y;
    currMB->mb_available_up   = NULL;
    currMB->mb_available_left = NULL;

    // Check MB to the left
    if ( pix_x >= MB_SIZE )
    {
        if ( currMB->slice_nr == img->mb_data[mb_nr - 1].slice_nr )
        {
            currMB->mb_available_left = &( img->mb_data[mb_nr - 1] );
        }
    }

    // Check MB above
    if ( pix_y >= MB_SIZE )
    {
        if ( currMB->slice_nr == img->mb_data[mb_nr - mb_width].slice_nr )
        {
            currMB->mb_available_up = &( img->mb_data[mb_nr - mb_width] );
        }
    }

    // Check MB left above
    if ( pix_x >= MB_SIZE && pix_y >= MB_SIZE )
    {
        if ( currMB->slice_nr == img->mb_data[mb_nr - mb_width - 1].slice_nr )
        {
            currMB->mb_available_leftup = &( img->mb_data[mb_nr - mb_width - 1] );
        }
    }

    // Check MB right above
    if ( pix_y >= MB_SIZE && img->mb_pix_x < ( img->width - MB_SIZE ) )
    {
        if ( currMB->slice_nr == img->mb_data[mb_nr - mb_width + 1].slice_nr )
        {
            currMB->mb_available_rightup = &( img->mb_data[mb_nr - mb_width + 1] );
        }
    }

    currMB->mb_addr_left = mb_nr - 1;
    currMB->mb_addr_up = mb_nr - img->PicWidthInMbs;
    currMB->mb_addr_rightup = mb_nr - img->PicWidthInMbs + 1;
    currMB->mb_addr_leftup = mb_nr - img->PicWidthInMbs - 1;
    currMB->mbAvailA = ( currMB->mb_available_left != NULL ) ? 1 : 0;
    currMB->mbAvailB = ( currMB->mb_available_up != NULL ) ? 1 : 0;
    currMB->mbAvailC = ( currMB->mb_available_rightup != NULL ) ? 1 : 0;
    currMB->mbAvailD = ( currMB->mb_available_leftup != NULL ) ? 1 : 0;
}

void init_mb_params( Macroblock *currMB )
{
    int i, j;
    assert( img->current_mb_nr >= 0 && img->current_mb_nr < img->max_mb_nr );
    CheckAvailabilityOfNeighbors( img, currMB );

    currMB->mb_type     = 0;
    currMB->cbp         = 0;
    currMB->c_ipred_mode = DC_PRED_8;

    for ( i = 0; i < 4; i++ )
    {
        currMB->sub_mb_trans_type[i] = 0;
    }

    for ( j = 0; j < 2; j++ )
    {
        for ( i = 0; i < 2; i++ )
        {
            currMB->mvd[0][j][i][0] = 0;
            currMB->mvd[0][j][i][1] = 0;
            currMB->mvd[1][j][i][0] = 0;
            currMB->mvd[1][j][i][1] = 0;
        }
    }
}

void init_img_params( ImgParams *img, int mb_nr )
{
    int i, j;
    img->mb_x = mb_nr % img->PicWidthInMbs;
    img->mb_y = mb_nr / img->PicWidthInMbs;

    img->mb_b8_y = img->mb_y * BLOCK_SIZE / 2;
    img->mb_b8_x = img->mb_x * BLOCK_SIZE / 2;

    img->mb_b4_y = img->mb_y * BLOCK_SIZE;
    img->mb_b4_x = img->mb_x * BLOCK_SIZE;

    img->mb_pix_y = img->mb_y * MB_SIZE;
    img->mb_pix_x = img->mb_x * MB_SIZE;

    img->mb_pix_y_cr = img->mb_y * MB_SIZE / 2;
    img->mb_pix_x_cr   = img->mb_x * MB_SIZE / 2;

    for ( j = 0; j < MB_SIZE; j++ )
    {
        for ( i = 0; i < MB_SIZE; i++ )
        {
            img->resi_blk[i][j] = 0;
        }
    }
}

/**************************************************************************
* Function:Interpret the mb mode for I-Frames
* Input:
* Output:
* Return:
* Attention:
**************************************************************************/
void interpret_mb_mode_I( Macroblock *currMB )
{
    int i;
    currMB->mb_type = I_MB;

    for ( i = 0; i < 4; i++ )
    {
        currMB->b8mode[i] = I_MB;
        currMB->b8pdir[i] = -1;
    }
}

/**************************************************************************
* Function:init macroblock I and P frames
* Input:
* Output:
* Return:
* Attention:
**************************************************************************/
void init_ref_mv( ImgParams *img, Macroblock *currMB, uchar_t uhBitSize )
{
    int i, j, k;
    int b8width = ( 1 << uhBitSize ) / B8_SIZE;
    if( img->type == B_IMG )
    {
        for ( i = 0; i < b8width; i++ )
        {
            for ( j = 0; j < b8width; j++ )
            {
                img->bfrm_fmv[img->mb_b8_y + j][img->mb_b8_x + i + 4][0] = img->bfrm_fmv[img->mb_b8_y + j][img->mb_b8_x + i + 4][1] = 0;
                img->bfrm_bmv[img->mb_b8_y + j][img->mb_b8_x + i + 4][0] = img->bfrm_bmv[img->mb_b8_y + j][img->mb_b8_x + i + 4][1] = 0;
            }
        }

        // Set the reference frame information for motion vector prediction
        if ( IS_INTRA( currMB ) || IS_DIRECT( currMB ) )
        {
            for ( j = 0; j < b8width; j++ )
            {
                for ( i = 0; i < b8width; i++ )
                {
                    img->bfrm_fref[img->mb_b8_y + j][img->mb_b8_x + i] = -1;
                    img->bfrm_bref[img->mb_b8_y + j][img->mb_b8_x + i] = -1;
                }
            }
        }
        else
        {
            for ( j = 0; j < b8width; j++ )
            {
                for ( i = 0; i < b8width; i++ )
                {
                    k = 2 * j + i;
                    img->bfrm_fref[img->mb_b8_y + j][img->mb_b8_x + i] = ( ( currMB->b8pdir[k] == FORWARD || currMB->b8pdir[k] == BSYM ) && currMB->b8mode[k] != PSKIP ? 0 : -1 );
                    img->bfrm_bref[img->mb_b8_y + j][img->mb_b8_x + i] = ( ( currMB->b8pdir[k] == BACKWORD || currMB->b8pdir[k] == BSYM ) && currMB->b8mode[k] != PSKIP ? 0 : -1 );
                }
            }
        }
    }
    else
    {
        for ( i = 0; i < b8width; i++ )
        {
            for ( j = 0; j < b8width; j++ )
            {
                img->pfrm_mv[img->mb_b8_y + j][img->mb_b8_x + i + 4][0] = 0;
                img->pfrm_mv[img->mb_b8_y + j][img->mb_b8_x + i + 4][1] = 0;
                img->bmv_mhp[i][j][0] = 0;
                img->bmv_mhp[i][j][1] = 0;
            }
        }
        // Set the reference frame information for motion vector prediction
        if ( IS_INTRA( currMB ) ) //! these should be removed to initialization
        {
            for ( j = 0; j < b8width; j++ )
            {
                for ( i = 0; i < b8width; i++ )
                {
                    img->pfrm_ref[img->mb_b8_y + j][img->mb_b8_x + i] = -1;
                }
            }
        }
        else if ( !IS_PNxN( currMB ) )
        {
            for ( j = 0; j < b8width; j++ )
            {
                for ( i = 0; i < b8width; i++ )
                {
                    img->pfrm_ref[img->mb_b8_y + j][img->mb_b8_x + i] = 0;
                }
            }
        }
        else
        {
            for ( j = 0; j < b8width; j++ )
            {
                for ( i = 0; i < b8width; i++ )
                {
                    img->pfrm_ref[img->mb_b8_y + j][img->mb_b8_x + i] = ( currMB->b8mode[2 * j + i] == I_MB ? -1 : 0 );
                }
            }
        }
        if ( img->ip_frm_idx > 1 && img->type == P_IMG && ( currMB->mb_type == P2Nx2N || currMB->mb_type == P2NxN || currMB->mb_type == PNx2N || currMB->mb_type == PNxN ) )
        {
            for ( j = 0; j < 2; j++ )
            {
                for ( i = 0; i < 2; i++ )
                {
                    img->pfrm_ref[img->mb_b8_y + j][img->mb_b8_x + i] = currMB->fw_ref[j * 2 + i];
                }
            }
        }
    }
}

void readMVD( ImgParams *img, Macroblock *currMB )
{
    int i, j, k, n;
    int step_h, step_v;
    int curr_mvd[2];
    int blk_step_h = BLOCK_STEP[currMB->mb_type][0];
    int blk_step_v = BLOCK_STEP[currMB->mb_type][1];

    int blk_x, blk_y;
    int b8mode, b8pdir;
    int img_b8_y, img_b8_x;

    //=====  READ FORWARD MOTION VECTORS =====
    for ( blk_y = 0; blk_y < 2; blk_y += blk_step_v )
    {
        for ( blk_x = 0; blk_x < 2; blk_x += blk_step_h )
        {
            k = 2 * blk_y + blk_x;
            b8mode = currMB->b8mode[k];
            b8pdir = currMB->b8pdir[k];
            if( b8mode == I_MB )
            {
                continue;
            }

            step_h   = BLOCK_STEP [b8mode][0];
            step_v   = BLOCK_STEP [b8mode][1];
            // forward MV
            if ( ( b8pdir == FORWARD || b8pdir == BSYM || b8pdir == MHP ) && ( b8mode != PSKIP ) )
            {
                for ( n = 0; n < 2; n++ )
                {
                    readMVD_AEC(currMB, img->cs_aec, k, &curr_mvd[n], n << 1 );
#if TRACE
                    output_trace_info( "FMVD = %3d", curr_mvd[n] );
#endif
                }

                for ( i = 0; i < step_h; i++ )
                {
                    for ( j = 0; j < step_v; j++ )
                    {
                        currMB->mvd[0][blk_y + j][blk_x + i][0] =  curr_mvd[0];
                        currMB->mvd[0][blk_y + j][blk_x + i][1] =  curr_mvd[1];
                    }
                }
            }
        }
    }

    //=====  READ BACKWARD MOTION VECTORS =====
    for ( blk_y = 0; blk_y < 2; blk_y += blk_step_v )
    {
        for ( blk_x = 0; blk_x < 2; blk_x += blk_step_h )
        {
            k = 2 * blk_y + blk_x;
            b8mode = currMB->b8mode[k];
            b8pdir = currMB->b8pdir[k];
            if( b8mode == I_MB )
            {
                continue;
            }
            step_h = BLOCK_STEP[b8mode][0];
            step_v = BLOCK_STEP[b8mode][1];
            img_b8_y = img->mb_b8_y + blk_y;
            img_b8_x = img->mb_b8_x + blk_x;
            if ( ( b8pdir == BACKWORD || b8pdir == BSYM ) && ( b8mode != PSKIP ) ) //has backward vector
            {
                if( b8pdir == BACKWORD )
                {
                    for ( n = 0; n < 2; n++ )
                    {
                        readMVD_AEC(currMB, img->cs_aec, k, &curr_mvd[n], ( n << 1 ) + 1 );
#if TRACE
                        output_trace_info( "BMVD = %3d", curr_mvd[n] );
#endif
                    }
                    for ( i = 0; i < step_h; i++ )
                    {
                        for ( j = 0; j < step_v; j++ )
                        {
                            currMB->mvd[1][blk_y + j][blk_x + i][0] =  curr_mvd[0];
                            currMB->mvd[1][blk_y + j][blk_x + i][1] =  curr_mvd[1];
                        }
                    }
                }
            }
        }
    }
}

void xDecMV( ImgParams *img, Macroblock *currMB )
{
    int i, j, k;
    int step_h, step_v;
    int bframe = ( img->type == B_IMG );

    int blk_step_h = BLOCK_STEP[currMB->mb_type][0];
    int blk_step_v = BLOCK_STEP[currMB->mb_type][1];
    int vec[2];
    int blk_x, blk_y, refframe;
    int pmv[2];
    int b8mode, b8pdir;
    int img_b8_y, img_b8_x;
    int ***frm_fmv = ( bframe ) ? img->bfrm_fmv : img->pfrm_mv;
    int ***frm_bmv = img->bfrm_bmv;
    int **fref = ( bframe ) ? img->bfrm_fref : img->pfrm_ref;
    int **bref = img->bfrm_bref;
    int *fmv, *bmv;

    for ( blk_y = 0; blk_y < 2; blk_y += blk_step_v )
    {
        for ( blk_x = 0; blk_x < 2; blk_x += blk_step_h )
        {
            k = 2 * blk_y + blk_x;
            b8mode = currMB->b8mode[k];
            b8pdir = currMB->b8pdir[k];
            if( b8mode == I_MB )
            {
                continue;
            }
            step_h = BLOCK_STEP [b8mode][0];
            step_v = BLOCK_STEP [b8mode][1];
            img_b8_y = img->mb_b8_y + blk_y;
            img_b8_x = img->mb_b8_x + blk_x;
            // forward MV
            if ( ( b8pdir == FORWARD || b8pdir == BSYM || b8pdir == MHP ) && ( b8mode != PSKIP ) )
            {
                refframe = fref[img_b8_y][img_b8_x];
                SetSpatialMVPredictor( img, currMB, pmv, fref, frm_fmv, refframe, blk_x, blk_y, B8_SIZE * step_h, 0 );

                if( b8pdir == MHP )
                {
                    for ( i = 0; i < step_h; i++ )
                    {
                        for ( j = 0; j < step_v; j++ )
                        {
                            img->bmv_mhp[blk_x+i][blk_y+j][0] = pmv[0];
                            img->bmv_mhp[blk_x+i][blk_y+j][1] = pmv[1];
                        }
                    }
                }

                for ( i = 0; i < step_h; i++ )
                {
                    for ( j = 0; j < step_v; j++ )
                    {
                        frm_fmv[img_b8_y + j][img_b8_x + i + BLOCK_SIZE][0] = currMB->mvd[0][blk_y + j][blk_x + i][0] + pmv[0];
                        frm_fmv[img_b8_y + j][img_b8_x + i + BLOCK_SIZE][1] = currMB->mvd[0][blk_y + j][blk_x + i][1] + pmv[1];
                    }
                }
            }

            if ( img->type == B_IMG && b8mode == PSKIP ) // B direct mode
            {
                img->bfrm_fref[img_b8_y][img_b8_x] = 0;
                img->bfrm_bref[img_b8_y][img_b8_x] = 0;
                fmv = img->bfrm_fmv[img_b8_y][img_b8_x + BLOCK_SIZE];
                bmv = img->bfrm_bmv[img_b8_y][img_b8_x + BLOCK_SIZE];
                GetBdirectMV( img, currMB, 16, fmv, bmv, img_b8_x, img_b8_y );
            }
        }
    }

    for ( blk_y = 0; blk_y < 2; blk_y += blk_step_v )
    {
        for ( blk_x = 0; blk_x < 2; blk_x += blk_step_h )
        {
            k = 2 * blk_y + blk_x;
            b8mode = currMB->b8mode[k];
            b8pdir = currMB->b8pdir[k];
            if( b8mode == I_MB )
            {
                continue;
            }
            step_h = BLOCK_STEP[b8mode][0];
            step_v = BLOCK_STEP[b8mode][1];
            img_b8_y = img->mb_b8_y + blk_y;
            img_b8_x = img->mb_b8_x + blk_x;
            if ( ( b8pdir == BACKWORD || b8pdir == BSYM ) && ( b8mode != PSKIP ) ) //has backward vector
            {
                if( b8pdir == BACKWORD )
                {
                    refframe = bref[img_b8_y][img_b8_x]; // always 0
                    SetSpatialMVPredictor( img, currMB, pmv, bref, frm_bmv, refframe, blk_x, blk_y, B8_SIZE * step_h, -1 );
                    for ( i = 0; i < step_h; i++ )
                    {
                        for ( j = 0; j < step_v; j++ )
                        {
                            frm_bmv[img_b8_y + j][img_b8_x + i + BLOCK_SIZE][0] = currMB->mvd[1][blk_y + j][blk_x + i][0] + pmv[0];
                            frm_bmv[img_b8_y + j][img_b8_x + i + BLOCK_SIZE][1] = currMB->mvd[1][blk_y + j][blk_x + i][1] + pmv[1];
                        }
                    }
                }
                else   // generate the second MV for SYM prediction
                {
                    vec[0] = GenSymBackMV(frm_fmv[img_b8_y][img_b8_x + BLOCK_SIZE][0]);
                    vec[1] = GenSymBackMV(frm_fmv[img_b8_y][img_b8_x + BLOCK_SIZE][1]);
                    for ( i = 0; i < step_h; i++ )
                    {
                        for ( j = 0; j < step_v; j++ )
                        {
                            frm_bmv[img_b8_y + j][img_b8_x + i + BLOCK_SIZE][0] = vec[0];
                            frm_bmv[img_b8_y + j][img_b8_x + i + BLOCK_SIZE][1] = vec[1];
                        }
                    }
                }
            }
        }
    }
}

// Note that partial mb types are generated in "xParsePredInfo"
void xParseMBType(ImgParams *img, Macroblock *currMB)
{
    int i;
    int bframe = (img->type == B_IMG);
    int a, b, act_ctx;

    if (bframe)
    {
        if (currMB->mb_available_up == NULL)
        {
            b = 0;
        }
        else
        {
            b = (((currMB->mb_available_up)->mb_type != 0) ? 1 : 0);
        }
        if (currMB->mb_available_left == NULL)
        {
            a = 0;
        }
        else
        {
            a = (((currMB->mb_available_left)->mb_type != 0) ? 1 : 0);
        }
        act_ctx = a + b;
    }

    readMBPartTypeInfo_AEC(img->cs_aec, &currMB->mb_type, bframe, act_ctx);

    if (bframe)
    {
        // derive the mode of the macroblock
        if (currMB->mb_type == PSKIP)
        {
            for (i = 0; i < 4; i++)
            {
                currMB->b8mode[i] = PSKIP;
                currMB->b8pdir[i] = BSYM;
            }
            currMB->cbp = 0;
        }
        else if (currMB->mb_type == P2Nx2N)
        {
            for (i = 0; i < 4; i++)
            {
                currMB->b8mode[i] = P2Nx2N;
            }
        }
        else if (currMB->mb_type == 2)
        {
            currMB->mb_type = PNx2N;
            for (i = 0; i < 4; i++)
            {
                currMB->b8mode[i] = PNx2N;
            }
        }
        else if (currMB->mb_type == 3)
        {
            currMB->mb_type = P2NxN;
            for (i = 0; i < 4; i++)
            {
                currMB->b8mode[i] = P2NxN;
            }
        }
        else if (currMB->mb_type == 4)        // B_Intra
        {
            currMB->mb_type = I_MB;
            currMB->cbp = 63;
            for (i = 0; i < 4; i++)
            {
                currMB->b8mode[i] = I_MB;
                currMB->b8pdir[i] = -1;
            }
        }
        else if (currMB->mb_type == 5)        // B_8x8
        {
            currMB->mb_type = PNxN;
        }
    }
    else if (img->type == P_IMG)
    {
        // derive the mode of the macroblock
        if (currMB->mb_type == PSKIP) // 2Nx2N or SKIP, which are distinguished by the prediction direction
        {
            currMB->mb_type = PSKIP;
            for (i = 0; i < 4; i++)
            {
                currMB->b8mode[i] = PSKIP;
            }
        }
        else if (currMB->mb_type == 1) // 1 : P_8x16
        {
            currMB->mb_type = PNx2N;
            for (i = 0; i < 4; i++)
            {
                currMB->b8mode[i] = PNx2N;
            }
        }
        else if (currMB->mb_type == 2)  // 2 : P_16x8
        {
            currMB->mb_type = P2NxN;
            for (i = 0; i < 4; i++)
            {
                currMB->b8mode[i] = P2NxN;
            }
        }
        else if (currMB->mb_type == 3)        // P_Intra
        {
            currMB->mb_type = I_MB;
            for (i = 0; i < 4; i++)
            {
                currMB->b8mode[i] = I_MB;
            }
        }
        else if (currMB->mb_type == 4)        // P_8x8
        {
            currMB->mb_type = PNxN;
        }
    }
#if TRACE
    if ((img->type == B_IMG) || currMB->mb_type != PSKIP)
    {
        output_trace_info("MB type = %3d", currMB->mb_type);
    }
#endif
}

void xParsePredInfo(ImgParams *img, Macroblock *currMB)
{
    int i, pred_type;
    if (img->type == B_IMG)
    {
        // derive the prediction direction of macroblock
        if (currMB->mb_type != PSKIP && currMB->mb_type != I_MB)
        {
            const int maptab[] = { BACKWORD, -1, FORWARD, BSYM };
            if (currMB->mb_type == P2Nx2N)     // B_16x16
            {
                read_Bfrm_PredTypeInfo_AEC(img->cs_aec, &pred_type);

                for (i = 0; i < 4; i++)
                {
                    currMB->b8pdir[i] = maptab[pred_type];
                }
#if TRACE
                output_trace_info("MB pdir = %3d", currMB->b8pdir[0]);
#endif
            }
            else if (currMB->mb_type == P2NxN || currMB->mb_type == PNx2N)        // 1 : B_8x16,  2 : B_16x8
            {
                int PredMode[2] = { -1, -1 };
                for (i = 0; i < 2; i++)
                {
                    read_Bfrm_PredTypeInfo_AEC(img->cs_aec, &pred_type);
                    PredMode[i] = pred_type;
                }

                if (currMB->mb_type == PNx2N)
                {
                    currMB->b8pdir[0] = currMB->b8pdir[2] = maptab[PredMode[0]];
                    currMB->b8pdir[1] = currMB->b8pdir[3] = maptab[PredMode[1]];
                }
                else
                {
                    currMB->b8pdir[0] = currMB->b8pdir[1] = maptab[PredMode[0]];
                    currMB->b8pdir[2] = currMB->b8pdir[3] = maptab[PredMode[1]];
                }
#if TRACE
                output_trace_info("MB pdir = %3d", currMB->b8pdir[0]);
                output_trace_info("MB pdir = %3d", currMB->b8pdir[3]);
#endif
            }
            else if (currMB->mb_type == PNxN)         // B_8x8
            {
                const int mode_maptab_8x8[4] = { PSKIP, PNxN, PNxN, PNxN };
                const int pdir_maptab_8x8[4] = { BSYM, FORWARD, BACKWORD, BSYM };
                for (i = 0; i < 4; i++)
                {
                    //mb_part_type is fix length coding(fix length equal 2)
                    read_B8x8_PredTypeInfo_AEC(img->cs_aec, &pred_type);
                    
                    currMB->b8mode[i] = mode_maptab_8x8[pred_type];
                    currMB->b8pdir[i] = pdir_maptab_8x8[pred_type];
#if TRACE
                    output_trace_info("8x8 MB type = %3d\n", currMB->b8mode[i]);
#endif
                }
            }
            else
            {
                printf("Entropy Syntax Error! B_Macroblock_Type : %d\n", currMB->mb_type);
                exit(0);
            }
        }
    }
    else if (img->type == P_IMG)
    {
        // derive the prediction direction of macroblock
        const int maptab[] = { FORWARD, MHP };
        if (currMB->mb_type == 0)     // P_16x16
        {
            read_P16x16_PredTypeInfo_AEC(img->cs_aec, &pred_type);

            switch (pred_type)
            {
            case 0:
                for (i = 0; i < 4; i++)
                {
                    currMB->b8pdir[i] = FORWARD;
                }
                break;
            case 2:
                currMB->mb_type = 1;
                for (i = 0; i < 4; i++)
                {
                    currMB->b8mode[i] = P2Nx2N;
                    currMB->b8pdir[i] = FORWARD;
                }
                break;
            case 3:
                currMB->mb_type = 1;
                for (i = 0; i < 4; i++)
                {
                    currMB->b8mode[i] = P2Nx2N;
                    currMB->b8pdir[i] = MHP;
                }
                break;
            default:
                printf("Entropy Syntax Error! P_16x16_Pred_Type : %d\n", pred_type);
                exit(0);
            }
#if TRACE
            output_trace_info("MB type = %3d", currMB->mb_type);
            output_trace_info("8x8 MB pdir = %3d\n", currMB->b8pdir[0]);
#endif
        }
        else if (currMB->mb_type == 2 || currMB->mb_type == 3)
        {
            int PredMode[2] = { -1, -1 };
            for (i = 0; i < 2; i++)
            {
                read_Pfrm_PredTypeInfo_AEC(img->cs_aec, &pred_type);
                PredMode[i] = pred_type;
            }

            if (currMB->mb_type == 3)
            {
                currMB->b8pdir[0] = currMB->b8pdir[2] = maptab[PredMode[0]];
                currMB->b8pdir[1] = currMB->b8pdir[3] = maptab[PredMode[1]];
            }
            else
            {
                currMB->b8pdir[0] = currMB->b8pdir[1] = maptab[PredMode[0]];
                currMB->b8pdir[2] = currMB->b8pdir[3] = maptab[PredMode[1]];
            }
#if TRACE
            output_trace_info("MB pdir = %3d", currMB->b8pdir[0]);
            output_trace_info("MB pdir = %3d", currMB->b8pdir[3]);
#endif
        }
        else if (currMB->mb_type == I_MB)
        {
            currMB->cbp = 63;
            for (i = 0; i < 4; i++)
            {
                currMB->b8pdir[i] = -1;
            }
        }
        else if (currMB->mb_type == PNxN)
        {
            for (i = 0; i < 4; i++)
            {
                //mb_part_type is fix length coding(fix length equal 2)
                read_Pfrm_PredTypeInfo_AEC(img->cs_aec, &pred_type);
                currMB->b8mode[i] = PNxN;
                currMB->b8pdir[i] = maptab[pred_type];
#if TRACE
                output_trace_info("8x8 MB type = %3d\n", currMB->b8mode[i]);
                output_trace_info("8x8 MB pdir = %3d", currMB->b8pdir[i]);
#endif
            }
        }
        else
        {
            printf("Entropy Syntax Error! P_Macroblock_Type : %d\n", currMB->mb_type);
            exit(0);
        }
    }
}

void xParseRefIdx(ImgParams *img, Macroblock *currMB)
{
    int i, ref;
    if (img->type == P_IMG && img->real_ref_num > 1 && img->ip_frm_idx > 1)
    {
        int ref_num[10] = { 0, 1, 3, 7, 11, 15, 19, 23, 27, 31 };

        if (currMB->mb_type == PNxN)
        {
            for (i = 0; i < 4; i++)
            {
                read_Reffrm_AEC(img->cs_aec, &ref);
                if (ref < 0 || ref > 9)
                {
                    printf("Reference Frame Index Error!\n");
                    exit(0);
                }
                currMB->fw_ref[i] = ref_num[ref];
            }
        }
        else if (currMB->mb_type == P2Nx2N)
        {
            read_Reffrm_AEC(img->cs_aec, &ref);

            if (ref < 0 || ref > 9)
            {
                printf("Reference Frame Index Error!\n");
                exit(0);
            }
            currMB->fw_ref[0] = currMB->fw_ref[1] = currMB->fw_ref[2] = currMB->fw_ref[3] = ref_num[ref];

        }
        else if (currMB->mb_type == P2NxN || currMB->mb_type == PNx2N)
        {
            for (i = 0; i < 2; i++)
            {
                read_Reffrm_AEC(img->cs_aec, &ref);

                if (ref < 0 || ref > 9)
                {
                    printf("Reference Frame Index Error!\n");
                    exit(0);
                }
                if (currMB->mb_type == 2)
                {
                    if (i == 0)
                    {
                        currMB->fw_ref[0] = currMB->fw_ref[1] = ref_num[ref];
                    }
                    else
                    {
                        currMB->fw_ref[2] = currMB->fw_ref[3] = ref_num[ref];
                    }
                }
                else
                {
                    if (i == 0)
                    {
                        currMB->fw_ref[0] = currMB->fw_ref[2] = ref_num[ref];
                    }
                    else
                    {
                        currMB->fw_ref[1] = currMB->fw_ref[3] = ref_num[ref];
                    }
                }
            }
        }
    }
}

// for TRANS_2Nx2N only the prediction mode need to be read
void xParseIntraPredInfo(ImgParams *img, Macroblock *currMB)
{
    if (currMB->mb_trans_type == TRANS_2Nx2N)
    {
        read_intra_luma_mode(img, 0, 0);
    }
    else     // for TRANS_NxN, the trans_split is required besides prediction mode
    {
        int b8, b4;

        for (b8 = 0; b8 < 4; b8++)
        {
            readSubMBTransType_AEC(img->cs_aec, &currMB->sub_mb_trans_type[b8]);
        }

        for (b8 = 0; b8 < 4; b8++)
        {
            int cnt = currMB->sub_mb_trans_type[b8] == 0 ? 1 : 4;
            for (b4 = 0; b4 < cnt; b4++)
            {
                read_intra_luma_mode(img, b8, b4);
            }
        }
    }

    readCIPredMode_AEC(img->cs_aec, &currMB->c_ipred_mode);
#if TRACE
    output_trace_info("Chroma intra pred mode %d", currMB->c_ipred_mode);
#endif

    if (currMB->c_ipred_mode < DC_PRED_8 || currMB->c_ipred_mode > PLANE_8)
    {
        printf("%d\n", img->current_mb_nr);
        error("illegal chroma intra pred mode!\n", 600);
    }
}

/**************************************************************************
* Function:Get the syntax elements from the NAL
* Input:
* Output:
* Return:
* Attention:
**************************************************************************/
int read_one_macroblock( ImgParams *img, Macroblock *currMB )
{
    int fixqp = (img->fixed_frame_level_qp || img->fixed_slice_qp);

    currMB->mb_trans_type = TRANS_NxN;
    if (img->type != I_IMG)
    {
        CheckAvailabilityOfNeighbors(img, currMB);

        xParseMBType(img, currMB);

        xParsePredInfo(img, currMB);

        // derive transform size
        if (currMB->mb_type == P2Nx2N)
        {
            currMB->mb_trans_type = TRANS_2Nx2N;
        }
        else
        {
            currMB->mb_trans_type = TRANS_NxN;
        }

        xParseRefIdx(img, currMB);
    }
    else
    {
        interpret_mb_mode_I( currMB );
    }

    init_ref_mv( img, currMB, 4 );

    if ( IS_INTRA( currMB ) )
    {
        readMBTransType_AEC(img->cs_aec, &currMB->mb_trans_type);
#if TRACE
        output_trace_info( "transform type: %d", currMB->mb_trans_type );
#endif
        xParseIntraPredInfo(img, currMB);
    }

    // read mb_qp_delta
    if (currMB->cbp > 0 && !fixqp)
    {
        readMBDeltaQp_AEC(img->cs_aec, &currMB->mb_qp_delta, img->previous_delta_qp);
        img->previous_delta_qp = currMB->mb_qp_delta;
    }

    if( currMB->mb_type != I_MB )
    {
        readMVD( img, currMB );
        xDecMV( img, currMB );
    }

    if ( IS_PSKIP( currMB ) ) //keep last macroblock
    {
        GetPskipMV( img, currMB, 1 << 4 );

        currMB->cbp = 0;
        return DECODE_MB;
    }

    if ( !IS_DIRECT( currMB ) )
    {
        readCBP_AEC(currMB, img->cs_aec, &currMB->cbp );
#if TRACE
        output_trace_info( "CBP = %3d", currMB->cbp );
#endif
        if ( fixqp )
        {
            img->qp = ( img->qp - MIN_QP + ( MAX_QP - MIN_QP + 1 ) ) % ( MAX_QP - MIN_QP + 1 ) + MIN_QP;
        }
    }
    // read CBP and Coeffs
    readCoeffBlks( img, currMB, 4 );
    return DECODE_MB;
}

void read_intra_luma_mode( ImgParams *img, int b8, int b4 )
{
    int b8_x, b8_y;
    int b4_x, b4_y;
    int mode;

    readIntraPredMode_AEC( img->cs_aec, &mode );
#if TRACE
    output_trace_info( "Intra mode = %3d", mode );
#endif
    b8_x = img->mb_b8_x + ( b8 & 1 );
    b8_y = img->mb_b8_y + ( b8 / 2 );

    //set
    b4_x = b8_x * 2 + ( b4 & 1 );
    b4_y = b8_y * 2 + ( b4 / 2 );
    img->ipredmode[b4_x + 1][b4_y + 1] = mode;
}

/*
*************************************************************************
* Function:Set context for reference frames
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

int BType2CtxRef( int btype )
{
    if ( btype < 4 )
    {
        return 0;
    }
    else
    {
        return 1;
    }
}


void readChromaCoeffBlk( CSobj *cs_aec, uchar_t uhBitSize, int *ACLevel, int *ACRun )
{
    int k, run;
    int bsize = 1 << ( uhBitSize - 1 );
    int coef_num = bsize * bsize;
    int level = 1;

    for ( k = 0; ( k < coef_num + 1 ) && ( level != 0 ); k++ )
    {
        readRunLevel_AEC( cs_aec, bsize, &level, &run, CHROMA_AC );
#if TRACE
        output_trace_info( "Chroma level = %3d", level );
        output_trace_info( "Chroma run = %3d", run );
#endif
        ACLevel[k] = level;
        ACRun[k] = run;
    }
}

void readLumaCoeff_AEC( CSobj *cs_aec, int bsize, int *ACLevel, int *ACRun )
{
    int k;
    int coef_num = bsize * bsize;
    int run;
    int level = 1;

    for ( k = 0; ( k < coef_num + 1 ) && ( level != 0 ); k++ )
    {
        readRunLevel_AEC( cs_aec, bsize, &level, &run, LUMA_8x8 );
        if( k == coef_num )
        {
            level = 0;
            run = 0;
        }
        else
        {
            ACLevel[k] = level;
            ACRun[k] = run;
        }
#if TRACE
        output_trace_info( "Luma level = %3d", level );
        output_trace_info( "Luma run = %3d", run );
#endif
    }
}

void readLumaCoeffBlk( Macroblock *currMB, CSobj *cs_aec, uchar_t uhBitSize, int *ACLevel, int *ACRun )
{
    int bsize;
    int isIntra = IS_INTRA( currMB );
    int coef_num;
    int b8;
    // luma coefficients, TRANS_2Nx2N
    if ( currMB->mb_trans_type == TRANS_2Nx2N )
    {
        bsize = 1 << uhBitSize;
        if ( currMB->cbp & 1 )
        {
            readLumaCoeff_AEC( cs_aec, bsize, ACLevel, ACRun );
        }
    }
    else
    {
        // luma coefficients
        for ( b8 = 0; b8 < 4; b8 ++ )
        {
            if ( currMB->sub_mb_trans_type[b8] == 0 )
            {
                bsize = 1 << ( uhBitSize - 1 );
                coef_num = bsize * bsize;
                if ( currMB->cbp & ( 1 << ( 4 * b8 ) ) )
                {
                    readLumaCoeff_AEC( cs_aec, bsize, ACLevel + b8 * coef_num, ACRun + b8 * coef_num );
                }
            }
            else
            {
                int b4;
                int b8_pix_num = 64;
                int b4_pix_num = 16;
                bsize = 4;
                for ( b4 = 0; b4 < 4; b4 ++ )
                {
                    if ( currMB->cbp & ( 1 << ( 4 * b8 + b4 ) ) )
                    {
                        readLumaCoeff_AEC( cs_aec, bsize, ACLevel + b8 * b8_pix_num + b4 * b4_pix_num, ACRun + b8 * b8_pix_num + b4 * b4_pix_num );
                    }
                }
            }
        }
    }
}
/*
*************************************************************************
* Function:Get coded block pattern and coefficients (run/level)
from the bitstream
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void readCoeffBlks( ImgParams *img, Macroblock *currMB, uchar_t uhBitSize )
{
    CSobj *cs_aec = img->cs_aec;
    int bsize;
    int coef_num;
    int uv;
    int isIntra = IS_INTRA( currMB );

    int *ACLevel = img->coefAC_luma[0];
    int *ACRun = img->coefAC_luma[1];

    // luma
    bsize = 1 << uhBitSize;
    coef_num = bsize * bsize;
    memset(ACLevel, 0, sizeof(ACLevel)*coef_num);
    memset(ACRun, 0, sizeof(ACRun)*coef_num);
    DCT_Pairs = -1;
    readLumaCoeffBlk( currMB, cs_aec, uhBitSize, ACLevel, ACRun );

    // chroma
    bsize = 1 << ( uhBitSize - 1 );
    coef_num = bsize * bsize;
    for ( uv = 0; uv < 2; uv ++ )
    {
        ACLevel = img->coefAC_chroma[uv][0];
        ACRun = img->coefAC_chroma[uv][1];
        memset(ACLevel, 0, sizeof(ACLevel)*coef_num);
        memset(ACRun, 0, sizeof(ACRun)*coef_num);

        if ( ( currMB->cbp >> ( 4 * ( uv + 4 ) ) ) & 0x1 )
        {
            readChromaCoeffBlk( cs_aec, uhBitSize, ACLevel, ACRun );
        }
    }
}

int decode_one_cu( ImgParams *img, Macroblock *currMB, uchar_t uhBitSize )
{
    img->pu_pix_x = img->mb_pix_x;
    img->pu_pix_y = img->mb_pix_y;
    if ( IS_INTRA( currMB ) )
    {
        DecodeIntraLumaCU( img, currMB, uhBitSize );
        DecodeIntraChromaCU( img, currMB, uhBitSize );
    }
    else
    {
        DecodeInterLumaCU( img, currMB, uhBitSize );
        DecodeInterChromaCU( img, currMB, uhBitSize );
    }

    return 0;
}