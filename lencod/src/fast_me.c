/*
*************************************************************************************
* File name:
* Function:
Fast integer pel motion estimation and fractional pel motion estimation
algorithms are described in this file.
1. get_mem_FME() and free_mem_FME() are functions for allocation and release
of memories about motion estimation
2. FME_BlockMotionSearch() is the function for fast integer pel motion
estimation and fractional pel motion estimation
3. DefineThreshold() defined thresholds for early termination
*
*************************************************************************************
*/

#include <math.h>
#include <string.h>
#include <memory.h>
#include <assert.h>

#include "memalloc.h"
#include "fast_me.h"
#include "refbuf.h"
#include "block.h"
#include "../../common/interPrediction.h"
#include "tz_search.h"
extern  int   *byte_abs;
extern  int   *mvbits;
extern  int   *square_points_x;
extern  int   *square_points_y;

extern  int check_mvd( int mvd_x, int mvd_y );
extern  int check_mv_range( int mv_x, int mv_y, int pix_x, int pix_y, int mode );
extern  int check_mv_range_bid( int mv_x, int mv_y, int pix_x, int pix_y, int mode );

static pel_t ( *PelY_14 ) ( pel_t*, int, int );
static pel_t ( *Fw_PelY_14 ) ( pel_t*, int, int );

i16u_t MEQ_TAB[64] =
{
    32768, 29775, 27554, 25268, 23170, 21247, 19369, 17770,
    16302, 15024, 13777, 12634, 11626, 10624, 9742, 8958,
    8192, 7512, 6889, 6305, 5793, 5303, 4878, 4467,
    4091, 3756, 3444, 3161, 2894, 2654, 2435, 2235,
    2048, 1878, 1722, 1579, 1449, 1329, 1218, 1117,
    1024, 939, 861, 790, 724, 664, 609, 558,
    512, 470, 430, 395, 362, 332, 304, 279,
    256, 235, 215, 197, 181, 166, 152, 140
};

void DefineThreshold()
{
    static float ThresholdFac[8] = {0, 8, 4, 4, 2.5, 1.5, 1.5, 1};
    static int ThreshUp[8] = {0, 1024, 512, 512, 448, 384, 384, 384};

    AlphaSec[1] = 0.01f;
    AlphaSec[2] = 0.01f;
    AlphaSec[3] = 0.01f;
    AlphaSec[4] = 0.02f;
    AlphaSec[5] = 0.03f;
    AlphaSec[6] = 0.03f;
    AlphaSec[7] = 0.04f;

    AlphaThird[1] = 0.06f;
    AlphaThird[2] = 0.07f;
    AlphaThird[3] = 0.07f;
    AlphaThird[4] = 0.08f;
    AlphaThird[5] = 0.12f;
    AlphaThird[6] = 0.11f;
    AlphaThird[7] = 0.15f;

    DefineThresholdMB();
    return;
}

void DefineThresholdMB()
{
    int gb_q_bits    = 15;
    int gb_qp_const, Thresh4x4;

    if ( img->type == I_IMG )
    {
        gb_qp_const = ( 1 << gb_q_bits ) / 3;  // intra
    }
    else
    {
        gb_qp_const = ( 1 << gb_q_bits ) / 6;  // inter
    }

    if ( input->low_delay == 1 )
    {
        Thresh4x4 = ( ( 1 << gb_q_bits ) - gb_qp_const ) / MEQ_TAB[input->qp_P_frm];
    }
    else
    {
        Thresh4x4 = ( ( 1 << gb_q_bits ) - gb_qp_const ) / MEQ_TAB[input->qp_B_frm];
    }

    Bsize[7]=( 16*16 ) * ( Thresh4x4/( 4*5.61f ) );

    Bsize[6] = Bsize[7] * 4;
    Bsize[5] = Bsize[7] * 4;
    Bsize[4] = Bsize[5] * 4;
    Bsize[3] = Bsize[4] * 4;
    Bsize[2] = Bsize[4] * 4;
    Bsize[1] = Bsize[2] * 4;
}

/*
*************************************************************************
* Function:Dynamic memory allocation of all infomation needed for Fast ME
* Input:
* Output:
* Return: Number of allocated bytes
* Attention:
*************************************************************************
*/

int get_mem_mincost( int **** **mv )
{
    int i, j, k, l;
    int *m;


    if ( ( *mv = ( int **** * )calloc( img->width / 4, sizeof( int ** ** ) ) ) == NULL )
    {
        no_mem_exit( "get_mem_mv: mv" );
    }
    for ( i = 0; i < img->width / 4; i++ )
    {
        if ( ( ( *mv )[i] = ( int ** ** )calloc( img->height / 4, sizeof( int ** * ) ) ) == NULL )
        {
            no_mem_exit( "get_mem_mv: mv" );
        }
        for ( j = 0; j < img->height / 4; j++ )
        {
            if ( ( ( *mv )[i][j] = ( int ** * )calloc( img->real_ref_num, sizeof( int ** ) ) ) == NULL )
            {
                no_mem_exit( "get_mem_mv: mv" );
            }
            for ( k = 0; k < img->real_ref_num ; k++ )
            {
                if ( ( ( *mv )[i][j][k] = ( int ** )calloc( MAXMODE, sizeof( int * ) ) ) == NULL )
                {
                    no_mem_exit( "get_mem_mv: mv" );
                }
            }
        }
    }

    ( *mv )[0][0][0][0] = ( int * )calloc( img->width / 4 * img->height / 4 * img->real_ref_num * MAXMODE * 3, sizeof( int ) );

    m = ( *mv )[0][0][0][0];

    for ( i = 0; i < img->width / 4; i++ )
    {
        for ( j = 0; j < img->height / 4; j++ )
            for ( k = 0; k < img->real_ref_num; k++ )
                for ( l = 0; l < MAXMODE; l++ )
                {
                    ( *mv )[i][j][k][l] = m;
                    m += 3;
                }
    }

    return img->width / 4 * img->height / 4 * 2 * 9 * 3 * sizeof( int ) * img->real_ref_num;
}

int get_mem_FME()
{
    int memory_size = 0;
    memory_size += get_mem2Dint( &McostState, 2 * input->search_range + 1, 2 * input->search_range + 1 );
    memory_size += get_mem_mincost( &( all_mincost ) );
    memory_size += get_mem_mincost( &( all_bwmincost ) );
    memory_size += get_mem2D( &SearchState, 7, 7 );

    return memory_size;
}

/*
*************************************************************************
* Function:free the memory allocated for of all infomation needed for Fast ME
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void free_mem_mincost( int ** *** mv )
{
    int i, j, k;

    free( mv[0][0][0][0] );
    for ( i = 0; i < img->width / 4; i++ )
    {
        for ( j = 0; j < img->height / 4; j++ )
        {
            for ( k = 0; k < img->real_ref_num; k++ )
            {
                free( mv[i][j][k] );
            }
            free( mv[i][j] );
        }
        free( mv[i] );
    }
    free( mv );

}




void free_mem_FME()
{
    free_mem2Dint( McostState );
    free_mem_mincost( all_mincost );
    free_mem_mincost( all_bwmincost );

    free_mem2D( SearchState );
}

void SetMotionVectorPredictorME( ImgParams *img, Macroblock *currMB, int pmv[2], int **refFrArr, int ***tmp_mv,
                                    int ref_frame,
                                    int pix_x_in_mb,
                                    int pix_y_in_mb,
                                    int bsize_x,
                                    int mode,
                                    int ref )
{
    int pu_b8_x          = img->mb_b8_x + ( pix_x_in_mb>>3 );
    int pu_b8_y          = img->mb_b8_y + ( pix_y_in_mb>>3 );
    int pu_b4_x          = img->mb_b4_x + ( pix_x_in_mb>>2 );
    int pu_b4_y          = img->mb_b4_y + ( pix_y_in_mb>>2 );
    int mb_nr             = img->current_mb_nr;
    int mb_width          = img->PicWidthInMbs;
    int mb_available_up   = ( img->mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr-mb_width].slice_nr );
    int mb_available_left = ( img->mb_x == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr-1       ].slice_nr );
    int mb_available_up_right = ( img->mb_x >= mb_width-1 || img->mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr-mb_width+1].slice_nr );
    int block_available_up, block_available_left, block_available_up_right, block_available_up_left;
    int mv_a, mv_b, mv_c, mv_d, pred_vec = 0;
    int mvPredType, rFrameL, rFrameU, rFrameUR, rFrameUL;
    int hv, diff_a, diff_b, diff_c;

    int SAD_a, SAD_b, SAD_c, SAD_d;
    int temp_pred_SAD[2];

    pred_SAD_space = 0;

    /* D B C */
    /* A X   */

    /* 1 A, B, D are set to 0 if unavailable       */
    /* 2 If C is not available it is replaced by D */

    block_available_up   = mb_available_up   || ( pix_y_in_mb > 0 );
    block_available_left = mb_available_left || ( pix_x_in_mb > 0 );
    block_available_up_left = block_available_up && block_available_left;

    if ( pix_y_in_mb > 0 )
    {
        if ( pix_x_in_mb < 8 ) // first column of 8x8 blocks
        {
            if ( pix_y_in_mb==8 )
            {
                block_available_up_right = ( bsize_x != 16 );
            }
            else
            {
                block_available_up_right = ( pix_x_in_mb+bsize_x != 8 );
            }
        }
        else
        {
            block_available_up_right = ( pix_x_in_mb+bsize_x != 16 );
        }
    }
    else if ( pix_x_in_mb+bsize_x != MB_SIZE )
    {
        block_available_up_right = block_available_up;
    }
    else
    {
        block_available_up_right = mb_available_up_right;
    }

    //FAST MOTION ESTIMATION. ZHIBO CHEN 2003.3
    SAD_a = block_available_left     ? ( ( ref == -1 ) ? all_bwmincost[pu_b4_x -1][pu_b4_y][0][mode][0]    : all_mincost[pu_b4_x -1][pu_b4_y][ref_frame][mode][0] )    : 0;
    SAD_b = block_available_up       ? ( ( ref == -1 ) ? all_bwmincost[pu_b4_x]   [pu_b4_y -1][0][mode][0] : all_mincost[pu_b4_x]   [pu_b4_y -1][ref_frame][mode][0] ) : 0;
    SAD_d = block_available_up_left  ? ( ( ref == -1 ) ? all_bwmincost[pu_b4_x -1][pu_b4_y -1][0][mode][0] : all_mincost[pu_b4_x -1][pu_b4_y -1][ref_frame][mode][0] ) : 0;
    SAD_c = block_available_up_right ? ( ( ref == -1 ) ? all_bwmincost[pu_b4_x +1][pu_b4_y -1][0][mode][0] : all_mincost[pu_b4_x +1][pu_b4_y -1][ref_frame][mode][0] ) : SAD_d;

    rFrameL    = block_available_left     ? refFrArr[pu_b8_y]  [pu_b8_x-1] : -1;
    rFrameU    = block_available_up       ? refFrArr[pu_b8_y-1][pu_b8_x]   : -1;
    rFrameUR   = block_available_up_right ? refFrArr[pu_b8_y-1][pu_b8_x + ( bsize_x >> 3 )] :
                 block_available_up_left  ? refFrArr[pu_b8_y-1][pu_b8_x-1] : -1;
    rFrameUL   = block_available_up_left  ? refFrArr[pu_b8_y-1][pu_b8_x-1] : -1;

    mvPredType = MVPRED_xy_MIN;
    if( ( rFrameL != -1 )&&( rFrameU == -1 )&&( rFrameUR == -1 ) )
    {
        mvPredType = MVPRED_L;
    }
    else if( ( rFrameL == -1 )&&( rFrameU != -1 )&&( rFrameUR == -1 ) )
    {
        mvPredType = MVPRED_U;
    }
    else if( ( rFrameL == -1 )&&( rFrameU == -1 )&&( rFrameUR != -1 ) )
    {
        mvPredType = MVPRED_UR;
    }

    for ( hv=0; hv < 2; hv++ )
    {
        mv_a = block_available_left     ? tmp_mv[pu_b8_y - 0][4 + pu_b8_x-1][hv] : 0;
        mv_b = block_available_up       ? tmp_mv[pu_b8_y - 1][4 + pu_b8_x  ][hv] : 0;
        mv_d = block_available_up_left  ? tmp_mv[pu_b8_y - 1][4 + pu_b8_x-1][hv] : 0;
        mv_c = block_available_up_right ? tmp_mv[pu_b8_y - 1][4 + pu_b8_x + ( bsize_x >> 3 )][hv] : mv_d;

        switch ( mvPredType )
        {
            case MVPRED_xy_MIN:
                mv_c = block_available_up_right ? mv_c : mv_d;

                if ( ( ( mv_a < 0 ) && ( mv_b > 0 ) && ( mv_c > 0 ) ) || ( mv_a > 0 ) && ( mv_b < 0 ) && ( mv_c < 0 ) )
                {
                    pmv[hv] = ( mv_b + mv_c ) / 2;
                    temp_pred_SAD[1] = temp_pred_SAD[0] = SAD_b;
                }
                else if ( ( ( mv_b < 0 ) && ( mv_a > 0 ) && ( mv_c > 0 ) ) || ( ( mv_b > 0 ) && ( mv_a < 0 ) && ( mv_c < 0 ) ) )
                {
                    pmv[hv] = ( mv_c + mv_a ) / 2;
                    temp_pred_SAD[1] = temp_pred_SAD[0] = SAD_c;
                }
                else if ( ( ( mv_c < 0 ) && ( mv_a > 0 ) && ( mv_b > 0 ) ) || ( ( mv_c > 0 ) && ( mv_a < 0 ) && ( mv_b < 0 ) ) )
                {
                    pmv[hv] = ( mv_a + mv_b ) / 2;
                    temp_pred_SAD[1] = temp_pred_SAD[0] = SAD_a;
                }
                else
                {
                    diff_a = abs( mv_a - mv_b );
                    diff_b = abs( mv_b - mv_c );
                    diff_c = abs( mv_c - mv_a );
                    pred_vec = MIN( diff_a, MIN( diff_b, diff_c ) );

                    if ( pred_vec == diff_a )
                    {
                        pmv[hv] = ( mv_a + mv_b ) / 2;
                        temp_pred_SAD[1] = temp_pred_SAD[0] = SAD_a;
                    }
                    else if( pred_vec == diff_b )
                    {
                        pmv[hv] = ( mv_b + mv_c ) / 2;
                        temp_pred_SAD[1] = temp_pred_SAD[0] = SAD_b;
                    }
                    else
                    {
                        pmv[hv] = ( mv_c + mv_a ) / 2;
                        temp_pred_SAD[1] = temp_pred_SAD[0] = SAD_c;
                    }
                }
                break;

            case MVPRED_L:
                pmv[hv] = mv_a;
                temp_pred_SAD[hv] = SAD_a;
                break;
            case MVPRED_U:
                pmv[hv] = mv_b;
                temp_pred_SAD[hv] = SAD_b;
                break;
            case MVPRED_UR:
                pmv[hv] = mv_c;
                temp_pred_SAD[hv] = SAD_c;
                break;
            default:
                break;
        }
    }
    pred_SAD_space = ( temp_pred_SAD[0] > temp_pred_SAD[1] ) ? temp_pred_SAD[1] : temp_pred_SAD[0];
}

