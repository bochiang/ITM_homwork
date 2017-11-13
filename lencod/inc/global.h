/*
*************************************************************************************
* File name:
* Function:
*
*************************************************************************************
*/

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <stdio.h>
#include "defines.h"
#include "minmax.h"

#define _BUGFIX_NON16MUL_PICSIZE

//! Boolean Type
typedef enum
{
    FALSE,
    TRUE
} Boolean;

//! definition of IVC syntax elements
typedef enum
{
    SE_HEADER,
    SE_MBTYPE,
    SE_INTRAPREDMODE,
    SE_TUTYPE,
    SE_MVD,
    SE_CBP_INTRA,
    SE_LUM_DC_INTRA,
    SE_CHR_DC_INTRA,
    SE_LUM_AC_INTRA,
    SE_CHR_AC_INTRA,
    SE_CBP_INTER,
    SE_LUM_DC_INTER,
    SE_CHR_DC_INTER,
    SE_LUM_AC_INTER,
    SE_CHR_AC_INTER,
    SE_DELTA_QUANT_INTER,
    SE_DELTA_QUANT_INTRA,
    SE_BFRAME,
    SE_EOS,
    SE_MAX_ELEMENTS  //!< number of maximum syntax elements
} SE_type;



typedef struct BbvBuffer_t
{
    char  bbv_mode;           // 0:not 0xFFFF, 1:0xFFFF
    char  vec_flag;           // the flag of video edit code(vec), 0: vec doesn't exit, 1: vec exit.
    int   bitrate;            // bit per second (bit/s)
    float framerate;          // frame per second (frame/s)
    float bbv_delay;
    float low_delay;
    int   BBS_size;
    float frmout_interval;
    int   currFrm_max_bit;
    int   currFrm_min_bit;
    int   frm_no;
    int   check_continue;
    int   *FrameBits;
} BbvBuffer_t;

//! Macroblock
typedef struct macroblock
{
    int slice_nr;
    // some storage of macroblock syntax elements for global access
    int mb_type;
    int trans_type;
    int sub_mb_trans_type[4];  // only used in I_MB
    int mvd[2][2][2][2]; // [fw/bw][block_y][block_x][x/y]
    int intra_pred_modes[16];
    int cbp;
    int b8mode[4];
    int b8pdir[4];
    int fw_ref[4];
    int bw_ref[4];
    int sym_ref[4][2];   // sym_ref[pu_idx][fw/bw]

    int c_ipred_mode;    // chroma intra prediction mode

    struct macroblock *mb_available_up;
    struct macroblock *mb_available_left;
    struct macroblock *mb_available_rightup;
    struct macroblock *mb_available_leftup;
    int mb_addr_left, mb_addr_up, mb_addr_rightup, mb_addr_leftup;
    int mbAvailA, mbAvailB, mbAvailC, mbAvailD;
    int skip_flag;

    int slice_header_flag;
    int sliceqp;
    int mb_qp_delta;
    uchar_t uhBitSizeMB;
} Macroblock;

//! Bitstream
typedef struct
{
    int   byte_pos;           //!< current position in bitstream;
    int   bits_to_go;         //!< current bitcounter
    pel_t  byte_buf;           //!< current buffer for last written byte

    pel_t  *streamBuffer;      //!< actual buffer for written bytes
} Bitstream;

void FreeBitstream( Bitstream *bitstr );
Bitstream *AllocateBitstream();

#define MAX_SLICE_NUM_IN_PIC 100

pel_t   *imgY_org_buffer;           //!< Reference luma image

#ifdef TDRDO
pel_t * imgY_pre_buffer;
#endif

// global picture format dependent buffers, mem allocation in image.c
uchar_t *imgY_rec;
uchar_t *imgU_rec;
uchar_t *imgV_rec;
uchar_t *imgY_org;          //!< Reference luma image
uchar_t *imgU_org;          //!< Reference chroma image
uchar_t *imgV_org;          //!< Reference chroma image
pel_t    **nextP_imgY;
pel_t    ***nextP_imgUV;

int *img4Y_tmp;          //!< for quarter pel interpolation

