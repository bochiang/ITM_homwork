#include <math.h>
#include <memory.h>
#include <assert.h>

#include "memalloc.h"
#include "refbuf.h"
#include "block.h"
#include "pixel.h"
#include "vlc.h"
#include "macroblock.h"
#include "global.h"
#include "AEC.h"
#include "mv-search.h"
#include "../../common/common.h"
#include "../../common/intraPrediction.h"
#include "../../common/interPrediction.h"

#ifdef FastME
#include "fast_me.h"
#endif

#ifdef TDRDO
#include "tdrdo.h"
#endif

int best_splite[4];

#define IS_FW (((currMB->b8pdir[k]==FORWARD || currMB->b8pdir[k]==BSYM || currMB->b8pdir[k] == MHP) && (mode != PNxN || currMB->b8mode[k] != PSKIP || !bframe)) || (currMB->b8pdir[k]==MHP && (!bframe)))
#define IS_BW ((currMB->b8pdir[k]==BACKWORD || currMB->b8pdir[k]==BSYM) && (mode != PNxN || currMB->b8mode[k] != PSKIP))

// just for 8x8 or 4x4 blocks
double xCheckRDCostIntraLumaMB( Macroblock *currMB, CSobj *cs_aec, int *nonzero, int b8, int b4, int ipmode, int bsize )
{
    int x, y, tmp_cbp;
    int block_x;
    int block_y;
    int pix_x;
    int pix_y;
    int coef_blk[MB_SIZE * MB_SIZE];
    int resi_blk[MB_SIZE * MB_SIZE];
    int tu_size = MIN( bsize, MB_SIZE );

    int iBlkIdx = ( bsize==4 ) ? b4 : b8;
    int rate;
    int distortion;

    block_x = 8 * ( b8 % 2 ) + 4 * ( b4 % 2 );
    block_y = 8 * ( b8 / 2 ) + 4 * ( b4 / 2 );
    pix_x = img->mb_pix_x + block_x;
    pix_y = img->mb_pix_y + block_y;

    // perform DCT, Q, IQ, IDCT, Reconstruction
    for ( y = 0; y < tu_size; y++ )
    {
        for ( x = 0; x < tu_size; x++ )
        {
            resi_blk[y*tu_size+x] = imgY_org[( pix_y + y )*( img->width ) + pix_x + x] - img->pred_blk_luma[( block_y + y ) * MB_SIZE + block_x + x];
        }
    }

    tmp_cbp = 0;

    transform( resi_blk, coef_blk, tu_size );
    scanquant( img, cs_aec, 4, b8, b4, coef_blk, resi_blk, &tmp_cbp, tu_size ); // '|4' indicate intra for quantization
    recon_luma_blk(img, b8, b4, coef_blk, bsize);

    *nonzero = ( tmp_cbp != 0 );

    //get distortion (SSD) of 8x8 block
    distortion = 0;
    for ( y = 0; y < tu_size; y++ )
    {
        for ( x = 0; x < tu_size; x++ )
        {
            distortion += img->quad[imgY_org[( pix_y + y )*( img->width ) + pix_x + x] - imgY_rec[( pix_y + y )*( img->iStride ) + pix_x + x]];
        }
    }

    // get the rate of intra prediction mode
    rate = writeIntraPredMode_AEC( cs_aec, ipmode );

    // get the rate of luma coefficients
    rate += writeLumaCoeff_AEC( cs_aec, tu_size, img->coefAC_luma[0]+iBlkIdx*tu_size*tu_size, img->coefAC_luma[1]+iBlkIdx*tu_size*tu_size );

    //calculate RD and return it.
    return ( double )distortion + img->lambda * ( double )rate;
}

int EncodeIntraLumaBlk( Macroblock *currMB, CSobj *cs_aec, int b8, int b4, int *min_cost, int bsize )
{
    int ipmode, best_ipmode, i, j;
    int c_nz, nonzero;
    double rdcost, min_rdcost;
    int pu_x_in_cu, pu_y_in_cu;
    int x, y;
    pel_t best_rec[LCU_SIZE][LCU_SIZE];
    int best_coef[2][MB_SIZE*MB_SIZE];
    int coef_num = bsize * bsize;
    int iBlkIdx = ( bsize == 8 )? b8 : b4;
    int tmp_mvd[2][2][2][2];

    CSobj *cs_tmp = create_coding_state();
    pel_t   *predblk;
    uchar_t edgepixels[4 * LCU_SIZE + 1];
    unsigned char *pedge = edgepixels + 2*LCU_SIZE;
    int pred_flag[IntraPredModeNum] = {-1};
    bool bAboveAvail, bLeftAvail;
    memset( edgepixels, 128, sizeof( edgepixels ) );
    memset( pred_flag, -1, IntraPredModeNum * sizeof( int ) );

    pu_x_in_cu = 8 * ( b8 % 2 ) + 4 * ( b4 % 2 );
    pu_y_in_cu = 8 * ( b8 / 2 ) + 4 * ( b4 / 2 );
    img->pu_pix_x = img->mb_pix_x + pu_x_in_cu;
    img->pu_pix_y = img->mb_pix_y + pu_y_in_cu;
    min_rdcost = MAX_COST;
    *min_cost = ( 1 << 20 );

    // INTRA PREDICTION FOR 8x8 BLOCK
    xPrepareIntraPattern( img, currMB, pedge, img->pu_pix_x, img->pu_pix_y, bsize, &bAboveAvail, &bLeftAvail );
    pred_flag[DC_PRED] = 0;
    if ( bAboveAvail )
    {
        pred_flag[VERT_PRED] = 0;
    }
    if ( bLeftAvail )
    {
        pred_flag[HOR_PRED] = 0;
    }
    if ( bLeftAvail && bAboveAvail )
    {
        pred_flag[DOWN_LEFT_PRED] = 0;
        pred_flag[DOWN_RIGHT_PRED] = 0;
#if USING_INTRA_5_9
        pred_flag[INTRA_BILINEAR] = 0;
        pred_flag[INTRA_PLANE] = 0;
        pred_flag[INTRA_XY20] = 0;
        pred_flag[INTRA_XY30] = 0;
#endif
    }

    predblk = img->pred_blk_luma + pu_y_in_cu*MB_SIZE + pu_x_in_cu;
    for ( ipmode = VERT_PRED; ipmode < IntraPredModeNum; ipmode++ )
    {
        if ( pred_flag[ipmode] >= 0 )      //changed this. the intra-pred function marks the invalid modes. At least one is always valid (the DC).
        {
            intra_pred_luma( pedge, predblk, MB_SIZE, ipmode, bsize, bAboveAvail, bLeftAvail );

            // get prediction and prediction error
            for ( j = 0; j < bsize; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    img->resi_blk[i][j] = imgY_org[( img->pu_pix_y + j )*( img->width ) + img->pu_pix_x + i] - img->pred_blk_luma[( pu_y_in_cu + j ) * MB_SIZE + pu_x_in_cu + i];
                }
            }

            store_coding_state( cs_tmp );
            memcpy ( tmp_mvd, currMB->mvd, 4 * 2 * 2 * sizeof( int ) );
            // get and check rate-distortion cost
            if ( ( rdcost = xCheckRDCostIntraLumaMB( currMB, cs_aec, &c_nz, b8, b4, ipmode, bsize ) ) < min_rdcost )
            {
                // set coefficients
                for ( j = 0; j < 2; j++ )
                {
                    memcpy( best_coef[j], img->coefAC_luma[j] + iBlkIdx*bsize*bsize, coef_num * sizeof( int ) );
                }

                // set reconstruction
                for ( y = 0; y < bsize; y++ )
                {
                    for ( x = 0; x < bsize; x++ )
                    {
                        best_rec[y][x] = imgY_rec[( img->pu_pix_y + y )*( img->iStride ) + img->pu_pix_x + x];
                    }
                }

                // flag if DCT-coefficients must be coded
                nonzero = c_nz;

                // set best mode update minimum cost
                min_rdcost = rdcost;
                *min_cost = ( int )min_rdcost;
                best_ipmode = ipmode;
            }
            reset_coding_state( cs_tmp );
            memcpy ( currMB->mvd, tmp_mvd, 4 * 2 * 2 * sizeof( int ) );
        }
    }

    // store best info for current PU
    currMB->intra_pred_modes[b8 * 4 + b4] = best_ipmode;
    for ( j = 0; j < 2; j++ )
    {
        memcpy( img->coefAC_luma[j]+iBlkIdx*bsize*bsize, best_coef[j], coef_num * sizeof( int ) );
    }

    for ( y = 0; y < bsize; y++ )
    {
        for ( x = 0; x < bsize; x++ )
        {
            imgY_rec[( img->pu_pix_y + y )*( img->iStride ) + img->pu_pix_x + x] = best_rec[y][x];
        }
    }
    delete_coding_state( cs_tmp );
    return nonzero;
}

