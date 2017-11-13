#include "interPrediction.h"
#include "block.h"

void com_if_filter_cpy( const pel_t *src, int i_src, pel_t *dst, int i_dst, int width, int height )
{
    int row;
    for ( row = 0; row < height; row++ )
    {
        memcpy( dst, src, sizeof( pel_t )* width );
        src += i_src;
        dst += i_dst;
    }
}

#define FLT_10TAP_HOR(src, i, coef) ( \
    (src)[i-4] * (coef)[0] + \
    (src)[i-3] * (coef)[1] + \
    (src)[i-2] * (coef)[2] + \
    (src)[i-1] * (coef)[3] + \
    (src)[i  ] * (coef)[4] + \
    (src)[i+1] * (coef)[5] + \
    (src)[i+2] * (coef)[6] + \
    (src)[i+3] * (coef)[7] + \
    (src)[i+4] * (coef)[8] + \
    (src)[i+5] * (coef)[9])

#define FLT_10TAP_VER(src, i, i_src, coef) ( \
    (src)[i-4 * i_src] * (coef)[0] + \
    (src)[i-3 * i_src] * (coef)[1] + \
    (src)[i-2 * i_src] * (coef)[2] + \
    (src)[i-1 * i_src] * (coef)[3] + \
    (src)[i          ] * (coef)[4] + \
    (src)[i+1 * i_src] * (coef)[5] + \
    (src)[i+2 * i_src] * (coef)[6] + \
    (src)[i+3 * i_src] * (coef)[7] + \
    (src)[i+4 * i_src] * (coef)[8] + \
    (src)[i+5 * i_src] * (coef)[9])

#define FLT_6TAP_HOR(src, i, coef) ( \
    (src)[i-2] * (coef)[0] + \
    (src)[i-1] * (coef)[1] + \
    (src)[i  ] * (coef)[2] + \
    (src)[i+1] * (coef)[3] + \
    (src)[i+2] * (coef)[4] + \
    (src)[i+3] * (coef)[5])

#define FLT_6TAP_VER(src, i, i_src, coef) ( \
    (src)[i-2 * i_src] * (coef)[0] + \
    (src)[i-1 * i_src] * (coef)[1] + \
    (src)[i          ] * (coef)[2] + \
    (src)[i+1 * i_src] * (coef)[3] + \
    (src)[i+2 * i_src] * (coef)[4] + \
    (src)[i+3 * i_src] * (coef)[5])

#define FLT_4TAP_HOR(src, i, coef) ( \
    (src)[i - 1] * (coef)[0] + \
    (src)[i    ] * (coef)[1] + \
    (src)[i + 1] * (coef)[2] + \
    (src)[i + 2] * (coef)[3])

#define FLT_4TAP_VER(src, i, i_src, coef) ( \
    (src)[i-1 * i_src] * (coef)[0] + \
    (src)[i          ] * (coef)[1] + \
    (src)[i+1 * i_src] * (coef)[2] + \
    (src)[i+2 * i_src] * (coef)[3])

static void com_if_filter_hor_4( const pel_t *src, int i_src, pel_t *dst, int i_dst, int width, int height, char_t const *coeff, int max_val )
{
    int row, col;
    int sum, val;

    for ( row = 0; row < height; row++ )
    {
        for ( col = 0; col < width; col++ )
        {
            sum = FLT_4TAP_HOR( src, col, coeff );
            val = ( sum + 32 ) >> 6;
            dst[col] = ( pel_t )COM_CLIP3( 0, max_val, val );
        }
        src += i_src;
        dst += i_dst;
    }
}

static void com_if_filter_hor_6( const pel_t *src, int i_src, pel_t *dst, int i_dst, int width, int height, char_t const *coeff, int max_val )
{
    int row, col;
    int sum, val;

    for ( row = 0; row < height; row++ )
    {
        for ( col = 0; col < width; col++ )
        {
            sum = FLT_6TAP_HOR( src, col, coeff );
            val = ( sum + 32 ) >> 6;
            dst[col] = ( pel_t )COM_CLIP3( 0, max_val, val );
        }
        src += i_src;
        dst += i_dst;
    }
}

static void com_if_filter_hor_10( const pel_t *src, int i_src, pel_t *dst, int i_dst, int width, int height, char_t const *coeff, int max_val )
{
    int row, col;
    int sum, val;

    for ( row = 0; row < height; row++ )
    {
        for ( col = 0; col < width; col++ )
        {
            sum = FLT_10TAP_HOR( src, col, coeff );
            val = ( sum + 32 ) >> 6;
            dst[col] = ( pel_t )COM_CLIP3( 0, max_val, val );
        }
        src += i_src;
        dst += i_dst;
    }
}

static void com_if_filter_ver_4( const pel_t *src, int i_src, pel_t *dst, int i_dst, int width, int height, char_t const *coeff, int max_val )
{
    int row, col;
    int sum, val;

    for ( row = 0; row < height; row++ )
    {
        for ( col = 0; col < width; col++ )
        {
            sum = FLT_4TAP_VER( src, col, i_src, coeff );
            val = ( sum + 32 ) >> 6;
            dst[col] = ( pel_t )COM_CLIP3( 0, max_val, val );
        }
        src += i_src;
        dst += i_dst;
    }
}

static void com_if_filter_ver_6( const pel_t *src, int i_src, pel_t *dst, int i_dst, int width, int height, char_t const *coeff, int max_val )
{
    int row, col;
    int sum, val;

    for ( row = 0; row < height; row++ )
    {
        for ( col = 0; col < width; col++ )
        {
            sum = FLT_6TAP_VER( src, col, i_src, coeff );
            val = ( sum + 32 ) >> 6;
            dst[col] = ( pel_t )COM_CLIP3( 0, max_val, val );
        }
        src += i_src;
        dst += i_dst;
    }
}

static void com_if_filter_ver_10( const pel_t *src, int i_src, pel_t *dst, int i_dst, int width, int height, char_t const *coeff, int max_val )
{
    int row, col;
    int sum, val;

    for ( row = 0; row < height; row++ )
    {
        for ( col = 0; col < width; col++ )
        {
            sum = FLT_10TAP_VER( src, col, i_src, coeff );
            val = ( sum + 32 ) >> 6;
            dst[col] = ( pel_t )COM_CLIP3( 0, max_val, val );
        }
        src += i_src;
        dst += i_dst;
    }
}

static void com_if_filter_hor_ver_4( const pel_t *src, int i_src, pel_t *dst, int i_dst, int width, int height, const char_t *coeff_h, const char_t *coeff_v, int max_val )
{
    int row, col;
    int sum, val;
    int add1, shift1;
    int add2, shift2;

    ALIGNED_16( i16s_t tmp_res[( 32 + 3 ) * 32] );
    i16s_t *tmp;

    shift1 = 0;
    shift2 = 12;

    add1 = ( 1 << ( shift1 ) ) >> 1;
    add2 = 1 << ( shift2 - 1 );

    src += -1 * i_src;
    tmp = tmp_res;

    for ( row = -1; row < height + 2; row++ )
    {
        for ( col = 0; col < width; col++ )
        {
            tmp[col] = FLT_4TAP_HOR( src, col, coeff_h );
        }
        src += i_src;
        tmp += 32;
    }

    tmp = tmp_res + 1 * 32;

    for ( row = 0; row < height; row++ )
    {
        for ( col = 0; col < width; col++ )
        {
            sum = FLT_4TAP_VER( tmp, col, 32, coeff_v );
            val = ( sum + add2 ) >> shift2;
            dst[col] = COM_CLIP3( 0, max_val, val );
        }
        dst += i_dst;
        tmp += 32;
    }
}