int Min_V_MV;
int Max_V_MV;
int Min_H_MV;
int Max_H_MV;

uchar_t *mref[33];                    //!< 1/4 pix luma
uchar_t *mcef[33][2];                 //!< pix chroma

uchar_t *reference_frame[33][3];      //[refnum][yuv][height*width]
uchar_t *current_frame[3];    //[yuv][height*width]

pel_t *Refbuf11[33];                //!< 1/1th pel (full pel) reference frame buffer

uchar_t *ref_frm[33][3];  //[refnum(4 for filed)][yuv][height(height/2)*width]

// global picture format dependend buffers, mem allocation in image.c (frame buffer)
uchar_t  *mref_frm[33];               //!< 1/4 pix luma //[2:ref_index]

int af_intra_cnt;

// reference frame buffer

uchar_t **reference_field[6][3];  //[refnum][yuv][height/2][width]
uchar_t ***ref[4];                //[refnum(4 for filed)][yuv][height(height/2)][width]
uchar_t ***b_ref[2],     ***f_ref[2];
uchar_t ***b_ref_frm[2], ***f_ref_frm[2];

int intras;              //!< Counts the intra updates in each frame.
int Bframe_ctr;
int tot_time;

#define ET_SIZE 300      //!< size of error text buffer
char errortext[ET_SIZE]; //!< buffer for error message for exit with error()

//! SNRParameters
typedef struct
{
    float snr_y;               //!< current Y SNR
    float snr_u;               //!< current U SNR
    float snr_v;               //!< current V SNR
    float snr_y1;              //!< SNR Y(dB) first frame
    float snr_u1;              //!< SNR U(dB) first frame
    float snr_v1;              //!< SNR V(dB) first frame
    float snr_ya;              //!< Average SNR Y(dB) remaining frames
    float snr_ua;              //!< Average SNR U(dB) remaining frames
    float snr_va;              //!< Average SNR V(dB) remaining frames
} SNRParameters;

// Set block sizes
static int blc_size[8][2] =
{
    {2, 2}, // PSKIP
    {2, 2}, // 2Nx2N
    {2, 1}, // 2NxN
    {1, 2}, // Nx2N
    {1, 1}  // NxN
};
//! all input parameters
typedef struct
{
    int FrmsToBeEncoded;          //!< number of frames to be encoded
    int qp_1st_frm;               //!< QP of first frame
    int qp_P_frm;                 //!< QP of remaining frames
    int hadamard;                 /*!< 0: 'normal' SAD in 1/3 pixel search.  1: use 4x4 Haphazard transform and '
                                       Sum of absolute transform difference' in 1/3 pixel search                   */
    int usefme;                   //!< Fast Motion Estimat. 0=disable, 1=UMHexagonS
    int search_range;             /*!< search range - integer pel search and 16x16 blocks.  The search window is
                                       generally around the predicted vector. Max vector is 2xmcrange.  For 8x8
                                       and 4x4 block sizes the search range is 1/2 of that for 16x16 blocks.       */
    int inp_ref_num;              /*!< 1: prediction from the last frame only. 2: prediction from the last or
                                       second last frame etc.  Maximum 5 frames                                    */
    int width_org;                //!< GH: if CUSTOM image format is chosen, use this size
    int height_org;               //!< GH: width and height must be a multiple of 16 pels
    int yuv_format;               //!< GH: YUV format (0=4:0:0, 1=4:2:0, 2=4:2:2, 3=4:4:4,currently only 4:2:0 is supported)
    int color_depth;              //!< GH: YUV color depth per component in bit/pel (currently only 8 bit/pel is supported)
    int intra_upd;                /*!< For error robustness. 0: no special action. 1: One GOB/frame is intra coded
                                       as regular 'update'. 2: One GOB every 2 frames is intra coded etc.
                                       In connection with this intra update, restrictions is put on motion vectors
                                       to prevent errors to propagate from the past                                */
    int  infile_header;           //!< If input file has a header set this to the length of the header
    char infile[1000];             //!< YUV 4:2:0 input format
    char outfile[1000];            //!< IVC compressed output bitstream
    char ReconFile[1000];          //!< Reconstructed Pictures
    int intra_period;

    // B pictures
    int successive_Bframe;        //!< number of B frames that will be used
    int qp_B_frm;                  //!< QP of B frames
    int SequenceHeaderType;

    int InterSearch8x8;
    int InterSearch16x8;
    int InterSearch8x16;

    int deblk_disable;
    int Alpha, Beta;

    char PictureTypeSequence[MAXPICTURETYPESEQUENCELEN];

    int aspect_ratio;
    int frame_rate_code;
    //int bit_rate;
    int bit_rate_lower;
    int bit_rate_upper;

    int vec_period;
    int seqheader_period;   // Random Access period though sequence header

    int bbv_buffer_size;
    int hour;
    int minute;
    int second;
    int frame_offset;
    int profile_id;
    int level_id;
    int low_delay;
    int chroma_format;
    int stream_length_flag;
    int fixed_picture_qp;
    int time_flag;
    int fixed_slice_qp;
    int display_horizontal_size;
    int display_vertical_size;
    int slice_parameter;
    int slice_row_nr;
    int output_enc_pic;  //output_enc_pic
    //! Rate Control on IVC standard
    int RCEnable;
    int bit_rate;
    int SeinitialQP;
    int basicunit;
    int channel_type;
    int frame_rate;
    int stuff_height;

#ifdef TDRDO
    int TRDOLength;
#endif

    int  BBS_size;
    char bbv_mode;

    int use_p_sub_type;
    int p_sub_type_delta0;
    int p_sub_type_delta1;
    int p_sub_non_type_coding;    // non-adaptive non-reference P frame coding
    int rdo_q_flag;
    int multiple_hp_flag;
    int abt_enable;
    int if_type;
    int real_ref_num;
    int chroma_enhance;
} InpParams; // InputParameters


