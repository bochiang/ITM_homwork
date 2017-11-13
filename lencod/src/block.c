/*
*************************************************************************************
* File name:
* Function:
*
*************************************************************************************
*/
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <memory.h>

#include "defines.h"
#include "global.h"
#include "vlc.h"
#include "block.h"
#include "golomb.h"
#include "transform.h"

#include "../../common/pixel.h"
#include "../../common/common.h"
#include "../../common/intraPrediction.h"

#ifdef WIN32
#define int16 __int16
#else
#define int16 int16_t
#endif

//////////////////////////////////////////////////////////////////////////
i16u_t Q_TAB[64] =
{
    32768,29775,27554,25268,23170,21247,19369,17770,
    16302,15024,13777,12634,11626,10624,9742,8958,
    8192,7512,6889,6305,5793,5303,4878,4467,
    4091,3756,3444,3161,2894,2654,2435,2235,
    2048,1878,1722,1579,1449,1329,1218,1117,
    1024,939,861,790,724,664,609,558,
    512,470,430,395,362,332,304,279,
    256,235,215,197,181,166,152,140
};

i16u_t IQ_TAB[64] =
{

    32768,36061,38968,42495,46341,50535,55437,60424,
    32932,35734,38968,42495,46177,50535,55109,59933,
    65535,35734,38968,42577,46341,50617,55027,60097,
    32809,35734,38968,42454,46382,50576,55109,60056,
    65535,35734,38968,42495,46320,50515,55109,60076,
    65535,35744,38968,42495,46341,50535,55099,60087,
    65535,35734,38973,42500,46341,50535,55109,60097,
    32771,35734,38965,42497,46341,50535,55109,60099

};

short IQ_SHIFT[64] =
{
    14, 14, 14, 14, 14, 14, 14, 14,
    13, 13, 13, 13, 13, 13, 13, 13,
    13, 12, 12, 12, 12, 12, 12, 12,
    11, 11, 11, 11, 11, 11, 11, 11,
    11, 10, 10, 10, 10, 10, 10, 10,
    10,  9,  9,  9,  9,  9,  9,  9,
    9,  8,  8,  8,  8,  8,  8,  8,
    7,  7,  7,  7,  7,  7,  7,  7
};

const pel_t QP_SCALE_CR[64]=
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10,11,12,13,14,15,16,17,18,19,
    20,21,22,23,24,25,26,27,28,29,
    30,31,32,33,34,35,36,37,38,39,
    40,41,42,42,43,43,44,44,45,45,
    46,46,47,47,48,48,48,49,49,49,
    50,50,50,51,
};//Lou 1014

#define absm(A) ((A)<(0) ? (-(A)):(A))

extern const int IVC_SCAN4[16];
extern const int IVC_SCAN8[64];
extern const int IVC_SCAN16[256];

#define B8_SIZE 8

void pre_rdoquant( const int bsize, const int *src, int *dst )
{
    int i, j;
    i = j = 0;

    for ( i = 0; i < bsize; i++ )
    {
        memcpy( dst, src, bsize*sizeof( int ) );

        // skip
        src += bsize;
        dst += bsize;
    }
}

void post_rdoquant( const int bsize, const int *src, int *dst )
{
    int i, j;
    i = j = 0;

    for ( i = 0; i < bsize; i++ )
    {
        for ( j = 0; j<bsize; j++ )
        {
            dst[i* bsize + j] = ( src[i*bsize + j] > 0 ) ? dst[i* bsize + j] : -dst[i* bsize + j];
        }
    }
}

void quant( int qp, int isIntra, int *curr_blk, int bsize, int Q )   //intra_flag=>mode
{
    int x, y;
    int val, temp;
    int qp_const;

    if ( isIntra )
    {
        qp_const = ( 1 << 15 ) * 10 / 31;
    }
    else
    {
        qp_const = ( 1 << 15 ) * 10 / 62;
    }

    // quantization
    for ( y = 0; y < bsize; y++ )
    {
        for ( x = 0; x < bsize; x++ )
        {
            val = curr_blk[y* bsize + x];
            temp = absm( val );
            curr_blk[y* bsize + x] = sign( ( ( temp * Q + qp_const ) >> 15 ), val );
        }
    }

    return;
}

