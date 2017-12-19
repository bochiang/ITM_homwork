#include "intraPrediction.h"
#include "assert.h"
#include "intra_add.h"

void xPrepareIntraPattern( ImgParams *img, Macroblock *currMB, pel_t *pedge, int img_x, int img_y, int bsize, bool *bAboveAvail, bool *bLeftAvail )
{
    int x, y;
    int bs_x = bsize;
    int bs_y = bsize;
    int MBRowSize = img->PicWidthInMbs;
    const int mb_nr = img->current_mb_nr;

    int pix_x_in_mb = img_x - img->mb_pix_x;
    int pix_y_in_mb = img_y - img->mb_pix_y;

    bool mb_left_available = ( img->mb_pix_x > 0 ) ? currMB->slice_nr == img->mb_data[mb_nr - 1].slice_nr : 0;
    bool mb_up_available = ( img->mb_pix_y > 0 ) ? currMB->slice_nr == img->mb_data[mb_nr - MBRowSize].slice_nr : 0;
    bool mb_up_right_available = ( img->mb_pix_y > 0 && ( img->mb_pix_x + MB_SIZE < img->width ) ) ? currMB->slice_nr == img->mb_data[mb_nr - MBRowSize + 1].slice_nr : 0;

    bool block_available_up, block_available_left, block_available_up_right, block_available_left_down;

    if ( bsize == 4 )
    {
        int x_b8_in_mb = ( pix_x_in_mb >> 3 ) << 3;
        int y_b8_in_mb = ( pix_y_in_mb >> 3 ) << 3;
        int x_in_b8 = pix_x_in_mb - x_b8_in_mb;
        int y_in_b8 = pix_y_in_mb - y_b8_in_mb;
        bool b8_available_up = ( y_b8_in_mb > 0 || mb_up_available );
        bool b8_available_left = ( x_b8_in_mb > 0 || mb_left_available );
        bool b8_available_up_right = ( x_b8_in_mb + 8 == 16 ) ? ( y_b8_in_mb == 0 ? mb_up_right_available : 0 ) : ( y_b8_in_mb == 0 ? mb_up_available : 1 );
        bool b8_available_left_down = ( x_b8_in_mb == 0 && y_b8_in_mb + 8 != 16 ) ? b8_available_left : 0;

        block_available_up = ( y_in_b8 > 0 || b8_available_up );
        block_available_left = ( x_in_b8 > 0 || b8_available_left );
        block_available_up_right = ( x_in_b8 + bs_x == 8 ) ? ( y_in_b8 == 0 ? b8_available_up_right : 0 ) : ( y_in_b8 == 0 ? b8_available_up : 1 );
        block_available_left_down = ( x_in_b8 != 0 ) ? 0 : ( y_in_b8 + bs_y != 8 ? block_available_left : b8_available_left_down );
    }
    else
    {
        block_available_up        = ( pix_y_in_mb > 0 || mb_up_available );
        block_available_left      = ( pix_x_in_mb > 0 || mb_left_available );
        block_available_up_right  = ( pix_x_in_mb + bs_x == 16 ) ? ( pix_y_in_mb == 0 ? mb_up_right_available : 0 ) : ( pix_y_in_mb == 0 ? mb_up_available : 1 );
        block_available_left_down = ( pix_x_in_mb == 0 && pix_y_in_mb + bs_y != 16 ) ? 1 : 0;
    }

    // returned values
    *bAboveAvail = block_available_up;
    *bLeftAvail = block_available_left;


    //get prediction pixels
    if ( block_available_up )
    {
        for ( x = 0; x < bs_x; x++ )
        {
            pedge[x + 1] = imgY_rec[( img_y - 1 )*( img->iStride ) + img_x + x];
        }

        if ( block_available_up_right )
        {
            for ( x = 0; x < bs_x; x++ )
            {
                pedge[1 + x + bs_x] = imgY_rec[( img_y - 1 )*( img->iStride ) + img_x + bs_x + x];
            }
            for ( ; x < bs_y; x++ )
            {
                pedge[1 + x + bs_x] = imgY_rec[( img_y - 1 )*( img->iStride ) + img_x + bs_x + bs_x - 1];
            }
        }
        else
        {
            for ( x = 0; x < bs_y; x++ )
            {
                pedge[1 + x + bs_x] = pedge[bs_x];
            }
        }

        for ( ; x < bs_y + 2; x++ )
        {
            pedge[1 + x + bs_x] = pedge[bs_x + x];
        }

        pedge[0] = imgY_rec[( img_y - 1 )*( img->iStride ) + img_x];
    }
    if ( block_available_left )
    {
        for ( y = 0; y < bs_y; y++ )
        {
            pedge[-1 - y] = imgY_rec[( img_y + y )*( img->iStride ) + img_x - 1];
        }

        if ( block_available_left_down )
        {
            for ( y = 0; y < bs_y; y++ )
            {
                pedge[-1 - y - bs_y] = imgY_rec[( img_y + bs_y + y )*( img->iStride ) + img_x - 1];
            }
            for ( ; y < bs_x; y++ )
            {
                pedge[-1 - y - bs_y] = imgY_rec[( img_y + bs_y + bs_y - 1 )*( img->iStride ) + img_x - 1];
            }
        }
        else
        {
            for ( y = 0; y < bs_x; y++ )
            {
                pedge[-1 - y - bs_y] = pedge[-bs_y];
            }
        }

        for ( ; y < bs_x + 2; y++ )
        {
            pedge[-1 - y - bs_y] = pedge[-y - bs_y];
        }

        pedge[0] = imgY_rec[img_y*( img->iStride ) + img_x - 1];
    }

    if ( block_available_up&&block_available_left )
    {
        pedge[0] = imgY_rec[( img_y - 1 )*( img->iStride ) + img_x - 1];
    }

	// Actually, two reference samples (c[-1], r[-1] in Spec.) for DC are indicated in "xPredIntraLumaDC".
    //if ( block_available_up && !block_available_left )
    //{
    //    pedge[-1] = pedge[0];
    //}
    //if ( !block_available_up && block_available_left )
    //{
    //    pedge[1] = pedge[0];
    //}
}