typedef struct
{
    int   quant0;                 //!< quant for the first frame
    int   quant1;                 //!< average quant for the remaining frames
    float bitr;                   //!< bit rate for current frame, used only for output til terminal
    float bitr0;                  //!< stored bit rate for the first frame
    float bitrate;                //!< average bit rate for the sequence except first frame
    int   bit_ctr;                //!< counter for bit usage
    int   bit_ctr_0;              //!< stored bit use for the first frame
    int   bit_ctr_n;              //!< bit usage for the current frame
    int   bit_slice;              //!< number of bits in current slice
    int   bit_use_mode_inter[2][MAXMODE]; //!< statistics of bit usage
    int   mode_use_intra[25];     //!< Macroblock mode usage for Intra frames
    int   mode_use_inter[2][MAXMODE];

    int   mb_use_mode[2];

    // B pictures
    int   *mode_use_Bframe;
    int   *bit_use_mode_Bframe;
    int   bit_ctr_P;
    int   bit_ctr_B;
    float bitrate_P;
    float bitrate_B;

#define NUM_PIC_TYPE 5
    int   bit_use_stuffingBits[NUM_PIC_TYPE];
    int   bit_use_mb_type[NUM_PIC_TYPE];
    int   bit_use_header[NUM_PIC_TYPE];
    int   tmp_bit_use_cbp[NUM_PIC_TYPE];
    int   bit_use_coeffY[NUM_PIC_TYPE];
    int   bit_use_coeffC[NUM_PIC_TYPE];
} StatParameters;

extern InpParams *input;
extern StatParameters *stat;
extern SNRParameters *snr;

/*!*******************************************************AEC**********************************!*/
typedef enum
{
    UVLC,
    AEC
} SymbolMode;

/***********************************************************************
 * D a t a    t y p e s   f o r  C A B A C
 ************************************************************************/
//! struct to characterize the state of the arithmetic coding engine
typedef struct
{
    i32u_t  Elow;
    i32u_t  E_s1;
    i32u_t  E_t1;
    uchar_t Ebuffer;
    i32u_t  Ebits_to_go;
    i32u_t  Ebits_to_follow;
    pel_t*  Ecodestrm;
    int*    Ecodestrm_len;
    int     C, E, B;
} Env_AEC;

