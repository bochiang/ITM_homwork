/*
*************************************************************************************
* File name: header.h
* Function: Prototypes for header.c
*
*************************************************************************************
*/


#ifndef _HEADER_H_
#define _HEADER_H_
#include "global.h"


void calc_picture_distance( ImgParams *img );
void SequenceHeader ( uchar_t *buf,int starcodepos, int length );
void user_data();
void video_edit_code_data();
void I_Picture_Header();
void PB_Picture_Header();
void SliceHeader( uchar_t *buf,int startcodepos, int length );
void CheckHighLevelSyntax ( int idx );
void CheckBitrate ( int bitrate, int second );
#endif

