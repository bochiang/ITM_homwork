#include "contributors.h"

#include <math.h>
#include <assert.h>
#include <limits.h>

#include "global.h"
#include "mv-search.h"
#include "refbuf.h"
#include "memalloc.h"
#include "block.h"
#include "../../common/interPrediction.h"

#ifdef FastME
#include "fast_me.h"
#endif

// These procedure pointers are used by motion_search() and one_eigthpel()
static pel_t ( *PelY_14 ) ( pel_t*, int, int );
static pel_t *( *PelYline_11 ) ( pel_t *, int, int );
static pel_t ( *Fw_PelY_14 ) ( pel_t*, int, int );

// Statistics, temporary
int    max_mvd;
int*   square_points_x;
int*   square_points_y;
int*   mvbits;
int*   refbits;
int*   byte_abs;
int*** motion_cost;
int*** motion_cost_sym;

#define MEDIAN(a,b,c)  (a + b + c - MIN(a, MIN(b, c)) - MAX(a, MAX(b, c)));  //jlzheng 6.30

extern const int ref_idx[32];

/*
******************************************************************************
*  Function: Determine the MVD's value (1/4 pixel) is legal or not.
*  Input:
*  Output:
*  Return: 0: out of the legal mv range; 1: in the legal mv range
*  Attention:
*  Author:
******************************************************************************
*/
int check_mvd( int mvd_x, int mvd_y )
{

    if ( mvd_x > 4095 || mvd_x < -4096 || mvd_y > 4095 || mvd_y < -4096 )
    {
        return 0;
    }

    return 1;
}


/*
******************************************************************************
*  Function: Determine the mv's value (1/4 pixel) is legal or not.
*  Input:
*  Output:
*  Return: 0: out of the legal mv range; 1: in the legal mv range
*  Attention:
*  Author:
******************************************************************************
*/
int check_mv_range( int mv_x, int mv_y, int pix_x, int pix_y, int mode )
{
    int curr_max_x, curr_min_x, curr_max_y, curr_min_y;
    int bx[6] = {8, 16, 16,  8, 8, 4};   // mode=0,1,2,3,PNxN, 4x4
    int by[6] = {8, 16,  8, 16, 8, 4};

    curr_max_x = ( img->width - ( pix_x + bx[mode] ) ) * 4 + 16 * 4;
    curr_min_x = pix_x * 4 + 16 * 4;
    curr_max_y = ( img->height - ( pix_y + by[mode] ) ) * 4 + 16 * 4;
    curr_min_y = pix_y * 4 + 16 * 4;


    if ( mv_x > curr_max_x || mv_x < -curr_min_x || mv_x > Max_H_MV || mv_x < Min_H_MV )
    {
        return 0;
    }
    if ( mv_y > curr_max_y || mv_y < -curr_min_y || mv_y > Max_V_MV || mv_y < Min_V_MV )
    {
        return 0;
    }

    return 1;
}


/*
******************************************************************************
*  Function: Determine the forward and backward mvs' value (1/4 pixel) is legal or not.
*  Input:
*  Output:
*  Return: 0: out of the legal mv range; 1: in the legal mv range
*  Attention:
*  Author:
******************************************************************************
*/
int check_mv_range_bid( int mv_x, int mv_y, int pix_x, int pix_y, int mode )
{
    int bw_mvx, bw_mvy;
    int curr_max_x, curr_min_x, curr_max_y, curr_min_y;
    int bx[5] = {8, 16, 16,  8, 8};
    int by[5] = {8, 16,  8, 16, 8};

    assert( img->type == B_IMG );

    bw_mvx = GenSymBackMV(mv_x);
    bw_mvy = GenSymBackMV(mv_y);

    curr_max_x = ( img->width -  ( pix_x+bx[mode] ) )*4 + 16*4;
    curr_min_x = pix_x * 4 + 16 * 4;
    curr_max_y = ( img->height - ( pix_y+by[mode] ) )*4 + 16*4;
    curr_min_y = pix_y * 4 + 16 * 4;


    if ( mv_x > curr_max_x || mv_x < -curr_min_x || mv_x > Max_H_MV || mv_x < Min_H_MV )
    {
        return 0;
    }
    if ( mv_y > curr_max_y || mv_y < -curr_min_y || mv_y > Max_V_MV || mv_y < Min_V_MV )
    {
        return 0;
    }

    if ( bw_mvx > curr_max_x || bw_mvx < -curr_min_x || bw_mvx > Max_H_MV || bw_mvx < Min_H_MV )
    {
        return 0;
    }
    if ( bw_mvy > curr_max_y || bw_mvy < -curr_min_y || bw_mvy > Max_V_MV || bw_mvy < Min_V_MV )
    {
        return 0;
    }

    return 1;
}