typedef struct
{
    Bitstream           *bitstream;
    // syntax element number and bit-counters
    int                 bitcounter[MAX_BITCOUNTER_MB];
    MotionInfoContexts  *mot_ctx;
    TextureInfoContexts *tex_ctx;
    Env_AEC ee_AEC;
} CSobj;
typedef CSobj* CSptr;

void  delete_coding_state( CSptr );
CSptr create_coding_state();
void  init_contexts( CSobj *cs_aec );
void  copy_coding_state( CSobj *cs_dst, CSobj *cs_src );

#define store_coding_state(cs_dst) copy_coding_state(cs_dst, img->cs_aec);
#define reset_coding_state(cs_src) copy_coding_state(img->cs_aec, cs_src);


int mv_out_of_range;

// files
FILE *p_stat;                    //!< status file for the last encoding session

int p_dec;   //!< internal decoded image for debugging
int p_in;                      //!< YUV

#if TRACE
FILE *p_trace;                   //!< Trace file
#endif

int  sign( int a,int b );
void init_img();
void report_summary();
void information_init();

int   SATD ( int*, int );

pel_t* FastLineX ( int, pel_t*, int, int );
pel_t* UMVLineX  ( int, pel_t*, int, int );


void LumaResidualCoding ( Macroblock *currMB, CSobj *cs_aec, uchar_t uhBitSize );
void EncodeInterChromaCU( Macroblock *currMB, CSobj *cs_aec, int isBdirect );

int  writeCUHeader( Macroblock *currMB, CSobj *cs_aec, int mb_nr, int bWrite );

extern int*   refbits;
extern int*** motion_cost;

int  LumaResidualCoding8x8_Bfrm     ( Macroblock*, CSobj*, int*, int );

int  writeOneMotionVector      ( Macroblock *currMB, CSobj *cs_aec, int, int, int, int );
int  MHMC_writeOneMotionVector ( Macroblock *currMB, CSobj *cs_aec, int, int, int, int );

int  writeChromaIntraPredMode  ();
int  B8Mode2Value ( int b8mode, int b8pdir );

// dynamic mem allocation
int  init_global_buffers();
void free_global_buffers();
void no_mem_exit  ( char *where );

int  get_mem_mv  ( int****** );
void free_mem_mv ( int***** );
void free_img    ();

#define COF_SIZE_ALL (16 * 4 * 6 * 2) // the last 2 indicates level and run
#define COF_SIZE_LUMA (16 * 4 * 4)    // (4x4+1) * (4x4), 17 is the total number of coefficients in one 4x4 block, and 16 is the number of 4x4 block is one MB

int writeLumaCoeffBlk( Macroblock *currMB, uchar_t uhBitSize, CSobj *cs_aec, int *ACLevel, int *ACRun );
int storeMotionInfo( Macroblock *currMB );

void  init_pu_pos( int mode, int iCuBitSize, int iBlkIdx );

void  update_statistics( Macroblock *currMB, CSobj *cs_aec );

void  CheckAvailabilityOfNeighbors( Macroblock *currMB, int mb_nr );

void error( char *text, int code );
int  start_sequence();
int  terminate_sequence();


void SetImgType();

extern int*** motion_cost_sym;

void  GetBdirectMV_enc( Macroblock *currMB );

//! Syntax element
typedef struct syntaxelement
{
    int    type;             //!< type of syntax element for data part.
    int    value1;           //!< numerical value of syntax element
    int    value2;           //!< for blocked symbols, e.g. run/level
    int    len;              //!< length of code
    int    inf;              //!< info part of UVLC code
    i32u_t bitpattern;       //!< UVLC bitpattern
    int    context;          //!< AEC context
    int    k;                //!< AEC context for coeff_count,uv
    int    golomb_grad;      //needed if type is a golomb element (IVC)
    int    golomb_maxlevels; // if this is zero, do not use the golomb coding. (IVC)

    //!< for mapping of syntaxElement to UVLC
    void( *mapping )( int value1, int value2, int* len_ptr, int* info_ptr );

    //!< edit start for mapping of syntaxElement to UVLC
    void( *writing )( struct syntaxelement *, Env_AEC* );
    //!< edit end for mapping of syntaxElement to UVLC

} SyntaxElement;