static void com_if_filter_hor_ver_6( const pel_t *src, int i_src, pel_t *dst, int i_dst, int width, int height, const char_t *coeff_h, const char_t *coeff_v, int max_val )
{
    int row, col;
    int sum, val;
    int add1, shift1;
    int add2, shift2;

    ALIGNED_16( i16s_t tmp_res[( 48 + 5 ) * 48] );
    i16s_t *tmp;

    shift1 = 0;
    shift2 = 12;

    add1 = ( 1 << ( shift1 ) ) >> 1;
    add2 = 1 << ( shift2 - 1 );

    src += -2 * i_src;
    tmp = tmp_res;

    for ( row = -2; row < height + 3; row++ )
    {
        for ( col = 0; col < width; col++ )
        {
            tmp[col] = FLT_6TAP_HOR( src, col, coeff_h );
        }
        src += i_src;
        tmp += 48;
    }

    tmp = tmp_res + 2 * 48;

    for ( row = 0; row < height; row++ )
    {
        for ( col = 0; col < width; col++ )
        {
            sum = FLT_6TAP_VER( tmp, col, 48, coeff_v );
            val = ( sum + add2 ) >> shift2;
            dst[col] = COM_CLIP3( 0, max_val, val );
        }
        dst += i_dst;
        tmp += 48;
    }
}


static void com_if_filter_hor_ver_10( const pel_t *src, int i_src, pel_t *dst, int i_dst, int width, int height, const char_t *coeff_h, const char_t *coeff_v, int max_val )
{
    int row, col;
    int sum, val;
    int add1, shift1;
    int add2, shift2;

    ALIGNED_16( i16s_t tmp_res[( 80 + 9 ) * 80] );
    i16s_t *tmp;

    shift1 = 0;
    shift2 = 12;

    add1 = ( 1 << ( shift1 ) ) >> 1;
    add2 = 1 << ( shift2 - 1 );

    src += -4 * i_src;
    tmp = tmp_res;

    for ( row = -4; row < height + 5; row++ )
    {
        for ( col = 0; col < width; col++ )
        {
            tmp[col] = FLT_10TAP_HOR( src, col, coeff_h );
        }
        src += i_src;
        tmp += 80;
    }

    tmp = tmp_res + 4 * 80;

    for ( row = 0; row < height; row++ )
    {
        for ( col = 0; col < width; col++ )
        {
            sum = FLT_10TAP_VER( tmp, col, 80, coeff_v );
            val = ( sum + add2 ) >> shift2;
            dst[col] = COM_CLIP3( 0, max_val, val );
        }
        dst += i_dst;
        tmp += 80;
    }
}

void get_block_adaptive( ImgParams  *img, pel_t *pDst, int i_dst, int bsize, int pos_x, int pos_y, uchar_t *ref_pic, int i_ref )
{
    int int_pos_x, int_pos_y;
    int sub_pos_x, sub_pos_y;
    pel_t *pSrc;
    int i_src = img->iStride;

    int tap, tap_min, tap_max;

    static const char_t COEF_4tap[3][4] =
    {
        { -6, 56, 15, -1 },
        { -4, 36, 36, -4 },
        { -1, 15, 56, -6 }
    };

    static const char_t COEF_6tap[3][6] =
    {
        { 2, -9, 57, 17, -4, 1 },
        { 2, -9, 39, 39, -9, 2 },
        { 1, -4, 17, 57, -9, 2 }
    };

    static const char_t COEF_10tap[3][10] =
    {
        { 1, -2, 4, -10, 57, 19,  -7, 3, -1, 0 },
        { 1, -2, 5, -12, 40, 40, -12, 5, -2, 1 },
        { 0, -1, 3,  -7, 19, 57, -10, 4, -2, 1 }
    };

    char_t const *COEFs[3];
    char_t const *COEF_HOR, *COEF_VER;
    sub_pos_x = pos_x&3;
    sub_pos_y = pos_y&3;
    int_pos_x = ( pos_x-sub_pos_x )/4;
    int_pos_y = ( pos_y-sub_pos_y )/4;

    pSrc = ref_pic + int_pos_y * i_ref + int_pos_x;

    if( img->height >= 1600 ) //for UHD, 2560x1600
    {
        COEFs[0] = COEF_4tap[0];
        COEFs[1] = COEF_4tap[1];
        COEFs[2] = COEF_4tap[2];
        tap = 4;
    }
    else if( img->height >= 720 ) //for 720p and 1080p
    {
        COEFs[0] = COEF_6tap[0];
        COEFs[1] = COEF_6tap[1];
        COEFs[2] = COEF_6tap[2];
        tap = 6;
    }
    else
    {
        COEFs[0] = COEF_10tap[0];
        COEFs[1] = COEF_10tap[1];
        COEFs[2] = COEF_10tap[2];
        tap = 10;
    }

    tap_min = ( tap>>1 ) -1;
    tap_max = ( tap>>1 ) +1;

    //    A  a  b  c  B
    //    d  e  f  g
    //    h  i  j  k
    //    n  p  q  r
    //    C           D
    if ( sub_pos_x == 0 && sub_pos_y == 0 )  //fullpel position: A
    {
        g_funs_handle.ipcpy( pSrc, i_src, pDst, i_dst, B8_SIZE, B8_SIZE );
    }
    else  /* other positions */
    {
        COEF_HOR = COEFs[sub_pos_x - 1];
        COEF_VER = COEFs[sub_pos_y - 1];
        if( sub_pos_y==0 ) //horizonal  position: a,b,c
        {
            if( tap == 4 )
            {
                g_funs_handle.ipflt[IPFILTER_H_4 ]( pSrc, i_ref, pDst, i_dst, bsize, bsize, COEF_HOR, 255 );
            }
            else if( tap == 6 )
            {
                g_funs_handle.ipflt[IPFILTER_H_6 ]( pSrc, i_ref, pDst, i_dst, bsize, bsize, COEF_HOR, 255 );
            }
            else if( tap == 10 )
            {
                g_funs_handle.ipflt[IPFILTER_H_10]( pSrc, i_ref, pDst, i_dst, bsize, bsize, COEF_HOR, 255 );
            }
        }
        else if( sub_pos_x==0 ) //vertical position: d,h,n
        {
            if( tap == 4 )
            {
                g_funs_handle.ipflt[IPFILTER_V_4 ]( pSrc, i_ref, pDst, i_dst, bsize, bsize, COEF_VER, 255 );
            }
            else if( tap == 6 )
            {
                g_funs_handle.ipflt[IPFILTER_V_6 ]( pSrc, i_ref, pDst, i_dst, bsize, bsize, COEF_VER, 255 );
            }
            else if( tap == 10 )
            {
                g_funs_handle.ipflt[IPFILTER_V_10]( pSrc, i_ref, pDst, i_dst, bsize, bsize, COEF_VER, 255 );
            }
        }
        else
        {
            if( tap == 4 )
            {
                g_funs_handle.ipflt_EXT[IPFILTER_EXT_4 ]( pSrc, i_ref, pDst, i_dst, bsize, bsize, COEF_HOR, COEF_VER, 255 );
            }
            else if( tap == 6 )
            {
                g_funs_handle.ipflt_EXT[IPFILTER_EXT_6 ]( pSrc, i_ref, pDst, i_dst, bsize, bsize, COEF_HOR, COEF_VER, 255 );
            }
            else if( tap == 10 )
            {
                g_funs_handle.ipflt_EXT[IPFILTER_EXT_10 ]( pSrc, i_ref, pDst, i_dst, bsize, bsize, COEF_HOR, COEF_VER, 255 );
            }
        }
    }
}