/**************************************************************************
* Function:Initialize the motion search
**************************************************************************/
void Init_Motion_Search_Module()
{
    int bits, i, imin, imax, k, l;
    int search_range               = input->search_range;
    int max_search_points          = ( 2 * search_range + 1 ) * ( 2 * search_range + 1 );
    int max_ref_bits               = 1 + 2 * ( int )floor( log( 16 ) / log( 2 ) + 1e-10 );
    int max_ref                    = ( 1 << ( ( max_ref_bits >> 1 ) + 1 ) ) - 1;
    int number_of_subpel_positions = 4 * ( 2 * search_range + 3 );
    int max_mv_bits                = 3 + 2 * ( int )ceil( log( number_of_subpel_positions + 1 ) / log( 2 ) + 1e-10 );

    max_mvd                        = ( 1 << ( ( max_mv_bits >> 1 ) ) ) - 1;

    //=====   CREATE ARRAYS   =====
    //-----------------------------
    if ( ( square_points_x = ( int * )calloc( max_search_points, sizeof( int ) ) ) == NULL )
    {
        no_mem_exit( "memory allocation: square_points" );
    }
    if ( ( square_points_y = ( int * )calloc( max_search_points, sizeof( int ) ) ) == NULL )
    {
        no_mem_exit( "memory allocation: square_points" );
    }
    if ( ( mvbits = ( int * )calloc( 2 * max_mvd + 1, sizeof( int ) ) ) == NULL )
    {
        no_mem_exit( "memory allocation: mvbits" );
    }
    if ( ( refbits = ( int * )calloc( max_ref, sizeof( int ) ) ) == NULL )
    {
        no_mem_exit( "Init_Motion_Search_Module: refbits" );
    }
    if ( ( byte_abs = ( int * )calloc( 512, sizeof( int ) ) ) == NULL )
    {
        no_mem_exit( "memory allocation: byte_abs" );
    }

    get_mem3Dint( &motion_cost, 8, 2 * ( img->real_ref_num + 1 ), 4 );
    get_mem3Dint( &motion_cost_sym, 8, 2 * ( img->real_ref_num + 1 ), 4 );

    //--- set array offsets ---
    mvbits   += max_mvd;
    byte_abs += 256;

    //=====   INIT ARRAYS   =====
    //---------------------------
    //--- init array: motion vector bits ---
    mvbits[0] = 1;

    for ( bits = 3; bits <= max_mv_bits; bits += 2 )
    {
        imax = 1    << ( bits >> 1 );
        imin = imax >> 1;

        for ( i = imin; i < imax; i++ )
        {
            mvbits[-i] = mvbits[i] = bits;
        }
    }
    //--- init array: reference frame bits ---
    refbits[0] = 1;
    for ( bits = 3; bits <= max_ref_bits; bits += 2 )
    {
        imax = ( 1   << ( ( bits >> 1 ) + 1 ) ) - 1;
        imin = imax >> 1;

        for ( i = imin; i < imax; i++ )
        {
            refbits[i] = bits;
        }
    }
    //--- init array: absolute value ---
    byte_abs[0] = 0;

    for ( i = 1; i < 256; i++ )
    {
        byte_abs[i] = byte_abs[-i] = i;
    }
    //--- init array: search pattern ---
    square_points_x[0] = square_points_y[0] = 0;
    for ( k = 1, l = 1; l <= MAX( 1, search_range ); l++ )
    {
        for ( i = -l + 1; i < l; i++ )
        {
            square_points_x[k] =  i;
            square_points_y[k++] = -l;
            square_points_x[k] =  i;
            square_points_y[k++] =  l;
        }
        for ( i = -l;   i <= l; i++ )
        {
            square_points_x[k] = -l;
            square_points_y[k++] =  i;
            square_points_x[k] =  l;
            square_points_y[k++] =  i;
        }
    }

}