int FastMVSeach( Macroblock *currMB, int ref, int pu_b8_x, int pu_b8_y, int mode )
{
    static pel_t orig_val[MB_SIZE*MB_SIZE];
    int       stride = MB_SIZE, index_pos = 0;
    int       pred_mv_x, pred_mv_y, mv_x, mv_y, i, j;

    int       max_value = ( 1 << 20 );
    int       min_mcost = max_value;

    int pu_pix_x = pu_b8_x << 3;
    int pu_pix_y = pu_b8_y << 3;
    int pu_pix_x_in_mb = pu_pix_x - img->mb_pix_x;
    int pu_pix_y_in_mb = pu_pix_y - img->mb_pix_y;
    int b8_x_in_mb = ( pu_pix_x_in_mb >> 3 );
    int b8_y_in_mb = ( pu_pix_y_in_mb>> 3 );
    int mb_b4_x = img->mb_b4_x;
    int mb_b4_y = img->mb_b4_y;

    int blk_step_x = blc_size[mode][0];
    int blk_step_y = blc_size[mode][1];
    int b4_step_x = blk_step_x << 1;
    int b4_step_y = blk_step_y << 1;
    int pu_bsize_x = blk_step_x << 3;
    int pu_bsize_y = blk_step_y << 3;

    int refframe  = ( ref == -1 ? 0 : ref );
    int **ref_array = ( ( img->type != B_IMG ) ? img->pfrm_ref : ref >= 0 ? img->bfrm_fref : img->bfrm_bref );
    int ***mv_array = ( ( img->type != B_IMG ) ? img->pfrm_mv  : ref >= 0 ? img->bfrm_fmv    : img->bfrm_bmv );
    int *****all_mv = ( ref < 0 ? img->bmv_com : img->fmv_com );

    int N_Bframe = input->successive_Bframe, n_Bframe = ( N_Bframe ) ? ( ( Bframe_ctr % N_Bframe ) ? Bframe_ctr % N_Bframe : N_Bframe ) : 0 ;


    int center_x = pu_pix_x;
    int center_y = pu_pix_y;

    int *pred_mv = ( ( img->type != B_IMG ) ? img->pfmv_com  : ref >= 0 ? img->pfmv_com : img->pbmv_com )[refframe][mode][b8_y_in_mb][b8_x_in_mb];

    //get original block
    for ( j = 0; j < pu_bsize_y; j++ )
    {
        for ( i = 0; i < pu_bsize_x; i++ )
        {
            orig_val[index_pos + i] = imgY_org[( pu_pix_y + j )*( img->width ) + pu_pix_x + i];
        }
        index_pos += stride;
    }

    if ( mode == 4 )
    {
        pred_MV_uplayer[0] = all_mv[refframe][2][b8_y_in_mb][b8_x_in_mb][0];
        pred_MV_uplayer[1] = all_mv[refframe][2][b8_y_in_mb][b8_x_in_mb][1];
        pred_SAD_uplayer    = ( ref == -1 ) ? ( all_bwmincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][0][2][0] ) : ( all_mincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][refframe][2][0] );
        pred_SAD_uplayer   /= 2;
    }
    else if ( mode > 1 )
    {
        pred_MV_uplayer[0] = all_mv[refframe][1][b8_y_in_mb][b8_x_in_mb][0];
        pred_MV_uplayer[1] = all_mv[refframe][1][b8_y_in_mb][b8_x_in_mb][1];
        pred_SAD_uplayer    = ( ref == -1 ) ? ( all_bwmincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][0][1][0] ) : ( all_mincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][refframe][1][0] );
        pred_SAD_uplayer   /= 2;
    }

    pred_SAD_uplayer = flag_intra_SAD ? 0 : pred_SAD_uplayer;// for irregular motion

    //coordinate prediction
    if ( img->ip_frm_idx > refframe + 1 )
    {
        pred_MV_time[0] = all_mincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][refframe][mode][1];
        pred_MV_time[1] = all_mincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][refframe][mode][2];
    }
    if ( ref == -1 && Bframe_ctr > 1 )
    {
        pred_MV_time[0] = ( int )( all_bwmincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][0][mode][1] * ( ( n_Bframe == 1 ) ? ( N_Bframe ) : ( N_Bframe - n_Bframe + 1.0 ) / ( N_Bframe - n_Bframe + 2.0 ) ) ); //should add a factor
        pred_MV_time[1] = ( int )( all_bwmincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][0][mode][2] * ( ( n_Bframe == 1 ) ? ( N_Bframe ) : ( N_Bframe - n_Bframe + 1.0 ) / ( N_Bframe - n_Bframe + 2.0 ) ) ); //should add a factor
    }

    if ( refframe > 0 )
    {
        //field_mode top_field
        if ( img->type == P_IMG )
        {
            pred_SAD_ref = all_mincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][( refframe )][mode][0];
            pred_SAD_ref = flag_intra_SAD ? 0 : pred_SAD_ref;//add this for irregular motion
            pred_MV_ref[0] = all_mincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][( refframe )][mode][1];
            pred_MV_ref[0] = ( int )( pred_MV_ref[0] * ( refframe + 1 ) / ( float )( refframe ) );
            pred_MV_ref[1] = all_mincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][( refframe )][mode][2];
            pred_MV_ref[1] = ( int )( pred_MV_ref[1] * ( refframe + 1 ) / ( float )( refframe ) );
        }
        else
        {
            pred_SAD_ref = all_mincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][( refframe - 1 )][mode][0];
            pred_SAD_ref = flag_intra_SAD ? 0 : pred_SAD_ref;//add this for irregular motion
            pred_MV_ref[0] = all_mincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][( refframe - 1 )][mode][1];
            pred_MV_ref[0] = ( int )( pred_MV_ref[0] * ( refframe + 1 ) / ( float )( refframe ) );
            pred_MV_ref[1] = all_mincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][( refframe - 1 )][mode][2];
            pred_MV_ref[1] = ( int )( pred_MV_ref[1] * ( refframe + 1 ) / ( float )( refframe ) );
        }
    }
    if ( img->type == B_IMG && ref == 0 )
    {
        pred_SAD_ref = all_bwmincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][0][mode][0];
        pred_SAD_ref = flag_intra_SAD ? 0 : pred_SAD_ref;//add this for irregular motion
        pred_MV_ref[0] = ( int )( all_bwmincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][0][mode][1] * ( -n_Bframe ) / ( N_Bframe - n_Bframe + 1.0f ) ); //should add a factor
        pred_MV_ref[1] = ( int )( all_bwmincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][0][mode][2] * ( -n_Bframe ) / ( N_Bframe - n_Bframe + 1.0f ) );
    }

#if USING_TZ_SEARCH
    int i_mvc;
    int pmv[5][2] = { { 0 } };
    i_mvc = Get_mvc(img, currMB, pmv, ref_array, mv_array, refframe, pu_pix_x_in_mb, pu_pix_y_in_mb, pu_bsize_x, mode, ref);
    pred_mv_x = 0;
    pred_mv_y = 0;
    SetMotionVectorPredictorME(img, currMB, pred_mv, ref_array, mv_array, refframe, pu_pix_x_in_mb, pu_pix_y_in_mb, pu_bsize_x, mode, ref);
    pred_mv_x = pred_mv[0];
    pred_mv_y = pred_mv[1];
    mv_x = pred_mv_x / 4;
    mv_y = pred_mv_y / 4;
#else
    // get motion mv predictor
    SetMotionVectorPredictorME( img, currMB, pred_mv, ref_array, mv_array, refframe, pu_pix_x_in_mb, pu_pix_y_in_mb, pu_bsize_x, mode, ref );
    pred_mv_x = pred_mv[0];
    pred_mv_y = pred_mv[1];

    // integer-pel search
    mv_x = pred_mv_x / 4;
    mv_y = pred_mv_y / 4;

    min_mcost = FastIntegerMVSearch( orig_val,stride, ref, center_x, center_y, mode,
                pred_mv_x, pred_mv_y, &mv_x, &mv_y, min_mcost );
#endif // USING_TZ_SEARCH
    for ( i = 0; i < b4_step_x; i++ )
    {
        for ( j = 0; j < b4_step_y; j++ )
        {
            if ( ref > -1 )
            {
                all_mincost[mb_b4_x + b8_x_in_mb + i][mb_b4_y + b8_y_in_mb + j][refframe][mode][0] = min_mcost;
            }
            else
            {
                all_bwmincost[mb_b4_x + b8_x_in_mb + i][mb_b4_y + b8_y_in_mb + j][0][mode][0] = min_mcost;
            }
        }
    }

    // sub-pel search
    if ( input->hadamard )
    {
        min_mcost = max_value;
    }

    if ( mode > 3 )
    {   // 4 points SAD
        min_mcost =  CrossFractionalMVSearchSAD( orig_val,stride, ref, center_x, center_y, mode,
                     pred_mv_x, pred_mv_y, &mv_x, &mv_y, min_mcost );
    }
    else
    {   // 8 points SATD
        min_mcost =  SquareFractionalMVSearchSATD( orig_val,stride, ref, center_x, center_y, mode,
                                              pred_mv_x, pred_mv_y, &mv_x, &mv_y, 9, 9, min_mcost );
    }

    for ( i = 0; i < b4_step_x; i++ )
    {
        for ( j = 0; j < b4_step_y; j++ )
        {
            if ( ref > -1 )
            {
                all_mincost[mb_b4_x + b8_x_in_mb + i][mb_b4_y + b8_y_in_mb + j][refframe][mode][1] = mv_x;
                all_mincost[mb_b4_x + b8_x_in_mb + i][mb_b4_y + b8_y_in_mb + j][refframe][mode][2] = mv_y;
            }
            else
            {
                all_bwmincost[mb_b4_x + b8_x_in_mb + i][mb_b4_y + b8_y_in_mb + j][0][mode][1] = mv_x;
                all_bwmincost[mb_b4_x + b8_x_in_mb + i][mb_b4_y + b8_y_in_mb + j][0][mode][2] = mv_y;
            }
        }
    }

    // set mv and return motion cost
    for ( i = 0; i < blk_step_x; i++ )
    {
        for ( j = 0; j < blk_step_y; j++ )
        {
            all_mv[refframe][mode][b8_y_in_mb + j][b8_x_in_mb + i][0] = mv_x;
            all_mv[refframe][mode][b8_y_in_mb + j][b8_x_in_mb + i][1] = mv_y;
        }
    }

    img->mv_range_flag = check_mv_range( mv_x, mv_y, pu_pix_x, pu_pix_y, mode );
    img->mv_range_flag *= check_mvd( ( mv_x - pred_mv_x ), ( mv_y - pred_mv_y ) );
    if ( !img->mv_range_flag )
    {
        min_mcost = 1 << 24;
        img->mv_range_flag = 1;
    }

    return min_mcost;
}

int FastMotionSearchMhp( Macroblock *currMB, int ref, int pu_b8_x, int pu_b8_y, int mode )
{
    static pel_t   orig_val [MB_SIZE*MB_SIZE];
    int       stride = MB_SIZE, index_pos = 0;
    int       pred_mv_x, pred_mv_y, mv_x, mv_y, i, j;
    int       bi_pred_mv_x, bi_pred_mv_y, bi_mv_x, bi_mv_y;

    int       max_value = ( 1 << 20 );
    int       min_mcost = max_value;
    int       refframe  = ( ref == -1 ? 0 : ref );
    int **ref_array = img->pfrm_ref;
    int ***mv_array = img->pfrm_mv;
    int *****all_mv = img->mv_sym_mhp;
    int *****bmv_mph = img->bmv_mhp;

    int       N_Bframe = input->successive_Bframe, n_Bframe = ( N_Bframe ) ? ( ( Bframe_ctr % N_Bframe ) ? Bframe_ctr % N_Bframe : N_Bframe ) : 0 ;


    int pu_pix_x = pu_b8_x << 3;
    int pu_pix_y = pu_b8_y << 3;
    int pu_pix_x_in_mb = pu_pix_x - img->mb_pix_x;
    int pu_pix_y_in_mb = pu_pix_y - img->mb_pix_y;
    int b8_x_in_mb = ( pu_pix_x_in_mb >> 3 );
    int b8_y_in_mb = ( pu_pix_y_in_mb>> 3 );
    int mb_b4_x = img->mb_b4_x;
    int mb_b4_y = img->mb_b4_y;

    int blk_step_x = blc_size[mode][0];
    int blk_step_y = blc_size[mode][1];
    int b4_step_x = blk_step_x << 1;
    int b4_step_y = blk_step_y << 1;
    int pu_bsize_x = blk_step_x << 3;
    int pu_bsize_y = blk_step_y << 3;


    int center_x = pu_pix_x;
    int center_y = pu_pix_y;

    int *pred_mv = img->pmv_sym_mhp[refframe][mode][b8_y_in_mb][b8_x_in_mb];

    // get original block
    for ( j = 0; j < pu_bsize_y; j++ )
    {
        for ( i = 0; i < pu_bsize_x; i++ )
        {
            orig_val[index_pos + i] = imgY_org[( pu_pix_y + j )*( img->width ) + pu_pix_x + i];
        }
        index_pos += stride;
    }


    if ( mode == 4 )
    {
        pred_MV_uplayer[0] = all_mv[refframe][2][b8_y_in_mb][b8_x_in_mb][0];
        pred_MV_uplayer[1] = all_mv[refframe][2][b8_y_in_mb][b8_x_in_mb][1];
        pred_SAD_uplayer    = ( ref == -1 ) ? ( all_bwmincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][0][2][0] ) : ( all_mincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][refframe][2][0] );
        pred_SAD_uplayer   /= 2;
    }
    else if ( mode > 1 )
    {
        pred_MV_uplayer[0] = all_mv[refframe][1][b8_y_in_mb][b8_x_in_mb][0];
        pred_MV_uplayer[1] = all_mv[refframe][1][b8_y_in_mb][b8_x_in_mb][1];
        pred_SAD_uplayer    = ( ref == -1 ) ? ( all_bwmincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][0][1][0] ) : ( all_mincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][refframe][1][0] );
        pred_SAD_uplayer   /= 2;
    }

    pred_SAD_uplayer = flag_intra_SAD ? 0 : pred_SAD_uplayer;// for irregular motion

    //coordinate prediction
    if ( img->ip_frm_idx > refframe + 1 )
    {
        pred_MV_time[0] = all_mincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][refframe][mode][1];
        pred_MV_time[1] = all_mincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][refframe][mode][2];
    }
    if ( ref == -1 && Bframe_ctr > 1 )
    {
        pred_MV_time[0] = ( int )( all_bwmincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][0][mode][1] * ( ( n_Bframe == 1 ) ? ( N_Bframe ) : ( N_Bframe - n_Bframe + 1.0 ) / ( N_Bframe - n_Bframe + 2.0 ) ) ); //should add a factor
        pred_MV_time[1] = ( int )( all_bwmincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][0][mode][2] * ( ( n_Bframe == 1 ) ? ( N_Bframe ) : ( N_Bframe - n_Bframe + 1.0 ) / ( N_Bframe - n_Bframe + 2.0 ) ) ); //should add a factor
    }

    if ( refframe > 0 )
    {
        pred_SAD_ref = all_mincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][( refframe )][mode][0];
        pred_SAD_ref = flag_intra_SAD ? 0 : pred_SAD_ref;//add this for irregular motion
        pred_MV_ref[0] = all_mincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][( refframe )][mode][1];
        pred_MV_ref[0] = ( int )( pred_MV_ref[0] * ( refframe + 1 ) / ( float )( refframe ) );
        pred_MV_ref[1] = all_mincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][( refframe )][mode][2];
        pred_MV_ref[1] = ( int )( pred_MV_ref[1] * ( refframe + 1 ) / ( float )( refframe ) );
    }

    //get motion mv predictor
    SetMotionVectorPredictorME( img, currMB, pred_mv, ref_array, mv_array, refframe, pu_pix_x_in_mb, pu_pix_y_in_mb, pu_bsize_x, mode, ref );

    pred_mv_x = pred_mv[0];
    pred_mv_y = pred_mv[1];
    bi_pred_mv_x = pred_mv[0];
    bi_pred_mv_y = pred_mv[1];
    bi_mv_x = pred_mv[0];
    bi_mv_y = pred_mv[1];

    for ( i = 0; i < blk_step_x; i++ )
    {
        for ( j = 0; j < blk_step_y; j++ )
        {
            bmv_mph[refframe][mode][b8_y_in_mb + j][b8_x_in_mb + i][0] = bi_mv_x;
            bmv_mph[refframe][mode][b8_y_in_mb + j][b8_x_in_mb + i][1] = bi_mv_y;
        }
    }

    // integer-pel search
    mv_x = pred_mv_x / 4;
    mv_y = pred_mv_y / 4;

    min_mcost = FastIntegerMVSearchMhp( orig_val,stride, ref, center_x, center_y, mode,
                pred_mv_x, pred_mv_y, &mv_x, &mv_y, &bi_mv_x, &bi_mv_y, min_mcost );


    for ( i = 0; i < b4_step_x; i++ )
    {
        for ( j = 0; j < b4_step_y; j++ )
        {
            if ( ref > -1 )
            {
                all_mincost[mb_b4_x + b8_x_in_mb + i][mb_b4_y + b8_y_in_mb + j][refframe][mode][0] = min_mcost;
            }
            else
            {
                all_bwmincost[mb_b4_x + b8_x_in_mb + i][mb_b4_y + b8_y_in_mb + j][0][mode][0] = min_mcost;
            }
        }
    }

    // sul-pel search
    if ( input->hadamard )
    {
        min_mcost = max_value;
    }

    if ( mode > 3 )
    {
        min_mcost =  CrossFractionalMVSearchSADMhp( orig_val,stride, ref, center_x, center_y, mode,
                     pred_mv_x, pred_mv_y, &mv_x, &mv_y, &bi_mv_x, &bi_mv_y, min_mcost );
    }
    else
    {
        min_mcost =  SquareFractionalMVSearchSATDMhp( orig_val, stride, ref, center_x, center_y, mode,
                     pred_mv_x, pred_mv_y, &mv_x, &mv_y, &bi_mv_x, &bi_mv_y, 9, 9,
                     min_mcost );
    }

    for ( i = 0; i < b4_step_x; i++ )
    {
        for ( j = 0; j < b4_step_y; j++ )
        {
            if ( ref > -1 )
            {
                all_mincost[mb_b4_x + b8_x_in_mb + i][mb_b4_y + b8_y_in_mb + j][refframe][mode][1] = mv_x;
                all_mincost[mb_b4_x + b8_x_in_mb + i][mb_b4_y + b8_y_in_mb + j][refframe][mode][2] = mv_y;
            }
            else
            {
                all_bwmincost[mb_b4_x + b8_x_in_mb + i][mb_b4_y + b8_y_in_mb + j][0][mode][1] = mv_x;
                all_bwmincost[mb_b4_x + b8_x_in_mb + i][mb_b4_y + b8_y_in_mb + j][0][mode][2] = mv_y;
            }
        }
    }

    // set mv and return motion cost
    for ( i = 0; i < blk_step_x; i++ )
    {
        for ( j = 0; j < blk_step_y; j++ )
        {
            all_mv[refframe][mode][b8_y_in_mb + j][b8_x_in_mb + i][0] = mv_x;
            all_mv[refframe][mode][b8_y_in_mb + j][b8_x_in_mb + i][1] = mv_y;
        }
    }

    img->mv_range_flag = check_mv_range( mv_x, mv_y, pu_pix_x, pu_pix_y, mode );
    img->mv_range_flag *= check_mvd( ( mv_x - pred_mv_x ), ( mv_y - pred_mv_y ) );
    if ( !img->mv_range_flag )
    {
        min_mcost = 1 << 24;
        img->mv_range_flag = 1;
    }

    return min_mcost;
}

_inline int PartCalMad( pel_t *ref_pic,pel_t* orig_val, int stride, pel_t *( *get_ref_line )( int, pel_t*, int, int ), int mode, int mcost, int min_mcost, int cand_x, int cand_y )
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

