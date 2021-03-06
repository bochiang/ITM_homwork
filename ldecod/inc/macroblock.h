#ifndef _MACROBLOCK_H_
#define _MACROBLOCK_H_

const uchar_t QP_SCALE_CR[64]=
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10,11,12,13,14,15,16,17,18,19,
    20,21,22,23,24,25,26,27,28,29,
    30,31,32,33,34,35,36,37,38,39,
    40,41,42,42,43,43,44,44,45,45,
    46,46,47,47,48,48,48,49,49,49,
    50,50,50,51,
};

const uchar_t NCBP[64][2]=
{
    {63, 0},{15, 15},{31, 63},{47, 31},{ 0, 16},{ 14,32},{13,47},{11,13},{7, 14},{5, 11},
    {10,12},{8,5},{12,10},{61,7},{4,48},{55,3},{1, 2},{2,8},{59,4},{3,1},
    {62, 61},{ 9, 55},{ 6,59},{29,62},{45,29},{51,27},{23,23},{39,19},{27,30},{46,28},
    {53,9},{30,6},{43,60},{37,21},{60,44},{16,26},{21,51},{ 28,35},{ 19,18},{ 35,20},
    { 42,24},{ 26,53},{ 44,17},{32,37},{58,39},{24,45},{20,58},{17,43},{18,42},{48,46},
    {22,36},{33,33},{25,34},{49,40},{40,52},{36,49},{34,50},{50,56},{52,25},{54,22},
    {41,54},{56,57},{38,41},{57,38},
};

void readCoeffBlks( ImgParams *img, Macroblock *currMB, uchar_t uhBitSize );

#endif

