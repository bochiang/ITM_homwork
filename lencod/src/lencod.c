#include "contributors.h"

#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include <IO.H>
#include <assert.h>

#include "global.h"
#include "configfile.h"
#include "memalloc.h"
#include "image.h"
#include "header.h"

#include "vlc.h"
#include "bitstream.h"

#include "block.h"

#ifdef FastME
#include "fast_me.h"
#endif

#include "x86/intrinsic.h"

#ifdef TDRDO
#include "tdrdo.h"
#endif

#ifdef RATECONTROL
#include "fuzzycontrol.h"
#endif

InpParams inputs, *input = &inputs;
ImgParams images, *img   = &images;
SNRParameters  snrs, *snr  = &snrs;
StatParameters stats,*stat = &stats;

BbvBuffer_t* pBbv = NULL;

void Init_Motion_Search_Module ();
void Clear_Motion_Search_Module ();

void xInit_seq()
{
    float FrameRate[8] = { { 24000 / 1001 }, { 24 }, { 25 }, { 30000 / 1001 }, { 30 }, { 50 }, { 60000 / 1001 }, { 60 } };

    int auto_pad_right, auto_pad_bottom;
    if (input->width_org% MB_SIZE != 0)
    {
        auto_pad_right = MB_SIZE - (input->width_org % MB_SIZE);
    }
    else
    {
        auto_pad_right = 0;
    }

    if (input->height_org % MB_SIZE != 0)
    {
        auto_pad_bottom = MB_SIZE - (input->height_org % MB_SIZE);
    }
    else
    {
        auto_pad_bottom = 0;
    }

    img->width = input->width_org + auto_pad_right;
    img->height = input->height_org + auto_pad_bottom;
    img->width_cr = img->width / 2;
    img->height_cr = img->height / 2;
    img->iStride = img->width + 2 * IMG_PAD_SIZE;
    img->iStrideC = img->iStride >> 1;
    img->PicWidthInMbs = img->width / MB_SIZE;
    img->PicHeightInMbs = img->height / MB_SIZE;
    img->PicSizeInMbs = img->PicWidthInMbs * img->PicHeightInMbs;

    img->framerate = (int)FrameRate[input->frame_rate_code - 1];

    input->abt_enable = 1;
}