_inline int MHMC_PartCalMad( pel_t *ref_pic,pel_t* fw_ref_pic, pel_t* orig_val, int stride, pel_t *( *get_ref_line )( int, pel_t*, int, int ), int mode, int mcost, int min_mcost, int cand_x, int cand_y, int pic_pix_x, int pic_pix_y, int fw_mv_x, int fw_mv_y )
{
    int y, x4;
    int ry, rx;
    int index_pos = 0;
    int blocksize_x  = ( blc_size[mode][0] << 3 );
    int blocksize_y  = ( blc_size[mode][1] << 3 );
    int blocksize_x4 = blocksize_x >> 2;
    pel_t *orig_line, *ref_line;
    for ( y = 0; y < blocksize_y; y++ )
    {
        ry = ( ( pic_pix_y + y ) << 2 ) + fw_mv_y;
        ref_line  = get_ref_line( blocksize_x, ref_pic, cand_y + y, cand_x );
        index_pos = y * stride;
        orig_line = orig_val + index_pos;

        for ( x4 = 0; x4 < blocksize_x4; x4++ )
        {
            rx = ( ( pic_pix_x + x4 * 4 ) << 2 ) + fw_mv_x;
            mcost += byte_abs[ *orig_line++ - ( ( *ref_line++ ) + Fw_PelY_14( fw_ref_pic, ry, rx ) ) / 2];
            mcost += byte_abs[ *orig_line++ - ( ( *ref_line++ ) + Fw_PelY_14( fw_ref_pic, ry, rx + 4 ) ) / 2];
            mcost += byte_abs[ *orig_line++ - ( ( *ref_line++ ) + Fw_PelY_14( fw_ref_pic, ry, rx + 8 ) ) / 2];
            mcost += byte_abs[ *orig_line++ - ( ( *ref_line++ ) + Fw_PelY_14( fw_ref_pic, ry, rx + 12 ) ) / 2];
        }
        if ( mcost >= min_mcost )
        {
            break;
        }
    }
    return mcost;
}

/*
*************************************************************************
* Function: FastIntegerPelBlockMotionSearch: fast pixel block motion search
this algrithm is called UMHexagonS which includes
four steps with different kinds of search patterns
* Input:
pel_t**   orig_val,     // <--  original picture
int       ref,          // <--  reference frame (0... or -1 (backward))
int       pic_pix_x,    // <--  absolute x-coordinate of regarded AxB block
int       pic_pix_y,    // <--  absolute y-coordinate of regarded AxB block
int       mode,    // <--  block type (1-16x16 ... 7-4x4)
int       pred_mv_x,    // <--  motion vector predictor (x) in sub-pel units
int       pred_mv_y,    // <--  motion vector predictor (y) in sub-pel units
int*      mv_x,         //  --> motion vector (x) - in pel units
int*      mv_y,         //  --> motion vector (y) - in pel units
int       search_range, // <--  1-d search range in pel units
int       min_mcost,    // <--  minimum motion cost (cost for center or huge value)
double    lambda        // <--  lagrangian parameter for determining motion cost
* Output:
* Return:
* Attention: in this function, three macro definitions is gives,
SEARCH_ONE_PIXEL: search one pixel in search range
SEARCH_ONE_PIXEL1(value_iAbort): search one pixel in search range,
but give a parameter to show if mincost refeshed
*************************************************************************
*/

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
                                   int       min_mcost )   // <--  minimum motion cost (cost for center or huge value)
{
    static int cross_points_x[4] = { -1, 0, 1, 0};
    static int cross_points_y[4] = {0, 1, 0, -1};
    static int Hexagon_x[6] = {2, 1, -1, -2, -1, 1};
    static int Hexagon_y[6] = {0, -2, -2, 0,  2, 2};
    static int Big_Hexagon_x[16] = {0, -2, -4, -4, -4, -4, -4, -2,  0,  2,  4,  4, 4, 4, 4, 2};
    static int Big_Hexagon_y[16] = {4, 3, 2,  1, 0, -1, -2, -3, -4, -3, -2, -1, 0, 1, 2, 3};

    int   pos, cand_x, cand_y,  mcost;
    int   search_range = input->search_range;
    pel_t *( *get_ref_line )( int, pel_t *, int, int );
    pel_t  *ref_pic       = img->type == B_IMG ? Refbuf11 [ref + 1] : Refbuf11[ref];
    int   lambda_factor = LAMBDA_FACTOR( sqrt( img->lambda ) );                // factor for determining lagragian motion cost
    int   mvshift       = 2;                  // motion vector shift for getting sub-pel units
    int   pred_x        = ( pic_pix_x << mvshift ) + pred_mv_x;     // predicted position x (in sub-pel units)
    int   pred_y        = ( pic_pix_y << mvshift ) + pred_mv_y;     // predicted position y (in sub-pel units)
    int   center_x      = pic_pix_x + *mv_x;                        // center position x (in pel units)
    int   center_y      = pic_pix_y + *mv_y;                        // center position y (in pel units)
    int    best_x = 0, best_y = 0;
    int   search_step, iYMinNow, iXMinNow;
    int   i, m, iSADLayer;
    int   iAbort;
    float betaSec, betaThird;

    int   height        = img->height;

    // set function for getting reference picture lines
    if ( ( center_x > search_range ) && ( center_x < img->width - 1 - search_range - ( blc_size[mode][0] << 3 ) ) &&
            ( center_y > search_range ) && ( center_y < height - 1 - search_range - ( blc_size[mode][1] << 3 ) ) )
    {
        get_ref_line = FastLineX;
    }
    else
    {
        get_ref_line = UMVLineX;
    }

    memset( McostState[0], 0, ( 2 * search_range + 1 ) * ( 2 * search_range + 1 ) * 4 );

    if ( img->type == B_IMG && ref > 0 )
    {
        if ( pred_SAD_ref != 0 )
        {
            betaSec = Bsize[mode] / ( pred_SAD_ref*pred_SAD_ref ) - AlphaSec[mode];
            betaThird = Bsize[mode] / ( pred_SAD_ref*pred_SAD_ref ) - AlphaThird[mode];
        }
        else
        {
            betaSec = 0;
            betaThird = 0;
        }
    }
    else
    {
        if ( mode == 1 )
        {
            if ( pred_SAD_space != 0 )
            {
                betaSec = Bsize[mode] / ( pred_SAD_space*pred_SAD_space ) - AlphaSec[mode];
                betaThird = Bsize[mode] / ( pred_SAD_space*pred_SAD_space ) - AlphaThird[mode];
            }
            else
            {
                betaSec = 0;
                betaThird = 0;
            }
        }
        else
        {
            if ( pred_SAD_uplayer != 0 )
            {
                betaSec = Bsize[mode] / ( pred_SAD_uplayer*pred_SAD_uplayer ) - AlphaSec[mode];
                betaThird = Bsize[mode] / ( pred_SAD_uplayer*pred_SAD_uplayer ) - AlphaThird[mode];
            }
            else
            {
                betaSec = 0;
                betaThird = 0;
            }
        }
    }

    // search around the predictor and (0,0)
    // check the center median predictor
    cand_x = center_x ;
    cand_y = center_y ;
    mcost = MV_COST( lambda_factor, mvshift, cand_x, cand_y, pred_x, pred_y );
    if ( ref != -1 )
    {
        mcost += REF_COST( lambda_factor, ref );
    }

    mcost = PartCalMad( ref_pic, orig_val, stride, get_ref_line, mode, mcost, min_mcost, cand_x, cand_y );
    McostState[search_range][search_range] = mcost;
    if ( mcost < min_mcost )
    {
        min_mcost = mcost;
        best_x = cand_x;
        best_y = cand_y;
    }

    iXMinNow = best_x;
    iYMinNow = best_y;
    for ( m = 0; m < 4; m++ )
    {
        cand_x = iXMinNow + cross_points_x[m];
        cand_y = iYMinNow + cross_points_y[m];
        SEARCH_ONE_PIXEL
    }

    if ( center_x != pic_pix_x || center_y != pic_pix_y )
    {
        cand_x = pic_pix_x ;
        cand_y = pic_pix_y ;
        SEARCH_ONE_PIXEL

        iXMinNow = best_x;
        iYMinNow = best_y;
        for ( m = 0; m < 4; m++ )
        {
            cand_x = iXMinNow + cross_points_x[m];
            cand_y = iYMinNow + cross_points_y[m];
            SEARCH_ONE_PIXEL
        }
    }

    if ( mode > 1 )
    {
        cand_x = pic_pix_x + ( pred_MV_uplayer[0] / 4 );
        cand_y = pic_pix_y + ( pred_MV_uplayer[1] / 4 );
        SEARCH_ONE_PIXEL
        if ( ( min_mcost - pred_SAD_uplayer )<pred_SAD_uplayer*betaThird )
        {
            goto third_step;
        }
        else if ( ( min_mcost - pred_SAD_uplayer )<pred_SAD_uplayer*betaSec )
        {
            goto sec_step;
        }
    }

    //coordinate position prediction
    if ( ( img->ip_frm_idx > 1 + ref && ref != -1 ) || ( ref == -1 && Bframe_ctr > 1 ) )
    {
        cand_x = pic_pix_x + pred_MV_time[0] / 4;
        cand_y = pic_pix_y + pred_MV_time[1] / 4;
        SEARCH_ONE_PIXEL
    }

    //prediciton using mV of last ref moiton vector
    if ( ( ref > 0 ) || ( img->type == B_IMG && ref == 0 ) )
    {
        cand_x = pic_pix_x + pred_MV_ref[0] / 4;
        cand_y = pic_pix_y + pred_MV_ref[1] / 4;
        SEARCH_ONE_PIXEL
    }
    //strengthen local search
    iXMinNow = best_x;
    iYMinNow = best_y;
    for ( m = 0; m < 4; m++ )
    {
        cand_x = iXMinNow + cross_points_x[m];
        cand_y = iYMinNow + cross_points_y[m];
        SEARCH_ONE_PIXEL
    }
    EARLY_TERMINATION

    //first_step : Unsymmetrical-cross search
    iXMinNow = best_x;
    iYMinNow = best_y;

    for ( i = 1; i <= search_range / 2; i++ )
    {
        search_step = 2 * i - 1;
        cand_x = iXMinNow + search_step;
        cand_y = iYMinNow ;
        SEARCH_ONE_PIXEL
        cand_x = iXMinNow - search_step;
        cand_y = iYMinNow ;
        SEARCH_ONE_PIXEL
    }

    for ( i = 1; i <= search_range / 4; i++ )
    {
        search_step = 2 * i - 1;
        cand_x = iXMinNow ;
        cand_y = iYMinNow + search_step;
        SEARCH_ONE_PIXEL
        cand_x = iXMinNow ;
        cand_y = iYMinNow - search_step;
        SEARCH_ONE_PIXEL
    }
    EARLY_TERMINATION

    iXMinNow = best_x;
    iYMinNow = best_y;
    // Uneven Multi-Hexagon-grid Search
    for ( pos = 1; pos < 25; pos++ )
    {
        cand_x = iXMinNow + square_points_x[pos];
        cand_y = iYMinNow + square_points_y[pos];
        SEARCH_ONE_PIXEL
    }
    EARLY_TERMINATION

    for ( i = 1; i <= search_range / 4; i++ )
    {
        iAbort = 0;
        for ( m = 0; m < 16; m++ )
        {
            cand_x = iXMinNow + Big_Hexagon_x[m] * i;
            cand_y = iYMinNow + Big_Hexagon_y[m] * i;
            SEARCH_ONE_PIXEL1( 1 )
        }
        if ( iAbort )
        {
            EARLY_TERMINATION
        }
    }

sec_step : //Extended Hexagon-based Search
    iXMinNow = best_x;
    iYMinNow = best_y;
    for ( i = 0; i < search_range; i++ )
    {
        iAbort = 1;
        for ( m = 0; m < 6; m++ )
        {
            cand_x = iXMinNow + Hexagon_x[m];
            cand_y = iYMinNow + Hexagon_y[m];
            SEARCH_ONE_PIXEL1( 0 )
        }
        if ( iAbort )
        {
            break;
        }
        iXMinNow = best_x;
        iYMinNow = best_y;
    }

third_step : //the third step with a small search pattern
    iXMinNow = best_x;
    iYMinNow = best_y;
    for ( i = 0; i < search_range; i++ )
    {
        iSADLayer = 65536;
        iAbort = 1;
        for ( m = 0; m < 4; m++ )
        {
            cand_x = iXMinNow + cross_points_x[m];
            cand_y = iYMinNow + cross_points_y[m];
            SEARCH_ONE_PIXEL1( 0 )
        }
        if ( iAbort )
        {
            break;
        }
        iXMinNow = best_x;
        iYMinNow = best_y;
    }

    *mv_x = best_x - pic_pix_x;
    *mv_y = best_y - pic_pix_y;
    return min_mcost;
}

