/*
*************************************************************************************
* File name: vlc.h
* Function: header for VLC coding functions
*
*************************************************************************************
*/


#ifndef _VLC_H_
#define _VLC_H_

int ue_v ( char *tracestring );
int u_1 ( char *tracestring );
int u_v ( int LenInBits, char *tracestring );
int i_8( char *tracestring );

// UVLC mapping
void linfo_ue( int len, int info, int *value1 );
void linfo_se( int len, int info, int *value1 );

void linfo_cbp_intra( int len,int info,int *cbp );
void linfo_cbp_inter( int len,int info,int *cbp );

int  readSyntaxElement_VLC ( SyntaxElement *sym );

int  GetVLCSymbol ( uchar_t buffer[],int totbitoffset,int *info, int bytecount );

int readSyntaxElement_FLC( SyntaxElement *sym );
int GetBits ( uchar_t buffer[],int totbitoffset,int *info, int bytecount,
              int numbits );
int ShowBits ( uchar_t buffer[],int totbitoffset,int bytecount, int numbits );

int get_uv ( int LenInBits, char*tracestring );
int GetSyntaxElement_FLC( SyntaxElement *sym );

#if M38817_DATA_STUFFING
int IsIVCStuffingPattern( int idx, int len );
#else
int IsStuffingPattern( int idx, int len );
#endif

#endif