void get_luma_block( ImgParams  *img, pel_t *pDst, int i_dst, int bsize, int x_pos, int y_pos, uchar_t *ref_pic, int i_ref )
{
    int dx, dy;
    int x, y;
    int i, j;
    int int_pos_x, int_pos_y;
    int maxold_x,maxold_y;
    int result;
    int tmp_res[26][26];
    pel_t *pSrc;

    static const int COEF_HALF_8tap[8] =
    {
        -1, 4, -11, 40, 40, -11, 4, -1
    };
    static const int COEF_QUART1_8tap[8] =
    {
        -1, 4, -10, 57, 18, -6, 3, -1
    };
    static const int COEF_QUART3_8tap[8] =
    {
        -1, 3, -6, 18, 57, -10, 4, -1
    };

    static const int COEF_HALF_6tap[6] =
    {
        2, -9, 39, 39, -9, 2
    };
    static const int COEF_QUART1_6tap[6] =
    {
        2, -9, 57, 17, -4, 1
    };
    static const int COEF_QUART3_6tap[6] =
    {
        1, -4, 17, 57, -9, 2
    };

    if ( input->if_type == 1 )
    {
        get_block_adaptive( img, pDst, i_dst, bsize, x_pos, y_pos, ref_pic, i_ref );
        return;
    }
    //    A  a  b  c  B
    //    d  e  f  g
    //    h  i  j  k
    //    n  p  q  r
    //    C           D

    dx = x_pos&3;
    dy = y_pos&3;
    int_pos_x = (x_pos - dx) / 4;
    int_pos_y = (y_pos - dy) / 4;
    maxold_x = img->width-1;
    maxold_y = img->height-1;
    pSrc = ref_pic;
    pSrc = ref_pic + int_pos_y * i_ref + int_pos_x;

    if ( dx == 0 && dy == 0 ) // integer position: A
    {
        for ( j = 0; j < bsize; j++ )
        {
            for ( i = 0; i < bsize; i++ )
            {
                pDst[i] = *( pSrc+j*i_ref+i );
            }
            pDst += i_dst;
        }
    }
    else
    {
        /* other positions */
        if( ( dx==2 ) && ( dy==0 ) ) //horizonal 1/2 position: b
        {
            for ( j = 0; j < bsize; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    for ( result = 0, x = -3; x < 5; x++ ) //-3~4
                    {
                        result += ( *( pSrc+j*i_ref+i+x ) )*COEF_HALF_8tap[x+3];
                    }
                    pDst[i] = ( pel_t )MAX( 0, MIN( 255, ( result+32 )/64 ) );
                }
                pDst += i_dst;
            }
        }

        else if( ( dx==1 ) && ( dy==0 ) ) //horizonal 1/4 position: a
        {
            for ( j = 0; j < bsize; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    for ( result = 0, x = -3; x < 5; x++ ) //-3~4
                    {
                        result += (*(pSrc + j*i_ref + i + x))*COEF_QUART1_8tap[x + 3];
                    }
                    pDst[i] = ( pel_t )MAX( 0, MIN( 255, ( result+32 )/64 ) );
                }
                pDst += i_dst;
            }
        }

        else if( ( dx==3 ) && ( dy==0 ) ) //horizonal 3/4 position: a
        {
            for ( j = 0; j < bsize; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    for ( result = 0, x = -3; x < 5; x++ ) //-3~4
                    {
                        result += ( *( pSrc+j*i_ref+i+x ) )*COEF_QUART3_8tap[x+3];
                    }
                    pDst[i] = ( pel_t )MAX( 0, MIN( 255, ( result+32 )/64 ) );
                }
                pDst += i_dst;
            }
        }

        //vertical
        else if( ( dy==2 ) && ( dx==0 ) ) //vertical 1/2 position: h
        {
            for ( j = 0; j < bsize; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    for ( result = 0, y = -3; y < 5; y++ )
                    {
						result += (*(pSrc + (j + y)*i_ref + i))*COEF_HALF_8tap[y + 3];
                    }
                    pDst[i] = ( pel_t )MAX( 0, MIN( 255, ( result+32 )/64 ) );
                }
                pDst += i_dst;
            }
        }

        else if( ( dy==1 ) && ( dx==0 ) ) //vertical 1/4 position: d
        {
            for ( j = 0; j < bsize; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    for ( result = 0, y = -3; y < 5; y++ )
                    {
						result += (*(pSrc + (j + y)*i_ref + i))*COEF_QUART1_8tap[y + 3];
                    }
                    pDst[i] = ( pel_t )MAX( 0, MIN( 255, ( result+32 )/64 ) );
                }
                pDst += i_dst;
            }
        }

        else if( ( dy==3 ) && ( dx==0 ) ) //vertical 3/4 position: n
        {
            for ( j = 0; j < bsize; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    for ( result = 0, y = -3; y < 5; y++ )
                    {
						result += (*(pSrc + (j + y)*i_ref + i))*COEF_QUART3_8tap[y + 3];
                    }
                    pDst[i] = ( pel_t )MAX( 0, MIN( 255, ( result+32 )/64 ) );
                }
                pDst += i_dst;
            }
        }

        //remaining 9 fractional-pel pixels
        //e,i,p
        else if( ( dx==1 ) && ( dy==1 ) ) //horizontal 1/4 and vertical 1/4 position: e
        {
            for ( j = -2; j < bsize+3; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    for ( result = 0, x = -3; x < 5; x++ )
                    {
                        result += ( *( pSrc+j*i_ref+i+x ) )*COEF_QUART1_8tap[x+3];
                    }
                    tmp_res[j+2][i] = result;
                }
            }
            for ( j = 0; j < bsize; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    for ( result = 0, y = -2; y < 4; y++ )
                    {
                        result += tmp_res[j+y+2][i]*COEF_QUART1_6tap[y+2];
                    }
                    pDst[i] = ( pel_t )MAX( 0, MIN( 255, ( result+2048 )/4096 ) );
                }
                pDst += i_dst;
            }
        }

        else if( ( dx==1 ) && ( dy==2 ) ) //horizontal 1/4 and vertical 1/2 position: i
        {
            for ( j = -2; j < bsize+3; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    for ( result = 0, x = -3; x < 5; x++ )
                    {
                        result += ( *( pSrc+j*i_ref+i+x ) )*COEF_QUART1_8tap[x+3];
                    }
                    tmp_res[j+2][i] = result;
                }
            }
            for ( j = 0; j < bsize; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    for ( result = 0, y = -2; y < 4; y++ )
                    {
                        result += tmp_res[j+y+2][i]*COEF_HALF_6tap[y+2];
                    }
                    pDst[i] = ( pel_t )MAX( 0, MIN( 255, ( result+2048 )/4096 ) );
                }
                pDst += i_dst;
            }
        }

        else if( ( dx==1 ) && ( dy==3 ) ) //horizontal 1/4 and vertical 3/4 position: p
        {
            for ( j = -2; j < bsize+3; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    for ( result = 0, x = -3; x < 5; x++ )
                    {
                        result += ( *( pSrc+j*i_ref+i+x ) )*COEF_QUART1_8tap[x+3];
                    }
                    tmp_res[j+2][i] = result;
                }
            }
            for ( j = 0; j < bsize; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    for ( result = 0, y = -2; y < 4; y++ )
                    {
                        result += tmp_res[j+y+2][i]*COEF_QUART3_6tap[y+2];
                    }
                    pDst[i] = ( pel_t )MAX( 0, MIN( 255, ( result+2048 )/4096 ) );
                }
                pDst += i_dst;
            }
        }

        //f,j,q
        else if( ( dx==2 ) && ( dy==1 ) ) //horizontal 1/2 and vertical 1/4 position: f
        {
            for ( j = -2; j < bsize+3; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    for ( result = 0, x = -3; x < 5; x++ )
                    {
                        result += ( *( pSrc+j*i_ref+i+x ) )*COEF_HALF_8tap[x+3];
                    }
                    tmp_res[j+2][i] = result;
                }
            }
            for ( j = 0; j < bsize; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    for ( result = 0, y = -2; y < 4; y++ )
                    {
                        result += tmp_res[j+y+2][i]*COEF_QUART1_6tap[y+2];
                    }
                    pDst[i] = ( pel_t )MAX( 0, MIN( 255, ( result+2048 )/4096 ) );
                }
                pDst += i_dst;
            }
        }

        else if( ( dx==2 ) && ( dy==2 ) ) //horizontal 1/4 and vertical 1/2 position: j
        {
            for ( j = -2; j < bsize+3; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    for ( result = 0, x = -3; x < 5; x++ )
                    {
                        result += ( *( pSrc+j*i_ref+i+x ) )*COEF_HALF_8tap[x+3];
                    }
                    tmp_res[j+2][i] = result;
                }
            }
            for ( j = 0; j < bsize; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    for ( result = 0, y = -2; y < 4; y++ )
                    {
                        result += tmp_res[j+y+2][i]*COEF_HALF_6tap[y+2];
                    }
                    pDst[i] = ( pel_t )MAX( 0, MIN( 255, ( result+2048 )/4096 ) );
                }
                pDst += i_dst;
            }
        }

        else if( ( dx==2 ) && ( dy==3 ) ) //horizontal 1/2 and vertical 3/4 position: q
        {
            for ( j = -2; j < bsize+3; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    for ( result = 0, x = -3; x < 5; x++ )
                    {
                        result += ( *( pSrc+j*i_ref+i+x ) )*COEF_HALF_8tap[x+3];
                    }
                    tmp_res[j+2][i] = result;
                }
            }
            for ( j = 0; j < bsize; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    for ( result = 0, y = -2; y < 4; y++ )
                    {
                        result += tmp_res[j+y+2][i]*COEF_QUART3_6tap[y+2];
                    }
                    pDst[i] = ( pel_t )MAX( 0, MIN( 255, ( result+2048 )/4096 ) );
                }
                pDst += i_dst;
            }
        }


        //g,k,r
        else if( ( dx==3 ) && ( dy==1 ) ) //horizontal 3/4 and vertical 1/4 position: g
        {
            for ( j = -2; j < bsize+3; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    for ( result = 0, x = -3; x < 5; x++ )
                    {
                        result += ( *( pSrc+j*i_ref+i+x ) )*COEF_QUART3_8tap[x+3];
                    }
                    tmp_res[j+2][i] = result;
                }
            }
            for ( j = 0; j < bsize; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    for ( result = 0, y = -2; y < 4; y++ )
                    {
                        result += tmp_res[j+y+2][i]*COEF_QUART1_6tap[y+2];
                    }
                    pDst[i] = ( pel_t )MAX( 0, MIN( 255, ( result+2048 )/4096 ) );
                }
                pDst += i_dst;
            }
        }

        else if( ( dx==3 ) && ( dy==2 ) ) //horizontal 3/4 and vertical 1/2 position: k
        {
            for ( j = -2; j < bsize+3; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    for ( result = 0, x = -3; x < 5; x++ )
                    {
                        result += ( *( pSrc+j*i_ref+i+x ) )*COEF_QUART3_8tap[x+3];
                    }
                    tmp_res[j+2][i] = result;
                }
            }
            for ( j = 0; j < bsize; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    for ( result = 0, y = -2; y < 4; y++ )
                    {
                        result += tmp_res[j+y+2][i]*COEF_HALF_6tap[y+2];
                    }
                    pDst[i] = ( pel_t )MAX( 0, MIN( 255, ( result+2048 )/4096 ) );
                }
                pDst += i_dst;
            }
        }

        else if( ( dx==3 ) && ( dy==3 ) ) //horizontal 3/4 and vertical 3/4 position: r
        {
            for ( j = -2; j < bsize+3; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    for ( result = 0, x = -3; x < 5; x++ )
                    {
                        result += ( *( pSrc+j*i_ref+i+x ) )*COEF_QUART3_8tap[x+3];
                    }
                    tmp_res[j+2][i] = result;
                }
            }
            for ( j = 0; j < bsize; j++ )
            {
                for ( i = 0; i < bsize; i++ )
                {
                    for ( result = 0, y = -2; y < 4; y++ )
                    {
                        result += tmp_res[j+y+2][i]*COEF_QUART3_6tap[y+2];
                    }
                    pDst[i] = ( pel_t )MAX( 0, MIN( 255, ( result+2048 )/4096 ) );
                }
                pDst += i_dst;
            }
        }
    }
}

