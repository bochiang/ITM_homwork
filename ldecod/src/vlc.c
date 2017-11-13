/*
*************************************************************************************
* File name: vlc.c
* Function: VLC support functions
*
*************************************************************************************
*/
#include "contributors.h"

#include <math.h>
#include <memory.h>
#include <string.h>
#include <assert.h>

#include "global.h"
#include "vlc.h"
#include "elements.h"
#include "header.h"

// A little trick to avoid those horrible #if TRACE all over the source code
#if TRACE
#define SYMTRACESTRING(s) strncpy(sym->tracestring,s,TRACESTRING_SIZE)
#else
#define SYMTRACESTRING(s) // do nothing
#endif

extern BbvBuffer_t *pBbv;

/*
*************************************************************************
* Function:ue_v, reads an ue(v) syntax element
* Input:
tracestring
the string for the trace file
bitstream
the stream to be read from
* Output:
* Return: the value of the coded syntax element
* Attention:
*************************************************************************
*/

int ue_v ( char *tracestring )
{
    SyntaxElement symbol, *sym=&symbol;

    assert ( currStream->streamBuffer != NULL );
    sym->type = SE_HEADER;
    sym->mapping = linfo_ue;   // Mapping rule
    SYMTRACESTRING( tracestring );
    readSyntaxElement_VLC ( sym );

    return sym->value1;
}

/*
*************************************************************************
* Function:ue_v, reads an u(v) syntax element
* Input:
tracestring
the string for the trace file
bitstream
the stream to be read from
* Output:
* Return: the value of the coded syntax element
* Attention:
*************************************************************************
*/

int u_v ( int LenInBits, char*tracestring )
{
    SyntaxElement symbol, *sym=&symbol;

    assert ( currStream->streamBuffer != NULL );
    sym->type = SE_HEADER;
    sym->mapping = linfo_ue;   // Mapping rule
    sym->len = LenInBits;
    readSyntaxElement_FLC ( sym );
    return sym->inf;
}
int i_8( char *tracestring )
{
    int frame_bitoffset = currStream->byte_offset;
    uchar_t *buf = currStream->streamBuffer;
    int BitstreamLengthInBytes = currStream->bs_length;
    SyntaxElement symbol, *sym=&symbol;
    assert ( currStream->streamBuffer != NULL );

    sym->len = 8;
    sym->type = SE_HEADER;
    sym->mapping = linfo_ue;

    if ( ( GetBits( buf, frame_bitoffset, &( sym->inf ), BitstreamLengthInBytes, sym->len ) ) < 0 )
    {
        return -1;
    }
    currStream->byte_offset += sym->len; // move bitstream pointer
    sym->value1 = sym->inf;
    if ( sym->inf & 0x80 )
    {
        sym->inf= -( ~( ( int )0xffffff00 | sym->inf ) + 1 );
    }
    return sym->inf;
}
/*
*************************************************************************
* Function:ue_v, reads an u(1) syntax element
* Input:
tracestring
the string for the trace file
bitstream
the stream to be read from
* Output:
* Return: the value of the coded syntax element
* Attention:
*************************************************************************
*/

int u_1 ( char *tracestring )
{
    return u_v ( 1, tracestring );
}

/*
*************************************************************************
* Function:mapping rule for ue(v) syntax elements
* Input:lenght and info
* Output:number in the code table
* Return:
* Attention:
*************************************************************************
*/

void linfo_ue( int len, int info, int *value1 )
{
    *value1 = ( int )pow( 2,( len/2 ) )+info-1; // *value1 = (int)(2<<(len>>1))+info-1;
}

/*
*************************************************************************
* Function:mapping rule for se(v) syntax elements
* Input:lenght and info
* Output:signed mvd
* Return:
* Attention:
*************************************************************************
*/

void linfo_se( int len,  int info, int *value1 )
{
    int n;
    n = ( int )pow( 2,( len/2 ) )+info-1;
    *value1 = ( n+1 )/2;
    if( ( n & 0x01 )==0 )                       // lsb is signed bit
    {
        *value1 = -*value1;
    }

}

/*
*************************************************************************
* Function:lenght and info
* Input:
* Output:cbp (intra)
* Return:
* Attention:
*************************************************************************
*/


void linfo_cbp_intra( int len,int info,int *cbp )
{
    extern const uchar_t NCBP[64][2];
    int cbp_idx;
    linfo_ue( len,info,&cbp_idx );
    *cbp=NCBP[cbp_idx][0];
}

