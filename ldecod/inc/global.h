/*
*************************************************************************************
* File name: global.h
* Function:  global definitions for for IVC decoder.
*
*************************************************************************************
*/

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <stdio.h>                              //!< for FILE
#include "defines.h"

//! Boolean Type
typedef enum
{
    FALSE,
    TRUE
} Boolean;


uchar_t *reference_frame[33][3];
uchar_t *p_ref_frm[33][3]; // only a pointer

uchar_t **b_ref_frm, **f_ref_frm;

uchar_t *p_mref[33];
uchar_t *p_mcef[33][2];

uchar_t *p_cur_frm[3];
uchar_t *imgY_rec; //reconstructed pixels
uchar_t *imgU_rec;
uchar_t *imgV_rec;

uchar_t *img_prev; // only will be used in calculating PSNR

// B pictures
int Bframe_ctr;

#define ET_SIZE 300      //!< size of error text buffer
char errortext[ET_SIZE]; //!< buffer for error message for exit with error()

int  bValidSyntax;
int  StartCodeValue;

//! definition of IVC syntax elements
typedef enum
{
    SE_HEADER,
    SE_MBTYPE,
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
    SE_MAX_ELEMENTS //!< number of maximum syntax elements, this MUST be the last one!
} SE_type;        // substituting the definitions in element.h

typedef struct
{
    i32u_t    Elow;
    i32u_t    E_s1;
    i32u_t    E_t1;
    i32u_t    value_s;
    i32u_t    value_t;
    i32u_t    Dbuffer;
    int       bits_to_go;
    uchar_t   *Dcodestrm;
    int       *Dcodestrm_len;
} Env_AEC;

struct img_par;
struct stat_par;

// struct of bbv buffer
typedef struct BbvBuffer_t
{
    char  bbv_mode;           // 0:not 0xFFFF, 1:0xFFFF
    int   bitrate;            // bit per second (bit/s)
    float framerate;          // frame per second (frame/s)
    float bbv_delay;          //(s)
    float low_delay;          //
    int   BBS_size;           //
    float frmout_interval;    //
    int   currFrm_max_bit;    //
    int   currFrm_min_bit;    //
    int   frm_no;             //
    int   check_continue;     //
    int   frame_code_bits;    //
    int   *FrameBits;         //
} BbvBuffer_t;

int  pminBBSsize;
int  pminFsize;
int  pbbv_mode;
int  pbitrate;

typedef struct StatBits
{
    int   curr_frame_bits;
    int   prev_frame_bits;
    int   emulate_bits;
    int   prev_emulate_bits;
    int   last_unit_bits;
    int   bitrate;
    int   total_bitrate[1000];
    int   coded_pic_num;
    int   time_s;
} StatBits;

typedef struct syntaxelement
{
    int           type;                  //!< type of syntax element for data part.
    int           value1;                //!< numerical value of syntax element
    int           value2;                //!< for blocked symbols, e.g. run/level
    int           len;                   //!< length of code
    int           inf;                   //!< info part of UVLC code
    i32u_t  bitpattern;            //!< UVLC bitpattern
    int           context;               //!< AEC context
    int           k;                     //!< AEC context for coeff_count,uv
    int           golomb_grad;           //!< Needed if type is a golomb element
    int           golomb_maxlevels;      //!< If this is zero, do not use the golomb coding
#if TRACE
#define       TRACESTRING_SIZE 100           //!< size of trace string
    char          tracestring[TRACESTRING_SIZE]; //!< trace string
#endif

    //! for mapping of UVLC to syntaxElement
    void    ( *mapping )( int len, int info, int *value1 );
    //! used for AEC: refers to actual coding method of each individual syntax element type
    void  ( *reading )( struct syntaxelement *, struct img_par *, Env_AEC * );

} SyntaxElement;

//! Macroblock
typedef struct macroblock
{
    int slice_nr;

    // some storage of macroblock syntax elements for global access
    int mb_type;
    int mb_trans_type;
    int mvd[2][BLOCK_MULTIPLE][BLOCK_MULTIPLE][2];      //!< indices correspond to [forw,backw][block_y][block_x][x,y]
    int cbp;

    int b8mode[4];
    int b8pdir[4];
    int sub_mb_trans_type[4];
    int fw_ref[4];

    int c_ipred_mode;         //!< chroma intra prediction mode
    int mb_qp_delta;
    struct macroblock   *mb_available_up;
    struct macroblock   *mb_available_left;
    struct macroblock   *mb_available_rightup;
    struct macroblock   *mb_available_leftup;
    int mb_addr_left, mb_addr_up, mb_addr_rightup, mb_addr_leftup;
    int mbAvailA, mbAvailB, mbAvailC, mbAvailD;
} Macroblock;