void get_chroma_subpix_Ext( const pel_t *pSrc, int i_ref, pel_t *pDst, int i_dst, const char_t *COEF_HOR, const char_t *COEF_VER )
{
    int val;
    const pel_t *pSrcTemp;
    int tmp[4], line;
    int col, row;

    for ( row = 0; row < 4; row++ )
    {
        for ( col = 0; col < 4; col++ )
        {
            for ( line = -1; line < 3; line++ )
            {
                pSrcTemp = pSrc + line *i_ref;
                tmp[line + 1] = FLT_4TAP_HOR( pSrcTemp, col, COEF_HOR );
            }
            val = tmp[0] * COEF_VER[0] + tmp[1] * COEF_VER[1] + tmp[2] * COEF_VER[2] + tmp[3] * COEF_VER[3];
            pDst[col] = ( pel_t )Clip1( ( val + 2048 ) >> 12 );
        }

        pSrc += i_ref;
        pDst += i_dst;
    }

}

void get_chroma_block( pel_t *pDst, int i_dst, int mv_x, int mv_y, int posx, int posy, uchar_t *refpic, int i_ref )
{
    pel_t *pSrc;

    const char_t COEFs[8][4] =
    {
        { 0, 64, 0, 0 },
        { -4, 62, 6, 0 },
        { -6, 56, 15, -1 },
        { -5, 47, 25, -3 },
        { -4, 36, 36, -4 },
        { -3, 25, 47, -5 },
        { -1, 15, 56, -6 },
        { 0, 6, 62, -4 }
    };
    const char_t *COEF_VER = COEFs[posy];
    const char_t *COEF_HOR = COEFs[posx];

    pSrc = refpic + ( mv_y>>3 )*i_ref + ( mv_x>>3 );
    if ( posy == 0 )
    {
        //get_chroma_subpix_Hor(pSrc, i_ref, pDst, i_dst, COEF_HOR);
        g_funs_handle.ipflt[IPFILTER_H_4]( pSrc, i_ref, pDst, i_dst, 4, 4, COEF_HOR, 255 );
    }
    else if ( posx == 0 )
    {
        //get_chroma_subpix_Ver(pSrc, i_ref, pDst, i_dst, COEF_VER);
        g_funs_handle.ipflt[IPFILTER_V_4]( pSrc, i_ref, pDst, i_dst, 4, 4, COEF_VER, 255 );
    }
    else
    {
        //get_chroma_subpix_Ext(pSrc, i_ref, pDst, i_dst, COEF_HOR, COEF_VER);
        g_funs_handle.ipflt_chroma_subpix_EXT[IPFILTER_EXT_4]( pSrc, i_ref, pDst, i_dst, COEF_HOR, COEF_VER );
    }
}

