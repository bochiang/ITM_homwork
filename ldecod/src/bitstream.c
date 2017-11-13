/*
*************************************************************************************
* File name: bitstream.c
* Function: decode bitstream
*
*************************************************************************************
*/

#include <string.h>
#include <assert.h>

#include "global.h"
#include "annexb.h"
#include "memalloc.h"
#include "biaridecod.h"

#define SVA_STREAM_BUF_SIZE 1024

extern StatBits *StatBitsPtr;   //ITM_r2
FILE *bitsfile = NULL;      //!< the bit stream file
uchar_t bit[8] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};

typedef struct
{
    FILE *f;
    uchar_t   buf[SVA_STREAM_BUF_SIZE];
    i32u_t    uClearBits;            // clear bits that has been read out (clear bit means without the emulation of start code)
    i32u_t    uPre3Bytes;
    int   iBytePosition;             // position of pointer in InputStream buffer
    int   iBufBytesNum;              // number of bytes in InputStream buffer
    int   iClearBitsNum;             // clear bit number that has been read out
    int   iStuffBitsNum;
    int iBitsCount;
} InputStream;

InputStream IRABS;
InputStream *pIRABS = &IRABS;

void OpenIRABS( InputStream *p, char *fname )
{
    p->f = fopen( fname,"rb" );
    if( p->f==NULL )
    {
      sprintf(errortext, "\n>>>>>>>>>> Can't open file %s <<<<<<<<<<\n", fname);
      error(errortext, -1);
    }

    p->uClearBits         = 0xffffffff;
    p->iBytePosition      = 0;
    p->iBufBytesNum       = 0;
    p->iClearBitsNum      = 0;
    p->iStuffBitsNum      = 0;
    p->iBitsCount         = 0;
    p->uPre3Bytes         = 0;
}

void CloseIRABS( InputStream *p )
{
    fclose( p->f );
}

/*
*************************************************************************
* Function: Check start code's type
* Input:
* Output:
* Return:
* Attention: If the start code is video_sequence_start_code,user_data_start_code
or extension_start_code, the demulation mechanism is forbidded.
* Author:
*************************************************************************
*/
void CheckType( int startcode )
{
    startcode = startcode&0x000000ff;
    switch( startcode )
    {
        case 0xb0:
        case 0xb2:
        case 0xb5:
            demulate_enable = 0;
            break;
        default:
            demulate_enable = 1;
            break;
    }
}

// move iBytePosition to the next byte of start code prefix
// The bit stream has been read to "p->buf" in this function
//return
//    0 : OK
//   -1 : arrive at stream end and start code is not found
//   -2 : p->iBytePosition error
//-------------------------------------------------------------------------
int find_start_code( InputStream *p )
{
    int i, m = 0;
    uchar_t a = 0, b = 0;  // a b 0 1 2 3 4 ... M-3 M-2 M-1

    while( 1 )
    {
        if( p->iBytePosition >= p->iBufBytesNum - 2 ) //if all bytes in buffer has been searched
        {
            m = p->iBufBytesNum - p->iBytePosition;
            if (m < 0)
            {
                error("\np->iBytePosition error!", -2);
                return -2;
            }
            else if (m == 1)
            {
                b = p->buf[p->iBytePosition + 1];
            }
            else if (m == 2)
            {
                b = p->buf[p->iBytePosition + 1];
                a = p->buf[p->iBytePosition];
            }
            p->iBufBytesNum = (int)fread( p->buf,1,SVA_STREAM_BUF_SIZE,p->f );
            p->iBytePosition = 0;
        }

        if( p->iBufBytesNum + m < 3 )
        {
            error("\narrive at stream end and start code is not found!", -1);
            return -1;
        }

        if( m==1 && b==0 && p->buf[0]==0 && p->buf[1]==1 )
        {
            p->iBytePosition  = 2;
            p->iClearBitsNum  = 0;
            p->iStuffBitsNum  = 0;
            p->iBitsCount     += 24;
            p->uPre3Bytes     = 1;
            return 0;
        }

        if( m==2 && b==0 && a==0 && p->buf[0]==1 )
        {
            p->iBytePosition  = 1;
            p->iClearBitsNum  = 0;
            p->iStuffBitsNum  = 0;
            p->iBitsCount     += 24;
            p->uPre3Bytes     = 1;
            return 0;
        }

        if( m==2 && b==0 && p->buf[0]==0 && p->buf[1]==1 )
        {
            p->iBytePosition  = 2;
            p->iClearBitsNum  = 0;
            p->iStuffBitsNum  = 0;
            p->iBitsCount     += 24;
            p->uPre3Bytes     = 1;
            return 0;
        }

        for( i = p->iBytePosition; i < p->iBufBytesNum - 2; i++ ) // find the next start code
        {
            if( p->buf[i]==0 && p->buf[i+1]==0 && p->buf[i+2]==1 )
            {
                p->iBytePosition    = i+3;
                p->iClearBitsNum    = 0;
                p->iStuffBitsNum    = 0;
                p->iBitsCount       += 24;
                p->uPre3Bytes       = 1;
                return 0;
            }
            p->iBitsCount += 8;
        }
        p->iBytePosition = i;
    }
}