/**************************************************************************
* Function:Main function for encoder.
* Input:argc : number of command line arguments
*       argv : command line arguments
* Return: exit code
**************************************************************************/
int main( int argc,char **argv )
{
    int len = 0;
    int ip_frm_num;  // number of I/P frames
#ifdef RATECONTROL
    RateControl RC;
#endif

    p_dec = -1;
    p_stat = NULL;
#if TRACE
    p_trace = NULL;
#endif

    // for statistics
    seq_header = 0;
    slice_header[0] = slice_header[1] = slice_header[2] = 0;

    // get input-struct info from the configure file
    Configure( argc, argv );

    CheckToolsInProfile();

    // add sse tool
    com_funs_init_ip_filter();
    com_funs_init_intra_pred();
    com_funs_init_deblock_filter();
    com_funs_init_forquant();
    com_funs_init_dct();
    com_funs_init_idct();
    com_funs_init_pixel_opt();

#if ENABLE_SSE
    com_init_intrinsic();
#endif
#if ENABLE_AVX2
    //com_init_intrinsic_256();
#endif

    xInit_seq();
    init_img();

    ip_frm_num = ( input->FrmsToBeEncoded + input->successive_Bframe - 1 ) / ( input->successive_Bframe + 1 ) + 1;

    //structure for bitstream of one picture
    frame_pic = malloc_picture();
#if MVRANGE
    srand((i32u_t)time(NULL));
    input->search_range = (rand() % (512 - 16)) + 16;
#endif
    init_global_buffers();

    Init_Motion_Search_Module();

    information_init();

    if ( input->usefme )
    {
        DefineThreshold();
    }

    // B pictures
    Bframe_ctr = 0;
    tot_time = 0;                 // time for total encoding session

#ifdef TDRDO
    OMCPDList = RealDList = NULL;
    porgF = ppreF = precF = prefF = NULL;
    KappaTable = NULL;
    Gop_size_all = input->successive_Bframe == 0 ? 4 : 8;
    StepLength = input->successive_Bframe ? input->successive_Bframe + 1 : 1;

    if ( input->TRDOLength != 0 )
    {
        OMCPDList = CreatDistortionList( input->FrmsToBeEncoded / StepLength + 1, input->width_org, input->height_org, WORKBLOCKSIZE, MAXBLOCKSIZE );
        RealDList = CreatDistortionList( input->FrmsToBeEncoded / StepLength + 1, input->width_org, input->height_org, WORKBLOCKSIZE, MAXBLOCKSIZE );
        porgF = ( Frame * )calloc( 1, sizeof( Frame ) );
        ppreF = ( Frame * )calloc( 1, sizeof( Frame ) );
        precF = ( Frame * )calloc( 1, sizeof( Frame ) );
        prefF = ( Frame * )calloc( 1, sizeof( Frame ) );
        porgF->FrameWidth = input->width_org;
        porgF->FrameHeight = input->height_org;
        porgF->nStrideY = input->width_org;
        porgF->nStrideC = input->width_org / 2;
        *ppreF = *precF = *prefF = *porgF;
    }
#endif

#ifdef RATECONTROL
    pRC = &RC;
    memset( pRC, 0, sizeof( RateControl ) );
    {
        int i;
        int IniQP = input->qp_1st_frm;
        char filename[64], *ch = NULL;

        input->SeinitialQP = input->RCEnable == 1 ? IniQP : input->SeinitialQP;
        gop_size_all = input->successive_Bframe ? input->successive_Bframe + 1 : ( input->use_p_sub_type ? 4 : 1 );

        for ( i = (int)strlen( input->infile ); i >= 0 && input->infile[i] != '\\'; ch = &input->infile[i--] )
        {
            ;
        }

        strncpy( filename, ch, sizeof( filename ) );

        ch = filename;
        while ( *ch != '\0' && *ch != '.' )
        {
            ch++;
        }
        *ch = '\0';

        Init_RateControl( pRC, input->RCEnable, input->FrmsToBeEncoded, input->intra_period, input->bit_rate, img->framerate, IniQP, input->width_org, input->height_org );
    }
#endif

    // Write sequence header
    stat->bit_use_header[3] = start_sequence();
    printf( "Sequence Header \n" );

    img->tag_hp = 0;
    img->p_sub_type_start = img->width / 8 * img->height / 8;

    img->EncodeEnd_flag = 0;

    for ( img->ip_frm_idx = 0; img->ip_frm_idx < ip_frm_num; img->ip_frm_idx++ )
    {
        //write sequence header to bitstream before random access point
        if ( img->ip_frm_idx != 0
                && input->seqheader_period != 0
                && img->ip_frm_idx % ( input->seqheader_period*input->intra_period ) == 0 )
        {
            stat->bit_use_header[3] += terminate_sequence();

            //write video edit code when the following bitstream can be edited independently
            if ( img->ip_frm_idx != 0 && input->vec_period != 0 && img->ip_frm_idx % ( input->vec_period*input->seqheader_period*input->intra_period ) == 0 )
            {
                len = WriteVideoEditCode();
                stat->bit_use_header[3] += len;
            }

            stat->bit_use_header[3] += start_sequence();
            printf( "Sequence Header \n" );

            img->ref_num = 0;
        }

        SetImgType();
        stat->bit_use_header[img->type] += len;

        encode_one_frame(); // encode one I- or P-frame

        img->ref_num += 1;
        img->ref_num = MIN( img->ref_num, input->inp_ref_num );

        if ( ( input->successive_Bframe != 0 ) && img->ip_frm_idx > 0 ) // B-frame(s) to encode
        {
            int no_b_frames;
            no_b_frames = MIN( input->successive_Bframe, input->FrmsToBeEncoded + input->successive_Bframe - img->ip_frm_idx * ( input->successive_Bframe + 1 ) - 1 ); //consider the last Pb...bP
            img->type = B_IMG;  // set image type to B-frame

            for ( img->b_frame_to_code = 1; img->b_frame_to_code <= no_b_frames; img->b_frame_to_code++ )
            {
                encode_one_frame();  // encode one B-frame
            }
        }
    }

#ifdef TDRDO
    if ( input->TRDOLength != 0 )
    {
        DestroyDistortionList( OMCPDList );
        DestroyDistortionList( RealDList );
        if ( KappaTable )
        {
            free( KappaTable );
        }
        if ( porgF )
        {
            free( porgF );
        }
        if ( ppreF )
        {
            free( ppreF );
        }
        if ( precF )
        {
            free( precF );
        }
        if ( prefF )
        {
            free( prefF );
        }
    }
#endif

#ifdef RATECONTROL
    if ( pRC->RConoff )
    {
        input->qp_1st_frm = pRC->qp0;
        input->qp_P_frm = pRC->qpN;
        input->qp_B_frm = pRC->qpB;
    }
#endif

    img->EncodeEnd_flag = 1;

    terminate_sequence();

    //bit, for the last sequence end
    if ( !( input->seqheader_period != 0 && img->ip_frm_idx % ( input->seqheader_period*input->intra_period ) == 0
            && ( input->successive_Bframe == 0 || ( img->b_frame_to_code - 1 ) == input->successive_Bframe ) ) )
    {
        if ( img->type == I_IMG )
        {
            stat->bit_ctr_0 += 32;
        }
        else if ( img->type == P_IMG )
        {
            stat->bit_ctr_P += 32;
        }
        else
        {
            stat->bit_ctr_B += 32;
        }
        stat->bit_use_stuffingBits[3] += 32;
    }

    _close( p_in );

    if ( p_dec && input->output_enc_pic )
    {
        _close( p_dec );
    }

#if TRACE
    if ( p_trace )
    {
        fclose( p_trace );
    }
#endif

    Clear_Motion_Search_Module();

    // report everything
    report_summary();

    free_picture( frame_pic );

    free_global_buffers();

    // free image mem
    free_img();

    return 0;
}

/*
*************************************************************************
* Function:Initializes the Image structure with appropriate parameters.
* Input:Input Parameters struct inp_par *inp
* Output:Image Parameters struct img_par *img
* Return:
* Attention:
*************************************************************************
*/


void init_img()
{
    int i;

    img->pfrm_interval = input->successive_Bframe + 1;

    if ( input->slice_row_nr==0 ) // set only one slice in one frame
    {
        input->slice_row_nr=img->height/MB_SIZE;
    }

#if MULSILICE
    srand ( ( i32u_t )time( NULL ) );
    input->slice_row_nr = img->height / MB_SIZE / ( ( rand()%10 ) + 1 ); // 0 ~ 10 SLICEs, SLICE number: (rand()%10) + 1
#endif

    get_mem_mv ( &( img->pfmv_com ) ); //forward mv predictors
    get_mem_mv ( &( img->pbmv_com ) ); //backward mv predictors
    get_mem_mv ( &( img->fmv_com ) ); //forward motion vectors
    get_mem_mv ( &( img->bmv_com ) ); //backward motion vectors

    get_mem_mv( &( img->bmv_mhp ) ); //forward motion vectors for multi-hypothesis mc in P frames

    get_mem_mv ( &( img->pmv_sym_mhp ) ); //motion vector predictors for forward symmetric mode
    get_mem_mv ( &( img->mv_sym_mhp ) ); //motion vectors for forward symmetric mode

    if ( ( img->quad = ( int* )calloc ( 511, sizeof( int ) ) ) == NULL )
    {
        no_mem_exit ( "init_img: img->quad" );
    }

    img->quad+=255;

    for ( i=0; i < 256; ++i )
    {
        img->quad[i]=img->quad[-i]=i*i;
    }

    if( ( ( img->mb_data ) = ( Macroblock * ) calloc( ( img->width/MB_SIZE ) * ( img->height/MB_SIZE ),sizeof( Macroblock ) ) ) == NULL )
    {
        no_mem_exit( "init_img: img->mb_data" );
    }

    for ( i=0; i < ( img->width/MB_SIZE ) * ( img->height/MB_SIZE ); i++ )
    {
        img->mb_data[i].slice_nr = 0;
    }

    img->previous_delta_qp = 0;
    img->enc_mb_delta_qp = 0;
}