void com_funs_init_ip_filter()
{
    //g_funs_handle.ipcpy[IPFILTER_EXT_4] = com_if_filter_cpy;
    //g_funs_handle.ipcpy[IPFILTER_EXT_8] = com_if_filter_cpy;
    g_funs_handle.ipcpy = com_if_filter_cpy;
    g_funs_handle.ipflt[IPFILTER_H_4 ] = com_if_filter_hor_4;
    g_funs_handle.ipflt[IPFILTER_H_6 ] = com_if_filter_hor_6;
    g_funs_handle.ipflt[IPFILTER_H_10] = com_if_filter_hor_10;
    g_funs_handle.ipflt[IPFILTER_V_4 ] = com_if_filter_ver_4;
    g_funs_handle.ipflt[IPFILTER_V_6 ] = com_if_filter_ver_6;
    g_funs_handle.ipflt[IPFILTER_V_10] = com_if_filter_ver_10;

    g_funs_handle.ipflt_EXT[IPFILTER_EXT_4 ]  = com_if_filter_hor_ver_4;
    g_funs_handle.ipflt_EXT[IPFILTER_EXT_6 ]  = com_if_filter_hor_ver_6;
    g_funs_handle.ipflt_EXT[IPFILTER_EXT_10 ] = com_if_filter_hor_ver_10;

    g_funs_handle.ipflt_chroma_subpix_EXT[IPFILTER_EXT_4] = get_chroma_subpix_Ext;

    /*g_funs_handle.ipflt_EXT_H[IPFILTER_EXT_4 ] = com_if_filter_hor_ver_4_H;
    g_funs_handle.ipflt_EXT_H[IPFILTER_EXT_6 ] = com_if_filter_hor_ver_6_H;
    g_funs_handle.ipflt_EXT_H[IPFILTER_EXT_10] = com_if_filter_hor_ver_10_H;

    g_funs_handle.ipflt_EXT_V[IPFILTER_EXT_4 ] = com_if_filter_hor_ver_4_V;
    g_funs_handle.ipflt_EXT_V[IPFILTER_EXT_6 ] = com_if_filter_hor_ver_6_V;
    g_funs_handle.ipflt_EXT_V[IPFILTER_EXT_10] = com_if_filter_hor_ver_10_V;*/
}


int com_if_filter_hor_chroma( pel_t *src, int i_src, const int COEF[8][4], int curx, int cury, int posx )
{
    int x0, x1, x2, x3;
    int y0, y1, y2, y3;
    int val;

    x0 = MAX( 0, MIN( img->width_cr - 1, curx - 1 ) );
    x1 = MAX( 0, MIN( img->width_cr - 1, curx - 0 ) );
    x2 = MAX( 0, MIN( img->width_cr - 1, curx + 1 ) );
    x3 = MAX( 0, MIN( img->width_cr - 1, curx + 2 ) );
    y0 = y1 = y2 = y3 = MAX( 0, MIN( img->height_cr - 1, cury ) );

    val = src[y0 * i_src + x0] * COEF[posx][0] +
          src[y1 * i_src + x1] * COEF[posx][1] +
          src[y2 * i_src + x2] * COEF[posx][2] +
          src[y3 * i_src + x3] * COEF[posx][3];

    return val;
}

int com_if_filter_ver_chroma( pel_t *src, int i_src, const int COEF[8][4], int curx, int cury, int posy )
{
    int x0, x1, x2, x3;
    int y0, y1, y2, y3;
    int val;

    y0 = MAX( 0, MIN( img->height_cr - 1, cury - 1 ) );
    y1 = MAX( 0, MIN( img->height_cr - 1, cury - 0 ) );
    y2 = MAX( 0, MIN( img->height_cr - 1, cury + 1 ) );
    y3 = MAX( 0, MIN( img->height_cr - 1, cury + 2 ) );
    x0 = x1 = x2 = x3 = MAX( 0, MIN( img->width_cr - 1, curx ) );
    val = src[y0 * i_src + x0] * COEF[posy][0] +
          src[y1 * i_src + x1] * COEF[posy][1] +
          src[y2 * i_src + x2] * COEF[posy][2] +
          src[y3 * i_src + x3] * COEF[posy][3];

    return val;
}

int com_if_filter_hor_ver_chroma( pel_t *src, int i_src, const int COEF[8][4], int curx, int cury, int posx, int posy )
{
    int x0, x1, x2, x3;
    int y0, y1, y2, y3;
    int val;
    int tmp[4], line;

    for ( line = -1; line < 3; line++ )
    {
        x0 = MAX( 0, MIN( img->width_cr - 1, curx - 1 ) );
        x1 = MAX( 0, MIN( img->width_cr - 1, curx - 0 ) );
        x2 = MAX( 0, MIN( img->width_cr - 1, curx + 1 ) );
        x3 = MAX( 0, MIN( img->width_cr - 1, curx + 2 ) );
        y0 = y1 = y2 = y3 = MAX( 0, MIN( img->height_cr - 1, cury + line ) );
        tmp[line + 1] = src[y0 * i_src + x0] * COEF[posx][0] +
                        src[y1 * i_src + x1] * COEF[posx][1] +
                        src[y2 * i_src + x2] * COEF[posx][2] +
                        src[y3 * i_src + x3] * COEF[posx][3];
    }

    val = ( tmp[0] * COEF[posy][0] +
            tmp[1] * COEF[posy][1] +
            tmp[2] * COEF[posy][2] +
            tmp[3] * COEF[posy][3] );

    return val;
}

