#include "contributors.h"

#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include <string.h>
#include <assert.h>
#ifdef WIN32
#include <IO.H>
#include <STDIO.H>
#endif

#include "global.h"
#include "header.h"
#include "annexb.h"
#include "vlc.h"
#include "memalloc.h"
#include "bbv.h"
#include "../../common/common.h"
#include "../../common/pixel.h"

//!EDIT START <added by  AEC
#include "AEC.h"
#include "biaridecod.h"
//!EDIT end <added by  AEC

void copy2buffer( ImgParams *img, inp_params *inp,int bot );
void replace2buffer( ImgParams *img, inp_params *inp,int bot );

BbvBuffer_t *pBbv = NULL;


uchar_t *temp_slice_buf;
int first_slice_length;
int first_slice_startpos;

extern StatBits *StatBitsPtr;


void xInit_seq()
{
    int auto_pad_right, auto_pad_bottom;

    img->real_ref_num = 32;
        
    if (horizontal_size% MB_SIZE != 0)
    {
        auto_pad_right = MB_SIZE - (horizontal_size % MB_SIZE);
    }
    else
    {
        auto_pad_right = 0;
    }

    if (vertical_size % MB_SIZE != 0)
    {
        auto_pad_bottom = MB_SIZE - (vertical_size % MB_SIZE);
    }
    else
    {
        auto_pad_bottom = 0;
    }

    img->width_org = horizontal_size;
    img->height_org = vertical_size;
    img->width = horizontal_size + auto_pad_right;
    img->height = vertical_size + auto_pad_bottom;
    img->width_cr = (img->width >> 1);
    img->height_cr = (img->height >> 1);
    img->iStride = img->width + 2 * IMG_PAD_SIZE;
    img->iStrideC = img->iStride >> 1;
    img->PicWidthInMbs = img->width / MB_SIZE;   // is also equal to (horizontal_size + 15) / MB_SIZE;
    img->PicHeightInMbs = img->height / MB_SIZE; // is also equal to (vertical_size + 15) / MB_SIZE;
    img->PicSizeInMbs = img->PicWidthInMbs * img->PicHeightInMbs;
    img->max_mb_nr = (img->width * img->height) / (MB_SIZE * MB_SIZE);
    assert(img->PicSizeInMbs == img->max_mb_nr); // after crop, these two variables are always same.
}

/**************************************************************************
* Function:Copy decoded P frame to temporary image array
* Input:
* Output:
* Return:
* Attention:
**************************************************************************/
void copy_Pframe( ImgParams *img, uchar_t *pDst )
{
    int i,j;

    int img_org_widthc = img->width_org >> 1;
    int img_org_heightc = img->height_org >> 1;

    int iStride = img->iStride;
    int iStrideC = img->iStrideC;

    for( i=0; i<img->height_org; i++ )
    {
        for (j = 0; j<img->width_org; j++)
        {
            *pDst++ = imgY_rec[i*iStride+j];
        }
    }

    for( i=0; i<img_org_heightc; i++ )
    {
        for( j=0; j<img_org_widthc; j++ )
        {
            *pDst++ = imgU_rec[i*iStrideC+j];
        }
    }

    for( i=0; i<img_org_heightc; i++ )
    {
        for( j=0; j<img_org_widthc; j++ )
        {
            *pDst++ = imgV_rec[i*iStrideC+j];
        }
    }
}