//! Slice
typedef struct
{
    int                 start_mb_nr;
} Slice;

typedef struct pix_pos
{
    int available;   //ABCD
    int mb_addr;    //MB position
    int x;
    int y;
    int pos_x;     //4x4 x-pos
    int pos_y;
} PixelPos;

typedef struct
{
    int   no_slices;
    float distortion_y;
    float distortion_u;
    float distortion_v;
    //!EDIT START <added by  AEC
    Slice slices[MAX_SLICE_NUM_IN_PIC];
    //!EDIT end <added by  AEC
} Picture;

Picture *frame_pic;

Picture *malloc_picture();

//! ImgParams
typedef struct
{
    int ip_frm_idx;              //!< I/P frame index
    int ref_num;
    int current_mb_nr;
    int total_number_mb;
    int current_slice_nr;
    int type;

    int frame_qp;                //!< QP for the current frame
    int qp;                      //!< QP for the current macroblock, it is equal to frame_qp, when using the fixed QP settings
    double lambda;
    int framerate;

    int width;                   //!< Number of pels
    int width_cr;                //!< Number of pels chroma
    int height;                  //!< Number of lines
    int height_cr;               //!< Number of lines  chroma
    int iStride;
    int iStrideC;
    int mb_y;                    //!< current MB vertical
    int mb_x;                    //!< current MB horizontal
    int mb_b8_x;
    int mb_b8_y;
    int mb_b4_y;                 //!< current block vertical
    int mb_b4_x;                 //!< current block horizontal
    int mb_pix_y;                //!< current pixel vertical
    int mb_pix_x;                //!< current pixel horizontal
    int mb_pix_y_cr;                 //!< current pixel chroma vertical
    int mb_pix_x_cr;                 //!< current pixel chroma horizontal
    int block_c_x;               //!< current block chroma vertical

    int cu_pix_y;
    int cu_pix_x;
    int cu_bitsize;
    int pu_pix_y;
    int pu_pix_x;
    int pu_width;
    int pu_height;

    int cod_counter;             //!< Current count of number of skipped macroblocks in a row

    pel_t pred_blk_luma[MB_NUM];            // current prediction block for temporary usage
    pel_t pred_blk_chroma[2][MB_NUM];
    int resi_blk[MB_SIZE][MB_SIZE];         // the pixel errors between original image and prediction block

    int coefAC_luma[2][LCU_SIZE*LCU_SIZE];
    int coefAC_chroma[2][2][LCU_SIZE*LCU_SIZE];
    int bst_coefAC_luma[2][LCU_SIZE*LCU_SIZE];
    int bst_coefAC_chroma[2][2][LCU_SIZE*LCU_SIZE];

    Macroblock *mb_data;          //!< array containing all MBs of a whole frame
    int *quad;                    //!< Array containing square values,used for snr computation  */                                         /* Values are limited to 5000 for pixel differences over 70 (sqr(5000)).
    int writeBSflag;
    int poc;
    int imgtr_next_P;
    int imgtr_last_P;
    int imgtr_prev_P;
    int pic_distance;

    // B pictures
    int b_interval;
    int pfrm_interval;
    int b_frame_to_code;

    // block mv: all_mv[b8_x][b8_y][ref][mode][x/y]
    int ** ** *fmv_com;     // forward block mv for common prediction modes
    int ** ** *bmv_com;     // backward block mv for common prediction modes
    int ** ** *pfmv_com;    // forward prediction mv for common prediction modes
    int ** ** *pbmv_com;    // backward prediction mv for common prediction modes

    int ** ** *mv_sym_mhp;  // forward block mv for sym and mhp direction
    int ** ** *bmv_mhp;     // backward block mv for mhp direction, only temporarily stored for predicting
    int ** ** *pmv_sym_mhp;

    int ***pfrm_mv;         //!< motion vector buffer(forward, backward, direct)
    int ***bfrm_fmv;
    int ***bfrm_bmv;

    int **pfrm_ref;         //!< Array for reference frames of each block
    int **bfrm_fref;
    int **bfrm_bref;

    int mv_range_flag;

    int auto_crop_right;
    int auto_crop_bottom;

    int coding_stage;
    int coded_mb_nr;

    int current_slice_start_mb;
    int dropflag;
    int current_mb_nr_fld;

    int currSliceLastMBNr; // the last MB no in current slice.

    int EncodeEnd_flag;
    int count;

    int p_sub_type_start;
    double tag_hp;
    int typeb;

    int real_ref_num;

    int level_SUM_bit;
    int level_fourframe_bit;
    int SUM_bit;
    int Mean_bit;
    int fourframe_bit;
    int p_one_bit;
    int p_three_bit;
    int p_sub_bits;
    int level_p_sub_bits;

    int model_number;
    Slice *currentSlice;                         //!< pointer to current Slice data struct
    CSptr cs_aec;
    int PicWidthInMbs;
    int PicHeightInMbs;
    int PicSizeInMbs;
    i16u_t bbv_delay;
    int picture_coding_type;
    int i_frm_idx;

    int previous_delta_qp;
    uchar_t enc_mb_delta_qp; // encoder Qp searching range (-enc_mb_delta_qp to +enc_mb_delta_qp)
} ImgParams;
extern ImgParams *img;

