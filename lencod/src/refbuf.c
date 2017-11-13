/*
*************************************************************************************
* File name:
* Function:
*
*************************************************************************************
*/

#include <stdio.h>
#include <memory.h>
#include <assert.h>

#include "refbuf.h"

#define CACHELINESIZE 32

static pel_t line[16];


/*
*************************************************************************
* Function:Reference buffer write routines
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void PutPel_14 ( pel_t *Pic, int y, int x, pel_t val )
{
#ifdef PADDING_FALG
    Pic[( IMG_SUBPIXEL_PAD_SIZE * 4 + y )*( ( img->iStride + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4 ) + IMG_SUBPIXEL_PAD_SIZE * 4 + x] = val;
#else
    Pic[( IMG_SUBPIXEL_PAD_SIZE * 4 + y )*( ( img->width + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4 ) + IMG_SUBPIXEL_PAD_SIZE * 4 + x] = val;
#endif // PADDING_FALG

}

/*
*************************************************************************
* Function:Reference buffer read, Full pel
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

pel_t *FastLineX ( int dummy, pel_t* Pic, int y, int x )
{
    return Pic + y*img->iStride + x;
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
pel_t *UMVLineX ( int size, pel_t* Pic, int y, int x )
{
    int i, maxx;
    pel_t *Picy;

    Picy = Pic + MAX( 0, MIN( img->height - 1, y ) ) * img->iStride;

    if ( x < 0 )                          // Left edge
    {
        maxx = MIN( 0, x + size );

        for ( i = x; i < maxx; i++ )
        {
            // Replicate left edge pixel
            line[i - x] = Picy[0];
        }

        maxx = x + size;

        for ( i = 0; i < maxx; i++ )        // Copy non-edge pixels
        {
            line[i - x] = Picy[i];
        }
    }
    else if ( x > img->width - size )       // Right edge
    {
        maxx = img->width;

        for ( i = x; i < maxx; i++ )
        {
            line[i - x] = Picy[i];          // Copy non-edge pixels
        }

        maxx = x + size;

        for ( i = MAX( img->width, x ); i < maxx; i++ )
        {
            // Replicate right edge pixel
            line[i - x] = Picy[img->width - 1];
        }
    }
    else    // No edge
    {
        return Picy + x;
    }

    return line;
}

/*
*************************************************************************
* Function:Reference buffer, 1/4 pel
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/



pel_t UMVPelY_14( pel_t *Pic, int y, int x )
{
    int width4 = ( ( img->width + 2 * IMG_SUBPIXEL_PAD_SIZE - 1 ) << 2 );
    int height4 = ( ( img->height + 2 * IMG_SUBPIXEL_PAD_SIZE - 1 ) << 2 );

    x = x + IMG_SUBPIXEL_PAD_SIZE * 4;
    y = y + IMG_SUBPIXEL_PAD_SIZE * 4;

    if ( x < 0 )
    {
        if ( y < 0 )
        {
            return Pic[( y & 3 )*( ( img->width + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4 ) + ( x & 3 )];
        }
        if ( y > height4 )
        {
            return Pic[( height4 + ( y & 3 ) )*( ( img->width + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4 ) + ( x & 3 )];
        }
        return Pic[( y )*( ( img->width + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4 ) + ( x & 3 )];
    }

    if ( x > width4 )
    {
        if ( y < 0 )
        {
            return Pic[( y & 3 )*( ( img->width + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4 ) + width4 + ( x & 3 )];
        }
        if ( y > height4 )
        {
            return Pic[( height4 + ( y & 3 ) )*( ( img->width + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4 ) + width4 + ( x & 3 )];
        }
        return Pic[( y )*( ( img->width + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4 ) + width4 + ( x & 3 )];
    }

    if ( y < 0 )  // note: corner pixels were already processed
    {
        return Pic[( y & 3 )*( ( img->width + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4 ) + x];
    }
    if ( y > height4 )
    {
        return Pic[( height4 + ( y & 3 ) )*( ( img->width + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4 ) + x];
    }

    return Pic[( y )*( ( img->width + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4 ) + x];
}

pel_t FastPelY_14( pel_t *Pic, int y, int x )
{
    return Pic[( IMG_SUBPIXEL_PAD_SIZE * 4 + y )*( ( img->width + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4 ) + IMG_SUBPIXEL_PAD_SIZE * 4 + x];
}

