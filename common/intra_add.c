#include "intra_add.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define Clip(x)            (min(255, max(0, (x))))                                                          ///< clip with bit-depth range


/////////////////////////////////////////////////////////////////////////////
/// variables definition
/////////////////////////////////////////////////////////////////////////////
const unsigned char g_aucXYflg[NUM_INTRA_PMODE] = {
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    1, 1, 1, 1, 1,
    1, 1, 1
};

const char g_aucDirDx[NUM_INTRA_PMODE] = {
    0, 0, 0, 11, 2,
    11, 1, 8, 1, 4,
    1, 1, 0, 1, 1,
    4, 1, 8, 1, 11,
    2, 11, 4, 8, 0,
    8, 4, 11, 2, 11,
    1, 8, 1
};

const char g_aucDirDy[NUM_INTRA_PMODE] = {
    0, 0, 0, -4, -1,
    -8, -1, -11, -2, -11,
    -4, -8, 0, 8, 4,
    11, 2, 11, 1, 8,
    1, 4, 1, 1, 0,
    -1, -1, -4, -1, -8,
    -1, -11, -2
};

const char g_aucSign[NUM_INTRA_PMODE] = {
    0, 0, 0, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, 0, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 0,
    -1, -1, -1, -1, -1,
    -1, -1, -1
};
const char g_aucDirDxDy[2][NUM_INTRA_PMODE][2] = {
    {
        // dx/dy
        { 0, 0 },{ 0, 0 },{ 0, 0 },{ 11, 2 },{ 2, 0 },
        { 11, 3 },{ 1, 0 },{ 93, 7 },{ 1, 1 },{ 93, 8 },
        { 1, 2 },{ 1, 3 },{ 0, 0 },{ 1, 3 },{ 1, 2 },
        { 93, 8 },{ 1, 1 },{ 93, 7 },{ 1, 0 },{ 11, 3 },
        { 2, 0 },{ 11, 2 },{ 4, 0 },{ 8, 0 },{ 0, 0 },
        { 8, 0 },{ 4, 0 },{ 11, 2 },{ 2, 0 },{ 11, 3 },
        { 1, 0 },{ 93, 7 },{ 1, 1 },
    },
    {
        // dy/dx
        { 0, 0 },{ 0, 0 },{ 0, 0 },{ 93, 8 },{ 1, 1 },
        { 93, 7 },{ 1, 0 },{ 11, 3 },{ 2, 0 },{ 11, 2 },
        { 4, 0 },{ 8, 0 },{ 0, 0 },{ 8, 0 },{ 4, 0 },
        { 11, 2 },{ 2, 0 },{ 11, 3 },{ 1, 0 },{ 93, 7 },
        { 1, 1 },{ 93, 8 },{ 1, 2 },{ 1, 3 },{ 0, 0 },
        { 1, 3 },{ 1, 2 },{ 93, 8 },{ 1, 1 },{ 93, 7 },
        { 1, 0 },{ 11, 3 },{ 2, 0 }
    }
};

void intra_init()
{
    memset(g_log2size, -1, MAX_CU_SIZE + 1);
    int c = 2;
    for (int k = 4; k <= MAX_CU_SIZE; k *= 2) {
        g_log2size[k] = c;
        c++;
    }
}

/////////////////////////////////////////////////////////////////////////////
/// function definition
/////////////////////////////////////////////////////////////////////////////

void xPredIntraPlaneAdi(uchar_t *pSrc, pel_t *pDst, int i_dst, int iWidth, int iHeight, int sample_bit_depth)
{
    int iH = 0;
    int iV = 0;
    int iA, iB, iC;
    int x, y;
    int iW2 = iWidth >> 1;
    int iH2 = iHeight >> 1;
    int ib_mult[5] = { 13, 17, 5, 11, 23 };
    int ib_shift[5] = { 7, 10, 11, 15, 19 };

    int im_h = ib_mult[g_log2size[iWidth] - 2];
    int is_h = ib_shift[g_log2size[iWidth] - 2];
    int im_v = ib_mult[g_log2size[iHeight] - 2];
    int is_v = ib_shift[g_log2size[iHeight] - 2];
    int iTmp, iTmp2;

    uchar_t  *rpSrc = pSrc;

    rpSrc = pSrc + 1;
    rpSrc += (iW2 - 1);
    for (x = 1; x < iW2 + 1; x++) {
        iH += x * (rpSrc[x] - rpSrc[-x]);
    }

    rpSrc = pSrc - 1;
    rpSrc -= (iH2 - 1);

    for (y = 1; y < iH2 + 1; y++) {
        iV += y * (rpSrc[-y] - rpSrc[y]);
    }

    rpSrc = pSrc;
    iA = (rpSrc[-1 - (iHeight - 1)] + rpSrc[1 + iWidth - 1]) << 4;
    iB = ((iH << 5) * im_h + (1 << (is_h - 1))) >> is_h;
    iC = ((iV << 5) * im_v + (1 << (is_v - 1))) >> is_v;

    iTmp = iA - (iH2 - 1) * iC - (iW2 - 1) * iB + 16;
    for (y = 0; y < iHeight; y++) {
        iTmp2 = iTmp;
        for (x = 0; x < iWidth; x++) {
            //img->mprr[PLANE_PRED][y][x] = Clip( iTmp2 >> 5 );
            pDst[y * i_dst + x] = Clip3(0, (1 << sample_bit_depth) - 1, iTmp2 >> 5);
            iTmp2 += iB;
        }
        iTmp += iC;
    }
}

