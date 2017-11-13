#include "contributors.h"

#include <math.h>
#include <assert.h>
#include <memory.h>

#include "macroblock.h"
#include "mv-search.h"
#include "refbuf.h"
#include "vlc.h"
#include "block.h"
#include "header.h"
#include "golomb.h"
#include "AEC.h"
#include "../../common/common.h"
#include "../../common/pixel.h"
#include "../../common/intraPrediction.h"
#include "../../common/interPrediction.h"


extern int check_mv_range( int mv_x, int mv_y, int pix_x, int pix_y, int mode );

extern const int NCBP[64][2];
const int ref_idx[32] = {0, 1, -1, 2, -1, -1, -1, 3, -1, -1, -1, 4, -1, -1, -1, 5, -1, -1, -1, 6, -1, -1, -1, 7, -1, -1, -1, 8, -1, -1, -1, 9};
extern int* mvbits;

/**************************************************************************
* Function:Update the coordinates for the next macroblock to be processed
* Input:mb: MB address in scan order
* Output:
* Return:
* Attention:
**************************************************************************/
void init_mb_params ( Macroblock *currMB, uchar_t uhBitSize, int mb_nr )
{
    int i, j;
    currMB->uhBitSizeMB = uhBitSize;

    currMB->mb_available_left = NULL;
    currMB->mb_available_rightup = NULL;
    currMB->mb_available_up = NULL;
    currMB->mb_available_leftup = NULL;

    CheckAvailabilityOfNeighbors( currMB, mb_nr );

    currMB->mb_type   = PSKIP;
    currMB->cbp       = 0;
    currMB->c_ipred_mode = DC_PRED_8;

    for ( i=0; i < 16; i++ )
    {
        currMB->intra_pred_modes[i] = DC_PRED;
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

    img->current_slice_nr = currMB->slice_nr;
}

void init_img_pos( ImgParams *img, int mb_nr )
{
    img->current_mb_nr = mb_nr;
    img->mb_x = mb_nr % img->PicWidthInMbs;
    img->mb_y = mb_nr / img->PicWidthInMbs;

    img->mb_b8_y  = img->mb_y * BLOCK_SIZE >> 1;
    img->mb_b8_x  = img->mb_x * BLOCK_SIZE >> 1;

    img->mb_b4_y  = img->mb_y * BLOCK_SIZE;
    img->mb_b4_x  = img->mb_x * BLOCK_SIZE;

    img->mb_pix_y = img->mb_y * MB_SIZE;
    img->mb_pix_x = img->mb_x * MB_SIZE;

    img->mb_pix_y_cr  = img->mb_y * MB_SIZE >> 1;
    img->mb_pix_x_cr  = img->mb_x * MB_SIZE >> 1;

    img->cu_pix_x = img->mb_x * MB_SIZE;
    img->cu_pix_y = img->mb_y * MB_SIZE;
}

void init_coding_state( ImgParams *img, CSobj *cs_aec, int mb_nr )
{
    // Initialize bit counters for this macroblock
    if( ( mb_nr == 0 ) || ( img->current_slice_nr == img->mb_data[mb_nr-1].slice_nr ) )
    {
        cs_aec->bitcounter[BITS_HEADER] = 0;
    }

    cs_aec->bitcounter[BITS_MB_MODE] = 0;
    cs_aec->bitcounter[BITS_COEFF_Y_MB] = 0;
    cs_aec->bitcounter[BITS_REF_IDX] = 0;
    cs_aec->bitcounter[BITS_INTER_MB] = 0;
    cs_aec->bitcounter[BITS_INTRA_INFO] = 0;
    cs_aec->bitcounter[BITS_CBP_MB] = 0;
    cs_aec->bitcounter[BITS_COEFF_UV_MB] = 0;
}

void init_pu_pos ( int mode, int iCuBitSize, int iBlkIdx )
{
    if( mode == PSKIP || mode == P2Nx2N )
    {
        img->pu_pix_x = img->cu_pix_x;
        img->pu_pix_y = img->cu_pix_y;
        img->pu_width = 1 << iCuBitSize;
        img->pu_height = 1 << iCuBitSize;
    }
    else if( mode == P2NxN )
    {
        img->pu_pix_x = img->cu_pix_x;
        img->pu_pix_y = img->cu_pix_y + iBlkIdx * ( 1 << ( iCuBitSize - 1 ) );
        img->pu_width = 1 << iCuBitSize;
        img->pu_height = 1 << ( iCuBitSize - 1 );
    }
    else if( mode == PNx2N )
    {
        img->pu_pix_x = img->cu_pix_x + iBlkIdx * ( 1 << ( iCuBitSize - 1 ) );
        img->pu_pix_y = img->cu_pix_y;
        img->pu_width = 1 << ( iCuBitSize - 1 );
        img->pu_height = 1 << iCuBitSize;
    }
}

/**************************************************************************
* Function:Update the coordinates and statistics parameter for the
next macroblock
* Input:
* Output:
* Return:
* Attention:
**************************************************************************/
void update_statistics( Macroblock *currMB, CSobj *cs_aec )
{
    int* bitCount = cs_aec->bitcounter;
    // Update the statistics
    stat->bit_use_mb_type[img->type]      += bitCount[BITS_MB_MODE];
    stat->bit_use_coeffY[img->type]       += bitCount[BITS_COEFF_Y_MB] ;
    stat->tmp_bit_use_cbp[img->type]      += bitCount[BITS_CBP_MB];
    stat->bit_use_coeffC[img->type]       += bitCount[BITS_COEFF_UV_MB];

    if ( img->type==I_IMG )
    {
        ++stat->mode_use_intra[currMB->mb_type];
    }
    else
    {
        if ( img->type != B_IMG )
        {
            ++stat->mode_use_inter[0][currMB->mb_type];
            stat->bit_use_mode_inter[0][currMB->mb_type]+= bitCount[BITS_INTER_MB];

        }
        else
        {
            stat->bit_use_mode_inter[1][currMB->mb_type]+= bitCount[BITS_INTER_MB];
            ++stat->mode_use_inter[1][currMB->mb_type];
        }
    }
    // Statistics
    if ( img->type == P_IMG )
    {
        ++stat->quant0;
        stat->quant1 += img->frame_qp;      // to find average quant for inter frames
    }
}

/**************************************************************************
* Function:Terminate processing of the current macroblock depending
on the chosen slice mode
* Input:
* Output:
* Return:
* Attention:
**************************************************************************/
void terminate_pic( ImgParams *img, CSobj *cs_aec, Boolean *end_of_picture )
{
    int rlc_bits=0;
    static int skip = FALSE;
    int mb_width = img->PicWidthInMbs;
    int slice_mb = input->slice_row_nr*mb_width;

    img->coded_mb_nr++;
    if( input->slice_row_nr && ( img->coded_mb_nr != img->total_number_mb ) )
    {
        if( img->coded_mb_nr%slice_mb == 0 )
        {
            fprintf(stdout, "encode [SLICE %d]\n", img->mb_data[img->current_mb_nr].slice_nr);
            if (img->total_number_mb - img->coded_mb_nr <= slice_mb)
            {
                fprintf(stdout, "encode [SLICE %d]\n", img->mb_data[img->current_mb_nr].slice_nr + 1);
            }
            img->mb_data[img->current_mb_nr+1].slice_nr = img->mb_data[img->current_mb_nr].slice_nr+1;
            img->currSliceLastMBNr =  MIN( img->currSliceLastMBNr + slice_mb, img->total_number_mb-1 ) ;
        }
        else
        {
            img->mb_data[img->current_mb_nr+1].slice_nr = img->mb_data[img->current_mb_nr].slice_nr;
        }
    }

    if ( img->coded_mb_nr == img->total_number_mb ) // maximum number of MBs reached
    {
        *end_of_picture = TRUE;
        img->coded_mb_nr= 0;
    }

    if( *end_of_picture == TRUE && img->cod_counter )
    {
        rlc_bits = writeRunLengthInfo2Buffer_AEC( cs_aec, img->cod_counter );
        cs_aec->bitcounter[BITS_MB_MODE]+=rlc_bits;
        img->cod_counter = 0;
    }
}

/**************************************************************************
* Function: Checks the availability of neighboring macroblocks of
the current macroblock for prediction and context determination;
marks the unavailable MBs for intra prediction in the
past of the current MB are checked.
* Input:
* Output:
* Return:
* Attention:
**************************************************************************/
void CheckAvailabilityOfNeighbors( Macroblock *currMB, int mb_nr )
{
    const int mb_width = img->PicWidthInMbs;
    int   pix_x = img->mb_pix_x;
    int   pix_y = img->mb_pix_y;

    // Check MB to the left
    if( pix_x >= MB_SIZE )
    {
        if ( currMB->slice_nr == img->mb_data[mb_nr-1].slice_nr )
        {
            currMB->mb_available_left = &( img->mb_data[mb_nr-1] );
        }
    }

    // Check MB above
    if( pix_y >= MB_SIZE )
    {
        if ( currMB->slice_nr == img->mb_data[mb_nr-mb_width].slice_nr )
        {
            currMB->mb_available_up = &( img->mb_data[mb_nr-mb_width] );
        }
    }

    // Check MB left above
    if( pix_x >= MB_SIZE && pix_y >= MB_SIZE )
    {
        if ( currMB->slice_nr == img->mb_data[mb_nr-mb_width-1].slice_nr )
        {
            currMB->mb_available_leftup = &( img->mb_data[mb_nr-mb_width-1] );
        }
    }

    // Check MB right above
    if( pix_y >= MB_SIZE && img->mb_pix_x < ( img->width-MB_SIZE ) )
    {
        if( currMB->slice_nr == img->mb_data[mb_nr-mb_width+1].slice_nr )
        {
            currMB->mb_available_rightup = &( img->mb_data[mb_nr-mb_width+1] );
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


void get_luma_blk( pel_t *pDst, int bsize, int x_pos, int y_pos, pel_t *ref_pic )
{
    int x, y;
    for( y = 0; y < bsize; y++ )
    {
        for( x = 0; x < bsize; x++ )
        {
            *pDst++ = UMVPelY_14( ref_pic, y_pos + 4 * y, x_pos + 4 * x );
        }
    }
}

/**************************************************************************
* Function:Predict one 8x8 Luma block
* Input:
* Output:
* Return:
* Attention:
**************************************************************************/
void LumaPrediction8x8( ImgParams *img, int blk_x, int blk_y, int fw_ref, int bw_ref, int b8pdir, int b8mode, int bsize )
{
    pel_t fw_pred[LCU_NUM];
    pel_t bw_pred[LCU_NUM];
    pel_t *predblk = img->pred_blk_luma + blk_y * MB_SIZE + blk_x;
    int  pic_pix_x = img->mb_pix_x + blk_x;
    int  pic_pix_y = img->mb_pix_y + blk_y;
    int  b8_x      = blk_x >> 3;
    int  b8_y      = blk_y >> 3;
    int direct     = ( b8mode == PSKIP && ( img->type == B_IMG ) );
    int *****fmv_array = ( ( b8mode != PSKIP && b8pdir == BSYM ) ||( b8pdir == MHP ) ) ? img->mv_sym_mhp:img->fmv_com;
    int *****bmv_array = img->bmv_com;
    int *****bmv_mhp = img->bmv_mhp;

    if ( direct )
    {
        fw_ref= 0;
        bw_ref= 0;
    }

    if ( b8pdir != BSYM )
    {
        if ( b8pdir == FORWARD || b8pdir == BACKWORD || b8pdir == MHP || b8mode == PSKIP )
        {
            if( b8pdir == BACKWORD )
            {
                int *mv = bmv_array[0][b8mode][b8_y][b8_x];
                int y_pos = ( pic_pix_y << 2 ) + mv[1];
                int x_pos = ( pic_pix_x << 2 ) + mv[0];
                mv_out_of_range *= check_mv_range( mv[0], mv[1], pic_pix_x, pic_pix_y, 4 );
                get_luma_blk( fw_pred, bsize, x_pos, y_pos, mref[0] );
            }
            else
            {
                pel_t *ref_pic = img->type == B_IMG ? mref[fw_ref + 1] : mref [fw_ref];
                int *mv = fmv_array[fw_ref][b8mode][b8_y][b8_x];
                int y_pos = ( pic_pix_y << 2 ) + mv[1];
                int x_pos = ( pic_pix_x << 2 ) + mv[0];
                mv_out_of_range *= check_mv_range( mv[0], mv[1], pic_pix_x, pic_pix_y, 4 );
                get_luma_blk( fw_pred, bsize, x_pos, y_pos, ref_pic );
            }
        }
        if ( b8pdir == MHP )
        {
            int *mv = bmv_mhp[fw_ref][b8mode][b8_y][b8_x];
            int y_pos = ( pic_pix_y << 2 ) + mv[1];
            int x_pos = ( pic_pix_x << 2 ) + mv[0];
            mv_out_of_range *= check_mv_range( mv[0], mv[1], pic_pix_x, pic_pix_y, 4 );
            get_luma_blk( bw_pred, bsize, x_pos, y_pos, mref[fw_ref] );
            avg_pel( predblk, MB_SIZE, fw_pred, bsize, bw_pred, bsize, bsize, bsize );
        }
        else
        {
            g_funs_handle.ipcpy( fw_pred, bsize, predblk, MB_SIZE, bsize, bsize );
        }
    }
    else
    {
        int ref = ( direct ? 0 : fw_ref );
        int *mv = fmv_array[fw_ref][b8mode][b8_y][b8_x];
        int y_pos = ( pic_pix_y << 2 ) + mv[1];
        int x_pos = ( pic_pix_x << 2 ) + mv[0];
        mv_out_of_range *= check_mv_range( mv[0], mv[1], pic_pix_x, pic_pix_y, 4 );
        get_luma_blk( fw_pred, bsize, x_pos, y_pos, mref[ref + 1] );

        if( b8mode != PSKIP ) // BSYM
        {
            int x_pos, y_pos;
            int mv[2];
            mv[0] = GenSymBackMV(fmv_array[fw_ref][b8mode][b8_y][b8_x][0]);
            mv[1] = GenSymBackMV(fmv_array[fw_ref][b8mode][b8_y][b8_x][1]);

            y_pos = ( pic_pix_y << 2 ) + mv[1];
            x_pos = ( pic_pix_x << 2 ) + mv[0];
            mv_out_of_range *= check_mv_range( mv[0], mv[1], pic_pix_x, pic_pix_y, 4 );
            get_luma_blk( bw_pred, bsize, x_pos, y_pos, mref[-bw_ref] );
        }
        else // B direct
        {
            int *mv = bmv_array[bw_ref][b8mode][b8_y][b8_x];
            int y_pos = ( pic_pix_y << 2 ) + mv[1];
            int x_pos = ( pic_pix_x << 2 ) + mv[0];
            mv_out_of_range *= check_mv_range( mv[0], mv[1], pic_pix_x, pic_pix_y, 4 );
            get_luma_blk( bw_pred, bsize, x_pos, y_pos, mref[-bw_ref] );
        }
        avg_pel( predblk, MB_SIZE, fw_pred, bsize, bw_pred, bsize, bsize, bsize );
    }
}


/**************************************************************************
* Function:Residual Coding of an 8x8 Luma block (not for intra)
* Input:
* Output:
* Return:
* Attention:
**************************************************************************/
int LumaResidualCoding8x8_Bfrm( Macroblock *currMB, CSobj *cs_aec, int *cbp, int b8 )
{
    int pic_pix_y, pic_pix_x, x, y;
    int coeff_cost = 0;
    int mb_y  = ( b8 / 2 ) << 3;
    int mb_x  = ( b8 % 2 ) << 3;
    int pix_x = img->mb_pix_x;
    int pix_y = img->mb_pix_y;
    int b8mode  = currMB->b8mode[b8];
    int b8pdir  = currMB->b8pdir[b8];
    int direct  = ( b8mode == PSKIP && ( img->type == B_IMG ) );
    int coef_blk[MB_SIZE * MB_SIZE]; // IVC 8x8 pred.error buffer
    int resi_blk[MB_SIZE * MB_SIZE]; // local residual block (16x16) for T/Q

    pic_pix_y = pix_y + mb_y;
    pic_pix_x = pix_x + mb_x;

    // get displaced frame difference
    for ( y=0; y<8; y++ )
    {
        for ( x=0; x<8; x++ )
        {
            img->resi_blk[x][y] = resi_blk[y*8+x] = imgY_org[( pic_pix_y + y ) * ( img->width ) + pic_pix_x + x] - img->pred_blk_luma[( y + mb_y ) * MB_SIZE + x + mb_x];
        }
    }

    if( !direct )
    {
        transform( resi_blk, coef_blk, 8 );
        coeff_cost = scanquant( img, cs_aec, 0, b8, 0, coef_blk, resi_blk, cbp, 8 );
        recon_luma_blk(img, b8, 0, coef_blk, 8);
    }

    if ( coeff_cost <= _LUMA_COEFF_COST_ )
    {
        coeff_cost  = 0;
        ( *cbp ) &= 1<<b8;

        for ( y=mb_y; y<mb_y+8; y++ )
        {
            for ( x=mb_x; x<mb_x+8; x++ )
            {
                imgY_rec[( img->mb_pix_y + y )*( img->iStride ) + img->mb_pix_x + x] = img->pred_blk_luma[y * MB_SIZE + x];
            }
        }
    }

    return coeff_cost;
}



/**************************************************************************
* Function:Set mode parameters and reference frames for an 8x8 block
* Input:
* Output:
* Return:
* Attention:
**************************************************************************/
void SetModesAndRefframe( Macroblock *currMB, int b8, int* fw_ref, int* bw_ref )
{
    int   blk_y = ( b8/2 );
    int   blk_x = ( b8%2 );
    int** frefarr = img->pfrm_ref;   // For MB level field/frame coding
    int** fw_refarr = img->bfrm_fref;  // For MB level field/frame coding
    int** bw_refarr = img->bfrm_bref;  // For MB level field/frame coding
    int   mb_b8_x = img->mb_b8_x;
    int   mb_b8_y = img->mb_b8_y; // For MB level field/frame coding

    if ( img->type != B_IMG )
    {
        *fw_ref  = frefarr[mb_b8_y + blk_y][mb_b8_x + blk_x];
        *bw_ref  = 0;
    }
    else
    {
        if ( currMB->b8pdir[b8]==-1 )
        {
            *fw_ref  = -1;
            *bw_ref  = -1;
        }
        else if ( currMB->b8pdir[b8]==FORWARD )
        {
            *fw_ref  = fw_refarr[mb_b8_y + blk_y][mb_b8_x + blk_x];
            *bw_ref  = 0;
        }
        else if ( currMB->b8pdir[b8]==BACKWORD )
        {
            *fw_ref  = 0;
            *bw_ref  = bw_refarr[mb_b8_y + blk_y][mb_b8_x + blk_x];
        }
        else
        {
            *fw_ref  = fw_refarr[mb_b8_y + blk_y][mb_b8_x + blk_x];
            *bw_ref  = bw_refarr[mb_b8_y + blk_y][mb_b8_x + blk_x];

            if ( currMB->b8mode[b8]==PSKIP ) // direct
            {
                if ( img->type==B_IMG )
                {
                    *fw_ref = 0;
                    *bw_ref = 0;
                }
                else
                {
                    *fw_ref = MAX( 0,frefarr[mb_b8_y + blk_y][mb_b8_x + blk_x] );
                    *bw_ref = 0;
                }
            }
        }
    }
}

void LumaResidualCoding ( Macroblock *currMB, CSobj *cs_aec, uchar_t uhBitSize )
{
    int x, y, b8;

    int sum_cnt_nonz;
    int coef_blk[MB_SIZE*MB_SIZE]; // IVC 8x8 pred.error buffer
    int resi_blk[MB_SIZE*MB_SIZE]; // local residual block (16x16) for T/Q
    int bsize = 1 << uhBitSize;
    int pu_mb_off_y = 0;
    int pu_mb_off_x = 0;
    int pix_y_in_cu = MB_SIZE * pu_mb_off_y;
    int pix_x_in_cu = MB_SIZE * pu_mb_off_x;
    assert( uhBitSize == 4 );
    currMB->cbp     = 0 ;
    sum_cnt_nonz    = 0 ;

    if (currMB->trans_type == TRANS_NxN)
    {
        for( b8 = 0; b8 < 4; b8++ )
        {
            int pix_y_in_mb = (b8 / 2) << 3;
            int pix_x_in_mb = (b8 % 2) << 3;
            int pic_pix_y = img->mb_pix_y + pix_y_in_mb;
            int pic_pix_x = img->mb_pix_x + pix_x_in_mb;
            int coeff_cost = 0;

            for ( y=0; y<8; y++ )
            {
                for ( x=0; x<8; x++ )
                {
                    img->resi_blk[x][y] = resi_blk[y*8+x] = imgY_org[( pic_pix_y + y )*( img->width ) + pic_pix_x + x] - img->pred_blk_luma[( y + pix_y_in_mb ) * MB_SIZE + x + pix_x_in_mb];
                }
            }

            if( currMB->b8mode[b8] != PSKIP )
            {
                transform( resi_blk, coef_blk, 8 );
                coeff_cost = scanquant( img, cs_aec, 0, b8, 0, coef_blk, resi_blk, &currMB->cbp, 8 );
                recon_luma_blk(img, b8, 0, coef_blk, 8);
            }

            if ( coeff_cost <= _LUMA_COEFF_COST_ )
            {
                coeff_cost = 0;
                currMB->cbp &= 0xFFFFFFFF ^ ( 0xF << ( b8*4 ) ); // regard this NxN block as all zero block

                for ( x=pix_x_in_mb; x<pix_x_in_mb+8; x++ )
                {
                    for ( y=pix_y_in_mb; y<pix_y_in_mb+8; y++ )
                    {
                        imgY_rec[( img->mb_pix_y + y )*( img->iStride ) + img->mb_pix_x + x] = img->pred_blk_luma[y * MB_SIZE + x];
                    }
                }
            }
            sum_cnt_nonz += coeff_cost;
        }
    }

    if ( currMB->trans_type == TRANS_2Nx2N )
    {
        bsize = 1 << uhBitSize;
        for ( y = 0; y < bsize; y++ )
        {
            for ( x = 0; x < bsize; x++ )
            {
                img->resi_blk[x][y] = resi_blk[y*MB_SIZE+x] = imgY_org[( img->mb_pix_y + y )*( img->width ) + img->mb_pix_x + x] - img->pred_blk_luma[y * MB_SIZE + x];
            }
        }

        transform( resi_blk, coef_blk, bsize );
        sum_cnt_nonz = scanquant( img, cs_aec, 0, 0, 0, coef_blk, resi_blk, &currMB->cbp, bsize );
        recon_luma_blk(img, 0, 0, coef_blk, bsize);

        if ( currMB->cbp & 0xffff )
        {
            currMB->cbp |= 0xffff;
        }
    }

    if ( sum_cnt_nonz <= 5 )
    {
        currMB->cbp &= 0xffff0000 ;
        for ( x=0; x < MB_SIZE; x++ )
        {
            for ( y=0; y < MB_SIZE; y++ )
            {
                imgY_rec[( img->mb_pix_y + y )*( img->iStride ) + img->mb_pix_x + x] = img->pred_blk_luma[y * MB_SIZE + x];
            }
        }
    }
}

/**************************************************************************
* Function:Predict one chroma 4x4 block
* Input:
* Output:
* Return:
* Attention:
**************************************************************************/
void ChromaPrediction4x4 ( ImgParams *img, int bsize, int uv, int blk_x, int blk_y, int fw_ref, int bw_ref, int b8pdir, int b8mode )
{
    pel_t fw_pred[16];
    pel_t bw_pred[16];
    pel_t *predblk = img->pred_blk_chroma[uv] + blk_y * MB_SIZE + blk_x;
    int pic_pix_x = img->mb_pix_x_cr + blk_x;
    int pic_pix_y = img->mb_pix_y_cr + blk_y;
    int direct    = ( b8mode == PSKIP && img->type == B_IMG );
    int *****fmv_array = ( ( b8mode != PSKIP && b8pdir == BSYM ) ||( b8pdir == MHP ) ) ? img->mv_sym_mhp : img->fmv_com;
    int *****bmv_array = img->bmv_com;
    int *****bmv_mhp = img->bmv_mhp;

    if ( direct )
    {
        fw_ref = 0;
        bw_ref = 0;
    }

    // inter prediction
    if ( b8pdir != BSYM )
    {
        if ( b8pdir == FORWARD || b8pdir == BACKWORD || b8pdir == MHP )
        {
            if( b8pdir == BACKWORD )
            {
                int *mvb = bmv_array[0][b8mode][blk_y >> 2][blk_x >> 2];
                int posx = ( mvb[0] & 7 );
                int posy = ( mvb[1] & 7 );
                int mv_x = pic_pix_x + ( mvb[0] >> 3 );
                int mv_y = pic_pix_y + ( mvb[1] >> 3 );
                get_chroma_block_enc( fw_pred, 4, bsize, mv_x, mv_y, posx, posy, mcef[0][uv] );
            }
            else
            {
                int *mvb = fmv_array[fw_ref][b8mode][blk_y >> 2][blk_x >> 2];
                int posx = ( mvb[0] & 7 );
                int posy = ( mvb[1] & 7 );
                int mv_x = pic_pix_x + ( mvb[0] >> 3 );
                int mv_y = pic_pix_y + ( mvb[1] >> 3 );
                pel_t *refpic = ( img->type == B_IMG ) ? mcef[fw_ref+1][uv] : mcef[fw_ref][uv];
                get_chroma_block_enc( fw_pred, bsize, bsize, mv_x, mv_y, posx, posy, refpic );
            }
        }
        if( b8pdir == MHP )
        {
            int *mvb = bmv_mhp[fw_ref][b8mode][blk_y >> 2][blk_x >> 2];
            int posx = ( mvb[0] & 7 );
            int posy = ( mvb[1] & 7 );
            int mv_x_bw = pic_pix_x + ( mvb[0] >> 3 );
            int mv_y_bw = pic_pix_y + ( mvb[1] >> 3 );
            get_chroma_block_enc( bw_pred, bsize, bsize, mv_x_bw, mv_y_bw, posx, posy, mcef[fw_ref][uv] );
            avg_pel( predblk, MB_SIZE, fw_pred, bsize, bw_pred, bsize, bsize, bsize );
        }
        else
        {
            g_funs_handle.ipcpy( fw_pred, bsize, predblk, MB_SIZE, bsize, bsize );
        }
    }
    else
    {
        int ref = ( direct ? 0 : fw_ref );
        int *mvb = fmv_array[ref][b8mode][blk_y >> 2][blk_x >> 2];
        int posx = ( mvb[0] & 7 );
        int posy = ( mvb[1] & 7 );
        int mv_x = pic_pix_x + ( mvb[0] >> 3 );
        int mv_y = pic_pix_y + ( mvb[1] >> 3 );
        get_chroma_block_enc( fw_pred, bsize, bsize, mv_x, mv_y, posx, posy, mcef[ref+1][uv] );

        if( b8mode != PSKIP && b8pdir == BSYM ) // BSYM
        {
            int *mvb = fmv_array[fw_ref][b8mode][( pic_pix_y - img->mb_pix_y_cr ) >> 2][( pic_pix_x - img->mb_pix_x_cr ) >> 2];
            int mv_x = GenSymBackMV(mvb[0]);
            int mv_y = GenSymBackMV(mvb[1]);

            int posx = ( mv_x & 7 );
            int posy = ( mv_y & 7 );
            mv_x = pic_pix_x + ( mv_x >> 3 );
            mv_y = pic_pix_y + ( mv_y >> 3 );
            get_chroma_block_enc( bw_pred, 4, bsize, mv_x, mv_y, posx, posy, mcef[0][uv] );
        }
        else
        {
            int *mvb = bmv_array[0][b8mode][blk_y >> 2][blk_x >> 2];
            int posx = ( mvb[0] & 7 );
            int posy = ( mvb[1] & 7 );
            int mv_x = pic_pix_x + ( mvb[0] >> 3 );
            int mv_y = pic_pix_y + ( mvb[1] >> 3 );
            get_chroma_block_enc( bw_pred, 4, bsize, mv_x, mv_y, posx, posy, mcef[0][uv] );
        }
        avg_pel( predblk, MB_SIZE, fw_pred, bsize, bw_pred, bsize, bsize, bsize );
    }
}

void EncodeInterChromaCU( Macroblock *currMB, CSobj *cs_aec, int isBdirect )
{
    int uv, b8_idx, block_y, block_x;
    int fw_ref, bw_ref;
    int skipped = ( currMB->mb_type == PSKIP && img->type == P_IMG );
    int tq_blk[MB_SIZE * MB_SIZE];
    int resi_blk[MB_SIZE * MB_SIZE];
    int isIntra = IS_INTRA ( currMB );

    int bsize = 4;
    int cr_cbp = 0;
    int i, j;

    int curr_val = 0;
    int pu_mb_off_y = 0;
    int pu_mb_off_x = 0;
    int pix_y_in_cu = MB_SIZE * pu_mb_off_y;
    int pix_x_in_cu = MB_SIZE * pu_mb_off_x;

    for( uv = 0; uv < 2; uv++ )
    {
        for( b8_idx = 0; b8_idx < 4; b8_idx++ )
        {
            block_y = ( b8_idx/2 ) << 2;
            block_x = ( b8_idx%2 ) << 2;
            SetModesAndRefframe( currMB, b8_idx, &fw_ref, &bw_ref );
            ChromaPrediction4x4( img, bsize, uv, block_x, block_y, fw_ref, bw_ref, currMB->b8pdir[b8_idx], currMB->b8mode[b8_idx] );
        }

        for( b8_idx = 0; b8_idx < 4; b8_idx++ )
        {
            block_y = ( b8_idx/2 ) << 2;
            block_x = ( b8_idx%2 ) << 2;

            if ( skipped || isBdirect )
            {
                for ( j = 0; j < bsize; j++ )
                {
                    for ( i = block_x; i < block_x + bsize; i++ )
                    {
                        if ( uv == 0 )
                        {
                            imgU_rec[( img->mb_pix_y_cr + block_y + j )*( img->iStrideC ) + img->mb_pix_x_cr + i] = img->pred_blk_chroma[uv][( block_y + j ) * MB_SIZE + i];
                        }
                        else
                        {
                            imgV_rec[( img->mb_pix_y_cr + block_y + j )*( img->iStrideC ) + img->mb_pix_x_cr + i] = img->pred_blk_chroma[uv][( block_y + j ) * MB_SIZE + i];
                        }
                    }
                }
            }
            else
            {
                for ( j = 0; j < bsize; j++ )
                {
                    for ( i = block_x; i < block_x + bsize; i++ )
                    {
                        if ( uv == 0 )
                        {
                            resi_blk[( block_y + j )*8 + i] = imgU_org[( img->mb_pix_y_cr + block_y + j )*( img->width_cr ) + img->mb_pix_x_cr + i] - img->pred_blk_chroma[uv][( block_y + j ) * MB_SIZE + i];
                        }
                        else
                        {
                            resi_blk[( block_y + j )*8 + i] = imgV_org[( img->mb_pix_y_cr + block_y + j )*( img->width_cr ) + img->mb_pix_x_cr + i] - img->pred_blk_chroma[uv][( block_y + j ) * MB_SIZE + i];
                        }
                    }
                }
            }
        }

        if ( !skipped && !isBdirect )
        {
            transform( resi_blk, tq_blk, 8 );
            scanquant( img, cs_aec, isIntra, 4 + uv, 0, tq_blk, resi_blk, &cr_cbp, 8 );
            recon_chroma_blk(img, uv, 0, tq_blk, img->pred_blk_chroma[uv], 8);
        }
    }

    currMB->cbp += cr_cbp;//((cr_cbp)<<4);
}

/**************************************************************************
* Function:Converts 8x8 block type to coding value
* Input:
* Output:
* Return:
* Attention:
**************************************************************************/
int B8Mode2Value ( int b8mode, int b8pdir )
{
    static const int b8start[8] = {0,0,0,0, 1, 4, 5, 10};
    static const int b8inc  [8] = {0,0,0,0, 1, 2, 2, 1};

    if ( img->type != B_IMG )
    {
        if( b8pdir == FORWARD )
        {
            return 0;
        }
        else if( b8pdir == MHP )
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return b8start[b8mode] + b8inc[b8mode] * b8pdir;
    }
}

int xCodePredInfo(ImgParams *img, Macroblock *currMB, CSobj *cs_aec)
{
    int i;
    int no_bits = 0;
    if (img->type != P_IMG && currMB->mb_type != PSKIP) // B-frame
    {
        const int iPdirMap[3] = { 2, 0, 3 }; // FORWORD, BACKWORD, SYM
        if (currMB->mb_type == P2Nx2N)        // 1 : B_16x16
        {
            int bit_len = write_Bfrm_PredTypeInfo_AEC(cs_aec, iPdirMap[currMB->b8pdir[0]]);
#if TRACE
            output_trace_info("MB pdir = %3d", currMB->b8pdir[0]);
#endif
            no_bits += bit_len;
        }
        else if (currMB->mb_type == P2NxN || currMB->mb_type == PNx2N)    // 2 : B_16x8, 3 : B_8x16
        {
            int PredMode[2] = { -1, -1 };
            PredMode[0] = iPdirMap[currMB->b8pdir[0]];
            PredMode[1] = iPdirMap[currMB->b8pdir[3]];
#if TRACE
            output_trace_info("MB pdir = %3d", currMB->b8pdir[0]);
            output_trace_info("MB pdir = %3d", currMB->b8pdir[3]);
#endif
            for (i = 0; i < 2; i++)
            {
                int bit_len = write_Bfrm_PredTypeInfo_AEC(cs_aec, PredMode[i]);
                no_bits += bit_len;
            }
        }
        else if (currMB->mb_type == PNxN)     // B_8x8
        {
            for (i = 0; i < 4; i++)
            {
                int val = B8Mode2Value(currMB->b8mode[i], currMB->b8pdir[i]);
                int bit_len = write_B8x8_PredTypeInfo_AEC(cs_aec, val);
#if TRACE
                output_trace_info("8x8 MB type = %3d\n", currMB->b8mode[i]);
#endif
                no_bits += bit_len;
            }
        }
    }
    else if (img->type == P_IMG)
    {
        const int skip = 0, fwd = 2, mh = 3;
        if (currMB->mb_type == PSKIP || currMB->mb_type == P2Nx2N)        // 0 : P_Skip, 1 : P_16x16
        {
            int mode, bit_len;
            if (currMB->mb_type == PSKIP)
            {
                mode = 0;
            }
            else
            {
                if (currMB->b8pdir[0] == FORWARD) // FORWARD
                {
                    mode = fwd;
                }
                else if (currMB->b8pdir[0] == MHP)   // MHP
                {
                    mode = mh;
                }
                else
                {
                    printf("Syntax Error! P_16x16_Pred_Type : %d\n", currMB->b8pdir[0]);
                    exit(0);
                }
            }
            bit_len = write_P16x16_PredTypeInfo_AEC(cs_aec, mode);
#if TRACE
            output_trace_info("8x8 MB pdir = %3d\n", currMB->b8pdir[0]);
#endif
            no_bits += bit_len;
        }
        else if (currMB->mb_type == P2NxN || currMB->mb_type == PNx2N)        // 2 : P_16x8, 3 : P_8x16
        {
            int PredMode[2] = { -1, -1 };

            if (currMB->b8pdir[0] == FORWARD) // FORWARD
            {
                PredMode[0] = fwd;
            }
            else if (currMB->b8pdir[0] == MHP)   // MHP
            {
                PredMode[0] = mh;
            }
            else
            {
                printf("Syntax Error! P_16x16_Pred_Type : %d\n", currMB->b8pdir[0]);
                exit(0);
            }

            if (currMB->b8pdir[3] == FORWARD) // FORWARD
            {
                PredMode[1] = fwd;
            }
            else if (currMB->b8pdir[3] == MHP)   // MHP
            {
                PredMode[1] = mh;
            }
            else
            {
                printf("Syntax Error! P_16x16_Pred_Type : %d\n", currMB->b8pdir[3]);
                exit(0);
            }

            for (i = 0; i < 2; i++)
            {
                int bit_len;
                bit_len = write_Pfrm_PredTypeInfo_AEC(cs_aec, PredMode[i]);
                no_bits += bit_len;
            }
#if TRACE
            output_trace_info("MB pdir = %3d", currMB->b8pdir[0]);
            output_trace_info("MB pdir = %3d", currMB->b8pdir[3]);
#endif
        }
        else if (currMB->mb_type == PNxN)         // P_8x8
        {
            for (i = 0; i < 4; i++)
            {
                int bit_len, mode;
                mode = B8Mode2Value(currMB->b8mode[i], currMB->b8pdir[i]);
                bit_len = write_Pfrm_PredTypeInfo_AEC(cs_aec, mode);
#if TRACE
                output_trace_info("8x8 MB type = %3d\n", currMB->b8mode[i]);
                output_trace_info("8x8 MB pdir = %3d", currMB->b8pdir[i]);
#endif
                no_bits += bit_len;
            }
        }
    }
    return no_bits;
}

int xCodeRefIdx(ImgParams *img, Macroblock *currMB, CSobj *cs_aec)
{
    int i;
    int no_bits = 0;
    if (img->real_ref_num > 1 && img->ip_frm_idx > 1)
    {
        int ref, bit_len;
        if (IS_PNxN(currMB))
        {
            for (i = 0; i < 4; i++)
            {
                ref = ref_idx[currMB->fw_ref[i]];
                if (ref < 0)
                {
                    printf("Reference Frame Index Error!\n");
                    exit(0);
                }
                bit_len = write_Reffrm_AEC(cs_aec, ref);

                refbits[ref_idx[currMB->fw_ref[i]]] = bit_len;
                no_bits += bit_len;
            }
        }
        else if (currMB->mb_type == P2Nx2N)
        {
            ref = ref_idx[currMB->fw_ref[0]];
            if (ref < 0)
            {
                printf("Reference Frame Index Error!\n");
                exit(0);
            }
            bit_len = write_Reffrm_AEC(cs_aec, ref);

            refbits[ref_idx[currMB->fw_ref[0]]] = bit_len;
            no_bits += bit_len;
        }
        else if (currMB->mb_type == P2NxN || currMB->mb_type == PNx2N)
        {
            for (i = 0; i < 2; i++)
            {
                if (currMB->mb_type == P2NxN)
                {
                    ref = ref_idx[currMB->fw_ref[i * 2]];
                }
                else
                {
                    ref = ref_idx[currMB->fw_ref[i]];
                }
                if (ref < 0)
                {
                    printf("Reference Frame Index Error!\n");
                    exit(0);
                }
                bit_len = write_Reffrm_AEC(cs_aec, ref);
                if (currMB->mb_type == P2NxN)
                {
                    refbits[ref_idx[currMB->fw_ref[i * 2]]] = bit_len;
                }
                else
                {
                    refbits[ref_idx[currMB->fw_ref[i]]] = bit_len;
                }
                no_bits += bit_len;
            }
        }
    }
    return no_bits;
}

// for TRANS_2Nx2N only the prediction mode need to be written
int xCodeIntraPredInfo(ImgParams *img, Macroblock *currMB, CSobj *cs_aec)
{
    int bit_len;
    int no_bits = 0;
    if (currMB->trans_type == TRANS_2Nx2N)
    {
#if TRACE
        output_trace_info("Intra mode = %3d", currMB->intra_pred_modes[0]);
#endif
        bit_len = writeIntraPredMode_AEC(cs_aec, currMB->intra_pred_modes[0]);
        no_bits += bit_len;
    }
    else
    {
        int b8, b4;
        for (b8 = 0; b8 < 4; b8++)
        {
            int bit_len = writeSubMBTransType_AEC(cs_aec, currMB->sub_mb_trans_type[b8]);
            no_bits += bit_len;
        }
        for (b8 = 0; b8 < 4; b8++)
        {
            int cnt = currMB->sub_mb_trans_type[b8] == 0 ? 1 : 4;
            for (b4 = 0; b4 < cnt; b4++)
            {
                int bit_len;
#if TRACE
                output_trace_info("Intra mode = %3d", currMB->intra_pred_modes[b8 * 4 + b4]);
#endif
                bit_len = writeIntraPredMode_AEC(cs_aec, currMB->intra_pred_modes[b8 * 4 + b4]);
                no_bits += bit_len;
            }
        }
    }

    // write chroma prediction mode
    bit_len = writeCIPredMode_AEC(cs_aec, currMB->c_ipred_mode);
    no_bits += bit_len;
#if TRACE
    output_trace_info("Chroma intra pred mode %d", currMB->c_ipred_mode);
#endif
    return no_bits;
}

/**************************************************************************
* Function: Codes macroblock header
* Input:
* Output:
* Return:
* Attention:
**************************************************************************/
int writeCUHeader( Macroblock *currMB, CSobj *cs_aec, int mb_nr, int bWrite )
{
    int*            bitCount  = cs_aec->bitcounter;
    int written_bits = 0;
    int             no_bits   = 0;
    int             eos_bit;

    eos_bit = ( img->current_mb_nr != img->currentSlice->start_mb_nr ) ? 1 : 0;

    CheckAvailabilityOfNeighbors( currMB, mb_nr );

    if ( eos_bit && bWrite ) // Actually, terminating bit is written in the end of macroblocks.
    {
        write_terminating_bit ( cs_aec, 0 );
    }

    if( img->type != I_IMG )
    {
        written_bits = writeMBPartTypeInfo_AEC(currMB, cs_aec, currMB->mb_type);
#if TRACE
        output_trace_info( "MB type = %3d", currMB->mb_type );
#endif

        bitCount[BITS_MB_MODE] += written_bits;
        no_bits += written_bits;

        written_bits = xCodePredInfo(img, currMB, img->cs_aec);
        bitCount[BITS_INTER_MB] += written_bits;
        no_bits += written_bits;

        if (img->type == P_IMG)
        {
            written_bits = xCodeRefIdx(img, currMB, img->cs_aec);
            bitCount[BITS_REF_IDX] += written_bits;
            no_bits += written_bits;
        }
    }

    if( IS_INTRA( currMB ) )
    {
        // write transform type
        written_bits = writeMBTransType_AEC(cs_aec, currMB->trans_type);
        bitCount[BITS_MB_MODE] += written_bits;
        no_bits += written_bits;
#if TRACE
        output_trace_info( "transform type: %d", currMB->trans_type );
#endif
        written_bits = xCodeIntraPredInfo(img, currMB, img->cs_aec);
        bitCount[BITS_INTRA_INFO] += written_bits;
        no_bits += written_bits;
    }

    // write mb_qp_delta
    bool bFixedQp = (input->fixed_picture_qp || input->fixed_slice_qp);
    if (currMB->cbp > 0 && !bFixedQp)
    {
        writeMBDeltaQp_AEC(cs_aec, currMB->mb_qp_delta, img->previous_delta_qp);
        img->previous_delta_qp = currMB->mb_qp_delta;
    }

    return no_bits;
}

/**************************************************************************
* Function:Writes motion vectors of an 8x8 block
* Input:
* Output:
* Return:
* Attention:
**************************************************************************/
int writeOneMotionVector ( Macroblock *currMB, CSobj *cs_aec, int b8, int refframe, int fwd_flag, int mv_mode )
{
    int blk_x = b8 % 2;
    int blk_y = b8 / 2;
    int            k, n, m;
    int            curr_mvd[2];
    int            bwflag      = ( ( refframe<0 || ( !fwd_flag ) )?1:0 );
    int            rate        = 0;
    int            blk_step_h  = blc_size[mv_mode][0];
    int            blk_step_v  = blc_size[mv_mode][1];
    int*           bitCount    = cs_aec->bitcounter;
    int            refindex    = ( refframe<0 ? 0 : refframe );
    int*****       all_mv      = ( fwd_flag ? img->fmv_com : img->bmv_com );
    int*****       pred_mv     = ( ( img->type!=B_IMG ) ? img->pfmv_com : ( fwd_flag ? img->pfmv_com : img->pbmv_com ) );
    if ( !fwd_flag )
    {
        bwflag = 1;
    }

    curr_mvd[0] = all_mv[refindex][mv_mode][blk_y][blk_x][0] - pred_mv[refindex][mv_mode][blk_y][blk_x][0];
    curr_mvd[1] = all_mv[refindex][mv_mode][blk_y][blk_x][1] - pred_mv[refindex][mv_mode][blk_y][blk_x][1];

    //--- store (oversampled) mvd ---
    for ( n=0; n < blk_step_v; n++ )
    {
        for ( m=0; m < blk_step_h; m++ )
        {
            currMB->mvd[bwflag][blk_y+n][blk_x+m][0] = curr_mvd[0];
            currMB->mvd[bwflag][blk_y+n][blk_x+m][1] = curr_mvd[1];
        }
    }

    for( k = 0; k < 2; k++ )
    {
        int bit_len = writeMVD_AEC( currMB, cs_aec, b8, curr_mvd[k], 2*k+fwd_flag );
#if TRACE
        if ( fwd_flag )
        {
            output_trace_info( "FMVD = %3d", curr_mvd[k] );
        }
        else
        {
            output_trace_info( "BMVD = %3d", curr_mvd[k] );
        }
#endif
        bitCount[BITS_INTER_MB] += bit_len;
        rate                    += bit_len;
    }
    return rate;
}



int MHMC_writeOneMotionVector( Macroblock *currMB, CSobj *cs_aec, int b8, int refframe, int fwd_flag, int mv_mode )
{
    int blk_x = b8 % 2;
    int blk_y = b8 / 2;
    int            k, n, m;
    int            curr_mvd[2];
    int            bwflag      = ( ( refframe<0 || ( !fwd_flag ) )?1:0 );
    int            rate        = 0;
    int            blk_step_h      = blc_size[mv_mode][0];
    int            blk_step_v      = blc_size[mv_mode][1];
    int*           bitCount    = cs_aec->bitcounter;
    int            refindex    = ( refframe<0 ? 0 : refframe );
    int*****       all_mv      = img->mv_sym_mhp;
    int*****       pred_mv     = img->pmv_sym_mhp;
    if ( !fwd_flag )
    {
        bwflag = 0;
    }

    curr_mvd[0] = all_mv[refindex][mv_mode][blk_y][blk_x][0] - pred_mv[refindex][mv_mode][blk_y][blk_x][0];
    curr_mvd[1] = all_mv[refindex][mv_mode][blk_y][blk_x][1] - pred_mv[refindex][mv_mode][blk_y][blk_x][1];

    //--- store (oversampled) mvd ---
    for ( n=0; n < blk_step_v; n++ )
    {
        for ( m=0; m < blk_step_h; m++ )
        {
            currMB->mvd[bwflag][blk_y+n][blk_x+m][0] = curr_mvd[0];
            currMB->mvd[bwflag][blk_y+n][blk_x+m][1] = curr_mvd[1];
        }
    }

    for ( k=0; k<2; k++ )
    {
        int bit_len = writeMVD_AEC( currMB, cs_aec, b8, curr_mvd[k], 2*k+bwflag );
#if TRACE
        if ( fwd_flag )
        {
            output_trace_info( "FMVD = %3d", curr_mvd[k] );
        }
        else
        {
            output_trace_info( "BMVD = %3d", curr_mvd[k] );
        }
#endif
        mvbits[curr_mvd[k]]      = bit_len;
        bitCount[BITS_INTER_MB] += bit_len;
        rate                    += bit_len;
    }
    return rate;
}

/*
*************************************************************************
* Function:Writes motion vectors of an 8x8 block
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

int writeOneMotionVector_sym ( Macroblock *currMB, CSobj *cs_aec, int blk_x, int blk_y, int refframe, int fwd_flag, int mv_mode )
{
    int            k, n, m;
    int            curr_mvd[2];
    int            bwflag     = ( ( refframe<0 || ( !fwd_flag ) )?1:0 );
    int            rate       = 0;
    int            blk_step_h = blc_size[mv_mode][0];
    int            blk_step_v = blc_size[mv_mode][1];
    int*           bitCount   = cs_aec->bitcounter;
    int            refindex   = ( refframe<0 ? 0 : refframe );
    int*****       all_mv     = ( fwd_flag ? img->fmv_com : img->bmv_com );
    int*****       pred_mv    = ( ( img->type!=B_IMG ) ? img->pfmv_com : ( fwd_flag ? img->pfmv_com : img->pbmv_com ) );
    int            blk_idx = blk_y * 2 + blk_x;
    if ( !fwd_flag )
    {
        bwflag = 1;
    }

    all_mv = img->mv_sym_mhp;
    pred_mv = img->pmv_sym_mhp;

    curr_mvd[0] = all_mv[refindex][mv_mode][blk_y][blk_x][0] - pred_mv[refindex][mv_mode][blk_y][blk_x][0];
    curr_mvd[1] = all_mv[refindex][mv_mode][blk_y][blk_x][1] - pred_mv[refindex][mv_mode][blk_y][blk_x][1];

    //--- store (oversampled) mvd ---
    for ( n=0; n < blk_step_v; n++ )
    {
        for ( m=0; m < blk_step_h; m++ )
        {
            currMB->mvd[bwflag][blk_y+n][blk_x+m][0] = curr_mvd[0];
            currMB->mvd[bwflag][blk_y+n][blk_x+m][1] = curr_mvd[1];
        }
    }

    for ( k=0; k<2; k++ )
    {
        int bit_len = writeMVD_AEC( currMB, cs_aec, blk_idx,curr_mvd[k],2*k+( 1-fwd_flag ) );
#if TRACE
        if ( fwd_flag )
        {
            output_trace_info( "FMVD = %3d", curr_mvd[k] );
        }
        else
        {
            output_trace_info( "BMVD = %3d", curr_mvd[k] );
        }
#endif
        mvbits[curr_mvd[k]]      = bit_len;
        bitCount[BITS_INTER_MB] += bit_len;
        rate                    += bit_len;
    }
    return rate;
}

int writeLumaCoeff_AEC ( CSobj *cs_aec, int bsize, int *ACLevel, int *ACRun )
{
    int             rate      = 0;
    int*            bitCount  = cs_aec->bitcounter;
    int             bit_len;
    int coef_num = bsize * bsize;
    int k;
    int DCT_Level[256];
    int DCT_Run[256];
    int DCT_Pairs = 0;
    memset(DCT_Level, 0, sizeof(DCT_Level));
    memset(DCT_Run, 0, sizeof(DCT_Run));

    for ( k = 0; k < coef_num; k++ )
    {
        if (!ACLevel[k])
            break;

        DCT_Level[DCT_Pairs] = ACLevel[k];
        DCT_Run[DCT_Pairs] = ACRun[k];
        DCT_Pairs++;
#if TRACE
        output_trace_info("Luma level = %3d", ACLevel[k]);
        output_trace_info("Luma run = %3d", ACRun[k]);
#endif
    }
#if TRACE // In the end of coding the coefficients of one block, (0,0) is written to signal the coefficient coding is finished.
    output_trace_info("Luma level = %3d", 0);
    output_trace_info("Luma run = %3d", 0);
#endif

    bit_len = writeRunLevel_AEC(cs_aec, bsize, DCT_Level, DCT_Run, DCT_Pairs, LUMA_8x8);
    bitCount[BITS_COEFF_Y_MB] += bit_len;
    rate += bit_len;

    return rate;
}

int writeChromaCoeff_AEC( CSobj *cs_aec, int isIntra, int bsize, int *ACLevel, int *ACRun )
{
    int             rate      = 0;
    int             bit_len;
    int*            bitCount  = cs_aec->bitcounter;
    int             k;
    int coef_num = bsize * bsize;
    int DCT_Level[256];
    int DCT_Run[256];
    int DCT_Pairs = 0;
    memset(DCT_Level, 0, sizeof(DCT_Level));
    memset(DCT_Run, 0, sizeof(DCT_Run));

    for( k = 0; k < coef_num; k++ )
    {
        if (!ACLevel[k])
            break;

        DCT_Level[DCT_Pairs] = ACLevel[k];
        DCT_Run[DCT_Pairs] = ACRun[k];
        DCT_Pairs++;
#if TRACE
        output_trace_info("Chroma level = %3d", ACLevel[k]);
        output_trace_info("Chroma run = %3d", ACRun[k]);
#endif
    }
#if TRACE // In the end of coding the coefficients of one block, (0,0) is written to signal the coefficient coding is finished.
    output_trace_info("Chroma level = %3d", 0);
    output_trace_info("Chroma run = %3d", 0);
#endif
    bit_len = writeRunLevel_AEC(cs_aec, bsize, DCT_Level, DCT_Run, DCT_Pairs, CHROMA_AC);
    bitCount[BITS_COEFF_UV_MB] += bit_len;
    rate += bit_len;
    return rate;
}

/*
*************************************************************************
* Function:Writes CBP, DQUANT, and Luma Coefficients of an macroblock
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void StoreOneMotionVector( Macroblock *currMB, int blk_x, int blk_y, int ref_fw, int ref_bw, int b8mode, int b8pdir )
{
    int n, m;
    int curr_mvd[2];
    int  step_h = blc_size[b8mode][0];
    int  step_v = blc_size[b8mode][1];

    int ref_idx_fw = ( ref_fw < 0 ? 0 : ref_fw );
    int ref_idx_bw = ( ref_bw < 0 ? 0 : ref_bw );
    int ***** fpred_mv;
    int ***** bpred_mv = img->pbmv_com;

    int ** ***fmv_all;
    int ** ***bmv_all = img->bmv_com;
    if ( ( b8pdir == BSYM && b8mode != PSKIP )||b8pdir == MHP )
    {
        fmv_all = img->mv_sym_mhp;
        fpred_mv = img->pmv_sym_mhp;
    }
    else
    {
        fmv_all = img->fmv_com;
        fpred_mv = img->pfmv_com;
    }

    if( b8pdir == FORWARD || b8pdir == BSYM || b8pdir == MHP )
    {
        curr_mvd[0] = fmv_all[ref_idx_fw][b8mode][blk_y][blk_x][0] - fpred_mv[ref_idx_fw][b8mode][blk_y][blk_x][0];
        curr_mvd[1] = fmv_all[ref_idx_fw][b8mode][blk_y][blk_x][1] - fpred_mv[ref_idx_fw][b8mode][blk_y][blk_x][1];
        for ( n = 0; n < step_v; n++ )
        {
            for ( m = 0; m < step_h; m++ )
            {
                currMB->mvd[0][blk_y + n][blk_x + m][0] = curr_mvd[0];
                currMB->mvd[0][blk_y + n][blk_x + m][1] = curr_mvd[1];
            }
        }
    }

    if( b8pdir == BACKWORD || b8pdir == BSYM )
    {
        curr_mvd[0] = bmv_all[ref_idx_bw][b8mode][blk_y][blk_x][0] - bpred_mv[ref_idx_bw][b8mode][blk_y][blk_x][0];
        curr_mvd[1] = bmv_all[ref_idx_bw][b8mode][blk_y][blk_x][1] - bpred_mv[ref_idx_bw][b8mode][blk_y][blk_x][1];
        for ( n = 0; n < step_v; n++ )
        {
            for ( m = 0; m < step_h; m++ )
            {
                currMB->mvd[1][blk_y + n][blk_x + m][0] = curr_mvd[0];
                currMB->mvd[1][blk_y + n][blk_x + m][1] = curr_mvd[1];
            }
        }
    }
}

/*
*************************************************************************
* Function:Writes motion vectors of an 8x8 block
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
int writeOneMVD( Macroblock *currMB, CSobj *cs_aec, int blk_x, int blk_y, int refframe, int fwd_flag )
{
    int            k;
    int            curr_mvd[2];
    int            bwflag     = ( ( refframe<0 || ( !fwd_flag ) )?1:0 );
    int            rate       = 0;
    int*           bitCount   = cs_aec->bitcounter;
    int            blk_idx = blk_y * 2 + blk_x;
    if ( !fwd_flag )
    {
        bwflag = 1;
    }

    curr_mvd[0] = currMB->mvd[bwflag][blk_y][blk_x][0];
    curr_mvd[1] = currMB->mvd[bwflag][blk_y][blk_x][1];

    for ( k=0; k<2; k++ )
    {
        int bit_len = writeMVD_AEC( currMB, cs_aec, blk_idx, curr_mvd[k], 2*k+bwflag );
#if TRACE
        if ( fwd_flag )
        {
            output_trace_info( "FMVD = %3d", curr_mvd[k] );
        }
        else
        {
            output_trace_info( "BMVD = %3d", curr_mvd[k] );
        }
#endif
        mvbits[curr_mvd[k]]      = bit_len;
        bitCount[BITS_INTER_MB] += bit_len;
        rate                    += bit_len;
    }
    return rate;
}

int writeCBPandDqp( Macroblock *currMB, CSobj *cs_aec )
{
    int            rate = 0;
    int*           bitCount = cs_aec->bitcounter;

    rate = writeCBP_AEC( currMB, cs_aec, currMB->cbp );
    bitCount[BITS_CBP_MB] += rate;
#if TRACE
    output_trace_info( "CBP = %3d", currMB->cbp );
#endif
    return ( rate >> 1 );
}



/*
*************************************************************************
* Function:Writes motion info
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

int storeMotionInfo( Macroblock *currMB )
{
    int k, blk_y, blk_x;
    int ref_fw = 0, ref_bw = 0;
    int no_bits = 0;

    int   bframe          = ( img->type == B_IMG );
    int** refframe_array  = ( bframe ? img->bfrm_fref : img->pfrm_ref );
    int** bw_refframe_array  = img->bfrm_bref;
    int   blk_step_h         = ( blc_size[currMB->mb_type][0] );
    int   blk_step_v         = ( blc_size[currMB->mb_type][1] );
    int   b8_y     = img->mb_b8_y;
    int   b8_x     = img->mb_b8_x;
    int   b8mode, b8pdir;

    // write motion vectors
    for ( blk_y=0; blk_y<2; blk_y+=blk_step_v )
    {
        for ( blk_x=0; blk_x<2; blk_x+=blk_step_h )
        {
            k=blk_y*2+blk_x;
            b8mode = currMB->b8mode[k];
            b8pdir = currMB->b8pdir[k];

            if ( b8mode != PSKIP )
            {
                ref_fw  = refframe_array[b8_y+blk_y][b8_x+blk_x];
                if( img->type == B_IMG )
                {
                    ref_bw  = bw_refframe_array[b8_y+blk_y][b8_x+blk_x];
                }
                StoreOneMotionVector( currMB, blk_x, blk_y, ref_fw, ref_bw, b8mode, b8pdir );

            }
        }
    }
    return no_bits;
}

/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

int writeMVD ( Macroblock *currMB, CSobj *cs_aec )
{
    int k, blk_y, blk_x, refframe;
    int rate = 0;
    int no_bits   = 0;

    int** refframe_array  = ( ( img->type==B_IMG ) ? img->bfrm_fref : img->pfrm_ref );
    int** bw_refframe_array = img->bfrm_bref;
    int   blk_step_h        = ( blc_size[currMB->mb_type][0] );
    int   blk_step_v        = ( blc_size[currMB->mb_type][1] );
    int   b8_y     = img->mb_b8_y;
    int   b8_x     = img->mb_b8_x;

    for ( blk_y=0; blk_y<2; blk_y += blk_step_v )
    {
        for ( blk_x=0; blk_x<2; blk_x += blk_step_h )
        {
            k=blk_y*2+blk_x;
            no_bits = 0;
            if ( currMB->b8mode[k] != PSKIP ) //has forward MVD
            {
                refframe  = refframe_array[b8_y+blk_y][b8_x+blk_x];
                if ( currMB->b8pdir[k] == BSYM )
                {
                    no_bits += writeOneMotionVector_sym( currMB, cs_aec, blk_x, blk_y, refframe, 1, currMB->b8mode[k] );
                }
                else if ( currMB->b8pdir[k] == MHP )
                {
                    no_bits += MHMC_writeOneMotionVector( currMB, cs_aec, k, refframe, 1, currMB->b8mode[k] );
                }
                else if ( currMB->b8pdir[k] == FORWARD )
                {
                    no_bits = writeOneMVD( currMB, cs_aec, blk_x, blk_y, refframe, 1 );
                }
            }
            rate += no_bits;
        }
    }

    if ( img->type==B_IMG )
    {
        for ( blk_y=0; blk_y<2; blk_y += blk_step_v )
        {
            for ( blk_x=0; blk_x<2; blk_x += blk_step_h )
            {
                k=blk_y*2+blk_x;
                no_bits = 0;
                if ( currMB->b8pdir[k] == BACKWORD && currMB->b8mode[k] != PSKIP ) //has backward MVD
                {
                    refframe  = bw_refframe_array[b8_y+blk_y][b8_x+blk_x];
                    no_bits   = writeOneMVD ( currMB, cs_aec, blk_x, blk_y, refframe, 0 );
                }
                rate += no_bits;
            }
        }
    }
    return rate;
}

int writeLumaCoeffBlk( Macroblock *currMB, uchar_t uhBitSize, CSobj *cs_aec, int *ACLevel, int *ACRun )
{
    int rate = 0;
    int bsize = 8;
    int coef_num;
    int b8;

    // LUMA, TRANS_NxN
    if( currMB->trans_type == TRANS_2Nx2N )
    {
        bsize = 1 << uhBitSize;
        if( currMB->cbp & 0xF )
        {
            rate += writeLumaCoeff_AEC( cs_aec, bsize, ACLevel, ACRun );
        }
        return rate;
    }
    else
    {
        for( b8 = 0; b8 < 4; b8++ )
        {
            int b8Split;
            b8Split = ( currMB->b8mode[b8] == I_MB ) ? currMB->sub_mb_trans_type[b8] : 0;
            if( b8Split ) // only enabled for intra encoding
            {
                int b4;
                int b8_pix_num = 64;
                int b4_pix_num = 16;
                bsize = 4;
                for ( b4 = 0; b4 < 4; b4++ )
                {
                    if ( currMB->cbp & ( 1 << ( b8 * 4 + b4 ) ) )
                    {
                        rate += writeLumaCoeff_AEC( cs_aec, bsize, ACLevel + b8 * b8_pix_num + b4 * b4_pix_num, ACRun + b8 * b8_pix_num + b4 * b4_pix_num );
                    }
                }
            }
            else     // TRANS_NxN for inter and intra coding
            {
                bsize = 1 << ( uhBitSize - 1 );
                coef_num = bsize * bsize;
                if( currMB->cbp & ( 0xF << ( b8 * 4 ) ) )
                {
                    rate += writeLumaCoeff_AEC( cs_aec, bsize, ACLevel + b8 * coef_num, ACRun + b8 * coef_num );
                }
            }
        }
    }
    return rate;
}

// this function makes no sense, but can be referenced in coding backward MVD of BID prediction
void  GetBidBackMVD(Macroblock *currMB, int curr_mv[2])
{
    int mv_mode, blk_step_h, blk_step_v, blk_y, blk_x;
    int pmv[2], mb_b8_x, mb_b8_y;
    int current_tr, refframe = 0, fw_refframe;
    int k, n, m;

    assert( img->type == B_IMG );

    blk_step_h = ( blc_size[currMB->mb_type][0] );
    blk_step_v = ( blc_size[currMB->mb_type][1] );

    for ( blk_y = 0; blk_y < 2; blk_y += blk_step_v )
    {
        for ( blk_x = 0; blk_x < 2; blk_x += blk_step_h )
        {
            k = blk_y*2 + blk_x;
            if( currMB->b8pdir[k] == BSYM && currMB->b8mode[k] != PSKIP )
            {
                mv_mode = currMB->b8mode[k];
                current_tr  = 2*( img->pic_distance/2 );
                // backward vector
                refframe = 0;
                fw_refframe = img->bfrm_fref[img->mb_b8_y+blk_y][img->mb_b8_y+blk_x];

                SetSpatialMVPredictor( img, currMB, pmv,img->bfrm_bref,img->bfrm_bmv,refframe,blk_x, blk_y, B8_SIZE * blk_step_h, -1 );

                mb_b8_y = img->mb_b8_y+blk_y;
                mb_b8_x = img->mb_b8_x+blk_x;

                // store (oversampled) mvd
                for ( n=0; n < blk_step_v; n++ )
                {
                    for ( m=0; m < blk_step_h; m++ )
                    {
                        currMB->mvd[1][blk_y+n][blk_x+m][0] = curr_mv[0] - pmv[0];
                        currMB->mvd[1][blk_y+n][blk_x+m][1] = curr_mv[1] - pmv[1];
                    }
                }
            }
        }
    }
}

/* Function:Passes the chosen syntax elements to the NAL */
void write_one_cu( ImgParams *img, Macroblock *currMB, CSobj *cs_aec, int mb_nr, uchar_t uhBitSize, int bWrite )
{
    int cu_mb_width = 1 << ( uhBitSize - MB_SIZE_LOG2 );
    int cu_mb_num = cu_mb_width * cu_mb_width;
    int mb_idx;

    int *bitCount = cs_aec->bitcounter;
    int uv;
    int rate;

    CheckAvailabilityOfNeighbors( currMB, mb_nr ); // prepare availabilities for addressing contexts

    // write header
    rate = writeCUHeader( currMB, cs_aec, mb_nr, bWrite );

    // Do nothing more if copy and inter mode
    if ( ( IS_INTERMV( currMB ) || IS_INTRA ( currMB ) ) || ( ( img->type==B_IMG ) && currMB->cbp != 0 ) )
    {
        if( IS_INTERMV( currMB ) )
        {
            writeMVD( currMB, cs_aec );
        }
        rate = writeCBPandDqp( currMB, cs_aec );
        for( mb_idx = 0; mb_idx < cu_mb_num; mb_idx++ )
        {
            rate = writeLumaCoeffBlk( currMB, 4, cs_aec, img->bst_coefAC_luma[0], img->bst_coefAC_luma[1] );
        }
        for( uv = 0; uv < 2; uv++ )
        {
            int isIntra = IS_INTRA( currMB );
            int cbp_cr = ( currMB->cbp >> 4*( uv+4 ) ) & 0xf;
            if( cbp_cr )
            {
                rate = writeChromaCoeff_AEC( cs_aec, isIntra, ( 1 << ( uhBitSize - 1 ) ), img->bst_coefAC_chroma[uv][0], img->bst_coefAC_chroma[uv][1] );
            }
        }
    }

    // set total bit-counter
    bitCount[BITS_TOTAL_MB] = bitCount[BITS_MB_MODE] + bitCount[BITS_COEFF_Y_MB] + bitCount[BITS_INTER_MB] + bitCount[BITS_CBP_MB] + bitCount[BITS_COEFF_UV_MB];
    stat->bit_slice += bitCount[BITS_TOTAL_MB];
}

void write_cu_tree( ImgParams *img, CSobj *cs_aec, uchar_t uhBitSize, int mb_nr )
{
    int i;
    int iCuSize = 1 << uhBitSize;
    Macroblock *currMB = &img->mb_data[mb_nr];
    int pix_x_InPic_start = ( mb_nr % img->PicWidthInMbs ) * MB_SIZE;
    int pix_y_InPic_start = ( mb_nr / img->PicWidthInMbs ) * MB_SIZE;
    int pix_x_InPic_end  = ( mb_nr % img->PicWidthInMbs ) * MB_SIZE + iCuSize;
    int pix_y_InPic_end  = ( mb_nr / img->PicWidthInMbs ) * MB_SIZE + iCuSize;
    //int iBoundary_start   = (pix_x_InPic_start >= input->img_width) || (pix_y_InPic_start >= input->img_height);
    int out_of_pic = ( pix_x_InPic_end > img->width ) || ( pix_y_InPic_end > img->height );

    //if (!iBoundary_start) {
    if ( currMB->uhBitSizeMB == uhBitSize )
    {
        if ( uhBitSize > 4 )
        {
            //writeSplitFlag_AEC(aec, 0, uiBitSize); // not split
        }
        write_one_cu( img, currMB, cs_aec, mb_nr, uhBitSize, 1 );
    }
    else
    {
        if ( !out_of_pic )
        {
            //writeSplitFlag_AEC(aec, 1, uiBitSize); // split
        }
        for ( i = 0; i < 4; i++ )
        {
            int mb_x  = ( i &  1 ) << ( uhBitSize - MB_SIZE_LOG2 - 1 );
            int mb_y  = ( i >> 1 ) << ( uhBitSize - MB_SIZE_LOG2 - 1 );
            int mb_pos = mb_nr + mb_y * img->PicWidthInMbs + mb_x;
            int pos_x = pix_x_InPic_start + ( mb_x << MB_SIZE_LOG2 );
            int pos_y = pix_y_InPic_start + ( mb_y << MB_SIZE_LOG2 );

            if ( pos_x >= img->width || pos_y >= img->height ) // check the starting position
            {
                continue;
            }
            write_cu_tree( img, cs_aec, uhBitSize - 1, mb_pos );
        }
    }
    //}
}
