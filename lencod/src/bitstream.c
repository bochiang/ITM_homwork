/*
*************************************************************************************
* File name:
* Function:  Annex B Byte Stream format NAL Unit writing routines
*
*************************************************************************************
*/
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "global.h"
#include "bitstream.h"
#define MAXHEADERSIZE 100
#include "vlc.h"
#include "AEC.h"

static FILE *f = NULL;    // the output file

uchar_t bit[8] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
OutputStream  ORABS;
OutputStream *pORABS = &ORABS;

void OpenORABS( OutputStream *p,char *fname )
{
    p->f = fopen( fname,"wb" );
    if( p->f==NULL )
    {
        printf ( "\nCan't open file %s",fname );
        exit( -1 );
    }

    p->iBytePosition      = 0;
    p->iBitOffset         = 0;
    p->iNumOfStuffBits        = 0;
    p->iBitsCount         = 0;
}
void CloseORABS( OutputStream *p )
{
    if( p->iBitOffset )
    {
        fwrite( p->buf,1,p->iBytePosition+1,p->f );
    }
    else
    {
        fwrite( p->buf,1,p->iBytePosition  ,p->f );
    }
    fclose( p->f );
}
void FlushORABS( OutputStream *p )
{
    fflush( p->f );
}


int write_1_bit( OutputStream *p,int b )
{
    int i;

    if( p->iBytePosition == STREAM_BUF_SIZE )
    {
        i = (int)fwrite( p->buf,1,STREAM_BUF_SIZE,p->f );
        if( i!=STREAM_BUF_SIZE )
        {
            printf ( "Fatal: write file error, exit (-1)\n" );
            exit ( -1 );
        }
        p->iBytePosition    = 0;
        p->iBitOffset       = 0;
    }
    if( b )
    {
        p->buf[p->iBytePosition] |= bit[p->iBitOffset];
    }
    else
    {
        p->buf[p->iBytePosition] &= ( ~bit[p->iBitOffset] );
    }
    p->iBitOffset++;
    if( p->iBitOffset==8 )
    {
        p->iBitOffset = 0;
        p->iBytePosition++;
    }
    p->iBitsCount++;
    return 0;
}
int write_n_bit( OutputStream *p,int b,int n )
{
    if( n>30 )
    {
        return 1;
    }
    while( n>0 )
    {
        write_1_bit( p,b&( 0x01<<( n-1 ) ) );
        n--;
    }
    return 0;
}


/*
*************************************************************************
* Function: one bit "1" is added to the end of stream, then some bits "0" are added to bytealigned position.
* Input:
* Output:
* Return:
* Attention:
* Author:
*************************************************************************
*/

#if M38817_DATA_STUFFING
int write_ivc_align_stuff( OutputStream *p )
{
    uchar_t c;
    int len;    //bit,ITM_r2
    c = 0xff << ( 8 - p->iBitOffset );
    p->buf[p->iBytePosition] = ( c & p->buf[p->iBytePosition] ) | ( 0xc0>>( p->iBitOffset ) );

    p->iBitsCount += 8 - p->iBitOffset;
    len   = 8 - p->iBitOffset;
    p->iNumOfStuffBits  += 8 - p->iBitOffset;
    p->iBitOffset = 0;
    p->iBytePosition++;

    if( p->iBitOffset == 7 )
    {
        p->buf[p->iBytePosition] = 0xc0<<( 8-p->iBitOffset );
        p->iBytePosition++;
        p->iBitsCount += 8;
        len += 8;
        p->iNumOfStuffBits  += 8;
    }
    return /*0*/len;   //bit,ITM_r2
}
#else
int write_align_stuff( OutputStream *p )
{
    uchar_t c;
    int len;    //bit,ITM_r2

    c = 0xff << ( 8 - p->iBitOffset );
    p->buf[p->iBytePosition] = ( c & p->buf[p->iBytePosition] ) | ( 0x80>>( p->iBitOffset ) );
    p->iBitsCount += 8 - p->iBitOffset;
    len   = 8 - p->iBitOffset;
    p->iNumOfStuffBits    += 8 - p->iBitOffset;
    p->iBitOffset = 0;
    p->iBytePosition++;
    return /*0*/len;   //bit,ITM_r2
}
#endif

