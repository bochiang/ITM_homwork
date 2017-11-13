#ifndef _PIXEL_H_
#define _PIXEL_H_

void com_cpy( const pel_t *src, int i_src, pel_t *dst, int i_dst, int width, int height );

void add_pel_clip( int b8, int b4, int* curr_blk, int bsize, \
                   pel_t *ppredblk, int ipix_y, \
                   int iStride, int ipix_x );

void avg_pel_1d( pel_t *dst, pel_t *src1, pel_t *src2, int len );
void avg_pel( pel_t *dst, int i_dst, pel_t *src1, int i_src1, pel_t *src2, int i_src2, int width, int height );

void image_padding( ImgParams *img, uchar_t *rec_y, uchar_t *rec_u, uchar_t *rec_v, int pad );

void recon_luma_blk(ImgParams *img, int b8, int b4, int* curr_blk, int bsize);
void recon_chroma_blk(ImgParams *img, int uv, int b4, int* resi_blk, pel_t *pred_blk, int bsize);


#endif