void xCheckIntraChromaTQ( Macroblock *currMB, CSobj *cs_aec, int uv, int mode, uchar_t uhBitSize )
{
    int tq_blk[MB_SIZE * MB_SIZE];
    int resi_blk[MB_SIZE * MB_SIZE];
    int bsize = 1 << ( uhBitSize - 1 );
    int cr_cbp = 0;
    int x, y;
    int curr_val = 0;
    int mb_pix_x = 0;
    int mb_pix_y = 0;

    // perform DCT, Q, IQ, IDCT, Reconstruction for each MB
    mb_pix_x = img->mb_pix_x_cr;
    mb_pix_y = img->mb_pix_y_cr;

    for ( y = 0; y < bsize; y++ )
    {
        for ( x = 0; x < bsize; x++ )
        {
            if ( uv == 0 )
            {
                resi_blk[y*8 + x] = imgU_org[( mb_pix_y + y )*( img->width_cr ) + mb_pix_x + x] - img->pred_blk_chroma[uv][y * MB_SIZE + x];
            }
            else
            {
                resi_blk[y*8 + x] = imgV_org[( mb_pix_y + y )*( img->width_cr ) + mb_pix_x + x] - img->pred_blk_chroma[uv][y * MB_SIZE + x];
            }
        }
    }

    transform( resi_blk, tq_blk, bsize );
    scanquant( img, cs_aec, 1, 4 + uv, 0, tq_blk, resi_blk, &cr_cbp, bsize );
    recon_chroma_blk(img, uv, 0, tq_blk, img->pred_blk_chroma[uv], bsize);

    currMB->cbp += cr_cbp;//((cr_cbp)<<4);
}