/*
*************************************************************************
* Function:
* Input:lenght and info
* Output:cbp (inter)
* Return:
* Attention:
*************************************************************************
*/

void linfo_cbp_inter( int len,int info,int *cbp )
{
    extern const uchar_t NCBP[64][2];
    int cbp_idx;
    linfo_ue( len,info,&cbp_idx );
    *cbp=NCBP[cbp_idx][1];
}

/*
*************************************************************************
* Function:read next UVLC codeword from UVLC-partition and
map it to the corresponding syntax element
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

int readSyntaxElement_VLC( SyntaxElement *sym )
{
    int frame_bitoffset = currStream->byte_offset;
    uchar_t *buf = currStream->streamBuffer;
    int BitstreamLengthInBytes = currStream->bs_length;

    sym->len =  GetVLCSymbol ( buf, frame_bitoffset, &( sym->inf ), BitstreamLengthInBytes );

    if ( sym->len == -1 )
    {
        return -1;
    }

    currStream->byte_offset += sym->len;
    sym->mapping( sym->len,sym->inf,&( sym->value1 ) );
    return 1;
}


/*
*************************************************************************
* Function: Moves the read pointer of the partition forward by one symbol
* Input:
byte buffer[]
containing VLC-coded data bits
int totbitoffset
bit offset from start of partition
int type
expected data type (Partiotion ID)
* Output:
* Return: Length and Value of the next symbol
* Attention:As in both nal_bits.c and nal_part.c all data of one partition, slice,
picture was already read into a buffer, there is no need to read any data
here again.
\par
This function could (and should) be optimized considerably
\par
If it is ever decided to have different VLC tables for different symbol
types, then this would be the place for the implementation
\par
An alternate VLC table is implemented based on exponential Golomb codes.
The encoder must have a matching define selected.
\par
GetVLCInfo was extracted because there should be only one place in the
source code that has knowledge about symbol extraction, regardless of
the number of different NALs.
*************************************************************************
*/

int GetVLCSymbol ( uchar_t buffer[],int totbitoffset,int *info, int bytecount )
{

    register int inf;
    long byteoffset;      // byte from start of buffer
    int bitoffset;      // bit from start of byte
    int ctr_bit=0;      // control bit for current bit posision
    int bitcounter=1;
    int len;
    int info_bit;

    byteoffset= totbitoffset/8;
    bitoffset= 7-( totbitoffset%8 );
    ctr_bit = ( buffer[byteoffset] & ( 0x01<<bitoffset ) ); // set up control bit

    len=1;
    while ( ctr_bit==0 )
    {
        // find leading 1 bit
        len++;
        bitoffset-=1;
        bitcounter++;
        if ( bitoffset<0 )
        {
            // finish with current byte ?
            bitoffset=bitoffset+8;
            byteoffset++;
        }
        ctr_bit=buffer[byteoffset] & ( 0x01<<( bitoffset ) );
    }

    // make infoword
    inf=0;                          // shortest possible code is 1, then info is always 0
    for( info_bit=0; ( info_bit<( len-1 ) ); info_bit++ )
    {
        bitcounter++;
        bitoffset-=1;
        if ( bitoffset<0 )
        {
            // finished with current byte ?
            bitoffset=bitoffset+8;
            byteoffset++;
        }
        if ( byteoffset > bytecount )
        {
            return -1;
        }
        inf=( inf<<1 );
        if( buffer[byteoffset] & ( 0x01<<( bitoffset ) ) )
        {
            inf |=1;
        }
    }

    *info = inf;

    if ( input->check_BBV_flag && pBbv )
    {
        pBbv->frame_code_bits += bitcounter;
    }


    return bitcounter;           // return absolute offset in bit from start of frame
}

/*
*************************************************************************
* Function:read FLC codeword from UVLC-partition
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

int readSyntaxElement_FLC( SyntaxElement *sym )
{
    int frame_bitoffset = currStream->byte_offset;
    uchar_t *buf = currStream->streamBuffer;
    int BitstreamLengthInBytes = currStream->bs_length;

    if ( ( GetBits( buf, frame_bitoffset, &( sym->inf ), BitstreamLengthInBytes, sym->len ) ) < 0 )
    {
        return -1;
    }

    currStream->byte_offset += sym->len; // move bitstream pointer
    sym->value1 = sym->inf;
    return 1;
}

/*
*************************************************************************
* Function:Reads bits from the bitstream buffer
* Input:
byte buffer[]
containing VLC-coded data bits
int totbitoffset
bit offset from start of partition
int bytecount
total bytes in bitstream
int numbits
number of bits to read
* Output:
* Return:
* Attention:
*************************************************************************
*/

