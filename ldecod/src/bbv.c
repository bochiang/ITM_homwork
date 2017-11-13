/***************************************************************************************

***************************************************************************************/
// ===================================================================================
//  File Name:   bbv.c
//  Description: bbv buffer for rate control
// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------
// ===================================================================================

#include "bbv.h"
#include <assert.h>

/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
BbvBuffer_t* init_bbv_memory( int frame_rate_code, int low_delay, int bbv_buffer_size, int bit_rate_upper, int bit_rate_lower )
{
    BbvBuffer_t *pBbv;
    static float frame_rate_table[] = {23.97f, 24.00f, 25.00f, 29.97f, 30.00f, 50.00f, 59.97f, 60.00f};

    if ( NULL == ( pBbv = ( struct BbvBuffer_t* )calloc( 1, sizeof( BbvBuffer_t ) ) ) )
    {
        printf( "memory for bbv buffer have error.\n" );
        return NULL;
    }

    if ( NULL == ( pBbv->FrameBits = ( int* )calloc( 1, 1000000*sizeof( int ) ) ) )
    {
        printf( "memory for frame bits of bbv buffer have error.\n" );
        return NULL;
    }

    pBbv->frame_code_bits = 0;
    pBbv->framerate       = frame_rate_table[frame_rate_code - 1];
    pBbv->frm_no          = 0;
    pBbv->bbv_mode        = 0;
    pBbv->low_delay       = ( float )low_delay;
    pBbv->frmout_interval = ( float )( 1.0/pBbv->framerate );
    pBbv->bitrate         = ( ( bit_rate_upper<<18 ) + bit_rate_lower )*400;    // bit per second
    pBbv->BBS_size        = bbv_buffer_size<<14;
    pBbv->check_continue  = 1;
    pBbv->currFrm_max_bit = pBbv->BBS_size;
    pBbv->currFrm_min_bit = 0;

    return pBbv;
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
void stat_bbv_buffer( BbvBuffer_t* pBbv )
{
    int minBBSsize, minFsize;

    calc_min_BBS_size( pBbv->FrameBits, pBbv->bitrate, pBbv->framerate, pBbv->frm_no, &minBBSsize, &minFsize );
    printf( "\nmin bbv_buffer_size is %d\n", ( minBBSsize + 16384 )>>14 ); // ITM_r1
    if ( !pBbv->bbv_mode ) // ITM_r1
    {
        printf( "min initial bbv_delay(0) time is %.4f(s)\n\n", ( float )minFsize/pBbv->bitrate );
    }
    else
    {
        printf( "\n\n" );
    }
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
BbvBuffer_t* free_bbv_memory( BbvBuffer_t* pBbv )
{
    free( pBbv->FrameBits );
    pBbv->FrameBits = NULL;
    free( pBbv );
    pBbv = NULL;

    return pBbv;
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
void update_bbv( BbvBuffer_t* pBbv, int code_bits )
{
    pBbv->FrameBits[pBbv->frm_no++] = code_bits;
    if ( ( code_bits < pBbv->currFrm_min_bit ) && !pBbv->low_delay && pBbv->check_continue )
    {
        fprintf( stdout,"Overflow will occur at the next time intervals between successive examination!\nThe removing data at the current decoding time (Image NO.%d) is %d.\n The removing data should be %d in order to avoid overflow.\n"
                 ,img->FrmNum-1+!( img->type==B_IMG ),code_bits,pBbv->currFrm_min_bit );
        bValidSyntax              = 0;
        pBbv->check_continue = 0;
    }
    if ( ( code_bits > pBbv->currFrm_max_bit ) && pBbv->check_continue )
    {
        fprintf( stdout,"Underflow occurs at the current decoding time (Image NO.%d)!\nThe data in BBV buffer is %d, whereas the removing data is %d.\n"
                 ,img->FrmNum-1+!( img->type==B_IMG ),pBbv->currFrm_max_bit,code_bits );
        bValidSyntax              = 0;
        pBbv->check_continue = 0;
    }

    if ( pBbv->bbv_mode ) // 0xFFFF
    {
        pBbv->currFrm_max_bit = MIN( pBbv->BBS_size, ( int )( pBbv->currFrm_max_bit - code_bits + pBbv->bitrate*pBbv->frmout_interval + 0.5 ) );
        pBbv->currFrm_min_bit = 0;
    }
    else // not 0xFFFF
    {
        pBbv->bbv_delay += pBbv->frmout_interval - ( ( float )code_bits )/( ( float )pBbv->bitrate );
        pBbv->currFrm_max_bit = ( int )( pBbv->bbv_delay*pBbv->bitrate + 0.5 );
        pBbv->currFrm_min_bit = MAX( 0, ( int )( ( pBbv->bbv_delay + pBbv->frmout_interval )*pBbv->bitrate - pBbv->BBS_size + 0.5 ) );
    }
    assert(bValidSyntax);
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
void calc_min_BBS_size( int* FrameBits, int BitRate, float FrameRate, int FrameNum, int *Bmin_out, int *Fmin_out )
{
    float B;
    float *Buff1, *Buff2;
    float minbuff;
    float Bmin, Fmin;
    int   i;

    Buff1 = ( float* )calloc( FrameNum + 1, sizeof( float ) );
    Buff2 = ( float* )calloc( FrameNum,     sizeof( float ) );
    B = ( float )( BitRate*20 );

    minbuff = Buff1[FrameNum] = B;
    for ( i = FrameNum - 1; i>=0; i-- )
    {
        Buff2[i] = Buff1[i + 1] - FrameBits[i];
        if ( Buff2[i]< minbuff )
        {
            minbuff = Buff2[i];
        }
        Buff1[i] = Buff2[i] + BitRate/FrameRate;
        if ( Buff1[i] > B )
        {
            Buff1[i] = B;
        }
    }
    Bmin = B - minbuff;
    *Bmin_out = ( int )Bmin;

    B = Bmin;
    Fmin = 0;
    Buff1[0] = Fmin;
    for ( i = 0; i<FrameNum; i++ )
    {
        Buff2[i] = Buff1[i] - FrameBits[i];
        if ( Buff2[i]< 0 )
        {
            Fmin += ( 0 - Buff2[i] );
            Buff2[i] = 0;
        }
        Buff1[i+1] = Buff2[i] + BitRate/FrameRate;
        if ( Buff1[i+1] > B )
        {
            Buff1[i+1] = B;
        }
    }
    *Fmin_out = ( int )Fmin;

    free( Buff1 );
    free( Buff2 );
}