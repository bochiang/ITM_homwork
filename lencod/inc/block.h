#ifndef _BLOCK_H_
#define _BLOCK_H_

#include "global.h"

extern const int BLOCK_STEP[5][2];

typedef struct level_data_struct
{
    int         level[3];    // candidate levels
    int         levelDouble; // coefficient before quantization
    double      errLevel[3]; // quantization errors of each candidate
    int         noLevels;    // number of candidate levels
    int         pre_level;   // norm quantization level
    double      pre_err;     // quantization errors of norm level
} levelDataStruct;

// functions
void xPrepareIntraPattern( ImgParams *img, Macroblock *currMB, pel_t *pedge, int img_x, int img_y, int bsize, bool *bAboveAvail, bool *bLeftAvail );

void transform( const int *curr_blk, int *coef_blk, int bsize );
void inv_transform( int *curr_blk, int bsize );
void quant( int qp, int mode, int *curr_blk, int bsize, int Q );
int  scanquant( ImgParams *img, CSobj *cs_aec, int isIntra, int b8, int b4, int curr_blk[MB_SIZE*MB_SIZE], int resi_blk[MB_SIZE*MB_SIZE], int *cbp, int bsize );

int  find_sad_8x8( int iMode, int iSizeX, int iSizeY, int iOffX, int iOffY, int m7[MB_SIZE][MB_SIZE] );
int  sad_hadamard( int iSizeX, int iSizeY, int iOffX, int iOffY, int m7[MB_SIZE][MB_SIZE] );

void com_funs_init_forquant();

#endif
