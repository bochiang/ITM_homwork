/*
*************************************************************************************
* File name:
* Function:
*
*************************************************************************************
*/
#include <string.h>
#include "global.h"
#include "../../common/common.h"

void EdgeLoopVer( uchar_t* SrcPtr, int istride, bool bChroma, int edge )
{
    int     pel;
    int     Axxxelta, LeftD, RightD ;
    uchar_t   L3, L2, L1, L0, R0, R1, R2, R3;
    int     fs = 0;
    int     flatedge = 0, edge_th = 0;

    for ( pel = 0; pel < 16 ; pel++ )
    {
        L3  = SrcPtr[-4] ;
        L2  = SrcPtr[-3] ;
        L1  = SrcPtr[-2] ;
        L0  = SrcPtr[-1] ;
        R0  = SrcPtr[ 0] ;
        R1  = SrcPtr[ 1] ;
        R2  = SrcPtr[ 2] ;
        R3  = SrcPtr[ 3] ;

        Axxxelta = abs ( R0 - L0 ) ;
        LeftD    = abs ( L0 - L1 ) ;
        RightD   = abs ( R0 - R1 ) ;

        edge_th = MIN(3, img->Beta);
        flatedge = LeftD < edge_th && RightD < edge_th; // (6)

        if (LeftD < img->Beta && RightD < img->Beta && Axxxelta > LeftD && Axxxelta > RightD && Axxxelta < img->Alpha) // condition (1) (2) (3)
        if (abs(L2 - L0) < img->Beta && abs(R2 - R0) < img->Beta) // (4)
            {
                fs = 2;
            }
            else
            {
                fs = 1;
            }
        else
        {
            fs = 0;
        }

        if ( fs && bChroma && !flatedge )
        {
            fs = 1;
        }

        if ((fs == 2) && flatedge && !edge && !bChroma && abs(R3 - R0) < img->Beta && abs(L3 - L0) < img->Beta) // (6) and (5)
        {
            fs = 3;
        }

        switch ( fs )
        {
            case 3:
                SrcPtr[-3] = ( ( 4 * ( L0 - R0 ) + 5 * ( R0 - L2 ) + 4 ) >> 3 ) + L2; //L2                         V0
                SrcPtr[-2] = ( ( 16 * ( R0 - L1 ) + 6 * ( L2 - R0 ) + 7* ( L0 - R0 ) + 8 ) >> 4 ) + L1; //L1     V2
                SrcPtr[-1] = ( ( 9 * ( L2 - R0 ) + 6 * ( R2 - L0 ) + 17 * ( R0 - L0 ) + 16 ) >> 5 ) + L0; //L0     V4
                SrcPtr[0]  = ( ( 9 * ( R2 - L0 ) + 6 * ( L2 - R0 ) + 17 * ( L0 - R0 ) + 16 ) >> 5 ) + R0; //R0     V5
                SrcPtr[1]  = ( ( 16 * ( L0 - R1 ) + 6 * ( R2 - L0 ) + 7* ( R0 - L0 ) + 8 ) >> 4 ) + R1; //R1     V3
                SrcPtr[2]  = ( ( 4 * ( R0 - L0 ) + 5 * ( L0 - R2 ) + 4 ) >> 3 ) + R2; //R2                         V1
                break;

            case 2:
                SrcPtr[-2] = ( ( ( L2 - R0 ) * 3 + ( R0 - L1 ) * 8 + ( L0 - R0 ) * 4 +8 )>> 4 ) + L1; //L1          V0
                SrcPtr[-1] = ( ( ( L2 - R0 ) + ( L1 - R0 ) * 4 + ( R1 - L0 ) + ( R0 - L0 ) * 9 + 8 )>> 4 ) + L0; //L0 V2
                SrcPtr[0]  = ( ( ( R2 - L0 ) + ( R1 - L0 ) * 4 + ( L1 - R0 ) + ( L0 - R0 ) * 9 + 8 )>> 4 ) + R0; //R0 V3
                SrcPtr[1]  = ( ( ( R2 - L0 ) * 3 + ( L0 - R1 ) * 8 + ( R0 - L0 ) * 4 +8 ) >> 4 ) + R1; //R1         V1
                break;

            case 1:
                SrcPtr[0]  = ( ( L0 - R0 + 2 )>>2 ) + R0; //R0 V0
                SrcPtr[-1] = ( ( R0 - L0 + 2 )>>2 ) + L0; //L0 V1
                break;

            default:
                break;
        }

        SrcPtr += istride;    // Next row or column
        pel    += bChroma;
    }
}

