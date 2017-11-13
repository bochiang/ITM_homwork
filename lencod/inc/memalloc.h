#ifndef _MEMALLOC_H_
#define _MEMALLOC_H_

#include "global.h"

int  get_mem2D( pel_t ***array2D, int rows, int columns );
int  get_mem2Dint( int ***array2D, int rows, int columns );
int  get_mem3D( pel_t ****array2D, int frames, int rows, int columns );
int  get_mem3Dint( int ****array3D, int frames, int rows, int columns );
int  get_mem4Dint( int *****array4D, int idx, int frames, int rows, int columns );

void free_mem2D( pel_t **array2D );
void free_mem2Dint( int **array2D );
void free_mem3D( pel_t ***array2D, int frames );
void free_mem3Dint( int ***array3D, int frames );
void free_mem4Dint( int ****array4D, int idx, int frames );

void no_mem_exit( char *where );

#endif
