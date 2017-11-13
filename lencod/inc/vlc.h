/**************************************************************************************
* File name: vlc.h
* Function: Prototypes for VLC coding functions
**************************************************************************************/

#ifndef _VLC_H_
#define _VLC_H_

int u_1 ( char *tracestring, int value, Bitstream *part );
int ue_v ( char *tracestring, int value, Bitstream *part );
int u_v ( int n, char *tracestring, int value, Bitstream *part );

int   writeSyntaxElement_UVLC( SyntaxElement *se, Bitstream *this_dataPart );
int   writeSyntaxElement_fixed( SyntaxElement *se, Bitstream *this_dataPart );

void  writeUVLC2buffer( SyntaxElement *se, Bitstream *currStream );
int   symbol2uvlc( SyntaxElement *se );
void  ue_linfo( int n, int dummy, int *len,int *info );
void  se_linfo( int mvd, int dummy, int *len,int *info );

#endif