typedef struct best_cu_info_t
{
    int coef_y[2][LCU_SIZE * LCU_SIZE];
    int coef_u[2][LCU_SIZE * LCU_SIZE / 4];
    int coef_v[2][LCU_SIZE * LCU_SIZE / 4];

    pel_t rec_mbY[LCU_SIZE][LCU_SIZE];
    pel_t rec_mbU[LCU_SIZE / 2][LCU_SIZE / 2];
    pel_t rec_mbV[LCU_SIZE / 2][LCU_SIZE / 2];

    int dQP;
    int QP;
    int cbp;
    int best_weighted_skipmode;

    int mv_info[4][2]; // [blk_id][fwd/bwd]
    int mvd[2][2][2][2];  // [blk_idx][fwd/bwd][x/y]

    int fref[2][2];
    int bref[2][2];

    int left_cu_qp;
    int previous_qp;

    uchar_t uhBitSize;
    int best_mode;
    int best_md_directskip_mode;
    int best_trans_type;
    int best_b8mode[4];
    int best_b8pdir[4];
    int best_c_imode;

    // variables need to be modified or removed !!!
    int best_intra_pred_modes_tmp[16]; // need to be revised to 4
} best_cu_info_t;

Bitstream *tmpStream;
int current_slice_bytepos;

int *****all_mincost;//store the MV and SAD information needed;

// loop-filter
void DeblockMb( ImgParams *img,
                uchar_t *imgY,
                uchar_t *imgU,
                uchar_t *imgV,
                int blk_y,
                int blk_x,
                int current_mb_nr_temp );
void DeblockFrame( ImgParams *img, pel_t *imgY, pel_t *imgU, pel_t *imgV );

void terminate_pic( ImgParams *img, CSobj *cs_aec, Boolean *end_of_picture );
void free_picture ( Picture *pic );
int  encode_one_slice( Picture *pic ); //! returns the number of MBs in the slice
void free_slice_list( Picture *currPic );

void Demulate( Bitstream *currStream, int current_slice_bytepos );
void GetPskipMV_enc( ImgParams *img, Macroblock *currMB );
void SetInterModesAndRef( ImgParams *img, Macroblock *currMB, int mode );
void SetIntraModesAndRef( ImgParams *img, Macroblock *currMB );
void SetModesAndRefframe ( Macroblock *currMB, int b8, int* fw_ref, int* bw_ref );

void  LumaPrediction8x8( ImgParams *img, int, int, int, int, int, int, int );
double EncodeOneCU( ImgParams *img, Macroblock *currMB, CSobj *cs_aec, best_cu_info_t *bst, uchar_t uhBitSize );
int  writeLumaCoeff_AEC( CSobj *cs_aec, int bsize, int *ACLevel, int *ACRun );
int  writeChromaCoeff_AEC( CSobj *cs_aec, int isIntra, int bsize, int *ACLevel, int *ACRun );
int  terminate_slice( CSobj *cs_aec );

int  seq_header;
int  slice_header[3];
#endif