/*
*************************************************************************
* Function:Free the Image structures
* Input:Image Parameters struct img_par *img
* Output:
* Return:
* Attention:
*************************************************************************
*/

void free_img ()
{
    free_mem_mv ( img->pfmv_com );
    free_mem_mv ( img->pbmv_com );
    free_mem_mv ( img->fmv_com );
    free_mem_mv ( img->bmv_com );

    free_mem_mv ( img->bmv_mhp );
    free_mem_mv ( img->pmv_sym_mhp );
    free_mem_mv ( img->mv_sym_mhp );

    free ( img->quad-255 );
}

/*
*************************************************************************
* Function:Allocates the picture structure along with its dependent
data structures
* Input:
* Output:
* Return: Pointer to a Picture
* Attention:
*************************************************************************
*/

Picture *malloc_picture()
{
    Picture *pic;

    if ( ( pic = ( Picture * )calloc( 1, sizeof ( Picture ) ) ) == NULL )
    {
        no_mem_exit( "malloc_picture: Picture structure" );
    }

    return pic;
}

/*
*************************************************************************
* Function:Frees a picture
* Input:pic: POinter to a Picture to be freed
* Output:
* Return:
* Attention:
*************************************************************************
*/

void free_picture( Picture *pic )
{
    if ( pic != NULL )
    {
        free ( pic );
    }
}
/*
*************************************************************************
* Function:Reports the gathered information to appropriate outputs
* Input:  struct inp_par *inp,                                            \n
struct img_par *img,                                            \n
struct stat_par *stat,                                          \n
struct stat_par *stat
* Output:
* Return:
* Attention:
*************************************************************************
*/