void xPrepareIntraPatternC( ImgParams *img, Macroblock *currMB, pel_t *pedge, int uv, int bsize, bool *bAboveAvail, bool *bLeftAvail )
{
    uchar_t *img_uv;
    int bs_x = bsize;
    int bs_y = bsize;
    int x, y;
    int img_cx   = img->mb_pix_x_cr;
    int img_cy   = img->mb_pix_y_cr;
    int b8_x     = img->mb_pix_x_cr >> 2;
    int mb_nr    = img->current_mb_nr;
    int mb_width = img->PicWidthInMbs;

    bool mb_available_up_right = ( ( img_cy==0 )||( b8_x>=( img->width_cr/BLOCK_SIZE-2 ) ) ) ? 0 : ( currMB->slice_nr == img->mb_data[mb_nr-mb_width+1].slice_nr );
    bool mb_available_up       = ( img_cy == 0 ) ? 0 : ( currMB->slice_nr == img->mb_data[mb_nr-mb_width].slice_nr );
    bool mb_available_left     = ( img_cx == 0 ) ? 0 : ( currMB->slice_nr == img->mb_data[mb_nr-1].slice_nr );

    *bAboveAvail = mb_available_up;
    *bLeftAvail = mb_available_left;

    img_uv = ( uv == 0 ) ? imgU_rec : imgV_rec;
    if( mb_available_up )
    {
        for ( x = 0; x < bs_x; x++ )
        {
            pedge[x + 1] = img_uv[( img_cy - 1 )*( img->iStrideC ) + img_cx + x];
        }

        if( mb_available_up_right )
        {
            for ( x = 0; x < bs_y; x++ )
            {
                pedge[1 + x + bs_x] = img_uv[( img_cy - 1 )*( img->iStrideC ) + img_cx + bs_x + x];
            }
        }
        else
        {
            for ( x = 0; x < bs_y; x++ )
            {
                pedge[1 + x + bs_x] = pedge[bs_x];
            }
        }
        pedge[0] = img_uv[( img_cy - 1 )* ( img->iStrideC ) + img_cx];
    }

    if( mb_available_left )
    {
        for ( y = 0; y < bs_y; y++ )
        {
            pedge[-1 - y] = img_uv[( img_cy + y )*( img->iStrideC ) + img_cx - 1];
        }

        for ( y = 0; y < bs_x; y++ )
        {
            pedge[-1 - y - bs_y] = pedge[-bs_y];
        }
        pedge[0] = img_uv[( img_cy )*( img->iStrideC ) + img_cx - 1];
    }

    if ( mb_available_up && mb_available_left )
    {
        pedge[0] = img_uv[( img_cy - 1 )*( img->iStrideC ) + img_cx - 1];
    }
}

