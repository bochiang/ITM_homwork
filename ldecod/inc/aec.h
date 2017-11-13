#ifndef _AEC_H_
#define _AEC_H_

#include "global.h"

void readMBTransType_AEC( CSobj *cs_aec, int *val );
void readMBPartTypeInfo_AEC( CSobj *cs_aec, int *val, int bframe, int act_ctx);
void read_B8x8_PredTypeInfo_AEC( CSobj *cs_aec, int *val );
void read_Bfrm_PredTypeInfo_AEC( CSobj *cs_aec, int *val );
void read_P16x16_PredTypeInfo_AEC( CSobj *cs_aec, int *val );
void read_Pfrm_PredTypeInfo_AEC( CSobj *cs_aec, int *val );

void readSubMBTransType_AEC( CSobj *cs_aec, int *val );
void readIntraPredMode_AEC( CSobj *cs_aec, int *val );
void readCIPredMode_AEC( CSobj *cs_aec, int *val );

void readMVD_AEC(Macroblock *currMB, CSobj *cs_aec, int b8, int *val_1, int val_2);
void readCBP_AEC(Macroblock *currMB, CSobj *cs_aec, int *val);
void readRunLevel_AEC( CSobj *cs_aec, int bsize, int *level, int *run, int ctx_in );
void read_Reffrm_AEC( CSobj *cs_aec, int *val );
void readMBDeltaQp_AEC(CSobj *cs_aec, int *val, int prev_qp);

#endif  // _AEC_H_