void inv_quant( int coef_num, int qp, int bsize, const int *IVC_SCAN, int *curr_blk, int shift, int QPI )
{
    //int xx, yy;
    int icoef;
    int val, temp;
    int clip1 = 0 - ( 1 << 15 );
    int clip2 = ( 1 << 15 ) - 1;

    // inv-quantization
    for ( icoef = 0; icoef < coef_num; icoef++ )
    {
        val = curr_blk[icoef];
        temp = ( val*QPI + ( 1 << ( shift - 1 ) ) ) >> shift;
        curr_blk[icoef] = Clip3( clip1, clip2, temp );
    }
}

/*
**********************************************************************
* RDO quantization functions
**********************************************************************
*/
// get the candidate levels, prepare for rdoq
void RDOquant_init( int qp, int isIntra, int curr_blk[MB_SIZE*MB_SIZE], levelDataStruct *levelData, int bsize )
{
    int xx, yy;
    double err, rec_double, temp;
    int level, lowerInt, ii, qp_const;
    int shift,QPI;
    int coeff_num = bsize * bsize;
    int coeff_ctr;

    if( isIntra > 3 )
    {
        qp_const = ( 1<<15 )*10/31;
    }
    else
    {
        qp_const = ( 1<<15 )*10/62;
    }

    shift = IQ_SHIFT[qp] + 1;
    QPI   = IQ_TAB[qp];

    for ( coeff_ctr=0; coeff_ctr<coeff_num; coeff_ctr++ )
    {
        xx = coeff_ctr % bsize;
        yy = coeff_ctr / bsize;

        levelData[coeff_ctr].levelDouble = absm( curr_blk[yy* bsize + xx] );

        level = ( int )( levelData[coeff_ctr].levelDouble*Q_TAB[qp]>>15 );
        rec_double = ( level*QPI+( 1<<( shift-1 ) ) ) >> ( shift );
        temp = 16384.0/Q_TAB[qp] ;
        lowerInt = ( ( levelData[coeff_ctr].levelDouble - rec_double ) <= temp ) ? 1 : 0;

        levelData[coeff_ctr].level[0] = 0;
        levelData[coeff_ctr].noLevels = 1;
        if ( level==0 && lowerInt==1 )
        {
            levelData[coeff_ctr].noLevels = 1;
        }
        else if ( level==0 && lowerInt==0 )
        {
            levelData[coeff_ctr].level[1] = 1;
            levelData[coeff_ctr].noLevels = 2;
        }
        else if ( level>0 && lowerInt==1 )
        {
            levelData[coeff_ctr].level[1] = level;
            levelData[coeff_ctr].noLevels = 2;
        }
        else
        {
            levelData[coeff_ctr].level[1] = level;
            levelData[coeff_ctr].level[2] = level+1;
            levelData[coeff_ctr].noLevels = 3;
        }

        for ( ii=0; ii<levelData[coeff_ctr].noLevels; ii++ )
        {
            err = ( double )( ( levelData[coeff_ctr].level[ii]*QPI+( 1<<( shift-1 ) ) )>>( shift ) )-( double )levelData[coeff_ctr].levelDouble;
            levelData[coeff_ctr].errLevel[ii] = err * err;
        }
        if( levelData[coeff_ctr].noLevels == 1 )
        {
            levelData[coeff_ctr].pre_level = 0;
            levelData[coeff_ctr].pre_err = levelData[coeff_ctr].errLevel[0];
        }
        else
        {
            levelData[coeff_ctr].pre_level = ( levelData[coeff_ctr].levelDouble * Q_TAB[qp]+qp_const )>>15;
            err = ( double )( ( levelData[coeff_ctr].pre_level*QPI+( 1<<( shift-1 ) ) )>>( shift ) )-( double )levelData[coeff_ctr].levelDouble;
            levelData[coeff_ctr].pre_err = err * err;
        }
    }
}