//! Bitstream
typedef struct
{
    int bits_to_go;
    int read_len;           //!< actual position in the code-buffer, AEC only
    int bs_length;          //!< over code-buffer length, byte oriented,
    int byte_offset;        //!< actual position in the code-buffer, bit-oriented, UVLC only
    uchar_t *streamBuffer;  //!< actual code-buffer for read bytes
} Bitstream;

void FreeBitstream( Bitstream *bitstr );
Bitstream *AllocateBitstream();

//! CSobj
typedef struct
{
    Bitstream           *bitstream;
    Env_AEC de_AEC;
    MotionInfoContexts  *mot_ctx;     //!< pointer to struct of context models for use in AEC
    TextureInfoContexts *tex_ctx;     //!< pointer to struct of context models for use in AEC
} CSobj;

typedef struct pix_pos
{
    int available;   //ABCD
    int mb_addr;    //MB position
    int x;
    int y;
    int pos_x;     //4x4 x-position
    int pos_y;
} PixelPos;

typedef enum
{
    UVLC,
    AEC
} SymbolMode;


// image parameters
typedef struct img_par
{
    int ip_frm_idx;                             //!< I/P frame index
    int FrmNum;
    int current_mb_nr;                          // bitstream order
    int max_mb_nr;
    int current_slice_nr;
    int poc;                                    //<! picture order count, i.e. the display order
    int qp;                                     //<! quant for the current frame
    int type;                                   //<! image type INTER/INTRA
    int typeb;
    int width;
    int height;
    int width_cr;                               //<! width chroma
    int height_cr;                              //<! height chroma
    int iStride;
    int iStrideC;
    int width_org;
    int height_org;

    int mb_y;
    int mb_x;

    int mb_b8_y, mb_b8_x;
    int mb_b4_y, mb_b4_x;
    int mb_pix_y, mb_pix_x;
    int mb_pix_y_cr, mb_pix_x_cr;
    int pu_pix_x, pu_pix_y;

    pel_t pred_blk_luma[MB_NUM];                     //<! predicted block, 16x16
    pel_t pred_blk_chroma[2][MB_NUM];
    int coefAC_luma[2][LCU_NUM];
    int coefAC_chroma[2][2][LCU_NUM];

    int bmv_mhp[2][2][2];
    int resi_blk[16][16];                       //<! final 4x4 block. Extended to 16x16 for ABT
    int **ipredmode;                            //<! prediction type
    int tab_sqr[256];

    int ***pfrm_mv;
    int ***bfrm_fmv;
    int ***bfrm_bmv;

    int **pfrm_ref;
    int **bfrm_fref;
    int **bfrm_bref;

    unsigned int imgtr_next_P;
    unsigned int imgtr_last_P;
    unsigned int tr_frm;
    unsigned int imgtr_prev_P;

    int no_forward_reference;

    int real_ref_num;

    // B pictures
    int direct_type;
    int writeBSflag;
    i32u_t pic_distance;

    int PicWidthInMbs;
    int PicHeightInMbs;
    int PicSizeInMbs;
    int types;

    int Bframe_number;

    CSobj       *cs_aec;                  //<! pointer to current CSobj data struct
    int new_seq_header_flag;
    int new_sequence_flag;
    int last_pic_bbv_delay;               //<! last picture's bbv_delay
    int sequence_end_flag;                //<! ITM_r2

    int current_slice_header_flag;
    Macroblock *mb_data;
    int p_subtype;

    int fixed_frame_level_qp;
    int fixed_slice_qp;
    int frame_qp;
    int slice_qp;

    int deblk_disable;
    int Alpha, Beta;

    int previous_delta_qp;
} ImgParams;

extern ImgParams *img;
extern Bitstream *currStream;

// signal to noice ratio parameters
typedef struct
{
    float snr_y;                      //<! current Y SNR
    float snr_u;                      //<! current U SNR
    float snr_v;                      //<! current V SNR
    float snr_sum_y;                  //<! Average SNR Y(dB) remaining frames
    float snr_sum_u;                  //<! Average SNR U(dB) remaining frames
    float snr_sum_v;                  //<! Average SNR V(dB) remaining frames
} snr_par;

int tot_time;

