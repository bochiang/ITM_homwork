/*
*****************************************************************************
* Contributors: Yimin ZHOU, yiminzhou@uestc.edu.cn
*               Minke LUO, 201321060445@std.uestc.edu.cn
*               Min ZHONG, 201321060446@std.uestc.edu.cn
* Institution:  University of Electronic Science and Technology of China
******************************************************************************
*/

#include <math.h>
#include <string.h>
#include "fuzzycontrol.h"

double QueryTable[13][13]=
{
    { -4.80, -4.80, -4.80, -4.80, -3.57, -3.57, -3.17, -3.17, -2.00, -2.00, -0.25, -0.25,  0.00, },
    { -4.80, -4.80, -4.80, -4.80, -3.57, -3.57, -3.17, -3.17, -2.00, -2.00, -0.25, -0.25,  0.00, },
    { -4.80, -4.80, -3.57, -3.57, -3.57, -3.57, -2.00, -2.00, -1.10, -1.10,  0.00,  0.00,  0.25, },
    { -4.80, -4.80, -3.57, -3.57, -3.57, -3.57, -2.00, -2.00, -1.10, -1.10,  0.00,  0.00,  0.25, },
    { -3.57, -3.57, -3.57, -3.57, -2.00, -2.00, -1.10, -1.10,  0.00,  0.00,  1.10,  1.10,  2.00, },
    { -3.57, -3.57, -3.57, -3.57, -2.00, -2.00, -1.10, -1.10,  0.00,  0.00,  1.10,  1.10,  2.00, },
    { -3.17, -3.17, -2.00, -2.00, -1.10, -1.10,  0.00,  0.00,  1.10,  1.10,  2.00,  2.00,  3.17, },
    { -3.17, -3.17, -2.00, -2.00, -1.10, -1.10,  0.00,  0.00,  1.10,  1.10,  2.00,  2.00,  3.17, },
    { -2.00, -2.00, -1.10, -1.10,  0.00,  0.00,  1.10,  1.10,  2.00,  2.00,  3.57,  3.57,  3.57, },
    { -2.00, -2.00, -1.10, -1.10,  0.00,  0.00,  1.10,  1.10,  2.00,  2.00,  3.57,  3.57,  3.57, },
    { -0.25, -0.25,  0.00,  0.00,  1.10,  1.10,  2.44,  2.44,  3.57,  3.57,  3.57,  3.57,  4.80, },
    { -0.25, -0.25,  0.00,  0.00,  1.10,  1.10,  2.44,  2.44,  3.57,  3.57,  3.57,  3.57,  4.80, },
    {  0.00,  0.00,  0.25,  0.25,  2.00,  2.00,  3.57,  3.57,  3.86,  3.86,  4.80,  4.80,  4.80, },
};

char RateControlQueryTable[13][13];

void Init_FuzzyController( double ScaFactor )
{
    int i,j;
    for( i = 0; i < 13; i++ )
        for( j = 0; j < 13; j++ )
        {
            RateControlQueryTable[i][j] = QueryTable[i][j]>0.0F ? ( char )ceil( QueryTable[i][j]*ScaFactor ) : ( char )floor( QueryTable[i][j]*ScaFactor );
        }
}

int  GetNewFrameDeltaQP_FuzzyController( double ActualValue,double DeltaValue,double Amax,double Amin,double Bmax,double Bmin )
{
    double dFuzAct,dFuzDel;
    int iFuzAct,iFuzDel,ConVal;

    dFuzAct = ( 12.0/( Amax - Amin ) )*( ActualValue - ( Amax + Amin )/2.0 );
    dFuzDel = ( 12.0/( Bmax - Bmin ) )*( DeltaValue - ( Bmax + Bmin )/2.0 );

    dFuzAct = fabs( dFuzAct )>6 ? 6.0*dFuzAct/fabs( dFuzAct ) : dFuzAct;
    dFuzDel = fabs( dFuzDel )>6 ? 6.0*dFuzDel/fabs( dFuzDel ) : dFuzDel;

    iFuzAct = ( int )( ( dFuzAct<0 ? floor( dFuzAct + 0.5 ) : ceil( dFuzAct - 0.5 ) )+6 );
    iFuzDel = ( int )( ( dFuzDel<0 ? floor( dFuzDel + 0.5 ) : ceil( dFuzDel - 0.5 ) )+6 );

    ConVal = RateControlQueryTable[iFuzAct][iFuzDel];
    return ConVal;
}

