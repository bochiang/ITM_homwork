#include "common.h"
#include "defines.h"
#include "global.h"

const int cst_log_level = 2;

funs_handle_t g_funs_handle;

#if DEBUG_TEST
FILE* g_debug_fp = NULL;
#endif

void biari_init_context_logac ( BiContextTypePtr ctx )
{
    ctx->LG_PMPS = ( QUARTER << LG_PMPS_SHIFTNO ) - 1; //10 bits precision
    ctx->MPS     = 0;
    ctx->cycno   = 0;
}


#define BIARI_CTX_INIT1_LOG(jj,ctx)          \
{                                            \
    for (j=0; j<jj; j++) {                   \
        biari_init_context_logac(&(ctx[j])); \
    }                                        \
}

#define BIARI_CTX_INIT2_LOG(ii,jj,ctx)              \
{                                                   \
    for (i=0; i<ii; i++) {                          \
        for (j=0; j<jj; j++) {                      \
            biari_init_context_logac(&(ctx[i][j])); \
        }                                           \
    }                                               \
}



void init_contexts( CSobj *cs_aec )
{
    MotionInfoContexts*  mc = cs_aec->mot_ctx;
    TextureInfoContexts* tc = cs_aec->tex_ctx;
    int i, j;

    //--- motion coding contexts ---
    BIARI_CTX_INIT2_LOG ( 3, NUM_MB_TYPE_CTX,   mc->mb_type_contexts );
    BIARI_CTX_INIT2_LOG ( 2, NUM_B8_TYPE_CTX,   mc->b8_type_contexts );
    BIARI_CTX_INIT2_LOG ( 2, NUM_MV_RES_CTX,    mc->mv_res_contexts );
    BIARI_CTX_INIT1_LOG (   NUM_DELTA_QP_CTX,  mc->delta_qp_contexts );

    //--- texture coding contexts ---
    BIARI_CTX_INIT1_LOG ( 1, tc->qsplit_contexts );
    BIARI_CTX_INIT1_LOG (                 NUM_IPR_CTX,  tc->ipr_contexts )
    BIARI_CTX_INIT1_LOG (                 NUM_CIPR_CTX, tc->cipr_contexts );
    BIARI_CTX_INIT2_LOG ( 3,               NUM_CBP_CTX,  tc->cbp_contexts );
    BIARI_CTX_INIT2_LOG ( NUM_BLOCK_TYPES, NUM_ABS_CTX,  tc->abs_contexts );
    BIARI_CTX_INIT2_LOG ( NUM_BLOCK_TYPES, NUM_MAP_CTX,  tc->map_contexts );
    BIARI_CTX_INIT2_LOG ( NUM_BLOCK_TYPES, NUM_LAST_CTX, tc->last_contexts );
    BIARI_CTX_INIT2_LOG ( NUM_BLOCK_TYPES, NUM_ONE_CTX,  tc->one_contexts )
    BIARI_CTX_INIT1_LOG (                             1, tc->tu_size_context );
    BIARI_CTX_INIT1_LOG (              NUM_DELTA_QP_CTX, tc->MBdeltaQP_contexts);
}

/****************************************************************************
 * com_malloc: malloc & free
 ****************************************************************************/
void *com_malloc( int i_size )
{
    int mask = ALIGN_BASIC - 1;
    uchar_t *align_buf;
    uchar_t *buf = ( uchar_t * )malloc( i_size + mask + sizeof( void ** ) );

    if ( buf )
    {
        align_buf = buf + mask + sizeof( void ** );
        align_buf -= ( intptr_t )align_buf & mask;
        *( ( ( void ** )align_buf ) - 1 ) = buf;
    }
    else
    {
        printf( "Error: malloc of size %d failed\n", i_size );
    }
    memset( align_buf, 0, i_size );
    return align_buf;
}

void com_free( void *p )
{
    if ( p )
    {
        free( *( ( ( void ** )p ) - 1 ) );
    }
    else
    {
        printf( "free a NULL pointer\n" );
    }
}

#if TRACE
void output_trace_info( char *notes, int val )
{
    if( img->writeBSflag )
    {
        fprintf( p_trace, notes, val );
        fprintf( p_trace, "\n" );
    }
}
#endif