/*
*************************************************************************
* Function: read clear bits from InputStream buffer, emulation removal may be invoked.
*           Clear bit means without the emulation of start code.
* Input:
* Output:
* Return:  0 : OK
-1 : arrive at stream end
-2 : meet another start code
* Attention:
*************************************************************************
*/
int read_clear_bits( InputStream *p )
{
    int i,k,j;
    uchar_t temp[3];
    i = p->iBytePosition;
    k = p->iBufBytesNum - i;
    if( k < 3 )
    {
        for( j=0; j<k; j++ )
        {
            temp[j] = p->buf[i+j];
        }
        p->iBufBytesNum = (int)fread( p->buf+k,1,SVA_STREAM_BUF_SIZE-k,p->f );
        if( p->iBufBytesNum == 0 )
        {
            if( k>0 )
            {
                p->uPre3Bytes = ( ( p->uPre3Bytes<<8 ) | p->buf[i] ) & 0x00ffffff;
                if( p->uPre3Bytes < 4 && demulate_enable )
                {
                    p->uClearBits = ( p->uClearBits << 6 ) | ( p->buf[i] >> 2 );
                    p->iClearBitsNum += 6;
                    StatBitsPtr->emulate_bits += 2;
                }
                else
                {
                    p->uClearBits = ( p->uClearBits << 8 ) | p->buf[i];
                    p->iClearBitsNum += 8;
                }
                p->iBytePosition++;
                return 0;
            }
            else
            {
                return -1;//arrive at stream end
            }
        }
        else
        {
            for( j=0; j<k; j++ )
            {
                p->buf[j] = temp[j];
            }
            p->iBufBytesNum += k;
            i = p->iBytePosition = 0;
        }
    }

    if( p->buf[i]==0 && p->buf[i+1]==0 && p->buf[i+2]==1 ) // 0x001
    {
        return -2;    // Another start code appears, which means the end of the current syntax segment and the beginning of the next syntax segment
    }

    p->uPre3Bytes = ( ( p->uPre3Bytes<<8 ) | p->buf[i] ) & 0x00ffffff;
    if( p->uPre3Bytes < 4 && demulate_enable ) // it means the next 3 bytes are 0x002 or 0x003
    {
        p->uClearBits = ( p->uClearBits << 6 ) | ( p->buf[i] >> 2 );
        p->iClearBitsNum += 6;
        StatBitsPtr->emulate_bits += 2;
    }
    else
    {
        p->uClearBits = ( p->uClearBits << 8 ) | p->buf[i];
        p->iClearBitsNum += 8;
    }
    p->iBytePosition++;
    return 0;
}


/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:  0 : OK
-1 : arrive at stream end
-2 : meet another start code
* Attention:
*************************************************************************
*/
int read_n_bit( InputStream *p, int n, int *v )
{
    int r;
    i32u_t t;
    while( n > p->iClearBitsNum )
    {
        r = read_clear_bits( p );
        if( r )
        {
            if( r==-1 )
            {
                if( p->iBufBytesNum - p->iBytePosition > 0 )
                {
                    break;
                }
            }
            return r;
        }
    }
    t = p->uClearBits;
    r = 32 - p->iClearBitsNum;
    *v = ( t << r ) >> ( 32 - n );
    p->iClearBitsNum -= n;
    return 0;
}
//==================================================================================

void OpenBitstreamFile ( char *fn )
{
    OpenIRABS( pIRABS, fn );
}

void CloseBitstreamFile()
{
    CloseIRABS( pIRABS );
}


/*
*************************************************************************
* Function:For supporting multiple sequences in a stream
* Input:
* Output:
* Return:
* Attention:
* Author:
*************************************************************************
*/
int IsEndOfBitstream ()
{
    int ret, seof, m;
    ret=feof( pIRABS->f );
    m = pIRABS->iBufBytesNum - pIRABS->iBytePosition;
    seof = ( ( ret ) && ( !pIRABS->iBufBytesNum ) ) || ( ( ret ) && ( m==1 ) );
    return ( ( !seof )?( 0 ):( 1 ) );
}