void EdgeLoopHor(uchar_t* SrcPtr, int istride, bool bChroma, int edge)
{
    int     pel;
    int     inc, inc2, inc3, inc4;
    int     Axxxelta, LeftD, RightD;
    uchar_t L3, L2, L1, L0, R0, R1, R2, R3;
    int     fs = 0;
    int     flatedge = 0, edge_th = 0;

    inc = istride;
    inc2 = istride << 1;
    inc3 = istride * 3;
    inc4 = istride << 2;

    for (pel = 0; pel < 16; pel++)
    {
        L3 = SrcPtr[-inc4];
        L2 = SrcPtr[-inc3];
        L1 = SrcPtr[-inc2];
        L0 = SrcPtr[-inc];
        R0 = SrcPtr[0];
        R1 = SrcPtr[inc];
        R2 = SrcPtr[inc2];
        R3 = SrcPtr[inc3];

        Axxxelta = abs(R0 - L0);
        LeftD = abs(L0 - L1);
        RightD = abs(R0 - R1);

        edge_th = MIN(3, img->Beta);
        flatedge = LeftD < edge_th && RightD < edge_th;

        if (LeftD < img->Beta && RightD < img->Beta && Axxxelta > LeftD && Axxxelta > RightD && Axxxelta < img->Alpha)
        {
            if (abs(L2 - L0) < img->Beta && abs(R2 - R0) < img->Beta)
            {
                fs = 2;
            }
            else
            {
                fs = 1;
            }
        }
        else
        {
            fs = 0;
        }

        if (fs && bChroma && !flatedge)
        {
            fs = 1;
        }

        if ((fs == 2) && flatedge && !edge && !bChroma && abs(R3 - R0) < img->Beta && abs(L3 - L0) < img->Beta)
        {
            fs = 3;
        }

        switch (fs)
        {
        case 3:
            SrcPtr[-inc3] = ((4 * (L0 - R0) + 5 * (R0 - L2) + 4) >> 3) + L2; //L2
            SrcPtr[-inc2] = ((16 * (R0 - L1) + 6 * (L2 - R0) + 7 * (L0 - R0) + 8) >> 4) + L1; //L1
            SrcPtr[-inc] = ((9 * (L2 - R0) + 6 * (R2 - L0) + 17 * (R0 - L0) + 16) >> 5) + L0; //L0
            SrcPtr[0] = ((9 * (R2 - L0) + 6 * (L2 - R0) + 17 * (L0 - R0) + 16) >> 5) + R0; //R0
            SrcPtr[inc] = ((16 * (L0 - R1) + 6 * (R2 - L0) + 7 * (R0 - L0) + 8) >> 4) + R1; //R1
            SrcPtr[inc2] = ((4 * (R0 - L0) + 5 * (L0 - R2) + 4) >> 3) + R2; //R2
            break;

        case 2:
            SrcPtr[-inc2] = (((L2 - R0) * 3 + (R0 - L1) * 8 + (L0 - R0) * 4 + 8) >> 4) + L1; //L1
            SrcPtr[-inc] = (((L2 - R0) + (L1 - R0) * 4 + (R1 - L0) + (R0 - L0) * 9 + 8) >> 4) + L0; //L0
            SrcPtr[0] = (((R2 - L0) + (R1 - L0) * 4 + (L1 - R0) + (L0 - R0) * 9 + 8) >> 4) + R0; //R0
            SrcPtr[inc] = (((R2 - L0) * 3 + (L0 - R1) * 8 + (R0 - L0) * 4 + 8) >> 4) + R1; //R1
            break;

        case 1:
            SrcPtr[0] = ((L0 - R0 + 2) >> 2) + R0; //R0
            SrcPtr[-inc] = ((R0 - L0 + 2) >> 2) + L0; //L0
            break;

        default:
            break;
        }

        SrcPtr += 1;    // Next row or column
        pel += bChroma;
    }
}

