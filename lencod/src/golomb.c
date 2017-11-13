/*
*************************************************************************************
* File name: golomb.c
* Function: Description
*
*************************************************************************************
*/


#include <assert.h>
#include "golomb.h"
#include "vlc.h"

/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void encode_golomb_word( i32u_t symbol,i32u_t grad0,i32u_t max_levels,i32u_t *res_bits,i32u_t *res_len )
{
    i32u_t level,res,numbits;

    res=1UL<<grad0;
    level=1UL;
    numbits=1UL+grad0;

    //find golomb level
    while( symbol>=res && level<max_levels )
    {
        symbol-=res;
        res=res<<1;
        level++;
        numbits+=2UL;
    }

    if( level>=max_levels )
    {
        if( symbol>=res )
        {
            symbol=res-1UL;    //crop if too large.
        }
    }

    //set data bits
    *res_bits=res|symbol;
    *res_len=numbits;
}

/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void encode_multilayer_golomb_word( i32u_t symbol,const i32u_t *grad,const i32u_t *max_levels,i32u_t *res_bits,i32u_t *res_len )
{
    unsigned accbits,acclen,bits,len,tmp;

    accbits=acclen=0UL;

    while( 1 )
    {
        encode_golomb_word( symbol,*grad,*max_levels,&bits,&len );
        accbits=( accbits<<len )|bits;
        acclen+=len;
        assert( acclen<=32UL ); //we'l be getting problems if this gets longer than 32 bits.
        tmp=*max_levels-1UL;

        if( !( ( len == ( tmp<<1 )+( *grad ) )&&( bits == ( 1UL<<( tmp+*grad ) )-1UL ) ) ) //is not last possible codeword? (Escape symbol?)
        {
            break;
        }

        tmp=*max_levels;
        symbol-=( ( ( 1UL<<tmp )-1UL )<<( *grad ) )-1UL;
        grad++;
        max_levels++;
    }
    *res_bits=accbits;
    *res_len=acclen;
}

/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

int writeSyntaxElement_GOLOMB( SyntaxElement *se, Bitstream *bitstream )
{
    i32u_t bits,len,i;
    i32u_t grad[4],max_lev[4];

    if( !( se->golomb_maxlevels&~0xFF ) )  //only bits 0-7 used? This means normal Golomb word.
    {
        encode_golomb_word( se->value1,se->golomb_grad,se->golomb_maxlevels,&bits,&len );
    }
    else
    {
        for( i=0UL; i<4UL; i++ )
        {
            grad[i]=( se->golomb_grad>>( i<<3 ) )&0xFFUL;
            max_lev[i]=( se->golomb_maxlevels>>( i<<3 ) )&0xFFUL;
        }
        encode_multilayer_golomb_word( se->value1,grad,max_lev,&bits,&len );
    }

    se->len=len;
    se->bitpattern=bits;

    writeUVLC2buffer( se, bitstream );
    return ( se->len );
}