pel_t get_chroma_subpix( int curx, int cury, int posx, int posy, pel_t *src, int i_src )
{
    int val;
    static const int COEF[8][4] =
    {
        { 0, 64, 0, 0 },
        { -4, 62, 6, 0 },
        { -6, 56, 15, -1 },
        { -5, 47, 25, -3 },
        { -4, 36, 36, -4 },
        { -3, 25, 47, -5 },
        { -1, 15, 56, -6 },
        { 0, 6, 62, -4 }
    };

    if ( posy == 0 )
    {
        val = com_if_filter_hor_chroma( src, i_src, COEF, curx, cury, posx );
    }
    else if ( posx == 0 )
    {
        val = com_if_filter_ver_chroma( src, i_src, COEF, curx, cury, posy );
    }
    else
    {
        val = com_if_filter_hor_ver_chroma( src, i_src, COEF, curx, cury, posx, posy );
    }

    if (  posx != 0 && posy != 0 )
    {
      return ( pel_t )Clip1( ( val + 2048 ) >> 12 );
    } 
    else
    {
      return ( pel_t )Clip1( ( val + 32 ) / 64 );
    }
}

void get_chroma_block_enc( pel_t *pDst, int i_dst, int bsize, int int_mv_x, int int_mv_y, int posx, int posy, pel_t *refpic )
{
    int x, y;
    int curx, cury;
    for ( y = 0; y < bsize; y++ )
    {
        for ( x = 0; x < bsize; x++ )
        {
            curx = x + int_mv_x;
            cury = y + int_mv_y;
            pDst[x] = get_chroma_subpix( curx, cury, posx, posy, refpic, img->iStrideC );
        }
        pDst += i_dst;
    }
}

/*******************************************************************************
*  Function: calculated  frame distance between current frame and the reference(frame).
*  Input: blkref
*  Output: distance for motion vector scale
*******************************************************************************/
int calculate_distance( int blkref, int fw_bw ) //fw_bw>=0: forward prediction.
{
    int distance = 1;

    if ( img->type == P_IMG )
    {
        if ( blkref == 0 )
        {
            distance = img->pic_distance - img->imgtr_last_P;
        }
        else if ( ( blkref >= 1 && blkref <= img->real_ref_num ) )
        {
            distance = img->pic_distance - img->imgtr_prev_P;
        }
        else
        {
            assert( 0 ); //only two reference pictures for P frame
        }
    }
    else
    {
        if ( fw_bw >= 0 ) //forward
        {
            distance = img->pic_distance - img->imgtr_last_P;
        }
        else
        {
            distance = img->imgtr_next_P  - img->pic_distance;
        }
    }

    distance = ( distance + 256 ) % 256;
    return distance;
}

// return SYM backward MV
int GenSymBackMV(int forward_mv)
{
#if DISABLE_TEMPORAL_MV_SCALING
    return -forward_mv;
#else
    int DistanceIndexFw, DistanceIndexBw;
    DistanceIndexFw = calculate_distance(0, 0);
    DistanceIndexBw = calculate_distance(0, -1);
#if USE_H263_TEMPORAL_MV_SCALING
    if ((DistanceIndexFw + DistanceIndexBw)/2 < 5)
    {
      return -(forward_mv * DistanceIndexBw / DistanceIndexFw);
    }
    else
    {
      return -forward_mv;
    }
#else
    return (-((forward_mv * DistanceIndexBw * (512 / DistanceIndexFw) + 256) >> 9));
#endif
#endif
}

// spatial MV predictor without MV scaling
void SetSpatialMVPredictor ( ImgParams  *img,
                             Macroblock *currMB,
                             int         *pmv,
                             int         **refFrArr,
                             int         ***tmp_mv,
                             int         ref_frame,
                             int         b8_x_in_mb,
                             int         b8_y_in_mb,
                             int         bsize_x,
                             int         ref )
{
    int pix_x_in_mb      = b8_x_in_mb << 3;
    int pix_y_in_mb      = b8_y_in_mb << 3;
    int pu_b8_x          = img->mb_b8_x + b8_x_in_mb;
    int pu_b8_y          = img->mb_b8_y + b8_y_in_mb;
    int mb_width         = img->PicWidthInMbs;
    int mb_nr = img->current_mb_nr;
    int mb_available_up   = ( img->mb_y == 0 ) ? 0 : ( currMB->slice_nr == img->mb_data[mb_nr-mb_width].slice_nr );
    int mb_available_left = ( img->mb_x == 0 ) ? 0 : ( currMB->slice_nr == img->mb_data[mb_nr-1       ].slice_nr );
    int mb_available_up_left  = ( img->mb_x == 0 ) ? 0 : ( ( img->mb_y == 0 ) ? 0 :
                                ( currMB->slice_nr == img->mb_data[mb_nr-mb_width-1].slice_nr ) );
    int mb_available_up_right = ( img->mb_y == 0 ) ? 0 : ( ( img->mb_x >= ( mb_width-1 ) ) ? 0 :
                                ( currMB->slice_nr == img->mb_data[mb_nr-mb_width+1].slice_nr ) );

    int block_available_up, block_available_left, block_available_up_right, block_available_up_left;
    int mv_a, mv_b, mv_c, mv_d, pred_vec = 0;
    int mvPredType, rFrameL, rFrameU, rFrameUR, rFrameUL;
    int hv, diff_a, diff_b, diff_c;


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
            if ( pix_y_in_mb == 8 )
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
    else if ( pix_x_in_mb + bsize_x != MB_SIZE )
    {
        block_available_up_right = block_available_up;
    }
    else
    {
        block_available_up_right = mb_available_up_right;
    }

    rFrameL    = block_available_left     ? refFrArr[pu_b8_y]  [pu_b8_x-1] : -1;
    rFrameU    = block_available_up       ? refFrArr[pu_b8_y-1][pu_b8_x]   : -1;
    rFrameUR   = block_available_up_right ? refFrArr[pu_b8_y-1][pu_b8_x+ bsize_x / B8_SIZE] :
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
        mv_a = block_available_left    ? tmp_mv[pu_b8_y  ][4+pu_b8_x-1][hv] : 0;
        mv_b = block_available_up      ? tmp_mv[pu_b8_y-1][4+pu_b8_x  ][hv] : 0;
        mv_d = block_available_up_left ? tmp_mv[pu_b8_y-1][4+pu_b8_x-1][hv] : 0;
        mv_c = block_available_up_right? tmp_mv[pu_b8_y-1][4+pu_b8_x+bsize_x / B8_SIZE][hv] : mv_d;
        switch ( mvPredType )
        {
            case MVPRED_xy_MIN:
                mv_c = block_available_up_right ? mv_c : mv_d;

                if ( ( ( mv_a < 0 ) && ( mv_b > 0 ) && ( mv_c > 0 ) ) || ( mv_a > 0 ) && ( mv_b < 0 ) && ( mv_c < 0 ) )
                {
                    pmv[hv] = ( mv_b + mv_c ) / 2;
                }
                else if ( ( ( mv_b < 0 ) && ( mv_a > 0 ) && ( mv_c > 0 ) ) || ( ( mv_b > 0 ) && ( mv_a < 0 ) && ( mv_c < 0 ) ) )
                {
                    pmv[hv] = ( mv_c + mv_a ) / 2;
                }
                else if ( ( ( mv_c < 0 ) && ( mv_a > 0 ) && ( mv_b > 0 ) ) || ( ( mv_c > 0 ) && ( mv_a < 0 ) && ( mv_b < 0 ) ) )
                {
                    pmv[hv] = ( mv_a + mv_b ) / 2;
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
                    }
                    else if( pred_vec == diff_b )
                    {
                        pmv[hv] = ( mv_b + mv_c ) / 2;
                    }
                    else
                    {
                        pmv[hv] = ( mv_c + mv_a ) / 2;
                    }
                }
                break;

            case MVPRED_L:
                pmv[hv] = mv_a;
                break;
            case MVPRED_U:
                pmv[hv] = mv_b;
                break;
            case MVPRED_UR:
                pmv[hv] = mv_c;
                break;
            default:
                break;
        }
    }
}