void xPredIntraBiAdi(uchar_t *pSrc, pel_t *pDst, int i_dst, int iWidth, int iHeight, int sample_bit_depth)
{
    int x, y;
    int ishift_x = g_log2size[iWidth];
    int ishift_y = g_log2size[iHeight];
    int ishift = min(ishift_x, ishift_y);
    int ishift_xy = ishift_x + ishift_y + 1;
    int offset = 1 << (ishift_x + ishift_y);
    int a, b, c, w, wxy, tmp;
    int predx;
    int pTop[MAX_CU_SIZE], pLeft[MAX_CU_SIZE], pT[MAX_CU_SIZE], pL[MAX_CU_SIZE], wy[MAX_CU_SIZE];

    for (x = 0; x < iWidth; x++) {
        pTop[x] = pSrc[1 + x];
    }
    for (y = 0; y < iHeight; y++) {
        pLeft[y] = pSrc[-1 - y];
    }

    a = pTop[iWidth - 1];
    b = pLeft[iHeight - 1];
    c = (iWidth == iHeight) ? (a + b + 1) >> 1 :
        (((a << ishift_x) + (b << ishift_y)) * 13 + (1 << (ishift + 5))) >> (ishift + 6);
    w = (c << 1) - a - b;


    for (x = 0; x < iWidth; x++) {
        pT[x] = b - pTop[x];
        pTop[x] <<= ishift_y;
    }
    tmp = 0;
    for (y = 0; y < iHeight; y++) {
        pL[y] = a - pLeft[y];
        pLeft[y] <<= ishift_x;
        wy[y] = tmp;
        tmp += w;
    }


    for (y = 0; y < iHeight; y++) {
        predx = pLeft[y];
        wxy = 0;
        for (x = 0; x < iWidth; x++) {
            predx += pL[y];

            pTop[x] += pT[x];
            pDst[y * i_dst + x] = Clip3(0, (1 << sample_bit_depth) - 1,
                (((predx << ishift_y) + (pTop[x] << ishift_x) + wxy + offset) >> ishift_xy));
            wxy += wy[y];
        }
    }

}

/* ---------------------------------------------------------------------------
*/
void intra_pred_ang_30_c(pel_t *src, pel_t *dst, int i_dst, int bsx, int bsy)
{
    pel_t first_line[64 + 64];
    int line_size = bsx + bsy - 1;
    int i;

    src -= 2;
    for (i = 0; i < line_size; i++, src--) {
        first_line[i] = (pel_t)((src[-1] + (src[0] << 1) + src[1] + 2) >> 2);
    }

    for (i = 0; i < bsy; i++) {
        memcpy(dst, first_line + i, bsx * sizeof(pel_t));
        dst += i_dst;
    }
}

void intra_pred_ang_20_c(pel_t *src, pel_t *dst, int i_dst, int bsx, int bsy)
{
    pel_t first_line[64 + 128];
    int left_size = ((bsy - 1) << 1) + 1;
    int top_size = bsx - 1;
    int line_size = left_size + top_size;
    int i;
    pel_t *pfirst = first_line + left_size - 1;

    src -= bsy;
    for (i = 0; i < left_size; i += 2, src++) {
        first_line[i] = (pel_t)((src[-1] + (src[0] + src[1]) * 3 + src[2] + 4) >> 3);
        first_line[i + 1] = (pel_t)((src[0] + (src[1] << 1) + src[2] + 2) >> 2);
    }
    i--;

    for (; i < line_size; i++, src++) {
        first_line[i] = (pel_t)((src[-1] + (src[0] << 1) + src[1] + 2) >> 2);
    }

    for (i = 0; i < bsy; i++) {
        memcpy(dst, pfirst, bsx * sizeof(pel_t));
        pfirst -= 2;
        dst += i_dst;
    }
}