/*
*************************************************************************************
* File name: elements.h
* Function: Header file for elements in IVC streams
*
*************************************************************************************
*/

#ifndef _ELEMENTS_H_
#define _ELEMENTS_H_

/*!
*  definition of IVC syntaxelements
*  order of elements follow dependencies for picture reconstruction
*/
/*!
* \brief   Assignment of old TYPE partition elements to new
*          elements
*
*/

#define SE_HEADER                    0
#define SE_MBTYPE                    1
#define SE_MVD                          2
#define SE_CBP_INTRA              3
#define SE_LUM_DC_INTRA      4
#define SE_CHR_DC_INTRA      5
#define SE_LUM_AC_INTRA     6
#define SE_CHR_AC_INTRA     7
#define SE_CBP_INTER             8
#define SE_LUM_DC_INTER     9
#define SE_CHR_DC_INTER     10
#define SE_LUM_AC_INTER     11
#define SE_CHR_AC_INTER     12
#define SE_DELTA_QUANT_INTER      13
#define SE_DELTA_QUANT_INTRA      14
#define SE_BFRAME           15
#define SE_EOS              16
#define SE_MAX_ELEMENTS     17
#define SE_TU_TYPE 18

#endif

