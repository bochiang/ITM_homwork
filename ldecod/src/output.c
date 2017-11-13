/*
*************************************************************************************
* File name:  output.c
* Function: Output an image and Trance support
*
*************************************************************************************
*/
#include "contributors.h"
#include "global.h"
#include <string.h>
#ifdef WIN32
#include <IO.H>
#endif

/*
*************************************************************************
* Function:Write decoded frame to output file
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void write_frame( ImgParams *img, FILE *p_out )
{
    int i, j;
    int img_width = img->width_org;
    int img_height = img->height_org;
    int img_width_cr = ( img_width/2 );
    int img_height_cr = ( img_height/ 2 );
    int iStride = img->iStride;
    int iStrideC = img->iStrideC;

    for( i=0; i<img_height; i++ )
        for( j=0; j<img_width; j++ )
        {
            fputc( imgY_rec[i*iStride+j],p_out );
        }

    for( i=0; i<img_height_cr; i++ )
        for( j=0; j<img_width_cr; j++ )
        {
            fputc( imgU_rec[i*iStrideC+j],p_out );
        }

    for( i=0; i<img_height_cr; i++ )
        for( j=0; j<img_width_cr; j++ )
        {
            fputc( imgV_rec[i*iStrideC+j],p_out );
        }

    fflush( p_out );
}

/*
*************************************************************************
* Function:Write previous decoded P frame to output file
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void write_prev_Pframe( ImgParams *img, FILE *p_out, uchar_t *pDst )
{
    fwrite( pDst, img->width_org*img->height_org*3/2, sizeof( uchar_t ), p_out );
    //fflush(p_out);
}