void Init_RateControl( RateControl * prc, int rconoff, int totalframes, int intraperiod, int targetbitrate, int framerate, int iniQP, int w, int h )
{
    prc->RConoff = rconoff;
    prc->qp0 = iniQP;
    prc->qpN = prc->qp0 + 2;
    prc->qpB = prc->qp0 + 6;
    prc->IntraPeriod = intraperiod;
    prc->TotalFrames = totalframes;
    prc->CodedFrameNumber = 0;
    prc->ImageQP = iniQP;
    prc->FrameWidth = w;
    prc->FrameHeight = h;
    prc->GopBpp = 0.0F;
    prc->GopAllKeyBpp = 0.0F;
    prc->GopAvgBaseQP = 0;
    prc->GopAvgBaseQPCount = 0;
    prc->TargetBitPerPixel = ( double )( 1.0 ) * targetbitrate / framerate / w / h;
    prc->TargetBufferLevel = 0.0F;
    prc->FirstBufferSizeLevel = 0.0F;
    prc->PreBufferError = prc->BufferError = 0.0F;
    prc->CurrentBufferSizeBpp = 0.0;
    prc->GopFlag = -100;
    prc->DeltaQP = 0;
    prc->FinalGopFrames = intraperiod == 0 ? 0 : ( totalframes-( ( intraperiod-1 )*gop_size_all+1 ) )%( intraperiod*gop_size_all );
    intraperiod == 1 ? Init_FuzzyController( 0.50 ) : Init_FuzzyController( 0.25 );
}

void Updata_RateControl( RateControl * prc, int framebits, int frameqp, int imgtype, int framenumber, int goplength )
{
    int LevelLength = 0;
    int RestFrames = prc->TotalFrames - prc->CodedFrameNumber;
    int GopGroupFrames = prc->IntraPeriod * goplength;
    double PI = 3.1415926;
    prc->ImageQP = frameqp;
    prc->ImageType = imgtype;
    prc->ImageBpp = ( double )( 1.0 ) * framebits / prc->FrameWidth / prc->FrameHeight;
    prc->IntraFrameBpp = imgtype==0 ? prc->ImageBpp : prc->IntraFrameBpp;
    prc->InterFrameBpp = imgtype==1 || imgtype==2 || imgtype==0 ? prc->ImageBpp : prc->InterFrameBpp;
    if ( imgtype == 0 || imgtype == 1 )
    {
        prc->GopAvgBaseQP = imgtype == 0 ? prc->ImageQP : prc->GopAvgBaseQP + prc->ImageQP;
        prc->GopAllKeyBpp = imgtype == 0 ? prc->ImageBpp : prc->GopAllKeyBpp + prc->ImageBpp;
        prc->GopAvgBaseQPCount = imgtype == 0 ? 1 : prc->GopAvgBaseQPCount + 1;
    }

    prc->CodedFrameNumber++;

    if( imgtype!=2 )
    {
        prc->GopBpp = prc->ImageBpp;
    }
    else
    {
        prc->GopBpp += prc->ImageBpp;
    }

    prc->CurrentBufferSizeBpp +=  prc->ImageBpp - prc->TargetBitPerPixel;

    if( prc->IntraPeriod == 1 )
    {
        prc->TargetBufferLevel = prc->DeltaBufferLevel = 0.0;
    }
    else if( prc->IntraPeriod == 0 )
    {
        if( imgtype==0 )
        {
            prc->TargetBufferLevel = prc->CurrentBufferSizeBpp;
            prc->DeltaBufferLevel  = prc->TargetBufferLevel / prc->TotalFrames;
        }
        else
        {
            prc->TargetBufferLevel = prc->TargetBufferLevel - prc->DeltaBufferLevel;
        }
    }
    else if ( prc->IntraPeriod == 3 || prc->IntraPeriod == 2 )
    {
        if( imgtype==0 && prc->CodedFrameNumber<2 )
        {
            prc->FirstBufferSizeLevel = prc->CurrentBufferSizeBpp;
            prc->TargetBufferLevel = prc->CurrentBufferSizeBpp;
        }
        else
        {
            prc->TargetBufferLevel = prc->FirstBufferSizeLevel * cos( PI / 2 * prc->CodedFrameNumber / prc->TotalFrames );
        }
    }
    else
    {
        if( imgtype==0 && prc->CodedFrameNumber<2 )
        {
            prc->TargetBufferLevel = prc->CurrentBufferSizeBpp;
            LevelLength = prc->CodedFrameNumber<2 ? ( prc->IntraPeriod-1 )*goplength : GopGroupFrames-1;
            prc->DeltaBufferLevel = prc->TargetBufferLevel/( prc->IntraPeriod==0 ? prc->TotalFrames : min( LevelLength,prc->TotalFrames-framenumber ) );
        }
        else if ( imgtype==0 && ( prc->FinalGopFrames / gop_size_all < 3 ) && ( RestFrames <= ( GopGroupFrames + prc->FinalGopFrames ) ) && ( RestFrames > GopGroupFrames ) )
        {
            prc->TargetBufferLevel = prc->CurrentBufferSizeBpp;
            prc->DeltaBufferLevel =( 2.0 - 1.0 * prc->FinalGopFrames / GopGroupFrames ) * prc->TargetBufferLevel / RestFrames;
        }
        else if ( imgtype==0 && prc->CodedFrameNumber>=2 && ( RestFrames > GopGroupFrames ) )
        {
            prc->GopFlag = prc->CodedFrameNumber;
            prc->TargetBufferLevel = prc->DeltaBufferLevel = 0.0;
        }
        else if ( prc->GopFlag == prc->CodedFrameNumber - goplength )
        {
            prc->TargetBufferLevel = prc->CurrentBufferSizeBpp;
            if( RestFrames <= GopGroupFrames )
            {
                LevelLength = RestFrames;
            }
            else
            {
                LevelLength = ( prc->IntraPeriod-1 )*goplength-1;
            }
            prc->DeltaBufferLevel = prc->TargetBufferLevel / LevelLength;
            prc->GopFlag = -100;
        }
        else if ( imgtype==0 && prc->CodedFrameNumber>=2 && ( RestFrames <= GopGroupFrames ) && ( ( prc->FinalGopFrames / gop_size_all ) >= 3 ) )
        {
            prc->TargetBufferLevel = prc->CurrentBufferSizeBpp;
            prc->DeltaBufferLevel = prc->TargetBufferLevel / RestFrames;
        }
        else
        {
            prc->TargetBufferLevel = prc->TargetBufferLevel - prc->DeltaBufferLevel;
        }
    }
}

