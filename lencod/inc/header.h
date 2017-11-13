/**************************************************************************************
* File name: header.h
* Function: Prototypes for header.c
**************************************************************************************/

#ifndef _HEADER_H_
#define _HEADER_H_
#include "global.h"

int slice_vertical_position_extension;
int slice_horizontal_position_extension;

int stream_length;

int frame_rate;
int tc0;
int marker_bit;

int  SliceHeader( Bitstream *bitstream, int slice_qp );
int  IPictureHeader( Bitstream *bitstream, int frame );
int  PBPictureHeader( Bitstream *bitstream );
void write_terminating_bit ( CSobj *cs_aec, uchar_t );
#endif

