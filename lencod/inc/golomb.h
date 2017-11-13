#ifndef GOLOMB_H
#define GOLOMB_H

#include "global.h"

void encode_golomb_word( i32u_t symbol,i32u_t grad0,i32u_t max_levels,i32u_t *res_bits,i32u_t *res_len ); //returns symbol coded. (might be cropped if max_levels is too small)
void encode_multilayer_golomb_word( i32u_t symbol,const i32u_t *grad,const i32u_t *max_levels,i32u_t *res_bits,i32u_t *res_len ); //terminate using a max_levels value of 30UL.

i32u_t decode_golomb_word( const uchar_t **buffer,i32u_t *bitoff,i32u_t grad0,i32u_t max_levels );

int writeSyntaxElement_GOLOMB( SyntaxElement *se, Bitstream *bitstream );

#endif