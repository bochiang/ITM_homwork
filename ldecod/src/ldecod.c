#include "contributors.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>
#include <assert.h>

#if defined WIN32
#include <conio.h>
#include <IO.H>
#include <FCNTL.H>
#endif

#include "global.h"
#include "memalloc.h"
#include "annexb.h"
#include "bbv.h"
#include "header.h"
#include "AEC.h"
#include "../../common/common.h"

#define _S_IREAD        0000400         /* read permission, owner */
#define _S_IWRITE       0000200         /* write permission, owner */

extern FILE* bits;

inp_params    *input;       //!< input parameters from input configuration file
ImgParams    *img;         //!< image parameters
StatBits *StatBitsPtr;

Bitstream *currStream;     //! bit stream buffer, which stores the bit date between two start codes, and update after each start code
FILE *reffile,*reffile2;

extern BbvBuffer_t *pBbv;


int main( int argc, char **argv )
{
    int i;
    snr_par *snr;

    // allocate memory for the structures
    if ( ( input =  ( inp_params * )calloc( 1, sizeof( inp_params ) ) )==NULL )
    {
        no_mem_exit( "main: input" );
    }
    if ( ( snr =  ( snr_par * )calloc( 1, sizeof( snr_par ) ) )==NULL )
    {
        no_mem_exit( "main: snr" );
    }
    if ( ( img =  ( ImgParams * )calloc( 1, sizeof( ImgParams ) ) )==NULL )
    {
        no_mem_exit( "main: img" );
    }

    if ( ( StatBitsPtr =  ( struct StatBits * )calloc( 1, sizeof( struct StatBits ) ) )==NULL )
    {
        no_mem_exit( "main: StatBits" );
    }

    StatBitsPtr->curr_frame_bits = StatBitsPtr->prev_frame_bits
                                   = StatBitsPtr->emulate_bits = StatBitsPtr->last_unit_bits = 0; //ITM_r2
    StatBitsPtr->bitrate = StatBitsPtr->coded_pic_num = StatBitsPtr->time_s = 0; //ITM_r2

    currStream = AllocateBitstream();

    eos = 0;
    bValidSyntax = 1;

    init_conf( input,argc,argv );

    OpenBitstreamFile ( input->infile );

    malloc_slice( input, img );

    com_funs_init_ip_filter();
    com_funs_init_intra_pred();
    com_funs_init_deblock_filter();
    com_funs_init_dct();
    com_funs_init_pixel_opt();

#if ENABLE_SSE
    com_init_intrinsic();
#endif
#if ENABLE_AVX2
    //com_init_intrinsic_256();
#endif

    // initialize the table of square values used in snr calculation
    for ( i=0; i <  256; i++ )
    {
        img->tab_sqr[i]=i*i;
    }

    img->ip_frm_idx=0;
    img->type = I_IMG;
    img->imgtr_last_P = 0;
    img->imgtr_next_P = 0;

    img->new_seq_header_flag = 1;
    img->new_sequence_flag   = 1;

    img->FrmNum = 0;

    // B pictures
    Bframe_ctr=0;

    // time for total decoding session
    tot_time = 0;
    do
    {
        while ( ( decode_one_frame( img, input, snr ) != EOS ) && ( !IsEndOfBitstream() ) );
    }
    while ( !IsEndOfBitstream() );

    if( StatBitsPtr->time_s==0 )
    {
        StatBitsPtr->total_bitrate[StatBitsPtr->time_s++] = StatBitsPtr->bitrate;
    }

    eos = 1;
    if ( p_ref)
    {
        find_snr( snr,img,p_ref, img_prev );    // if ref sequence exist

        if (pre_img_type == I_IMG) // I picture, ref order
        {
            fprintf(stdout, "%3d(I)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s %8d %6d\n",
                img->FrmNum, pre_img_tr, pre_img_qp, snr->snr_y, snr->snr_u, snr->snr_v, pre_tmp_time, "FRM", StatBitsPtr->prev_frame_bits, StatBitsPtr->prev_emulate_bits);
        }
        else if (pre_img_type == P_IMG) // P pictures
        {
            fprintf(stdout, "%3d(P)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s %8d %6d\n",
                img->FrmNum, pre_img_tr, pre_img_qp, snr->snr_y, snr->snr_u, snr->snr_v, pre_tmp_time, "FRM", StatBitsPtr->prev_frame_bits, StatBitsPtr->prev_emulate_bits);
        }
    }

    img->FrmNum++;
    // B PICTURE : save the last P picture
    if ( input->bwrite_dec_frm ) // output_dec_pic
    {
        write_prev_Pframe( img, p_out, img_prev );
    }

    if ( input->check_BBV_flag )
    {
        stat_bbv_buffer( pBbv );
        pBbv = free_bbv_memory( pBbv );
        printf( "min bbv_buffer_size in bitstream is %d\n", ( ( pminBBSsize>>14 ) + ( pminBBSsize&0x3FFF? 1 : 0 ) ) );
        if ( !pbbv_mode )
        {
            printf( "min initial bbv_delay(0) time is %.4f(s)\n", ( float )pminFsize/pbitrate );
        }
    }

    if ( img->sequence_end_flag )
    {
        fprintf( stdout, "Sequence End\n\n" );
    }

    {
        int i;
        float framerate[8]= {24000/1001,24,25,30000/1001,30,50,60000/1001,60};
        if( ( int )( StatBitsPtr->coded_pic_num - ( StatBitsPtr->time_s+1 )*framerate[frame_rate_code-1] + 0.5 ) == 0 )
        {
            StatBitsPtr->total_bitrate[StatBitsPtr->time_s++] = StatBitsPtr->bitrate;
            CheckBitrate( StatBitsPtr->bitrate,StatBitsPtr->time_s );
            StatBitsPtr->bitrate = 0;
        }
        StatBitsPtr->total_bitrate[StatBitsPtr->time_s-1] += 32;
        printf( "Second(s)\tBitrate(bit/s)\n" );
        for( i=0; i<StatBitsPtr->time_s; i++ )
        {
            printf( " %3d\t\t %d\n", i, StatBitsPtr->total_bitrate[i] );
        }
        if( StatBitsPtr->time_s==0 )
        {
            printf( " %3d\t\t %d\n", 0, StatBitsPtr->total_bitrate[0] );
        }
    }

    report_summary( input, img, snr );


    FreeBitstream( currStream );
    free_slice( img );
    free_global_buffers( img );

    CloseBitstreamFile();

    if (input->bwrite_dec_frm)
    {
        fclose(p_out);
    }

    if ( p_ref )
    {
        fclose( p_ref );
    }

#if TRACE
    fclose( p_trace );
#endif

    free ( input );
    free ( snr );
    free ( img );

    //system("pause");
    return 0;
}