int GetBits ( uchar_t buffer[],int totbitoffset,int *info, int bytecount,
              int numbits )
{
    register int inf;
    long byteoffset;      // byte from start of buffer
    int bitoffset;      // bit from start of byte

    int bitcounter=numbits;

    byteoffset= totbitoffset/8;
    bitoffset= 7-( totbitoffset%8 );

    inf=0;
    while ( numbits )
    {
        inf <<=1;
        inf |= ( buffer[byteoffset] & ( 0x01<<bitoffset ) )>>bitoffset;
        numbits--;
        bitoffset--;
        if ( bitoffset < 0 )
        {
            byteoffset++;
            bitoffset += 8;
            if ( byteoffset > bytecount )
            {
                return -1;
            }
        }
    }

    *info = inf;

    if ( input->check_BBV_flag && pBbv )
    {
        pBbv->frame_code_bits += bitcounter;
    }

    return bitcounter;           // return absolute offset in bit from start of frame
}

/*
*************************************************************************
* Function:Reads bits from the bitstream buffer
* Input:
byte buffer[]
containing VLC-coded data bits
int totbitoffset
bit offset from start of partition
int bytecount
total bytes in bitstream
int numbits
number of bits to read
* Output:
* Return:
* Attention:
*************************************************************************
*/

int ShowBits ( uchar_t buffer[],int totbitoffset,int bytecount, int numbits )
{

    register int inf;
    long byteoffset;      // byte from start of buffer
    int bitoffset;      // bit from start of byte

    if ( input->check_BBV_flag && pBbv )
    {
        pBbv->frame_code_bits += numbits;
    }

    byteoffset= totbitoffset/8;
    bitoffset= 7-( totbitoffset%8 );

    inf=0;
    while ( numbits )
    {
        inf <<=1;
        inf |= ( buffer[byteoffset] & ( 0x01<<bitoffset ) )>>bitoffset;
        numbits--;
        bitoffset--;
        if ( bitoffset < 0 )
        {
            byteoffset++;
            bitoffset += 8;
            if ( byteoffset > bytecount )
            {
                return -1;
            }
        }
    }

    return inf;           // return absolute offset in bit from start of frame
}


int get_uv ( int LenInBits, char*tracestring )
{
    SyntaxElement symbol, *sym=&symbol;

    assert ( currStream->streamBuffer != NULL );
    sym->mapping = linfo_ue;   // Mapping rule
    sym->len = LenInBits;
    SYMTRACESTRING( tracestring );
    GetSyntaxElement_FLC ( sym );

    return sym->inf;
}


int GetSyntaxElement_FLC( SyntaxElement *sym )
{
    int frame_bitoffset = currStream->byte_offset;
    uchar_t *buf = currStream->streamBuffer;
    int BitstreamLengthInBytes = currStream->bs_length;

    if ( ( GetBits( buf, frame_bitoffset, &( sym->inf ), BitstreamLengthInBytes, sym->len ) ) < 0 )
    {
        return -1;
    }

    sym->value1 = sym->inf;
    return 1;
}

