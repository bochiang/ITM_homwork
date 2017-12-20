#ifndef _COMMONSTRUCTS_H_
#define _COMMONSTRUCTS_H_

#define ITM         "17.0"
#define VERSION     "201705"
#include "intra_add.h"

#define TRACE 0
#if TRACE
#define DEC_TRACE_FILE   "trace_dec.txt"
#define ENC_TRACE_FILE   "trace_enc.txt"
#endif

#define DISABLE_TEMPORAL_MV_SCALING 0

#if !DISABLE_TEMPORAL_MV_SCALING
#define USE_H263_TEMPORAL_MV_SCALING 1
#endif

// --------------------------------------- Data Type ----------------------------------------
typedef signed long long   i64s_t;
typedef signed int         i32s_t;
typedef signed short       i16s_t;
typedef signed char        char_t;
typedef unsigned long long i64u_t;
typedef unsigned int       i32u_t;
typedef unsigned short     i16u_t;
typedef unsigned char      uchar_t;
typedef unsigned char      bool;
typedef signed short       coef_t;

#define COMPILE_FOR_8BIT 1

#if COMPILE_FOR_8BIT
typedef uchar_t   pel_t;
#else
typedef i16u_t    pel_t;
#endif

#if COMPILE_FOR_8BIT
#if defined(_WIN32)
#define ENABLE_AVX2 1
#define ENABLE_SSE  0
#define ENABLE_NEON 0
#else
#define ENABLE_AVX2 0
#define ENABLE_SSE  0
#define ENABLE_NEON 1
#endif
#endif

// --------------------------------------- Constants ----------------------------------------
#define MAX_VALUE       999999           // used for start value for some variables
#define ERR_VALUE       -1
#define MAX_REF         8                // define it only for allocating MV arrays
// -------------------------------------- Image Type ----------------------------------------
typedef enum
{
    I_IMG = 0,
    P_IMG,
    B_IMG,
} PictureType;

// ------------------------------------ Transform Type ---------------------------------------
#define TRANS_2Nx2N    0
#define TRANS_NxN      1

// ----------------------------------------- Mode --------------------------------------------
enum PredModes
{
    PSKIP = 0,
    P2Nx2N,
    P2NxN,
    PNx2N,
    PNxN,
    I_MB,
    MAXMODE
};

enum PMbPartType
{
    P_16x16,
    P_8x16,
    P_16x8,
    I_Block,
    P_8x8
};

#define IS_INTRA(MB)    ((MB)->mb_type == I_MB)
#define IS_INTER(MB)    ((MB)->mb_type != I_MB)
#define IS_PSKIP(MB)    ((MB)->mb_type == PSKIP && (img->type == P_IMG))
#define IS_DIRECT(MB)   ((MB)->mb_type == PSKIP && (img->type == B_IMG))
#define IS_INTERMV(MB)  ((MB)->mb_type != I_MB && (MB)->mb_type != PSKIP)
#define IS_INTERMV_MODE(mode)  (mode != I_MB && mode != PSKIP)
#define IS_PNxN(MB)     ((MB)->mb_type == PNxN)


// ----------------------------------------- QP ---------------------------------------------
#define MIN_QP          0
#define MAX_QP          63
#define SHIFT_QP        11


// --------------------------------- Prediction Direction ------------------------------------
enum BFrmDirs
{
    FORWARD = 0,
    BACKWORD,
    BSYM,
    MHP,
    MaxBDir
};

// ------------------------------------- Block Sizes ------------------------------------------
#define B4_SIZE         4
#define BLOCK_SIZE      4
#define B8_SIZE         8
#define MB_SIZE_LOG2    4
#define MB_SIZE   (1<<MB_SIZE_LOG2)
#define MB_NUM    (MB_SIZE * MB_SIZE)
#define BLOCK_MULTIPLE      (MB_SIZE/(2*BLOCK_SIZE))

#define LARGE_CODING_UNIT 1
#if LARGE_CODING_UNIT
#define MAX_CU_BIT_SIZE 6
#define LCU_SIZE  (1<<MAX_CU_BIT_SIZE)
#define LCU_NUM   (LCU_SIZE*LCU_SIZE)
#define MB_WIDTH_IN_LCU  (LCU_SIZE/MB_SIZE)
#endif