/*
*************************************************************************
* Function:Read input from configuration file
* Input:Name of configuration filename
* Output:
* Return:
* Attention:
*************************************************************************
*/

void init_conf( inp_params *inp, int numpar,char **config_str )
{
    FILE *fd = NULL;

    inp->check_BBV_flag = 0;

    // read the decoder configuration file
    if (numpar != 2 && numpar != 3 && numpar != 4 && numpar != 5 && numpar != 6)
    {
        fprintf(stderr, "Usage: %s <IVC bitstream> <dec filename> <ref filename> <bbv_check> <write dec file> \n",config_str[0]);
        error( errortext, 300 );
    }

    strcpy(inp->infile, config_str[1]);
    strcpy(inp->outfile, "test_dec.yuv");
    inp->check_BBV_flag = 0;
    inp->bwrite_dec_frm = 1;

    if (numpar > 2)
    {
        strcpy(inp->outfile, config_str[2]);
        if (numpar > 3)
        {
            strcpy(inp->reffile, config_str[3]);
            if (numpar > 4)
            {
                inp->check_BBV_flag = atoi(config_str[4]);

                if (numpar > 5)
                {
                    inp->bwrite_dec_frm = atoi(config_str[5]);
                }
            }
        }
    }

#if TRACE
    if ( ( p_trace=fopen( DEC_TRACE_FILE,"w" ) )==NULL )
    {
        fprintf(stderr, "Error open file %s!",DEC_TRACE_FILE );
        error( errortext,500 );
    }
#endif
    bValidSyntax = 1;

    if (input->bwrite_dec_frm)
    {
        if ((p_out = fopen(inp->outfile, "wb")) == 0)
        {
            fprintf(stderr, "Error open file %s ", inp->outfile);
            error(errortext, 500);
        }
    }

    fprintf( stdout,"--------------------------------------------------------------------------\n" );
    fprintf( stdout," Decoder config file                    : %s \n",config_str[0] );
    fprintf( stdout,"--------------------------------------------------------------------------\n" );
    fprintf( stdout," Input IVC bitstream                    : %s \n",inp->infile );
    fprintf( stdout," Output decoded YUV 4:2:0               : %s \n",inp->outfile );

    if ( ( p_ref=fopen( inp->reffile,"rb" ) )==0 )
    {
        fprintf( stdout," Input reference file                   : %s does not exist \n",inp->reffile );
        fprintf( stdout,"                                          SNR values are not available\n" );
    }
    else
    {
        fprintf( stdout," Input reference file                   : %s \n",inp->reffile );
    }

    fprintf( stdout,"--------------------------------------------------------------------------\n" );
    fprintf( stdout," Frame   TR    QP   SnrY    SnrU    SnrV   Time(ms)   FRM/FLD  Bits  EmulateBits\n" );
}