int FastIntegerMVSearchMhp  ( pel_t*   orig_val,    // <--  not used
        int       stride,
        int       ref,          // <--  reference frame (0... or -1 (backward))
        int       pic_pix_x,    // <--  absolute x-coordinate of regarded AxB block
        int       pic_pix_y,    // <--  absolute y-coordinate of regarded AxB block
        int       mode,    // <--  block type (1-16x16 ... 7-4x4)
        int       pred_mv_x,    // <--  motion vector predictor (x) in sub-pel units
        int       pred_mv_y,    // <--  motion vector predictor (y) in sub-pel units
        int      *mv_x,         //  --> motion vector (x) - in pel units
        int      *mv_y,         //  --> motion vector (y) - in pel units
        int      *fw_mv_x,      //  --> forward motion vector (x) - in pel units
        int      *fw_mv_y,      //  --> forward motion vector (y) - in pel units
        int       min_mcost )   // <--  minimum motion cost (cost for center or huge value)
{
    static int cross_points_x[4] = { -1, 0, 1, 0};
    static int cross_points_y[4] = {0, 1, 0, -1};
    static int Hexagon_x[6] = {2, 1, -1, -2, -1, 1};
    static int Hexagon_y[6] = {0, -2, -2, 0,  2, 2};
    static int Big_Hexagon_x[16] = {0, -2, -4, -4, -4, -4, -4, -2,  0,  2,  4,  4, 4, 4, 4, 2};
    static int Big_Hexagon_y[16] = {4, 3, 2,  1, 0, -1, -2, -3, -4, -3, -2, -1, 0, 1, 2, 3};

    int   pos, cand_x, cand_y,  mcost;
    int   search_range = input->search_range;
    pel_t *( *get_ref_line )( int, pel_t *, int, int );
    pel_t  *ref_pic       = img->type == B_IMG ? Refbuf11 [ref + 1] : Refbuf11[ref];

    pel_t*  fw_ref_pic = mref[0];
    int   lambda_factor = LAMBDA_FACTOR( sqrt( img->lambda ) );                // factor for determining lagragian motion cost
    int   mvshift       = 2;                  // motion vector shift for getting sub-pel units
    int   blocksize_y   = ( blc_size[mode][1] << 3 );          // vertical block size
    int   blocksize_x   = ( blc_size[mode][0] << 3 );          // horizontal block size
    int   pred_x        = ( pic_pix_x << mvshift ) + pred_mv_x;     // predicted position x (in sub-pel units)
    int   pred_y        = ( pic_pix_y << mvshift ) + pred_mv_y;     // predicted position y (in sub-pel units)
    int   center_x      = pic_pix_x + *mv_x;                        // center position x (in pel units)
    int   center_y      = pic_pix_y + *mv_y;                        // center position y (in pel units)
    int   best_x = 0, best_y = 0;
    int   search_step, iYMinNow, iXMinNow;
    int   i, m, iSADLayer;
    int   iAbort;
    float betaSec,betaThird;
    int   pic4_pix_x      = ( pic_pix_x << 2 );
    int   pic4_pix_y      = ( pic_pix_y << 2 );
    int   max_pos_x4      = ( ( img->width - blocksize_x + 1 ) << 2 );
    int   max_pos_y4      = ( ( img->height - blocksize_y + 1 ) << 2 );

    int   height        = img->height;

    // set function for getting reference picture lines
    if ( ( center_x > search_range ) && ( center_x < img->width - 1 - search_range - blocksize_x ) &&
            ( center_y > search_range ) && ( center_y < height - 1 - search_range - blocksize_y ) )
    {
        get_ref_line = FastLineX;
    }
    else
    {
        get_ref_line = UMVLineX;
    }
    if ( ( pic4_pix_x + *fw_mv_x > 1 ) && ( pic4_pix_x + *fw_mv_x < max_pos_x4 - 2 ) &&
            ( pic4_pix_y + *fw_mv_y > 1 ) && ( pic4_pix_y + *fw_mv_y < max_pos_y4 - 2 ) )
    {
        Fw_PelY_14 = FastPelY_14;
    }
    else
    {
        Fw_PelY_14 = UMVPelY_14;
    }

    memset( McostState[0], 0, ( 2 * search_range + 1 ) * ( 2 * search_range + 1 ) * 4 );

    if( ref > 0 )
    {
        if( pred_SAD_ref!=0 )
        {
            betaSec = Bsize[mode]/( pred_SAD_ref*pred_SAD_ref )-AlphaSec[mode];
            betaThird = Bsize[mode]/( pred_SAD_ref*pred_SAD_ref )-AlphaThird[mode];
        }
        else
        {
            betaSec = 0;
            betaThird = 0;
        }
    }
    else
    {
        if( mode==1 )
        {
            if( pred_SAD_space !=0 )
            {
                betaSec = Bsize[mode]/( pred_SAD_space*pred_SAD_space )-AlphaSec[mode];
                betaThird = Bsize[mode]/( pred_SAD_space*pred_SAD_space )-AlphaThird[mode];
            }
            else
            {
                betaSec = 0;
                betaThird = 0;
            }
        }
        else
        {
            if( pred_SAD_uplayer !=0 )
            {
                betaSec = Bsize[mode]/( pred_SAD_uplayer*pred_SAD_uplayer )-AlphaSec[mode];
                betaThird = Bsize[mode]/( pred_SAD_uplayer*pred_SAD_uplayer )-AlphaThird[mode];
            }
            else
            {
                betaSec = 0;
                betaThird = 0;
            }
        }
    }

    // search around the predictor and (0,0)
    // check the center median predictor
    cand_x = center_x ;
    cand_y = center_y ;
    mcost = MV_COST( lambda_factor, mvshift, cand_x, cand_y, pred_x, pred_y );
    if ( ref != -1 )
    {
        mcost += REF_COST( lambda_factor, ref );
    }

    mcost = MHMC_PartCalMad( ref_pic, fw_ref_pic, orig_val, stride, get_ref_line, mode, mcost, min_mcost, cand_x, cand_y, pic_pix_x, pic_pix_y, *fw_mv_x, *fw_mv_y );
    McostState[search_range][search_range] = mcost;
    if ( mcost < min_mcost )
    {
        min_mcost = mcost;
        best_x = cand_x;
        best_y = cand_y;
    }

    iXMinNow = best_x;
    iYMinNow = best_y;
    for ( m = 0; m < 4; m++ )
    {
        cand_x = iXMinNow + cross_points_x[m];
        cand_y = iYMinNow + cross_points_y[m];
        MHMC_SEARCH_ONE_PIXEL
    }

    if ( center_x != pic_pix_x || center_y != pic_pix_y )
    {
        cand_x = pic_pix_x ;
        cand_y = pic_pix_y ;
        MHMC_SEARCH_ONE_PIXEL

        iXMinNow = best_x;
        iYMinNow = best_y;
        for ( m = 0; m < 4; m++ )
        {
            cand_x = iXMinNow + cross_points_x[m];
            cand_y = iYMinNow + cross_points_y[m];
            MHMC_SEARCH_ONE_PIXEL
        }
    }

    if( mode > 1 )
    {
        cand_x = pic_pix_x + ( pred_MV_uplayer[0] / 4 );
        cand_y = pic_pix_y + ( pred_MV_uplayer[1] / 4 );
        MHMC_SEARCH_ONE_PIXEL

        if ( ( min_mcost-pred_SAD_uplayer )<pred_SAD_uplayer*betaThird )
        {
            goto third_step;
        }
        else if( ( min_mcost-pred_SAD_uplayer )<pred_SAD_uplayer*betaSec )
        {
            goto sec_step;
        }
    }

    //coordinate position prediction
    if ( ( img->ip_frm_idx > 1 + ref && ref != -1 ) || ( ref == -1 && Bframe_ctr > 1 ) )
    {
        cand_x = pic_pix_x + pred_MV_time[0] / 4;
        cand_y = pic_pix_y + pred_MV_time[1] / 4;
        MHMC_SEARCH_ONE_PIXEL
    }

    //prediciton using mV of last ref moiton vector
    if ( ( ref > 0 ) || ( img->type == B_IMG && ref == 0 ) )
    {
        cand_x = pic_pix_x + pred_MV_ref[0] / 4;
        cand_y = pic_pix_y + pred_MV_ref[1] / 4;
        MHMC_SEARCH_ONE_PIXEL
    }
    //strengthen local search
    iXMinNow = best_x;
    iYMinNow = best_y;
    for ( m = 0; m < 4; m++ )
    {
        cand_x = iXMinNow + cross_points_x[m];
        cand_y = iYMinNow + cross_points_y[m];
        MHMC_SEARCH_ONE_PIXEL
    }

    EARLY_TERMINATION


    //first_step : Unsymmetrical-cross search
    iXMinNow = best_x;
    iYMinNow = best_y;

    for ( i = 1; i <= search_range / 2; i++ )
    {
        search_step = 2 * i - 1;
        cand_x = iXMinNow + search_step;
        cand_y = iYMinNow ;
        MHMC_SEARCH_ONE_PIXEL
        cand_x = iXMinNow - search_step;
        cand_y = iYMinNow ;
        MHMC_SEARCH_ONE_PIXEL
    }

    for ( i = 1; i <= search_range / 4; i++ )
    {
        search_step = 2 * i - 1;
        cand_x = iXMinNow ;
        cand_y = iYMinNow + search_step;
        MHMC_SEARCH_ONE_PIXEL
        cand_x = iXMinNow ;
        cand_y = iYMinNow - search_step;
        MHMC_SEARCH_ONE_PIXEL
    }
    EARLY_TERMINATION

    iXMinNow = best_x;
    iYMinNow = best_y;
    // Uneven Multi-Hexagon-grid Search
    for ( pos = 1; pos < 25; pos++ )
    {
        cand_x = iXMinNow + square_points_x[pos];
        cand_y = iYMinNow + square_points_y[pos];
        MHMC_SEARCH_ONE_PIXEL
    }
    EARLY_TERMINATION

    for ( i = 1; i <= search_range / 4; i++ )
    {
        iAbort = 0;
        for ( m = 0; m < 16; m++ )
        {
            cand_x = iXMinNow + Big_Hexagon_x[m] * i;
            cand_y = iYMinNow + Big_Hexagon_y[m] * i;
            MHMC_SEARCH_ONE_PIXEL1( 1 )
        }
        if ( iAbort )
        {
            EARLY_TERMINATION
        }
    }
sec_step:  //Extended Hexagon-based Search
    iXMinNow = best_x;
    iYMinNow = best_y;
    for ( i = 0; i < search_range; i++ )
    {
        iAbort = 1;
        for ( m = 0; m < 6; m++ )
        {
            cand_x = iXMinNow + Hexagon_x[m];
            cand_y = iYMinNow + Hexagon_y[m];
            MHMC_SEARCH_ONE_PIXEL1( 0 )
        }
        if ( iAbort )
        {
            break;
        }
        iXMinNow = best_x;
        iYMinNow = best_y;
    }
third_step: // the third step with a small search pattern
    iXMinNow = best_x;
    iYMinNow = best_y;
    for ( i = 0; i < search_range; i++ )
    {
        iSADLayer = 65536;
        iAbort = 1;
        for ( m = 0; m < 4; m++ )
        {
            cand_x = iXMinNow + cross_points_x[m];
            cand_y = iYMinNow + cross_points_y[m];
            MHMC_SEARCH_ONE_PIXEL1( 0 )
        }
        if ( iAbort )
        {
            break;
        }
        iXMinNow = best_x;
        iYMinNow = best_y;
    }

    *mv_x = best_x - pic_pix_x;
    *mv_y = best_y - pic_pix_y;
    return min_mcost;
}

int CalculateSADCost( int pic_pix_x, int pic_pix_y, int mode,
                     int cand_mv_x,int cand_mv_y, pel_t *ref_pic, pel_t* orig_val, int stride, int mv_mcost )
{
    int abort_search, y0, x0, rx0, ry0, ry;
    int  index_pos = 0;
    int   blocksize_x     = ( blc_size[mode][0] << 3 );
    int   blocksize_y     = ( blc_size[mode][1] << 3 );
    pel_t *orig_line;
    int   diff[16], *d;
    int  mcost = mv_mcost;
    int yy, kk, xx;
    int   curr_diff[MB_SIZE][MB_SIZE]; // for ABT SATD calculation

    for ( y0 = 0, abort_search = 0; y0 < blocksize_y && !abort_search; y0 += 4 )
    {
        ry0 = ( ( pic_pix_y + y0 ) << 2 ) + cand_mv_y;

        for ( x0 = 0; x0 < blocksize_x; x0 += 4 )
        {
            rx0 = ( ( pic_pix_x + x0 ) << 2 ) + cand_mv_x;
            d   = diff;

            index_pos = y0 * stride;
            orig_line = orig_val + index_pos;
            ry = ry0;
            *d++      = orig_line[x0  ]  -  PelY_14( ref_pic, ry, rx0 );
            *d++      = orig_line[x0 + 1]  -  PelY_14( ref_pic, ry, rx0 + 4 );
            *d++      = orig_line[x0 + 2]  -  PelY_14( ref_pic, ry, rx0 + 8 );
            *d++      = orig_line[x0 + 3]  -  PelY_14( ref_pic, ry, rx0 + 12 );

            index_pos += stride;
            orig_line = orig_val + index_pos;
            ry = ry0 + 4;
            *d++      = orig_line[x0  ]  -  PelY_14( ref_pic, ry, rx0 );
            *d++      = orig_line[x0 + 1]  -  PelY_14( ref_pic, ry, rx0 + 4 );
            *d++      = orig_line[x0 + 2]  -  PelY_14( ref_pic, ry, rx0 + 8 );
            *d++      = orig_line[x0 + 3]  -  PelY_14( ref_pic, ry, rx0 + 12 );

            index_pos += stride;
            orig_line = orig_val + index_pos;
            ry = ry0 + 8;
            *d++      = orig_line[x0  ]  -  PelY_14( ref_pic, ry, rx0 );
            *d++      = orig_line[x0 + 1]  -  PelY_14( ref_pic, ry, rx0 + 4 );
            *d++      = orig_line[x0 + 2]  -  PelY_14( ref_pic, ry, rx0 + 8 );
            *d++      = orig_line[x0 + 3]  -  PelY_14( ref_pic, ry, rx0 + 12 );

            index_pos += stride;
            orig_line = orig_val + index_pos;
            ry = ry0 + 12;
            *d++      = orig_line[x0  ]  -  PelY_14( ref_pic, ry, rx0 );
            *d++      = orig_line[x0 + 1]  -  PelY_14( ref_pic, ry, rx0 + 4 );
            *d++      = orig_line[x0 + 2]  -  PelY_14( ref_pic, ry, rx0 + 8 );
            *d        = orig_line[x0 + 3]  -  PelY_14( ref_pic, ry, rx0 + 12 );
            for ( yy = y0, kk = 0; yy < y0 + 4; yy++ )
                for ( xx = x0; xx < x0 + 4; xx++, kk++ )
                {
                    curr_diff[yy][xx] = diff[kk];
                }
        }
    }
    mcost += find_sad_8x8( input->hadamard, blocksize_x, blocksize_y, 0, 0, curr_diff );

    return mcost;
}

int CalculateSADCostMhp( int pic_pix_x, int pic_pix_y, int mode,
                         int cand_mv_x,int cand_mv_y, int fw_mv_x, int fw_mv_y, pel_t *ref_pic, pel_t* orig_val, int stride, int mv_mcost )
{
    int abort_search, y0, x0, rx0, ry0, ry;
    int rx1, ry1;
    int index_pos = 0;
    int blocksize_y   = ( blc_size[mode][1] << 3 );
    int blocksize_x   = ( blc_size[mode][0] << 3 );
    pel_t *orig_line;
    int   diff[16], *d;
    int  mcost = mv_mcost;
    int yy, kk, xx;
    int   curr_diff[MB_SIZE][MB_SIZE]; // for ABT SATD calculation

    for ( y0 = 0, abort_search = 0; y0 < blocksize_y && !abort_search; y0 += 4 )
    {
        ry0 = ( ( pic_pix_y + y0 ) << 2 ) + cand_mv_y;
        ry1 = ( ( pic_pix_y + y0 ) << 2 ) + fw_mv_y;

        for ( x0 = 0; x0 < blocksize_x; x0 += 4 )
        {
            rx0 = ( ( pic_pix_x + x0 ) << 2 ) + cand_mv_x;
            rx1 = ( ( pic_pix_x + x0 ) << 2 ) + fw_mv_x;
            d   = diff;

            index_pos = y0 * stride;
            orig_line = orig_val + index_pos;
            ry = ry0;
            *d++      = orig_line[x0  ]  - ( PelY_14( ref_pic, ry, rx0 ) + Fw_PelY_14( ref_pic, ry1, rx1 ) ) / 2;
            *d++      = orig_line[x0 + 1]  - ( PelY_14( ref_pic, ry, rx0 + 4 ) + Fw_PelY_14( ref_pic, ry1, rx1 + 4 ) ) / 2;
            *d++      = orig_line[x0 + 2]  - ( PelY_14( ref_pic, ry, rx0 + 8 ) + Fw_PelY_14( ref_pic, ry1, rx1 + 8 ) ) / 2;
            *d++      = orig_line[x0 + 3]  - ( PelY_14( ref_pic, ry, rx0 + 12 ) + Fw_PelY_14( ref_pic, ry1, rx1 + 12 ) ) / 2;

            index_pos += stride;
            orig_line = orig_val + index_pos;
            ry = ry0 + 4;
            *d++      = orig_line[x0  ]  - ( PelY_14( ref_pic, ry, rx0 ) + Fw_PelY_14( ref_pic, ry1 + 4, rx1 ) ) / 2;
            *d++      = orig_line[x0 + 1]  - ( PelY_14( ref_pic, ry, rx0 + 4 ) + Fw_PelY_14( ref_pic, ry1 + 4, rx1 + 4 ) ) / 2;
            *d++      = orig_line[x0 + 2]  - ( PelY_14( ref_pic, ry, rx0 + 8 ) + Fw_PelY_14( ref_pic, ry1 + 4, rx1 + 8 ) ) / 2;
            *d++      = orig_line[x0 + 3]  - ( PelY_14( ref_pic, ry, rx0 + 12 ) + Fw_PelY_14( ref_pic, ry1 + 4, rx1 + 12 ) ) / 2;

            index_pos += stride;
            orig_line = orig_val + index_pos;
            ry = ry0 + 8;
            *d++      = orig_line[x0  ]  - ( PelY_14( ref_pic, ry, rx0 ) + Fw_PelY_14( ref_pic, ry1 + 8, rx1 ) ) / 2;
            *d++      = orig_line[x0 + 1]  - ( PelY_14( ref_pic, ry, rx0 + 4 ) + Fw_PelY_14( ref_pic, ry1 + 8, rx1 + 4 ) ) / 2;
            *d++      = orig_line[x0 + 2]  - ( PelY_14( ref_pic, ry, rx0 + 8 ) + Fw_PelY_14( ref_pic, ry1 + 8, rx1 + 8 ) ) / 2;
            *d++      = orig_line[x0 + 3]  - ( PelY_14( ref_pic, ry, rx0 + 12 ) + Fw_PelY_14( ref_pic, ry1 + 8, rx1 + 12 ) ) / 2;

            index_pos += stride;
            orig_line = orig_val + index_pos;
            ry = ry0 + 12;
            *d++      = orig_line[x0  ]  - ( PelY_14( ref_pic, ry, rx0 ) + Fw_PelY_14( ref_pic, ry1 + 12, rx1 ) ) / 2;
            *d++      = orig_line[x0 + 1]  - ( PelY_14( ref_pic, ry, rx0 + 4 ) + Fw_PelY_14( ref_pic, ry1 + 12, rx1 + 4 ) ) / 2;
            *d++      = orig_line[x0 + 2]  - ( PelY_14( ref_pic, ry, rx0 + 8 ) + Fw_PelY_14( ref_pic, ry1 + 12, rx1 + 8 ) ) / 2;
            *d++      = orig_line[x0 + 3]  - ( PelY_14( ref_pic, ry, rx0 + 12 ) + Fw_PelY_14( ref_pic, ry1 + 12, rx1 + 12 ) ) / 2;
            for ( yy = y0, kk = 0; yy < y0 + 4; yy++ )
                for ( xx = x0; xx < x0 + 4; xx++, kk++ )
                {
                    curr_diff[yy][xx] = diff[kk];
                }
        }
    }
    mcost += find_sad_8x8( input->hadamard, blocksize_x, blocksize_y, 0, 0, curr_diff );

    return mcost;
}

