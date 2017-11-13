#ifndef _TDRDO_H_
#define _TDRDO_H_

#include "global.h"

#define MAXBLOCKSIZE  16
#define WORKBLOCKSIZE 16
#define SEARCHRANGE   64

typedef struct Block
{
    i32u_t  BlockWidth;
    i32u_t  BlockHeight;
    i32u_t  OriginX;
    i32u_t  OriginY;
    pel_t           lume[MAXBLOCKSIZE*MAXBLOCKSIZE];    // 4096==64*64
    pel_t           cr[MAXBLOCKSIZE*MAXBLOCKSIZE/4];    // 1024==4096/4
    pel_t           cb[MAXBLOCKSIZE*MAXBLOCKSIZE/4];    // 1024==4096/4
} Block;

typedef struct Frame
{
    i32u_t    FrameWidth;
    i32u_t    FrameHeight;
    i32u_t    nStrideY;
    i32u_t    nStrideC;
    pel_t   * base;
    pel_t   * Y;
    pel_t   * U;
    pel_t   * V;
} Frame;

typedef struct BlockDistortion
{
    i32u_t  GlobalBlockNumber;
    i16u_t  BlockNumInHeight;
    i16u_t  BlockNumInWidth;
    i16u_t  BlockWidth;
    i16u_t  BlockHeight;
    i16u_t  OriginX;
    i16u_t  OriginY;
    i16u_t  SearchRange;
    //  short           MVx;
    //  short           MVy;
    double          MSE;
    //  double          MVL;
    short           BlockQP;
    double          BlockLambda;
    short           BlockType;
} BlockDistortion,BD;

typedef struct FrameDistortion
{
    i32u_t  FrameNumber;
    i32u_t  BlockSize;
    i32u_t  CUSize;
    i32u_t  TotalNumOfBlocks;
    i32u_t  TotalBlockNumInHeight;
    i32u_t  TotalBlockNumInWidth;
    BD              *BlockDistortionArray;
} FrameDistortion,FD;

typedef struct DistortionList
{
    i32u_t  TotalFrameNumber;
    i32u_t  FrameWidth;
    i32u_t  FrameHeight;
    i32u_t  BlockSize;
    FD              *FrameDistortionArray;
} DistortionList,DL;

DL * CreatDistortionList( i32u_t totalframenumber, i32u_t w, i32u_t h, i32u_t blocksize, i32u_t cusize );
void DestroyDistortionList( DL * SeqD );
void MotionDistortion( FD *currentFD, Frame * FA, Frame * FB, i32u_t searchrange );
void StoreLCUInf( FD *curRealFD, int LeaderBlockNumber, int cuinwidth, int iqp, double dlambda, int curtype );
void CaculateKappaTableLDP( DL *omcplist, DL *realDlist, int keyframenum, int FrameQP );
void CaculateKappaTableRA( DL *omcplist, DL *realDlist, int framenum, int FrameQP );
void AdjustLcuQPLambdaLDP( FD * curOMCPFD, int LeaderBlockNumber, int cuinwidth, double *plambda );

Frame *porgF,*ppreF,*precF,*prefF;

DL *OMCPDList;
DL *RealDList;
FD *pOMCPFD, *pRealFD,*subpOMCPFD ;

int     StepLength;
double  AvgBlockMSE;
double  *KappaTable;
double  *KappaTable1;
int Gop_size_all;
double  GlobeLambdaRatio;
int     GlobeFrameNumber;
int     CurMBQP;
int     QpOffset[32];
int     GroupSize;

#endif