/**************************************************************************
* Function:decodes one I- or P-frame
* Input:
* Output:
* Return:
* Attention:
**************************************************************************/
int decode_one_frame( ImgParams *img,inp_params *inp, snr_par *snr )
{
    int current_header;

    time_t ltime1;                  // for time measurement
    time_t ltime2;

    struct timeb tstruct1;
    struct timeb tstruct2;

    int tmp_time;                   // time used by decoding the last frame

    float framerate[8]= {24000/1001,24,25,30000/1001,30,50,60000/1001,60}; //ITM_r2

    ftime ( &tstruct1 );            // start time ms
    time( &ltime1 );                // start time s

    img->current_mb_nr = -4711; // initialized to an impossible value for debugging -- correct value is taken from slice header

    if ( input->check_BBV_flag && img->ip_frm_idx && pBbv )
    {
        pBbv->frame_code_bits = 0;
    }
    current_header = find_headers();

    if ( current_header == EOS )
    {
        return EOS;
    }

    img->current_mb_nr = 0;
    img->previous_delta_qp = 0;
    init_frame( img, inp, snr );

    img->types = img->type;

    if( ( img->type==P_IMG )&&( ( pre_dec_img_type == I_IMG )||( pre_dec_img_type==B_IMG ) ) )
    {
        b_pre_dec_intra_img=TRUE;
    }
    else if ( ( img->type==P_IMG )&&( pre_dec_img_type==P_IMG ) )
    {
        b_pre_dec_intra_img=FALSE;
    }

    pre_dec_img_type=img->type;
    if( img->type!=B_IMG )
    {
        pre_img_type  = img->type;
        pre_img_types = img->types;
    }
#if TRACE
    img->writeBSflag = 1;
#endif
    picture_data( img );

    if ( img->type==B_IMG )
    {
        if ( p_ref )
        {
            find_snr( snr,img,p_ref, img_prev );    // if ref sequence exist
        }
    }

    ftime ( &tstruct2 );  // end time ms
    time( &ltime2 );                                // end time sec
    tmp_time=( int )( ( ltime2*1000+tstruct2.millitm ) - ( ltime1*1000+tstruct1.millitm ) );
    tot_time=tot_time + tmp_time;

    StatBitsPtr->curr_frame_bits = StatBitsPtr->curr_frame_bits*8 + StatBitsPtr->emulate_bits - StatBitsPtr->last_unit_bits;
    StatBitsPtr->bitrate += StatBitsPtr->curr_frame_bits;
    StatBitsPtr->coded_pic_num++;
    if( ( int )( StatBitsPtr->coded_pic_num - ( StatBitsPtr->time_s+1 )*framerate[frame_rate_code-1] + 0.5 ) == 0 )
    {
        StatBitsPtr->total_bitrate[StatBitsPtr->time_s++] = StatBitsPtr->bitrate;
        CheckBitrate( StatBitsPtr->bitrate,StatBitsPtr->time_s );
        StatBitsPtr->bitrate = 0;
    }

    if ( img->type!=B_IMG )
    {
        pre_tmp_time = tmp_time;
        pre_img_tr   = img->poc;
        pre_img_qp   = img->qp;
        StatBitsPtr->prev_frame_bits = StatBitsPtr->curr_frame_bits;
        StatBitsPtr->prev_emulate_bits = StatBitsPtr->emulate_bits;
    }

    if (img->type == B_IMG && p_ref)
    {
        fprintf(stdout, "%3d(B)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s %8d %6d\n",
            img->FrmNum, img->poc, img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time, "FRM", StatBitsPtr->curr_frame_bits, StatBitsPtr->emulate_bits);
    }

    if ( input->check_BBV_flag )
    {
        pBbv->frame_code_bits        = StatBitsPtr->curr_frame_bits;
    }
    StatBitsPtr->curr_frame_bits = 0;
    StatBitsPtr->emulate_bits    = 0;
    StatBitsPtr->last_unit_bits  = 0;

    fflush( stdout );

    if( img->type == I_IMG || img->type == P_IMG ) // I or P pictures
    {
        copy_Pframe( img, img_prev );
        Update_Picture_Buffers();
    }
    else if ( input->bwrite_dec_frm ) // B pictures
    {
        write_frame( img, p_out );    // write image to output YUV file
    }

    if ( input->check_BBV_flag )
    {
        update_bbv( pBbv, pBbv->frame_code_bits );
    }

    if( img->type == I_IMG || img->type == P_IMG ) // I or P pictures
    {
        img->ip_frm_idx++;
    }
    else
    {
        Bframe_ctr++;    // B pictures
        img->FrmNum++;
    }

    return ( SOP );
}

/* ---------------------------------------------------------------------------
* Function   : calculate the MAD of 2 frames
* Parameters :
*      [in ] : width      - width   of frame
*            : height     - height  of frame
*            : rec        - pointer to reconstructed frame buffer
*            : rec_stride - stride  of reconstructed frame
*            : dst        - pointer to decoded frame buffer
*            : dst_stride - stride  of decoded frame
*      [out] : none
* Return     : mad of 2 frames
* ---------------------------------------------------------------------------
*/
static __inline int
cal_mad( int width, int height, uchar_t *rec, int rec_stride, uchar_t *dst, int dst_stride )
{
    int is_error = 0;
    int d = 0;
    int i, j, t;

    for ( j = 0; j < height; j++ )
    {
        for ( i = 0; i < width; i++ )
        {
            t = dst[i] - rec[i];
            d += t * t;
        }
        rec += rec_stride;
        dst += dst_stride;
    }

    if ( is_error )
    {
        fprintf( stderr, "\n" );
    }

    return d;
}

