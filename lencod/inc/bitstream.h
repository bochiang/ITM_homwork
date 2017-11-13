#ifndef _BITSTREAM_H_
#define _BITSTREAM_H_
#include <stdio.h>

#define STREAM_BUF_SIZE 1024 //must large than 3
typedef struct
{
    FILE *f;
    uchar_t buf[STREAM_BUF_SIZE];
    int iBytePosition;
    int iBitOffset;
    int iNumOfStuffBits;
    int iBitsCount;
} OutputStream;
int write_start_code( OutputStream *p,uchar_t code );
extern OutputStream *pORABS;

void CloseBitStreamFile();
void OpenBitStreamFile( char *Filename );
int  WriteSequenceHeader();
int  WriteUserData( char *userdata );
int  WriteSequenceEnd();
int  WriteVideoEditCode();
void WriteBitstreamtoFile( Bitstream *bitstr );
#endif