int RDOQ_estimate_bits( CSobj *cs_aec, int curr_blk[MB_SIZE*MB_SIZE], int bsize )
{
    int  run;
    int  xx, yy;
    int  icoef, ipos;
    int* ACLevel;
    int* ACRun;
    int  curr_val;
    int coef_num = bsize * bsize;
    const int* IVC_SCAN;
    int coefAC_tmp[2][LCU_SIZE*LCU_SIZE];

    ACLevel = coefAC_tmp[0];
    ACRun = coefAC_tmp[1];
    for ( xx = 0; xx < coef_num + 1; xx++ )
    {
        ACRun[xx] = ACLevel[xx] = 0;
    }

    run  = -1;
    ipos = 0;

    if ( bsize == 4 )
    {
        IVC_SCAN = IVC_SCAN4;
    }
    else if ( bsize == 8 )
    {
        IVC_SCAN = IVC_SCAN8;
    }
    else
    {
        IVC_SCAN = IVC_SCAN16;
    }

    for ( icoef=0; icoef<coef_num; icoef++ )
    {
        run++;
        xx = IVC_SCAN[icoef] % bsize;
        yy = IVC_SCAN[icoef] / bsize;

        curr_val = curr_blk[yy* bsize + xx];

        if ( curr_val != 0 )
        {
            ACLevel[ipos] = curr_val;
            ACRun[ipos]   = run;
            run = -1;
            ipos++;
        }
    }

    return writeLumaCoeff_AEC( cs_aec, bsize, ACLevel, ACRun );
}

void choose_level( CSobj *cs_aec, levelDataStruct *levelData, int curr_blk[MB_SIZE*MB_SIZE], double lambda, int bsize )
{
    int xx, yy, lastnonzero, coeff_ctr;
    int cstat, bestcstat = 0;
    double lagr, lagrAcc, minlagr = 0;
    int rate ;
    int coeff_num = bsize * bsize;
    CSptr cs_tmp  = create_coding_state ();

    store_coding_state( cs_tmp );

    lastnonzero = -1;
    lagrAcc = 0;
    for ( coeff_ctr=0; coeff_ctr<coeff_num; coeff_ctr++ )
    {
        xx = coeff_ctr % bsize;
        yy = coeff_ctr / bsize;
        lagrAcc += levelData[coeff_ctr].pre_err;
        curr_blk[yy* bsize + xx] = levelData[coeff_ctr].pre_level;
        if( levelData[coeff_ctr].noLevels > 1 )
        {
            lastnonzero = coeff_ctr;
        }
    }

    if( lastnonzero != -1 )
    {
        for( coeff_ctr=lastnonzero; coeff_ctr>=0; coeff_ctr-- )
        {
            xx = coeff_ctr % bsize;
            yy = coeff_ctr / bsize;
            if( levelData[coeff_ctr].noLevels == 1 )
            {
                curr_blk[yy* bsize + xx] = 0;
                continue;
            }

            lagrAcc -= levelData[coeff_ctr].pre_err;
            for( cstat=0; cstat<levelData[coeff_ctr].noLevels; cstat++ )
            {
                curr_blk[yy* bsize + xx] = ( int )levelData[coeff_ctr].level[cstat];

                lagr = lagrAcc + levelData[coeff_ctr].errLevel[cstat];
                rate = RDOQ_estimate_bits( cs_aec, curr_blk, bsize );

                lagr += lambda * rate ;

                if( cstat==0 || lagr<minlagr )
                {
                    minlagr = lagr;
                    bestcstat = cstat;
                }
                reset_coding_state( cs_tmp );
            }
            lagrAcc += levelData[coeff_ctr].errLevel[bestcstat];
            curr_blk[yy* bsize + xx] = ( int )levelData[coeff_ctr].level[bestcstat];
        }
    }
    reset_coding_state( cs_tmp );
    delete_coding_state ( cs_tmp );
}