/*
*************************************************************************
* Function:Mode Decision for an 8x8 Intra block
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

int EncodeIntraLumaPU8x8( ImgParams *img, Macroblock *currMB, CSobj *cs_aec, int b8, int *cost )
{
    int cbp_8 = 0, cbp_4 = 0;
    int cost_tmp;
    int cost_8 = 0;
    int cost_4 = 0;
    int b4;
    int ipred8[4];
    int bsize = 8;
    int coef_num = bsize * bsize;
    int bst_coefs[2][64 * 4];
    pel_t rec8[8][8];
    int stx, sty;
    int x, y;
    double lambda = img->lambda;

    // try 8x8
    currMB->sub_mb_trans_type[b8] = 0;
    if ( EncodeIntraLumaBlk( currMB, cs_aec, b8, 0, &cost_tmp, 8 ) )
    {
        cbp_8 = 0xF << ( b8 * 4 );
    }
    cost_8 = cost_tmp + ( int )floor( 2.0 * lambda + 0.4999 );

    // backup four 8x8 coefficient blocks
    // if only backup current coefficient block, the following 8x8 blocks will cover the data, unless, the internal rdoq also use the same part of this buffer
    memcpy( ipred8, &currMB->intra_pred_modes[b8 * 4], sizeof( int ) * 4 );
    memcpy( bst_coefs[0], img->coefAC_luma[0], coef_num * 4 * sizeof( int ) );
    memcpy( bst_coefs[1], img->coefAC_luma[1], coef_num * 4 * sizeof( int ) );

    stx = bsize * ( b8 & 1 );
    sty = bsize * ( b8 / 2 );

    for ( y = 0; y < bsize; y++ )
    {
        for ( x = 0; x < bsize; x++ )
        {
            rec8[y][x] = imgY_rec[( img->mb_pix_y + sty + y )*( img->iStride ) + img->mb_pix_x + stx + x];
        }
    }

    // try 4x4
    currMB->sub_mb_trans_type[b8] = 1;
    for ( b4 = 0; b4 < 4; b4++ )
    {
        if ( EncodeIntraLumaBlk( currMB, cs_aec, b8, b4, &cost_tmp, 4 ) )
        {
            cbp_4 |= 1 << ( b8 * 4 + b4 );
        }
        cost_4 += cost_tmp + ( int )floor( 2.0 * lambda + 0.4999 );
    }

    if ( cost_8 < cost_4 )
    {
        currMB->sub_mb_trans_type[b8] = 0;
        memcpy( &currMB->intra_pred_modes[b8 * 4], ipred8, sizeof( int ) * 4 );
        // recover all coefficient blocks
        memcpy( img->coefAC_luma[0], bst_coefs[0], coef_num * 4 * sizeof( int ) );
        memcpy( img->coefAC_luma[1], bst_coefs[1], coef_num * 4 * sizeof( int ) );
        for ( y = 0; y < 8; y++ )
        {
            for ( x = 0; x < 8; x++ )
            {
                imgY_rec[( img->mb_pix_y + sty + y )*( img->iStride ) + img->mb_pix_x + stx + x] = rec8[y][x];
            }
        }

        *cost = cost_8;
        return cbp_8;
    }
    else
    {
        // place the coefficients to a large scale buffer
        memcpy( bst_coefs[0] + b8 * coef_num, img->coefAC_luma[0], coef_num * sizeof( int ) );
        memcpy( bst_coefs[1] + b8 * coef_num, img->coefAC_luma[1], coef_num * sizeof( int ) );
        // the best coefficients are also saved in img->coefAC_luma
        memcpy( img->coefAC_luma[0], bst_coefs[0], coef_num * 4 * sizeof( int ) );
        memcpy( img->coefAC_luma[1], bst_coefs[1], coef_num * 4 * sizeof( int ) );
        *cost = cost_4;
        return cbp_4;
    }
}

int EncodeIntraLumaPUNxN( ImgParams *img, Macroblock *currMB, CSobj *cs_aec, int *costNxN, uchar_t uhBitSize )
{
    int cbp = 0;
    int blkidx;
    int cost;
    uchar_t bitsize = uhBitSize - 1;
    *costNxN = 0;

    for ( blkidx = 0; blkidx < 4; blkidx++ )
    {
        img->pu_pix_y = img->cu_pix_y + ( ( blkidx / 2 ) << bitsize );
        img->pu_pix_x = img->cu_pix_x + ( ( blkidx % 2 ) << bitsize );
        cbp |= EncodeIntraLumaPU8x8( img, currMB, cs_aec, blkidx, &cost );
        *costNxN += cost;
    }

    return cbp;
}



int EncodeIntraLumaCU( ImgParams *img, Macroblock *currMB, CSobj *cs_aec, uchar_t uhBitSize )
{
    int cbpNxN = 0, cbp2Nx2N = 0;
    int costNxN, cost2Nx2N;
    int bak_coef[2][COF_SIZE_LUMA];
    int bak_ipredmode[MB_SIZE];
    pel_t bak_rec[MB_SIZE][MB_SIZE];
    int x, y;
    int bsize = ( 1 << uhBitSize );
    int coef_num = bsize * bsize;

    if( uhBitSize == 4 )
    {
        currMB->trans_type = TRANS_NxN;
        cbpNxN = EncodeIntraLumaPUNxN( img, currMB, cs_aec, &costNxN, uhBitSize );
        // backup 8x8 encode informations
        memcpy( bak_ipredmode, currMB->intra_pred_modes, sizeof( bak_ipredmode ) );
        memcpy( bak_coef[0], img->coefAC_luma[0], coef_num * sizeof( int ) ); // copy all LUMA coefficients
        memcpy( bak_coef[1], img->coefAC_luma[1], coef_num * sizeof( int ) );

        for ( y = 0; y < bsize; y++ )
        {
            for ( x = 0; x < bsize; x++ )
            {
                bak_rec[y][x] = imgY_rec[( img->mb_pix_y + y )*( img->iStride ) + img->mb_pix_x + x];
            }
        }
    }
    else
    {
        costNxN = MAX_COST;
    }

    currMB->trans_type = TRANS_2Nx2N;
    if ( EncodeIntraLumaBlk( currMB, cs_aec, 0, 0, &cost2Nx2N, bsize ) )
    {
        cbp2Nx2N = 0xFFFF;
    }
    cost2Nx2N += ( int )floor( 2.0 * img->lambda + 0.4999 );

    if ( cost2Nx2N > costNxN )
    {
        currMB->trans_type = TRANS_NxN;
        memcpy( img->bst_coefAC_luma[0], bak_coef[0], coef_num * sizeof( int ) );
        memcpy( img->bst_coefAC_luma[1], bak_coef[1], coef_num * sizeof( int ) );

        for ( y = 0; y < bsize; y++ )
        {
            for ( x = 0; x < bsize; x++ )
            {
                imgY_rec[( img->mb_pix_y + y )*( img->iStride ) + img->mb_pix_x + x] = bak_rec[y][x];
            }
        }
        memcpy( currMB->intra_pred_modes, bak_ipredmode, sizeof( bak_ipredmode ) );
        return cbpNxN;
    }
    else
    {
        memcpy( img->bst_coefAC_luma[0], img->coefAC_luma[0], coef_num * sizeof( int ) );
        memcpy( img->bst_coefAC_luma[1], img->coefAC_luma[1], coef_num * sizeof( int ) );
        return cbp2Nx2N;
    }
}

/*
*************************************************************************
* Function:R-D Cost for an 8x8 Partition
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

double RDCost_for_8x8blocks ( ImgParams *img, Macroblock *currMB, CSobj *cs_aec, int b8_idx,
                              int     ref,        // <-- reference frame
                              int     bwd_ref )  // <-- abp type
{
    int  i, j;
    int  rate=0, distortion=0;
    int  dummy, mrate;
    int  cbp     = 0;
    int  blk_x   = b8_idx%2;
    int  blk_y   = b8_idx/2;
    int  pax     = 8*blk_x;
    int  pay     = 8*blk_y;
    int  bframe  = ( img->type==B_IMG );
    int  b8mode = currMB->b8mode[b8_idx];
    int  b8pdir = currMB->b8pdir[b8_idx];
    int  direct  = ( bframe && b8mode==PSKIP );
    int  b8value = B8Mode2Value ( b8mode, b8pdir );
    int  cnt_nonz;

    int  mb_b8_y = img->mb_b8_y;
    int  mb_b8_x = img->mb_b8_x;
    int pix_y = img->mb_pix_y;
    int pix_x = img->mb_pix_x;
    uchar_t *imgY_original  =  imgY_org;
    int bsize = 8;
    int coef_num = bsize * bsize;

    int pix_y_in_mb = ( b8_idx / 2 ) << 3;
    int pix_x_in_mb = ( b8_idx % 2 ) << 3;

	// get prediction PNxN block
	int fw_ref = direct ? MAX( ref,img->pfrm_ref[mb_b8_y+blk_y][mb_b8_x+blk_x] ) : ref;
	LumaPrediction8x8( img, pix_x_in_mb, pix_y_in_mb, fw_ref, bwd_ref, b8pdir, b8mode, bsize );

    // GET COEFFICIENTS, RECONSTRUCTIONS, CBP
    cnt_nonz = LumaResidualCoding8x8_Bfrm( currMB, cs_aec, &cbp, b8_idx );

    for ( j = 0; j < bsize; j++ )
    {
        for ( i = pax; i < pax + bsize; i++ )
        {
            distortion += img->quad[imgY_original[( pix_y + pay +j )*( img->width ) + pix_x + i] - imgY_rec[( img->mb_pix_y + pay + j )*( img->iStride ) + img->mb_pix_x + i]];
        }
    }
    //  GET RATE
    // block 8x8 mode
    ue_linfo ( b8value, dummy, &mrate, &dummy );
    rate += mrate;

    // motion information
    if ( !direct )
    {
        if ( b8pdir == FORWARD  || b8pdir == BSYM )
        {
            rate  += writeOneMotionVector ( currMB, cs_aec, b8_idx, ref, 1, b8mode );
        }
        if ( b8pdir == BACKWORD || b8pdir == BSYM )
        {
            rate  += writeOneMotionVector ( currMB, cs_aec, b8_idx, bwd_ref, 0, b8mode );
        }
    }

    //----- luminance coefficients -----
    if ( cnt_nonz )
    {
        memcpy( img->bst_coefAC_luma[0] + b8_idx * coef_num, img->coefAC_luma[0] + b8_idx * coef_num, coef_num * sizeof( int ) );
        memcpy( img->bst_coefAC_luma[1] + b8_idx * coef_num, img->coefAC_luma[1] + b8_idx * coef_num, coef_num * sizeof( int ) );

        currMB->cbp = cbp;
        rate += writeLumaCoeff_AEC( cs_aec, bsize, img->coefAC_luma[0] + b8_idx * coef_num, img->coefAC_luma[1] + b8_idx * bsize * bsize );
    }

    return ( double )distortion + img->lambda * ( double )rate;
}

/*
*************************************************************************
* Function:Sets modes and reference frames for an macroblock
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/


void SetInterModesAndRef( ImgParams *img, Macroblock *currMB, int mode )
{
    int i, j, k;
    int  bframe = ( img->type == B_IMG );
    int  mb_b8_y = img->mb_b8_y;
    int  mb_b8_x = img->mb_b8_x;
    for ( i = 0; i < 4; i++ )
    {
        //currMB->trans_split[i] = 0;
    }

    // block 8x8 mode and prediction direction
    switch ( mode )
    {
        case PSKIP:
            for ( i = 0; i < 4; i++ )
            {
                currMB->b8mode[i] = PSKIP;
                currMB->b8pdir[i] = ( bframe ? BSYM : FORWARD );
                currMB->fw_ref[i] = -1;
            }
            break;
        case P2Nx2N:
        case P2NxN:
        case PNx2N:
            for ( i = 0; i < 4; i++ )
            {
                currMB->b8mode[i] = mode;
            }
            break;
        case PNxN:
            for ( i = 0; i < 4; i++ )
            {
                currMB->b8mode[i] = currMB->b8mode[i];
            }
            break;
        default:
            printf( "Unsupported mode in SetModesAndRefframeForBlocks!\n" );
            exit( 1 );
    }

    // reference frame arrays
    if ( mode == PSKIP )
    {
        if ( bframe )
        {
            for ( j = 0; j < 2; j++ )
            {
                for ( i = 0; i < 2; i++ )
                {
                    k = 2 * j + i;
                    if ( !mode )
                    {
                        img->bfrm_fref[mb_b8_y + j][mb_b8_x + i] = 0;
                        img->bfrm_bref[mb_b8_y + j][mb_b8_x + i] = 0;
                    }
                    else
                    {
                        img->bfrm_fref[mb_b8_y + j][mb_b8_x + i] = -1;
                        img->bfrm_bref[mb_b8_y + j][mb_b8_x + i] = -1;
                    }
                }
            }
        }
        else
        {
            for ( j = 0; j < 2; j++ )
            {
                for ( i = 0; i < 2; i++ )
                {
                    img->pfrm_ref[mb_b8_y + j][mb_b8_x + i] = 0;
                }
            }
        }
    }
    else
    {
        if ( bframe )
        {
            for ( j = 0; j < 2; j++ )
                for ( i = 0; i < 2; i++ )
                {
                    k = 2 * j + i;
                    if ( ( mode == PNxN ) && ( currMB->b8mode[k] == PSKIP ) )
                    {
                        img->bfrm_fref[mb_b8_y + j][mb_b8_x + i] = 0;
                        img->bfrm_bref[mb_b8_y + j][mb_b8_x + i] = 0;
                    }
                    else
                    {
                        if ( IS_FW&&IS_BW )
                        {
                            img->bfrm_fref[mb_b8_y+ j][mb_b8_x + i] = currMB->sym_ref[k][0];
                            img->bfrm_bref[mb_b8_y+ j][mb_b8_x + i] = currMB->sym_ref[k][1];
                        }
                        else
                        {
                            img->bfrm_fref[mb_b8_y+ j][mb_b8_x + i] = ( IS_FW ? currMB->fw_ref[k] : -1 );
                            img->bfrm_bref[mb_b8_y+ j][mb_b8_x + i] = ( IS_BW ? currMB->bw_ref[k] : -1 );
                        }
                    }
                }
        }
        else
        {
            for ( j = 0; j < 2; j++ )
            {
                for ( i = 0; i < 2; i++ )
                {
                    k = 2 * j + i;
                    img->pfrm_ref[mb_b8_y + j][mb_b8_x + i] = ( IS_FW ? currMB->fw_ref[k] : -1 );
                }
            }
        }
    }// mode != PSKIP
}

void SetIntraModesAndRef( ImgParams *img, Macroblock *currMB )
{
    int i, j;
    int bframe = ( img->type == B_IMG );
    int pframe = ( img->type == P_IMG );
    int mb_b8_y = img->mb_b8_y;
    int mb_b8_x = img->mb_b8_x;

    // block 8x8 mode and prediction direction
    for ( i = 0; i < 4; i++ )
    {
        currMB->sub_mb_trans_type[i] = 0;
        currMB->b8mode[i] = I_MB;
        currMB->b8pdir[i] = -1;
        currMB->fw_ref[i] = -1;
    }

    // reference frame arrays
    if ( bframe )
    {
        for ( j = 0; j < 2; j++ )
        {
            for ( i = 0; i < 2; i++ )
            {
                img->bfrm_fref[mb_b8_y + j][mb_b8_x + i] = -1;
                img->bfrm_bref[mb_b8_y + j][mb_b8_x + i] = -1;

            }
        }
    }
    else
    {
        for( j = 0; j < 2; j++ )
        {
            for ( i = 0; i < 2; i++ )
            {
                img->pfrm_ref[mb_b8_y + j][mb_b8_x + i] = -1;
            }
        }
    }
}

/*
*************************************************************************
* Function:Sets motion vectors for an macroblock
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void SetMotionVectorsMB ( Macroblock* currMB, int bframe )
{
    int b8_x, b8_y, k, b8mode, b8pdir, ref, by, bx, bxr, dref;
    int *****fmv_all = img->fmv_com;
    int *****bmv_all = img->bmv_com;
    int mb_b8_y      = img->mb_b8_y;
    int mb_b8_x      = img->mb_b8_x;
    int bw_ref;

    for ( b8_y = 0; b8_y < 2; b8_y++ )
    {
        for ( b8_x = 0; b8_x < 2; b8_x++ )
        {
            b8mode = currMB->b8mode[k=2*b8_y+b8_x];
            b8pdir = currMB->b8pdir[k];

            if( b8pdir == BSYM && b8mode != PSKIP )
            {
                fmv_all = img->mv_sym_mhp;
            }
            else if( b8pdir == MHP )
            {
                fmv_all = img->mv_sym_mhp;
            }
            else
            {
                fmv_all = img->fmv_com;
            }

            by    = mb_b8_y+b8_y;
            bxr   = mb_b8_x+b8_x;
            bx    = mb_b8_x+b8_x+4;
            ref   = ( bframe?img->bfrm_fref:img->pfrm_ref )[by][bxr];
            bw_ref = ( bframe?img->bfrm_bref:img->pfrm_ref )[by][bxr];

            if ( !bframe )
            {
                if ( b8mode != I_MB && ref != -1 )
                {
                    img->pfrm_mv[by][bx][0] = fmv_all [ref][b8mode][b8_y][b8_x][0];
                    img->pfrm_mv[by][bx][1] = fmv_all [ref][b8mode][b8_y][b8_x][1];
                }
                else
                {
                    img->pfrm_mv[by][bx][0] = 0;
                    img->pfrm_mv[by][bx][1] = 0;
                }
            }
            else
            {
                if ( b8pdir == -1 ) // intra
                {
                    img->bfrm_fmv[by][bx][0] = 0;
                    img->bfrm_fmv[by][bx][1] = 0;
                    img->bfrm_bmv[by][bx][0] = 0;
                    img->bfrm_bmv[by][bx][1] = 0;
                }
                else if ( b8pdir == FORWARD )  // forward
                {
                    img->bfrm_fmv[by][bx][0] = fmv_all[ref][b8mode][b8_y][b8_x][0];
                    img->bfrm_fmv[by][bx][1] = fmv_all[ref][b8mode][b8_y][b8_x][1];
                    img->bfrm_bmv[by][bx][0] = 0;
                    img->bfrm_bmv[by][bx][1] = 0;
                }
                else if ( b8pdir == BACKWORD )  // backward
                {
                    img->bfrm_fmv[by][bx][0] = 0;
                    img->bfrm_fmv[by][bx][1] = 0;
                    img->bfrm_bmv[by][bx][0] = bmv_all[bw_ref][b8mode][b8_y][b8_x][0];
                    img->bfrm_bmv[by][bx][1] = bmv_all[bw_ref][b8mode][b8_y][b8_x][1];
                }
                else if ( b8mode != PSKIP )  // SYM
                {
                    img->bfrm_fmv[by][bx][0] = fmv_all[ref][b8mode][b8_y][b8_x][0];
                    img->bfrm_fmv[by][bx][1] = fmv_all[ref][b8mode][b8_y][b8_x][1];
                    img->bfrm_bmv[by][bx][0] = GenSymBackMV(fmv_all[ref][b8mode][b8_y][b8_x][0]);
                    img->bfrm_bmv[by][bx][1] = GenSymBackMV(fmv_all[ref][b8mode][b8_y][b8_x][1]);
                }
                else    // direct
                {
                    dref = 0;
                    img->bfrm_fmv[by][bx][0] = fmv_all[dref][0][b8_y][b8_x][0];
                    img->bfrm_fmv[by][bx][1] = fmv_all[dref][0][b8_y][b8_x][1];
                    img->bfrm_bmv[by][bx][0] = bmv_all[   0][0][b8_y][b8_x][0];
                    img->bfrm_bmv[by][bx][1] = bmv_all[   0][0][b8_y][b8_x][1];
                }
            }
        }
    }
}
/*
*************************************************************************
* Function:Store macroblock parameters
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void store_best_parameters ( ImgParams *img, Macroblock *currMB, best_cu_info_t *bst, int mode, uchar_t uhBitSize )
{
    int x, y;
    int bframe = ( img->type == B_IMG );
    int **frefar = bframe ? img->bfrm_fref : img->pfrm_ref;
    int **brefar = img->bfrm_bref;
    int b8_x = img->mb_b8_x;
    int b8_y = img->mb_b8_y;
    int bsize = ( 1 << uhBitSize );
    int bsize_cr = ( bsize >> 1 );
    int blk_pix_num = bsize * bsize;
    int blk_pix_num_cr = bsize_cr * bsize_cr;

    // store best info
    bst->uhBitSize = uhBitSize;
    bst->best_mode = mode;
    bst->best_trans_type = currMB->trans_type;

    // reconstructed blocks
    for ( y = 0; y < bsize; y++ )
    {
        for ( x = 0; x < bsize; x++ )
        {
            bst->rec_mbY[y][x] = imgY_rec[( img->mb_pix_y + y )*( img->iStride ) + img->mb_pix_x + x];
        }
    }

    for ( y = 0; y < bsize_cr; y++ )
    {
        for ( x = 0; x < bsize_cr; x++ )
        {
            bst->rec_mbU[y][x] = imgU_rec[( img->mb_pix_y_cr + y )*( img->iStrideC ) + img->mb_pix_x_cr + x];
            bst->rec_mbV[y][x] = imgV_rec[( img->mb_pix_y_cr + y )*( img->iStrideC ) + img->mb_pix_x_cr + x];
        }
    }

    // coeff, cbp, kac
    if ( mode || bframe )
    {
        memcpy( bst->coef_y[0], img->bst_coefAC_luma[0], blk_pix_num * sizeof( int ) );
        memcpy( bst->coef_y[1], img->bst_coefAC_luma[1], blk_pix_num * sizeof( int ) );
        memcpy( bst->coef_u[0], img->coefAC_chroma[0][0], blk_pix_num_cr * sizeof( int ) );
        memcpy( bst->coef_u[1], img->coefAC_chroma[0][1], blk_pix_num_cr * sizeof( int ) );
        memcpy( bst->coef_v[0], img->coefAC_chroma[1][0], blk_pix_num_cr * sizeof( int ) );
        memcpy( bst->coef_v[1], img->coefAC_chroma[1][1], blk_pix_num_cr * sizeof( int ) );
        bst->cbp = currMB->cbp;
    }
    else
    {
        bst->cbp = 0;
    }

    for ( x = 0; x < 4; x++ )
    {
        bst->best_b8mode[x] = currMB->b8mode[x];
        bst->best_b8pdir[x] = currMB->b8pdir[x];
    }

    if( mode == I_MB )
    {
        bst->best_c_imode = currMB->c_ipred_mode;
        memcpy( best_splite, currMB->sub_mb_trans_type, sizeof( best_splite ) );
        for ( x = 0; x < bsize; x++ )
        {
            bst->best_intra_pred_modes_tmp[x] = currMB->intra_pred_modes[x];
        }
    }
    else
    {
        for ( y = 0; y < 2; y++ )
        {
            for ( x = 0; x < 2; x++ )
            {
                bst->fref[y][x] = frefar[b8_y + y][b8_x + x];
                if ( bframe )
                {
                    bst->bref[y][x] = brefar[b8_y + y][b8_x + x];
                }
            }
        }
        if( mode != PSKIP )
        {
            memcpy( bst->mvd, currMB->mvd, 4 * 2 * 2 * sizeof( int ) );
        }
    }
}

double GetDistortion( ImgParams *img, int bsize )
{
    int dist = 0;
    int bsize_c = bsize >> 1;
    int x, y;

    int pix_y = img->mb_pix_y;
    int pix_x = img->mb_pix_x;
    int pix_c_y = img->mb_pix_y_cr;
    int pix_c_x = img->mb_pix_x_cr;

    //luma
    for ( y = 0; y < bsize; y++ )
    {
        for ( x = 0; x < bsize; x++ )
        {
            dist += img->quad[imgY_org[( y + pix_y )*( img->width ) + x + pix_x] - imgY_rec[( img->mb_pix_y + y )*( img->iStride ) + img->mb_pix_x + x]];
        }
    }

    //chroma
    for ( y = 0; y < bsize_c; y++ )
    {
        for ( x = 0; x < bsize_c; x++ )
        {
            dist += img->quad[imgU_org[( y + pix_c_y )*( img->width_cr ) + x + pix_c_x] - imgU_rec[( img->mb_pix_y_cr + y )*( img->iStrideC ) + img->mb_pix_x_cr + x]];
            dist += img->quad[imgV_org[( y + pix_c_y )*( img->width_cr ) + x + pix_c_x] - imgV_rec[( img->mb_pix_y_cr + y )*( img->iStrideC ) + img->mb_pix_x_cr + x]];
        }
    }
    return dist;
}

/*
*************************************************************************
* Function:R-D Cost for a macroblock
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void xEncodeInterPU( ImgParams *img, Macroblock *currMB, CSobj *cs_aec, best_cu_info_t *bst, double lambda, int mode, double *min_rdcost, int isBdirect, uchar_t uhBitSize )
{
    double rdcost;
    double distortion = 0;
    int bsize = ( 1 << uhBitSize );
    int bsize_cr = ( 1 << ( uhBitSize - 1 ) );
    int isBfrm = ( img->type == B_IMG );
    int pix_y = img->mb_pix_y;
    int pix_c_y = img->mb_pix_y_cr;
    int pix_x = img->mb_pix_x;
    int pix_c_x = img->mb_pix_x_cr;
    int rate = 0;
	int b8;
    int uv_idx;
    int tmp_cbp;
    int tmp_cbp_luma;
    int tmp_mvd[2][2][2][2];
    CSobj *cs_tmp = create_coding_state();

    currMB->mb_type = mode;
    currMB->trans_type = TRANS_NxN;
    mv_out_of_range = 1;
    if ( mode == P2Nx2N )
    {
        currMB->trans_type = TRANS_2Nx2N;
    }

    // set reference frames and block modes
    SetInterModesAndRef( img, currMB, mode );

    for ( b8 = 0; b8 < 4; b8++ ) {
        int fw_ref, bw_ref;
        int pix_y_in_mb = ( b8 / 2 ) << 3;
        int pix_x_in_mb = ( b8 % 2 ) << 3;
        int b8mode = currMB->b8mode[b8];
        int b8pdir = currMB->b8pdir[b8];
        
        SetModesAndRefframe( currMB, b8, &fw_ref, &bw_ref );
        LumaPrediction8x8( img, pix_x_in_mb, pix_y_in_mb, fw_ref, bw_ref, b8pdir, b8mode, (bsize >> 1) );
    }

    // GET COEFFICIENTS, RECONSTRUCTIONS, CBP
    LumaResidualCoding( currMB, cs_aec, uhBitSize );
    tmp_cbp_luma = currMB->cbp;
    EncodeInterChromaCU( currMB, cs_aec, isBdirect );

    store_coding_state( cs_tmp );
    memcpy ( tmp_mvd, currMB->mvd, 4 * 2 * 2 * sizeof( int ) );

    tmp_cbp  = ( currMB->cbp != 0 );
    rate = writeCUHeader( currMB, cs_aec, img->current_mb_nr, 0 );
    if ( mode )
    {
        storeMotionInfo( currMB );
        rate += writeMVD( currMB, cs_aec );
        rate += ( 2 * writeCBPandDqp( currMB, cs_aec ) );
    }
    if ( mode || ( isBfrm && tmp_cbp ) )
    {
        rate += writeLumaCoeffBlk( currMB, uhBitSize, cs_aec, img->coefAC_luma[0], img->coefAC_luma[1] );
        for( uv_idx = 0; uv_idx < 2; uv_idx++ )
        {
            int isIntra = IS_INTRA( currMB );
            int cbp_cr = ( currMB->cbp >> 4*( uv_idx+4 ) );
            if( cbp_cr & 0xf )
            {
                rate += writeChromaCoeff_AEC( cs_aec, isIntra, bsize_cr, img->coefAC_chroma[uv_idx][0], img->coefAC_chroma[uv_idx][1] );
            }
        }
    }
    reset_coding_state( cs_tmp );
    memcpy ( currMB->mvd, tmp_mvd, 4 * 2 * 2 * sizeof( int ) );
    distortion = GetDistortion( img, bsize );
    rdcost = distortion + lambda * ( double )( rate );

    if ( rdcost < *min_rdcost && mv_out_of_range )
    {
        *min_rdcost = rdcost;
        memcpy(img->bst_coefAC_luma[0], img->coefAC_luma[0], COF_SIZE_LUMA * sizeof(int));
        memcpy(img->bst_coefAC_luma[1], img->coefAC_luma[1], COF_SIZE_LUMA * sizeof(int));
        store_best_parameters( img, currMB, bst, mode, uhBitSize );
    }
    delete_coding_state(cs_tmp);
}

void xEncodeIntraCU( ImgParams *img, Macroblock *currMB, CSobj *cs_aec, best_cu_info_t *bst, int mode, double *min_rdcost, uchar_t uhBitSize )
{
    double     rdcost;
    double     distortion;
    int rate, uv;
    int bsize = ( 1 << uhBitSize );
    int tmp_cbp_luma;
    //int bsize    = (1 << uhBitSize);
    int bsize_cr = ( 1 << ( uhBitSize-1 ) );
    int cu_mb_width = 1 << ( uhBitSize - MB_SIZE_LOG2 );
    int cu_mb_num = cu_mb_width * cu_mb_width;
    int mb_idx;
    int tmp_mvd[2][2][2][2];
    CSobj *cs_tmp =  create_coding_state();

    int img_cx   = img->mb_pix_x_cr;
    int img_cy   = img->mb_pix_y_cr;
    int mb_nr    = img->current_mb_nr;
    int mb_width = img->PicWidthInMbs;
    bool mb_available_up   = ( img_cy == 0 ) ? 0 : ( currMB->slice_nr == img->mb_data[mb_nr-mb_width].slice_nr );
    bool mb_available_left = ( img_cx == 0 ) ? 0 : ( currMB->slice_nr == img->mb_data[mb_nr-1].slice_nr );
    bool mb_available_up_left = ( img_cx/BLOCK_SIZE == 0 || img_cy/BLOCK_SIZE == 0 ) ? 0 : ( currMB->slice_nr == img->mb_data[mb_nr-mb_width-1].slice_nr );

    // this variables for intra chroma 8x8 block prediction.
    uchar_t edgepix_uv [4*LCU_SIZE+2] = {0};
    uchar_t *pedge       = ( edgepix_uv + LCU_SIZE );
    uchar_t *edgepix_org = ( edgepix_uv + 2*LCU_SIZE+1 );
    uchar_t *pedge_uv    = ( edgepix_org + LCU_SIZE );
    uchar_t *psrc;
    bool bAboveAvail, bLeftAvail;

    currMB->mb_type = mode;
    currMB->trans_type = TRANS_NxN;

    SetIntraModesAndRef( img, currMB );
    currMB->cbp = EncodeIntraLumaCU( img, currMB, cs_aec, uhBitSize );
    tmp_cbp_luma = currMB->cbp;

    for ( currMB->c_ipred_mode = DC_PRED_8; currMB->c_ipred_mode < ChromaPredModeNum; currMB->c_ipred_mode++ )
    {
        if ( ( currMB->c_ipred_mode == VERT_PRED_8 && !mb_available_up ) ||
                ( currMB->c_ipred_mode == HOR_PRED_8 && !mb_available_left ) ||
                ( currMB->c_ipred_mode == PLANE_8 && ( !mb_available_left || !mb_available_up || !mb_available_up_left ) ) )
        {
            continue;
        }

        currMB->cbp = tmp_cbp_luma;

        // Predict an intra chroma 8x8 block for both U and V
        for ( uv = 0; uv < 2; uv++ )
        {
            xPrepareIntraPatternC( img, currMB, pedge, uv, bsize_cr, &bAboveAvail, &bLeftAvail );

            // save the original edge pixels before filtering
            memcpy( edgepix_org, edgepix_uv, sizeof( pel_t )*( 2*LCU_SIZE+1 ) );

            psrc = ( currMB->c_ipred_mode == DC_PRED_8 ) ? pedge : pedge_uv;
            intra_pred_chroma( psrc, img->pred_blk_chroma[uv], MB_SIZE, currMB->c_ipred_mode, 8, bAboveAvail, bLeftAvail );

            xCheckIntraChromaTQ( currMB, cs_aec, uv, currMB->c_ipred_mode, uhBitSize );
        }

        store_coding_state( cs_tmp );
        memcpy ( tmp_mvd, currMB->mvd, 4 * 2 * 2 * sizeof( int ) );

        rate = writeCUHeader( currMB, cs_aec, img->current_mb_nr, 0 );
        rate += ( 2 * writeCBPandDqp( currMB, cs_aec ) );
        for( mb_idx = 0; mb_idx < cu_mb_num; mb_idx++ )
        {
            rate += writeLumaCoeffBlk( currMB, uhBitSize, cs_aec, img->bst_coefAC_luma[0], img->bst_coefAC_luma[1] );
        }

        for( uv = 0; uv < 2; uv++ )
        {
            int isIntra = IS_INTRA( currMB );
            int cbp_cr = ( currMB->cbp >> 4*( uv+4 ) );
            if( cbp_cr & 0xf )
            {
                rate += writeChromaCoeff_AEC( cs_aec, isIntra, bsize_cr, img->coefAC_chroma[uv][0], img->coefAC_chroma[uv][1] );
            }
        }
        reset_coding_state( cs_tmp );
        memcpy ( currMB->mvd, tmp_mvd, 2*2*BLOCK_MULTIPLE*BLOCK_MULTIPLE*sizeof( int ) );
        distortion = GetDistortion( img, bsize );
        rdcost = distortion + img->lambda * ( double )( rate );
        if ( rdcost < *min_rdcost )
        {
            *min_rdcost = rdcost;
            store_best_parameters( img, currMB, bst, mode, uhBitSize );
        }
    }
    delete_coding_state( cs_tmp );
}

/*
*************************************************************************
* Function:Set stored macroblock parameters
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void revert_large_cu_param( ImgParams *img, int mb_nr, best_cu_info_t *bst, uchar_t uhBitSize )
{
    int x, y;
    int mode = bst->best_mode;
    int bframe = ( img->type == B_IMG );
    int **frefar = bframe ? img->bfrm_fref : img->pfrm_ref;
    int **brefar = img->bfrm_bref;
    int b4_x = img->mb_b4_x;
    int b4_y = img->mb_b4_y;
    int bsize = ( 1 << uhBitSize );
    int bsize_cr = ( bsize >> 1 );
    int blk_pix_num = bsize * bsize;
    int blk_pix_num_cr = bsize_cr * bsize_cr;
    int cu_mb_width = 1 << ( uhBitSize - MB_SIZE_LOG2 );
    Macroblock *tmpMB = img->mb_data + mb_nr;

    init_mb_params( tmpMB, uhBitSize, mb_nr );

    tmpMB->uhBitSizeMB = uhBitSize;
    tmpMB->cbp = bst->cbp;
    tmpMB->mb_type = mode;
    tmpMB->trans_type = bst->best_trans_type;

    // reconstruction values
    for ( y = 0; y < bsize; y++ )
    {
        for ( x = 0; x < bsize; x++ )
        {
            imgY_rec[( img->mb_pix_y + y )*( img->iStride ) + img->mb_pix_x + x] = bst->rec_mbY[y][x];
        }
    }

    for ( y = 0; y < bsize_cr; y++ )
    {
        for ( x = 0; x < bsize_cr; x++ )
        {
            imgU_rec[( img->mb_pix_y_cr + y )*( img->iStrideC ) + img->mb_pix_x_cr + x] = bst->rec_mbU[y][x];
            imgV_rec[( img->mb_pix_y_cr + y )*( img->iStrideC ) + img->mb_pix_x_cr + x] = bst->rec_mbV[y][x];
        }
    }

    // coefficients and cbp
    memcpy( img->bst_coefAC_luma[0],  bst->coef_y[0], blk_pix_num * sizeof( int ) );
    memcpy( img->bst_coefAC_luma[1],  bst->coef_y[1], blk_pix_num * sizeof( int ) );
    memcpy( img->bst_coefAC_chroma[0][0], bst->coef_u[0], blk_pix_num_cr * sizeof( int ) );
    memcpy( img->bst_coefAC_chroma[0][1], bst->coef_u[1], blk_pix_num_cr * sizeof( int ) );
    memcpy( img->bst_coefAC_chroma[1][0], bst->coef_v[0], blk_pix_num_cr * sizeof( int ) );
    memcpy( img->bst_coefAC_chroma[1][1], bst->coef_v[1], blk_pix_num_cr * sizeof( int ) );

    for ( x = 0; x < 4; x++ )
    {
        tmpMB->b8mode[x] = bst->best_b8mode[x];
        tmpMB->b8pdir[x] = bst->best_b8pdir[x];
    }

    if ( tmpMB->mb_type == I_MB )
    {
        for ( x = 0; x < 16; x++ )
        {
            tmpMB->intra_pred_modes[x] = bst->best_intra_pred_modes_tmp[x];
        }
        tmpMB->c_ipred_mode = bst->best_c_imode;
        memcpy( tmpMB->sub_mb_trans_type, best_splite, sizeof( best_splite ) );
    }
    else
    {
        // reference frames. Due to MV of B-direct mode is derived from col-located P block, so the best ref and MV of P block must be stored.
        for ( y = 0; y < 2; y++ )
        {
            for ( x = 0; x < 2; x++ )
            {
                frefar[( b4_y >> 1 ) + y][( b4_x >> 1 ) + x] = bst->fref[y][x];
                if ( img->type == P_IMG )
                {
                    tmpMB->fw_ref[y * 2 + x] = bst->fref[y][x];
                }
            }
        }
        if ( bframe )
        {
            for ( y = 0; y < 2; y++ )
            {
                for ( x = 0; x < 2; x++ )
                {
                    brefar[( b4_y >> 1 ) + y][( b4_x >> 1 ) + x] = bst->bref[y][x];
                }
            }
        }
    }

    if( img->type != I_IMG )
    {
        SetMotionVectorsMB( tmpMB, bframe );
    }
    if ( IS_INTERMV( tmpMB ) )
    {
        storeMotionInfo( tmpMB );
    }
}

void SetRefAndMotionVectors ( int b8, int b8mode, int ref, int bw_ref,int b8pdir )
{
    int  bframe     = ( img->type==B_IMG );
    int  b8_y       = ( b8/2 );
    int  b8_x       = ( b8%2 );
    int**  frefArr  = ( bframe ? img->bfrm_fref : img->pfrm_ref );
    int**  brefArr  = img->bfrm_bref;

    int  mb_b8_x    = img->mb_b8_x;
    int  mb_b8_y    = img->mb_b8_y;

    int  bidmhp = ( b8pdir == BSYM && b8mode != PSKIP ) || ( ( img->type == P_IMG ) && ( b8pdir == MHP ) );
    int** ***fmv_all = ( bidmhp ) ? img->mv_sym_mhp : img->fmv_com;
    int** ***bmv_all = img->bmv_com;

    int by  = mb_b8_y + b8_y;
    int bxr = mb_b8_x + b8_x;
    int bx  = mb_b8_x + b8_x + 4;

    frefArr[by][bxr] = -1;

    if ( !bframe ) // P frame
    {
        if ( b8mode != I_MB && ref != -1 )
        {
            frefArr [by][bxr] = ref;
            img->pfrm_mv [by][bx][0] = fmv_all[ref][b8mode][b8_y][b8_x][0];
            img->pfrm_mv [by][bx][1] = fmv_all[ref][b8mode][b8_y][b8_x][1];
        }
        else
        {
            img->pfrm_mv [by][bx][0] = 0;
            img->pfrm_mv [by][bx][1] = 0;
            frefArr [by][bxr] = -1;
        }
    }
    else     // B frame
    {
        brefArr[by][bxr] = -1;
        if ( b8pdir == -1 )
        {
            img->bfrm_fmv [by][bx][0] = 0;
            img->bfrm_fmv [by][bx][1] = 0;
            img->bfrm_bmv [by][bx][0] = 0;
            img->bfrm_bmv [by][bx][1] = 0;
            frefArr  [by][bxr] = -1;
            brefArr  [by][bxr] = -1;
        }
        else if ( b8pdir == FORWARD )
        {
            frefArr  [by][bxr] = ref;
            img->bfrm_fmv [by][bx][0] = fmv_all[ref][b8mode][b8_y][b8_x][0];
            img->bfrm_fmv [by][bx][1] = fmv_all[ref][b8mode][b8_y][b8_x][1];
            img->bfrm_bmv [by][bx][0] = 0;
            img->bfrm_bmv [by][bx][1] = 0;
        }
        else if ( b8pdir == BACKWORD )
        {
            brefArr  [by][bxr] = bw_ref;
            img->bfrm_fmv [by][bx][0] = 0;
            img->bfrm_fmv [by][bx][1] = 0;
            img->bfrm_bmv [by][bx][0] = bmv_all[bw_ref][b8mode][b8_y][b8_x][0];
            img->bfrm_bmv [by][bx][1] = bmv_all[bw_ref][b8mode][b8_y][b8_x][1];
        }
        else if ( b8mode != PSKIP ) // SYM
        {
            frefArr  [by][bxr] = ref;
            brefArr  [by][bxr] = bw_ref;
            img->bfrm_fmv [by][bx][0] = fmv_all[ref][b8mode][b8_y][b8_x][0];
            img->bfrm_fmv [by][bx][1] = fmv_all[ref][b8mode][b8_y][b8_x][1];
            img->bfrm_bmv [by][bx][0] = GenSymBackMV(fmv_all[ref][b8mode][b8_y][b8_x][0]);
            img->bfrm_bmv [by][bx][1] = GenSymBackMV(fmv_all[ref][b8mode][b8_y][b8_x][1]);
        }
        else // PSKIP or BDIRECT
        {
            frefArr  [by][bxr] = 0;
            brefArr  [by][bxr] = 0;
            img->bfrm_fmv [by][bx][0] = fmv_all[0][0][b8_y][b8_x][0];
            img->bfrm_fmv [by][bx][1] = fmv_all[0][0][b8_y][b8_x][1];
            img->bfrm_bmv [by][bx][0] = bmv_all[0][0][b8_y][b8_x][0];
            img->bfrm_bmv [by][bx][1] = bmv_all[0][0][b8_y][b8_x][1];
        }
    }
}

void MotionSearch( ImgParams *img, Macroblock *currMB, int mode, int pu_idx, int *best_pdir, int  *best_fw_ref, int *best_bw_ref, int *bid_best_fw_ref, int *bid_best_bw_ref )
{
    int bframe = ( img->type == B_IMG );
    int fw_mcost, bw_mcost, bid_mcost;
    int p_bid_mcost = MAX_COST;
    int p_bid_ref   = -1;

    if( mode == PNxN && bframe )
    {
        ForwardMVSearch( currMB, &fw_mcost, best_fw_ref, best_bw_ref, mode,  pu_idx );
        *best_pdir = FORWARD;
        BidirectionalMVSearch( currMB, best_bw_ref, &bw_mcost, &bid_mcost, bid_best_fw_ref, bid_best_bw_ref, mode,  pu_idx );
        if ( fw_mcost < bw_mcost && fw_mcost < bid_mcost )
        {
            *best_pdir = FORWARD;
            *best_bw_ref = 0;
        }
        else if ( bw_mcost < fw_mcost && bw_mcost < bid_mcost )
        {
            *best_pdir = BACKWORD;
            *best_fw_ref = 0;
        }
        else
        {
            *best_pdir = BSYM;
        }
    }
    else
    {
        ForwardMVSearch( currMB, &fw_mcost, best_fw_ref, best_bw_ref, mode,  pu_idx );
        *best_pdir = FORWARD;

        if ( img->type == P_IMG && input->multiple_hp_flag )
        {
            ForwardMVSearchMhp( currMB, &p_bid_mcost, &p_bid_ref, best_bw_ref, mode, pu_idx );
            if ( fw_mcost <= p_bid_mcost )
            {
                *best_pdir = FORWARD;
            }
            else
            {
                *best_pdir = MHP;
                *best_fw_ref = p_bid_ref;
            }
        }

        if ( bframe )
        {
            BidirectionalMVSearch( currMB, best_bw_ref, &bw_mcost, &bid_mcost, bid_best_fw_ref, bid_best_bw_ref, mode,  pu_idx );
            //get prediction direction
            if ( fw_mcost <= bw_mcost && fw_mcost <= bid_mcost )
            {
                *best_pdir = FORWARD;
                *best_bw_ref = 0;
            }
            else if ( bw_mcost <= fw_mcost && bw_mcost <= bid_mcost )
            {
                *best_pdir = BACKWORD;
                *best_fw_ref = 0;
            }
            else
            {
                *best_pdir = BSYM;
            }
        }
    }
}

void MotionEstimationBfrmSkip( ImgParams *img, Macroblock *currMB, CSobj *cs_aec, uchar_t uhCuBitSize, int mode, int* valid )
{
    double rdcost;
    int best_fw_ref=0, best_bw_ref=0, bid_best_fw_ref=0, bid_best_bw_ref=0, best_pdir=0;
    int pu_idx;
    CSptr cs_b8   = create_coding_state();
    CSptr cs_tmp  = create_coding_state();
    int tmp_mvd1[2][2][2][2];
    double pskip_rdcost[4];

    // store coding state of macroblock
    store_coding_state( cs_tmp );

    //B frame sub-block SKIP
    for ( pu_idx = 0; pu_idx < 4; pu_idx++ )
    {
        best_fw_ref = -1;
        best_pdir   = BSYM;
        best_bw_ref = 0;
        bid_best_fw_ref = 0;
        bid_best_bw_ref = 0;

        currMB->b8mode[pu_idx] = PSKIP;
        currMB->b8pdir[pu_idx] = best_pdir;
        pskip_rdcost[pu_idx] = RDCost_for_8x8blocks( img, currMB, cs_aec, pu_idx, bid_best_fw_ref, bid_best_bw_ref );
        if ( !img->mv_range_flag )
        {
            pskip_rdcost[pu_idx] = MAX_COST;
            img->mv_range_flag = 1;
        }

        store_coding_state( cs_b8 );
        memcpy( tmp_mvd1, currMB->mvd, 4 * 2 * 2 * sizeof( int ) );

        //set the coding state after current block
        reset_coding_state( cs_b8 );
        memcpy( currMB->mvd, tmp_mvd1, 4 * 2 * 2 * sizeof( int ) );
    }//end of each 8x8 block

    //B frame sub-block ME
    for ( pu_idx = 0; pu_idx < 4; pu_idx++ )
    {
        MotionSearch( img, currMB, PNxN, pu_idx, &best_pdir, &best_fw_ref, &best_bw_ref, &bid_best_fw_ref, &bid_best_bw_ref );

        currMB->b8mode[pu_idx] = PNxN;
        currMB->b8pdir[pu_idx] = best_pdir;
        rdcost = RDCost_for_8x8blocks( img, currMB, cs_aec, pu_idx, ( best_pdir==BSYM ? bid_best_fw_ref : best_fw_ref ), ( best_pdir==BSYM ? bid_best_bw_ref : best_bw_ref ) );
        if ( rdcost < pskip_rdcost[pu_idx] )
        {
            store_coding_state( cs_b8 ); //store coding state
            memcpy( tmp_mvd1, currMB->mvd, 4 * 2 * 2 * sizeof( int ) );
        }
        else
        {
            currMB->b8mode[pu_idx] = PSKIP;
            best_fw_ref = -1;
            best_pdir   = BSYM;
            best_bw_ref = 0;
            bid_best_fw_ref = 0;
            bid_best_bw_ref = 0;
        }

        //these info is only used in mv prediction of subsequent blocks in same CU, so the info of last block is useless.
        if ( pu_idx < 3 )
        {
            SetRefAndMotionVectors( pu_idx, currMB->b8mode[pu_idx], best_pdir == BSYM ? bid_best_fw_ref : best_fw_ref, best_pdir == BSYM ? bid_best_bw_ref : best_bw_ref, best_pdir );
        }

        currMB->b8pdir[pu_idx]     = best_pdir;
        currMB->fw_ref [pu_idx]    = best_fw_ref;
        currMB->bw_ref [pu_idx]    = best_bw_ref;
        currMB->sym_ref[pu_idx][0] = bid_best_fw_ref;
        currMB->sym_ref[pu_idx][1] = bid_best_bw_ref;

        //set the coding state after current block
        reset_coding_state( cs_b8 );
        memcpy( currMB->mvd, tmp_mvd1, 4 * 2 * 2 * sizeof( int ) );
    }//end of each 8x8 block

    // re-set coding state (as it was before 8x8 block coding)
    reset_coding_state( cs_tmp );

    delete_coding_state( cs_b8 );
    delete_coding_state( cs_tmp );
}

void MotionEstimation( ImgParams *img, Macroblock *currMB, uchar_t uhCuBitSize, int mode )
{
    int bframe = ( img->type == B_IMG );
    int max_mcost = MAX_COST;
    int pu_num[5] = {1, 1, 2, 2, 4};
    int best_fw_ref=0, best_bw_ref=0, bid_best_fw_ref=0, bid_best_bw_ref=0, best_pdir=0;
    int pu_idx;

    // motion estimation for 16x16 blocks
    if( mode > PSKIP && mode <= PNxN )
    {
        for ( pu_idx = 0; pu_idx < pu_num[mode]; pu_idx++ )
        {
            if ( mode < PNxN )
            {
                init_pu_pos( mode, uhCuBitSize, pu_idx );
            }

            MotionSearch( img, currMB, mode, pu_idx, &best_pdir, &best_fw_ref, &best_bw_ref, &bid_best_fw_ref, &bid_best_bw_ref );
            if ( mode == PNx2N )
            {
                currMB->fw_ref [pu_idx]    = currMB->fw_ref [pu_idx+2]    = best_fw_ref;
                currMB->b8pdir [pu_idx]    = currMB->b8pdir [pu_idx+2]    = best_pdir;
                currMB->bw_ref [pu_idx]    = currMB->bw_ref [pu_idx+2]    = best_bw_ref;
                currMB->sym_ref[pu_idx][0] = currMB->sym_ref[pu_idx+2][0] = bid_best_fw_ref;
                currMB->sym_ref[pu_idx][1] = currMB->sym_ref[pu_idx+2][1] = bid_best_bw_ref;
            }
            else if ( mode == P2NxN )
            {
                currMB->fw_ref [2*pu_idx]    = currMB->fw_ref [2*pu_idx+1]    = best_fw_ref;
                currMB->b8pdir [2*pu_idx]    = currMB->b8pdir [2*pu_idx+1]    = best_pdir;
                currMB->bw_ref [2*pu_idx]    = currMB->bw_ref [2*pu_idx+1]    = best_bw_ref;
                currMB->sym_ref[2*pu_idx][0] = currMB->sym_ref[2*pu_idx+1][0] = bid_best_fw_ref;
                currMB->sym_ref[2*pu_idx][1] = currMB->sym_ref[2*pu_idx+1][1] = bid_best_bw_ref;
            }
            else if ( mode == P2Nx2N )
            {
                currMB->fw_ref [0]    = currMB->fw_ref[1]     = currMB->fw_ref[2]     = currMB->fw_ref[3]     = best_fw_ref;
                currMB->b8pdir [0]    = currMB->b8pdir[1]     = currMB->b8pdir[2]     = currMB->b8pdir[3]     = best_pdir;
                currMB->bw_ref [0]    = currMB->bw_ref[1]     = currMB->bw_ref[2]     = currMB->bw_ref[3]     = best_bw_ref;
                currMB->sym_ref[0][0] = currMB->sym_ref[1][0] = currMB->sym_ref[2][0] = currMB->sym_ref[3][0] = bid_best_fw_ref;
                currMB->sym_ref[0][1] = currMB->sym_ref[1][1] = currMB->sym_ref[2][1] = currMB->sym_ref[3][1] = bid_best_bw_ref;
            }
            else if ( mode == PNxN )
            {
                currMB->fw_ref [pu_idx]    = best_fw_ref;
                currMB->b8pdir [pu_idx]    = best_pdir;
                currMB->b8mode [pu_idx]    = mode;
                currMB->bw_ref [pu_idx]    = best_bw_ref;
                currMB->sym_ref[pu_idx][0] = bid_best_fw_ref;
                currMB->sym_ref[pu_idx][1] = bid_best_bw_ref;
            }

            // these info is only used in mv prediction of subsequent blocks in same CU, so the info of last block is useless.
            if ( ( mode > P2Nx2N && pu_idx == 0 && mode < PNxN ) || ( !bframe && mode == PNxN && pu_idx < 3 ) )
            {
                SetRefAndMotionVectors( pu_idx, mode, best_pdir == BSYM ? bid_best_fw_ref : best_fw_ref, best_pdir == BSYM ? bid_best_bw_ref : best_bw_ref, best_pdir );
            }
        }
    }
}

/*
*************************************************************************
* Function:Mode Decision for a macroblock
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
double EncodeOneCU(ImgParams *img, Macroblock *currMB, CSobj *cs_aec, best_cu_info_t *bst, uchar_t uhBitSize)
{
    static const int  mb_mode_table[MAXMODE - 1] = { PSKIP, P2Nx2N, P2NxN, PNx2N, PNxN };
    int    valid[MAXMODE - 1];
    int    index, mode;
    double min_rdcost = (1 << 30);
    int    isIntraFrm = (img->type == I_IMG);
    int    isPFrm = (img->type == P_IMG);
    int    isBFrm = (img->type == B_IMG);
    int isBdirect, isPskip;
    int dir;
    int qp_offset;
    img->mv_range_flag = 1;

    if (input->usefme)
    {
        decide_intrabk_SAD();
    }

    // SET VALID MODES
    valid[PSKIP] = (!isIntraFrm); //skip
    valid[P2Nx2N] = (!isIntraFrm); //16x16
    valid[P2NxN] = (!isIntraFrm && input->InterSearch16x8);
    valid[PNx2N] = (!isIntraFrm && input->InterSearch8x16);
    valid[PNxN] = (!isIntraFrm && input->InterSearch8x8);
    
    for (qp_offset = -img->enc_mb_delta_qp; qp_offset <= img->enc_mb_delta_qp; qp_offset++)
    {
        img->qp = img->frame_qp + qp_offset;
        for (index = 0; index < MAXMODE - 1; index++)
        {
            mode = mb_mode_table[index];
            if (valid[mode])
            {
                isBdirect = isBFrm && mode == PSKIP;
                isPskip = isPFrm && mode == PSKIP;
                if (isBdirect)
                {
                    GetBdirectMV_enc(currMB);
                }
                if (isPskip)
                {
                    GetPskipMV_enc(img, currMB);
                }

                if (IS_INTERMV_MODE(mode))
                {
                    if (mode == PNxN && isBFrm)
                    {
                        MotionEstimationBfrmSkip(img, currMB, cs_aec, uhBitSize, mode, valid);
                    }
                    else
                    {
                        MotionEstimation(img, currMB, uhBitSize, mode);
                    }

                }
                if (isPFrm && !mode && !img->mv_range_flag)
                {
                    img->mv_range_flag = 1;
                    continue;
                }

                //--- for P2NX2N check all prediction directions ---
                if (mode == P2Nx2N && isBFrm)
                {
                    for (dir = 0; dir < MaxBDir - 1; dir++)
                    {
                        currMB->b8pdir[0] = currMB->b8pdir[1] = currMB->b8pdir[2] = currMB->b8pdir[3] = dir;
                        xEncodeInterPU(img, currMB, cs_aec, bst, img->lambda, mode, &min_rdcost, isBdirect, uhBitSize);
                    }
                }
                else
                {
                    xEncodeInterPU(img, currMB, cs_aec, bst, img->lambda, mode, &min_rdcost, isBdirect, uhBitSize);
                }
            }
        }
        xEncodeIntraCU(img, currMB, cs_aec, bst, I_MB, &min_rdcost, uhBitSize);
    }

    if ( img->current_mb_nr == 0 )
    {
        intras=0;
    }
    if ( isPFrm && IS_INTRA( currMB ) )
    {
        intras++;
    }

    if ( input->usefme )
    {
        reset_mincost( img, bst->best_mode );
    }
    return min_rdcost;
}

