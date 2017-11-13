#ifndef _REFBUF_H_
#define _REFBUF_H_

#include "global.h"

pel_t UMVPelY_14 ( pel_t *Pic, int y, int x );
pel_t FastPelY_14 ( pel_t *Pic, int y, int x );

void PutPel_14 ( pel_t *Pic, int y, int x, pel_t val );
void PutPel_11 ( pel_t *Pic, int y, int x, pel_t val );

#endif