/*
*************************************************************************
* Function:Reports the gathered information to appropriate outputs
* Input:
inp_params *inp,
img_params *img,
* Output:
* Return:
* Attention:
*************************************************************************
*/

void report_summary( inp_params *inp, ImgParams *img, snr_par *snr )
{
    if (p_ref)
    {
        fprintf(stdout, "-------------------- Average SNR all frames ------------------------------\n");
        fprintf(stdout, " SNR Y(dB)           : %5.2f\n", snr->snr_sum_y);
        fprintf(stdout, " SNR U(dB)           : %5.2f\n", snr->snr_sum_u);
        fprintf(stdout, " SNR V(dB)           : %5.2f\n", snr->snr_sum_v);
    }
    fprintf( stdout," Total decoding time : %.3f sec \n",tot_time*0.001 );
    fprintf( stdout,"--------------------------------------------------------------------------\n" );
    if( !bValidSyntax )
    {
        fprintf( stdout, " SOME BITSTREAM SYNTAX IS ILLEGAL\n" );
        assert(bValidSyntax);
    }
    fprintf( stdout," Exit ITM %s decoder, ver %s ",ITM, VERSION );
    fprintf( stdout,"\n" );
}




/*
*************************************************************************
* Function:Dynamic memory allocation of frame size related global buffers
buffers are defined in global.h, allocated memory must be freed in
void free_global_buffers()
* Input:Input Parameters inp_params *inp, Image Parameters img_params *img
* Output:Number of allocated bytes
* Return:
* Attention:
*************************************************************************
*/