void report_summary()
{
    int bit_use[2][2] ;
    int i,j;
    int bit_use_Bframe=0;
    int total_bits;
    float frame_rate;
    float mean_motion_info_bit_use[2];

    int no_IPframes = ( input->FrmsToBeEncoded + input->successive_Bframe - 1 ) / ( input->successive_Bframe + 1 ) + 1;

    bit_use[0][0]=1;
    bit_use[1][0]=MAX( 1,no_IPframes-1 );

    //  Accumulate bit usage for inter and intra frames
    bit_use[0][1]=bit_use[1][1]=0;

    for ( i=0; i < 11; i++ )
    {
        bit_use[1][1] += stat->bit_use_mode_inter[0][i];
    }

    for ( j=0; j<2; j++ )
    {
        bit_use[j][1]+=stat->bit_use_header[j];
        bit_use[j][1]+=stat->bit_use_mb_type[j];
        bit_use[j][1]+=stat->tmp_bit_use_cbp[j];
        bit_use[j][1]+=stat->bit_use_coeffY[j];
        bit_use[j][1]+=stat->bit_use_coeffC[j];
        bit_use[j][1]+=stat->bit_use_stuffingBits[j];
    }

    // B pictures
    if( Bframe_ctr!=0 )
    {
        bit_use_Bframe=0;
        for( i=0; i<11; i++ )
        {
            bit_use_Bframe += stat->bit_use_mode_inter[1][i];
        }
        bit_use_Bframe += stat->bit_use_header[2];
        bit_use_Bframe += stat->bit_use_mb_type[2];
        bit_use_Bframe += stat->tmp_bit_use_cbp[2];
        bit_use_Bframe += stat->bit_use_coeffY[2];
        bit_use_Bframe += stat->bit_use_coeffC[2];
        bit_use_Bframe +=stat->bit_use_stuffingBits[2];

        stat->bitrate_P=( stat->bit_ctr_0+stat->bit_ctr_P )*( float )( img->framerate/( input->successive_Bframe+1 ) )/no_IPframes;
        stat->bitrate_B=( stat->bit_ctr_B )*( float )( img->framerate/( input->successive_Bframe+1 ) )*input->successive_Bframe/Bframe_ctr;
    }

    fprintf( stdout,"-----------------------------------------------------------------------------\n" );
    fprintf( stdout,   " Freq. for encoded bitstream       : %1.0f\n",( float )( img->framerate*( input->successive_Bframe+1 ) )/( float )( input->successive_Bframe+1 ) );
    if( input->hadamard )
    {
        fprintf( stdout," Hadamard transform                : Used\n" );
    }
    else
    {
        fprintf( stdout," Hadamard transform                : Not used\n" );
    }


    fprintf( stdout," Image (Encoding) format             : %dx%d\n",img->width,img->height );
    fprintf(stdout, " Image (Recon) format                : %dx%d\n", input->width_org, input->height_org);

    if( input->intra_upd )
    {
        fprintf( stdout," Error robustness                  : On\n" );
    }
    else
    {
        fprintf( stdout," Error robustness                  : Off\n" );
    }
    fprintf( stdout,    " Fast Motion Estimation            : %s\n",input->usefme ? "On":"Off" );
    fprintf( stdout,    " Search range                      : %d\n",input->search_range );


    fprintf( stdout,   " Num of ref. frames used in P pred : %d\n",input->inp_ref_num );
    if( input->successive_Bframe != 0 )
    {
        fprintf( stdout,   " Num of ref. frames used in B pred : %d\n",input->inp_ref_num );
    }


    fprintf( stdout,   " Total encoding time for the seq.  : %.3f sec \n",tot_time*0.001 );

    // B pictures
    fprintf( stdout, " Sequence type                     :" );

    if( input->successive_Bframe==1 )   fprintf( stdout, " IBPBP (QP: I %d, P %d, B %d) \n",
                input->qp_1st_frm, input->qp_P_frm, input->qp_B_frm );
    else if( input->successive_Bframe==2 ) fprintf( stdout, " IBBPBBP (QP: I %d, P %d, B %d) \n",
                input->qp_1st_frm, input->qp_P_frm, input->qp_B_frm );
    else if( input->successive_Bframe==0 && input->intra_period!=1 ) fprintf( stdout, " IPPP (QP: I %d, P %d) \n",
                input->qp_1st_frm, input->qp_P_frm ); //ITM
    else if( input->successive_Bframe==0 && input->intra_period==1 ) fprintf( stdout, " IPPP (QP: I %d) \n",
                input->qp_1st_frm );             //ITM

    fprintf( stdout,"------------------ Average data all frames  ---------------------------------\n" );
    fprintf( stdout," SNR Y(dB)                         : %5.2f\n",snr->snr_ya );
    fprintf( stdout," SNR U(dB)                         : %5.2f\n",snr->snr_ua );
    fprintf( stdout," SNR V(dB)                         : %5.2f\n",snr->snr_va );

    if( Bframe_ctr!=0 )
    {
        fprintf( stdout, " Total bits                        : %d (I %5d, P %5d, B %d) \n",
                 total_bits=stat->bit_ctr_P + stat->bit_ctr_0 + stat->bit_ctr_B, stat->bit_ctr_0, stat->bit_ctr_P, stat->bit_ctr_B );

        frame_rate = ( float )( img->framerate *( input->successive_Bframe + 1 ) ) / ( float ) ( input->successive_Bframe+1 );
        stat->bitrate= ( ( float ) total_bits * frame_rate )/( ( float )( input->FrmsToBeEncoded ) );

        fprintf( stdout, " Bit rate (kbit/s)  @ %2.2f Hz     : %5.2f\n", frame_rate, stat->bitrate/1000 );
    }
    else
    {
        fprintf( stdout, " Total bits                        : %d (I %5d, P %5d) \n",
                 total_bits=stat->bit_ctr_P + stat->bit_ctr_0 , stat->bit_ctr_0, stat->bit_ctr_P );

        frame_rate = ( float )img->framerate / ( ( float ) ( input->successive_Bframe + 1 ) );
        stat->bitrate= ( ( float ) total_bits * frame_rate )/( ( float ) input->FrmsToBeEncoded );

        fprintf( stdout, " Bit rate (kbit/s)  @ %2.2f Hz     : %5.2f\n", frame_rate, stat->bitrate/1000 );
    }

    fprintf( stdout,"-----------------------------------------------------------------------------\n" );
    fprintf( stdout,"Exit ITM %s encoder ver %s ", ITM, VERSION );
    fprintf( stdout,"\n" );

    // status file
    if ( ( p_stat=fopen( "stat.dat","at" ) )==0 )
    {
        fprintf(stderr, "Error open file %s", "stat.dat");
        error( errortext, 500 );
    }

    fprintf( p_stat,"\n ------------------ Average data all frames  ------------------------------\n" );
    fprintf( p_stat," SNR Y(dB)                         : %5.2f\n",snr->snr_ya );
    fprintf( p_stat," SNR U(dB)                         : %5.2f\n",snr->snr_ua );
    fprintf( p_stat," SNR V(dB)                         : %5.2f\n",snr->snr_va );
    fprintf( p_stat, " Total bits                        : %d (I %5d, P BS %5d, B %d) \n",
             total_bits=stat->bit_ctr_P + stat->bit_ctr_0 + stat->bit_ctr_B, stat->bit_ctr_0, stat->bit_ctr_P, stat->bit_ctr_B );

    fprintf( p_stat, " Bit rate (kbit/s)  @ %2.2f Hz     : %5.2f\n", frame_rate, stat->bitrate/1000 );
    fprintf( p_stat," -------------------------------------------------------------- \n" );
    fprintf( p_stat,"  This file contains statistics for the last encoded sequence   \n" );
    fprintf( p_stat," -------------------------------------------------------------- \n" );
    fprintf( p_stat,   " Sequence                     : %s\n",input->infile );
    fprintf( p_stat,   " No.of coded pictures         : %4d\n",input->FrmsToBeEncoded );
    fprintf( p_stat,   " Freq. for encoded bitstream  : %4.0f\n",frame_rate );

    // B pictures
    if( input->successive_Bframe != 0 )
    {
        fprintf( p_stat,   " IPFrame Bitrate(kb/s)      : %6.2f\n", stat->bitrate_P/1000 );
        fprintf( p_stat,   " BFrame Bitrate(kb/s)  : %6.2f\n", stat->bitrate_B/1000 );
    }
    else
    {
        fprintf( p_stat,   " Bitrate(kb/s)                : %6.2f\n", stat->bitrate/1000 );
    }

    if( input->hadamard )
    {
        fprintf( p_stat," Hadamard transform           : Used\n" );
    }
    else
    {
        fprintf( p_stat," Hadamard transform           : Not used\n" );
    }

    fprintf( p_stat,  " Image format                 : %dx%d\n",img->width,img->height );

    if( input->intra_upd )
    {
        fprintf( p_stat," Error robustness             : On\n" );
    }
    else
    {
        fprintf( p_stat," Error robustness             : Off\n" );
    }

    fprintf( p_stat,  " Search range                 : %d\n",input->search_range );


    fprintf( p_stat,   " No of frame used in P pred   : %d\n",input->inp_ref_num );
    if( input->successive_Bframe != 0 )
    {
        fprintf( p_stat, " No of frame used in B pred   : %d\n",input->inp_ref_num );
    }

    fprintf( p_stat,   " Entropy coding method        : AEC\n" );

    fprintf( p_stat," Search range restrictions    : none\n" );

    fprintf( p_stat," -------------------|---------------|---------------|\n" );
    fprintf( p_stat,"     Item           |     Intra     |   All frames  |\n" );
    fprintf( p_stat," -------------------|---------------|---------------|\n" );
    fprintf( p_stat," SNR Y(dB)          |" );
    fprintf( p_stat," %5.2f         |",snr->snr_y1 );
    fprintf( p_stat," %5.2f         |\n",snr->snr_ya );
    fprintf( p_stat," SNR U/V (dB)       |" );
    fprintf( p_stat," %5.2f/%5.2f   |",snr->snr_u1,snr->snr_v1 );
    fprintf( p_stat," %5.2f/%5.2f   |\n",snr->snr_ua,snr->snr_va );

    // QUANT.
    fprintf( p_stat," Average quant      |" );
    fprintf( p_stat," %5d         |",absm( input->qp_1st_frm ) );
    fprintf( p_stat," %5.2f         |\n",( float )stat->quant1/MAX( 1.0,( float )stat->quant0 ) );

    // MODE
    fprintf( p_stat,"\n -------------------|---------------|\n" );
    fprintf( p_stat,"   Intra            |   Mode used   |\n" );
    fprintf( p_stat," -------------------|---------------|\n" );
    fprintf( p_stat,"\n -------------------|---------------|-----------------|\n" );
    fprintf( p_stat,"   Inter            |   Mode used   | MotionInfo bits |\n" );
    fprintf( p_stat," -------------------|---------------|-----------------|" );
    fprintf( p_stat,"\n Mode  0  (skip)    | %5d         |    %8.2f     |",stat->mode_use_inter[0][PSKIP ],( float )stat->bit_use_mode_inter[0][PSKIP ]/( float )bit_use[1][0] );
    fprintf( p_stat,"\n Mode  1  (16x16)   | %5d         |    %8.2f     |",stat->mode_use_inter[0][P2Nx2N],( float )stat->bit_use_mode_inter[0][P2Nx2N]/( float )bit_use[1][0] );
    fprintf( p_stat,"\n Mode  2  (16x8)    | %5d         |    %8.2f     |",stat->mode_use_inter[0][P2NxN ],( float )stat->bit_use_mode_inter[0][P2NxN ]/( float )bit_use[1][0] );
    fprintf( p_stat,"\n Mode  3  (8x16)    | %5d         |    %8.2f     |",stat->mode_use_inter[0][PNx2N ],( float )stat->bit_use_mode_inter[0][PNx2N ]/( float )bit_use[1][0] );
    fprintf( p_stat,"\n Mode  4  (8x8)     | %5d         |    %8.2f     |",stat->mode_use_inter[0][PNxN  ],( float )stat->bit_use_mode_inter[0][PNxN  ]/( float )bit_use[1][0] );
    mean_motion_info_bit_use[0] = ( float )( stat->bit_use_mode_inter[0][0] + stat->bit_use_mode_inter[0][1] + stat->bit_use_mode_inter[0][2]
                                  + stat->bit_use_mode_inter[0][3] + stat->bit_use_mode_inter[0][PNxN] )/( float ) bit_use[1][0];

    // B pictures
    if( input->successive_Bframe!=0 && Bframe_ctr!=0 )
    {
        fprintf( p_stat,"\n\n -------------------|---------------|-----------------|\n" );
        fprintf( p_stat,"   B frame          |   Mode used   | MotionInfo bits |\n" );
        fprintf( p_stat," -------------------|---------------|-----------------|" );
        fprintf( p_stat,"\n Mode  0  (skip)    | %5d         |    %8.2f     |",stat->mode_use_inter[1][PSKIP ],( float )stat->bit_use_mode_inter[1][PSKIP ]/( float )Bframe_ctr );
        fprintf( p_stat,"\n Mode  1  (16x16)   | %5d         |    %8.2f     |",stat->mode_use_inter[1][P2Nx2N],( float )stat->bit_use_mode_inter[1][P2Nx2N]/( float )Bframe_ctr );
        fprintf( p_stat,"\n Mode  2  (16x8)    | %5d         |    %8.2f     |",stat->mode_use_inter[1][P2NxN ],( float )stat->bit_use_mode_inter[1][P2NxN ]/( float )Bframe_ctr );
        fprintf( p_stat,"\n Mode  3  (8x16)    | %5d         |    %8.2f     |",stat->mode_use_inter[1][PNx2N ],( float )stat->bit_use_mode_inter[1][PNx2N ]/( float )Bframe_ctr );
        fprintf( p_stat,"\n Mode  4  (8x8)     | %5d         |    %8.2f     |",stat->mode_use_inter[1][PNxN  ],( float )stat->bit_use_mode_inter[1][PNxN  ]/( float )Bframe_ctr );
        mean_motion_info_bit_use[1] = ( float )( stat->bit_use_mode_inter[1][0] + stat->bit_use_mode_inter[1][1] + stat->bit_use_mode_inter[1][2]
                                      + stat->bit_use_mode_inter[1][3] + stat->bit_use_mode_inter[1][PNxN] )/( float ) Bframe_ctr;
    }

    fprintf( p_stat,"\n\n --------------------|----------------|----------------|----------------|\n" );
    fprintf( p_stat,"  Bit usage:         |      Intra     |      Inter     |    B frame     |\n" );
    fprintf( p_stat," --------------------|----------------|----------------|----------------|\n" );
    fprintf( p_stat," Seq heaer           |" );
    fprintf( p_stat," %10.2f              |\n",( float ) seq_header );

    fprintf( p_stat," Seq End             |" );
    fprintf( p_stat," %10.2f              |\n",( float ) stat->bit_use_stuffingBits[3] );

    fprintf( p_stat," Slice Header        |" );
    fprintf( p_stat," %10.2f     |",( float ) slice_header[0]/bit_use[0][0] );
    fprintf( p_stat," %10.2f     |",( float ) slice_header[1]/bit_use[1][0] );

    if( input->successive_Bframe!=0 && Bframe_ctr!=0 )
    {
        fprintf( p_stat," %10.2f     |",( float ) slice_header[2]/Bframe_ctr );
    }
    else
    {
        fprintf( p_stat," %10.2f     |", 0. );
    }
    fprintf( p_stat,"\n" );

    fprintf( p_stat," Header              |" );
    fprintf( p_stat," %10.2f     |",( float ) stat->bit_use_header[0]/bit_use[0][0] );
    fprintf( p_stat," %10.2f     |",( float ) stat->bit_use_header[1]/bit_use[1][0] );

    if( input->successive_Bframe!=0 && Bframe_ctr!=0 )
    {
        fprintf( p_stat," %10.2f     |",( float ) stat->bit_use_header[2]/Bframe_ctr );
    }
    else
    {
        fprintf( p_stat," %10.2f     |", 0. );
    }

    fprintf( p_stat,"\n" );
    fprintf( p_stat," Mode                |" );
    fprintf( p_stat," %10.2f     |",( float )stat->bit_use_mb_type[0]/bit_use[0][0] );
    fprintf( p_stat," %10.2f     |",( float )stat->bit_use_mb_type[1]/bit_use[1][0] );

    if( input->successive_Bframe!=0 && Bframe_ctr!=0 )
    {
        fprintf( p_stat," %10.2f     |",( float )stat->bit_use_mb_type[2]/Bframe_ctr );
    }
    else
    {
        fprintf( p_stat," %10.2f     |", 0. );
    }

    fprintf( p_stat,"\n" );
    fprintf( p_stat," Motion Info         |" );
    fprintf( p_stat,"        ./.     |" );
    fprintf( p_stat," %10.2f     |",mean_motion_info_bit_use[0] );

    if( input->successive_Bframe!=0 && Bframe_ctr!=0 )
    {
        fprintf( p_stat," %10.2f     |",mean_motion_info_bit_use[1] );
    }
    else
    {
        fprintf( p_stat," %10.2f     |", 0. );
    }

    fprintf( p_stat,"\n" );
    fprintf( p_stat," CBP Y/C             |" );

    for ( j=0; j < 2; j++ )
    {
        fprintf( p_stat," %10.2f     |", ( float )stat->tmp_bit_use_cbp[j]/bit_use[j][0] );
    }

    if( input->successive_Bframe!=0 && Bframe_ctr!=0 )
    {
        fprintf( p_stat," %10.2f     |", ( float )stat->tmp_bit_use_cbp[2]/Bframe_ctr );
    }
    else
    {
        fprintf( p_stat," %10.2f     |", 0. );
    }

    fprintf( p_stat,"\n" );

    if( input->successive_Bframe!=0 && Bframe_ctr!=0 )
        fprintf( p_stat," Coeffs. Y           | %10.2f     | %10.2f     | %10.2f     |\n",
                 ( float )stat->bit_use_coeffY[0]/bit_use[0][0], ( float )stat->bit_use_coeffY[1]/bit_use[1][0], ( float )stat->bit_use_coeffY[2]/Bframe_ctr );
    else
        fprintf( p_stat," Coeffs. Y           | %10.2f     | %10.2f     | %10.2f     |\n",
                 ( float )stat->bit_use_coeffY[0]/bit_use[0][0], ( float )stat->bit_use_coeffY[1]/( float )bit_use[1][0], 0. );

    if( input->successive_Bframe!=0 && Bframe_ctr!=0 )
        fprintf( p_stat," Coeffs. C           | %10.2f     | %10.2f     | %10.2f     |\n",
                 ( float )stat->bit_use_coeffC[0]/bit_use[0][0], ( float )stat->bit_use_coeffC[1]/bit_use[1][0], ( float )stat->bit_use_coeffC[2]/Bframe_ctr );
    else
        fprintf( p_stat," Coeffs. C           | %10.2f     | %10.2f     | %10.2f     |\n",
                 ( float )stat->bit_use_coeffC[0]/bit_use[0][0], ( float )stat->bit_use_coeffC[1]/bit_use[1][0], 0. );

    if( input->successive_Bframe!=0 && Bframe_ctr!=0 )
        fprintf( p_stat," Stuffing Bits       | %10.2f     | %10.2f     | %10.2f     |\n",
                 ( float )stat->bit_use_stuffingBits[0]/bit_use[0][0], ( float )stat->bit_use_stuffingBits[1]/bit_use[1][0], ( float )stat->bit_use_stuffingBits[2]/Bframe_ctr );
    else
        fprintf( p_stat," Stuffing Bits       | %10.2f     | %10.2f     | %10.2f     |\n",
                 ( float )stat->bit_use_stuffingBits[0]/bit_use[0][0], ( float )stat->bit_use_stuffingBits[1]/bit_use[1][0], 0. );

    fprintf( p_stat," --------------------|----------------|----------------|----------------|\n" );
    fprintf( p_stat," average bits/frame  |" );

    for ( i=0; i < 2; i++ )
    {
        fprintf( p_stat," %10.2f     |", ( float ) bit_use[i][1]/( float ) bit_use[i][0] );
    }

    if( input->successive_Bframe!=0 && Bframe_ctr!=0 )
    {
        fprintf( p_stat," %10.2f     |", ( float ) bit_use_Bframe/ ( float ) Bframe_ctr );
    }
    else
    {
        fprintf( p_stat," %10.2f     |", 0. );
    }

    fprintf( p_stat,"\n" );
    fprintf( p_stat," --------------------|----------------|----------------|----------------|\n" );

    fclose( p_stat );
}