/* ---------------------------------------------------------------------------
* Function   : calculate and output the psnr (only for YUV 4:2:0)
* Parameters :
*      [in ] : rec    - pointer to buffer of reconstructed picture
*            : dst    - pointer to buffer of decoded picture
*            : width  - width  of picture
*            : height - height of picture
*      [out] : none
* Return     : void
* ---------------------------------------------------------------------------
*/
static void cal_psnr( snr_par * snr, uchar_t *rec, uchar_t *dst, int width, int height )
{
    int stride = width;         /* stride of frame/field  (luma) */
    int size_l = width * height;/* size   of frame/field  (luma) */
    int diff;                   /* difference between decoded and reconstructed picture */
    uchar_t *p1;          /* pointer to buffer of reconstructed picture */
    uchar_t *p2;          /* pointer to buffer of decoded picture */
    float fpsnr;

    /* Y */
    p1 = rec;
    p2 = dst;
    diff = cal_mad( width, height, p1, stride, p2, stride );
    if ( diff == 0 )
    {
        fpsnr = 0.0f;
    }
    else
    {
        fpsnr = ( float )( 10 * log10( 65025.0F * size_l / diff ) );
    }
    snr->snr_y = fpsnr;
    snr->snr_sum_y += fpsnr;

    width >>= 1;               /* width  of frame/field  (chroma) */
    height >>= 1;               /* height of frame/field  (chroma, with padding) */
    stride >>= 1;               /* stride of frame/field  (chroma) */

    /* U */
    p1 += size_l;
    p2 += size_l;
    diff = cal_mad( width, height, p1, stride, p2, stride );
    if ( diff == 0 )
    {
        fpsnr = 0.0f;
    }
    else
    {
        fpsnr = ( float )( 10 * log10( 65025.0F * size_l / diff ) );
    }
    snr->snr_u = fpsnr;
    snr->snr_sum_u += fpsnr;

    /* V */
    p1 += size_l / 4;
    p2 += size_l / 4;
    diff = cal_mad( width, height, p1, stride, p2, stride );
    if ( diff == 0 )
    {
        fpsnr = 0.0f;
    }
    else
    {
        fpsnr = ( float )( 10 * log10( 65025.0F * size_l / diff ) );
    }
    snr->snr_v = fpsnr;
    snr->snr_sum_v += fpsnr;
}

void find_snr( snr_par *snr, ImgParams *img, FILE *p_ref, uchar_t *pic_dec_prev )
{
    int i,j;
    int  status;

    // find the real width and height of image
    int img_org_widthc = img->width_org >> 1;
    int img_org_heightc = img->height_org >> 1;

    const i32u_t  bytes_y = img->width_org * img->height_org;
    const i32u_t  bytes_uv = img_org_widthc * img_org_heightc;
    const int framesize_in_bytes = bytes_y + 2*bytes_uv;

    uchar_t *dec_buf = ( uchar_t * )malloc( framesize_in_bytes );
    uchar_t *out_buf = ( uchar_t * )malloc( framesize_in_bytes );
    uchar_t *out_buf_bak = out_buf;

    if( img->type==B_IMG && !eos)
    {
        for( j = 0; j < img->height_org; j++ )
        {
            for( i = 0; i < img->width_org; i++ )
            {
                *out_buf++ = imgY_rec[j*img->iStride+i];
            }
        }
        for( j = 0; j < img_org_heightc; j++ )
        {
            for( i = 0; i < img_org_widthc; i++ )
            {
                *out_buf++ = imgU_rec[j*img->iStrideC+i];
            }
        }
        for( j = 0; j < img_org_heightc; j++ )
        {
            for( i = 0; i < img_org_widthc; i++ )
            {
                *out_buf++ = imgV_rec[j*img->iStrideC+i];
            }
        }
        out_buf -= framesize_in_bytes;
    }
    else
    {
        out_buf = pic_dec_prev;
    }

    // go to the position of corresponding frame in decoding yuv file
    status = fseek ( p_ref, framesize_in_bytes * img->FrmNum, 0 );

    if ( status != 0 )
    {
        fprintf(stderr, "Error in seeking img->tr: %d", img->poc);
    }
    
    fread(dec_buf, img->width_org*img->height_org * 3 / 2, 1, p_ref);
    cal_psnr(snr, dec_buf, out_buf, img->width_org, img->height_org);

    free( dec_buf );
    free( out_buf_bak );
}

