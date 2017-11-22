#ifndef TZ_SEARCH_H
#define TZ_SEARCH_H

#define USING_TZ_SEARCH     1

#include "global.h"

int Get_mvc(ImgParams *img, Macroblock *currMB, int(*pmv)[2], int **refFrArr, int ***tmp_mv,
    int ref_frame,
    int pix_x_in_mb,
    int pix_y_in_mb,
    int bsize_x,
    int mode,
    int ref);



#endif // !TZ_SEARCH_H