/*
*************************************************************************
* Function: Check the stuffing pattern and stuffing_byte_pattern
at the end of picture header and slice
* Input:
* Output:
* Return:
* Attention:
* Author:
*************************************************************************
*/
#if M38817_DATA_STUFFING
int IsIVCStuffingPattern( int idx, int len )
{
    int  stuffing_bit_pattern;
    int  stuffing_byte_pattern;
    int  stuffing_byte_len;
    int  is_sutffing_byte_pattern;
    i32u_t  stuffing_byte_value;

    is_sutffing_byte_pattern = 1;
    stuffing_byte_value = 0;

    if( idx == 1 )
    {
        if ( len == 0 )
        {
            fprintf( stdout,"No Stuffing Bit At The End Of Slice!\n" );
            fprintf( stdout,"Slice Vertical Position: %d\n\n", slice_vertical_position );
            bValidSyntax = 0;
            return 0;
        }
        else if ( len > 8 )
        {
            fprintf( stdout,"Invalid Stuffing Pattern at the end of slice!\n%d Stuffing Bits Found (Too Much Stuffing Bits)!\n", len );
            stuffing_bit_pattern = get_uv( len, "stuffing pattern" );
            fprintf( stdout,"Slice Vertical Position: %d\n", slice_vertical_position );
            fprintf( stdout,"Invalid Stuffing Pattern: 0x%X\n\n", stuffing_bit_pattern );
            bValidSyntax = 0;
            return 0;
        }

        stuffing_bit_pattern = get_uv( len, "stuffing pattern" );
        if( stuffing_bit_pattern == ( 1 << ( len-1 ) ) )
        {
            return 1;
        }
        else
        {
            fprintf( stdout,"Wrong Stuffing Pattern at the end of slice!\n", len );
            fprintf( stdout,"Slice Vertical Position: %d\n", slice_vertical_position );
            fprintf( stdout,"Wrong Stuffing Pattern (%d Stuffing Bits): 0x%X\n\n", len, stuffing_bit_pattern );
            bValidSyntax = 0;
            return 0;
        }
    }
    else
    {
        int  stuffing_bit_num;
        char StartCodeType[20];

        switch( StartCodeValue )
        {
            case 0xB0:
                strcpy( StartCodeType, "Sequence_Header" );
                break;
            case 0xB2:
                strcpy( StartCodeType, "UserData" );
                break;
            case 0xB3:
                strcpy( StartCodeType, "I_Picture_Header" );
                break;
            case 0xB6:
                strcpy( StartCodeType, "PB_Picture_Header" );
                break;
            case 0xB5:
                strcpy( StartCodeType, "ExtensionData" );
                break;
            default:
                break;
        }

        if( len == 0 )
        {
            if( StartCodeValue != 0xB3 || StartCodeValue != 0xB6 )
            {
                fprintf( stdout, "\n**********************************************\n" );
            }
            fprintf( stdout,"No Stuffing Bit At The End Of %s!\n", StartCodeType );
            bValidSyntax = 0;
            return 0;
        }
        stuffing_bit_num = len % 8;
        stuffing_bit_num = stuffing_bit_num== 0 ? 8 : stuffing_bit_num;
        stuffing_bit_num = stuffing_bit_num == 1 ? stuffing_bit_num + 8 : stuffing_bit_num;

        //assert(len <= 8); // it is not always true for LDP configuration
        assert(stuffing_bit_num >= 2);
        assert(stuffing_bit_num <= 9); // In this newly proposed method, the length of stuffing bits is less than or equal to 9, and larger than 2.
                                       // 9 stuffing bits are only used when len = 1.
        stuffing_bit_pattern = u_v( stuffing_bit_num, "stuffing pattern" );
        if( stuffing_bit_pattern == ( 0x3 << ( stuffing_bit_num-2 ) ) )
        {
            if( len==stuffing_bit_num )
            {
                return 1;
            }
            else if (stuffing_bit_num == 9)
            {
                assert(len == 1);
                return 1;
            }
            else
            {
                assert(0);
                stuffing_byte_len = len-stuffing_bit_num;
                if( stuffing_byte_len <= 32 )
                {
                    stuffing_byte_pattern = get_uv( stuffing_byte_len, "stuffing_byte" );
                    stuffing_byte_value   = stuffing_byte_pattern;
                    is_sutffing_byte_pattern = stuffing_byte_pattern==0 ? 1 : 0;
                }
                else
                {
                    while ( stuffing_byte_len > 32 )
                    {
                        stuffing_byte_pattern = get_uv( 32, "stuffing_byte" );
                        is_sutffing_byte_pattern = stuffing_byte_pattern==0 ? is_sutffing_byte_pattern : 0;
                        stuffing_byte_value += stuffing_byte_pattern;
                        stuffing_byte_len -= 32;
                    }
                    stuffing_byte_pattern = get_uv( stuffing_byte_len, "stuffing_byte" );
                    is_sutffing_byte_pattern = stuffing_byte_pattern==0 ? is_sutffing_byte_pattern : 0;
                    stuffing_byte_value += stuffing_byte_pattern;
                }
                if( is_sutffing_byte_pattern==0 )
                {
                    if( StartCodeValue != 0xB3 || StartCodeValue != 0xB6 )
                    {
                        fprintf( stdout, "\n**********************************************\n" );
                    }
                    fprintf( stdout,"Wrong Stuffing Byte Pattern At The End Of %s!\n", StartCodeType );
                    bValidSyntax = 0;
                    return 0;
                }
                else
                {
                    return 1;
                }
            }
        }
        else
        {
            assert(0);
            if( StartCodeValue != 0xB3 || StartCodeValue != 0xB6 )
            {
                fprintf( stdout, "\n**********************************************\n" );
            }
            fprintf( stdout,"Wrong Stuffing Pattern At The End Of %s!\n", StartCodeType );
            fprintf( stdout,"Wrong Stuffing Pattern (%d Stuffing Bits): 0x%X\n\n", stuffing_bit_num, stuffing_bit_pattern );
            bValidSyntax = 0;
            if( len==stuffing_bit_num )
            {
                return 0;
            }
            else
            {
                stuffing_byte_len = len-stuffing_bit_num;
                if( stuffing_byte_len <= 32 )
                {
                    stuffing_byte_pattern = get_uv( stuffing_byte_len, "stuffing_byte" );
                    stuffing_byte_value   = stuffing_byte_pattern;
                    is_sutffing_byte_pattern = stuffing_byte_pattern==0 ? 1 : 0;
                }
                else
                {
                    while ( stuffing_byte_len > 32 )
                    {
                        stuffing_byte_pattern = get_uv( 32, "stuffing_byte" );
                        is_sutffing_byte_pattern = stuffing_byte_pattern==0 ? is_sutffing_byte_pattern : 0;
                        stuffing_byte_value += stuffing_byte_pattern;
                        stuffing_byte_len -= 32;
                    }
                    stuffing_byte_pattern = get_uv( stuffing_byte_len, "stuffing_byte" );
                    is_sutffing_byte_pattern = stuffing_byte_pattern==0 ? is_sutffing_byte_pattern : 0;
                    stuffing_byte_value  += stuffing_byte_pattern;
                }
                if( is_sutffing_byte_pattern==0 )
                {
                    if( StartCodeValue != 0xB3 || StartCodeValue != 0xB6 )
                    {
                        fprintf( stdout, "\n**********************************************\n" );
                    }
                    fprintf( stdout,"Wrong Stuffing Byte Pattern At The End Of %s!\n", StartCodeType );
                    bValidSyntax = 0;
                    return 0;
                }
                else
                {
                    return 0;
                }
            }
        }
    }
}
#else
int IsStuffingPattern( int idx, int len )
{
    int  stuffing_bit_pattern;
    int  stuffing_byte_pattern;
    int  stuffing_byte_len;
    int  is_sutffing_byte_pattern;
    i32u_t  stuffing_byte_value;

    is_sutffing_byte_pattern = 1;
    stuffing_byte_value = 0;

    if( idx == 1 )
    {
        if ( len == 0 )
        {
            fprintf( stdout,"No Stuffing Bit At The End Of Slice!\n" );
            fprintf( stdout,"Slice Vertical Position: %d\n\n", slice_vertical_position );
            bValidSyntax = 0;
            return 0;
        }
        else if ( len > 8 )
        {
            fprintf( stdout,"Invalid Stuffing Pattern at the end of slice!\n%d Stuffing Bits Found (Too Much Stuffing Bits)!\n", len );
            stuffing_bit_pattern = get_uv( len, "stuffing pattern" );
            fprintf( stdout,"Slice Vertical Position: %d\n", slice_vertical_position );
            fprintf( stdout,"Invalid Stuffing Pattern: 0x%X\n\n", stuffing_bit_pattern );
            bValidSyntax = 0;
            return 0;
        }

        stuffing_bit_pattern = get_uv( len, "stuffing pattern" );
        if( stuffing_bit_pattern == ( 1 << ( len-1 ) ) )
        {
            return 1;
        }
        else
        {
            fprintf( stdout,"Wrong Stuffing Pattern at the end of slice!\n", len );
            fprintf( stdout,"Slice Vertical Position: %d\n", slice_vertical_position );
            fprintf( stdout,"Wrong Stuffing Pattern (%d Stuffing Bits): 0x%X\n\n", len, stuffing_bit_pattern );
            bValidSyntax = 0;
            return 0;
        }
    }
    else
    {
        int  stuffing_bit_num;
        char StartCodeType[20];

        switch( StartCodeValue )
        {
            case 0xB0:
                strcpy( StartCodeType, "Sequence_Header" );
                break;
            case 0xB2:
                strcpy( StartCodeType, "UserData" );
                break;
            case 0xB3:
                strcpy( StartCodeType, "I_Picture_Header" );
                break;
            case 0xB6:
                strcpy( StartCodeType, "PB_Picture_Header" );
                break;
            case 0xB5:
                strcpy( StartCodeType, "ExtensionData" );
                break;
            default:
                break;
        }

        if( len == 0 )
        {
            if( StartCodeValue != 0xB3 || StartCodeValue != 0xB6 )
            {
                fprintf( stdout, "\n**********************************************\n" );
            }
            fprintf( stdout,"No Stuffing Bit At The End Of %s!\n", StartCodeType );
            bValidSyntax = 0;
            return 0;
        }
        stuffing_bit_num = len % 8;
        stuffing_bit_num = stuffing_bit_num== 0 ? 8 : stuffing_bit_num;
        stuffing_bit_pattern = u_v( stuffing_bit_num, "stuffing pattern" );
        if( stuffing_bit_pattern == ( 1 << ( stuffing_bit_num-1 ) ) )
        {
            if( len==stuffing_bit_num )
            {
                return 1;
            }
            else
            {
                stuffing_byte_len = len-stuffing_bit_num;
                if( stuffing_byte_len <= 32 )
                {
                    stuffing_byte_pattern = get_uv( stuffing_byte_len, "stuffing_byte" );
                    stuffing_byte_value   = stuffing_byte_pattern;
                    is_sutffing_byte_pattern = stuffing_byte_pattern==0 ? 1 : 0;
                }
                else
                {
                    while ( stuffing_byte_len > 32 )
                    {
                        stuffing_byte_pattern = get_uv( 32, "stuffing_byte" );
                        is_sutffing_byte_pattern = stuffing_byte_pattern==0 ? is_sutffing_byte_pattern : 0;
                        stuffing_byte_value += stuffing_byte_pattern;
                        stuffing_byte_len -= 32;
                    }
                    stuffing_byte_pattern = get_uv( stuffing_byte_len, "stuffing_byte" );
                    is_sutffing_byte_pattern = stuffing_byte_pattern==0 ? is_sutffing_byte_pattern : 0;
                    stuffing_byte_value += stuffing_byte_pattern;
                }
                if( is_sutffing_byte_pattern==0 )
                {
                    if( StartCodeValue != 0xB3 || StartCodeValue != 0xB6 )
                    {
                        fprintf( stdout, "\n**********************************************\n" );
                    }
                    fprintf( stdout,"Wrong Stuffing Byte Pattern At The End Of %s!\n", StartCodeType );
                    bValidSyntax = 0;
                    return 0;
                }
                else
                {
                    return 1;
                }
            }
        }
        else
        {
            if( StartCodeValue != 0xB3 || StartCodeValue != 0xB6 )
            {
                fprintf( stdout, "\n**********************************************\n" );
            }
            fprintf( stdout,"Wrong Stuffing Pattern At The End Of %s!\n", StartCodeType );
            fprintf( stdout,"Wrong Stuffing Pattern (%d Stuffing Bits): 0x%X\n\n", stuffing_bit_num, stuffing_bit_pattern );
            bValidSyntax = 0;
            if( len==stuffing_bit_num )
            {
                return 0;
            }
            else
            {
                stuffing_byte_len = len-stuffing_bit_num;
                if( stuffing_byte_len <= 32 )
                {
                    stuffing_byte_pattern = get_uv( stuffing_byte_len, "stuffing_byte" );
                    stuffing_byte_value   = stuffing_byte_pattern;
                    is_sutffing_byte_pattern = stuffing_byte_pattern==0 ? 1 : 0;
                }
                else
                {
                    while ( stuffing_byte_len > 32 )
                    {
                        stuffing_byte_pattern = get_uv( 32, "stuffing_byte" );
                        is_sutffing_byte_pattern = stuffing_byte_pattern==0 ? is_sutffing_byte_pattern : 0;
                        stuffing_byte_value += stuffing_byte_pattern;
                        stuffing_byte_len -= 32;
                    }
                    stuffing_byte_pattern = get_uv( stuffing_byte_len, "stuffing_byte" );
                    is_sutffing_byte_pattern = stuffing_byte_pattern==0 ? is_sutffing_byte_pattern : 0;
                    stuffing_byte_value  += stuffing_byte_pattern;
                }
                if( is_sutffing_byte_pattern==0 )
                {
                    if( StartCodeValue != 0xB3 || StartCodeValue != 0xB6 )
                    {
                        fprintf( stdout, "\n**********************************************\n" );
                    }
                    fprintf( stdout,"Wrong Stuffing Byte Pattern At The End Of %s!\n", StartCodeType );
                    bValidSyntax = 0;
                    return 0;
                }
                else
                {
                    return 0;
                }
            }
        }
    }
}
#endif