static int FindStartCode ( uchar_t *Buf, int i )
{
    if( Buf[i]==0 && Buf[i+1]==0 && Buf[i+2]==1 )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}


/*
*************************************************************************
* Function: check the stuffing pattern in the end of slices
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
int check_slice_stuffing()
{
    int temp_i, temp_val;

    //multiple slice
    Env_AEC * dep_dp;

    dep_dp = &img->cs_aec->de_AEC;
    currStream->byte_offset = arideco_bits_read( dep_dp );
    //multiple slice

    if ( currStream->bs_length*8 - currStream->byte_offset == 0 )
    {
        return 1;
    }

    if( img->current_mb_nr == 0 )
    {
#if M38817_DATA_STUFFING
        if (  currStream->bs_length*8 - currStream->byte_offset <=9
                && currStream->bs_length*8 - currStream->byte_offset > 0  )
        {
            temp_i = currStream->bs_length*8 - currStream->byte_offset;
            assert( temp_i >= 2 );
            temp_val = get_uv( temp_i, "filling data" ) ;
        }
#else
        if (  currStream->bs_length*8 - currStream->byte_offset <=8
                && currStream->bs_length*8 - currStream->byte_offset > 0  )
        {
            temp_i = currStream->bs_length*8 - currStream->byte_offset;
            assert( temp_i > 0 );
            temp_val = get_uv( temp_i, "filling data" ) ;
        }
#endif
    }

    if( img->current_mb_nr == 0 )
    {
        if( next_start_code_pos>4 && next_start_code_pos<0x000fffff )
        {
            return 1;
        }
        else
        {
            currStream->byte_offset = currentbitoffset;
            return 0;
        }
    }

    if( img->current_mb_nr != 0 )
    {
#if M38817_DATA_STUFFING // For multiple slices, this branch will be invoked.
        if ( currStream->bs_length*8 - currStream->byte_offset <= 9
                && currStream->bs_length*8 - currStream->byte_offset >0 )
        {
            temp_i = currStream->bs_length*8 - currStream->byte_offset;
            
            temp_val = get_uv( temp_i, "stuffing pattern" ) ;
            assert( temp_i >= 2 );
            assert(temp_i <= 9);
            if ( temp_val == ( 3 << ( temp_i -2 ) ) )
            {
                return 1;    // last MB in current slice
            }
            else
            {
                assert(0);
            }
        }
#else
        if ( currStream->bs_length*8 - currStream->byte_offset <= 8
                && currStream->bs_length*8 - currStream->byte_offset >0 )
        {
            temp_i = currStream->bs_length*8 - currStream->byte_offset;
            assert( temp_i > 0 );
            temp_val = get_uv( temp_i, "stuffing pattern" ) ; //modified at ITM
            if ( temp_val == ( 1 << ( temp_i -1 ) ) )
            {
                return 1;    // last MB in current slice
            }
        }
#endif
        return 0;       // not last MB in current slice
        //---end
    }

    return 1;
}

/*
*************************************************************************
* Function: get one syntax unit, which begins from a start code and ends with another
* \param buf
*    returned buffer with all data in the current syntax unit(segment)
* \param startcodepos
*    returned the position start code suffix in "buf"
* \param length
*    the length of "buf"
* \return
*    the length of "buf", also the position of next start code
*************************************************************************
*/
int GetOneUnit(uchar_t *buf, int *startcodepos, int *length)
{
    int i, j, k;
    assert(!find_start_code(pIRABS)); // find the start code prefix, i.e. 0x000001

    buf[0] = 0;
    buf[1] = 0;
    buf[2] = 1;
    *startcodepos = 3; // this is the position of the start code suffix
    i = read_n_bit( pIRABS,8,&j );
    buf[3] = ( char )j;
    CheckType( buf[3] );
    if( buf[3]==SEQUENCE_END_CODE )
    {
        *length = 4;
        return -1;
    }
    k = 4;
    while( 1 )
    {
        i = read_n_bit( pIRABS,8,&j );
        if( i<0 ) // another start code appear
        {
            break;
        }
        buf[k++] = ( char )j;
    }

    if( pIRABS->iClearBitsNum>0 )
    {
        int shift;
        shift = 8 - pIRABS->iClearBitsNum;
        i = read_n_bit( pIRABS,pIRABS->iClearBitsNum,&j );

        if( j!=0 )
        {
            buf[k++] = ( char )( j<<shift );
        }
        StatBitsPtr->last_unit_bits += shift;
    }
    *length = k;
    return k;
}