int CrossFractionalMVSearchSAD ( pel_t*   orig_val,     // <--  original pixel values for the AxB block
                                  int       stride,
                                  int       ref,           // <--  reference frame (0... or -1 (backward))
                                  int       pic_pix_x,     // <--  absolute x-coordinate of regarded AxB block
                                  int       pic_pix_y,     // <--  absolute y-coordinate of regarded AxB block
                                  int       mode,          // <--  block type (1-16x16 ... 7-4x4)
                                  int       pred_mv_x,     // <--  motion vector predictor (x) in sub-pel units
                                  int       pred_mv_y,     // <--  motion vector predictor (y) in sub-pel units
                                  int      *mv_x,          // <--> in: search center (x) / out: motion vector (x) - in pel units
                                  int      *mv_y,          // <--> in: search center (y) / out: motion vector (y) - in pel units
                                  int       min_mcost )    // <--  minimum motion cost (cost for center or huge value)
{
    const int cross_points_x[4] = { -1, 0, 1, 0};
    const int cross_points_y[4] = {0, 1, 0, -1};
    int   mcost;
    int   cand_mv_x, cand_mv_y;
    pel_t *ref_pic = img->type == B_IMG ? mref[ref + 1] : mref[ref];

    int   lambda_factor   = LAMBDA_FACTOR( sqrt( img->lambda ) );
    int   mv_shift        = 0;
    int   blocksize_x     = ( blc_size[mode][0] << 3 );
    int   blocksize_y     = ( blc_size[mode][1] << 3 );
    int   pic4_pix_x      = ( pic_pix_x << 2 );
    int   pic4_pix_y      = ( pic_pix_y << 2 );
    int   max_pos_x4      = ( ( img->width - blocksize_x + 1 ) << 2 );
    int   max_pos_y4      = ( ( img->height - blocksize_y + 1 ) << 2 );

    int   search_range_dynamic, iXMinNow, iYMinNow, i;
    int   iSADLayer, m;
    int   currmv_x = 0, currmv_y = 0, iCurrSearchRange;
    int   pred_frac_mv_x, pred_frac_mv_y, abort_search;
    int   mv_cost;

    ref_pic       = img->type == B_IMG ? mref [ref + 1] : mref [ref];

    *mv_x <<= 2;
    *mv_y <<= 2;
    if ( ( pic4_pix_x + *mv_x > 1 ) && ( pic4_pix_x + *mv_x < max_pos_x4 - 2 ) &&
            ( pic4_pix_y + *mv_y > 1 ) && ( pic4_pix_y + *mv_y < max_pos_y4 - 2 ) )
    {
        PelY_14 = FastPelY_14;
    }
    else
    {
        PelY_14 = UMVPelY_14;
    }

    search_range_dynamic = 3;
    pred_frac_mv_x = ( pred_mv_x - *mv_x ) % 4;
    pred_frac_mv_y = ( pred_mv_y - *mv_y ) % 4;

    memset( SearchState[0], 0, ( 2 * search_range_dynamic + 1 ) * ( 2 * search_range_dynamic + 1 ) );

    if ( input->hadamard )
    {
        cand_mv_x = *mv_x;
        cand_mv_y = *mv_y;
        mv_cost = MV_COST( lambda_factor, mv_shift, cand_mv_x, cand_mv_y, pred_mv_x, pred_mv_y );
        if ( ref != -1 )
        {
            mv_cost += REF_COST( lambda_factor, ref );
        }
        mcost = CalculateSADCost( pic_pix_x, pic_pix_y, mode, cand_mv_x, cand_mv_y, ref_pic, orig_val, stride, mv_cost );
        SearchState[search_range_dynamic][search_range_dynamic] = 1;

        if ( mcost < min_mcost )
        {
            min_mcost = mcost;
            currmv_x = cand_mv_x;
            currmv_y = cand_mv_y;
        }
    }
    else
    {
        SearchState[search_range_dynamic][search_range_dynamic] = 1;
        currmv_x = *mv_x;
        currmv_y = *mv_y;
    }

    if ( pred_frac_mv_x != 0 || pred_frac_mv_y != 0 )
    {
        cand_mv_x = *mv_x + pred_frac_mv_x;
        cand_mv_y = *mv_y + pred_frac_mv_y;
        mv_cost = MV_COST( lambda_factor, mv_shift, cand_mv_x, cand_mv_y, pred_mv_x, pred_mv_y );
        if ( ref != -1 )
        {
            mv_cost += REF_COST( lambda_factor, ref );
        }
        mcost = CalculateSADCost( pic_pix_x, pic_pix_y, mode, cand_mv_x, cand_mv_y, ref_pic, orig_val, stride, mv_cost );
        SearchState[cand_mv_y - *mv_y + search_range_dynamic][cand_mv_x - *mv_x + search_range_dynamic] = 1;

        if ( mcost < min_mcost )
        {
            min_mcost = mcost;
            currmv_x = cand_mv_x;
            currmv_y = cand_mv_y;
        }
    }


    iXMinNow = currmv_x;
    iYMinNow = currmv_y;
    iCurrSearchRange = 2 * search_range_dynamic + 1;
    for ( i = 0; i < iCurrSearchRange; i++ )
    {
        abort_search = 1;
        iSADLayer = 65536;
        for ( m = 0; m < 4; m++ )
        {
            cand_mv_x = iXMinNow + cross_points_x[m];
            cand_mv_y = iYMinNow + cross_points_y[m];

            img->mv_range_flag = check_mv_range( cand_mv_x, cand_mv_y, pic_pix_x, pic_pix_y, mode );
            if ( !img->mv_range_flag )
            {
                img->mv_range_flag = 1;
                continue;
            }

            if ( abs( cand_mv_x - *mv_x ) <= search_range_dynamic && abs( cand_mv_y - *mv_y ) <= search_range_dynamic )
            {
                if ( !SearchState[cand_mv_y - *mv_y + search_range_dynamic][cand_mv_x - *mv_x + search_range_dynamic] )
                {
                    mv_cost = MV_COST( lambda_factor, mv_shift, cand_mv_x, cand_mv_y, pred_mv_x, pred_mv_y );
                    if ( ref != -1 )
                    {
                        mv_cost += REF_COST( lambda_factor, ref );
                    }
                    mcost = CalculateSADCost( pic_pix_x, pic_pix_y, mode, cand_mv_x, cand_mv_y, ref_pic, orig_val, stride, mv_cost );
                    SearchState[cand_mv_y - *mv_y + search_range_dynamic][cand_mv_x - *mv_x + search_range_dynamic] = 1;
                    if ( mcost < min_mcost )
                    {
                        min_mcost = mcost;
                        currmv_x = cand_mv_x;
                        currmv_y = cand_mv_y;
                        abort_search = 0;

                    }
                }
            }
        }
        iXMinNow = currmv_x;
        iYMinNow = currmv_y;
        if ( abort_search )
        {
            break;
        }
    }

    *mv_x = currmv_x;
    *mv_y = currmv_y;

    //===== return minimum motion cost =====
    return min_mcost;
}

int CrossFractionalMVSearchSADMhp ( pel_t*   orig_val,     // <--  original pixel values for the AxB block
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
                                      int       min_mcost )    // <--  minimum motion cost (cost for center or huge value)
{
    const int cross_points_x[4] = { -1, 0, 1, 0 };
    const int cross_points_y[4] = { 0, 1, 0, -1 };
    int   mcost;
    int   cand_mv_x, cand_mv_y;
    pel_t *ref_pic = img->type == B_IMG ? mref[ref + 1] : mref[ref];

    int   lambda_factor   = LAMBDA_FACTOR( sqrt( img->lambda ) );
    int   mv_shift        = 0;
    int   pic4_pix_x      = ( pic_pix_x << 2 );
    int   pic4_pix_y      = ( pic_pix_y << 2 );
    int   max_pos_x4      = ( ( img->width  - ( blc_size[mode][0] << 3 ) + 1 ) << 2 );
    int   max_pos_y4      = ( ( img->height - ( blc_size[mode][1] << 3 ) + 1 ) << 2 );

    int   search_range_dynamic, iXMinNow, iYMinNow, i;
    int   iSADLayer, m;
    int   currmv_x = 0, currmv_y = 0, iCurrSearchRange;
    int   pred_frac_mv_x, pred_frac_mv_y, abort_search;
    int   mv_cost;

    ref_pic       = img->type == B_IMG ? mref [ref + 1] : mref [ref];

    *mv_x <<= 2;
    *mv_y <<= 2;
    if ( ( pic4_pix_x + *mv_x > 1 ) && ( pic4_pix_x + *mv_x < max_pos_x4 - 2 ) &&
            ( pic4_pix_y + *mv_y > 1 ) && ( pic4_pix_y + *mv_y < max_pos_y4 - 2 ) )
    {
        PelY_14 = FastPelY_14;
    }
    else
    {
        PelY_14 = UMVPelY_14;
    }
    if ( ( pic4_pix_x + *fw_mv_x > 1 ) && ( pic4_pix_x + *fw_mv_x < max_pos_x4 - 2 ) &&
            ( pic4_pix_y + *fw_mv_y > 1 ) && ( pic4_pix_y + *fw_mv_y < max_pos_y4 - 2 ) )
    {
        Fw_PelY_14 = FastPelY_14;
    }
    else
    {
        Fw_PelY_14 = UMVPelY_14;
    }

    search_range_dynamic = 3;
    pred_frac_mv_x = ( pred_mv_x - *mv_x ) % 4;
    pred_frac_mv_y = ( pred_mv_y - *mv_y ) % 4;

    memset( SearchState[0], 0, ( 2 * search_range_dynamic + 1 ) * ( 2 * search_range_dynamic + 1 ) );

    if ( input->hadamard )
    {
        cand_mv_x = *mv_x;
        cand_mv_y = *mv_y;
        mv_cost = MV_COST( lambda_factor, mv_shift, cand_mv_x, cand_mv_y, pred_mv_x, pred_mv_y );
        if ( ref != -1 )
        {
            mv_cost += REF_COST( lambda_factor, ref );
        }

        mcost = CalculateSADCostMhp( pic_pix_x, pic_pix_y, mode, cand_mv_x, cand_mv_y, *fw_mv_x, *fw_mv_y, ref_pic, orig_val, stride, mv_cost );
        SearchState[search_range_dynamic][search_range_dynamic] = 1;

        if ( mcost < min_mcost )
        {
            min_mcost = mcost;
            currmv_x = cand_mv_x;
            currmv_y = cand_mv_y;
        }
    }
    else
    {
        SearchState[search_range_dynamic][search_range_dynamic] = 1;
        currmv_x = *mv_x;
        currmv_y = *mv_y;
    }

    if ( pred_frac_mv_x != 0 || pred_frac_mv_y != 0 )
    {
        cand_mv_x = *mv_x + pred_frac_mv_x;
        cand_mv_y = *mv_y + pred_frac_mv_y;
        mv_cost = MV_COST( lambda_factor, mv_shift, cand_mv_x, cand_mv_y, pred_mv_x, pred_mv_y );
        if ( ref != -1 )
        {
            mv_cost += REF_COST( lambda_factor, ref );
        }
        mcost = CalculateSADCostMhp( pic_pix_x, pic_pix_y, mode, cand_mv_x, cand_mv_y, *fw_mv_x, *fw_mv_y, ref_pic, orig_val, stride, mv_cost );
        SearchState[cand_mv_y - *mv_y + search_range_dynamic][cand_mv_x - *mv_x + search_range_dynamic] = 1;

        if ( mcost < min_mcost )
        {
            min_mcost = mcost;
            currmv_x = cand_mv_x;
            currmv_y = cand_mv_y;
        }
    }


    iXMinNow = currmv_x;
    iYMinNow = currmv_y;
    iCurrSearchRange = 2 * search_range_dynamic + 1;
    for ( i = 0; i < iCurrSearchRange; i++ )
    {
        abort_search = 1;
        iSADLayer = 65536;
        for ( m = 0; m < 4; m++ )
        {
            cand_mv_x = iXMinNow + cross_points_x[m];
            cand_mv_y = iYMinNow + cross_points_y[m];

            img->mv_range_flag = check_mv_range( cand_mv_x, cand_mv_y, pic_pix_x, pic_pix_y, mode );
            if ( !img->mv_range_flag )
            {
                img->mv_range_flag = 1;
                continue;
            }

            if ( abs( cand_mv_x - *mv_x ) <= search_range_dynamic && abs( cand_mv_y - *mv_y ) <= search_range_dynamic )
            {
                if ( !SearchState[cand_mv_y - *mv_y + search_range_dynamic][cand_mv_x - *mv_x + search_range_dynamic] )
                {
                    mv_cost = MV_COST( lambda_factor, mv_shift, cand_mv_x, cand_mv_y, pred_mv_x, pred_mv_y );
                    if ( ref != -1 )
                    {
                        mv_cost += REF_COST( lambda_factor, ref );
                    }
                    mcost = CalculateSADCostMhp( pic_pix_x, pic_pix_y, mode, cand_mv_x, cand_mv_y, *fw_mv_x, *fw_mv_y, ref_pic, orig_val, stride, mv_cost );
                    SearchState[cand_mv_y - *mv_y + search_range_dynamic][cand_mv_x - *mv_x + search_range_dynamic] = 1;
                    if ( mcost < min_mcost )
                    {
                        min_mcost = mcost;
                        currmv_x = cand_mv_x;
                        currmv_y = cand_mv_y;
                        abort_search = 0;

                    }
                }
            }
        }
        iXMinNow = currmv_x;
        iYMinNow = currmv_y;
        if ( abort_search )
        {
            break;
        }
    }

    *mv_x = currmv_x;
    *mv_y = currmv_y;

    //===== return minimum motion cost =====
    return min_mcost;
}


int                                               //  ==> minimum motion cost after search
SquareFractionalMVSearchSATD ( pel_t*   orig_val,     // <--  original pixel values for the AxB block
                          int       stride,
                          int       ref,           // <--  reference frame (0... or -1 (backward))
                          int       pic_pix_x,     // <--  absolute x-coordinate of regarded AxB block
                          int       pic_pix_y,     // <--  absolute y-coordinate of regarded AxB block
                          int       mode,     // <--  block type (1-16x16 ... 7-4x4)
                          int       pred_mv_x,     // <--  motion vector predictor (x) in sub-pel units
                          int       pred_mv_y,     // <--  motion vector predictor (y) in sub-pel units
                          int      *mv_x,          // <--> in: search center (x) / out: motion vector (x) - in pel units
                          int      *mv_y,          // <--> in: search center (y) / out: motion vector (y) - in pel units
                          int       search_pos2,   // <--  search positions for    half-pel search  (default: 9)
                          int       search_pos4,   // <--  search positions for quarter-pel search  (default: 9)
                          int       min_mcost )    // <--  minimum motion cost (cost for center or huge value)
{
    int   diff[16], *d;
    int   pos, best_pos, mcost, abort_search;
    int   y0, x0, ry0, rx0, ry;
    int   cand_mv_x, cand_mv_y;
    int   index_pos = 0;
    pel_t *orig_line;
    int incr = ( img->type == B_IMG );
    pel_t *ref_pic;
    int   lambda_factor = LAMBDA_FACTOR( sqrt( img->lambda ) );
    int   mv_shift      = 0;
    int   blocksize_x   = blc_size[mode][0] << 3;
    int   blocksize_y   = blc_size[mode][1] << 3;
    int   pic4_pix_x    = ( pic_pix_x << 2 );
    int   pic4_pix_y    = ( pic_pix_y << 2 );
    int   max_pos_x4    = ( ( img->width - blocksize_x + 1 ) << 2 );
    int   max_pos_y4    = ( ( img->height - blocksize_y + 1 ) << 2 );
    int   min_pos2      = ( input->hadamard ? 0 : 1 );
    int   max_pos2      = ( input->hadamard ? MAX( 1, search_pos2 ) : search_pos2 );

    int   curr_diff[MB_SIZE][MB_SIZE]; // for IVC 8x8 SATD calculation
    int   xx, yy, kk;                              // indicees for curr_diff

    ref_pic = img->type == B_IMG ? mref [ref + incr] : mref [ref];


    /* HALF-PEL REFINEMENT  */
    // convert search center to quarter-pel units
    *mv_x <<= 2;
    *mv_y <<= 2;
    //===== set function for getting pixel values =====
    if ( ( pic4_pix_x + *mv_x > 1 ) && ( pic4_pix_x + *mv_x < max_pos_x4 - 2 ) &&
            ( pic4_pix_y + *mv_y > 1 ) && ( pic4_pix_y + *mv_y < max_pos_y4 - 2 ) )
    {
        PelY_14 = FastPelY_14;
    }
    else
    {
        PelY_14 = UMVPelY_14;
    }
    // loop over search positions
    for ( best_pos = 0, pos = min_pos2; pos < max_pos2; pos++ )
    {
        cand_mv_x = *mv_x + ( square_points_x[pos] << 1 );  // quarter-pel units
        cand_mv_y = *mv_y + ( square_points_y[pos] << 1 );  // quarter-pel units

        // set motion vector cost
        mcost = MV_COST( lambda_factor, mv_shift, cand_mv_x, cand_mv_y, pred_mv_x, pred_mv_y );
        if ( ref != -1 )
        {
            mcost += REF_COST( lambda_factor, ref );
        }

        // add up SATD
        for ( y0 = 0, abort_search = 0; y0 < blocksize_y && !abort_search; y0 += 4 )
        {
            ry0 = ( ( pic_pix_y + y0 ) << 2 ) + cand_mv_y;

            for ( x0 = 0; x0 < blocksize_x; x0 += 4 )
            {
                rx0 = ( ( pic_pix_x + x0 ) << 2 ) + cand_mv_x;
                d   = diff;

                index_pos = y0 * stride;
                orig_line = orig_val + index_pos;
                ry = ry0;
                *d++      = orig_line[x0  ]  -  PelY_14( ref_pic, ry, rx0 );
                *d++      = orig_line[x0 + 1]  -  PelY_14( ref_pic, ry, rx0 + 4 );
                *d++      = orig_line[x0 + 2]  -  PelY_14( ref_pic, ry, rx0 + 8 );
                *d++      = orig_line[x0 + 3]  -  PelY_14( ref_pic, ry, rx0 + 12 );

                index_pos += stride;
                orig_line = orig_val + index_pos;
                ry = ry0 + 4;
                *d++      = orig_line[x0  ]  -  PelY_14( ref_pic, ry, rx0 );
                *d++      = orig_line[x0 + 1]  -  PelY_14( ref_pic, ry, rx0 + 4 );
                *d++      = orig_line[x0 + 2]  -  PelY_14( ref_pic, ry, rx0 + 8 );
                *d++      = orig_line[x0 + 3]  -  PelY_14( ref_pic, ry, rx0 + 12 );

                index_pos += stride;
                orig_line = orig_val + index_pos;
                ry = ry0 + 8;
                *d++      = orig_line[x0  ]  -  PelY_14( ref_pic, ry, rx0 );
                *d++      = orig_line[x0 + 1]  -  PelY_14( ref_pic, ry, rx0 + 4 );
                *d++      = orig_line[x0 + 2]  -  PelY_14( ref_pic, ry, rx0 + 8 );
                *d++      = orig_line[x0 + 3]  -  PelY_14( ref_pic, ry, rx0 + 12 );

                index_pos += stride;
                orig_line = orig_val + index_pos;
                ry = ry0 + 12;
                *d++      = orig_line[x0  ]  -  PelY_14( ref_pic, ry, rx0 );
                *d++      = orig_line[x0 + 1]  -  PelY_14( ref_pic, ry, rx0 + 4 );
                *d++      = orig_line[x0 + 2]  -  PelY_14( ref_pic, ry, rx0 + 8 );
                *d        = orig_line[x0 + 3]  -  PelY_14( ref_pic, ry, rx0 + 12 );

                for ( yy = y0, kk = 0; yy < y0 + 4; yy++ )
                    for ( xx = x0; xx < x0 + 4; xx++, kk++ )
                    {
                        curr_diff[yy][xx] = diff[kk];
                    }
            }
        }

        mcost += find_sad_8x8( input->hadamard, blocksize_x, blocksize_y, 0, 0, curr_diff );
        if ( mcost < min_mcost )
        {
            min_mcost = mcost;
            best_pos  = pos;
        }
    }

    if ( best_pos )
    {
        *mv_x += ( square_points_x [best_pos] << 1 );
        *mv_y += ( square_points_y [best_pos] << 1 );
    }

    /* QUARTER-PEL REFINEMENT */
    // set function for getting pixel values
    if ( ( pic4_pix_x + *mv_x > 1 ) && ( pic4_pix_x + *mv_x < max_pos_x4 - 1 ) &&
            ( pic4_pix_y + *mv_y > 1 ) && ( pic4_pix_y + *mv_y < max_pos_y4 - 1 ) )
    {
        PelY_14 = FastPelY_14;
    }
    else
    {
        PelY_14 = UMVPelY_14;
    }

    // loop over search positions
    for ( best_pos = 0, pos = 1; pos < search_pos4; pos++ )
    {
        cand_mv_x = *mv_x + square_points_x[pos];    // quarter-pel units
        cand_mv_y = *mv_y + square_points_y[pos];    // quarter-pel units

        img->mv_range_flag = check_mv_range( cand_mv_x, cand_mv_y, pic_pix_x, pic_pix_y, mode );
        if ( !img->mv_range_flag )
        {
            img->mv_range_flag = 1;
            continue;
        }

        // set motion vector cost
        mcost = MV_COST( lambda_factor, mv_shift, cand_mv_x, cand_mv_y, pred_mv_x, pred_mv_y );
        if ( ref != -1 )
        {
            mcost += REF_COST( lambda_factor, ref );
        }
        // add up SATD
        for ( y0 = 0, abort_search = 0; y0 < blocksize_y && !abort_search; y0 += 4 )
        {
            ry0 = ( ( pic_pix_y + y0 ) << 2 ) + cand_mv_y;

            for ( x0 = 0; x0 < blocksize_x; x0 += 4 )
            {
                rx0 = ( ( pic_pix_x + x0 ) << 2 ) + cand_mv_x;
                d   = diff;

                index_pos = y0 * stride;
                orig_line = orig_val + index_pos;
                ry = ry0;
                *d++      = orig_line[x0  ]  -  PelY_14( ref_pic, ry, rx0 );
                *d++      = orig_line[x0 + 1]  -  PelY_14( ref_pic, ry, rx0 + 4 );
                *d++      = orig_line[x0 + 2]  -  PelY_14( ref_pic, ry, rx0 + 8 );
                *d++      = orig_line[x0 + 3]  -  PelY_14( ref_pic, ry, rx0 + 12 );

                index_pos += stride;
                orig_line = orig_val + index_pos;
                ry = ry0 + 4;
                *d++      = orig_line[x0  ]  -  PelY_14( ref_pic, ry, rx0 );
                *d++      = orig_line[x0 + 1]  -  PelY_14( ref_pic, ry, rx0 + 4 );
                *d++      = orig_line[x0 + 2]  -  PelY_14( ref_pic, ry, rx0 + 8 );
                *d++      = orig_line[x0 + 3]  -  PelY_14( ref_pic, ry, rx0 + 12 );

                index_pos += stride;
                orig_line = orig_val + index_pos;
                ry = ry0 + 8;
                *d++      = orig_line[x0  ]  -  PelY_14( ref_pic, ry, rx0 );
                *d++      = orig_line[x0 + 1]  -  PelY_14( ref_pic, ry, rx0 + 4 );
                *d++      = orig_line[x0 + 2]  -  PelY_14( ref_pic, ry, rx0 + 8 );
                *d++      = orig_line[x0 + 3]  -  PelY_14( ref_pic, ry, rx0 + 12 );

                index_pos += stride;
                orig_line = orig_val + index_pos;
                ry = ry0 + 12;
                *d++      = orig_line[x0  ]  -  PelY_14( ref_pic, ry, rx0 );
                *d++      = orig_line[x0 + 1]  -  PelY_14( ref_pic, ry, rx0 + 4 );
                *d++      = orig_line[x0 + 2]  -  PelY_14( ref_pic, ry, rx0 + 8 );
                *d        = orig_line[x0 + 3]  -  PelY_14( ref_pic, ry, rx0 + 12 );

                for ( yy = y0, kk = 0; yy < y0 + 4; yy++ )
                    for ( xx = x0; xx < x0 + 4; xx++, kk++ )
                    {
                        curr_diff[yy][xx] = diff[kk];
                    }
            }
        }

        mcost += find_sad_8x8( input->hadamard, blocksize_x, blocksize_y, 0, 0, curr_diff );
        if ( mcost < min_mcost )
        {
            min_mcost = mcost;
            best_pos  = pos;
        }
    }

    if ( best_pos )
    {
        *mv_x += square_points_x [best_pos];
        *mv_y += square_points_y [best_pos];
    }

    // return minimum motion cost
    return min_mcost;
}

