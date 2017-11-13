#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <memory.h>

#include "defines.h"
#include "global.h"
#include "elements.h"
#include "block.h"
#include "vlc.h"
#include "AEC.h"
#include "../../common/common.h"
#include "../../common/pixel.h"
#include "../../common/intraPrediction.h"
#include "../../common/interPrediction.h"


/*
*************************************************************************
* Function:Copy region of img->m7 corresponding to block8x8 to curr_blk[][].
* Input:
* Output:
* Return:
* Attention:img->m7 is [x][y] and curr_blk is normal buffer.
*************************************************************************
*/

extern  short IQ_SHIFT[64];
extern  unsigned  short IQ_TAB[64];
void inv_quant_luma( ImgParams *img, Macroblock *currMB, int *curr_blk, int b8, int b4, int bsize )
{
    int  x, y, k;
    int coef_num;
    const int *IVC_SCAN;
    int level, coef_ctr;
    int *ACLevel = img->coefAC_luma[0];
    int *ACRun = img->coefAC_luma[1];
    int *pACLevel, *pACRun;
    int qp = img->qp;
    int shift, QPI;

    int  mb_y = 8 * ( b8 / 2 ) + 4 * ( b4 / 2 );
    int  mb_x = 8 * ( b8 % 2 ) + 4 * ( b4 % 2 );

    for ( y = 0; y < bsize; y++ )
    {
        for ( x = 0; x < bsize; x++ )
        {
            curr_blk[y * bsize + x] = 0;
        }
    }

    shift = IQ_SHIFT[qp];
    QPI = IQ_TAB[qp];
    if ( currMB->mb_trans_type == TRANS_2Nx2N )
    {
        //bsize = 1 << uhBitSize;
        coef_num = bsize * bsize;
        IVC_SCAN = IVC_SCAN16;

        if ( currMB->cbp & 1 )
        {
            coef_ctr = - 1;
            level = 1;
            for ( k = 0; ( k < coef_num ) && ( level != 0 ); k++ )
            {
                level = ACLevel[k];
                //============ decode =============
                if ( ACLevel[k] != 0 )
                {
                    coef_ctr += ACRun[k] + 1;
                    x = IVC_SCAN[coef_ctr] % bsize;
                    y = IVC_SCAN[coef_ctr] / bsize;
                    curr_blk[y * bsize + x] = ( level * QPI + ( 1 << ( shift - 1 ) ) ) >> shift;
                }
            }
        }
    }
    else
    {
        // luma coefficients
        if ( currMB->sub_mb_trans_type[b8] == 0 )
        {
            //bsize = 1 << (uhBitSize - 1);
            coef_num = bsize * bsize;
            IVC_SCAN = IVC_SCAN8;

            if ( currMB->cbp & ( 1 << ( 4 * b8 ) ) )
            {
                coef_ctr = - 1;
                level = 1;
                pACLevel = ACLevel + b8 * coef_num;
                pACRun = ACRun + b8 * coef_num;
                for ( k = 0; ( k < coef_num ) && ( level != 0 ); k++ )
                {
                    level = pACLevel[k];
                    if ( level != 0 )
                    {
                        coef_ctr += pACRun[k] + 1;
                        x = IVC_SCAN[coef_ctr] % bsize;
                        y = IVC_SCAN[coef_ctr] / bsize;
                        curr_blk[y * bsize + x] = ( level * QPI + ( 1 << ( shift - 1 ) ) ) >> shift;
                    }
                }
            }
        }
        else
        {
            int b8_pix_num = 64;
            int b4_pix_num = 16;
            //bsize = 4;
            coef_num = bsize * bsize;
            IVC_SCAN = IVC_SCAN4;

            if ( currMB->cbp & ( 1 << ( 4 * b8 + b4 ) ) )
            {
                coef_ctr = - 1;
                level = 1;
                pACLevel = ACLevel + b8 * b8_pix_num + b4 * b4_pix_num;
                pACRun = ACRun + b8 * b8_pix_num + b4 * b4_pix_num;
                for ( k = 0; ( k < coef_num ) && ( level != 0 ); k++ )
                {
                    level = pACLevel[k];
                    if ( level != 0 )
                    {
                        coef_ctr += pACRun[k] + 1;
                        x = IVC_SCAN[coef_ctr] % bsize;
                        y = IVC_SCAN[coef_ctr] / bsize;
                        curr_blk[y * bsize + x] = ( level * QPI + ( 1 << ( shift - 1 ) ) ) >> shift;
                    }
                }
            }
        }
    }
}

