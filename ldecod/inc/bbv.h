#ifndef  _BBV_H_
#define  _BBV_H_

#include "global.h"

BbvBuffer_t* init_bbv_memory( int frame_rate_code, int low_delay, int bbv_buffer_size, int bit_rate_upper, int bit_rate_lower );
BbvBuffer_t* free_bbv_memory( BbvBuffer_t* pBbv );
void stat_bbv_buffer( BbvBuffer_t* pBbv );
void update_bbv( BbvBuffer_t* pBbv, int code_bits );
void calc_min_BBS_size( int* FrameBits, int BitRate, float FrameRate, int FrameNum, int *Bmin_out, int *Fmin_out );


#endif //_BBV_H_