//---end
int write_start_code( OutputStream *p,uchar_t code )
{
    int i;


    if( p->iBytePosition >= STREAM_BUF_SIZE-4 && p->iBytePosition >0 )
    {
        i = (int)fwrite( p->buf,1,p->iBytePosition,p->f );
        if( i != p->iBytePosition )
        {
            printf ( "\nWrite file error" );
            exit ( -1 );
        }
        p->iBytePosition    = 0;
        p->iBitOffset       = 0;
    }
    p->buf[p->iBytePosition  ] = 0; // 0000 0000 0000 0000 0000 0001 + code
    p->buf[p->iBytePosition+1] = 0;
    p->buf[p->iBytePosition+2] = 1;
    p->buf[p->iBytePosition+3] = code;
    p->iBytePosition += 4;
    p->iBitsCount += 32;

    return 0;
}

/*
*************************************************************************
* Function:Open the output file for the bytestream
* Input: The filename of the file to be opened
* Output:
* Return: none.Function terminates the program in case of an error
* Attention:
*************************************************************************
*/


void OpenBitStreamFile( char *Filename )
{
    OpenORABS( pORABS,Filename );
}
void CloseBitStreamFile()
{
    CloseORABS( pORABS );
}

/*
*************************************************************************
* Function:Write video edit code
* Input:
* Output:
* Return: 32bit for video edit code
* Attention:
*************************************************************************
*/

int WriteVideoEditCode()
{
    Bitstream *bitstream;
    pel_t VideoEditCode[32];
    int  bitscount=0;

    if ( ( bitstream=calloc( 1, sizeof( Bitstream ) ) )==NULL )
    {
        no_mem_exit( "Seuqence Header: bitstream" );
    }

    bitstream->streamBuffer = VideoEditCode;
    bitstream->bits_to_go = 8;

    bitscount = u_v( 32, "video_edit_code",0x1B7,bitstream );

    write_start_code( pORABS, 0xb7 );

    free( bitstream );

    return bitscount;
}

/*
*************************************************************************
* Function:Write sequence header information
* Input:
* Output:
* Return: sequence header length, including stuffing bits
* Attention:
*************************************************************************
*/
int WriteSequenceHeader()
{
    Bitstream *bitstream;
    pel_t SequenceHeader[MAXHEADERSIZE];
    int  bitscount=0;
    int  stuffbits;
    int  i,j,k;
    if ( ( bitstream=calloc( 1, sizeof( Bitstream ) ) )==NULL )
    {
        no_mem_exit( "Seuqence Header: bitstream" );
    }

    bitstream->streamBuffer = SequenceHeader;
    bitstream->bits_to_go = 8;

    input->display_horizontal_size = img->width;
    input->display_vertical_size   = img->height;

    input->bbv_buffer_size = input->BBS_size>>14;

    input->aspect_ratio=1;
    input->bit_rate_lower=( input->bit_rate/400 )&( 0x3FFFF );
    input->bit_rate_upper=( input->bit_rate/400 )>>18;

    bitscount+=u_v( 32,"seqence start code",0x1b0,bitstream );
    bitscount+=u_v( 8,"profile_id",input->profile_id,bitstream );
    bitscount+=u_v( 8,"level_id",input->level_id,bitstream );

    bitscount+=u_v( 14,"picture width", input->width_org,bitstream );
    bitscount += u_v(14, "picture height", input->height_org, bitstream);

    bitscount+=u_v( 2,"chroma foramt",input->chroma_format,bitstream );
    bitscount+=u_v( 3,"sample precision", 1,bitstream );
    bitscount+=u_v( 4,"aspect ratio",input->aspect_ratio,bitstream );
    bitscount+=u_v( 4,"frame rate code",input->frame_rate_code,bitstream );

    bitscount+=u_v( 18,"bit rate lower",input->bit_rate_lower,bitstream );
    bitscount+=u_v( 1,"marker bit",1,bitstream );
    bitscount+=u_v( 12,"bit rate upper",input->bit_rate_upper,bitstream );
    bitscount+=u_v( 1,"low delay",input->low_delay,bitstream );
    bitscount+=u_v( 1,"marker bit",1,bitstream );
    bitscount+=u_v( 18,"bbv buffer size",input->bbv_buffer_size,bitstream );
    bitscount+=u_v( 1, "abt enable", input->abt_enable, bitstream );
    bitscount+=u_v( 1, "if type", input->if_type, bitstream );
    bitscount+=u_v( 4,"reserved bits",0,bitstream );

    k = bitscount >> 3;
    j = bitscount % 8;

    stuffbits = 8-( bitscount%8 );

    // write the start code of the sequence header
    write_start_code( pORABS, 0xb0 );

    // write the information of sequence header to bit stream file
    // the information has been stored in "bitstream" (SequenceHeader[]), but not written to bit stream file
    for( i=4; i<k; i++ )
    {
        write_n_bit( pORABS,SequenceHeader[i],8 );
    }

    if( j!=0 )
    {
        write_n_bit( pORABS, bitstream->byte_buf,j );
    }
#if M38817_DATA_STUFFING
    bitscount+=write_ivc_align_stuff( pORABS );
#else
    bitscount+=write_align_stuff( pORABS );
#endif

    free( bitstream );

    return bitscount;
}