// input parameters from configuration file
typedef struct
{
    char infile[1000];                //<! IVC input
    char outfile[1000];               //<! Decoded YUV 4:2:0 output
    char reffile[1000];               //<! Optional YUV 4:2:0 reference file for SNR measurement
    int FileFormat;                   //<! File format of the Input file, PAR_OF_ANNEXB or PAR_OF_RTP
    int yuv_structure;                //<! Specifies reference frame's yuv structure
    int check_BBV_flag;               //<! Check BBV buffer (0: disable, 1: enable)
    bool bwrite_dec_frm;               //<! if write out the decoded reconstructed frame
    int abt_enable;
    int if_type;
} inp_params;

extern inp_params *input;

// files
FILE *p_out;                         //<! pointer to output YUV file
FILE *p_ref;                         //<! pointer to input original reference YUV file file

#if TRACE
FILE *p_trace;
#endif

void read_intra_luma_mode( ImgParams *img, int b8, int b4 );

void init_img_params ( ImgParams *img, int mb );

// prototypes
void init_conf( inp_params *inp,int numpar,char **config_str );
void report_summary( inp_params *inp, ImgParams *img, snr_par *snr );
void find_snr( snr_par *snr,ImgParams *img, FILE *p_ref, uchar_t *img_prev );

int  decode_one_frame( ImgParams *img,inp_params *inp, snr_par *snr );
void init_frame( ImgParams *img, inp_params *inp, snr_par *snr );
void init_frame_buffer();

void write_frame( ImgParams *img, FILE *p_out );
void write_prev_Pframe( ImgParams *img,FILE *p_out, uchar_t *pDst ); // B pictures


int  read_new_slice();
void decode_one_slice( ImgParams *img,inp_params *inp );
void picture_data( ImgParams *img );
void init_mb_params( Macroblock *currMB );
int  read_one_macroblock( ImgParams *img, Macroblock *currMB );
int  decode_one_cu( ImgParams *img, Macroblock *currMB, uchar_t uhBitSize );

int  sign( int a , int b );

int find_headers();

void DeblockFrame( ImgParams *img, uchar_t *pic_y, uchar_t *pic_u, uchar_t *pic_v );

// Direct interpolation
void CheckAvailabilityOfNeighbors( ImgParams *img, Macroblock *currMB );

void error( char *text, int code );

// dynamic mem allocation
int  init_global_buffers( inp_params *inp, ImgParams *img );
void free_global_buffers( ImgParams *img );

void Update_Picture_Buffers();

int get_direct_mv ( int****** mv,int mb_x,int mb_y );
void free_direct_mv ( int***** mv,int mb_x,int mb_y );

int *****direct_mv; // only to verify result
int ipdirect_x,ipdirect_y;
int demulate_enable;  // prevent the emulation of start code

int bbv_buffer_size;
int currentbitoffset;
int chroma_format;
int profile_id;
int level_id;
int horizontal_size;
int vertical_size;
int chroma_format;
int sample_precision;
int aspect_ratio;
int frame_rate_code;
int bit_rate;
int low_delay;
int bit_rate_lower;
int bit_rate_upper;

int eos;
int pre_img_type;
int pre_img_types;
int pre_img_tr;
int pre_img_qp;
int pre_tmp_time;
snr_par *snr;

/* I_pictures_header() */
int stream_length_flag;
int stream_length;
int picture_decode_order_flag;
int picture_decode_order;
int time_flag;
int time_code;

int bby_delay;
int hour;
int minute;
int sec;
int frame_offset;

/* Pb_picture_header() */
int picture_coding_type;
int bbv_delay;
int bbv_delay_extension;
int  marker_bit;

/* slice_header() */
int start_code_prefix;
int slice_vertical_position;
int slice_vertical_position_extension;
int slice_start_code;
int next_start_code_pos;

#define SLICE_START_CODE_MIN    0x00
#define SLICE_START_CODE_MAX    0xAF
#define SEQUENCE_HEADER_CODE    0xB0
#define SEQUENCE_END_CODE       0xB1
#define USER_DATA_START_CODE    0xB2
#define I_PICTURE_START_CODE    0xB3
#define PB_PICTURE_START_CODE   0xB6
#define VIDEO_EDIT_CODE         0xB7


#define SEQUENCE_DISPLAY_EXTENSION_ID            2
#define COPYRIGHT_EXTENSION_ID                   4
#define CAMERAPARAMETERS_EXTENSION_ID            11
#define PICTURE_DISPLAY_EXTENSION_ID             7

void malloc_slice( inp_params *inp, ImgParams *img );
int  read_terminating_bit( ImgParams *img, int eos_bit );
void free_slice( ImgParams *img );

int b_pre_dec_intra_img;
int pre_dec_img_type;
int slice_horizontal_positon;
int slice_horizontal_positon_extension;
#endif