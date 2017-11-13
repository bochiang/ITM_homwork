#include "defines.h"
#include "transform.h"
#include "../../common/common.h"
#include "header.h"

i16u_t IQ_TAB[64] =
{

    32768, 36061, 38968, 42495, 46341, 50535, 55437, 60424,
    32932, 35734, 38968, 42495, 46177, 50535, 55109, 59933,
    65535, 35734, 38968, 42577, 46341, 50617, 55027, 60097,
    32809, 35734, 38968, 42454, 46382, 50576, 55109, 60056,
    65535, 35734, 38968, 42495, 46320, 50515, 55109, 60076,
    65535, 35744, 38968, 42495, 46341, 50535, 55099, 60087,
    65535, 35734, 38973, 42500, 46341, 50535, 55109, 60097,
    32771, 35734, 38965, 42497, 46341, 50535, 55109, 60099

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

const int trans_core_16[16][16] =
{
    { 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32 },
    { 45, 43, 40, 35, 29, 21, 13, 4, -4, -13, -21, -29, -35, -40, -43, -45 },
    { 44, 38, 25, 9, -9, -25, -38, -44, -44, -38, -25, -9, 9, 25, 38, 44 },
    { 43, 29, 4, -21, -40, -45, -35, -13, 13, 35, 45, 40, 21, -4, -29, -43 },
    { 42, 17, -17, -42, -42, -17, 17, 42, 42, 17, -17, -42, -42, -17, 17, 42 },
    { 40, 4, -35, -43, -13, 29, 45, 21, -21, -45, -29, 13, 43, 35, -4, -40 },
    { 38, -9, -44, -25, 25, 44, 9, -38, -38, 9, 44, 25, -25, -44, -9, 38 },
    { 35, -21, -43, 4, 45, 13, -40, -29, 29, 40, -13, -45, -4, 43, 21, -35 },
    { 32, -32, -32, 32, 32, -32, -32, 32, 32, -32, -32, 32, 32, -32, -32, 32 },
    { 29, -40, -13, 45, -4, -43, 21, 35, -35, -21, 43, 4, -45, 13, 40, -29 },
    { 25, -44, 9, 38, -38, -9, 44, -25, -25, 44, -9, -38, 38, 9, -44, 25 },
    { 21, -45, 29, 13, -43, 35, 4, -40, 40, -4, -35, 43, -13, -29, 45, -21 },
    { 17, -42, 42, -17, -17, 42, -42, 17, 17, -42, 42, -17, -17, 42, -42, 17 },
    { 13, -35, 45, -40, 21, 4, -29, 43, -43, 29, -4, -21, 40, -45, 35, -13 },
    { 9, -25, 38, -44, 44, -38, 25, -9, -9, 25, -38, 44, -44, 38, -25, 9 },
    { 4, -13, 21, -29, 35, -40, 43, -45, 45, -43, 40, -35, 29, -21, 13, -4 }
};

void partialButterflyInverse4x4( int *curr_blk )
{
    int i, j;
    int temp1[4], temp;

    for ( i = 0; i < 4; i++ )
    {
        temp1[0] = curr_blk[i * 4] + curr_blk[i * 4 + 2];
        temp1[2] = curr_blk[i * 4] - curr_blk[i * 4 + 2];
        temp = ( curr_blk[i * 4 + 1] + curr_blk[i * 4 + 3] ) * 69 >> 7;
        temp1[1] = temp + ( curr_blk[i * 4 + 1] * 98 >> 7 );
        temp1[3] = temp - ( curr_blk[i * 4 + 3] * 236 >> 7 );

        curr_blk[i * 4] = temp1[0] + temp1[1];
        curr_blk[i * 4 + 3] = temp1[0] - temp1[1];
        curr_blk[i * 4 + 1] = temp1[2] + temp1[3];
        curr_blk[i * 4 + 2] = temp1[2] - temp1[3];
    }
    for ( i = 0; i < 4; i++ )
    {
        temp1[0] = curr_blk[i] + curr_blk[2 * 4 + i];
        temp1[2] = curr_blk[i] - curr_blk[2 * 4 + i];
        temp = ( curr_blk[1 * 4 + i] + curr_blk[3 * 4 + i] ) * 69 >> 7;
        temp1[1] = temp + ( curr_blk[1 * 4 + i] * 98 >> 7 );
        temp1[3] = temp - ( curr_blk[3 * 4 + i] * 236 >> 7 );

        curr_blk[i] = temp1[0] + temp1[1];
        curr_blk[3 * 4 + i] = temp1[0] - temp1[1];
        curr_blk[1 * 4 + i] = temp1[2] + temp1[3];
        curr_blk[2 * 4 + i] = temp1[2] - temp1[3];

        for ( j = 0; j<4; j++ )
        {

            if ( curr_blk[j * 4 + i]>0 )
            {
                curr_blk[j * 4 + i] = ( curr_blk[j * 4 + i] + 4 ) >> 3;
            }
            else
            {
                curr_blk[j * 4 + i] = -( ( -curr_blk[j * 4 + i] + 4 ) >> 3 );
            }
        }
    }
}

void partialButterflyInverse8x8( int *curr_blk )
{
    int i, j;
    int temp1[8], temp2[8];

    for ( i = 0; i < 8; i++ )
    {
        // step 1
        temp1[0] = ( curr_blk[i * 8] + curr_blk[i * 8 + 4] ) * 181 >> 7;
        temp1[1] = ( curr_blk[i * 8] - curr_blk[i * 8 + 4] ) * 181 >> 7;
        temp1[2] = ( curr_blk[i * 8 + 2] * 196 >> 8 ) - ( curr_blk[i * 8 + 6] * 473 >> 8 );
        temp1[3] = ( curr_blk[i * 8 + 2] * 473 >> 8 ) + ( curr_blk[i * 8 + 6] * 196 >> 8 );

        temp2[4] = curr_blk[i * 8 + 1] - curr_blk[i * 8 + 7];
        temp2[7] = curr_blk[i * 8 + 1] + curr_blk[i * 8 + 7];
        temp2[5] = curr_blk[i * 8 + 3] * 181 >> 7;
        temp2[6] = curr_blk[i * 8 + 5] * 181 >> 7;
        temp1[4] = temp2[4] + temp2[6];
        temp1[5] = temp2[7] - temp2[5];
        temp1[6] = temp2[4] - temp2[6];
        temp1[7] = temp2[7] + temp2[5];

        // step 2
        temp2[0] = temp1[0] + temp1[3];
        temp2[3] = temp1[0] - temp1[3];
        temp2[1] = temp1[1] + temp1[2];
        temp2[2] = temp1[1] - temp1[2];
        temp2[4] = ( temp1[4] * 301 >> 8 ) - ( temp1[7] * 201 >> 8 );
        temp2[7] = ( temp1[4] * 201 >> 8 ) + ( temp1[7] * 301 >> 8 );
        temp2[5] = ( temp1[5] * 710 >> 9 ) - ( temp1[6] * 141 >> 9 );
        temp2[6] = ( temp1[5] * 141 >> 9 ) + ( temp1[6] * 710 >> 9 );

        // step 3
        curr_blk[i * 8] = temp2[0] + temp2[7];      //
        curr_blk[i * 8 + 7] = temp2[0] - temp2[7];
        curr_blk[i * 8 + 1] = temp2[1] + temp2[6];  //
        curr_blk[i * 8 + 6] = temp2[1] - temp2[6];
        curr_blk[i * 8 + 2] = temp2[2] + temp2[5];  //
        curr_blk[i * 8 + 5] = temp2[2] - temp2[5];
        curr_blk[i * 8 + 3] = temp2[3] + temp2[4];  //
        curr_blk[i * 8 + 4] = temp2[3] - temp2[4];
    }
    for ( i = 0; i < 8; i++ )
    {
        // step 1
        temp1[0] = ( curr_blk[0 * 8 + i] + curr_blk[4 * 8 + i] ) * 181 >> 7;
        temp1[1] = ( curr_blk[0 * 8 + i] - curr_blk[4 * 8 + i] ) * 181 >> 7;
        temp1[2] = ( curr_blk[2 * 8 + i] * 196 >> 8 ) - ( curr_blk[6 * 8 + i] * 473 >> 8 );
        temp1[3] = ( curr_blk[2 * 8 + i] * 473 >> 8 ) + ( curr_blk[6 * 8 + i] * 196 >> 8 );

        temp2[4] = curr_blk[1 * 8 + i] - curr_blk[7 * 8 + i];
        temp2[7] = curr_blk[1 * 8 + i] + curr_blk[7 * 8 + i];
        temp2[5] = curr_blk[3 * 8 + i] * 181 >> 7;
        temp2[6] = curr_blk[5 * 8 + i] * 181 >> 7;
        temp1[4] = temp2[4] + temp2[6];
        temp1[5] = temp2[7] - temp2[5];
        temp1[6] = temp2[4] - temp2[6];
        temp1[7] = temp2[7] + temp2[5];

        // step 2
        temp2[0] = temp1[0] + temp1[3];
        temp2[3] = temp1[0] - temp1[3];
        temp2[1] = temp1[1] + temp1[2];
        temp2[2] = temp1[1] - temp1[2];
        temp2[4] = ( temp1[4] * 301 >> 8 ) - ( temp1[7] * 201 >> 8 );
        temp2[7] = ( temp1[4] * 201 >> 8 ) + ( temp1[7] * 301 >> 8 );
        temp2[5] = ( temp1[5] * 710 >> 9 ) - ( temp1[6] * 141 >> 9 );
        temp2[6] = ( temp1[5] * 141 >> 9 ) + ( temp1[6] * 710 >> 9 );

        // step 3
        curr_blk[0 * 8 + i] = temp2[0] + temp2[7];
        curr_blk[7 * 8 + i] = temp2[0] - temp2[7];
        curr_blk[1 * 8 + i] = temp2[1] + temp2[6];
        curr_blk[6 * 8 + i] = temp2[1] - temp2[6];
        curr_blk[2 * 8 + i] = temp2[2] + temp2[5];
        curr_blk[5 * 8 + i] = temp2[2] - temp2[5];
        curr_blk[3 * 8 + i] = temp2[3] + temp2[4];
        curr_blk[4 * 8 + i] = temp2[3] - temp2[4];

        for ( j = 0; j<8; j++ )
        {
            if ( curr_blk[j * 8 + i]>0 )
            {
                curr_blk[j * 8 + i] = ( curr_blk[j * 8 + i] + 16 ) >> 5;
            }
            else
            {
                curr_blk[j * 8 + i] = -( ( -curr_blk[j * 8 + i] + 16 ) >> 5 );
            }
        }
    }
}

void partialButterflyInverse16x16( int* src, int* dst )
{
    int j, k;
    int E[8], O[8];
    int EE[4], EO[4];
    int EEE[2], EEO[2];

    for ( j = 0; j < 16; j++ )
    {
        /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
        for ( k = 0; k < 8; k++ )
        {
            O[k] = trans_core_16[1][k] * src[1 * 16 + j] + trans_core_16[3][k] * src[3 * 16 + j] + trans_core_16[5][k] * src[5 * 16 + j] + trans_core_16[7][k] * src[7 * 16 + j] +
                   trans_core_16[9][k] * src[9 * 16 + j] + trans_core_16[11][k] * src[11 * 16 + j] + trans_core_16[13][k] * src[13 * 16 + j] + trans_core_16[15][k] * src[15 * 16 + j];
        }
        for ( k = 0; k < 4; k++ )
        {
            EO[k] = trans_core_16[2][k] * src[2 * 16 + j] + trans_core_16[6][k] * src[6 * 16 + j] + trans_core_16[10][k] * src[10 * 16 + j] + trans_core_16[14][k] * src[14 * 16 + j];
        }
        EEO[0] = trans_core_16[4][0] * src[4 * 16 + j] + trans_core_16[12][0] * src[12 * 16 + j];
        EEE[0] = trans_core_16[0][0] * src[0 * 16 + j] + trans_core_16[8][0] * src[8 * 16 + j];
        EEO[1] = trans_core_16[4][1] * src[4 * 16 + j] + trans_core_16[12][1] * src[12 * 16 + j];
        EEE[1] = trans_core_16[0][1] * src[0 * 16 + j] + trans_core_16[8][1] * src[8 * 16 + j];

        /* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */
        for ( k = 0; k < 2; k++ )
        {
            EE[k] = EEE[k] + EEO[k];
            EE[k + 2] = EEE[1 - k] - EEO[1 - k];
        }
        for ( k = 0; k < 4; k++ )
        {
            E[k] = EE[k] + EO[k];
            E[k + 4] = EE[3 - k] - EO[3 - k];
        }
        for ( k = 0; k < 8; k++ )
        {
            dst[j * 16 + k] = E[k] + O[k];
            dst[j * 16 + k + 8] = E[7 - k] - O[7 - k];
        }
    }
}

void idct_4x4( int *curr_blk )
{
    partialButterflyInverse4x4( curr_blk );
}

void idct_8x8( int *curr_blk )
{
    partialButterflyInverse8x8( curr_blk );
}

void idct_16x16( int *curr_blk, int bsize )
{
    int i, j;
    int shift = 15;

    ALIGNED_16( int tmp_blk[MB_SIZE*MB_SIZE] );

    partialButterflyInverse16x16( curr_blk, tmp_blk );
    partialButterflyInverse16x16( tmp_blk, curr_blk );

    for ( i = 0; i < bsize; i++ )
    {
        for ( j = 0; j < bsize; j++ )
        {
            curr_blk[i * 16 + j] = sign( ( int )( ( abs( curr_blk[i * 16 + j] ) + ( 1 << ( shift - 1 ) ) ) >> shift ), curr_blk[i * 16 + j] );
            curr_blk[i * 16 + j] = Clip3( -255, 255, curr_blk[i * 16 + j] );
        }
    }
}

/*
*************************************************************************
* Function:Inverse transform 8x8 block.
* Input:
* Output:
* Return:
* Attention:input curr_blk has been changed to be [lines * pixels]
*************************************************************************
*/
void inv_transform( int* curr_blk, int bsize )
{
    if ( bsize == 4 )
    {
        g_funs_handle.idct_sqt[0]( curr_blk );
    }
    else if ( bsize == 8 )
    {
        g_funs_handle.idct_sqt[1]( curr_blk );
    }
    else
    {
        g_funs_handle.idct_sqt_16( curr_blk, bsize );
    }
}

void com_funs_init_dct()
{
    g_funs_handle.idct_sqt[0] = idct_4x4;
    g_funs_handle.idct_sqt[1] = idct_8x8;
    g_funs_handle.idct_sqt_16 = idct_16x16;
}