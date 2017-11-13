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

#define  IClip( Min, Max, Val) (((Val)<(Min))? (Min):(((Val)>(Max))? (Max):(Val)))

/*
*************************************************************************
* Function: EdgeLoopVer
* Input:  byte* SrcPtr,int QP, int dir,int istride,int Chro
* Output: void
* Return: void
*************************************************************************
*/

void EdgeLoopVer( pel_t* SrcPtr, int istride, bool bChroma, int edge )
{
    int     pel;
    int     Axxxelta, LeftD, RightD;
    uchar_t L3, L2, L1, L0, R0, R1, R2, R3;
    int     fs = 0;
    int     flatedge = 0, edge_th = 0;

    for ( pel = 0; pel < 16; pel++ )
    {
        L3 = SrcPtr[-4];
        L2 = SrcPtr[-3];
        L1 = SrcPtr[-2];
        L0 = SrcPtr[-1];
        R0 = SrcPtr[0];
        R1 = SrcPtr[1];
        R2 = SrcPtr[2];
        R3 = SrcPtr[3];

        Axxxelta = abs( R0 - L0 );
        LeftD    = abs( L0 - L1 );
        RightD   = abs( R0 - R1 );

        edge_th = min( 3, input->Beta );
        flatedge = LeftD < edge_th && RightD < edge_th;

        if ( LeftD < input->Beta && RightD < input->Beta && Axxxelta > LeftD && Axxxelta > RightD && Axxxelta < input->Alpha )
        {
            if ( abs( L2 - L0 ) < input->Beta && abs( R2 - R0 ) < input->Beta )
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

        if ( fs && bChroma && !flatedge )
        {
            fs = 1;
        }

        if ( ( fs == 2 ) && flatedge && !edge && !bChroma && abs( R3 - R0 ) < input->Beta && abs( L3 - L0 ) < input->Beta )
        {
            fs = 3;
        }

        switch ( fs )
        {
            case 3:
                SrcPtr[-3] = ( ( 4 * ( L0 - R0 ) + 5 * ( R0 - L2 ) + 4 ) >> 3 ) + L2; //L2
                SrcPtr[-2] = ( ( 16 * ( R0 - L1 ) + 6 * ( L2 - R0 ) + 7 * ( L0 - R0 ) + 8 ) >> 4 ) + L1; //L1
                SrcPtr[-1] = ( ( 9 * ( L2 - R0 ) + 6 * ( R2 - L0 ) + 17 * ( R0 - L0 ) + 16 ) >> 5 ) + L0; //L0
                SrcPtr[0] = ( ( 9 * ( R2 - L0 ) + 6 * ( L2 - R0 ) + 17 * ( L0 - R0 ) + 16 ) >> 5 ) + R0; //R0
                SrcPtr[1] = ( ( 16 * ( L0 - R1 ) + 6 * ( R2 - L0 ) + 7 * ( R0 - L0 ) + 8 ) >> 4 ) + R1; //R1
                SrcPtr[2] = ( ( 4 * ( R0 - L0 ) + 5 * ( L0 - R2 ) + 4 ) >> 3 ) + R2; //R2
                break;

            case 2:
                SrcPtr[-2] = ( ( ( L2 - R0 ) * 3 + ( R0 - L1 ) * 8 + ( L0 - R0 ) * 4 + 8 ) >> 4 ) + L1; //L1
                SrcPtr[-1] = ( ( ( L2 - R0 ) + ( L1 - R0 ) * 4 + ( R1 - L0 ) + ( R0 - L0 ) * 9 + 8 ) >> 4 ) + L0; //L0
                SrcPtr[0] = ( ( ( R2 - L0 ) + ( R1 - L0 ) * 4 + ( L1 - R0 ) + ( L0 - R0 ) * 9 + 8 ) >> 4 ) + R0; //R0
                SrcPtr[1] = ( ( ( R2 - L0 ) * 3 + ( L0 - R1 ) * 8 + ( R0 - L0 ) * 4 + 8 ) >> 4 ) + R1; //R1
                break;

            case 1:
                SrcPtr[0] = ( ( L0 - R0 + 2 ) >> 2 ) + R0; //R0
                SrcPtr[-1] = ( ( R0 - L0 + 2 ) >> 2 ) + L0; //L0
                break;

            default:
                break;
        }

        SrcPtr += istride;    // Next row or column
        pel += bChroma;
    }
}


/*
*************************************************************************
* Function: EdgeLoopHor
* Input:  byte* SrcPtr,int QP, int dir,int istride,int Chro
* Output: void
* Return: void
*************************************************************************
*/

void EdgeLoopHor( pel_t* SrcPtr, int istride, bool bChroma, int edge )
{
    int     pel;
    int     inc, inc2, inc3, inc4;
    int     Axxxelta, LeftD, RightD;
    uchar_t L3, L2, L1, L0, R0, R1, R2, R3;
    int     fs = 0; //fs stands for filtering strength.  The larger fs is, the stronger filter is applied.
    int     flatedge = 0, edge_th = 0;

    inc  = istride;
    inc2 = istride << 1;
    inc3 = istride * 3;
    inc4 = istride << 2;

    for ( pel = 0; pel < 16; pel++ )
    {
        L3 = SrcPtr[-inc4];
        L2 = SrcPtr[-inc3];
        L1 = SrcPtr[-inc2];
        L0 = SrcPtr[-inc];
        R0 = SrcPtr[0];
        R1 = SrcPtr[inc];
        R2 = SrcPtr[inc2];
        R3 = SrcPtr[inc3];

        Axxxelta = abs( R0 - L0 );
        LeftD = abs( L0 - L1 );
        RightD = abs( R0 - R1 );

        edge_th = min( 3, input->Beta );
        flatedge = LeftD < edge_th && RightD < edge_th;

        if ( LeftD < input->Beta && RightD < input->Beta && Axxxelta > LeftD && Axxxelta > RightD && Axxxelta < input->Alpha )
        {
            if ( abs( L2 - L0 ) < input->Beta && abs( R2 - R0 ) < input->Beta )
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

        if ( fs && bChroma && !flatedge )
        {
            fs = 1;
        }

        if ( ( fs == 2 ) && flatedge && !edge && !bChroma && abs( R3 - R0 ) < input->Beta && abs( L3 - L0 ) < input->Beta )
        {
            fs = 3;
        }

        switch ( fs )
        {
            case 3:
                SrcPtr[-inc3] = ( pel_t )( ( ( 4  * ( L0 - R0 ) + 5 * ( R0 - L2 ) + 4 ) >> 3 ) + L2 ); //L2
                SrcPtr[-inc2] = ( pel_t )( ( ( 16 * ( R0 - L1 ) + 6 * ( L2 - R0 ) + 7  * ( L0 - R0 ) + 8 ) >> 4 ) + L1 ); //L1
                SrcPtr[-inc]  = ( pel_t )( ( ( 9  * ( L2 - R0 ) + 6 * ( R2 - L0 ) + 17 * ( R0 - L0 ) + 16 ) >> 5 ) + L0 ); //L0
                SrcPtr[0]     = ( pel_t )( ( ( 9  * ( R2 - L0 ) + 6 * ( L2 - R0 ) + 17 * ( L0 - R0 ) + 16 ) >> 5 ) + R0 ); //R0
                SrcPtr[inc]   = ( pel_t )( ( ( 16 * ( L0 - R1 ) + 6 * ( R2 - L0 ) + 7  * ( R0 - L0 ) + 8 ) >> 4 ) + R1 ); //R1
                SrcPtr[inc2]  = ( pel_t )( ( ( 4  * ( R0 - L0 ) + 5 * ( L0 - R2 ) + 4 ) >> 3 ) + R2 ); //R2
                break;

            case 2:
                SrcPtr[-inc2] = ( pel_t )( ( ( ( L2 - R0 ) * 3 + ( R0 - L1 ) * 8 + ( L0 - R0 ) * 4 + 8 ) >> 4 ) + L1 ); //L1
                SrcPtr[-inc]  = ( pel_t )( ( ( ( L2 - R0 ) + ( L1 - R0 ) * 4 + ( R1 - L0 ) + ( R0 - L0 ) * 9 + 8 ) >> 4 ) + L0 ); //L0
                SrcPtr[0]     = ( pel_t )( ( ( ( R2 - L0 ) + ( R1 - L0 ) * 4 + ( L1 - R0 ) + ( L0 - R0 ) * 9 + 8 ) >> 4 ) + R0 ); //R0
                SrcPtr[inc]   = ( pel_t )( ( ( ( R2 - L0 ) * 3 + ( L0 - R1 ) * 8 + ( R0 - L0 ) * 4 + 8 ) >> 4 ) + R1 ); //R1
                break;

            case 1:
                SrcPtr[0]    = ( pel_t )( ( ( L0 - R0 + 2 ) >> 2 ) + R0 ); //R0
                SrcPtr[-inc] = ( pel_t )( ( ( R0 - L0 + 2 ) >> 2 ) + L0 ); //L0
                break;

            default:
                break;
        }

        SrcPtr += 1;    // Next row or column
        pel += bChroma;
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

void DeblockMb( ImgParams *img,
                pel_t *imgY,
                pel_t *imgU,
                pel_t *imgV,
                int mb_y,
                int mb_x,
                int current_mb_nr_temp )
{
    int           EdgeCondition;
    int           edge;
    pel_t          *SrcY, *SrcU = NULL, *SrcV = NULL;
    Macroblock    *MbQ;
    const int mb_width = img->PicWidthInMbs;
    Macroblock *currMB = &img->mb_data[current_mb_nr_temp];

    SrcY = &( imgY[( mb_y << 4 ) * ( img->iStride )] ) + ( mb_x << 4 ); // pointers to source
    if ( imgU != NULL && imgV != NULL )
    {
        SrcU = &( imgU[( mb_y << 3 ) * ( img->iStrideC )] ) + ( mb_x << 3 );
        SrcV = &( imgV[( mb_y << 3 ) * ( img->iStrideC )] ) + ( mb_x << 3 );
    }

    MbQ = &img->mb_data[mb_y*( img->width >> 4 ) + mb_x];   // current Mb

    // filter vertical edges
    EdgeCondition = ( mb_x != 0 );  // can not filter beyond frame boundaries
    for ( edge = 0; edge < 2; edge++ )                                          // first 4 vertical strips of 16 pel
    {
        // then  4 horicontal
        if ( edge || EdgeCondition )
        {
            g_funs_handle.deblock_edge[0]( SrcY + ( edge << 3 ), img->iStride, 0, edge );
            if ( ( imgU != NULL ) && ( imgV != NULL ) && ( !edge & 1 ) )
            {
                g_funs_handle.deblock_edge[0]( SrcU + ( edge << 2 ), img->iStrideC, 1, edge );
                g_funs_handle.deblock_edge[0]( SrcV + ( edge << 2 ), img->iStrideC, 1, edge );
            }
        }
    }

    // filter horizontal edges
    EdgeCondition = ( mb_y != 0 );  // can not filter beyond frame boundaries
    if ( mb_y )
    {
        EdgeCondition = ( currMB->slice_nr == img->mb_data[current_mb_nr_temp - mb_width].slice_nr ) ? EdgeCondition : 0; //  can not filter beyond slice boundaries   jlzheng 7.8
    }
    for ( edge = 0; edge < 2; edge++ )                                          // first 4 vertical strips of 16 pel
    {
        // then  4 horicontal
        if ( edge || EdgeCondition )
        {
            g_funs_handle.deblock_edge[1]( SrcY + ( edge << 3 )* img->iStride, img->iStride, 0, edge );
            if ( ( imgU != NULL ) && ( imgV != NULL ) && ( !edge & 1 ) )
            {
                g_funs_handle.deblock_edge[1]( SrcU + ( edge << 2 ) * img->iStrideC, img->iStrideC, 1, edge );
                g_funs_handle.deblock_edge[1]( SrcV + ( edge << 2 ) * img->iStrideC, img->iStrideC, 1, edge );
            }
        }
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

void DeblockFrame( ImgParams *img, pel_t *imgY, pel_t *imgU, pel_t *imgV )
{
    int  mb_x, mb_y;

    //img->current_mb_nr = -1;         // jlzheng  7.18
    int current_mb_nr_temp = -1;

    for ( mb_y = 0; mb_y < ( img->height >> 4 ); mb_y++ )
    {
        for ( mb_x = 0; mb_x < ( img->width >> 4 ); mb_x++ )
        {
            //img->current_mb_nr++;    // jlzheng 7.18
            current_mb_nr_temp++;
            DeblockMb( img, imgY, imgU, imgV, mb_y, mb_x, current_mb_nr_temp );
        }
    }
}

void com_funs_init_deblock_filter()
{
    g_funs_handle.deblock_edge[0] = EdgeLoopVer;
    g_funs_handle.deblock_edge[1] = EdgeLoopHor;
}