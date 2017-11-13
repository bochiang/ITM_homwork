#ifndef _AEC_H_
#define _AEC_H_

#include "global.h"

// AEC
void arienco_start_encoding( Env_AEC* eep, pel_t *code_buffer, int *code_len );
int  arienco_bits_written( Env_AEC* eep );
void arienco_done_encoding( Env_AEC* eep );

void biari_init_context_logac ( BiContextTypePtr ctx );

void biari_encode_symbol( Env_AEC* eep, uchar_t  symbol, BiContextTypePtr bi_ct );
void biari_encode_symbolW( Env_AEC* eep, uchar_t  symbol, BiContextTypePtr bi_ct1,  BiContextTypePtr bi_ct2 );
void biari_encode_symbol_eq_prob( Env_AEC* eep, uchar_t  symbol );
void biari_encode_symbol_final( Env_AEC* eep, uchar_t  symbol );


int writeMBPartTypeInfo_AEC( Macroblock *currMB, CSobj *cs_aec, int val );
int write_B8x8_PredTypeInfo_AEC( CSobj *cs_aec, int val );
int write_Bfrm_PredTypeInfo_AEC( CSobj *cs_aec, int val );

int write_P16x16_PredTypeInfo_AEC( CSobj *cs_aec, int val );
int write_Pfrm_PredTypeInfo_AEC( CSobj *cs_aec, int val );

int writeMBTransType_AEC( CSobj *cs_aec, int val );
int writeSubMBTransType_AEC( CSobj *cs_aec, int val );
int writeIntraPredMode_AEC( CSobj *cs_aec, int val );
int writeCIPredMode_AEC( CSobj *cs_aec, int val );
int writeMVD_AEC( Macroblock *currMB, CSobj *cs_aec, int b8, int val_1, int val_2 );
int writeCBP_AEC( Macroblock *currMB, CSobj *cs_aec, int val );
int writeRunLevel_AEC( CSobj *cs_aec, int tu_size, int* levels, int* runs, int pairs, int ctx );
void writeCBP_BIT_AEC ( Macroblock* currMB, CSobj *cs_aec, int b8, int b4, int bit, int cbp, Env_AEC* eep_dp );
int write_Reffrm_AEC( CSobj *cs_aec, int val );
int writeRunLengthInfo2Buffer_AEC( CSobj *cs_aec, int val );
int writeMBDeltaQp_AEC(CSobj *cs_aec, int val, int prev_qp);

#endif  // AEC_H