int init_global_buffers( inp_params *inp, ImgParams *img )
{
    int i;
    int refnum;

    int memory_size=0;

    int img_height = img->height;
    int img_width = img->width;
    int img_heightc = img->height_cr;
    int img_widthc = img->width_cr;

    int pic_luma_size_pad = ( img_height + 2*IMG_PAD_SIZE ) * ( img_width + 2*IMG_PAD_SIZE );
    int pic_chroma_size_pad = pic_luma_size_pad >> 2;

    int buf_offset_y = img->iStride * IMG_PAD_SIZE + IMG_PAD_SIZE;
    int buf_offset_uv = ( img->iStrideC * IMG_PAD_SIZE / 2 ) + ( IMG_PAD_SIZE / 2 );

    // allocate memory for reconstructed image
    //img_rec = (uchar_t*)malloc(img_width * img_height * 3/2);

    // allocate memory for img_prev
    img_prev = ( uchar_t* )malloc( img->width_org * img->height_org * 3/2 );




    // allocate memory in structure img
    if( ( ( img->mb_data ) = ( Macroblock * ) calloc( ( img->width/MB_SIZE ) * ( img_height /MB_SIZE ),sizeof( Macroblock ) ) ) == NULL )
    {
        no_mem_exit( "init global buffers: img->mb_data" );
    }

    memory_size += get_mem2Dint( &( img->ipredmode ), img->width / B4_SIZE + 2, img_height / B4_SIZE + 2 );

    // allocate memory for reference frames of each block
    memory_size += get_mem2Dint( &img->pfrm_ref,   img_height /B8_SIZE, img->width/B8_SIZE );
    memory_size += get_mem2Dint( &( img->bfrm_fref ),img_height /B8_SIZE, img->width/B8_SIZE );
    memory_size += get_mem2Dint( &( img->bfrm_bref ),img_height /B8_SIZE, img->width/B8_SIZE );

    memory_size += get_mem3Dint( &( img->pfrm_mv ),  img_height /B8_SIZE, img->width/B8_SIZE + 4, 3 );
    memory_size += get_mem3Dint( &( img->bfrm_fmv ), img_height /B8_SIZE, img->width/B8_SIZE + 4, 3 );
    memory_size += get_mem3Dint( &( img->bfrm_bmv ), img_height /B8_SIZE, img->width/B8_SIZE + 4, 3 );

    // allocate buffers for reference frames
    for( refnum=0 ; refnum<img->real_ref_num+1 ; refnum++ )
    {
        reference_frame[refnum][0] = ( uchar_t * )malloc( pic_luma_size_pad );
        reference_frame[refnum][1] = ( uchar_t * )malloc( pic_chroma_size_pad );
        reference_frame[refnum][2] = ( uchar_t * )malloc( pic_chroma_size_pad );
    }

    //forward reference frame buffer
    for( i = 0 ; i < img->real_ref_num+1 ; i++ )
    {
        p_ref_frm[i][0] = reference_frame[i][0] + buf_offset_y;
        p_ref_frm[i][1] = reference_frame[i][1] + buf_offset_uv;
        p_ref_frm[i][2] = reference_frame[i][2] + buf_offset_uv;
    }
    p_cur_frm[0] = reference_frame[img->real_ref_num][0] + buf_offset_y;
    p_cur_frm[1] = reference_frame[img->real_ref_num][1] + buf_offset_uv;
    p_cur_frm[2] = reference_frame[img->real_ref_num][2] + buf_offset_uv;

    //luma for backward
    //forward/backward reference buffer
    f_ref_frm = p_ref_frm[1]; // ref_index=0 for B frame,
    b_ref_frm = p_ref_frm[0]; // ref_index=0 for B frame,

    return ( memory_size );
}

/*
*************************************************************************
* Function:Free allocated memory of frame size related global buffers
buffers are defined in global.h, allocated memory is allocated in
int init_global_buffers()
* Input:Input Parameters inp_params *inp, Image Parameters img_params *img
* Output:
* Return:
* Attention:
*************************************************************************
*/

void free_global_buffers( ImgParams *img )
{
    int  i,j;

    free( img_prev );

    // free mem, allocated for structure img
    if ( img->mb_data       != NULL )
    {
        free( img->mb_data );
    }

    j = ( img->PicWidthInMbs )*( img->PicHeightInMbs );

    free_mem2Dint ( img->ipredmode );

    free_mem2Dint( img->pfrm_ref );
    free_mem2Dint( img->bfrm_fref );
    free_mem2Dint( img->bfrm_bref );

    free_mem3Dint( img->pfrm_mv, img->height /B8_SIZE );
    free_mem3Dint( img->bfrm_fmv,img->height /B8_SIZE );
    free_mem3Dint( img->bfrm_bmv,img->height /B8_SIZE );

    for ( i=0; i<33; i++ )
    {
        for( j=0; j<3; j++ )
        {
            free( reference_frame[i][j] );
        }
    }
}