void GetPskipMV( ImgParams *img, Macroblock *currMB, int bsize )
{
    int bx, by, pmv[2];
    int ***fmv = img->pfrm_mv;
    int mb_available_up   = ( img->mb_y == 0 )  ? 0 : ( currMB->slice_nr == img->mb_data[img->current_mb_nr - img->width/MB_SIZE].slice_nr );
    int mb_available_left = ( img->mb_x == 0 )  ? 0 : ( currMB->slice_nr == img->mb_data[img->current_mb_nr - 1].slice_nr );
    int zeroMotionAbove = !mb_available_up ? 1 : img->pfrm_ref[img->mb_b8_y - 1][img->mb_b8_x] == 0 && fmv[img->mb_b8_y - 1][4 + img->mb_b8_x][0] == 0 && fmv[img->mb_b8_y - 1][4 + img->mb_b8_x][1] == 0 ? 1 : 0;
    int zeroMotionLeft = !mb_available_left ? 1 : img->pfrm_ref[img->mb_b8_y][img->mb_b8_x - 1] == 0 && fmv[img->mb_b8_y][4 + img->mb_b8_x - 1][0] == 0 && fmv[img->mb_b8_y][4 + img->mb_b8_x - 1][1] == 0 ? 1 : 0;

    if ( zeroMotionAbove || zeroMotionLeft )
    {
        pmv[0] = pmv[1] = 0;
    }
    else
    {
        SetSpatialMVPredictor( img, currMB, pmv, img->pfrm_ref, img->pfrm_mv, 0, 0, 0, bsize, 0 );
    }

    for ( bx = 0; bx < 2; bx++ )
    {
        for ( by = 0; by < 2; by++ )
        {
            fmv[img->mb_b8_y + by][img->mb_b8_x + bx + BLOCK_SIZE][0] = pmv[0];
            fmv[img->mb_b8_y + by][img->mb_b8_x + bx + BLOCK_SIZE][1] = pmv[1];

            img->pfrm_ref[img->mb_b8_y + by][img->mb_b8_x + bx] = 0;
        }
    }
}

void GetBdirectMV( ImgParams *img, Macroblock *currMB, int bsize, int *fmv, int *bmv, int img_b8_x, int img_b8_y )
{
    int forw_ref_distance, pfrm_ref_distance, back_ref_distance;
    int pfrm_ref0_distance, pfrm_ref1_distance;
    int ref;
    int i;

    img->bfrm_fref[img_b8_y][img_b8_x] = 0;
    img->bfrm_bref[img_b8_y][img_b8_x] = 0;
    ref = img->pfrm_ref[img_b8_y][img_b8_x];
    if ( ref == -1 ) // if the collocated block of last P frame is intra, the MVs are generated based on spatial neighboring blocks
    {
        for ( i = 0; i < 2; i++ )
        {
            fmv[i] = 0;
            bmv[i] = 0;
        }
        SetSpatialMVPredictor( img, currMB, fmv, img->bfrm_fref, img->bfrm_fmv, 0, 0, 0, bsize, 0 );
        SetSpatialMVPredictor( img, currMB, bmv, img->bfrm_bref, img->bfrm_bmv, 0, 0, 0, bsize, -1 );
    }
    else
    {
        pfrm_ref0_distance = ( img->imgtr_next_P - img->imgtr_last_P ); // one P interval
        pfrm_ref0_distance = (pfrm_ref0_distance + 256) % 256;
        pfrm_ref1_distance = ( img->imgtr_next_P - img->imgtr_prev_P ); // two times P intervals
        pfrm_ref1_distance = (pfrm_ref1_distance + 256) % 256;
        pfrm_ref_distance  = ref == 0 ? pfrm_ref0_distance : pfrm_ref1_distance; // ref==0: first ref, the distance is one P interval; ref==1: second ref, the distance is two P intervals.
        pfrm_ref_distance = (pfrm_ref_distance + 256) % 256;

        forw_ref_distance = img->pic_distance - img->imgtr_last_P;
        forw_ref_distance = (forw_ref_distance + 256) % 256;
        back_ref_distance = img->imgtr_next_P - img->pic_distance;
        back_ref_distance = (back_ref_distance + 256) % 256;

        for ( i = 0; i < 2; i++ )
        {
#if DISABLE_TEMPORAL_MV_SCALING
            fmv[i] = img->pfrm_mv[img_b8_y][img_b8_x + 4][i];
            bmv[i] = -img->pfrm_mv[img_b8_y][img_b8_x + 4][i];
#else
#if USE_H263_TEMPORAL_MV_SCALING
          assert(pfrm_ref0_distance == forw_ref_distance + back_ref_distance);
          if (pfrm_ref0_distance < 5) // because here the P-P distance is doubled 
          {
            int pfactor = (ref == 0) ? 1 : 2;
            if (forw_ref_distance == 1 && back_ref_distance == 1)
            {
              fmv[i] = img->pfrm_mv[img_b8_y][img_b8_x + 4][i] / 2 / pfactor;
              bmv[i] = -(img->pfrm_mv[img_b8_y][img_b8_x + 4][i] / 2 / pfactor);
            }
            else if (forw_ref_distance == 1 && back_ref_distance == 2)
            {
              fmv[i] = img->pfrm_mv[img_b8_y][img_b8_x + 4][i] / 3 / pfactor;
              bmv[i] = -(img->pfrm_mv[img_b8_y][img_b8_x + 4][i] * 2 / 3 / pfactor);
            }
            else if (forw_ref_distance == 2 && back_ref_distance == 1)
            {
              fmv[i] = img->pfrm_mv[img_b8_y][img_b8_x + 4][i] * 2 / 3 / pfactor;
              bmv[i] = -(img->pfrm_mv[img_b8_y][img_b8_x + 4][i] / 3 / pfactor);
            }
            else if (forw_ref_distance == 1 && back_ref_distance == 3)
            {
              fmv[i] = img->pfrm_mv[img_b8_y][img_b8_x + 4][i] / 4 / pfactor;
              bmv[i] = -(img->pfrm_mv[img_b8_y][img_b8_x + 4][i] * 3 / 4 / pfactor);
            }
            else if (forw_ref_distance == 2 && back_ref_distance == 2)
            {
              fmv[i] = img->pfrm_mv[img_b8_y][img_b8_x + 4][i] / 2 / pfactor;
              bmv[i] = -(img->pfrm_mv[img_b8_y][img_b8_x + 4][i] / 2 / pfactor);
            }
            else if (forw_ref_distance == 3 && back_ref_distance == 1)
            {
              fmv[i] = img->pfrm_mv[img_b8_y][img_b8_x + 4][i] * 3 / 4 / pfactor;
              bmv[i] = -(img->pfrm_mv[img_b8_y][img_b8_x + 4][i] / 4 / pfactor);
            }
            else
            {
              assert(0);
            }
            //fmv[i] = forw_ref_distance * img->pfrm_mv[img_b8_y][img_b8_x + 4][i] / pfrm_ref0_distance / pfactor;
            //bmv[i] = -(back_ref_distance * img->pfrm_mv[img_b8_y][img_b8_x + 4][i] / pfrm_ref0_distance) / pfactor;
          }
          else
          {
            fmv[i] = img->pfrm_mv[img_b8_y][img_b8_x + 4][i];
            bmv[i] = -img->pfrm_mv[img_b8_y][img_b8_x + 4][i];
          }
#else
            if ( img->pfrm_mv[img_b8_y][img_b8_x + 4][i] < 0 )
            {
                fmv[i] =  -( ( ( 16384 / pfrm_ref_distance ) * ( 1 - forw_ref_distance * img->pfrm_mv[img_b8_y][img_b8_x + 4][i] ) - 1 ) >> 14 );
                bmv[i] = ( ( 16384 / pfrm_ref_distance ) * ( 1 - back_ref_distance * img->pfrm_mv[img_b8_y][img_b8_x + 4][i] ) - 1 ) >> 14;
            }
            else
            {
                fmv[i] = ((16384 / pfrm_ref_distance) * (1 + forw_ref_distance * img->pfrm_mv[img_b8_y][img_b8_x + 4][i]) - 1) >> 14;
                bmv[i] = -(((16384 / pfrm_ref_distance) * (1 + back_ref_distance * img->pfrm_mv[img_b8_y][img_b8_x + 4][i]) - 1) >> 14);
            }
#endif
#endif
        }
    }
}