/*!
************************************************************************
* \brief
*    returns the x and y sample coordinates for a given MbAddress.
*    Refer to Spec. 4.12.9
************************************************************************
*/
static void xGet_mb_pos(int mb_addr, int *x, int *y)
{
    *x = (mb_addr % img->PicWidthInMbs);
    *y = (mb_addr / img->PicWidthInMbs);
    (*x) *= MB_SIZE;
    (*y) *= MB_SIZE;
}

/*!
************************************************************************
* \brief
*    get neighboring positions.
*    Refer to Spec. 4.12.9
* \param curr_mb_nr
*   current macroblock number (decoding order)
* \param xN
*    input x position
* \param yN
*    input y position
* \param luma
*    1 if luma coding, 0 for chroma
* \param pix
*    returns position informations
* \author
*    added by
************************************************************************
*/
void getNeighbour(Macroblock *currMb, int curr_mb_nr, int xN, int yN, int isLuma, PixelPos *pix)
{
    int maxWH = isLuma ? 16 : 8;

    if ((xN < 0) && (yN < 0))
    {
        pix->mb_addr = currMb->mb_addr_leftup;
        pix->available = currMb->mbAvailD;
    }
    else if ((xN < 0) && ((yN >= 0) && (yN < maxWH)))
    {
        pix->mb_addr = currMb->mb_addr_left;
        pix->available = currMb->mbAvailA;
    }
    else if (((xN >= 0) && (xN < maxWH)) && (yN < 0))
    {
        pix->mb_addr = currMb->mb_addr_up;
        pix->available = currMb->mbAvailB;
    }
    else if (((xN >= 0) && (xN < maxWH)) && ((yN >= 0) && (yN < maxWH)))
    {
        pix->mb_addr = curr_mb_nr;
        pix->available = 1;
    }
    else if ((xN >= maxWH) && (yN < 0))
    {
        pix->mb_addr = currMb->mb_addr_rightup;
        pix->available = currMb->mbAvailC;
    }
    else
    {
        pix->available = 0;
    }
    if (pix->available)
    {
        pix->x = (xN + maxWH) % maxWH;
        pix->y = (yN + maxWH) % maxWH;
        xGet_mb_pos(pix->mb_addr, &(pix->pos_x), &(pix->pos_y));
        if (isLuma)
        {
            pix->pos_x += pix->x;
            pix->pos_y += pix->y;
        }
        else
        {
            pix->pos_x = (pix->pos_x / 2) + pix->x;
            pix->pos_y = (pix->pos_y / 2) + pix->y;
        }
    }
}

/*!
************************************************************************
* \brief
*    get neighboring get neighboring 8x8 luma block
*    Refer to Spec. 4.12.8.3
* \param curr_mb_nr
*   current macroblock number (decoding order)
* \param block_x
*    input x block position
* \param block_y
*    input y block position
* \param rel_x
*    relative x position of neighbor
* \param rel_y
*    relative y position of neighbor
* \param pix
*    returns position informations
* \author
*    added by
************************************************************************
*/

void getLuma8x8Neighbour(Macroblock *currMB, int curr_mb_nr, int b8, int rel_x, int rel_y, PixelPos *pix)
{
    int b8_x = b8 % 2;
    int b8_y = b8 / 2;
    int pix_x_in_mb = B8_SIZE * b8_x + rel_x;
    int pix_y_in_mb = B8_SIZE * b8_y + rel_y;

    int b8_dest;
    int pdir, pdir_dest;
    int isLuma = 1;
    getNeighbour(currMB, curr_mb_nr, pix_x_in_mb, pix_y_in_mb, isLuma, pix);

    if (pix->available)
    {
        b8_dest = pix->x / B8_SIZE + pix->y / B8_SIZE * 2;
        pdir = img->mb_data[curr_mb_nr].b8pdir[b8];
        pdir_dest = img->mb_data[pix->mb_addr].b8pdir[b8_dest];
        if ((pdir == FORWARD && pdir_dest == BACKWORD) || (pdir == BACKWORD && pdir_dest == FORWARD) || (pdir == BSYM && pdir_dest == BACKWORD) || (pdir == BACKWORD && pdir_dest == BSYM))
        {
            pix->available = 0;
        }

        pix->x /= B8_SIZE;
        pix->y /= B8_SIZE;
        pix->pos_x /= B8_SIZE;
        pix->pos_y /= B8_SIZE;
    }
}