int get_direct_mv ( int****** mv,int mb_x,int mb_y )
{
    int i, j, k, l;

    if ( ( *mv = ( int***** )calloc( mb_y,sizeof( int**** ) ) ) == NULL )
    {
        no_mem_exit ( "get_mem_mv: mv" );
    }
    for ( i=0; i<mb_y; i++ )
    {
        if ( ( ( *mv )[i] = ( int**** )calloc( mb_x,sizeof( int*** ) ) ) == NULL )
        {
            no_mem_exit ( "get_mem_mv: mv" );
        }
        for ( j=0; j<mb_x; j++ )
        {
            if ( ( ( *mv )[i][j] = ( int*** )calloc( 2,sizeof( int** ) ) ) == NULL )
            {
                no_mem_exit ( "get_mem_mv: mv" );
            }

            for ( k=0; k<2; k++ )
            {
                if ( ( ( *mv )[i][j][k] = ( int** )calloc( 2,sizeof( int* ) ) ) == NULL )
                {
                    no_mem_exit ( "get_mem_mv: mv" );
                }
                for ( l=0; l<2; l++ )
                    if ( ( ( *mv )[i][j][k][l] = ( int* )calloc( 3,sizeof( int ) ) ) == NULL )
                    {
                        no_mem_exit ( "get_mem_mv: mv" );
                    }
            }
        }
    }
    return mb_x*mb_y*2*2*3*sizeof( int );
}

/*
*************************************************************************
* Function:Free memory from mv
* Input:int****** mv
* Output:
* Return:
* Attention:
*************************************************************************
*/

void free_direct_mv ( int***** mv,int mb_x,int mb_y )
{
    int i, j, k, l;

    for ( i=0; i<mb_y; i++ )
    {
        for ( j=0; j<mb_x; j++ )
        {
            for ( k=0; k<2; k++ )
            {
                for ( l=0; l<2; l++ )
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

/*
*************************************************************************
* Function:update the decoder picture buffer
* Input:frame number in the bitstream and the video sequence
* Output:
* Return:
* Attention:
*************************************************************************
*/

void Update_Picture_Buffers()
{
    int i;

    if( img->p_subtype==2 )
    {
        return;
    }

    if( img->p_subtype==3 )
    {
        //forward/backward reference buffer
        f_ref_frm = p_ref_frm[0]; // ref_index=0 for B frame,
        b_ref_frm = p_ref_frm[1]; // ref_index=0 for B frame,

        return;
    }

    // update reference frames, append the new encoded frame to reference buffer
    for( i = img->real_ref_num-1 ; i>0 ; i-- )
    {
        p_ref_frm[i][0] = p_ref_frm[i-1][0];
        p_ref_frm[i][1] = p_ref_frm[i-1][1];
        p_ref_frm[i][2] = p_ref_frm[i-1][2];
    }
    for( i = 0; i < 3; i++ )
    {
        p_ref_frm[0][i] = p_cur_frm[i];
        p_cur_frm[i] = p_ref_frm[img->real_ref_num-1][i];
    }
    //forward/backward reference buffer
    f_ref_frm = p_ref_frm[1]; // ref_index=0 for B frame,
    b_ref_frm = p_ref_frm[0]; // ref_index=0 for B frame,
}

/*!
************************************************************************
* \brief
*    Allocates the slice structure along with its dependent
*    data structures
*
* \par Input:
*    Input Parameters inp_params *inp,  img_params *img
* \author
*
************************************************************************
*/
void malloc_slice( inp_params *inp, ImgParams *img )
{
    CSobj *currSlice;

    img->cs_aec = ( CSobj * ) calloc( 1, sizeof( CSobj ) );
    if ( ( currSlice = img->cs_aec ) == NULL )
    {
        fprintf(stderr, "Memory allocation for CSobj datastruct in NAL-mode %d failed", inp->FileFormat );
        error( errortext,100 );
    }
    // create all context models
    currSlice->mot_ctx = create_contexts_MotionInfo();
    currSlice->tex_ctx = create_contexts_TextureInfo();
}
/*!
************************************************************************
* \brief
*    Memory frees of the CSobj structure and of its dependent
*    data structures
*
* \par Input:
*    Input Parameters inp_params *inp,  img_params *img
* \author
*
************************************************************************
*/
void free_slice( ImgParams *img )
{
    CSobj *currSlice = img->cs_aec;

    // delete all context models
    delete_contexts_MotionInfo( currSlice->mot_ctx );
    delete_contexts_TextureInfo( currSlice->tex_ctx );

    free( img->cs_aec );

    currSlice = NULL;
}