#ifndef _MACROBLOCK_H_
#define _MACROBLOCK_H_

#include "global.h"

void init_mb_params ( Macroblock *currMB, uchar_t uhBitSize, int mb_nr );
void init_coding_state( ImgParams *img, CSobj *cs_aec, int mb_nr );
void init_img_pos( ImgParams *img, int mb_nr );
int writeMVD( Macroblock *currMB, CSobj *cs_aec );
int writeCBPandDqp( Macroblock *currMB, CSobj *cs_aec );

void write_one_cu( ImgParams *img, Macroblock* currMB, CSobj *cs_aec, int mb_nr, uchar_t uhBitSize, int bWrite );
void write_cu_tree( ImgParams *img, CSobj *cs_aec, unsigned char uhBitSize, int mb_nr );

#endif