// intra-pred mode operation
void xPredIntraLumaDC( uchar_t *pSrc, pel_t *pDst, int i_dst, int iBlkWidth, int iBlkHeight, bool bAboveAvail, bool bLeftAvail )
{
    int x, y, tmp;

    if ( !bAboveAvail && !bLeftAvail )
    {
        for ( y = 0; y < iBlkHeight; y++ )
        {
            for ( x = 0; x < iBlkWidth; x++ )
            {
                pDst[x] = 128;
            }
            pDst += i_dst;
        }
    }
    else if ( bAboveAvail && !bLeftAvail )
    {
        for ( y = 0; y < iBlkHeight; y++ )
        {
            for ( x = 0; x < iBlkWidth; x++ )
            {
                pDst[x] = ( pel_t )( ( (x==0 ? pSrc[x] : pSrc[x - 1]) + 4 * pSrc[x] + 6 * pSrc[1 + x] + 4 * pSrc[2 + x] + pSrc[3 + x] + 8 ) >> 4 );
            }
            pDst += i_dst;
        }
    }
    else if ( !bAboveAvail && bLeftAvail )
    {
        for ( y = 0; y < iBlkHeight; y++ )
        {
            tmp = ( pel_t )( ( (y==0 ? pSrc[-y] : pSrc[-y + 1]) + 4 * pSrc[-y] + 6 * pSrc[-1 - y] + 4 * pSrc[-2 - y] + pSrc[-3 - y] + 8 ) >> 4 );
            for ( x = 0; x < iBlkWidth; x++ )
            {
                pDst[x] = tmp;
            }
            pDst += i_dst;
        }
    }
    else if ( bAboveAvail && bLeftAvail )
    {
        for ( y = 0; y < iBlkHeight; y++ )
        {
            if( y == 0 )
            {
                tmp = ( ( pSrc[-y] + 4 * pSrc[-y] + 6 * pSrc[-1 - y] + 4 * pSrc[-2 - y] + pSrc[-3 - y] + 8 ) >> 4 );
            }
            else
            {
                tmp = ( ( pSrc[-y + 1] + 4 * pSrc[-y] + 6 * pSrc[-1 - y] + 4 * pSrc[-2 - y] + pSrc[-3 - y] + 8 ) >> 4 );
            }

            for ( x = 0; x < iBlkWidth; x++ )
            {
                if( x == 0 )
                {
                    pDst[x] = ( pel_t )( ( ( ( pSrc[x] + 4 * pSrc[x] + 6 * pSrc[1 + x] + 4 * pSrc[2 + x] + pSrc[3 + x] + 8 ) >> 4 ) + tmp ) >> 1 );
                }
                else
                {
                    pDst[x] = ( pel_t )( ( ( ( pSrc[x - 1] + 4 * pSrc[x] + 6 * pSrc[1 + x] + 4 * pSrc[2 + x] + pSrc[3 + x] + 8 ) >> 4 ) + tmp ) >> 1 );
                }
            }
            pDst += i_dst;
        }
    }
}

void xPredIntraVer( uchar_t *pSrc, pel_t *pDst, int i_dst, int iBlkWidth, int iBlkHeight )
{
    int x, y;

    for ( y = 0; y < iBlkHeight; y++ )
    {
        for ( x = 0; x < iBlkWidth; x++ )
        {
            pDst[x] = pSrc[x + 1];
        }
        pDst += i_dst;
    }
}

void xPredIntraHor( uchar_t *pSrc, pel_t *pDst, int i_dst, int iBlkWidth, int iBlkHeight )
{
    int x, y;

    for ( y = 0; y < iBlkHeight; y++ )
    {
        for ( x = 0; x < iBlkWidth; x++ )
        {
            pDst[x] = pSrc[-1 - y];
        }
        pDst += i_dst;
    }
}