/*
*************************************************************************
* Function: Read the content beween two successive start code from the bitstream
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

int find_headers()
{
    uchar_t *Buf;
    int startcodepos,length;
    static unsigned long prev_pos=0 , curr_pos=0;
    static unsigned long prev_pic=0, curr_pic=0;

    if ( ( Buf = ( uchar_t* )calloc ( MAX_CODED_FRAME_SIZE , sizeof( char ) ) ) == NULL )
    {
        no_mem_exit( "GetAnnexbNALU: Buf" );
    }

    while ( 1 )
    {
        next_start_code_pos = GetOneUnit( Buf,&startcodepos,&length );
        StatBitsPtr->curr_frame_bits += length;
        switch( Buf[startcodepos] ) // check the type of start code
        {
            case SEQUENCE_HEADER_CODE:
                if ( input->check_BBV_flag && !img->new_seq_header_flag && pBbv )
                {
                    stat_bbv_buffer( pBbv );
                    pBbv = free_bbv_memory( pBbv );
                    img->new_seq_header_flag = 1;
                }

                img->new_sequence_flag = 1;
                SequenceHeader( Buf,startcodepos,length );
                xInit_seq();
                if ( input->check_BBV_flag && img->new_seq_header_flag )
                {
                    pBbv = init_bbv_memory( frame_rate_code, low_delay, bbv_buffer_size, bit_rate_upper, bit_rate_lower );
                    img->new_seq_header_flag = 0;
                }
                break;
            case USER_DATA_START_CODE:
                user_data( Buf,startcodepos,length );
                break;
            case VIDEO_EDIT_CODE:
                video_edit_code_data( Buf,startcodepos,length );
                break;
            case I_PICTURE_START_CODE:
                curr_pic=prev_pos;
                I_Picture_Header( Buf,startcodepos,length );
                calc_picture_distance(img);
                if ( input->check_BBV_flag )
                {
                    if ( !img->new_sequence_flag )
                    {
                        if ( 0x20==profile_id && ( ( 0xFFFF==img->last_pic_bbv_delay && 0xFFFF!=bbv_delay ) || ( 0xFFFF!=img->last_pic_bbv_delay && 0xFFFF==bbv_delay ) ) )
                        {
                            fprintf( stdout, "If one picture's bbv_delay is 0xFFFF, then all pictures's bbv_delay is 0xFFFF.\n" );
                            bbv_delay = 0xFFFF;
                        }
                        if ( 0x20!=profile_id && ( ( 0xFFFFFF==img->last_pic_bbv_delay && 0xFFFFFF!=bbv_delay ) || ( 0xFFFFFF!=img->last_pic_bbv_delay && 0xFFFFFF==bbv_delay ) ) )
                        {
                            printf( "If one picture's bbv_delay is 0xFFFFFF, then all pictures's bbv_delay is 0xFFFFFF in non JiZhun Profile.\n" );
                            bbv_delay = 0xFFFFFF;
                        }
                    }

                    if ( ( 0x20==profile_id && 0xFFFF==bbv_delay ) || ( 0x20!=profile_id && 0xFFFFFF==bbv_delay ) )
                    {
                        pBbv->bbv_mode = 1;
                    }
                    else
                    {
                        if ( img->new_sequence_flag )
                        {
                            pBbv->bbv_delay = ( float )( bbv_delay/90000.0 );
                        }
                        else if ( pBbv->check_continue && ( bbv_delay<pBbv->bbv_delay*90000*0.95 || bbv_delay>pBbv->bbv_delay*90000*1.05 ) )
                        {
                            fprintf( stdout,"\nThe bbv_delay of frame %d should be %.3f rather than %.3f.\n", img->FrmNum, pBbv->bbv_delay, bbv_delay/90000.0 );
                        }
                    }

                    img->last_pic_bbv_delay = bbv_delay;
                }

                break;
            case PB_PICTURE_START_CODE:
                curr_pic=prev_pos;
                PB_Picture_Header( Buf,startcodepos,length );
                calc_picture_distance( img );

                if ( input->check_BBV_flag )
                {
                    if ( 0x20==profile_id && ( ( 0xFFFF==img->last_pic_bbv_delay && 0xFFFF!=bbv_delay ) || ( 0xFFFF!=img->last_pic_bbv_delay && 0xFFFF==bbv_delay ) ) )
                    {
                        fprintf( stdout, "If one picture's bbv_delay is 0xFFFF, then all pictures's bbv_delay is 0xFFFF.\n" );
                        bbv_delay = 0xFFFF;
                    }
                    if ( 0x20!=profile_id && ( ( 0xFFFFFF==img->last_pic_bbv_delay && 0xFFFFFF!=bbv_delay ) || ( 0xFFFFFF!=img->last_pic_bbv_delay && 0xFFFFFF==bbv_delay ) ) )
                    {
                        printf( "If one picture's bbv_delay is 0xFFFFFF, then all pictures's bbv_delay is 0xFFFFFF in non Baseline Profile.\n" );
                        bbv_delay = 0xFFFFFF;
                    }

                    if ( ( 0x20==profile_id && 0xFFFF==bbv_delay ) || ( 0x20!=profile_id && 0xFFFFFF==bbv_delay ) )
                    {
                        pBbv->bbv_mode = 1;
                    }
                    else if ( pBbv->check_continue && ( bbv_delay<pBbv->bbv_delay*90000*0.95 || bbv_delay>pBbv->bbv_delay*90000*1.05 ) )
                    {
                        fprintf( stdout,"\nThe bbv_delay of frame %d should be %.3f rather than %.3f.\n", img->FrmNum, pBbv->bbv_delay, bbv_delay/90000.0 );
                    }

                    img->last_pic_bbv_delay = bbv_delay;
                }

                break;
            case SEQUENCE_END_CODE:
                img->new_sequence_flag = 1;
                img->sequence_end_flag = 1;
                free( Buf );
                return EOS;
                break;
            default:
                if (Buf[startcodepos] >= SLICE_START_CODE_MIN && Buf[startcodepos] <= SLICE_START_CODE_MAX)
                {
                    if ( ( temp_slice_buf = ( uchar_t* )calloc ( MAX_CODED_FRAME_SIZE , sizeof( char ) ) ) == NULL )
                    {
                        no_mem_exit( "GetAnnexbNALU: Buf" );
                    }
                    first_slice_length=length;
                    first_slice_startpos=startcodepos;
                    memcpy( temp_slice_buf,Buf,length );
                    free( Buf );
                    return SOP;
                }
                else
                {
                    printf( "Can't find start code" );
                    free( Buf );
                    return EOS;
                }
        }
    }
}

/*
*************************************************************************
* Function:Initializes the parameters for a new frame
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void init_frame(ImgParams *img, inp_params *inp, snr_par *snr)
{
    int i;

    if ( img->ip_frm_idx == 0 ) // first picture
    {
    }
    else if( img->type == I_IMG || img->type == P_IMG )
    {
        if ( p_ref )
        {
            find_snr( snr,img,p_ref, img_prev );    // if ref sequence exist
        }

        if( pre_img_type == I_IMG ) // I picture
        {
            if( img->new_sequence_flag == 1 )
            {
                if( img->FrmNum!=0 )
                {
                    printf( "min bbv_buffer_size in bitstream is %d\n", ( ( pminBBSsize>>14 ) + ( pminBBSsize&0x3FFF? 1 : 0 ) ) );
                    if ( !pbbv_mode )
                    {
                        printf( "min initial bbv_delay(0) time is %.4f(s)\n", ( float )pminFsize/pbitrate );
                    }
                }

                if ( img->sequence_end_flag )
                {
                    img->sequence_end_flag = 0;
                    fprintf( stdout, "Sequence End\n\n" );
                }

                fprintf( stdout, "Sequence Header\n" );
                img->new_sequence_flag = 0;
            }
            
            if (p_ref)
            {
                fprintf(stdout, "%3d(I)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s %8d %6d\n",
                    img->FrmNum, pre_img_tr, pre_img_qp, snr->snr_y, snr->snr_u, snr->snr_v, pre_tmp_time, "FRM", StatBitsPtr->prev_frame_bits, StatBitsPtr->prev_emulate_bits);
            }
        }
        else if (pre_img_type == P_IMG) // P pictures
        {
            if (p_ref)
            {
                fprintf(stdout, "%3d(P)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s %8d %6d\n",
                    img->FrmNum, pre_img_tr, pre_img_qp, snr->snr_y, snr->snr_u, snr->snr_v, pre_tmp_time, "FRM", StatBitsPtr->prev_frame_bits, StatBitsPtr->prev_emulate_bits);
            }
        }

        img->FrmNum++;
        if ( input->bwrite_dec_frm )
        {
            write_prev_Pframe( img, p_out, img_prev );
        }
    }

    // allocate memory for frame buffers
    if ( img->ip_frm_idx == 0 )
    {
        init_global_buffers( inp, img );
    }

    for( i=0; i<img->max_mb_nr; i++ )
    {
        img->mb_data[i].slice_nr = -1;
    }

    for( i = 0 ; i < img->real_ref_num ; i++ )
    {
        p_mref[i] = p_ref_frm[i][0];
        p_mcef[i][0] = p_ref_frm[i][1];
        p_mcef[i][1] = p_ref_frm[i][2];
    }

    imgY_rec =  p_cur_frm[0];
    imgU_rec =  p_cur_frm[1];
    imgV_rec =  p_cur_frm[2];
}


void init_frame_buffer()
{
    int i;
    for( i = 0 ; i < img->real_ref_num ; i++ )
    {
        p_mref[i] = p_ref_frm[i][0];
        p_mcef[i][0] = p_ref_frm[i][1];
        p_mcef[i][1] = p_ref_frm[i][2];
    }
}


/*
*************************************************************************
* Function:decodes one picture
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void picture_data( ImgParams *img )
{
    uchar_t *Buf;
    int startcodepos,length;
    int mb_width = img->PicWidthInMbs;
    int first_slice =1;
    CSobj *currSlice = img->cs_aec;
    int    ByteStartPosition;
    int    new_slice=0;
    Boolean   aec_mb_stuffing_bit;
    Macroblock *currMB;

    img->current_slice_nr = -1;
    currentbitoffset = currStream->byte_offset;
    currStream->byte_offset = 0;
    if ( ( Buf = ( uchar_t* )calloc ( MAX_CODED_FRAME_SIZE , sizeof( char ) ) ) == NULL )
    {
        no_mem_exit( "GetAnnexbNALU: Buf" );
    }

    while ( img->current_mb_nr<img->PicSizeInMbs ) // loop over macroblocks
    {
        if( img->current_mb_nr%mb_width ==0 ) // check new slice after decoding every MB row
        {
            if( first_slice )
            {
                SliceHeader( temp_slice_buf,first_slice_startpos,first_slice_length );
                free( temp_slice_buf );
                img->current_slice_nr++;
                first_slice=0;
                new_slice=1;
            }
            else // One Slice ends with another start code appearing
            {
                if( check_slice_stuffing() )
                {
                    GetOneUnit( Buf,&startcodepos,&length );
                    StatBitsPtr->curr_frame_bits += length;
                    SliceHeader( Buf,startcodepos,length );
                    if (img->current_slice_nr == 0)
                    {
                        fprintf(stdout, "decode [SLICE %d]\n", img->current_slice_nr);
                    }
                    img->current_slice_nr++;
                    fprintf(stdout, "decode [SLICE %d]\n", img->current_slice_nr);
                    new_slice=1;
                }
                else
                {
                    new_slice=0;
                }
            }

            if( new_slice )
            {
                init_contexts( img->cs_aec );
                ByteStartPosition = ( currStream->byte_offset )/8;
                img->cs_aec->bitstream = currStream ;
                currStream = currSlice->bitstream;

                if ( ( currStream->byte_offset )%8!=0 )
                {
                    ByteStartPosition++;
                }
                arideco_start_decoding ( &img->cs_aec->de_AEC, currStream->streamBuffer, ( ByteStartPosition ), &( currStream->read_len ) );
            }
        }  //decode slice header
        img->mb_data[img->current_mb_nr].slice_nr = img->current_slice_nr;

#if TRACE
        //fprintf(p_trace, "\n*********** POC: %i MB: %i Slice Idx: %i Type %d **********\n", img->tr, img->current_mb_nr, img->current_slice_nr, img->type);
        fprintf(p_trace, "\n*********** MB: %i Slice Idx: %i Type %d **********\n", img->current_mb_nr, img->current_slice_nr, img->type);
#endif
        init_img_params( img, img->current_mb_nr );
        currMB = &img->mb_data[img->current_mb_nr];
        init_mb_params( currMB );

        read_one_macroblock( img, currMB );
        decode_one_cu( img, currMB, 4 );

        aec_mb_stuffing_bit = read_terminating_bit( img, 1 );
        img->current_mb_nr++;
    }

    free( Buf );
    if (!img->deblk_disable)
    {
        DeblockFrame ( img, imgY_rec, imgU_rec, imgV_rec );
    }

    // padding
    image_padding( img, imgY_rec, imgU_rec, imgV_rec, IMG_PAD_SIZE );
}