// ----------------------------------- Intra prediction mode -----------------------------------
enum IntraPredModeLuma
{
    VERT_PRED = 0,
    HOR_PRED,
    DC_PRED,
    DOWN_LEFT_PRED,
    DOWN_RIGHT_PRED,
#if USING_INTRA_5_9
    INTRA_BILINEAR,
    INTRA_PLANE,
    INTRA_XY20,
    INTRA_XY30,
#endif
    IntraPredModeNum
};

enum IntraPredModeChroma
{
    DC_PRED_8 = 0,
    HOR_PRED_8,
    VERT_PRED_8,
    PLANE_8,
    ChromaPredModeNum
};


// --------------------------------- MV prediction---------------------------------------------
#define MVPRED_xy_MIN   0
#define MVPRED_L        1
#define MVPRED_U        2
#define MVPRED_UR       3

#define MAX(a, b)          ((a) > (b) ? (a) : (b))
#define MIN(a, b)          ((a) < (b) ? (a) : (b))
#define absm(A)            ((A)<(0) ? (-(A)):(A))
#define Clip1(a)           ((a)>255?255:((a)<0?0:(a)))
#define Clip3(min,max,val) (((val)<(min))?(min):(((val)>(max))?(max):(val)))
#define clamp(a,b,c)       ((a) < (b) ? (b) : ((a) > (c) ? (c) : (a)))


// --------------------------------- block types for AEC --------------------------------------
#define LUMA_8x8        2
#define CHROMA_DC       6
#define CHROMA_AC       7

#define MAX_CODED_FRAME_SIZE 1500000         //!< bytes for one frame

#define B_BITS  10
#define LG_PMPS_SHIFTNO 2
#define QUARTER   (1 << (B_BITS-2))


// ------------------------- contexts for RM syntax elements -----------------------------------
typedef enum
{
    BITS_HEADER,
    BITS_TOTAL_MB,
    BITS_MB_MODE,
    BITS_REF_IDX,
    BITS_INTER_MB,
    BITS_INTRA_INFO,
    BITS_CBP_MB,
    BITS_COEFF_Y_MB,
    BITS_COEFF_UV_MB,
    MAX_BITCOUNTER_MB
} BitCountType;

// ------------------------- struct for context management ------------------------------------
typedef struct
{
    uchar_t MPS;     //1  bit
    i32u_t  LG_PMPS; //10 bits
    uchar_t cycno;   //2  bits
} BiContextType;

typedef BiContextType *BiContextTypePtr;


#define NUM_MB_TYPE_CTX    11
#define NUM_B8_TYPE_CTX    9
#define NUM_MV_RES_CTX     10
#define NUM_DELTA_QP_CTX   4

typedef struct
{
    BiContextType mb_type_contexts [3][NUM_MB_TYPE_CTX];
    BiContextType b8_type_contexts [2][NUM_B8_TYPE_CTX];
    BiContextType mv_res_contexts  [2][NUM_MV_RES_CTX];
    BiContextType delta_qp_contexts[NUM_DELTA_QP_CTX];
} MotionInfoContexts;

#define NUM_CBP_CTX    4
#define NUM_MAP_CTX   16
#define NUM_LAST_CTX  16
#define NUM_ONE_CTX    5
#define NUM_ABS_CTX    5
#define NUM_IPR_CTX    4
#define NUM_CIPR_CTX   4
#define NUM_DELTAQP_CTX 4

#define NUM_BLOCK_TYPES 8

typedef struct
{
    BiContextType qsplit_contexts[1];
    BiContextType  ipr_contexts [NUM_IPR_CTX];
    BiContextType  cipr_contexts[NUM_CIPR_CTX];
    BiContextType  cbp_contexts [3][NUM_CBP_CTX];
    BiContextType  one_contexts [NUM_BLOCK_TYPES][NUM_ONE_CTX];
    BiContextType  abs_contexts [NUM_BLOCK_TYPES][NUM_ABS_CTX];
    BiContextType  map_contexts [NUM_BLOCK_TYPES][NUM_MAP_CTX];
    BiContextType  last_contexts[NUM_BLOCK_TYPES][NUM_LAST_CTX];
    BiContextType  tu_size_context[1];
    BiContextType  MBdeltaQP_contexts[NUM_DELTAQP_CTX];
} TextureInfoContexts;


MotionInfoContexts *create_contexts_MotionInfo( void );
TextureInfoContexts *create_contexts_TextureInfo( void );
void delete_contexts_MotionInfo( MotionInfoContexts *enco_ctx );
void delete_contexts_TextureInfo( TextureInfoContexts *enco_ctx );

#endif