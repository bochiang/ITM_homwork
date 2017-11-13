/*!
***************************************************************************
* \file
*    biaridecod.h
*
* \brief
*    Headerfile for binary arithmetic decoder routines
*
* \author
*    Ping Yang
*
**************************************************************************
*/

#ifndef _BIARIDECOD_H_
#define _BIARIDECOD_H_


/************************************************************************
* D e f i n i t i o n s
***********************************************************************
*/

void arideco_start_decoding( Env_AEC *dep, uchar_t *cpixcode, int firstbyte, int *cpixcode_len );
int  arideco_bits_read( Env_AEC * dep );
void arideco_done_decoding( Env_AEC * dep );

i32u_t biari_decode_symbol( Env_AEC * dep, BiContextTypePtr bi_ct );
i32u_t biari_decode_symbolW( Env_AEC * dep, BiContextTypePtr bi_ct1, BiContextTypePtr bi_ct2 );
i32u_t biari_decode_symbol_eq_prob( Env_AEC * dep );
i32u_t biari_decode_final( Env_AEC * dep );

#endif  // BIARIDECOD_H_

