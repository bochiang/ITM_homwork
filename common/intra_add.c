#include "intra_add.h"

/**
* ===========================================================================
* local tables
* ===========================================================================
*/

/* ---------------------------------------------------------------------------
*/
static const int16_t tab_log2size[MAX_CU_SIZE + 1] = {
    -1, -1, -1, -1,  2, -1, -1, -1,
    3, -1, -1, -1, -1, -1, -1, -1,
    4
};

/* ---------------------------------------------------------------------------
*/
static const char tab_auc_dir_dx[NUM_INTRA_MODE] = {
    0, 0,  0,
    11, 2, 11, 1,  8, 1,  4, 1,  1,                    /* X  */
    0,
    1, 1,  4, 1,  8, 1, 11, 2, 11, 4, 8,              /* XY */
    0,
    8, 4, 11, 2, 11, 1,  8, 1                         /* Y  */
};

/* ---------------------------------------------------------------------------
*/
static const char tab_auc_dir_dy[NUM_INTRA_MODE] = {
    0,  0,  0,
    -4, -1, -8, -1, -11, -2, -11, -4, -8,              /* X  */
    0,
    8,  4, 11,  2,  11,  1,   8,  1,  4,  1,  1,      /* XY */
    0,
    -1, -1, -4, -1,  -8, -1, -11, -2                   /* Y  */
};

/* ---------------------------------------------------------------------------
*/
static const char tab_auc_dir_dxdy[2][NUM_INTRA_MODE][2] = {
    {
        // dx/dy
        { 0,0 },{ 0,0 },{ 0,0 },
        { 11,2 },{ 2,0 },{ 11,3 },{ 1,0 },{ 93,7 },{ 1,1 },{ 93,8 },{ 1,2 },{ 1,3 },                 /* X  */
        { 0,0 },
        { 1,3 },{ 1,2 },{ 93,8 },{ 1,1 },{ 93,7 },{ 1,0 },{ 11,3 },{ 2,0 },{ 11,2 },{ 4,0 },{ 8,0 },   /* XY */
        { 0,0 },
        { 8,0 },{ 4,0 },{ 11,2 },{ 2,0 },{ 11,3 },{ 1,0 },{ 93,7 },{ 1,1 },                         /* Y  */
    },{
        // dy/dx
        { 0,0 },{ 0,0 },{ 0,0 },
        { 93,8 },{ 1,1 },{ 93,7 },{ 1,0 },{ 11,3 },{ 2,0 },{ 11,2 },{ 4,0 },{ 8,0 },                 /* X  */
        { 0,0 },
        { 8,0 },{ 4,0 },{ 11,2 },{ 2,0 },{ 11,3 },{ 1,0 },{ 93,7 },{ 1,1 },{ 93,8 },{ 1,2 },{ 1,3 },   /* XY */
        { 0,0 },
        { 1,3 },{ 1,2 },{ 93,8 },{ 1,1 },{ 93,7 },{ 1,0 },{ 11,3 },{ 2,0 }                          /* Y  */
    }
};

/* ---------------------------------------------------------------------------
*/
static void intra_pred_plane_c(pel_t *src, pel_t *dst, int i_dst, int bsx, int bsy)
{
    /*                 size in bits:       2   3   4   5   6 */
    static const int ib_mult[8] = { 0, 0, 13, 17,  5, 11, 23, 0 };
    static const int ib_shift[8] = { 0, 0,  7, 10, 11, 15, 19, 0 };
    const int mult_h = ib_mult[tab_log2size[bsx]];
    const int mult_v = ib_mult[tab_log2size[bsy]];
    const int shift_h = ib_shift[tab_log2size[bsx]];
    const int shift_v = ib_shift[tab_log2size[bsy]];
    const int W2 = bsx >> 1;              /* half block width */
    const int H2 = bsy >> 1;              /* half block height */
    const int vmax = (1 << 8) - 1;  /* max value of pixel */
    int H = 0;
    int V = 0;
    int a, b, c;
    int x, y;
    pel_t *p_src;

    /* calculate H and V */
    p_src = src + W2;
    for (x = 1; x < W2 + 1; x++) {
        H += x * (p_src[x] - p_src[-x]);
    }
    p_src = src - H2;
    for (y = 1; y < H2 + 1; y++) {
        V += y * (p_src[-y] - p_src[y]);
    }

    a = (src[-bsy] + src[bsx]) << 4;
    b = ((H << 5) * mult_h + (1 << (shift_h - 1))) >> shift_h;
    c = ((V << 5) * mult_v + (1 << (shift_v - 1))) >> shift_v;
    a += 16 - b * (W2 - 1) - c * (H2 - 1);

    for (y = 0; y < bsy; y++) {
        int pix = a;
        for (x = 0; x < bsx; x++) {
            dst[x] = (pel_t)CAVS2_CLIP3(0, vmax, pix >> 5);
            pix += b;
        }
        dst += i_dst;
        a += c;
    }
}

/* ---------------------------------------------------------------------------
*/
static void intra_pred_plane_c(pel_t *src, pel_t *dst, int i_dst, int bsx, int bsy)
{
    /*                 size in bits:       2   3   4   5   6 */
    static const int ib_mult[8] = { 0, 0, 13, 17,  5, 11, 23, 0 };
    static const int ib_shift[8] = { 0, 0,  7, 10, 11, 15, 19, 0 };
    const int mult_h = ib_mult[tab_log2size[bsx]];
    const int mult_v = ib_mult[tab_log2size[bsy]];
    const int shift_h = ib_shift[tab_log2size[bsx]];
    const int shift_v = ib_shift[tab_log2size[bsy]];
    const int W2 = bsx >> 1;              /* half block width */
    const int H2 = bsy >> 1;              /* half block height */
    const int vmax = (1 << 8) - 1;  /* max value of pixel */
    int H = 0;
    int V = 0;
    int a, b, c;
    int x, y;
    pel_t *p_src;

    /* calculate H and V */
    p_src = src + W2;
    for (x = 1; x < W2 + 1; x++) {
        H += x * (p_src[x] - p_src[-x]);
    }
    p_src = src - H2;
    for (y = 1; y < H2 + 1; y++) {
        V += y * (p_src[-y] - p_src[y]);
    }

    a = (src[-bsy] + src[bsx]) << 4;
    b = ((H << 5) * mult_h + (1 << (shift_h - 1))) >> shift_h;
    c = ((V << 5) * mult_v + (1 << (shift_v - 1))) >> shift_v;
    a += 16 - b * (W2 - 1) - c * (H2 - 1);

    for (y = 0; y < bsy; y++) {
        int pix = a;
        for (x = 0; x < bsx; x++) {
            dst[x] = (pel_t)CAVS2_CLIP3(0, vmax, pix >> 5);
            pix += b;
        }
        dst += i_dst;
        a += c;
    }
}