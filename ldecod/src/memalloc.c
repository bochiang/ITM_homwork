/*
*************************************************************************************
* File name: memalloc.c
* Function: Memory allocation and free helper funtions
*
*************************************************************************************
*/
#include "memalloc.h"
/*
*************************************************************************
* Function:Allocate 2D memory array -> uchar_t array2D[rows][columns]
* Input:
* Output:memory size in bytes
* Return:
* Attention:
*************************************************************************
*/
int get_mem2D( uchar_t ***array2D, int rows, int columns )
{
    int i;

    if( ( *array2D      = ( uchar_t** )calloc( rows,        sizeof( uchar_t* ) ) ) == NULL )
    {
        no_mem_exit( "get_mem2D: array2D" );
    }
    if( ( ( *array2D )[0] = ( uchar_t* )calloc( columns*rows,sizeof( uchar_t ) ) ) == NULL )
    {
        no_mem_exit( "get_mem2D: array2D" );
    }

    for( i=1; i<rows; i++ )
    {
        ( *array2D )[i] = ( *array2D )[i-1] + columns ;
    }

    return rows*columns;
}

/*
*************************************************************************
* Function:Allocate 2D memory array -> int array2D[rows][columns]
* Input:
* Output: memory size in bytes
* Return:
* Attention:
*************************************************************************
*/
int get_mem2Dint( int ***array2D, int rows, int columns )
{
    int i;

    if( ( *array2D      = ( int** )calloc( rows,        sizeof( int* ) ) ) == NULL )
    {
        no_mem_exit( "get_mem2Dint: array2D" );
    }
    if( ( ( *array2D )[0] = ( int* )calloc( rows*columns,sizeof( int ) ) ) == NULL )
    {
        no_mem_exit( "get_mem2Dint: array2D" );
    }

    for( i=1 ; i<rows ; i++ )
    {
        ( *array2D )[i] =  ( *array2D )[i-1] + columns  ;
    }

    return rows*columns*sizeof( int );
}

/*
*************************************************************************
* Function:Allocate 3D memory array -> uchar_t array3D[frames][rows][columns]
* Input:
* Output:memory size in bytes
* Return:
* Attention:
*************************************************************************
*/
int get_mem3D( uchar_t ****array3D, int frames, int rows, int columns )
{
    int  j;

    if( ( ( *array3D ) = ( uchar_t*** )calloc( frames,sizeof( uchar_t** ) ) ) == NULL )
    {
        no_mem_exit( "get_mem3D: array3D" );
    }

    for( j=0; j<frames; j++ )
    {
        get_mem2D( ( *array3D )+j, rows, columns ) ;
    }

    return frames*rows*columns;
}

/*
*************************************************************************
* Function:Allocate 3D memory array -> int array3D[frames][rows][columns]
* Input:
* Output:memory size in bytes
* Return:
* Attention:
*************************************************************************
*/
int get_mem3Dint( int ****array3D, int frames, int rows, int columns )
{
    int  j;

    if( ( ( *array3D ) = ( int*** )calloc( frames,sizeof( int** ) ) ) == NULL )
    {
        no_mem_exit( "get_mem3Dint: array3D" );
    }

    for( j=0; j<frames; j++ )
    {
        get_mem2Dint( ( *array3D )+j, rows, columns ) ;
    }

    return frames*rows*columns*sizeof( int );
}

/*
*************************************************************************
* Function:Allocate 4D memory array -> int array3D[frames][rows][columns][component]
* Input:
* Output:memory size in bytes
* Return:
* Attention:
*************************************************************************
*/
int get_mem4Dint( int *****array4D, int idx, int frames, int rows, int columns )
{
    int  j;

    if( ( ( *array4D ) = ( int**** )calloc( idx,sizeof( int** ) ) ) == NULL )
    {
        no_mem_exit( "get_mem4Dint: array4D" );
    }

    for( j=0; j<idx; j++ )
    {
        get_mem3Dint( ( *array4D )+j, frames, rows, columns ) ;
    }

    return idx*frames*rows*columns*sizeof( int );
}

/*
*************************************************************************
* Function:free 2D memory array
which was alocated with get_mem2D()
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void free_mem2D( uchar_t **array2D )
{
    if ( array2D )
    {
        if ( array2D[0] )
        {
            free ( array2D[0] );
        }
        else
        {
            error ( "free_mem2D: trying to free unused memory",100 );
        }

        free ( array2D );
    }
    else
    {
        error ( "free_mem2D: trying to free unused memory",100 );
    }
}

/*
*************************************************************************
* Function:free 2D memory array
which was alocated with get_mem2Dint()
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void free_mem2Dint( int **array2D )
{
    if ( array2D )
    {
        if ( array2D[0] )
        {
            free ( array2D[0] );
        }
        else
        {
            error ( "free_mem2D: trying to free unused memory",100 );
        }

        free ( array2D );

    }
    else
    {
        error ( "free_mem2D: trying to free unused memory",100 );
    }
}

/*
*************************************************************************
* Function:free 3D memory array
which was alocated with get_mem3D()
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void free_mem3D( uchar_t ***array3D, int frames )
{
    int i;

    if ( array3D )
    {
        for ( i=0; i<frames; i++ )
        {
            free_mem2D( array3D[i] );
        }
        free ( array3D );
    }
    else
    {
        error ( "free_mem3D: trying to free unused memory",100 );
    }
}

/*
*************************************************************************
* Function:free 3D memory array
which was alocated with get_mem3Dint()
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void free_mem3Dint( int ***array3D, int frames )
{
    int i;

    if ( array3D )
    {
        for ( i=0; i<frames; i++ )
        {
            free_mem2Dint( array3D[i] );
        }
        free ( array3D );
    }
    else
    {
        error ( "free_mem3D: trying to free unused memory",100 );
    }
}

/*
*************************************************************************
* Function:free 4D memory array
which was alocated with get_mem4Dint()
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void free_mem4Dint( int ****array4D, int idx, int frames )
{
    int  j;

    if ( array4D )
    {
        for( j=0; j<idx; j++ )
        {
            free_mem3Dint( array4D[j], frames ) ;
        }
        free ( array4D );
    }
    else
    {
        error ( "free_mem4D: trying to free unused memory",100 );
    }

}

/*
*************************************************************************
* Function:Exit program if memory allocation failed (using error())
* Input:  where
string indicating which memory allocation failed
* Output:
* Return:
* Attention:
*************************************************************************
*/
void no_mem_exit( char *where )
{
    fprintf(stderr, "Could not allocate memory: %s",where );
    error ( errortext, 100 );
}