void xPredIntraDownRight( uchar_t *pSrc, pel_t *pDst, int i_dst, int iBlkWidth, int iBlkHeight )
{
    int x, y;

    for ( y = 0; y < iBlkHeight; y++ )
    {
        for ( x = 0; x < iBlkWidth; x++ )
        {
            pDst[x] = pSrc[x - y];
        }
        pDst += i_dst;
    }
}

void xPredIntraDownLeft( uchar_t *pSrc, pel_t *pDst, int i_dst, int iBlkWidth, int iBlkHeight )
{
    int x, y;

    for ( y = 0; y < iBlkHeight; y++ )
    {
        for ( x = 0; x < iBlkWidth; x++ )
        {
            pDst[x] = ( pSrc[2 + x + y] + pSrc[-2 - ( x + y )] ) >> 1;
        }
        pDst += i_dst;
    }
}

void xPredIntraChromaPlane( uchar_t *pSrc, pel_t *pDst, int i_dst, int iBlkWidth, int iBlkHeight )
{
    int x, y, i;
    int ih, iv, ib, ic, iaa;
    ih = 0;
    iv = 0;

    for ( i = 1; i < 5; i++ )
    {
        ih += i*( pSrc[1 + i + 3] - pSrc[1 - i + 3] );
        iv += i*( pSrc[-1 - i - 3] - pSrc[-1 + i - 3] );
    }

    ib = ( 17 * ih + 16 ) >> 5;
    ic = ( 17 * iv + 16 ) >> 5;
    iaa = 16 * ( pSrc[1 + 7] + pSrc[-1 - 7] );

    for ( y = 0; y < iBlkHeight; y++ )
    {
        for ( x = 0; x < iBlkWidth; x++ )
        {
            pDst[x] = ( pel_t )MAX( 0, MIN( 255, ( iaa + ( x - 3 )*ib + ( y - 3 )*ic + 16 ) / 32 ) );
        }
        pDst += i_dst;
    }
}


void xPredIntraChromaDC( uchar_t *pSrc, pel_t *pDst, int i_dst, int iBlkWidth, int iBlkHeight, bool bAboveAvail, bool bLeftAvail )
{
    int x, y;
    uchar_t edgepix_uv[4 * LCU_SIZE + 2] = { 0 };
    uchar_t *pedge = (edgepix_uv + LCU_SIZE);

    //low-pass (Those elements that are not needed will not disturb)
    xIntraPredFilter(pSrc, pedge, iBlkWidth);
    assert(iBlkWidth == iBlkHeight);

    if ( !bAboveAvail && !bLeftAvail )
    {
        for ( y = 0; y < iBlkHeight; y++ )
        {
            for ( x = 0; x < iBlkWidth; x++ )
            {
                pDst[x] = 128;
            }
            pDst += i_dst;
        }
    }
    else if ( bAboveAvail && !bLeftAvail )
    {
        for ( y = 0; y < iBlkHeight; y++ )
        {
            for ( x = 0; x < iBlkWidth; x++ )
            {
                pDst[x] = pedge[1 + x];
            }
            pDst += i_dst;
        }
    }
    else if ( !bAboveAvail && bLeftAvail )
    {
        for ( y = 0; y < iBlkHeight; y++ )
        {
            for ( x = 0; x < iBlkWidth; x++ )
            {
                pDst[x] = pedge[-1 - y];
            }
            pDst += i_dst;
        }
    }
    else if ( bAboveAvail && bLeftAvail )
    {
        for ( y = 0; y < iBlkHeight; y++ )
        {
            for ( x = 0; x < iBlkWidth; x++ )
            {
                pDst[x] = ( pedge[1 + x] + pedge[-1 - y] ) >> 1;
            }
            pDst += i_dst;
        }
    }
}

void xIntraPredFilter( pel_t *pSrc, pel_t *pDst, int bsize )
{
    int i;

    for( i = -bsize; i <= bsize; i++ )
    {
        pDst[i] = ( pel_t )( ( (i == -bsize ? pSrc[i] : pSrc[i-1]) + ( pSrc[i]<<1 ) + pSrc[i+1] + 2 )>>2 );
    }
}