/*
*************************************************************************
* Function:Free memory used by motion search
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void Clear_Motion_Search_Module()
{
    //--- correct array offset ---
    mvbits   -= max_mvd;
    byte_abs -= 256;

    free( square_points_x );
    free( square_points_y );
    free( mvbits );
    free( refbits );
    free( byte_abs );
    free_mem3Dint( motion_cost, 8 );
    free_mem3Dint( motion_cost_sym, 8 );
}

/*
*************************************************************************
* Function:Full pixel block motion search
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

int                                               //  ==> minimum motion cost after search
FullPelBlockMotionSearch ( pel_t*   orig_val,    // <--  original pixel values for the AxB block
                           int       stride,
                           int       ref,          // <--  reference frame (0... or -1 (backward))
                           int       pic_pix_x,    // <--  absolute x-coordinate of regarded AxB block
                           int       pic_pix_y,    // <--  absolute y-coordinate of regarded AxB block
                           int       mode,         // <--  block type (1-16x16 ... 7-4x4)
                           int       pred_mv_x,    // <--  motion vector predictor (x) in sub-pel units
                           int       pred_mv_y,    // <--  motion vector predictor (y) in sub-pel units
                           int      *mv_x,         // <--> in: search center (x) / out: motion vector (x) - in pel units
                           int      *mv_y,         // <--> in: search center (y) / out: motion vector (y) - in pel units
                           int       min_mcost )   // <--  minimum motion cost (cost for center or huge value)
{
    int   pos, cand_x, cand_y, y, x8, mcost;
    int   index_pos = 0;
    int   search_range = input->search_range;
    pel_t *orig_line, *ref_line;
    pel_t *( *get_ref_line )( int, pel_t *, int, int );
    pel_t  *ref_pic       = img->type == B_IMG ? Refbuf11 [ref + 1] : Refbuf11[ref];

    int   best_pos      = 0;                                        // position with minimum motion cost
    int   max_pos       = ( 2 * search_range + 1 ) * ( 2 * search_range + 1 ); // number of search positions
    int   lambda_factor = LAMBDA_FACTOR( sqrt( img->lambda ) );                // factor for determining lagragian motion cost
    int   blocksize_y   = blc_size[mode][1] << 3;            // vertical block size
    int   blocksize_x   = blc_size[mode][0] << 3;            // horizontal block size
    int   blocksize_x8  = blocksize_x >> 3;                         // horizontal block size in 4-pel units
    int   pred_x        = ( pic_pix_x << 2 ) + pred_mv_x;     // predicted position x (in sub-pel units)
    int   pred_y        = ( pic_pix_y << 2 ) + pred_mv_y;     // predicted position y (in sub-pel units)
    int   center_x      = pic_pix_x + *mv_x;                        // center position x (in pel units)
    int   center_y      = pic_pix_y + *mv_y;                        // center position y (in pel units)

    int   height        = img->height;

    //===== set function for getting reference picture lines =====
    if ( ( center_x > search_range ) && ( center_x < img->width - 1 - search_range - blocksize_x ) &&
            ( center_y > search_range ) && ( center_y < height - 1 - search_range - blocksize_y ) )
    {
        get_ref_line = FastLineX;
    }
    else
    {
        get_ref_line = UMVLineX;
    }



    //===== loop over all search positions =====
    for ( pos = 0; pos < max_pos; pos++ )
    {
        //--- set candidate position (absolute position in pel units) ---
        cand_x = center_x + square_points_x[pos];
        cand_y = center_y + square_points_y[pos];

        //--- initialize motion cost (cost for motion vector) and check ---
        mcost = MV_COST( lambda_factor, 2, cand_x, cand_y, pred_x, pred_y );
        if ( ref != -1 )
        {
            mcost += REF_COST( lambda_factor, ref );
        }

        if ( mcost >= min_mcost )
        {
            continue;
        }

        //--- add residual cost to motion cost ---
        for ( y = 0; y < blocksize_y; y++ )
        {
            ref_line  = get_ref_line( blocksize_x, ref_pic, cand_y + y, cand_x );
            orig_line = orig_val + index_pos;
            index_pos += stride;

            for ( x8 = 0; x8 < blocksize_x8; x8++ )
            {
                mcost += byte_abs[ *orig_line++ - *ref_line++ ];
                mcost += byte_abs[ *orig_line++ - *ref_line++ ];
                mcost += byte_abs[ *orig_line++ - *ref_line++ ];
                mcost += byte_abs[ *orig_line++ - *ref_line++ ];
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

        //--- check if motion cost is less than minimum cost ---
        if ( mcost < min_mcost )
        {
            best_pos  = pos;
            min_mcost = mcost;
        }
    }

    //===== set best motion vector and return minimum motion cost =====
    if ( best_pos )
    {
        *mv_x += square_points_x[best_pos];
        *mv_y += square_points_y[best_pos];
    }

    return min_mcost;
}

/*
*************************************************************************
* Function:Calculate SA(T)D
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
int SATD( int *diff, int use_hadamard )
{
    int k, satd = 0, m[16], dd, *d = diff;

    if ( use_hadamard )
    {
        /*===== hadamard transform =====*/
        m[ 0] = d[ 0] + d[12];
        m[ 4] = d[ 4] + d[ 8];
        m[ 8] = d[ 4] - d[ 8];
        m[12] = d[ 0] - d[12];
        m[ 1] = d[ 1] + d[13];
        m[ 5] = d[ 5] + d[ 9];
        m[ 9] = d[ 5] - d[ 9];
        m[13] = d[ 1] - d[13];
        m[ 2] = d[ 2] + d[14];
        m[ 6] = d[ 6] + d[10];
        m[10] = d[ 6] - d[10];
        m[14] = d[ 2] - d[14];
        m[ 3] = d[ 3] + d[15];
        m[ 7] = d[ 7] + d[11];
        m[11] = d[ 7] - d[11];
        m[15] = d[ 3] - d[15];

        d[ 0] = m[ 0] + m[ 4];
        d[ 8] = m[ 0] - m[ 4];
        d[ 4] = m[ 8] + m[12];
        d[12] = m[12] - m[ 8];
        d[ 1] = m[ 1] + m[ 5];
        d[ 9] = m[ 1] - m[ 5];
        d[ 5] = m[ 9] + m[13];
        d[13] = m[13] - m[ 9];
        d[ 2] = m[ 2] + m[ 6];
        d[10] = m[ 2] - m[ 6];
        d[ 6] = m[10] + m[14];
        d[14] = m[14] - m[10];
        d[ 3] = m[ 3] + m[ 7];
        d[11] = m[ 3] - m[ 7];
        d[ 7] = m[11] + m[15];
        d[15] = m[15] - m[11];

        m[ 0] = d[ 0] + d[ 3];
        m[ 1] = d[ 1] + d[ 2];
        m[ 2] = d[ 1] - d[ 2];
        m[ 3] = d[ 0] - d[ 3];
        m[ 4] = d[ 4] + d[ 7];
        m[ 5] = d[ 5] + d[ 6];
        m[ 6] = d[ 5] - d[ 6];
        m[ 7] = d[ 4] - d[ 7];
        m[ 8] = d[ 8] + d[11];
        m[ 9] = d[ 9] + d[10];
        m[10] = d[ 9] - d[10];
        m[11] = d[ 8] - d[11];
        m[12] = d[12] + d[15];
        m[13] = d[13] + d[14];
        m[14] = d[13] - d[14];
        m[15] = d[12] - d[15];

        d[ 0] = m[ 0] + m[ 1];
        d[ 1] = m[ 0] - m[ 1];
        d[ 2] = m[ 2] + m[ 3];
        d[ 3] = m[ 3] - m[ 2];
        d[ 4] = m[ 4] + m[ 5];
        d[ 5] = m[ 4] - m[ 5];
        d[ 6] = m[ 6] + m[ 7];
        d[ 7] = m[ 7] - m[ 6];
        d[ 8] = m[ 8] + m[ 9];
        d[ 9] = m[ 8] - m[ 9];
        d[10] = m[10] + m[11];
        d[11] = m[11] - m[10];
        d[12] = m[12] + m[13];
        d[13] = m[12] - m[13];
        d[14] = m[14] + m[15];
        d[15] = m[15] - m[14];

        /*===== sum up =====*/
        for ( dd = diff[k = 0]; k < 16; dd = diff[++k] )
        {
            satd += ( dd < 0 ? -dd : dd );
        }
        satd >>= 1;
    }
    else
    {
        /*===== sum up =====*/
        for ( k = 0; k < 16; k++ )
        {
            satd += byte_abs [diff [k]];
        }
    }

    return satd;
}




