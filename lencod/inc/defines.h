/*
*************************************************************************************
* File name:
* Function:
*
*************************************************************************************
*/
#ifndef _DEFINES_H_
#define _DEFINES_H_

#include <stdlib.h>
#include "../../common/commonstructs.h"


#define MAX_COST       2147483647


#define GEN_CONFORMANCE_BITSTR 1
#if GEN_CONFORMANCE_BITSTR
#define MULQP        0
#define MULSILICE    0 // == DBLK_E
#define DBLK_ABC     0
#define DBLK_D       0
#define DBLK_F       MULSILICE & DBLK_ABC
#define MVRANGE      0
#endif

#define M38817_DATA_STUFFING 1
#define FastME

#define HPGOPSIZE 4
#define L1_IMG          1   //sub P frame    
#define L2_IMG          2   //sub P frame       
#define L3_IMG          3   //sub P frame

#define _LUMA_COEFF_COST_       4 //!< threshold for luma coeffs

#define IMG_SUBPIXEL_PAD_SIZE    5          //!< Number of sub-pixels padded around the reference frame (>=4)
#define IMG_SUBPIXEL_PAD_SIZE_FOR_CLIP    17          //!< Number of sub-pixels padded around the reference frame (>=4)
#define IMG_PAD_SIZE             (32)   //!< Number of pixels padded around the reference frame (>=4)
//#define IMG_SUBPIXEL_PAD_SIZE    (IMG_SUBPIXEL_EXTRAPAD_SIZE+IMG_PAD_SIZE)


#define  LAMBDA_ACCURACY_BITS         16
#define  LAMBDA_FACTOR(lambda)        ((int)((double)(1<<LAMBDA_ACCURACY_BITS)*lambda+0.5))
#define  WEIGHTED_COST(factor,bits)   (((factor)*(bits))>>LAMBDA_ACCURACY_BITS)
#define  MV_COST(f,s,cx,cy,px,py)     (WEIGHTED_COST(f,mvbits[((cx)<<(s))-px]+mvbits[((cy)<<(s))-py]))
#define  REF_COST(f,ref)              (WEIGHTED_COST(f,refbits[(ref)]))

#define MAX_SYMBOLS_PER_MB  1200  //!< Maximum number of different syntax elements for one MB. CAVLC needs more symbols per MB

/*!< Maximum size of the string that defines the picture types to be coded, e.g. "IBBPBBPBB" */
#define MAXPICTURETYPESEQUENCELEN 100

#define RATECONTROL
#define TDRDO

#endif