int                                               //  ==> minimum motion cost after search
SquareFractionalMVSearchSATDMhp ( pel_t*   orig_val,     // <--  original pixel values for the AxB block
                              int       stride,
                              int       ref,           // <--  reference frame (0... or -1 (backward))
                              int       pic_pix_x,     // <--  absolute x-coordinate of regarded AxB block
                              int       pic_pix_y,     // <--  absolute y-coordinate of regarded AxB block
                              int       mode,     // <--  block type (1-16x16 ... 7-4x4)
                              int       pred_mv_x,     // <--  motion vector predictor (x) in sub-pel units
                              int       pred_mv_y,     // <--  motion vector predictor (y) in sub-pel units
                              int      *mv_x,          // <--> in: search center (x) / out: motion vector (x) - in pel units
                              int      *mv_y,          // <--> in: search center (y) / out: motion vector (y) - in pel units
                              int      *fw_mv_x,         //  --> forward motion vector (x) - in pel units
                              int      *fw_mv_y,         //  --> forward motion vector (y) - in pel units
                              int       search_pos2,   // <--  search positions for    half-pel search  (default: 9)
                              int       search_pos4,   // <--  search positions for quarter-pel search  (default: 9)
                              int       min_mcost )    // <--  minimum motion cost (cost for center or huge value)
{
    int   diff[16], *d;
    int   pos, best_pos, mcost, abort_search;
    int   y0, x0, ry0, rx0, ry;
    int   ry1, rx1;
    int   index_pos = 0;
    int   cand_mv_x, cand_mv_y;
    pel_t *orig_line;
    int incr = ( img->type == B_IMG );
    pel_t *ref_pic;
    int   lambda_factor   = LAMBDA_FACTOR( sqrt( img->lambda ) );
    int   mv_shift        = 0;
    int   blocksize_x     = ( blc_size[mode][0] << 3 );
    int   blocksize_y     = ( blc_size[mode][1] << 3 );
    int   pic4_pix_x      = ( pic_pix_x << 2 );
    int   pic4_pix_y      = ( pic_pix_y << 2 );
    int   max_pos_x4      = ( ( img->width - blocksize_x + 1 ) << 2 );
    int   max_pos_y4      = ( ( img->height - blocksize_y + 1 ) << 2 );
    int   min_pos2        = ( input->hadamard ? 0 : 1 );
    int   max_pos2        = ( input->hadamard ? MAX( 1, search_pos2 ) : search_pos2 );

    int   curr_diff[MB_SIZE][MB_SIZE]; // for IVC 8x8 SATD calculation
    int   xx, yy, kk;                              // indicees for curr_diff


    ref_pic = img->type == B_IMG ? mref [ref + incr] : mref [ref];


    /* HALF-PEL REFINEMENT */
    // convert search center to quarter-pel units
    *mv_x <<= 2;
    *mv_y <<= 2;
    // set function for getting pixel values
    if ( ( pic4_pix_x + *mv_x > 1 ) && ( pic4_pix_x + *mv_x < max_pos_x4 - 2 ) &&
            ( pic4_pix_y + *mv_y > 1 ) && ( pic4_pix_y + *mv_y < max_pos_y4 - 2 ) )
    {
        PelY_14 = FastPelY_14;
    }
    else
    {
        PelY_14 = UMVPelY_14;
    }
    if ( ( pic4_pix_x + *fw_mv_x > 1 ) && ( pic4_pix_x + *fw_mv_x < max_pos_x4 - 2 ) &&
            ( pic4_pix_y + *fw_mv_y > 1 ) && ( pic4_pix_y + *fw_mv_y < max_pos_y4 - 2 ) )
    {
        Fw_PelY_14 = FastPelY_14;
    }
    else
    {
        Fw_PelY_14 = UMVPelY_14;
    }

    //===== loop over search positions =====
    for ( best_pos = 0, pos = min_pos2; pos < max_pos2; pos++ )
    {
        cand_mv_x = *mv_x + ( square_points_x[pos] << 1 );  // quarter-pel units
        cand_mv_y = *mv_y + ( square_points_y[pos] << 1 );  // quarter-pel units

        //----- set motion vector cost -----
        mcost = MV_COST( lambda_factor, mv_shift, cand_mv_x, cand_mv_y, pred_mv_x, pred_mv_y );
        if ( ref != -1 )
        {
            mcost += REF_COST( lambda_factor, ref );
        }

        //----- add up SATD -----
        for ( y0 = 0, abort_search = 0; y0 < blocksize_y && !abort_search; y0 += 4 )
        {
            ry0 = ( ( pic_pix_y + y0 ) << 2 ) + cand_mv_y;
            ry1 = ( ( pic_pix_y + y0 ) << 2 ) + *fw_mv_y;

            for ( x0 = 0; x0 < blocksize_x; x0 += 4 )
            {
                rx0 = ( ( pic_pix_x + x0 ) << 2 ) + cand_mv_x;
                rx1 = ( ( pic_pix_x + x0 ) << 2 ) + *fw_mv_x;
                d   = diff;

                index_pos = y0 * stride;
                orig_line = orig_val + index_pos;
                ry = ry0;
                *d++      = orig_line[x0  ]  - ( PelY_14( ref_pic, ry, rx0 ) + Fw_PelY_14( ref_pic, ry1, rx1 ) ) / 2;
                *d++      = orig_line[x0 + 1]  - ( PelY_14( ref_pic, ry, rx0 + 4 ) + Fw_PelY_14( ref_pic, ry1, rx1 + 4 ) ) / 2;
                *d++      = orig_line[x0 + 2]  - ( PelY_14( ref_pic, ry, rx0 + 8 ) + Fw_PelY_14( ref_pic, ry1, rx1 + 8 ) ) / 2;
                *d++      = orig_line[x0 + 3]  - ( PelY_14( ref_pic, ry, rx0 + 12 ) + Fw_PelY_14( ref_pic, ry1, rx1 + 12 ) ) / 2;

                index_pos += stride;
                orig_line = orig_val + index_pos;
                ry = ry0 + 4;
                *d++      = orig_line[x0  ]  - ( PelY_14( ref_pic, ry, rx0 ) + Fw_PelY_14( ref_pic, ry1 + 4, rx1 ) ) / 2;
                *d++      = orig_line[x0 + 1]  - ( PelY_14( ref_pic, ry, rx0 + 4 ) + Fw_PelY_14( ref_pic, ry1 + 4, rx1 + 4 ) ) / 2;
                *d++      = orig_line[x0 + 2]  - ( PelY_14( ref_pic, ry, rx0 + 8 ) + Fw_PelY_14( ref_pic, ry1 + 4, rx1 + 8 ) ) / 2;
                *d++      = orig_line[x0 + 3]  - ( PelY_14( ref_pic, ry, rx0 + 12 ) + Fw_PelY_14( ref_pic, ry1 + 4, rx1 + 12 ) ) / 2;

                index_pos += stride;
                orig_line = orig_val + index_pos;
                ry = ry0 + 8;
                *d++      = orig_line[x0  ]  - ( PelY_14( ref_pic, ry, rx0 ) + Fw_PelY_14( ref_pic, ry1 + 8, rx1 ) ) / 2;
                *d++      = orig_line[x0 + 1]  - ( PelY_14( ref_pic, ry, rx0 + 4 ) + Fw_PelY_14( ref_pic, ry1 + 8, rx1 + 4 ) ) / 2;
                *d++      = orig_line[x0 + 2]  - ( PelY_14( ref_pic, ry, rx0 + 8 ) + Fw_PelY_14( ref_pic, ry1 + 8, rx1 + 8 ) ) / 2;
                *d++      = orig_line[x0 + 3]  - ( PelY_14( ref_pic, ry, rx0 + 12 ) + Fw_PelY_14( ref_pic, ry1 + 8, rx1 + 12 ) ) / 2;

                index_pos += stride;
                orig_line = orig_val + index_pos;
                ry = ry0 + 12;
                *d++      = orig_line[x0  ]  - ( PelY_14( ref_pic, ry, rx0 ) + Fw_PelY_14( ref_pic, ry1 + 12, rx1 ) ) / 2;
                *d++      = orig_line[x0 + 1]  - ( PelY_14( ref_pic, ry, rx0 + 4 ) + Fw_PelY_14( ref_pic, ry1 + 12, rx1 + 4 ) ) / 2;
                *d++      = orig_line[x0 + 2]  - ( PelY_14( ref_pic, ry, rx0 + 8 ) + Fw_PelY_14( ref_pic, ry1 + 12, rx1 + 8 ) ) / 2;
                *d++      = orig_line[x0 + 3]  - ( PelY_14( ref_pic, ry, rx0 + 12 ) + Fw_PelY_14( ref_pic, ry1 + 12, rx1 + 12 ) ) / 2;

                for ( yy = y0, kk = 0; yy < y0 + 4; yy++ )
                    for ( xx = x0; xx < x0 + 4; xx++, kk++ )
                    {
                        curr_diff[yy][xx] = diff[kk];
                    }
            }
        }

        mcost += find_sad_8x8( input->hadamard, blocksize_x, blocksize_y, 0, 0, curr_diff );
        if ( mcost < min_mcost )
        {
            min_mcost = mcost;
            best_pos  = pos;
        }
    }

    if ( best_pos )
    {
        *mv_x += ( square_points_x [best_pos] << 1 );
        *mv_y += ( square_points_y [best_pos] << 1 );
    }

    /************************************
    *****                          *****
    *****  QUARTER-PEL REFINEMENT  *****
    *****                          *****
    ************************************/
    //===== set function for getting pixel values =====
    if ( ( pic4_pix_x + *mv_x > 1 ) && ( pic4_pix_x + *mv_x < max_pos_x4 - 1 ) &&
            ( pic4_pix_y + *mv_y > 1 ) && ( pic4_pix_y + *mv_y < max_pos_y4 - 1 ) )
    {
        PelY_14 = FastPelY_14;
    }
    else
    {
        PelY_14 = UMVPelY_14;
    }

    //===== loop over search positions =====
    for ( best_pos = 0, pos = 1; pos < search_pos4; pos++ )
    {
        cand_mv_x = *mv_x + square_points_x[pos];    // quarter-pel units
        cand_mv_y = *mv_y + square_points_y[pos];    // quarter-pel units

        img->mv_range_flag = check_mv_range( cand_mv_x, cand_mv_y, pic_pix_x, pic_pix_y, mode );
        if ( !img->mv_range_flag )
        {
            img->mv_range_flag = 1;
            continue;
        }

        //----- set motion vector cost -----
        mcost = MV_COST( lambda_factor, mv_shift, cand_mv_x, cand_mv_y, pred_mv_x, pred_mv_y );
        if ( ref != -1 )
        {
            mcost += REF_COST( lambda_factor, ref );
        }

        //----- add up SATD -----
        for ( y0 = 0, abort_search = 0; y0 < blocksize_y && !abort_search; y0 += 4 )
        {
            ry0 = ( ( pic_pix_y + y0 ) << 2 ) + cand_mv_y;
            ry1 = ( ( pic_pix_y + y0 ) << 2 ) + *fw_mv_y;

            for ( x0 = 0; x0 < blocksize_x; x0 += 4 )
            {
                rx0 = ( ( pic_pix_x + x0 ) << 2 ) + cand_mv_x;
                rx1 = ( ( pic_pix_x + x0 ) << 2 ) + *fw_mv_x;
                d   = diff;

                index_pos = y0 * stride;
                orig_line = orig_val + index_pos;
                ry = ry0;
                *d++      = orig_line[x0  ]  - ( PelY_14( ref_pic, ry, rx0 ) + Fw_PelY_14( ref_pic, ry1, rx1 ) ) / 2;
                *d++      = orig_line[x0 + 1]  - ( PelY_14( ref_pic, ry, rx0 + 4 ) + Fw_PelY_14( ref_pic, ry1, rx1 + 4 ) ) / 2;
                *d++      = orig_line[x0 + 2]  - ( PelY_14( ref_pic, ry, rx0 + 8 ) + Fw_PelY_14( ref_pic, ry1, rx1 + 8 ) ) / 2;
                *d++      = orig_line[x0 + 3]  - ( PelY_14( ref_pic, ry, rx0 + 12 ) + Fw_PelY_14( ref_pic, ry1, rx1 + 12 ) ) / 2;

                index_pos += stride;
                orig_line = orig_val + index_pos;
                ry = ry0 + 4;
                *d++      = orig_line[x0  ]  - ( PelY_14( ref_pic, ry, rx0 ) + Fw_PelY_14( ref_pic, ry1 + 4, rx1 ) ) / 2;
                *d++      = orig_line[x0 + 1]  - ( PelY_14( ref_pic, ry, rx0 + 4 ) + Fw_PelY_14( ref_pic, ry1 + 4, rx1 + 4 ) ) / 2;
                *d++      = orig_line[x0 + 2]  - ( PelY_14( ref_pic, ry, rx0 + 8 ) + Fw_PelY_14( ref_pic, ry1 + 4, rx1 + 8 ) ) / 2;
                *d++      = orig_line[x0 + 3]  - ( PelY_14( ref_pic, ry, rx0 + 12 ) + Fw_PelY_14( ref_pic, ry1 + 4, rx1 + 12 ) ) / 2;

                index_pos += stride;
                orig_line = orig_val + index_pos;
                ry = ry0 + 8;
                *d++      = orig_line[x0  ]  - ( PelY_14( ref_pic, ry, rx0 ) + Fw_PelY_14( ref_pic, ry1 + 8, rx1 ) ) / 2;
                *d++      = orig_line[x0 + 1]  - ( PelY_14( ref_pic, ry, rx0 + 4 ) + Fw_PelY_14( ref_pic, ry1 + 8, rx1 + 4 ) ) / 2;
                *d++      = orig_line[x0 + 2]  - ( PelY_14( ref_pic, ry, rx0 + 8 ) + Fw_PelY_14( ref_pic, ry1 + 8, rx1 + 8 ) ) / 2;
                *d++      = orig_line[x0 + 3]  - ( PelY_14( ref_pic, ry, rx0 + 12 ) + Fw_PelY_14( ref_pic, ry1 + 8, rx1 + 12 ) ) / 2;

                index_pos += stride;
                orig_line = orig_val + index_pos;
                ry = ry0 + 12;
                *d++      = orig_line[x0  ]  - ( PelY_14( ref_pic, ry, rx0 ) + Fw_PelY_14( ref_pic, ry1 + 12, rx1 ) ) / 2;
                *d++      = orig_line[x0 + 1]  - ( PelY_14( ref_pic, ry, rx0 + 4 ) + Fw_PelY_14( ref_pic, ry1 + 12, rx1 + 4 ) ) / 2;
                *d++      = orig_line[x0 + 2]  - ( PelY_14( ref_pic, ry, rx0 + 8 ) + Fw_PelY_14( ref_pic, ry1 + 12, rx1 + 8 ) ) / 2;
                *d++      = orig_line[x0 + 3]  - ( PelY_14( ref_pic, ry, rx0 + 12 ) + Fw_PelY_14( ref_pic, ry1 + 12, rx1 + 12 ) ) / 2;

                for ( yy = y0, kk = 0; yy < y0 + 4; yy++ )
                    for ( xx = x0; xx < x0 + 4; xx++, kk++ )
                    {
                        curr_diff[yy][xx] = diff[kk];
                    }
            }
        }

        mcost += find_sad_8x8( input->hadamard, blocksize_x, blocksize_y, 0, 0, curr_diff );
        if ( mcost < min_mcost )
        {
            min_mcost = mcost;
            best_pos  = pos;
        }
    }

    if ( best_pos )
    {
        *mv_x += square_points_x [best_pos];
        *mv_y += square_points_y [best_pos];
    }

    //===== return minimum motion cost =====
    return min_mcost;
}

