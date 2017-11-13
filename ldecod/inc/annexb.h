/*
*************************************************************************************
* File name: annexb.h
* Function: Annex B byte stream buffer handling.
*
*************************************************************************************
*/

#ifndef _ANNEXB_H_
#define _ANNEXB_H_

void OpenBitstreamFile ( char *fn );
void CloseBitstreamFile();
int  IsEndOfBitstream ();
int  GetOneUnit ( uchar_t *buf,int *startcodepos,int *       length );
int get_uv ( int LenInBits, char*tracestring );
int check_slice_stuffing();

#endif

