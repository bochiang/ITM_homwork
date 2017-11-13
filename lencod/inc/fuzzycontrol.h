/*
*****************************************************************************
* Contributors: Yimin ZHOU, yiminzhou@uestc.edu.cn
*               Minke LUO, luominke@hotmail.com
*               Min ZHONG, 201321060446@std.uestc.edu.cn
* institution:  University of Electronic Science and Technology of China
******************************************************************************
*/

#ifndef _FUZZYCONTRAL_H_
#define _FUZZYCONTRAL_H_

#include <stdio.h>
#include "defines.h"

typedef struct RateControl
{
    int     RConoff;
    int     qp0;
    int     qpN;
    int     qpB;
    int     IntraPeriod;
    int     TotalFrames;
    int     CodedFrameNumber;
    int     ImageType;
    int     ImageQP;
    int     DeltaQP;
    int     FinalGopFrames;
    int     FrameWidth;
    int     FrameHeight;
    double  ImageBpp;
    double  Belta;
    double  GopBpp;
    int     GopAvgBaseQP;
    int     GopAvgBaseQPCount;
    double  GopAllKeyBpp;
    double  FirstBufferSizeLevel;
    double  IntraFrameBpp;
    double  InterFrameBpp;
    double  TargetBitPerPixel;
    double  TargetBufferLevel;
    double  DeltaBufferLevel;
    double  CurrentBufferSizeBpp;
    double  BufferError;
    double  BufferDifError;
    double  PreBufferError;
    int     GopFlag;
    FILE    *ReportFile;
} RateControl;

void Init_RateControl( RateControl * prc, int rconoff, int totalframes, int intraperiod, int targetbitrate, int framerate, int iniQP, int w, int h );
void Updata_RateControl( RateControl * prc, int framebits, int frameqp, int imgtype, int framenumber, int goplength );
int  CalculateGopDeltaQP_RateControl( RateControl * prc, int imgtype, int goplength );
void Init_FuzzyController( double ScaFactor );
int SetCommonTestDefaultTargetBitRate( char * name, int intraperiod, int qpI );

RateControl * pRC;
int gop_size_all;

#endif