int SquareFractionalMVSearchSATDSym ( pel_t*   orig_val,     // <--  original pixel values for the AxB block
                                  int       stride,
                                  int       ref,           // <--  reference frame (0... or -1 (backward))
                                  int       pic_pix_x,     // <--  absolute x-coordinate of regarded AxB block
                                  int       pic_pix_y,     // <--  absolute y-coordinate of regarded AxB block
                                  int       mode,     // <--  block type (1-16x16 ... 7-4x4)
                                  int       pred_mv_x,     // <--  motion vector predictor (x) in sub-pel units
                                  int       pred_mv_y,     // <--  motion vector predictor (y) in sub-pel units
                                  int      *mv_x,          // <--> in: search center (x) / out: motion vector (x) - in pel units
                                  int      *mv_y,          // <--> in: search center (y) / out: motion vector (y) - in pel units
                                  int       search_pos2,   // <--  search positions for    half-pel search  (default: 9)
                                  int       search_pos4,   // <--  search positions for quarter-pel search  (default: 9)
                                  int       min_mcost )    // <--  minimum motion cost (cost for center or huge value)
{
    int   diff[16], *d;
    int   pos, best_pos, mcost, abort_search;
    int   y0, x0, ry0, rx0, ry;
    int xx, yy, kk;
    int   index_pos = 0;
    int   curr_diff[MB_SIZE][MB_SIZE]; // for IVC 8x8 SATD calculation
    int   ry0_bid, rx0_bid, ry_bid;
    int   cand_mv_x, cand_mv_y;
    pel_t *orig_line;
    pel_t *ref_pic, *ref_pic_bid;
    int   lambda_factor   = LAMBDA_FACTOR( sqrt( img->lambda ) );
    int   mv_shift        = 0;
    int   blocksize_x     = ( blc_size[mode][0] << 3 );
    int   blocksize_y     = ( blc_size[mode][1] << 3 );
    int   min_pos2        = ( input->hadamard ? 0 : 1 );
    int   max_pos2        = ( input->hadamard ? MAX( 1, search_pos2 ) : search_pos2 );
    assert( img->type == B_IMG );

    ref_pic     = mref[ref + 1];
    ref_pic_bid = mref[0];

    /*   HALF-PEL REFINEMENT  */
    // convert search center to quarter-pel units
    *mv_x <<= 2;
    *mv_y <<= 2;
    PelY_14 = UMVPelY_14;

    // loop over search positions
    for ( best_pos = 0, pos = min_pos2; pos < max_pos2; pos++ )
    {
        cand_mv_x = *mv_x + ( square_points_x[pos] << 1 );  // quarter-pel units
        cand_mv_y = *mv_y + ( square_points_y[pos] << 1 );  // quarter-pel units

        // set motion vector cost
        mcost = MV_COST( lambda_factor, mv_shift, cand_mv_x, cand_mv_y, pred_mv_x, pred_mv_y );
        if ( ref != -1 )
        {
            mcost += REF_COST( lambda_factor, ref );
        }

        // add up SATD
        for ( y0 = 0, abort_search = 0; y0 < blocksize_y && !abort_search; y0 += 4 )
        {
            ry0 = ( ( pic_pix_y + y0 ) << 2 ) + cand_mv_y;

            ry0_bid = ((pic_pix_y + y0) << 2) + GenSymBackMV(cand_mv_y);
            for ( x0 = 0; x0 < blocksize_x; x0 += 4 )
            {
                rx0 = ( ( pic_pix_x + x0 ) << 2 ) + cand_mv_x;
                rx0_bid = ((pic_pix_x + x0) << 2) + GenSymBackMV(cand_mv_x);
                d   = diff;

                index_pos =  y0 * stride;
                orig_line = orig_val + index_pos;
                ry = ry0;
                ry_bid = ry0_bid;

                *d++      = orig_line[x0  ]  - ( PelY_14( ref_pic, ry, rx0 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid ) ) / 2;
                *d++      = orig_line[x0 + 1]  - ( PelY_14( ref_pic, ry, rx0 + 4 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 4 ) ) / 2;
                *d++      = orig_line[x0 + 2]  - ( PelY_14( ref_pic, ry, rx0 + 8 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 8 ) ) / 2;
                *d++      = orig_line[x0 + 3]  - ( PelY_14( ref_pic, ry, rx0 + 12 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 12 ) ) / 2;

                index_pos += stride;
                orig_line = orig_val + index_pos;
                ry = ry0 + 4;
                ry_bid = ry0_bid + 4;
                *d++      = orig_line[x0  ]  - ( PelY_14( ref_pic, ry, rx0 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid ) ) / 2;
                *d++      = orig_line[x0 + 1]  - ( PelY_14( ref_pic, ry, rx0 + 4 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 4 ) ) / 2;
                *d++      = orig_line[x0 + 2]  - ( PelY_14( ref_pic, ry, rx0 + 8 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 8 ) ) / 2;
                *d++      = orig_line[x0 + 3]  - ( PelY_14( ref_pic, ry, rx0 + 12 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 12 ) ) / 2;

                index_pos += stride;
                orig_line = orig_val + index_pos;
                ry = ry0 + 8;
                ry_bid = ry0_bid + 8;
                *d++      = orig_line[x0  ]  - ( PelY_14( ref_pic, ry, rx0 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid ) ) / 2;
                *d++      = orig_line[x0 + 1]  - ( PelY_14( ref_pic, ry, rx0 + 4 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 4 ) ) / 2;
                *d++      = orig_line[x0 + 2]  - ( PelY_14( ref_pic, ry, rx0 + 8 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 8 ) ) / 2;
                *d++      = orig_line[x0 + 3]  - ( PelY_14( ref_pic, ry, rx0 + 12 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 12 ) ) / 2;

                index_pos += stride;
                orig_line = orig_val + index_pos;
                ry = ry0 + 12;
                ry_bid = ry0_bid + 12;
                *d++      = orig_line[x0  ]  - ( PelY_14( ref_pic, ry, rx0 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid ) ) / 2;
                *d++      = orig_line[x0 + 1]  - ( PelY_14( ref_pic, ry, rx0 + 4 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 4 ) ) / 2;
                *d++      = orig_line[x0 + 2]  - ( PelY_14( ref_pic, ry, rx0 + 8 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 8 ) ) / 2;
                *d        = orig_line[x0 + 3]  - ( PelY_14( ref_pic, ry, rx0 + 12 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 12 ) ) / 2;
                for ( yy = y0, kk = 0; yy < y0 + 4; yy++ )
                    for ( xx = x0; xx < x0 + 4; xx++, kk++ )
                    {
                        curr_diff[yy][xx] = diff[kk];
                    }
            }
        }
        mcost += find_sad_8x8( input->hadamard, blocksize_x, blocksize_y, 0, 0, curr_diff );

        if ( mcost < min_mcost )
        {
            min_mcost = mcost;
            best_pos  = pos;
        }
    }
    if ( best_pos )
    {
        *mv_x += ( square_points_x [best_pos] << 1 );
        *mv_y += ( square_points_y [best_pos] << 1 );
    }


    /* QUARTER-PEL REFINEMENT */
    // set function for getting pixel values
    PelY_14 = UMVPelY_14;

    // loop over search positions
    for ( best_pos = 0, pos = 1; pos < search_pos4; pos++ )
    {
        cand_mv_x = *mv_x + square_points_x[pos];    // quarter-pel units
        cand_mv_y = *mv_y + square_points_y[pos];    // quarter-pel units

        img->mv_range_flag = check_mv_range_bid( cand_mv_x, cand_mv_y, pic_pix_x, pic_pix_y, mode );
        if ( !img->mv_range_flag )
        {
            img->mv_range_flag = 1;
            continue;
        }

        //----- set motion vector cost -----
        mcost = MV_COST( lambda_factor, mv_shift, cand_mv_x, cand_mv_y, pred_mv_x, pred_mv_y );
        if ( ref != -1 )
        {
            mcost += REF_COST( lambda_factor, ref );
        }
        // add up SATD
        for ( y0 = 0, abort_search = 0; y0 < blocksize_y && !abort_search; y0 += 4 )
        {
            ry0 = ( ( pic_pix_y + y0 ) << 2 ) + cand_mv_y;

            ry0_bid = ((pic_pix_y + y0) << 2) + GenSymBackMV(cand_mv_y);
            for ( x0 = 0; x0 < blocksize_x; x0 += 4 )
            {
                rx0 = ( ( pic_pix_x + x0 ) << 2 ) + cand_mv_x;

                rx0_bid = ((pic_pix_x + x0) << 2) + GenSymBackMV(cand_mv_x);
                d   = diff;

                index_pos =  y0 * stride;
                orig_line = orig_val + index_pos;
                ry = ry0;
                ry_bid = ry0_bid;
                *d++      = orig_line[x0  ]  - ( PelY_14( ref_pic, ry, rx0 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid ) ) / 2;
                *d++      = orig_line[x0 + 1]  - ( PelY_14( ref_pic, ry, rx0 + 4 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 4 ) ) / 2;
                *d++      = orig_line[x0 + 2]  - ( PelY_14( ref_pic, ry, rx0 + 8 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 8 ) ) / 2;
                *d++      = orig_line[x0 + 3]  - ( PelY_14( ref_pic, ry, rx0 + 12 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 12 ) ) / 2;

                index_pos += stride;
                orig_line = orig_val + index_pos;
                ry = ry0 + 4;
                ry_bid = ry0_bid + 4;
                *d++      = orig_line[x0  ]  - ( PelY_14( ref_pic, ry, rx0 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid ) ) / 2;
                *d++      = orig_line[x0 + 1]  - ( PelY_14( ref_pic, ry, rx0 + 4 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 4 ) ) / 2;
                *d++      = orig_line[x0 + 2]  - ( PelY_14( ref_pic, ry, rx0 + 8 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 8 ) ) / 2;
                *d++      = orig_line[x0 + 3]  - ( PelY_14( ref_pic, ry, rx0 + 12 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 12 ) ) / 2;

                index_pos += stride;
                orig_line = orig_val + index_pos;
                ry = ry0 + 8;
                ry_bid = ry0_bid + 8;
                *d++      = orig_line[x0  ]  - ( PelY_14( ref_pic, ry, rx0 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid ) ) / 2;
                *d++      = orig_line[x0 + 1]  - ( PelY_14( ref_pic, ry, rx0 + 4 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 4 ) ) / 2;
                *d++      = orig_line[x0 + 2]  - ( PelY_14( ref_pic, ry, rx0 + 8 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 8 ) ) / 2;
                *d++      = orig_line[x0 + 3]  - ( PelY_14( ref_pic, ry, rx0 + 12 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 12 ) ) / 2;

                index_pos += stride;
                orig_line = orig_val + index_pos;
                ry = ry0 + 12;
                ry_bid = ry0_bid + 12;
                *d++      = orig_line[x0  ]  - ( PelY_14( ref_pic, ry, rx0 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid ) ) / 2;
                *d++      = orig_line[x0 + 1]  - ( PelY_14( ref_pic, ry, rx0 + 4 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 4 ) ) / 2;
                *d++      = orig_line[x0 + 2]  - ( PelY_14( ref_pic, ry, rx0 + 8 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 8 ) ) / 2;
                *d        = orig_line[x0 + 3]  - ( PelY_14( ref_pic, ry, rx0 + 12 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 12 ) ) / 2;

                if ( ( mcost += SATD( diff, input->hadamard ) ) > min_mcost )
                {
                    abort_search = 1;
                    break;
                }
            }
        }

        if ( mcost < min_mcost )
        {
            min_mcost = mcost;
            best_pos  = pos;
        }
    }
    if ( best_pos )
    {
        *mv_x += square_points_x [best_pos];
        *mv_y += square_points_y [best_pos];
    }

    // return minimum motion cost
    return min_mcost;
}
/*
*************************************************************************
* Function:Functions for SAD prediction of intra block cases.
1. void   decide_intrabk_SAD() judges the block coding type(intra/inter)
of neibouring blocks
2. void skip_intrabk_SAD() set the SAD to zero if neigouring block coding
type is intra
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void decide_intrabk_SAD()
{
    if ( img->type != 0 )
    {
        if ( img->mb_pix_x == 0 && img->mb_pix_y == 0 )
        {
            flag_intra_SAD = 0;
        }
        else if ( img->mb_pix_x == 0 )
        {
            flag_intra_SAD = flag_intra[( img->mb_pix_x ) >> 4];
        }
        else if ( img->mb_pix_y == 0 )
        {
            flag_intra_SAD = flag_intra[( ( img->mb_pix_x ) >> 4 ) - 1];
        }
        else
        {
            flag_intra_SAD = ( ( flag_intra[( img->mb_pix_x ) >> 4] ) || ( flag_intra[( ( img->mb_pix_x ) >> 4 ) - 1] ) || ( flag_intra[( ( img->mb_pix_x ) >> 4 ) + 1] ) ) ;
        }
    }
    return;
}

void reset_mincost( ImgParams *img, int best_mode )
{
    int i, j, k, ref;
    int mb_b4_x = img->mb_b4_x;
    int mb_b4_y = img->mb_b4_y;

    if ( img->ip_frm_idx > 0 )
    {
        flag_intra[( img->mb_pix_x ) >> 4] = ( best_mode == I_MB || best_mode == 10 ) ? 1 : 0;
    }
    if ( img->type != I_IMG  && best_mode == I_MB )
    {
        for ( i = 0; i < 4; i++ )
        {
            for ( j = 0; j < 4; j++ )
            {
                for ( ref = 0; ref < 1; ref++ )
                {
                    for ( k = 1; k < MAXMODE; k++ )
                    {
                        all_mincost[mb_b4_x + i][mb_b4_y + j][ref][k][0] = 0;
                    }
                }
            }
        }
    }
    return;
}


int FastMotionSearchSym( Macroblock *currMB, int ref, int pu_b8_x, int pu_b8_y, int mode )
{
    static pel_t   orig_val [MB_SIZE*MB_SIZE];
    int stride = MB_SIZE, index_pos = 0;

    int pred_mv_x, pred_mv_y, mv_x, mv_y, i, j;
    int max_value = ( 1<<20 );
    int       min_mcost = ( 1 << 20 );
    int **ref_array = img->bfrm_fref;
    int ***mv_array = img->bfrm_fmv;
    int *****all_mv = img->mv_sym_mhp;

    int N_Bframe = input->successive_Bframe, n_Bframe = ( N_Bframe ) ? ( ( Bframe_ctr % N_Bframe ) ? Bframe_ctr % N_Bframe : N_Bframe ) : 0 ;

    int pu_pix_x = pu_b8_x << 3;
    int pu_pix_y = pu_b8_y << 3;
    int pu_pix_x_in_mb = pu_pix_x - img->mb_pix_x;
    int pu_pix_y_in_mb = pu_pix_y - img->mb_pix_y;
    int b8_x_in_mb = ( pu_pix_x_in_mb >> 3 );
    int b8_y_in_mb = ( pu_pix_y_in_mb>> 3 );
    int mb_b4_x = img->mb_b4_x;
    int mb_b4_y = img->mb_b4_y;

    int blk_step_x = blc_size[mode][0];
    int blk_step_y = blc_size[mode][1];
    int b4_step_x = blk_step_x << 1;
    int b4_step_y = blk_step_y << 1;
    int pu_bsize_x = blk_step_x << 3;
    int pu_bsize_y = blk_step_y << 3;

    int center_x = pu_pix_x;
    int center_y = pu_pix_y;

    int *pred_mv = img->pmv_sym_mhp[ref][mode][b8_y_in_mb][b8_x_in_mb];

    // get original block
    for ( j = 0; j < pu_bsize_y; j++ )
    {
        for ( i = 0; i < pu_bsize_x; i++ )
        {
            orig_val[index_pos + i] = imgY_org[( pu_pix_y + j )*( img->width ) + pu_pix_x + i];
        }
        index_pos += stride;
    }

    if ( mode == 4 )
    {
        pred_MV_uplayer[0] = all_mv[ref][2][b8_y_in_mb][b8_x_in_mb][0];
        pred_MV_uplayer[1] = all_mv[ref][2][b8_y_in_mb][b8_x_in_mb][1];
        pred_SAD_uplayer    = ( ref == -1 ) ? ( all_bwmincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][0][2][0] ) : ( all_mincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][ref][2][0] );
        pred_SAD_uplayer   /= 2;
    }
    else if ( mode > 1 )
    {
        pred_MV_uplayer[0] = all_mv[ref][1][b8_y_in_mb][b8_x_in_mb][0];
        pred_MV_uplayer[1] = all_mv[ref][1][b8_y_in_mb][b8_x_in_mb][1];
        pred_SAD_uplayer    = ( ref == -1 ) ? ( all_bwmincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][0][1][0] ) : ( all_mincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][ref][1][0] );
        pred_SAD_uplayer   /= 2;
    }

    pred_SAD_uplayer = flag_intra_SAD ? 0 : pred_SAD_uplayer;// for irregular motion

    //coordinate prediction
    if ( img->ip_frm_idx > ref + 1 )
    {
        pred_MV_time[0] = all_mincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][ref][mode][1];
        pred_MV_time[1] = all_mincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][ref][mode][2];
    }
    if ( ref == -1 && Bframe_ctr > 1 )
    {
        pred_MV_time[0] = ( int )( all_bwmincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][0][mode][1] * ( ( n_Bframe == 1 ) ? ( N_Bframe ) : ( N_Bframe - n_Bframe + 1.0 ) / ( N_Bframe - n_Bframe + 2.0 ) ) ); //should add a factor
        pred_MV_time[1] = ( int )( all_bwmincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][0][mode][2] * ( ( n_Bframe == 1 ) ? ( N_Bframe ) : ( N_Bframe - n_Bframe + 1.0 ) / ( N_Bframe - n_Bframe + 2.0 ) ) ); //should add a factor
    }

    if ( ref > 0 )
    {
        //field_mode top_field
        pred_SAD_ref = all_mincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][( ref - 1 )][mode][0];
        pred_SAD_ref = flag_intra_SAD ? 0 : pred_SAD_ref;//add this for irregular motion
        pred_MV_ref[0] = all_mincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][( ref - 1 )][mode][1];
        pred_MV_ref[0] = ( int )( pred_MV_ref[0] * ( ref + 1 ) / ( float )( ref ) );
        pred_MV_ref[1] = all_mincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][( ref - 1 )][mode][2];
        pred_MV_ref[1] = ( int )( pred_MV_ref[1] * ( ref + 1 ) / ( float )( ref ) );
    }
    if ( ref == 0 )
    {
        pred_SAD_ref = all_bwmincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][0][mode][0];
        pred_SAD_ref = flag_intra_SAD ? 0 : pred_SAD_ref;//add this for irregular motion
        pred_MV_ref[0] = ( int )( all_bwmincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][0][mode][1] * ( -n_Bframe ) / ( N_Bframe - n_Bframe + 1.0f ) ); //should add a factor
        pred_MV_ref[1] = ( int )( all_bwmincost[mb_b4_x + b8_x_in_mb][mb_b4_y + b8_y_in_mb][0][mode][2] * ( -n_Bframe ) / ( N_Bframe - n_Bframe + 1.0f ) );
    }

    // get mv predictor
    SetMotionVectorPredictorME( img, currMB, pred_mv, ref_array, mv_array, ref, pu_pix_x_in_mb, pu_pix_y_in_mb, pu_bsize_x, mode, ref );
    pred_mv_x = pred_mv[0];
    pred_mv_y = pred_mv[1];

    // integer-pel search
    mv_x = pred_mv_x / 4;
    mv_y = pred_mv_y / 4;

    min_mcost = FastIntegerMVSearch( orig_val,stride, ref, center_x, center_y,/*pic_pix_x, pic_pix_y,*/ mode,
                pred_mv_x, pred_mv_y, &mv_x, &mv_y, min_mcost );

    for ( i = 0; i < b4_step_x; i++ )
    {
        for ( j = 0; j < b4_step_y; j++ )
        {
            if ( ref > -1 )
            {
                all_mincost[mb_b4_x + b8_x_in_mb + i][mb_b4_y + b8_y_in_mb + j][ref][mode][0] = min_mcost;
            }
            else
            {
                all_bwmincost[mb_b4_x + b8_x_in_mb + i][mb_b4_y + b8_y_in_mb + j][0][mode][0] = min_mcost;
            }
        }
    }

    // sub-pel search
    if ( input->hadamard )
    {
        min_mcost = max_value;
    }

    if ( mode > 3 )
    {
        min_mcost =  CrossFractionalMVSearchSADSym( orig_val, stride, ref, center_x, center_y, mode,
                     pred_mv_x, pred_mv_y, &mv_x, &mv_y, min_mcost );
    }
    else
        min_mcost =  SquareFractionalMVSearchSATDSym( orig_val, stride, ref, center_x, center_y, mode,
                     pred_mv_x, pred_mv_y, &mv_x, &mv_y, 9, 9, min_mcost );

    for ( i = 0; i < b4_step_x; i++ )
    {
        for ( j = 0; j < b4_step_y; j++ )
        {
            if ( ref > -1 )
            {
                all_mincost[mb_b4_x + b8_x_in_mb + i][mb_b4_y + b8_y_in_mb + j][ref][mode][1] = mv_x;
                all_mincost[mb_b4_x + b8_x_in_mb + i][mb_b4_y + b8_y_in_mb + j][ref][mode][2] = mv_y;
            }
            else
            {
                all_bwmincost[mb_b4_x + b8_x_in_mb + i][mb_b4_y + b8_y_in_mb + j][0][mode][1] = mv_x;
                all_bwmincost[mb_b4_x + b8_x_in_mb + i][mb_b4_y + b8_y_in_mb + j][0][mode][2] = mv_y;
            }
        }
    }

    // set mv and return motion cost
    for ( i = 0; i < blk_step_x; i++ )
    {
        for ( j = 0; j < blk_step_y; j++ )
        {
            all_mv[ref][mode][b8_y_in_mb + j][b8_x_in_mb + i][0] = mv_x;
            all_mv[ref][mode][b8_y_in_mb + j][b8_x_in_mb + i][1] = mv_y;
        }
    }

    img->mv_range_flag = check_mv_range_bid( mv_x, mv_y, pu_pix_x, pu_pix_y, mode );
    img->mv_range_flag *= check_mvd( ( mv_x - pred_mv_x ), ( mv_y - pred_mv_y ) );
    if ( !img->mv_range_flag )
    {
        min_mcost = 1 << 24;
        img->mv_range_flag = 1;
    }

    return min_mcost;
}

