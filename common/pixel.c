#include "defines.h"
#include "global.h"
#include <memory.h>
#include "../../common/common.h"

void com_cpy( const pel_t *src, int i_src, pel_t *dst, int i_dst, int width, int height )
{
    int row, col;

    for ( row = 0; row < height; row++ )
    {
        for ( col = 0; col < width; col++ )
        {
            dst[col] = src[col];
        }
        src += i_src;
        dst += i_dst;
    }
}

// no use...
void avg_pel_1d( pel_t *dst, pel_t *src1, pel_t *src2, int len )
{
    int j;
    for ( j = 0; j < len; j++ )
    {
        dst[j] = ( src1[j] + src2[j] + 1 ) >> 1;
    }
}

void avg_pel( pel_t *dst, int i_dst, pel_t *src1, int i_src1, pel_t *src2, int i_src2, int width, int height )
{
    int i, j;
    for ( i = 0; i < height; i++ )
    {
        for ( j = 0; j < width; j++ )
        {
            dst[j] = ( src1[j] + src2[j] + 1 ) >> 1;
        }
        dst += i_dst;
        src1 += i_src1;
        src2 += i_src2;
    }
}

void padding_rows( pel_t *src, int i_src, int width, int height, int pad )
{
    int i, j;
    pel_t *p;

    // bottom border
    p = src + i_src * ( height - 1 );
    for ( i = 1; i <= pad; i++ )
    {
        memcpy( p + i_src * i, p, width * sizeof( pel_t ) );
    }

    // above border
    p = src;
    for ( i = 1; i <= pad; i++ )
    {
        memcpy( p - i_src * i, p, width * sizeof( pel_t ) );
    }

    p = src - pad * i_src;

    // left & right
    for ( i = 0; i < height + 2 * pad; i++ )
    {
        for ( j = 0; j < pad; j++ )
        {
            p[-pad + j] = p[0];
            p[width + j] = p[width - 1];
        }
        p += i_src;
    }
}

// iPadSize = input->searchrange + max_ipfilt_size; // max_ipfilt_size = 5
void image_padding( ImgParams *img, uchar_t *rec_y, uchar_t *rec_u, uchar_t *rec_v, int pad )
{
    int padc = pad >> 1;

    g_funs_handle.padding_rows( rec_y, img->iStride, img->width, img->height, pad );
    g_funs_handle.padding_rows( rec_u, img->iStrideC, img->width_cr, img->height_cr, padc );
    g_funs_handle.padding_rows( rec_v, img->iStrideC, img->width_cr, img->height_cr, padc );
}

void com_funs_init_pixel_opt()
{
    g_funs_handle.com_cpy = com_cpy;
    g_funs_handle.avg_pel = avg_pel;
    g_funs_handle.padding_rows = padding_rows;

#ifdef MT_ENABLE
    g_funs_handle.padding_rows_LR_mt = padding_rows_LR_mt;
#endif // MT_ENABLE
}

void recon_luma_blk(ImgParams *img, int b8, int b4, int* curr_blk, int bsize)
{
    int  x, y;
    int  by = B8_SIZE * (b8 / 2) + 4 * (b4 / 2);
    int  bx = B8_SIZE * (b8 % 2) + 4 * (b4 % 2);
    int  curr_val;

    for (y = 0; y < bsize; y++) {
        for (x = 0; x < bsize; x++) {
            curr_val = img->pred_blk_luma[(by + y) * MB_SIZE + bx + x] + curr_blk[y*bsize + x];
            imgY_rec[(img->mb_pix_y + by + y)*img->iStride + img->mb_pix_x + bx + x] = (uchar_t)clamp(curr_val, 0, 255);
        }
    }
}

void recon_chroma_blk(ImgParams *img, int uv, int b4, int* resi_blk, pel_t *pred_blk, int bsize)
{
    int x, y;
    int curr_val;

    for (y = 0; y < bsize; y++) {
        for (x = 0; x < bsize; x++) {
            curr_val = pred_blk[y * MB_SIZE + x] + resi_blk[y*bsize + x];
            if (uv == 1)
                imgV_rec[(img->mb_pix_y_cr + y)*img->iStrideC + img->mb_pix_x_cr + x] = (uchar_t)clamp(curr_val, 0, 255);
            else
                imgU_rec[(img->mb_pix_y_cr + y)*img->iStrideC + img->mb_pix_x_cr + x] = (uchar_t)clamp(curr_val, 0, 255);
        }
    }
}