void RDOquant( CSobj *cs_aec, int qp, int isIntra, int curr_blk[MB_SIZE*MB_SIZE], double lambda, int bsize )
{
    levelDataStruct levelData[256];
    int temp[256];

    pre_rdoquant( bsize, curr_blk, temp );

    RDOquant_init( qp, isIntra, curr_blk, levelData, bsize );
    choose_level( cs_aec, levelData, curr_blk, lambda, bsize );

    g_funs_handle.post_rdoquant( bsize, temp, curr_blk );

}

/*
*************************************************************************
* Function:
Quantization, scan and reconstruction of a transformed 8x8 bock
Return coeff_cost of the block.
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
int scanquant( ImgParams *img, CSobj *cs_aec, int isIntra, int b8, int b4,
               int curr_blk[MB_SIZE*MB_SIZE],
               int resi_blk[MB_SIZE*MB_SIZE],
               int *cbp,
               int bsize )
{
    int  run;
    int  xx, yy;
    int  icoef, ipos;
    int iBlkIdx = ( bsize==4 )? b4 : b8;
    int  coeff_cost = 0;
    int* ACLevel;
    int* ACRun;
    int  curr_val;
    int clip1 = 0 - ( 1 << 15 );
    int clip2 = ( 1 << 15 ) - 1;
    int coef_num = bsize * bsize;
    const int* IVC_SCAN;
    int isChroma = ( b8 >= 4 );
    int qp = ( isChroma )?QP_SCALE_CR[img->qp - MIN_QP]:img->qp - MIN_QP;
    double lambda = img->lambda;

    int quant_blk[MB_SIZE][MB_SIZE];
    int temp_blk[MB_SIZE*MB_SIZE];
    int quant_add, best_add = 0;
    int best_sad_cost = INT_MAX, curr_sad_cost;
    int scrFlag = ( img->type==B_IMG );

    // Quantization
    if ( input->rdo_q_flag && !isChroma )
    {
        RDOquant( cs_aec, qp, isIntra, curr_blk, lambda, bsize );
    }
    else
    {
        g_funs_handle.quant( qp, isIntra, curr_blk, bsize, Q_TAB[qp] );
    }
    if( isChroma )
    {
        ACLevel = img->coefAC_chroma[b8-4][0];
        ACRun = img->coefAC_chroma[b8-4][1];
    }
    else
    {
        ACLevel = img->coefAC_luma[0] + iBlkIdx * coef_num;
        ACRun = img->coefAC_luma[1] + iBlkIdx * coef_num;
    }

    memset( ACLevel, 0, coef_num * sizeof( int ) );
    memset( ACRun, 0, coef_num * sizeof( int ) );

    if ( bsize == 4 )
    {
        IVC_SCAN = IVC_SCAN4;
    }
    else if ( bsize == 8 )
    {
        IVC_SCAN = IVC_SCAN8;
    }
    else
    {
        IVC_SCAN = IVC_SCAN16;
    }

    run = -1;
    ipos = 0;
    if ( input->chroma_enhance == 1 && isChroma && isIntra )
    {
        for ( yy = 0; yy < bsize; ++yy )
        {
            for ( xx = 0; xx < bsize; ++xx )
            {
                quant_blk[yy][xx] = curr_blk[yy* bsize + xx];
            }
        }

        for ( quant_add = -1; quant_add <= 1; ++quant_add ) // Adapt chroma DC coefficient value
        {
            // get quantization block
            for ( yy = 0; yy < bsize; ++yy )
            {
                for ( xx = 0; xx < bsize; ++xx )
                {
                    temp_blk[yy*bsize + xx] = quant_blk[yy][xx];
                }
            }
            temp_blk[0] += quant_add;
            for ( icoef = 0; icoef < coef_num; icoef++ )
            {
                xx = IVC_SCAN[icoef] % bsize;
                yy = IVC_SCAN[icoef] / bsize;

                curr_val = temp_blk[yy*bsize + xx];
            }

            // Inverse quant (the shift bits is equal to the specified bit length minus 1, N - 1)
            g_funs_handle.inv_quant( coef_num, qp, bsize, IVC_SCAN, temp_blk, IQ_SHIFT[qp], IQ_TAB[qp] );
            // Inverse transform
            inv_transform( temp_blk, bsize );

            // Calculate SAD cost
            curr_sad_cost = 0;
            for ( yy = 0; yy < bsize; ++yy )
            {
                for ( xx = 0; xx < bsize; ++xx )
                {
                    curr_sad_cost += absm( temp_blk[yy*bsize + xx] - resi_blk[yy* bsize + xx] );
                }
            }

            // Select best coefficient value
            if ( curr_sad_cost < best_sad_cost )
            {
                best_add = quant_add;
                best_sad_cost = curr_sad_cost;

                for ( yy = 0; yy < bsize; ++yy )
                {
                    for ( xx = 0; xx < bsize; ++xx )
                    {
                        curr_blk[yy* bsize + xx] = temp_blk[yy*bsize + xx];
                    }
                }
            }
        }
        run = -1;
        ipos = 0;
        quant_blk[0][0] += best_add;
        for ( icoef = 0; icoef < coef_num; icoef++ )
        {
            run++;
            xx = IVC_SCAN[icoef] % bsize;
            yy = IVC_SCAN[icoef] / bsize;

            curr_val = quant_blk[yy][xx];

            if ( curr_val != 0 )
            {
                ACLevel[ipos] = curr_val;
                ACRun[ipos] = run;

                run = -1; // recover to original state
                ipos++;
            }
        }
    }
    else
    {
        for ( icoef = 0; icoef < coef_num; icoef++ )
        {
            run++;
            xx = IVC_SCAN[icoef] % bsize;
            yy = IVC_SCAN[icoef] / bsize;
            curr_val = curr_blk[yy * bsize + xx];

            if ( curr_val != 0 )
            {
                ACLevel[ipos] = curr_val;
                ACRun[ipos] = run;

                if ( scrFlag && absm( curr_val ) == 1 )
                {
                    coeff_cost += 1;
                }
                else
                {
                    coeff_cost += MAX_VALUE; // block has to be saved
                }
                run = -1;
                ipos++;
            }
        }
        // Inverse quant (the shift bits is equal to the specified bit length minus 1, N - 1)
        g_funs_handle.inv_quant( coef_num, qp, bsize, IVC_SCAN, curr_blk, IQ_SHIFT[qp], IQ_TAB[qp] );
        // Inverse transform
        inv_transform( curr_blk, bsize );
    }

    if ( ipos > 0 ) // if not all zero
    {
        if( !isChroma )
        {
            if ( bsize == 4 )
            {
                ( *cbp ) |= ( 0x1 << ( b8 * 4 + b4 ) );
            }
            else if ( bsize == 8 )
            {
                ( *cbp ) |= ( 0xF << ( b8 * 4 ) );
            }
            else
            {
                ( *cbp ) |= 0xFFFF;
            }
        }
        else
        {
            ( *cbp ) |= ( 0xF << ( b8 * 4 ) );
        }
    }
    return coeff_cost;
}

/*
*************************************************************************
* Function: Calculate SAD or SATD for a prediction error block of size
iSizeX x iSizeY.
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

int find_sad_8x8( int iMode,
                  int iSizeX,
                  int iSizeY,
                  int iOffX,
                  int iOffY,
                  int m7[MB_SIZE][MB_SIZE]
                )
{
    int i,j;
    int bmode;
    int ishift = 0;
    int sad    = 0;

    assert( ( iSizeX+iOffX ) <= MB_SIZE );
    assert( ( iSizeY+iOffY ) <= MB_SIZE );

    // m7[y,j,line][x,i,pixel]
    switch ( iMode )
    {
        case 0 : // ---------- SAD ----------
            for ( j=iOffY; j < iOffY+iSizeY; j++ )
                for ( i=iOffX; i < iOffX+iSizeX; i++ )
                {
                    sad += absm( m7[j][i] );
                }
            break;

        case 1 : // --------- SATD ----------
            bmode = iSizeX+iSizeY;
            if ( bmode<24 ) // 8x8
            {
                sad = sad_hadamard( iSizeY,iSizeX,iOffY,iOffX,m7 ); // Attention: sad_hadamard() is X/Y flipped
                ishift = 2;
                sad = ( sad + ( 1<<( ishift-1 ) ) )>>ishift;
            }
            else // 8x16-16x16
            {
                switch ( bmode )
                {
                    case 24 :               // 16x8 8x16
                        sad  = sad_hadamard( 8,8,iOffY,iOffX,m7 );
                        sad += sad_hadamard( 8,8,iOffY+( ( iSizeY==16 )?8:0 ) , iOffX+( ( iSizeX==16 )?8:0 ) ,m7 );
                        ishift = 2;
                        break;
                    case 32 :               // 16x16
                        sad  = sad_hadamard( 8,8,0,0,m7 );
                        sad += sad_hadamard( 8,8,8,0,m7 );
                        sad += sad_hadamard( 8,8,0,8,m7 );
                        sad += sad_hadamard( 8,8,8,8,m7 );
                        ishift = 2;
                        break;
                    default :
                        assert( 0==1 );
                }
                sad = ( sad + ( 1<<( ishift-1 ) ) )>>ishift;
            }
            break;

        default :
            assert( 0==1 );              // more switches may be added here later
    }

    return sad;
}

/*
*************************************************************************
* Function:
calculates the SAD of the Hadamard transformed block of
size iSizeX*iSizeY. Block may have an offset of (iOffX,iOffY).
If offset!=0 then iSizeX/Y has to be <=8.
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

int sad_hadamard( int iSizeX, int iSizeY, int iOffX, int iOffY, int m7[MB_SIZE][MB_SIZE] )
{
    int i,j,ii;
    int m1[MB_SIZE][MB_SIZE];
    int m2[MB_SIZE][MB_SIZE];
    int m3[MB_SIZE][MB_SIZE];
    int sad = 0;

    int iy[MB_SIZE] = {iOffY};

    // in this routine, cols are j,y and rows are i,x
    assert( ( ( iOffX==0 )||( iSizeX<=8 ) ) && ( ( iOffY==0 )||( iSizeY<=8 ) ) );
    for ( j=1; j<iSizeY; j++ )
    {
        iy[j] = iOffY + j;
    }

    // vertical transform
    if ( iSizeY == 4 )
        for ( i=0; i < iSizeX; i++ )
        {
            ii = i+iOffX;

            m1[i][0] = m7[ii][iy[0]] + m7[ii][iy[3]];
            m1[i][1] = m7[ii][iy[1]] + m7[ii][iy[2]];
            m1[i][2] = m7[ii][iy[1]] - m7[ii][iy[2]];
            m1[i][3] = m7[ii][iy[0]] - m7[ii][iy[3]];

            m3[i][0] = m1[i][0] + m1[i][1];
            m3[i][1] = m1[i][0] - m1[i][1];
            m3[i][2] = m1[i][2] + m1[i][3];
            m3[i][3] = m1[i][3] - m1[i][2];
        }
    else
        for ( i=0; i < iSizeX; i++ )
        {
            ii = i+iOffX;

            m1[i][0] = m7[ii][iy[0]] + m7[ii][iy[4]];
            m1[i][1] = m7[ii][iy[1]] + m7[ii][iy[5]];
            m1[i][2] = m7[ii][iy[2]] + m7[ii][iy[6]];
            m1[i][3] = m7[ii][iy[3]] + m7[ii][iy[7]];
            m1[i][4] = m7[ii][iy[0]] - m7[ii][iy[4]];
            m1[i][5] = m7[ii][iy[1]] - m7[ii][iy[5]];
            m1[i][6] = m7[ii][iy[2]] - m7[ii][iy[6]];
            m1[i][7] = m7[ii][iy[3]] - m7[ii][iy[7]];

            m2[i][0] = m1[i][0] + m1[i][2];
            m2[i][1] = m1[i][1] + m1[i][3];
            m2[i][2] = m1[i][0] - m1[i][2];
            m2[i][3] = m1[i][1] - m1[i][3];
            m2[i][4] = m1[i][4] + m1[i][6];
            m2[i][5] = m1[i][5] + m1[i][7];
            m2[i][6] = m1[i][4] - m1[i][6];
            m2[i][7] = m1[i][5] - m1[i][7];

            m3[i][0] = m2[i][0] + m2[i][1];
            m3[i][1] = m2[i][0] - m2[i][1];
            m3[i][2] = m2[i][2] + m2[i][3];
            m3[i][3] = m2[i][2] - m2[i][3];
            m3[i][4] = m2[i][4] + m2[i][5];
            m3[i][5] = m2[i][4] - m2[i][5];
            m3[i][6] = m2[i][6] + m2[i][7];
            m3[i][7] = m2[i][6] - m2[i][7];
        }

    // horizontal transform
    if ( iSizeX == 4 )
        for ( j=0; j < iSizeY; j++ )
        {
            m1[0][j]=m3[0][j]+m3[3][j];
            m1[1][j]=m3[1][j]+m3[2][j];
            m1[2][j]=m3[1][j]-m3[2][j];
            m1[3][j]=m3[0][j]-m3[3][j];

            m2[0][j]=m1[0][j]+m1[1][j];
            m2[1][j]=m1[0][j]-m1[1][j];
            m2[2][j]=m1[2][j]+m1[3][j];
            m2[3][j]=m1[3][j]-m1[2][j];

            for ( i=0; i < iSizeX; i++ )
            {
                sad += absm( m2[i][j] );
            }
        }
    else
        for ( j=0; j < iSizeY; j++ )
        {
            m2[0][j] = m3[0][j] + m3[4][j];
            m2[1][j] = m3[1][j] + m3[5][j];
            m2[2][j] = m3[2][j] + m3[6][j];
            m2[3][j] = m3[3][j] + m3[7][j];
            m2[4][j] = m3[0][j] - m3[4][j];
            m2[5][j] = m3[1][j] - m3[5][j];
            m2[6][j] = m3[2][j] - m3[6][j];
            m2[7][j] = m3[3][j] - m3[7][j];

            m1[0][j] = m2[0][j] + m2[2][j];
            m1[1][j] = m2[1][j] + m2[3][j];
            m1[2][j] = m2[0][j] - m2[2][j];
            m1[3][j] = m2[1][j] - m2[3][j];
            m1[4][j] = m2[4][j] + m2[6][j];
            m1[5][j] = m2[5][j] + m2[7][j];
            m1[6][j] = m2[4][j] - m2[6][j];
            m1[7][j] = m2[5][j] - m2[7][j];

            m2[0][j] = m1[0][j] + m1[1][j];
            m2[1][j] = m1[0][j] - m1[1][j];
            m2[2][j] = m1[2][j] + m1[3][j];
            m2[3][j] = m1[2][j] - m1[3][j];
            m2[4][j] = m1[4][j] + m1[5][j];
            m2[5][j] = m1[4][j] - m1[5][j];
            m2[6][j] = m1[6][j] + m1[7][j];
            m2[7][j] = m1[6][j] - m1[7][j];

            for ( i=0; i < iSizeX; i++ )
            {
                sad += absm( m2[i][j] );
            }
        }

    return( sad );
}

void com_funs_init_forquant()
{
    // quant
    g_funs_handle.post_rdoquant = post_rdoquant;
    g_funs_handle.quant         = quant;

    // inverse-quant
    g_funs_handle.inv_quant     = inv_quant;
}