int CrossFractionalMVSearchSADSym ( pel_t*   orig_val,     // <--  original pixel values for the AxB block
                                      int       stride,
                                      int       ref,           // <--  reference frame (0... or -1 (backward))
                                      int       pic_pix_x,     // <--  absolute x-coordinate of regarded AxB block
                                      int       pic_pix_y,     // <--  absolute y-coordinate of regarded AxB block
                                      int       mode,          // <--  block type (1-16x16 ... 7-4x4)
                                      int       pred_mv_x,     // <--  motion vector predictor (x) in sub-pel units
                                      int       pred_mv_y,     // <--  motion vector predictor (y) in sub-pel units
                                      int      *mv_x,          // <--> in: search center (x) / out: motion vector (x) - in pel units
                                      int      *mv_y,          // <--> in: search center (y) / out: motion vector (y) - in pel units
                                      int       min_mcost )    // <--  minimum motion cost (cost for center or huge value)
{
    const int cross_points_x[4] = { -1, 0, 1, 0 };
    const int cross_points_y[4] = { 0, 1, 0, -1 };
    int   mcost;
    int   cand_mv_x, cand_mv_y;
    pel_t *ref_pic = mref[ref + 1];
    pel_t *ref_pic_bid;

    int   lambda_factor   = LAMBDA_FACTOR( sqrt( img->lambda ) );
    int   mv_shift        = 0;

    int   search_range_dynamic, iXMinNow, iYMinNow, i;
    int   iSADLayer, m;
    int   currmv_x = 0, currmv_y = 0, iCurrSearchRange;
    int   pred_frac_mv_x, pred_frac_mv_y, abort_search;
    int   mv_cost;

    ref_pic_bid = img->type == B_IMG ? mref [0] : mref [ref];

    *mv_x <<= 2;
    *mv_y <<= 2;
    PelY_14 = UMVPelY_14;

    search_range_dynamic = 3;
    pred_frac_mv_x = ( pred_mv_x - *mv_x ) % 4;
    pred_frac_mv_y = ( pred_mv_y - *mv_y ) % 4;

    memset( SearchState[0], 0, ( 2 * search_range_dynamic + 1 ) * ( 2 * search_range_dynamic + 1 ) );

    if ( input->hadamard )
    {
        cand_mv_x = *mv_x;
        cand_mv_y = *mv_y;
        mv_cost = MV_COST( lambda_factor, mv_shift, cand_mv_x, cand_mv_y, pred_mv_x, pred_mv_y );
        if ( ref != -1 )
        {
            mv_cost += REF_COST( lambda_factor, ref );
        }

        mcost = CalculateSADCostSym( pic_pix_x, pic_pix_y, mode, cand_mv_x, cand_mv_y, ref_pic,
                                     ref_pic_bid, orig_val, stride, mv_cost);
        SearchState[search_range_dynamic][search_range_dynamic] = 1;

        if ( mcost < min_mcost )
        {
            min_mcost = mcost;
            currmv_x = cand_mv_x;
            currmv_y = cand_mv_y;
        }


    }
    else
    {
        SearchState[search_range_dynamic][search_range_dynamic] = 1;
        currmv_x = *mv_x;
        currmv_y = *mv_y;
    }

    if ( pred_frac_mv_x != 0 || pred_frac_mv_y != 0 )
    {
        cand_mv_x = *mv_x + pred_frac_mv_x;
        cand_mv_y = *mv_y + pred_frac_mv_y;
        mv_cost = MV_COST( lambda_factor, mv_shift, cand_mv_x, cand_mv_y, pred_mv_x, pred_mv_y );
        if ( ref != -1 )
        {
            mv_cost += REF_COST( lambda_factor, ref );
        }
        mcost = CalculateSADCostSym( pic_pix_x, pic_pix_y, mode, cand_mv_x, cand_mv_y, ref_pic,
                                     ref_pic_bid, orig_val, stride, mv_cost);
        SearchState[cand_mv_y - *mv_y + search_range_dynamic][cand_mv_x - *mv_x + search_range_dynamic] = 1;

        if ( mcost < min_mcost )
        {
            min_mcost = mcost;
            currmv_x = cand_mv_x;
            currmv_y = cand_mv_y;
        }
    }


    iXMinNow = currmv_x;
    iYMinNow = currmv_y;
    iCurrSearchRange = 2 * search_range_dynamic + 1;
    for ( i = 0; i < iCurrSearchRange; i++ )
    {
        abort_search = 1;
        iSADLayer = 65536;
        for ( m = 0; m < 4; m++ )
        {
            cand_mv_x = iXMinNow + cross_points_x[m];
            cand_mv_y = iYMinNow + cross_points_y[m];

            img->mv_range_flag = check_mv_range_bid( cand_mv_x, cand_mv_y, pic_pix_x, pic_pix_y, mode );
            if ( !img->mv_range_flag )
            {
                img->mv_range_flag = 1;
                continue;
            }

            if ( abs( cand_mv_x - *mv_x ) <= search_range_dynamic && abs( cand_mv_y - *mv_y ) <= search_range_dynamic )
            {
                if ( !SearchState[cand_mv_y - *mv_y + search_range_dynamic][cand_mv_x - *mv_x + search_range_dynamic] )
                {
                    mv_cost = MV_COST( lambda_factor, mv_shift, cand_mv_x, cand_mv_y, pred_mv_x, pred_mv_y );
                    if ( ref != -1 )
                    {
                        mv_cost += REF_COST( lambda_factor, ref );
                    }
                    mcost = CalculateSADCostSym( pic_pix_x, pic_pix_y, mode, cand_mv_x, cand_mv_y, ref_pic,
                                                 ref_pic_bid, orig_val, stride, mv_cost);
                    SearchState[cand_mv_y - *mv_y + search_range_dynamic][cand_mv_x - *mv_x + search_range_dynamic] = 1;
                    if ( mcost < min_mcost )
                    {
                        min_mcost = mcost;
                        currmv_x = cand_mv_x;
                        currmv_y = cand_mv_y;
                        abort_search = 0;

                    }
                }
            }
        }
        iXMinNow = currmv_x;
        iYMinNow = currmv_y;
        if ( abort_search )
        {
            break;
        }
    }

    *mv_x = currmv_x;
    *mv_y = currmv_y;

    //===== return minimum motion cost =====
    return min_mcost;
}

/*
*************************************************************************
* Function:Functions for fast fractional pel motion estimation.
1. int AddUpSADQuarter() returns SADT of a fractiona pel MV
2. int FastSubPelBlockMotionSearch () proceed the fast fractional pel ME
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

int CalculateSADCostSym( int pic_pix_x,int pic_pix_y,int mode,
                         int cand_mv_x,int cand_mv_y, pel_t *ref_pic, pel_t *ref_pic_bid, pel_t* orig_val, int stride,
                         int mv_mcost)
{
    int abort_search, y0, x0, rx0, ry0, ry;
    pel_t *orig_line;
    int  index_pos = 0;
    int  blocksize_x = ( blc_size[mode][0] << 3 );
    int  blocksize_y = ( blc_size[mode][1] << 3 );
    int   diff[16], *d;
    int  mcost = mv_mcost;
    int yy, kk, xx;
    int   curr_diff[MB_SIZE][MB_SIZE]; // for ABT SATD calculation

    int ry0_bid, rx0_bid, ry_bid;

    for ( y0 = 0, abort_search = 0; y0 < blocksize_y && !abort_search; y0 += 4 )
    {
        ry0 = ( ( pic_pix_y + y0 ) << 2 ) + cand_mv_y;
        ry0_bid = ((pic_pix_y + y0) << 2) + GenSymBackMV(cand_mv_y);
        for ( x0 = 0; x0 < blocksize_x; x0 += 4 )
        {
            rx0 = ( ( pic_pix_x + x0 ) << 2 ) + cand_mv_x;
            rx0_bid = ((pic_pix_x + x0) << 2) + GenSymBackMV(cand_mv_x);
            d   = diff;

            index_pos = y0 * stride;
            orig_line = orig_val + index_pos;
            ry = ry0;
            ry_bid = ry0_bid;
            *d++      = orig_line[x0  ]  - ( PelY_14( ref_pic, ry, rx0 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid ) ) / 2;
            *d++      = orig_line[x0 + 1]  - ( PelY_14( ref_pic, ry, rx0 + 4 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 4 ) ) / 2;
            *d++      = orig_line[x0 + 2]  - ( PelY_14( ref_pic, ry, rx0 + 8 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 8 ) ) / 2;
            *d++      = orig_line[x0 + 3]  - ( PelY_14( ref_pic, ry, rx0 + 12 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 12 ) ) / 2;

            index_pos += stride;
            orig_line = orig_val + index_pos;
            ry = ry0 + 4;
            ry_bid = ry0_bid + 4;
            *d++      = orig_line[x0  ]  - ( PelY_14( ref_pic, ry, rx0 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid ) ) / 2;
            *d++      = orig_line[x0 + 1]  - ( PelY_14( ref_pic, ry, rx0 + 4 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 4 ) ) / 2;
            *d++      = orig_line[x0 + 2]  - ( PelY_14( ref_pic, ry, rx0 + 8 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 8 ) ) / 2;
            *d++      = orig_line[x0 + 3]  - ( PelY_14( ref_pic, ry, rx0 + 12 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 12 ) ) / 2;

            index_pos += stride;
            orig_line = orig_val + index_pos;
            ry = ry0 + 8;
            ry_bid = ry0_bid + 8;
            *d++      = orig_line[x0  ]  - ( PelY_14( ref_pic, ry, rx0 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid ) ) / 2;
            *d++      = orig_line[x0 + 1]  - ( PelY_14( ref_pic, ry, rx0 + 4 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 4 ) ) / 2;
            *d++      = orig_line[x0 + 2]  - ( PelY_14( ref_pic, ry, rx0 + 8 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 8 ) ) / 2;
            *d++      = orig_line[x0 + 3]  - ( PelY_14( ref_pic, ry, rx0 + 12 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 12 ) ) / 2;

            index_pos += stride;
            orig_line = orig_val + index_pos;
            ry = ry0 + 12;
            ry_bid = ry0_bid + 12;
            *d++      = orig_line[x0  ]  - ( PelY_14( ref_pic, ry, rx0 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid ) ) / 2;
            *d++      = orig_line[x0 + 1]  - ( PelY_14( ref_pic, ry, rx0 + 4 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 4 ) ) / 2;
            *d++      = orig_line[x0 + 2]  - ( PelY_14( ref_pic, ry, rx0 + 8 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 8 ) ) / 2;
            *d        = orig_line[x0 + 3]  - ( PelY_14( ref_pic, ry, rx0 + 12 ) + PelY_14( ref_pic_bid, ry_bid, rx0_bid + 12 ) ) / 2;
            for ( yy = y0, kk = 0; yy < y0 + 4; yy++ )
                for ( xx = x0; xx < x0 + 4; xx++, kk++ )
                {
                    curr_diff[yy][xx] = diff[kk];
                }
        }
    }
    mcost += find_sad_8x8( input->hadamard, blocksize_x, blocksize_y, 0, 0, curr_diff );

    return mcost;
}
