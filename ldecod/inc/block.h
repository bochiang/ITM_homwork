#ifndef _BLOCK_H_
#define _BLOCK_H_

// external variables
extern const uchar_t QP_SCALE_CR[64] ;

extern const int IVC_SCAN4[16];
extern const int IVC_SCAN8[64];
extern const int IVC_SCAN16[256];
extern const int BLOCK_STEP[5][2];

// functions
void IntraLumaPred( ImgParams *img, Macroblock *currMB, int img_x, int img_y, int bsize );

void inv_transform( int *curr_blk, int bsize );
void inv_quant_luma ( ImgParams *img, Macroblock *currMB, int *curr_blk, int b8, int b4, int bsize );

void DecodeIntraChromaCU( ImgParams *img, Macroblock *currMB, uchar_t uhBitSize );
void DecodeIntraLumaCU( ImgParams *img, Macroblock *currMB, uchar_t uhBitSize );
void DecodeInterChromaCU( ImgParams *img, Macroblock *currMB, uchar_t uhBitSize );
void DecodeInterLumaCU( ImgParams *img, Macroblock *currMB, uchar_t uhBitSize );

#endif