void get_chroma_coef( ImgParams *img, Macroblock *currMB, int *curr_blk, int uv, int bsize )
{
    int  x, y, k;
    int coef_num;
    const int *IVC_SCAN = IVC_SCAN8;
    int level, coef_ctr;
    int *ACLevel = img->coefAC_chroma[uv][0];
    int *ACRun = img->coefAC_chroma[uv][1];
    int qp = QP_SCALE_CR[img->qp];
    int shift = IQ_SHIFT[qp];
    int QPI = IQ_TAB[qp];

    for ( y = 0; y < bsize; y++ )
    {
        for ( x = 0; x < bsize; x++ )
        {
            curr_blk[y*bsize + x] = 0;
        }
    }

    coef_num = bsize * bsize;

    if ( ( currMB->cbp >> ( 4 * ( uv + 4 ) ) ) & 0x1 )
    {
        coef_ctr = -1;
        level = 1;
        for ( k = 0; ( k < coef_num ) && ( level != 0 ); k++ )
        {
            level = ACLevel[k];
            if ( level != 0 )
            {
                coef_ctr = coef_ctr + ACRun[k] + 1;
                x = IVC_SCAN[coef_ctr] % 8;
                y = IVC_SCAN[coef_ctr] / 8;
                curr_blk[y*bsize + x] = ( level * QPI + ( 1 << ( shift - 1 ) ) ) >> shift;
            }
        }
    }
}
/*
*************************************************************************
* Function:Make Intra prediction for all 5 modes for 8*8 blocks.
bs_x and bs_y may be only 4 and 8.
img_x and img_y are pixels offsets in the picture.
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void IntraLumaPred( ImgParams *img, Macroblock *currMB, int img_x, int img_y, int bsize )
{
    pel_t edgepixels[4*LCU_SIZE + 1];
    pel_t *pedge = edgepixels +  2*LCU_SIZE;
    unsigned int x_off, y_off;
    pel_t *predblk;
    int predmode = img->ipredmode[img_x / B4_SIZE + 1][img_y / B4_SIZE + 1];
    bool bAboveAvail, bLeftAvail;
    assert( predmode >= 0 );
    memset( edgepixels, 128, sizeof( edgepixels ) );
    xPrepareIntraPattern( img, currMB, pedge, img_x, img_y, bsize, &bAboveAvail, &bLeftAvail );


    x_off = img_x & 0x0FU;
    y_off = img_y & 0x0FU;
    predblk = img->pred_blk_luma + y_off*MB_SIZE + x_off;
    intra_pred_luma( pedge, predblk, MB_SIZE, predmode, bsize, bAboveAvail, bLeftAvail );
}

void DecodeIntraChromaCU( ImgParams *img, Macroblock *currMB, uchar_t uhBitSize )
{
    pel_t edgepixuv[2*LCU_SIZE+1];
    pel_t *pedge = edgepixuv + LCU_SIZE;
    int bsize = 1 << ( uhBitSize - 1 );
    int curr_blk[MB_SIZE * MB_SIZE];
    int uv;
    bool bAboveAvail, bLeftAvail;

    //Chroma
    for( uv = 0; uv < 2; uv++ )
    {
        xPrepareIntraPatternC( img, currMB, pedge, uv, bsize, &bAboveAvail, &bLeftAvail );

        intra_pred_chroma( pedge, img->pred_blk_chroma[uv], MB_SIZE, currMB->c_ipred_mode, 8, bAboveAvail, bLeftAvail );

        get_chroma_coef( img, currMB, curr_blk, uv, bsize );

        inv_transform( curr_blk, bsize );
        recon_chroma_blk(img, uv, 0, curr_blk, img->pred_blk_chroma[uv], bsize);
    }
}

void DecodeIntraLumaCU( ImgParams *img, Macroblock *currMB, uchar_t uhBitSize )
{
    int bsize = 1 << uhBitSize;
    int img_b8_y=0, img_b8_x=0;
    int curr_blk[MB_SIZE * MB_SIZE];
    int b8;


    //LUMA
    if ( currMB->mb_trans_type == TRANS_2Nx2N )
    {
        inv_quant_luma( img, currMB, curr_blk, 0, 0, bsize );
        IntraLumaPred( img, currMB, ( img->mb_x << 4 ), ( img->mb_y << 4 ), bsize );

        inv_transform( curr_blk, bsize );
        recon_luma_blk(img, 0, 0, curr_blk, bsize);
    }
    else
    {
        for ( b8 = 0; b8 < 4; b8++ )
        {
            int b4;
            int blk_cnt = currMB->sub_mb_trans_type[b8] ? 4 : 1;
            bsize = currMB->sub_mb_trans_type[b8] ? 4 : 8;
            for ( b4 = 0; b4 < blk_cnt; b4++ )
            {
                int img_x = 16 * img->mb_x + 8 * ( b8 & 1 ) + 4 * ( b4 & 1 );
                int img_y = 16 * img->mb_y + 8 * ( b8 / 2 ) + 4 * ( b4 / 2 );
                inv_quant_luma( img, currMB, curr_blk, b8, b4, bsize );
                IntraLumaPred( img, currMB, img_x, img_y, bsize );

                inv_transform( curr_blk, bsize );
                recon_luma_blk(img, b8, b4, curr_blk, bsize);
            }
        }
    }
}

void DecodeInterChromaCU( ImgParams *img, Macroblock *currMB, uchar_t uhBitSize )
{
    int bsize = 1 << ( uhBitSize - 1 );
    int img_b4_x, img_b4_y;
    int uv, b8, off_x, off_y;
    int curr_blk[MB_SIZE * MB_SIZE];
    pel_t bw_pred[LCU_NUM], fw_pred[LCU_NUM];

    int mv_x, mv_y, mv_x_bw, mv_y_bw;
    int if_pos_x, if_pos_y, if_pos_x_bw, if_pos_y_bw;
    int i_refc = img->iStrideC;
    int refframe, fw_refframe, bw_refframe, b8mode, pred_dir;
    int bframe = ( img->type == B_IMG );
    pel_t *predblk;

    //Chroma
    for( uv = 0; uv < 2; uv++ )
    {
        for ( b8 = 0; b8 < 4; b8++ ) // loop over 4x4 chroma blocks
        {
            off_y = ( b8/2 ) << 2;
            off_x = ( b8%2 ) << 2;
            img_b4_y=img->mb_b8_y + ( b8/2 );
            img_b4_x=img->mb_b8_x + ( b8%2 );
            predblk = img->pred_blk_chroma[uv] + off_y * MB_SIZE + off_x;

            // PREDICTION
            b8mode  = currMB->b8mode[b8];
            pred_dir = currMB->b8pdir[b8];
            get_mv_ref( img, currMB, b8, bframe, img_b4_x, img_b4_y, &mv_x, &mv_y, &mv_x_bw, &mv_y_bw, &refframe, &fw_refframe, &bw_refframe );

            if_pos_y = mv_y & 0x7;
            if_pos_x = mv_x & 0x7;
            if_pos_y_bw = mv_y_bw & 0x7;
            if_pos_x_bw = mv_x_bw & 0x7;

            mv_y += ( ( img->mb_pix_y_cr+off_y )<<3 );
            mv_x += ( ( img->mb_pix_x_cr+off_x )<<3 );
            mv_y_bw += ( ( img->mb_pix_y_cr+off_y )<<3 );
            mv_x_bw += ( ( img->mb_pix_x_cr+off_x )<<3 );

            if ( pred_dir != BSYM )
            {
                if( !bframe )
                {
                    if( ( b8mode != PSKIP ) && ( pred_dir == MHP ) )
                    {
                        get_chroma_block( fw_pred, 4, mv_x, mv_y, if_pos_x, if_pos_y, p_mcef[refframe][uv], i_refc );
                        get_chroma_block( bw_pred, 4, mv_x_bw, mv_y_bw, if_pos_x_bw, if_pos_y_bw, p_mcef[refframe][uv], i_refc );
                        g_funs_handle.avg_pel( predblk, MB_SIZE, fw_pred, 4, bw_pred, 4, 4, 4 );
                    }
                    else
                    {
                        get_chroma_block( predblk, MB_SIZE, mv_x, mv_y, if_pos_x, if_pos_y, p_mcef[refframe][uv], i_refc );
                    }
                }
                else
                {
                    refframe = 0;
                    if( !pred_dir )
                    {
                        get_chroma_block( predblk, MB_SIZE, mv_x, mv_y, if_pos_x, if_pos_y, f_ref_frm[uv+1], i_refc );
                    }
                    else
                    {
                        get_chroma_block( predblk, MB_SIZE, mv_x, mv_y, if_pos_x, if_pos_y, b_ref_frm[uv+1], i_refc );
                    }
                }
            }
            else
            {
                get_chroma_block( fw_pred, 4, mv_x, mv_y, if_pos_x, if_pos_y, f_ref_frm[uv+1], i_refc );
                get_chroma_block( bw_pred, 4, mv_x_bw, mv_y_bw, if_pos_x_bw, if_pos_y_bw, b_ref_frm[uv+1], i_refc );
                g_funs_handle.avg_pel( predblk, MB_SIZE, fw_pred, 4, bw_pred, 4, 4, 4 );
            }
        }
        get_chroma_coef( img, currMB, curr_blk, uv, bsize );

        inv_transform( curr_blk, bsize );
        recon_chroma_blk(img, uv, 0, curr_blk, img->pred_blk_chroma[uv], bsize);
    }
}

void DecodeInterLumaCU( ImgParams *img, Macroblock *currMB, uchar_t uhBitSize )
{
    pel_t tmp_block[LCU_NUM];
    pel_t tmp_blockbw[LCU_NUM];

    int bsize = 1 << uhBitSize;
    int img_b8_y = 0, img_b8_x = 0;
    int b8, off_x, off_y;
    int mv_x = 0, mv_y = 0, mv_x_bw = 0, mv_y_bw = 0;
    int curr_blk[MB_SIZE * MB_SIZE];
    int blk_step_h = BLOCK_STEP[currMB->mb_type][0];
    int blk_step_v = BLOCK_STEP[currMB->mb_type][1];

    int i_ref = img->iStride;
    int refframe, fw_refframe, bw_refframe, b8mode, b8pdir;
    int bframe = ( img->type == B_IMG );
    pel_t *predblk;

    bsize = bsize >> 1;
    for ( b8 = 0; b8 < 4; b8++ )
    {
        off_y = ( b8 / 2 ) * bsize;
        off_x = ( b8 % 2 ) * bsize;
        img_b8_x = img->mb_b8_x + b8 % 2;
        img_b8_y = img->mb_b8_y + b8 / 2;

        predblk = img->pred_blk_luma + off_y*MB_SIZE + off_x;

        b8mode = currMB->b8mode[b8];
        b8pdir = currMB->b8pdir[b8];
        get_pred_mv( img, currMB, b8, bframe, img_b8_x, img_b8_y, &mv_x, &mv_y, &mv_x_bw, &mv_y_bw, &refframe, &fw_refframe, &bw_refframe );

        if( b8pdir!=BSYM )
        {
            if ( !bframe )
            {
                get_luma_block( img, tmp_block, bsize, bsize, mv_x, mv_y, p_mref[refframe], i_ref );
            }
            else
            {
                if ( !b8pdir )
                {
                    get_luma_block( img, tmp_block, bsize, bsize, mv_x, mv_y, f_ref_frm[0], i_ref );
                }
                else
                {
                    get_luma_block( img, tmp_block, bsize, bsize, mv_x, mv_y, b_ref_frm[0], i_ref );
                }
            }

            if ( b8pdir == MHP )
            {
                get_luma_block( img, tmp_blockbw, bsize, bsize, mv_x_bw, mv_y_bw, p_mref[refframe], i_ref );
                g_funs_handle.avg_pel( predblk, MB_SIZE, tmp_block, bsize, tmp_blockbw, bsize, bsize, bsize );
            }
            else     // FORWARD or BACKWORD
            {
                g_funs_handle.com_cpy( tmp_block, bsize, predblk, MB_SIZE, bsize, bsize );
            }
        }
        else
        {
            get_luma_block( img, tmp_block, bsize, bsize, mv_x, mv_y, f_ref_frm[0], i_ref );
            get_luma_block( img, tmp_blockbw, bsize, bsize, mv_x_bw, mv_y_bw, b_ref_frm[0], i_ref );
            g_funs_handle.avg_pel( predblk, MB_SIZE, tmp_block, bsize, tmp_blockbw, bsize, bsize, bsize );
        }

        if ( currMB->mb_trans_type == TRANS_NxN )
        {
            inv_quant_luma( img, currMB, curr_blk, b8, 0, bsize );

            inv_transform( curr_blk, bsize );
            recon_luma_blk(img, b8, 0, curr_blk, bsize);
        }
    }

    bsize = 1 << uhBitSize;
    if ( currMB->mb_trans_type == TRANS_2Nx2N )
    {
        inv_quant_luma ( img, currMB, curr_blk, 0, 0, bsize );

        inv_transform( curr_blk, bsize );
        recon_luma_blk(img, 0, 0, curr_blk, bsize);
    }
}