void get_pred_mv( ImgParams *img, Macroblock *currMB, int b8, int bframe, int img_b8_x, int img_b8_y, int *vec1_x, int *vec1_y, int *vec2_x, int *vec2_y, int *refframe, int *fw_ref, int *bw_ref )
{
    int b8mode;
    int b8pdir;
    int blk_x = b8 % 2;
    int blk_y = b8 / 2;
    int img_x = img_b8_x << 3;
    int img_y = img_b8_y << 3;
    int img_x_4 = img_x << 2;
    int img_y_4 = img_y << 2;

    int ***frm_fmv = ( bframe ) ? img->bfrm_fmv : img->pfrm_mv;
    int ***frm_bmv = img->bfrm_bmv;
    int ***frm_mv;
    int **fref = ( bframe ) ? img->bfrm_fref : img->pfrm_ref;
    int **bref = img->bfrm_bref;
    int step_h, step_v;
    b8mode = currMB->b8mode[b8];
    b8pdir = currMB->b8pdir[b8];
    frm_mv = ( b8pdir == BACKWORD ) ? frm_bmv : frm_fmv;
    if ( b8pdir != BSYM )
    {
        if ( b8pdir == FORWARD || b8pdir == MHP || b8mode == PSKIP ) // MHP
        {
            step_h   = BLOCK_STEP [b8mode][0];
            step_v   = BLOCK_STEP [b8mode][1];
            *refframe = fref[img_b8_y][img_b8_x];

            if( b8pdir == MHP )
            {
                *vec2_x = img_x_4 + img->bmv_mhp[blk_x][blk_y][0];
                *vec2_y = img_y_4 + img->bmv_mhp[blk_x][blk_y][1];
            }
        }
        else // BACKWORD
        {
            *refframe = img->bfrm_bref[img_b8_y][img_b8_x];
        }

        if ( bframe )
        {
            *refframe = 0;
        }

        *vec1_x = img_x_4 + frm_mv[img_b8_y][img_b8_x + BLOCK_SIZE][0];
        *vec1_y = img_y_4 + frm_mv[img_b8_y][img_b8_x + BLOCK_SIZE][1];
    }
    else
    {
        int *fmv = img->bfrm_fmv[img_b8_y][img_b8_x + BLOCK_SIZE];
        int *bmv = img->bfrm_bmv[img_b8_y][img_b8_x + BLOCK_SIZE];

        if ( b8mode != PSKIP ) // BSYM
        {
            *fw_ref = img->bfrm_fref[img_b8_y][img_b8_x];
            *bw_ref = img->bfrm_bref[img_b8_y][img_b8_x];
        }
        else
        {
            *fw_ref = 0;
            *bw_ref = 0;
            img->bfrm_fref[img_b8_y][img_b8_x] = 0;
            img->bfrm_bref[img_b8_y][img_b8_x] = 0;
            GetBdirectMV( img, currMB, 16, fmv, bmv, img_b8_x, img_b8_y );
        }

        *vec1_x = img_x_4 + frm_fmv[img_b8_y][img_b8_x + BLOCK_SIZE][0];
        *vec1_y = img_y_4 + frm_fmv[img_b8_y][img_b8_x + BLOCK_SIZE][1];

        *vec2_x = img_x_4 + frm_bmv[img_b8_y][img_b8_x + BLOCK_SIZE][0];
        *vec2_y = img_y_4 + frm_bmv[img_b8_y][img_b8_x + BLOCK_SIZE][1];
    }
}

void get_mv_ref( ImgParams *img, Macroblock *currMB, int b8, int bframe, int img_b8_x, int img_b8_y, int *vec1_x, int *vec1_y, int *vec2_x, int *vec2_y, int *refframe, int *fw_ref, int *bw_ref )
{
    int b8mode;
    int b8pdir;
    int b8_x = b8 % 2;
    int b8_y = b8 / 2;

    int ***frm_fmv = ( bframe ) ? img->bfrm_fmv : img->pfrm_mv;
    int ***frm_bmv = img->bfrm_bmv;
    int ***frm_mv;
    int **fref = ( bframe ) ? img->bfrm_fref : img->pfrm_ref;
    int **bref = img->bfrm_bref;

    b8mode = currMB->b8mode[b8];
    b8pdir = currMB->b8pdir[b8];
    frm_mv = ( b8pdir == BACKWORD ) ? frm_bmv : frm_fmv;
    if ( b8pdir != BSYM )
    {
        if ( b8pdir == FORWARD || b8pdir == MHP || b8mode == PSKIP ) // MHP
        {
            *refframe = fref[img_b8_y][img_b8_x];

            if( b8pdir == MHP )
            {
                *vec2_x = img->bmv_mhp[b8_x][b8_y][0];
                *vec2_y = img->bmv_mhp[b8_x][b8_y][1];
            }
        }
        else     // BACKWORD
        {
            *refframe = bref[img_b8_y][img_b8_x];
        }

        *vec1_x = frm_mv[img_b8_y][img_b8_x + 4][0];
        *vec1_y = frm_mv[img_b8_y][img_b8_x + 4][1];
    }
    else
    {
        if ( b8mode != PSKIP )
        {
            *fw_ref = fref[img_b8_y][img_b8_x];
            *bw_ref = bref[img_b8_y][img_b8_x];
        }
        else
        {
            *fw_ref = 0;
            *bw_ref = 0;
        }

        *vec1_x = frm_fmv[img_b8_y][img_b8_x + 4][0];
        *vec1_y = frm_fmv[img_b8_y][img_b8_x + 4][1];

        *vec2_x = frm_bmv[img_b8_y][img_b8_x + 4][0];
        *vec2_y = frm_bmv[img_b8_y][img_b8_x + 4][1];
    }
}