void intra_pred_luma( uchar_t *pSrc, pel_t *pDst, int i_dst, int predmode, int uhBlkWidth, bool bAboveAvail, bool bLeftAvail )
{
    switch( predmode )
    {
        case DC_PRED:// 0 DC
            g_funs_handle.intra_pred_luma_dc( pSrc, pDst, i_dst, uhBlkWidth, uhBlkWidth, bAboveAvail, bLeftAvail );
            break;
        case VERT_PRED:// 1 vertical
            assert( bAboveAvail );
            g_funs_handle.intra_pred_ver( pSrc, pDst, i_dst, uhBlkWidth, uhBlkWidth );
            break;
        case HOR_PRED:// 2 horizontal
            assert( bLeftAvail );
            g_funs_handle.intra_pred_hor( pSrc, pDst, i_dst, uhBlkWidth, uhBlkWidth );
            break;
        case DOWN_RIGHT_PRED:// 3 down-right
            assert( bLeftAvail && bAboveAvail );
            g_funs_handle.intra_pred_downright( pSrc, pDst, i_dst, uhBlkWidth, uhBlkWidth );
            break;

        case DOWN_LEFT_PRED:// 4 up-right bidirectional
            assert( bLeftAvail && bAboveAvail );
            g_funs_handle.intra_pred_downleft( pSrc, pDst, i_dst, uhBlkWidth, uhBlkWidth );
            break;
#if USING_INTRA_5_9
        case INTRA_BILINEAR:// 5 pbilinear
            assert(bLeftAvail && bAboveAvail);
            xPredIntraPlaneAdi(pSrc, pDst, i_dst, uhBlkWidth, uhBlkWidth, 8);
            break;
        case INTRA_PLANE:// 6 down-right
            assert(bLeftAvail && bAboveAvail);
            xPredIntraBiAdi(pSrc, pDst, i_dst, uhBlkWidth, uhBlkWidth, 8);
            break;

        case INTRA_XY16:// 7 avs2 실똑16
            assert(bLeftAvail && bAboveAvail);
            intra_pred_ang_xy_16_c(pSrc, pDst, i_dst, uhBlkWidth, uhBlkWidth);
            break;

        case INTRA_XY20:// 8 avs2 실똑20
            assert(bLeftAvail && bAboveAvail);
            intra_pred_ang_xy_20_c(pSrc, pDst, i_dst, uhBlkWidth, uhBlkWidth);
            break;
#endif
    }
}

void intra_pred_chroma( uchar_t *pSrc, pel_t *pDst, int i_dst, int predmode, int bsize, bool bAboveAvail, bool bLeftAvail )
{
    switch ( predmode )
    {
        case DC_PRED_8:
            g_funs_handle.intra_pred_chroma_dc( pSrc, pDst, i_dst, bsize, bsize, bAboveAvail, bLeftAvail );
            break;
        case HOR_PRED_8:
            g_funs_handle.intra_pred_hor( pSrc, pDst, i_dst, bsize, bsize );
            break;
        case VERT_PRED_8:
            g_funs_handle.intra_pred_ver( pSrc, pDst, i_dst, bsize, bsize );
            break;
        case PLANE_8:
            g_funs_handle.intra_pred_chroma_plane( pSrc, pDst, i_dst, bsize, bsize );
            break;
        default:
            assert( 0 );
            break;
    }
}

void com_funs_init_intra_pred()
{
    g_funs_handle.intra_pred_luma_dc   = xPredIntraLumaDC;
    g_funs_handle.intra_pred_ver       = xPredIntraVer;
    g_funs_handle.intra_pred_hor       = xPredIntraHor;
    g_funs_handle.intra_pred_downright = xPredIntraDownRight;
    g_funs_handle.intra_pred_downleft  = xPredIntraDownLeft;

    g_funs_handle.intra_pred_chroma_dc    = xPredIntraChromaDC;
    g_funs_handle.intra_pred_chroma_plane = xPredIntraChromaPlane;

#if USING_INTRA_5_9
    intra_init();
#endif
}