void DeblockMb( ImgParams *img, uchar_t *pic_y, uchar_t *pic_u, uchar_t *pic_v, int mb_y, int mb_x )
{
    int           EdgeCondition;
    int           edge ;
    Macroblock    *MbQ ;
    const int mb_width = img->PicWidthInMbs;
    const int mb_nr    = img->current_mb_nr;
    Macroblock *currMB = &img->mb_data[mb_nr];

    uchar_t *SrcY = pic_y + ( mb_y<<4 )*img->iStride + ( mb_x<<4 ); // pointers to source
    uchar_t *SrcU = pic_u + ( mb_y<<3 )*img->iStrideC + ( mb_x<<3 );
    uchar_t *SrcV = pic_v + ( mb_y<<3 )*img->iStrideC + ( mb_x<<3 );

    MbQ  = &img->mb_data[mb_y*( img->width>>4 ) + mb_x]; // current Mb

    // filter vertical edges
    EdgeCondition = ( mb_x != 0 );  // can not filter beyond frame boundaries
    for( edge=0 ; edge<2 ; edge++ )
    {
        if( edge || EdgeCondition )
        {
            g_funs_handle.deblock_edge[0]( SrcY + ( edge << 3 ), img->iStride, 0, edge );

            if ( ( pic_u != NULL && pic_v != NULL ) && ( !edge & 1 ) )
            {
                g_funs_handle.deblock_edge[0]( SrcU + ( edge << 2 ), img->iStrideC, 1, edge );
                g_funs_handle.deblock_edge[0]( SrcV + ( edge << 2 ), img->iStrideC, 1, edge );
            }
        }
    }

    // filter horizontal edges
    EdgeCondition = ( mb_y != 0 );
    if( mb_y )
    {
        EdgeCondition = ( currMB->slice_nr == img->mb_data[img->current_mb_nr-mb_width].slice_nr )? EdgeCondition:0; //  can not filter beyond slice boundaries   jlzheng 7.8
    }
    for( edge=0 ; edge<2 ; edge++ )
    {
        if( edge || EdgeCondition )
        {
            g_funs_handle.deblock_edge[1]( SrcY + ( edge << 3 )*img->iStride, img->iStride, 0, edge );

            if ( ( pic_u != NULL && pic_v != NULL ) && ( !edge & 1 ) )
            {
                g_funs_handle.deblock_edge[1]( SrcU + ( edge << 2 ) * img->iStrideC, img->iStrideC, 1, edge );
                g_funs_handle.deblock_edge[1]( SrcV + ( edge << 2 ) * img->iStrideC, img->iStrideC, 1, edge );
            }
        }
    }
}

void DeblockFrame( ImgParams *img, uchar_t *pic_y, uchar_t *pic_u, uchar_t *pic_v )
{
    int mb_x, mb_y ;
    img->current_mb_nr = -1;

    // 16 * 16 Block
    for( mb_y=0 ; mb_y<( img->height>>4 ) ; mb_y++ )
    {
        for( mb_x=0 ; mb_x<( img->width>>4 ) ; mb_x++ )
        {
            img->current_mb_nr++;
            DeblockMb( img, pic_y, pic_u, pic_v, mb_y, mb_x ) ;
        }
    }
}

void com_funs_init_deblock_filter()
{
    g_funs_handle.deblock_edge[0] = EdgeLoopVer;
    g_funs_handle.deblock_edge[1] = EdgeLoopHor;

}