int WriteSequenceEnd()
{
    write_start_code( pORABS, 0xb1 );
    return 32;
}

/*
*************************************************************************
* Function:Write user data
* Input:
* Output:
* Return: user data length
* Attention:
*************************************************************************
*/

int WriteUserData( char *userdata )
{
    Bitstream *bitstream;
    pel_t UserData[MAXHEADERSIZE];
    int  bitscount=0;

    if ( ( bitstream=calloc( 1, sizeof( Bitstream ) ) )==NULL )
    {
        no_mem_exit( "User data: bitstream" );
    }
    bitstream->streamBuffer = UserData;
    bitstream->bits_to_go = 8;

    bitscount += u_v( 32,"user data start code", 0x1b2,bitstream );
    write_start_code( pORABS, 0xb2 );
    while ( *userdata )
    {
        write_n_bit( pORABS,*userdata,8 );
        bitscount += u_v( 8,"user data", *userdata++,bitstream );
    }
#if M38817_DATA_STUFFING
    bitscount+=write_ivc_align_stuff( pORABS );
#else
    bitscount+=write_align_stuff( pORABS );
#endif
    free( bitstream );

    return bitscount;
}

/*
*************************************************************************
* Function:Write bit steam to file
* Input:
* Output:
* Return: none
* Attention:
*************************************************************************
*/
void WriteBitstreamtoFile( Bitstream *bitstr )
{
    int n, i;
    n = bitstr->byte_pos;

    for( i=0; i<n; i++ )
    {
        if( bitstr->streamBuffer[i]==0 && bitstr->streamBuffer[i+1]==0 && bitstr->streamBuffer[i+2]==1 )
        {
            write_start_code( pORABS, bitstr->streamBuffer[i+3] );
            i=i+4;
        }

        write_n_bit( pORABS, bitstr->streamBuffer[i],8 );
    }

    if( img->type == I_IMG && img->ip_frm_idx == 0 )
    {
        seq_header = stat->bit_use_header[3];
    }
    stat->bit_ctr += 8*n;
    stat->bit_ctr += stat->bit_use_header[3];
    stat->bit_use_header[3] = 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////
/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void error( char *text, int code )
{
    fprintf( stderr, "%s\n", text );
    exit( code );
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
int start_sequence()
{
    int len = 0;
    char id_string[255] = "IVC test stream";

    if ( img->ip_frm_idx == 0 )
    {
        OpenBitStreamFile( input->outfile );
    }

    len = WriteSequenceHeader();

    if ( img->ip_frm_idx == 0 )
    {
        if ( strlen( id_string ) > 1 )
        {
            len += WriteUserData( id_string );
        }
    }

    return len;
}
/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:
* Attention:Mainly flushing of everything Add termination symbol, etc.
*************************************************************************
*/
int terminate_sequence()
{
    int len;
    len = WriteSequenceEnd();

    if ( img->EncodeEnd_flag==1 )
    {
        CloseBitStreamFile();
    }
    printf( "Sequence End\n" );
    return len;
}