int  CalculateGopDeltaQP_RateControl( RateControl * prc, int imgtype, int goplength )
{
    int deltaQP = 0;
    double BufferRange,deltaBufferRange;
    double Tbpp;
    prc->Belta = imgtype==0 ? 0.12F : 0.12F;

    prc->BufferError = prc->CurrentBufferSizeBpp - prc->TargetBufferLevel;

    if ( ( prc->CodedFrameNumber % goplength == 1 ) || ( pRC->IntraPeriod == 1 ) || ( pRC->IntraPeriod == 0 ) )
    {
        prc->BufferDifError = prc->BufferError - prc->PreBufferError;
        prc->PreBufferError = prc->BufferError;
    }
    if( prc->IntraPeriod == 1 )
    {
        Tbpp = prc->IntraFrameBpp;
        BufferRange = Tbpp * prc->Belta * 2;
        BufferRange = prc->CodedFrameNumber < 2 ? Tbpp * 4 : BufferRange;
        BufferRange = BufferRange<0.0001 ? 0.0001 : BufferRange;
        deltaBufferRange = BufferRange * 2;
        deltaQP = GetNewFrameDeltaQP_FuzzyController( prc->BufferError, prc->BufferDifError, BufferRange, -BufferRange, deltaBufferRange, -deltaBufferRange );
        prc->DeltaQP = deltaQP;
        return deltaQP;
    }

    if( prc->IntraPeriod == 0 )
    {
        Tbpp = prc->InterFrameBpp;
        BufferRange = Tbpp * prc->Belta * 2;
        BufferRange = prc->CodedFrameNumber < 2 ? Tbpp * 4 : BufferRange;
        BufferRange = BufferRange<0.0001 ? 0.0001 : BufferRange;
        deltaBufferRange = BufferRange * 2;
        deltaQP = GetNewFrameDeltaQP_FuzzyController( prc->BufferError, prc->BufferDifError, BufferRange, -BufferRange, deltaBufferRange, -deltaBufferRange );
        prc->DeltaQP = deltaQP;
        return deltaQP;
    }

    if( prc->IntraPeriod > 1 )
    {
        if( imgtype==0 )
        {
            Tbpp = prc->GopAllKeyBpp;
            BufferRange = Tbpp * prc->Belta * 2;
            BufferRange = prc->CodedFrameNumber < 2 ? Tbpp * 4 : BufferRange;
            BufferRange = BufferRange<0.0001 ? 0.0001 : BufferRange;
            deltaBufferRange = BufferRange * 2;
            deltaQP = GetNewFrameDeltaQP_FuzzyController( prc->BufferError, prc->BufferDifError, BufferRange, -BufferRange, deltaBufferRange, -deltaBufferRange );
            prc->DeltaQP = deltaQP;
            return deltaQP;
        }
        else
        {
            Tbpp = prc->GopBpp;
            BufferRange = Tbpp * prc->Belta * 2;
            BufferRange = prc->CodedFrameNumber < 2 ? Tbpp * 4 : BufferRange;
            BufferRange = BufferRange<0.0001 ? 0.0001 : BufferRange;
            deltaBufferRange = BufferRange / 2;
            deltaQP = GetNewFrameDeltaQP_FuzzyController( prc->BufferError, prc->BufferDifError, BufferRange, -BufferRange, deltaBufferRange, -deltaBufferRange );
            prc->DeltaQP = deltaQP;
            return deltaQP;
        }
    }
    exit( 0x7777 );
}