/*
*************************************************************************
* Function:Prints the header of the protocol.
* Input:struct inp_par *inp
* Output:
* Return:
* Attention:
*************************************************************************
*/

void information_init()
{
    printf( "-----------------------------------------------------------------------------\n" );
    printf( " Input YUV file                    : %s \n", input->infile );
    printf( " Output IVC bitstream              : %s \n", input->outfile );
    if ( p_dec != -1 )
    {
        printf( " Output YUV file                   : %s \n", input->ReconFile );
    }

    printf( " Output statistics file            : stat.dat \n" );
    printf( "-----------------------------------------------------------------------------\n" );
    printf( " Frame   Bit/pic   QP   SnrY    SnrU    SnrV    Time(ms)  FRM/FLD  IntraMBs\n" );
}

/*
*************************************************************************
* Function:Dynamic memory allocation of frame size related global buffers
buffers are defined in global.h, allocated memory must be freed in
void free_global_buffers()
* Input:  Input Parameters struct inp_par *inp,                            \n
Image Parameters struct img_par *img
* Output:
* Return: Number of allocated bytes
* Attention:
*************************************************************************
*/


int init_global_buffers()
{
    int i;
    int refnum;
    int pic_luma_size_pad;
    int pic_chroma_size_pad;
    int buf_offset_y;
    int buf_offset_uv;
    int memory_size = 0;

    pic_luma_size_pad   = ( img->height + 2 * IMG_PAD_SIZE ) * ( img->width + 2 * IMG_PAD_SIZE );
    pic_chroma_size_pad = pic_luma_size_pad >> 2;
    buf_offset_y  = img->iStride * IMG_PAD_SIZE + IMG_PAD_SIZE;
    buf_offset_uv = ( img->iStrideC * IMG_PAD_SIZE / 2 ) + ( IMG_PAD_SIZE / 2 );

#ifdef TDRDO
    imgY_pre_buffer = ( pel_t* )malloc( input->height_org*input->width_org * 3 / 2 );
#endif

    imgY_org_buffer = ( pel_t* )malloc( input->height_org*input->width_org * 3 / 2 );
    memory_size += input->height_org*input->width_org * 3 / 2;

    // allocate memory for reference frame buffers: imgY_org, imgUV_org
    imgY_org = ( uchar_t * )calloc( img->height * img->width, sizeof( uchar_t ) );
    memory_size += img->height * img->width * sizeof( uchar_t );
    imgU_org = ( uchar_t * )calloc( img->height_cr * img->width_cr, sizeof( uchar_t ) );
    memory_size += img->height_cr * img->width_cr * sizeof( uchar_t );
    imgV_org = ( uchar_t * )calloc( img->height_cr * img->width_cr, sizeof( uchar_t ) );
    memory_size += img->height_cr * img->width_cr * sizeof( uchar_t );

    //memory_size += get_mem2D(&imgY_org_frm, img->height, img->width);
    //memory_size += get_mem3D(&imgUV_org_frm, 2, img->height_cr, img->width_cr);

    memory_size += get_mem3Dint( &img->pfrm_mv, img->height / B8_SIZE, img->width / B8_SIZE + 4, 2 );
    if ( input->successive_Bframe != 0 )
    {
        memory_size += get_mem3Dint( &img->bfrm_fmv, img->height / B8_SIZE, img->width / B8_SIZE + 4, 2 ); // B forward MV
        memory_size += get_mem3Dint( &img->bfrm_bmv, img->height / B8_SIZE, img->width / B8_SIZE + 4, 2 ); // B backward MV
    }

    // allocate memory for reference frames of each block: img->pfrm_ref
    memory_size += get_mem2Dint( &img->pfrm_ref, img->height / B8_SIZE, img->width / B8_SIZE );
    if ( input->successive_Bframe != 0 )
    {
        // allocate memory for temp B-frame motion vector buffer: img->bfrm_fref, img->bfrm_bref
        memory_size += get_mem2Dint( &img->bfrm_fref, img->height / BLOCK_SIZE, img->width / BLOCK_SIZE );
        memory_size += get_mem2Dint( &img->bfrm_bref, img->height / BLOCK_SIZE, img->width / BLOCK_SIZE );
    }

    // allocate memory for B frame coding: nextP_imgY, nextP_imgUV
    memory_size += get_mem2D( &nextP_imgY, img->height, img->width );
    memory_size += get_mem3D( &nextP_imgUV, 2, img->height_cr, img->width_cr );

    // allocate memory for temp quarter pel luma frame buffer: img4Y_tmp
#ifdef PADDING_FALG
    memory_size += get_mem2Dint( &img4Y_tmp, ( img->height + 2 * ( IMG_SUBPIXEL_PAD_SIZE + IMG_PAD_SIZE ) ) * 4, ( img->width + 2 * ( IMG_SUBPIXEL_PAD_SIZE + IMG_PAD_SIZE ) ) * 4 );
#else
    img4Y_tmp = ( int * )calloc( ( ( img->height + 2 * ( IMG_SUBPIXEL_PAD_SIZE + IMG_SUBPIXEL_PAD_SIZE_FOR_CLIP ) ) * 4 ) * ( ( img->width + 2 * ( IMG_SUBPIXEL_PAD_SIZE + IMG_SUBPIXEL_PAD_SIZE_FOR_CLIP ) ) * 4 ), sizeof( int ) );
    memory_size += ( ( img->height + 2 * ( IMG_SUBPIXEL_PAD_SIZE + IMG_SUBPIXEL_PAD_SIZE_FOR_CLIP ) ) * 4 ) * ( ( img->width + 2 * ( IMG_SUBPIXEL_PAD_SIZE + IMG_SUBPIXEL_PAD_SIZE_FOR_CLIP ) ) * 4 )*sizeof( int );

#endif

    if ( input->usefme )
    {
        memory_size += get_mem_FME();
    }

    for ( refnum = 0; refnum < img->real_ref_num + 1; refnum++ )
    {
        for ( i = 0; i < 3; i++ )
        {
            if ( i == 0 )
            {
                reference_frame[refnum][i] = ( uchar_t * )calloc( pic_luma_size_pad, sizeof( uchar_t ) );
            }
            else
            {
                reference_frame[refnum][i] = ( uchar_t * )calloc( pic_chroma_size_pad, sizeof( uchar_t ) );
            }
        }
    }

    for ( i = 0; i < img->real_ref_num; i++ )
    {

#ifdef PADDING_FALG
        mref_frm[i] = ( uchar_t * )calloc( ( ( img->height + 2 * ( IMG_SUBPIXEL_PAD_SIZE + IMG_PAD_SIZE ) ) * 4 ) * ( ( img->width + 2 * ( IMG_SUBPIXEL_PAD_SIZE + IMG_PAD_SIZE ) ) * 4 ), sizeof( uchar_t ) );
#else
        mref_frm[i] = ( uchar_t * )calloc( ( ( img->height + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4 ) * ( ( img->width + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4 ), sizeof( uchar_t ) );
#endif

    }

    current_frame[0] = reference_frame[0][0] + buf_offset_y;
    current_frame[1] = reference_frame[0][1] + buf_offset_uv;
    current_frame[2] = reference_frame[0][2] + buf_offset_uv;

    for ( i = 0; i < img->real_ref_num; i++ )
    {
        ref_frm[i][0] = reference_frame[i+1][0] + buf_offset_y;
        ref_frm[i][1] = reference_frame[i+1][1] + buf_offset_uv;
        ref_frm[i][2] = reference_frame[i+1][2] + buf_offset_uv;
    }

    return ( memory_size );
}

/*
*************************************************************************
* Function:Free allocated memory of frame size related global buffers
buffers are defined in global.h, allocated memory is allocated in
int get_mem4global_buffers()
* Input: Input Parameters struct inp_par *inp,                             \n
Image Parameters struct img_par *img
* Output:
* Return:
* Attention:
*************************************************************************
*/

void free_global_buffers()
{
    int  i;
    int j;

#ifdef TDRDO
    free( imgY_pre_buffer );
#endif

    free( imgY_org_buffer );

    free( imgY_org );
    free( imgU_org );
    free( imgV_org );

    free_mem3Dint( img->pfrm_mv, img->height / B8_SIZE );
    free_mem2Dint( img->pfrm_ref );

    // number of reference frames increased by one for next P-frame
    for ( i = 0; i < img->real_ref_num; i++ )
    {
        free( mref_frm[i] );
    }

    for( j = 0 ; j < img->real_ref_num+1 ; j++ )
    {
        for ( i = 0; i < 3; i++ )
        {
            free( reference_frame[j][i] );
        }

        // free(reference_frame[j]);
    }

    free_mem2D( nextP_imgY );
    free_mem3D( nextP_imgUV,2 );

    // free multiple ref frame buffers
    // number of reference frames increased by one for next P-frame

    if( input->successive_Bframe!=0 )
    {
        // free last P-frame buffers for B-frame coding
        free_mem3Dint( img->bfrm_fmv, img->height / B8_SIZE );
        free_mem3Dint( img->bfrm_bmv, img->height / B8_SIZE );
        free_mem2Dint( img->bfrm_fref );
        free_mem2Dint( img->bfrm_bref );
    } // end if B frame

    //free_mem2Dint(img4Y_tmp);    // free temp quarter pel frame buffer
    free( img4Y_tmp );
    free( img->mb_data );


    if( input->usefme )
    {
        free_mem_FME();
    }

}

/**************************************************************************
* Function:Allocate memory for mv
* Input:Image Parameters struct img_par *img
int****** mv
* Output:
* Return: memory size in bytes
* Attention:
**************************************************************************/
int get_mem_mv ( int****** mv )
{
    int i, j, k, l;

    if ( ( *mv = ( int***** )calloc( img->real_ref_num,sizeof( int**** ) ) ) == NULL )
    {
        no_mem_exit ( "get_mem_mv: mv" );
    }
    for ( i=0; i<img->real_ref_num; i++ )
    {
        if ( ( ( *mv )[i] = ( int**** )calloc( MAXMODE,sizeof( int*** ) ) ) == NULL )
        {
            no_mem_exit ( "get_mem_mv: mv" );
        }
        for ( j=0; j<MAXMODE; j++ )
        {
            if ( ( ( *mv )[i][j] = ( int*** )calloc( 2,sizeof( int** ) ) ) == NULL )
            {
                no_mem_exit ( "get_mem_mv: mv" );
            }
            for ( k=0 ; k<2; k++ )
            {
                if ( ( ( *mv )[i][j][k] = ( int** )calloc( 2,sizeof( int* ) ) ) == NULL )
                {
                    no_mem_exit ( "get_mem_mv: mv" );
                }
                for ( l=0; l<2; l++ )
                {
                    if ( ( ( *mv )[i][j][k][l] = ( int* )calloc( 2,sizeof( int ) ) ) == NULL )
                    {
                        no_mem_exit ( "get_mem_mv: mv" );
                    }
                }
            }
        }
    }
    return 2*2*img->real_ref_num*MAXMODE*2*sizeof( int ); //block_x, block_y, refframe, mode, mv_x, mv_y
}

/**************************************************************************
* Function:Free memory from mv
* Input:int****** mv
* Output:
* Return:
* Attention:
**************************************************************************/
void free_mem_mv ( int***** mv )
{
    int i, j, k, l;

    for ( i = 0; i < img->real_ref_num; i++ )
    {
        for ( j = 0; j < MAXMODE; j++ )
        {
            for ( k = 0; k < 2; k++ )
            {
                for ( l = 0; l < 2; l++ )
                {
                    free ( mv[i][j][k][l] );
                }
                free ( mv[i][j][k] );
            }
            free ( mv[i][j] );
        }
        free ( mv[i] );
    }
    free ( mv );
}

/**************************************************************************
* Function:SetImgType
* Input:
* Output:
* Return:
* Attention:
**************************************************************************/
void SetImgType()
{
    if ( input->intra_period == 0 )
    {
        if ( img->ip_frm_idx == 0 )
        {
            img->type = I_IMG;        // set image type for first image to I-frame
        }
        else
        {
            img->type = P_IMG;        // P-frame
        }
    }
    else
    {
        if ( ( img->ip_frm_idx%input->intra_period ) == 0 )
        {
            img->type = I_IMG;
        }
        else
        {
            img->type = P_IMG;        // P-frame
        }
    }

    if ( img->type == I_IMG )
    {
        img->i_frm_idx = img->ip_frm_idx;
    }
}





