/*
*************************************************************************
* Function:Block motion search
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

int FullMotionSearch( Macroblock *currMB,
                       int       ref,           // <--  reference frame (0... or -1 (backward))
                       int       pic_pix_x,     // <--  absolute x-coordinate of regarded AxB block
                       int       pic_pix_y,     // <--  absolute y-coordinate of regarded AxB block
                       int       mode )    // <--  block type (1-16x16 ... 7-4x4)
{
    static pel_t   orig_val [MB_SIZE*MB_SIZE];
    int       stride = MB_SIZE, index_pos = 0;
    int       pred_mv_x, pred_mv_y, mv_x, mv_y, i, j;
    int       max_value     = ( 1 << 20 );
    int       min_mcost     = max_value;
    int       pix_x_in_mb   = pic_pix_x - img->mb_pix_x;
    int       pix_y_in_mb   = pic_pix_y - img->mb_pix_y;
    int       b8_x_in_mb    = ( pix_x_in_mb >> 3 );
    int       b8_y_in_mb    = ( pix_y_in_mb >> 3 );
    int       bsize_x       = blc_size[mode][0] << 3;
    int       bsize_y       = blc_size[mode][1] << 3;
    int       refframe      = ( ref == -1 ? 0 : ref );
    int      *pred_mv;
    int     **ref_array     = ( ( img->type != B_IMG ) ? img->pfrm_ref : ref >= 0 ? img->bfrm_fref : img->bfrm_bref );
    int ***    mv_array     = ( ( img->type != B_IMG ) ? img->pfrm_mv : ( ref >= 0 ? img->bfrm_fmv : img->bfrm_bmv ) );
    int ** ***  all_mv      = ( ref < 0 ? img->bmv_com : img->fmv_com );
    uchar_t*  imgY_org_pic  = imgY_org;
    int       center_x = pic_pix_x;
    int       center_y = pic_pix_y;

    pred_mv = ( ( img->type != B_IMG ) ? img->pfmv_com : ref >= 0 ? img->pfmv_com : img->pbmv_com )[refframe][mode][pix_y_in_mb >> 3][pix_x_in_mb >> 3];

    // GET ORIGINAL BLOCK
    for ( j = 0; j < bsize_y; j++ )
    {
        for ( i = 0; i < bsize_x; i++ )
        {
            orig_val[index_pos + i] = imgY_org_pic[( pic_pix_y +j )*( img->width ) + pic_pix_x + i];
        }
        index_pos += stride;
    }

    // GET MOTION VECTOR PREDICTOR
    SetSpatialMVPredictor( img, currMB, pred_mv, ref_array, mv_array, refframe, b8_x_in_mb, b8_y_in_mb, bsize_x, ref );
    pred_mv_x = pred_mv[0];
    pred_mv_y = pred_mv[1];

    // INTEGER-PEL SEARCH
    // set search center
    mv_x = pred_mv_x / 4;
    mv_y = pred_mv_y / 4;

    // perform motion search
    min_mcost = FullPelBlockMotionSearch( orig_val, stride, ref, center_x, center_y, mode, pred_mv_x, pred_mv_y, &mv_x, &mv_y, min_mcost );

    // SUB-PEL SEARCH
    if ( input->hadamard )
    {
        min_mcost = max_value;
    }
    min_mcost = SquareFractionalMVSearchSATD( orig_val, stride, ref, center_x, center_y, mode, pred_mv_x, pred_mv_y, &mv_x, &mv_y, 9, 9, min_mcost );

    // SET MV'S AND RETURN MOTION COST
    for ( i = 0; i < ( bsize_x >> 3 ); i++ )
    {
        for ( j = 0; j < ( bsize_y >> 3 ); j++ )
        {
            all_mv[refframe][mode][b8_y_in_mb + j][b8_x_in_mb + i][0] = mv_x;
            all_mv[refframe][mode][b8_y_in_mb + j][b8_x_in_mb + i][1] = mv_y;
        }
    }

    for ( i = 0; i < ( bsize_x >> 2 ); i++ )
    {
        for ( j = 0; j < ( bsize_y >> 2 ); j++ )
        {
            if ( ref > -1 )
            {
                all_mincost[( img->mb_pix_x >> 2 ) + b8_x_in_mb + i][( img->mb_pix_y >> 2 ) + b8_y_in_mb + j][refframe][mode][1] = mv_x;
                all_mincost[( img->mb_pix_x >> 2 ) + b8_x_in_mb + i][( img->mb_pix_y >> 2 ) + b8_y_in_mb + j][refframe][mode][2] = mv_y;
            }
            else
            {
                all_bwmincost[( img->mb_pix_x >> 2 ) + b8_x_in_mb + i][( img->mb_pix_y >> 2 ) + b8_y_in_mb + j][0][mode][1] = mv_x;
                all_bwmincost[( img->mb_pix_x >> 2 ) + b8_x_in_mb + i][( img->mb_pix_y >> 2 ) + b8_y_in_mb + j][0][mode][2] = mv_y;
            }
        }
    }

    img->mv_range_flag = check_mv_range( mv_x, mv_y, pic_pix_x, pic_pix_y, mode );
    img->mv_range_flag *= check_mvd( ( mv_x - pred_mv_x ), ( mv_y - pred_mv_y ) );
    if ( !img->mv_range_flag )
    {
        min_mcost = 1 << 24;
        img->mv_range_flag = 1;
    }

    return min_mcost;
}

/*
*************************************************************************
* Function:Block motion search
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

int FullMotionSearchSym ( Macroblock *currMB,
                            int       ref,          // <--  reference frame (0... or -1 (backward))
                            int       pic_pix_x,    // <--  absolute x-coordinate of regarded AxB block
                            int       pic_pix_y,    // <--  absolute y-coordinate of regarded AxB block
                            int       mode )        // <--  block type (1-16x16 ... 7-4x4)
{
    static pel_t   orig_val [MB_SIZE*MB_SIZE];
    int       stride = MB_SIZE, index_pos = 0;
    int       pred_mv_x, pred_mv_y, mv_x, mv_y, i, j;

    int       max_value = ( 1 << 20 );
    int       min_mcost = max_value;
    int       pix_x_in_mb      = pic_pix_x - img->mb_pix_x;
    int       pix_y_in_mb      = pic_pix_y - img->mb_pix_y;
    int       b8_x_in_mb   = ( pix_x_in_mb >> 3 );
    int       b8_y_in_mb   = ( pix_y_in_mb >> 3 );
    int       bsx = blc_size[mode][0] << 3;
    int       bsy = blc_size[mode][1] << 3;
    int       refframe  = ( ref == -1 ? 0 : ref );
    int      *pred_mv;
    int     **ref_array = ( ( img->type != B_IMG ) ? img->pfrm_ref : ref >= 0 ? img->bfrm_fref : img->bfrm_bref );
    int ***    mv_array = ( ( img->type != B_IMG ) ? img->pfrm_mv   : ref >= 0 ? img->bfrm_fmv : img->bfrm_bmv );
    int ** ***  all_mv  = ( ref == -1 ? img->bmv_com : img->mv_sym_mhp );
    uchar_t*   imgY_org_pic = imgY_org;
    int       center_x = pic_pix_x;
    int       center_y = pic_pix_y;

    pred_mv = ( ref >= 0 ? img->pmv_sym_mhp : img->pbmv_com )[refframe][mode][pix_y_in_mb >> 3][pix_x_in_mb >> 3];

    // GET ORIGINAL BLOCK
    for ( j = 0; j < bsy; j++ )
    {
        for ( i = 0; i < bsx; i++ )
        {
            orig_val[index_pos + i] = imgY_org_pic[( pic_pix_y + j )*( img->width ) + pic_pix_x + i];
        }
        index_pos += stride;
    }

    // GET MOTION VECTOR PREDICTOR
    SetSpatialMVPredictor( img, currMB, pred_mv, ref_array, mv_array, refframe, pix_x_in_mb, pix_y_in_mb, bsx, ref );
    pred_mv_x = pred_mv[0];
    pred_mv_y = pred_mv[1];


    // INTEGER-PEL SEARCH
    // set search center
    mv_x = pred_mv_x / 4;
    mv_y = pred_mv_y / 4;

    // perform motion search
    min_mcost = FullPelBlockMotionSearch( orig_val,stride, ref, center_x, center_y, mode,
                                          pred_mv_x, pred_mv_y, &mv_x, &mv_y, min_mcost );


    // SUB-PEL SEARCH
    if ( input->hadamard )
    {
        min_mcost = max_value;
    }
    min_mcost =  SquareFractionalMVSearchSATDSym( orig_val,stride, ref, center_x, center_y, mode,
                 pred_mv_x, pred_mv_y, &mv_x, &mv_y, 9, 9, min_mcost );

    // SET MV'S AND RETURN MOTION COST
    for ( i = 0; i < ( bsx >> 3 ); i++ )
    {
        for ( j = 0; j < ( bsy >> 3 ); j++ )
        {
            all_mv[refframe][mode][b8_y_in_mb + j][b8_x_in_mb + i][0] = mv_x;
            all_mv[refframe][mode][b8_y_in_mb + j][b8_x_in_mb + i][1] = mv_y;
        }
    }

    for ( i = 0; i < ( bsx >> 2 ); i++ )
    {
        for ( j = 0; j < ( bsy >> 2 ); j++ )
        {
            if ( ref > -1 )
            {
                all_mincost[( img->mb_pix_x >> 2 ) + b8_x_in_mb + i][( img->mb_pix_y >> 2 ) + b8_y_in_mb + j][refframe][mode][1] = mv_x;
                all_mincost[( img->mb_pix_x >> 2 ) + b8_x_in_mb + i][( img->mb_pix_y >> 2 ) + b8_y_in_mb + j][refframe][mode][2] = mv_y;
            }
            else
            {
                all_bwmincost[( img->mb_pix_x >> 2 ) + b8_x_in_mb + i][( img->mb_pix_y >> 2 ) + b8_y_in_mb + j][0][mode][1] = mv_x;
                all_bwmincost[( img->mb_pix_x >> 2 ) + b8_x_in_mb + i][( img->mb_pix_y >> 2 ) + b8_y_in_mb + j][0][mode][2] = mv_y;
            }
        }
    }

    img->mv_range_flag = check_mv_range_bid( mv_x, mv_y, pic_pix_x, pic_pix_y, mode );
    img->mv_range_flag *= check_mvd( ( mv_x - pred_mv_x ), ( mv_y - pred_mv_y ) );
    if ( !img->mv_range_flag )
    {
        min_mcost = 1 << 24;
        img->mv_range_flag = 1;
    }

    return min_mcost;
}

/*
*************************************************************************
* Function:Find motion vector for the Skip mode
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void GetPskipMV_enc( ImgParams *img, Macroblock *currMB )
{
    int bx, by;
    int pmv[2];

    int mb_width = ( img->width >> 4 );
    int mb_nr = img->current_mb_nr;
    int mb_available_up = ( img->mb_y == 0 ) ? 0 : ( currMB->slice_nr == img->mb_data[mb_nr - mb_width].slice_nr );
    int mb_available_left = ( img->mb_x == 0 ) ? 0 : ( currMB->slice_nr == img->mb_data[mb_nr - 1].slice_nr );
    int zeroMotionAbove = !mb_available_up ? 1 : img->pfrm_ref[img->mb_b8_y - 1][img->mb_b8_x] == 0 && img->pfrm_mv[img->mb_b8_y - 1][4 + img->mb_b8_x][0] == 0 && img->pfrm_mv[img->mb_b8_y - 1][4 + img->mb_b8_x][1] == 0 ? 1 : 0;
    int zeroMotionLeft = !mb_available_left ? 1 : img->pfrm_ref[img->mb_b8_y][img->mb_b8_x - 1] == 0 && img->pfrm_mv[img->mb_b8_y][4 + img->mb_b8_x - 1][0] == 0 && img->pfrm_mv[img->mb_b8_y][4 + img->mb_b8_x - 1][1] == 0 ? 1 : 0;

    int ***fmv = img->fmv_com[0][0];
    int *mv    = img->pfmv_com[0][0][0][0];


    if ( zeroMotionAbove || zeroMotionLeft )
    {
        pmv[0] = pmv[1] = 0;
    }
    else
    {
        SetSpatialMVPredictor( img, currMB, pmv, img->pfrm_ref, img->pfrm_mv, 0, 0, 0, 16, 0 );
        mv[0] = pmv[0];
        mv[1] = pmv[1];
    }

    for ( by = 0; by < 2; by++ )
    {
        for ( bx = 0; bx < 2; bx++ )
        {
            fmv[by][bx][0] = pmv[0];
            fmv[by][bx][1] = pmv[1];
        }
    }

    img->mv_range_flag = check_mv_range( mv[0], mv[1], img->mb_pix_x, img->mb_pix_y, 1 );
}

static void PartitionMotionSearch(Macroblock *currMB, int mode, int pu_idx)
{
    int   ref, refinx, refframe, mcost;
    int   pu_b8_x, pu_b8_y;
    int   bframe = ( img->type == B_IMG );

    int b8 = ( mode == P2NxN )? pu_idx * 2 : pu_idx; // translate pu_idx to b8
    int b8_y = b8 / 2;
    int b8_x = b8 % 2;
    int min_ref   = ( bframe ? -1 : 0 );
    int max_ref = bframe ? 1 : MIN( img->real_ref_num, af_intra_cnt );

    // loop over reference frames
    for ( ref = min_ref; ref < max_ref; ref++ )
    {
        if ( img->type == P_IMG && ref_idx[ref] < 0 )
        {
            continue;
        }
        refinx    = bframe ? ref + 1 : ref ;
        refframe  = ( ref < 0 ? 0 : ref );

        motion_cost[mode][refinx][pu_idx] = 0;

        pu_b8_y = img->mb_b8_y + b8_y;
        pu_b8_x = img->mb_b8_x + b8_x;

        // motion search for block
        if ( input->usefme )
        {
            mcost = FastMVSeach( currMB, ref, pu_b8_x, pu_b8_y, mode );  //should modify the constant?
        }
        else
        {
            mcost = FullMotionSearch( currMB, ref, 8 * pu_b8_x, 8 * pu_b8_y, mode );
        }

        motion_cost[mode][refinx][pu_idx] += mcost;
    }
}

static void PartitionMotionSearchMhp( Macroblock *currMB, int mode, int pu_idx )
{
    int ref, mcost;
    int pu_b8_x, pu_b8_y;

    int b8 = ( mode == P2NxN )? pu_idx * 2 : pu_idx; // translate pu_idx to b8
    int b8_y = b8 / 2;
    int b8_x = b8 % 2;
    int max_ref = MIN( img->real_ref_num, af_intra_cnt );

    assert( img->type == P_IMG );

    // loop over reference frames
    for ( ref = 0; ref < max_ref; ref++ )
    {
        if ( ref_idx[ref] < 0 )
        {
            continue;
        }

        pu_b8_y = img->mb_b8_y + b8_y;
        pu_b8_x = img->mb_b8_x + b8_x;

        if ( input->usefme )
        {
            mcost = FastMotionSearchMhp( currMB, ref, pu_b8_x, pu_b8_y, mode );
        }
        else
        {
            mcost = FullMotionSearch( currMB, ref, 8 * pu_b8_x, 8 * pu_b8_y, mode );
        }

        motion_cost[mode][ref][pu_idx] = mcost;
    }
}

static void PartitionMotionSearchSym(Macroblock *currMB, int mode, int pu_idx)
{
    int ref, mcost;
    int pu_b8_x, pu_b8_y;

    int b8 = ( mode == P2NxN )? pu_idx * 2 : pu_idx; // translate pu_idx to b8
    int b8_y = b8 / 2;
    int b8_x = b8 % 2;
    int max_ref = MIN( 1, img->real_ref_num );


    // loop over reference frames
    for ( ref = 0; ref < max_ref; ref++ )
    {
        pu_b8_y = img->mb_b8_y + b8_y;
        pu_b8_x = img->mb_b8_x + b8_x;

        if ( input->usefme )
        {
            mcost = FastMotionSearchSym( currMB, ref, pu_b8_x, pu_b8_y, mode );
        }
        else
        {
            mcost = FullMotionSearchSym( currMB, ref, 8 * pu_b8_x, 8 * pu_b8_y, mode );
        }
        motion_cost_sym[mode][ref + 1][pu_idx] = mcost;
    }
}

void ForwardMVSearch( Macroblock *currMB, int *fw_mcost, int *best_fw_ref, int *best_bw_ref, int mode, int block )
{
    int ref;
    int mcost;
    int m_ref = MIN( img->real_ref_num, af_intra_cnt );

    PartitionMotionSearch( currMB, mode, block );

    // get cost and reference frame for forward prediction
    if ( img->type == P_IMG )
    {
        for ( *fw_mcost = MAX_COST, ref = 0; ref < m_ref ; ref++ )
        {
            if ( img->type == P_IMG && ref_idx[ref] < 0 )
            {
                continue;
            }
            mcost = motion_cost[mode][ref][block];
            if ( mcost < *fw_mcost )
            {
                *fw_mcost    = mcost;
                *best_fw_ref = ref;
            }
        }
        *best_bw_ref = 0;
    }
    else
    {
        *fw_mcost = MAX_COST;
        mcost = motion_cost[mode][1][block];
        if ( mcost < *fw_mcost )
        {
            *fw_mcost    = mcost;
            *best_fw_ref = 0;
        }
        *best_bw_ref = 0;
    }
}

void ForwardMVSearchMhp( Macroblock *currMB, int *p_bid_mcost, int *best_fw_ref, int *best_bw_ref, int mode, int block )
{
    int ref;
    int mcost;
    int m_ref = MIN( img->real_ref_num, af_intra_cnt );
    assert( img->type == P_IMG );

    PartitionMotionSearchMhp( currMB, mode, block );

    // get cost and reference frame for forward prediction
    for ( *p_bid_mcost = MAX_COST, ref = 0; ref < m_ref ; ref++ )
    {
        if ( ref_idx[ref] < 0 )
        {
            continue;
        }
        mcost  = motion_cost[mode][ref][block];
        if ( mcost < *p_bid_mcost )
        {
            *p_bid_mcost    = mcost;
            *best_fw_ref = ref;
        }
    }
    *best_bw_ref = 0;
}

void BidirectionalMVSearch( Macroblock *currMB, int *best_bw_ref, int *bw_mcost, int *bid_mcost, int *bid_best_fw_ref, int *bid_best_bw_ref, int mode, int block )
{
    int mcost;
    int ref;
    int m_ref = MIN( img->ref_num, 1 );

    assert( img->type == B_IMG );
    // get cost for bidirectional prediction
    PartitionMotionSearchSym( currMB, mode, block );
    *best_bw_ref = 0;

    for ( *bw_mcost = MAX_COST, ref = 0; ref < m_ref; ref++ )
    {
        mcost = motion_cost[mode][ref][block];
        if ( mcost < *bw_mcost )
        {
            *bw_mcost    = mcost;
            *best_bw_ref = ref;
        }
    }

    for ( *bid_mcost = MAX_COST, ref = 0; ref < m_ref; ref++ )
    {
        mcost = motion_cost_sym[mode][ref + 1][block];
        if ( mcost < *bid_mcost )
        {
            *bid_mcost    = mcost;
            *bid_best_fw_ref = ref;
            *bid_best_bw_ref = ref;
        }
    }
}

void GetBdirectMV_enc( Macroblock *currMB )
{
    int  blk_x, blk_y, img_b8_x, img_b8_y;
    for ( blk_y = 0; blk_y < 2; blk_y++ )
    {
        img_b8_y = img->mb_b8_y + blk_y;

        for ( blk_x = 0; blk_x < 2; blk_x++ )
        {
            int *fmv = img->fmv_com[0][0][blk_y][blk_x];
            int *bmv = img->bmv_com[0][0][blk_y][blk_x];
            img_b8_x = img->mb_b8_x + blk_x;
            GetBdirectMV( img, currMB, 16, fmv, bmv, img_b8_x, img_b8_y );
            img->mv_range_flag = check_mv_range( fmv[0], fmv[1], img->mb_x + ( blk_x << 3 ), img->mb_y + ( blk_y << 3 ), 0 );
            img->mv_range_flag *= check_mv_range( bmv[0], bmv[1], img->mb_x + ( blk_x << 3 ), img->mb_y + ( blk_y << 3 ), 0 );
        }
    }
}

/**************************************************************************
* Function:control the sign of a with b
**************************************************************************/
int sign( int a, int b )
{
    int x;
    x = absm( a );

    if ( b >= 0 )
    {
        return x;
    }
    else
    {
        return -x;
    }
}