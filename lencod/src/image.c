/*
*************************************************************************************
* File name:
* Function:
*
*************************************************************************************
*/
#include "contributors.h"

#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include <string.h>
#include <memory.h>
#include <assert.h>
#include <IO.H>

#include "global.h"
#include "image.h"
#include "refbuf.h"
#include "header.h"
#include "memalloc.h"
#include "bitstream.h"
#include "vlc.h"
#include "configfile.h"
#include "AEC.h"
#include "../../common/common.h"

#ifdef RATECONTROL
#include "fuzzycontrol.h"
#endif

#ifdef TDRDO
#include"tdrdo.h"
#endif


static void code_a_picture( Picture *frame );
static void ReadOneFrame ( int FrameNoInFile, int HeaderSize );
static void write_reconstructed_image();
static int  writeout_picture( Bitstream * bitstrv );
static int  writeout_slice();
static void find_snr();
static void frame_mode_buffer ();
static void init_frame();


int terminate_picture();


void put_buffer_frame();
void interpolate_frame_to_fb();

static void CopyFrameToOrigYUVBuffers ();
static void UnifiedOneForthPix( pel_t * imgY, pel_t *out4Y );
static void UnifiedOneForthPixAdaptive( pel_t * imgY, pel_t *out4Y );
static void ReportFirstframe( int tmp_time );
static void ReportIntra( int tmp_time );
static void ReportP( int tmp_time );
static void ReportB( int tmp_time );

static int FrameNumberInFile;       // The current frame number in the input file

extern void DecideMvRange();

#define  IClip( Min, Max, Val) (((Val)<(Min))? (Min):(((Val)>(Max))? (Max):(Val)))

/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

static void picture_header( Bitstream *bitstream )
{
    int len = 0;
    img->cod_counter = 0;

    if ( img->type == I_IMG )
    {
        len = len + IPictureHeader( bitstream, img->ip_frm_idx );
        img->count = img->count + 2;
    }
    else
    {
        len = len + PBPictureHeader( bitstream );
    }

    // Update statistics
    stat->bit_slice += len;
    stat->bit_use_header[img->type] += len;
}


/*
*************************************************************************
* Function:Encodes one frame
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

int encode_one_frame()
{
    time_t ltime1;
    time_t ltime2;

#ifdef WIN32
    struct _timeb tstruct1;
    struct _timeb tstruct2;
#else
    struct timeb tstruct1;
    struct timeb tstruct2;
#endif

    int tmp_time;

#ifdef WIN32
    _ftime( &tstruct1 );         // start time ms
#else
    ftime ( &tstruct1 );
#endif
    time( &ltime1 );             // start time s

    init_frame();           // initial frame variables

    FrameNumberInFile = img->poc;

    ReadOneFrame( FrameNumberInFile, input->infile_header );

#ifdef TDRDO
    if ( input->TRDOLength != 0 )
    {
        GlobeFrameNumber = FrameNumberInFile;
        pRealFD = &RealDList->FrameDistortionArray[GlobeFrameNumber / StepLength];
        pRealFD->BlockDistortionArray = ( BD * )calloc( pRealFD->TotalNumOfBlocks, sizeof( BD ) );
        if ( GlobeFrameNumber % StepLength == 0 )
        {
            if ( FrameNumberInFile == 0 )
            {
                porgF->base = imgY_org_buffer;
                porgF->Y = porgF->base;
                porgF->U = porgF->Y + porgF->FrameWidth * porgF->FrameHeight;
                porgF->V = porgF->U + porgF->FrameWidth * porgF->FrameHeight / 4;
                ppreF->base = imgY_pre_buffer;
                ppreF->Y = ppreF->base;
                ppreF->U = ppreF->Y + ppreF->FrameWidth * ppreF->FrameHeight;
                ppreF->V = ppreF->U + ppreF->FrameWidth * ppreF->FrameHeight / 4;
                memcpy( imgY_pre_buffer, imgY_org_buffer, input->width_org*input->height_org * 3 / 2 * sizeof( pel_t ) );
            }
            else  if ( FrameNumberInFile < input->FrmsToBeEncoded )
            {
                if ( !input->successive_Bframe )
                {
                    pOMCPFD = &OMCPDList->FrameDistortionArray[FrameNumberInFile / StepLength - 1];
                    pOMCPFD->BlockDistortionArray = ( BD * )calloc( pOMCPFD->TotalNumOfBlocks, sizeof( BD ) );
                    MotionDistortion( pOMCPFD, ppreF, porgF, SEARCHRANGE );
                    memcpy( imgY_pre_buffer, imgY_org_buffer, input->width_org*input->height_org * 3 / 2 * sizeof( pel_t ) );
                }
                if ( input->successive_Bframe && FrameNumberInFile + StepLength < input->FrmsToBeEncoded )
                {
                    if ( FrameNumberInFile == StepLength )
                    {
                        pOMCPFD = &OMCPDList->FrameDistortionArray[FrameNumberInFile / StepLength - 1];
                        pOMCPFD->BlockDistortionArray = ( BD * )calloc( pOMCPFD->TotalNumOfBlocks, sizeof( BD ) );
                        MotionDistortion( pOMCPFD, ppreF, porgF, SEARCHRANGE );
                        memcpy( imgY_pre_buffer, imgY_org_buffer, input->width_org*input->height_org * 3 / 2 * sizeof( pel_t ) );
                    }
                    FrameNumberInFile += StepLength;
                    ReadOneFrame( FrameNumberInFile, input->infile_header );
                    pOMCPFD = &OMCPDList->FrameDistortionArray[FrameNumberInFile / StepLength - 1];
                    pOMCPFD->BlockDistortionArray = ( BD * )calloc( pOMCPFD->TotalNumOfBlocks, sizeof( BD ) );
                    MotionDistortion( pOMCPFD, ppreF, porgF, SEARCHRANGE );
                    memcpy( imgY_pre_buffer, imgY_org_buffer, input->width_org*input->height_org * 3 / 2 * sizeof( pel_t ) );
                    FrameNumberInFile = GlobeFrameNumber;
                    ReadOneFrame( FrameNumberInFile, input->infile_header );
                }
            }
            pOMCPFD = NULL;
        }

    }
#endif

#ifdef RATECONTROL
    if ( input->RCEnable )
    {
        if ( pRC->IntraPeriod == 0 )
        {
            if ( FrameNumberInFile != 0 )
            {
                pRC->DeltaQP = CalculateGopDeltaQP_RateControl( pRC, img->type, gop_size_all );
            }
        }
        else if ( pRC->IntraPeriod == 1 )
        {
            if ( ( FrameNumberInFile%gop_size_all == 0 ) && ( FrameNumberInFile != 0 ) )
            {
                pRC->DeltaQP = CalculateGopDeltaQP_RateControl( pRC, img->type, gop_size_all );
            }
        }
        else
        {
            if ( ( img->type == 0 ) && ( ( pRC->TotalFrames - pRC->CodedFrameNumber ) <= ( pRC->IntraPeriod * gop_size_all ) ) )
            {
                Init_FuzzyController( 0.50 );
            }
            if ( ( FrameNumberInFile%gop_size_all == 0 ) && ( FrameNumberInFile != 0 ) )
            {
                pRC->DeltaQP = CalculateGopDeltaQP_RateControl( pRC, img->type, gop_size_all );
            }
            else if ( pRC->TotalFrames - pRC->CodedFrameNumber == ( pRC->TotalFrames - 1 ) % gop_size_all )
            {
                pRC->DeltaQP = CalculateGopDeltaQP_RateControl( pRC, img->type, gop_size_all );
            }
        }

        if ( ( pRC->CodedFrameNumber % gop_size_all == 1 ) || ( pRC->IntraPeriod == 1 ) || ( pRC->IntraPeriod == 0 ) ) //
        {
            if ( pRC->IntraPeriod > 1 && img->type == 0 )
            {
                if ( ( ( pRC->TotalFrames - pRC->CodedFrameNumber ) < ( pRC->IntraPeriod * gop_size_all ) ) && ( pRC->FinalGopFrames / gop_size_all < 3 ) )
                {
                    pRC->DeltaQP += 6;
                }
                img->frame_qp = pRC->GopAvgBaseQP / pRC->GopAvgBaseQPCount + pRC->DeltaQP - 3;
                input->qp_1st_frm = img->frame_qp;
            }
            else
            {
                img->frame_qp += pRC->DeltaQP;
                input->qp_1st_frm += pRC->DeltaQP;
            }
            img->frame_qp = MAX(MIN_QP, MIN(img->frame_qp, MAX_QP - 6));
            input->qp_1st_frm = MAX( MIN_QP, MIN( input->qp_1st_frm, MAX_QP - 6 ) );
            input->qp_P_frm = input->qp_1st_frm + 2;
            input->qp_B_frm = input->qp_1st_frm + 6;
        }
    }
#endif

    CopyFrameToOrigYUVBuffers();

    current_slice_bytepos = 0;

    //prepare buffer pointer for encoding process
#ifdef TDRDO
    if ( input->TRDOLength != 0 && GlobeFrameNumber % StepLength == 0 && GlobeFrameNumber<input->FrmsToBeEncoded - 1 )
    {
        if ( input->successive_Bframe && GlobeFrameNumber>StepLength )
        {
            CaculateKappaTableRA(OMCPDList, RealDList, GlobeFrameNumber, img->frame_qp);
        }
        if ( !input->successive_Bframe && GlobeFrameNumber > 0 )
        {
            CaculateKappaTableLDP(OMCPDList, RealDList, GlobeFrameNumber, img->frame_qp);
        }
    }
#endif

    put_buffer_frame();     //initialize frame buffer

    DecideMvRange();

    if ( img->type == B_IMG )
    {
        Bframe_ctr++;         // Bframe_ctr only used for statistics, should go to stat->
    }

    code_a_picture( frame_pic );

    //update the reference picture buffer if necessary
    if ( img->type != B_IMG )
    {
        Update_Picture_Buffers();
    }

    //interpolating the reconstructed frame if necessary
    frame_mode_buffer();

    find_snr();

#ifdef TDRDO
    if ( input->TRDOLength != 0 )
    {
        int DelFDNumber;
        FD *pDelFD;
        precF->base = NULL;

        precF->Y = imgY_rec;
        precF->U = imgU_rec;
        precF->V = imgV_rec;

        // reassign the nStrideY and nStrideC for padding
        precF->nStrideY = img->width + 2 * IMG_PAD_SIZE;
        precF->nStrideC = precF->nStrideY >>1;

        if ( FrameNumberInFile % StepLength == 0 )
        {
            MotionDistortion( pRealFD, porgF, precF, 0 );
        }

        pRealFD->FrameNumber = FrameNumberInFile;

        DelFDNumber = FrameNumberInFile / StepLength - 2;
        if ( DelFDNumber >= 0 )
        {
            pDelFD = &RealDList->FrameDistortionArray[DelFDNumber];
            if ( pDelFD->BlockDistortionArray != NULL )
            {
                free( pDelFD->BlockDistortionArray );
            }
            pDelFD->BlockDistortionArray = NULL;
        }
        if ( FrameNumberInFile % StepLength == 0 )
        {
            DelFDNumber = FrameNumberInFile / StepLength - 2;
            if ( DelFDNumber >= 0 )
            {
                pDelFD = &OMCPDList->FrameDistortionArray[DelFDNumber];
                if ( pDelFD->BlockDistortionArray != NULL )
                {
                    free( pDelFD->BlockDistortionArray );
                }
                pDelFD->BlockDistortionArray = NULL;
            }
        }
    }
#endif

#ifdef RATECONTROL
    Updata_RateControl(pRC, stat->bit_ctr - stat->bit_ctr_n, img->frame_qp, img->type, FrameNumberInFile, gop_size_all);
#endif
    time( &ltime2 );             // end time sec
#ifdef WIN32
    _ftime ( &tstruct2 );         // end time ms
#else
    ftime( &tstruct2 );          // end time ms
#endif

    tmp_time = ( int )( ( ltime2 * 1000 + tstruct2.millitm ) - ( ltime1 * 1000 + tstruct1.millitm ) );
    tot_time = tot_time + tmp_time;

    // Write reconstructed images
    if ( input->output_enc_pic ) //output_enc_pic
    {
        write_reconstructed_image();
    }

    if ( img->ip_frm_idx == 0 )
    {
        ReportFirstframe( tmp_time );
    }
    else
    {
        switch ( img->type )
        {
            case I_IMG:
                //stat->bit_ctr_P += stat->bit_ctr - stat->bit_ctr_n;
                stat->bit_ctr_0 += stat->bit_ctr - stat->bit_ctr_n;
                if ( input->seqheader_period != 0 && ( img->ip_frm_idx + 1 ) % ( input->seqheader_period*input->intra_period ) == 0 && input->successive_Bframe == 0 )
                {
                    stat->bit_ctr_0 += 32;
                    stat->bit_ctr += 32;
                    stat->bit_use_stuffingBits[0] += 32;
                }
                ReportIntra( tmp_time );
                break;
            case B_IMG:
                stat->bit_ctr_B += stat->bit_ctr - stat->bit_ctr_n;
                if ( input->seqheader_period != 0 && ( img->ip_frm_idx + 1 ) % ( input->seqheader_period*input->intra_period ) == 0 && img->b_frame_to_code == input->successive_Bframe )
                {
                    stat->bit_ctr_B += 32;
                    stat->bit_ctr += 32;
                    stat->bit_use_stuffingBits[2] += 32;
                }
                ReportB( tmp_time );
                break;
            default:      // P, P_MULTPRED?
                stat->bit_ctr_P += stat->bit_ctr - stat->bit_ctr_n;
                if ( input->seqheader_period != 0 && ( img->ip_frm_idx + 1 ) % ( input->seqheader_period*input->intra_period ) == 0 && input->successive_Bframe == 0 )
                {
                    stat->bit_ctr_P += 32;
                    stat->bit_ctr += 32;
                    stat->bit_use_stuffingBits[1] += 32;
                }
                ReportP( tmp_time );
        }
    }
    {
        if ( img->ip_frm_idx != 0 )
        {
            img->SUM_bit = stat->bit_ctr - stat->bit_ctr_n;
        }
        {
            if ( ( img->ip_frm_idx ) % HPGOPSIZE == 1 )
            {
                img->p_one_bit = stat->bit_ctr - stat->bit_ctr_n;
            }
            else if ( ( img->ip_frm_idx ) % HPGOPSIZE == 3 )
            {
                img->p_three_bit = stat->bit_ctr - stat->bit_ctr_n;
            }
        }
    }
    stat->bit_ctr_n = stat->bit_ctr;

    if ( img->ip_frm_idx == 0 )
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/*
*************************************************************************
* Function:This function write out a picture
* Input:
* Output:
* Return: 0 if OK,                                                         \n
1 in case of error
* Attention:
*************************************************************************
*/

static int writeout_picture( Bitstream * bitstr )
{

    assert ( bitstr->bits_to_go == 8 );  //! should always be the case,
    //! the byte alignment is done in terminate_slice
    WriteBitstreamtoFile( bitstr );

    return 0;
}

/*
*************************************************************************
* Function:Encodes a frame picture
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

static void code_a_picture ( Picture *frame )
{
    CSobj aec;
    Bitstream *currStream;
    img->cs_aec = &aec;
    img->cs_aec->mot_ctx = create_contexts_MotionInfo();
    img->cs_aec->tex_ctx = create_contexts_TextureInfo();

    img->cs_aec->bitstream = AllocateBitstream();
    currStream = img->cs_aec->bitstream;

    picture_header( currStream );

    if ( img->type == I_IMG || img->type == P_IMG )
    {
        img->p_sub_type_start = 0;
    }
    picture_data( frame, img->cs_aec );

    writeout_picture( currStream );
    FreeBitstream( currStream );

    delete_contexts_MotionInfo( img->cs_aec->mot_ctx );
    delete_contexts_TextureInfo( img->cs_aec->tex_ctx );
}

/*
*************************************************************************
* Function:Frame Mode Buffer
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

static void frame_mode_buffer ()
{
    if ( img->type != B_IMG && img->typeb<=L1_IMG )     //all I- and P-frames
    {
        interpolate_frame_to_fb ();
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
static void init_frame ()
{
    int i, j, k;
    int widthMB, heightMB;

    img->current_mb_nr = 0;
    img->current_slice_nr = 0;

    widthMB = img->width  / MB_SIZE;
    heightMB = img->height / MB_SIZE;
    img->currSliceLastMBNr = ( input->slice_row_nr != 0 )
                                 ? MIN( input->slice_row_nr * widthMB - 1, widthMB * heightMB - 1 )
                                 : widthMB * heightMB - 1 ;

    stat->bit_slice = 0;
    img->coded_mb_nr = 0;

    img->mb_y = img->mb_x = 0;
    img->mb_b4_y = img->mb_pix_y = img->mb_pix_y_cr = 0;
    img->mb_b4_x = img->mb_pix_x = img->block_c_x = img->mb_pix_x_cr = 0;

#if MULQP
    srand((i32u_t)time(NULL));
    input->qp_1st_frm = (rand() % 62) + 1;
#endif

    if ( img->type != B_IMG )
    {
        img->poc = img->ip_frm_idx * img->pfrm_interval;

        img->imgtr_prev_P = img->imgtr_last_P; // also equal to (( img->PfrmIdx - 1 ) * img->pfrm_interval % 256)
        // for P and I frames, imgtr_next_P is equal to pic_distance, but for B frames, is not.
        img->pic_distance = img->poc % 256;
        img->imgtr_last_P = img->imgtr_next_P;
        img->imgtr_next_P = img->pic_distance;

        // Qp setting
        if( !input->RCEnable )
        {
            if ( img->type == I_IMG )
            {
                img->frame_qp = input->qp_1st_frm;   // set quant. parameter for I-frame
            }
            else
            {
                if( input->use_p_sub_type && img->tag_hp )
                {
                    if ((img->ip_frm_idx - img->i_frm_idx) % HPGOPSIZE == 0)
                    {
                        img->frame_qp = input->qp_P_frm;
                    }
                    else if ((img->ip_frm_idx - img->i_frm_idx) % HPGOPSIZE == HPGOPSIZE / 2)
                    {
                        img->frame_qp = input->qp_P_frm + input->p_sub_type_delta1;
                    }
                    else
                    {
                        img->frame_qp = input->qp_P_frm + input->p_sub_type_delta0;
                    }
                }
                else
                {
                    img->frame_qp = input->qp_P_frm;
                }
            }
        }
    }
    else
    {
        img->b_interval = (int)((float)(img->pfrm_interval) / (input->successive_Bframe + 1.0) + 0.49999);

        img->poc = (img->ip_frm_idx - 1) * img->pfrm_interval + img->b_interval * img->b_frame_to_code;

        img->pic_distance = img->poc % 256;

        if( !input->RCEnable )
        {
            img->frame_qp = input->qp_B_frm;
        }

        // initialize arrays
        for ( i = 0; i < img->height / B8_SIZE; i++ )
        {
            for ( j = 0; j < img->width / B8_SIZE + 4; j++ )
            {
                for ( k = 0; k < 2; k++ )
                {
                    img->bfrm_fmv[i][j][k] = 0;
                    img->bfrm_bmv[i][j][k] = 0;
                }
            }
        }

        for ( i = 0; i < img->height / BLOCK_SIZE; i++ )
        {
            for ( j = 0; j < img->width / BLOCK_SIZE; j++ )
            {
                img->bfrm_fref[i][j] = img->bfrm_bref[i][j] = -1;
            }
        }
    }
    img->poc = MIN(img->poc, input->FrmsToBeEncoded - 1);
    img->total_number_mb = ( img->width * img->height ) / ( MB_SIZE * MB_SIZE );

}
/*
*************************************************************************
* Function:Writes reconstructed image(s) to file
This can be done more elegant!
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

static void write_reconstructed_image ()
{
    int i, j, k;
    int start = 0, inc = 1;

#ifdef _BUGFIX_NON16MUL_PICSIZE
    int writeout_width = input->width_org;
    int writeout_height = input->height_org;
    int writeout_width_cr = writeout_width / 2;
    int writeout_height_cr = writeout_height / 2;
#endif

    uchar_t *buf;
    buf = ( uchar_t * )malloc( writeout_width * writeout_height );

    if ( p_dec != -1 )
    {
        if ( img->type != B_IMG )
        {
          // write reconstructed image in AI/LDP(IPPP), or the last P frame
          if ((input->successive_Bframe == 0) || ((img->poc == input->FrmsToBeEncoded - 1) && ((input->FrmsToBeEncoded % (input->successive_Bframe + 1) == 2) || (input->successive_Bframe == 1))))
            {
                for ( j = 0; j < writeout_height; j++ )
                {
                    memcpy( buf + j * writeout_width, &( imgY_rec[j*( img->iStride ) + 0] ), writeout_width );
                }
                _write( p_dec, buf, writeout_height*writeout_width );

                for ( j = 0; j < writeout_height_cr; j++ )
                {
                    memcpy( buf + j * writeout_width_cr, &( imgU_rec[j*( img->iStrideC ) + 0] ), writeout_width_cr );
                }
                _write( p_dec, buf, writeout_height_cr*writeout_width_cr );

                for ( j = 0; j < writeout_height_cr; j++ )
                {
                    memcpy( buf + j * writeout_width_cr, &( imgV_rec[j*( img->iStrideC ) + 0] ), writeout_width_cr );
                }
                _write( p_dec, buf, writeout_height_cr*writeout_width_cr );
            }
            // write intra reconstructed image in RA (IBPBP)
            else if ( img->ip_frm_idx == 0 && input->successive_Bframe != 0 )
            {
                for ( j = 0; j < writeout_height; j++ )
                {
                    memcpy( buf + j * writeout_width, &( imgY_rec[j*( img->iStride ) + 0] ), writeout_width );
                }
                _write( p_dec, buf, writeout_height*writeout_width );

                for ( j = 0; j < writeout_height_cr; j++ )
                {
                    memcpy( buf + j * writeout_width_cr, &( imgU_rec[j*( img->iStrideC ) + 0] ), writeout_width_cr );
                }
                _write( p_dec, buf, writeout_height_cr*writeout_width_cr );

                for ( j = 0; j < writeout_height_cr; j++ )
                {
                    memcpy( buf + j * writeout_width_cr, &( imgV_rec[j*( img->iStrideC ) + 0] ), writeout_width_cr );
                }
                _write( p_dec, buf, writeout_height_cr*writeout_width_cr );
            }
            // next P picture. This is saved with recon B picture after B picture coding
            else if ( img->ip_frm_idx != 0 && input->successive_Bframe != 0 )
            {
                for ( i = start; i < writeout_height; i += inc )
                {
                    for ( j = 0; j < writeout_width; j++ )
                    {
                        nextP_imgY[i][j] = imgY_rec[i*( img->iStride ) + j];
                    }
                }

                for ( k = 0; k < 2; ++k )
                {
                    for ( i = start; i < writeout_height_cr; i += inc )
                    {
                        for ( j = 0; j < writeout_width_cr; j++ )
                        {
                            if ( 0 == k )
                            {
                                nextP_imgUV[k][i][j] = imgU_rec[i*( img->iStrideC ) + j];
                            }
                            else
                            {
                                nextP_imgUV[k][i][j] = imgV_rec[i*( img->iStrideC ) + j];
                            }

                        }
                    }
                }
            }
        }
        else
        {
            for ( j = 0; j < writeout_height; j++ )
            {
                memcpy( buf + j * writeout_width, &( imgY_rec[j*( img->iStride ) + 0] ), writeout_width );
            }
            _write( p_dec, buf, writeout_height*writeout_width );

            for ( j = 0; j < writeout_height_cr; j++ )
            {
                memcpy( buf + j * writeout_width_cr, &( imgU_rec[j*( img->iStrideC ) + 0] ), writeout_width_cr );
            }
            _write( p_dec, buf, writeout_height_cr*writeout_width_cr );

            for ( j = 0; j < writeout_height_cr; j++ )
            {
                memcpy( buf + j * writeout_width_cr, &( imgV_rec[j*( img->iStrideC ) + 0] ), writeout_width_cr );
            }
            _write( p_dec, buf, writeout_height_cr*writeout_width_cr );

            if ( ( img->b_frame_to_code == input->successive_Bframe ) || ( img->poc == input->FrmsToBeEncoded - 2 ) )
            {
                for ( j = 0; j < writeout_height; j++ )
                {
                    memcpy( buf + j * writeout_width, &( nextP_imgY[j][0] ), writeout_width );
                }
                _write( p_dec, buf, writeout_height*writeout_width );

                for ( j = 0; j < writeout_height_cr; j++ )
                {
                    memcpy( buf + j * writeout_width_cr, &( nextP_imgUV[0][j][0] ), writeout_width_cr );
                }
                _write( p_dec, buf, writeout_height_cr*writeout_width_cr );

                for ( j = 0; j < writeout_height_cr; j++ )
                {
                    memcpy( buf + j * writeout_width_cr, &( nextP_imgUV[1][j][0] ), writeout_width_cr );
                }
                _write( p_dec, buf, writeout_height_cr*writeout_width_cr );
            }
        }
    }

    free( buf );
}

/*
*************************************************************************
* Function:Choose interpolation method depending on MV-resolution
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void interpolate_frame_to_fb ()
{
    // write to mref[]
    if ( input->if_type == 0 )
    {
        //UnifiedOneForthPix(imgY, imgU, imgV, mref[0]);
        UnifiedOneForthPix( imgY_rec, mref[0] );
    }
    else
    {
        //UnifiedOneForthPixAdaptive(imgY, imgU, imgV, mref[0]);
        UnifiedOneForthPixAdaptive( imgY_rec, mref[0] );
    }
}

/*
*************************************************************************
* Function:Upsample 4 times, store them in out4x.  Color is simply copied
* Input:srcy, srcu, srcv, out4y, out4u, out4v
* Output:
* Return:
* Attention:Side Effects_
Uses (writes) img4Y_tmp.  This should be moved to a static variable
in this module
*************************************************************************
*/
#define  IClip( Min, Max, Val) (((Val)<(Min))? (Min):(((Val)>(Max))? (Max):(Val)))

/* get 1 1/2 1/4 3/4 sub-pixel value */
static void UnifiedOneForthPix( pel_t *imgY, pel_t *out4Y )
{
    int is;
    int i, j;
    int jj, maxy;

    // IMG_SUBPIXEL_EXTRAPAD_SIZE+IMG_PAD_SIZE
    int subpixel_istride = ( img->width + 2 * ( IMG_SUBPIXEL_PAD_SIZE + IMG_SUBPIXEL_PAD_SIZE_FOR_CLIP ) ) * 4;
    int offset_subpixel = subpixel_istride*IMG_SUBPIXEL_PAD_SIZE_FOR_CLIP + IMG_SUBPIXEL_PAD_SIZE_FOR_CLIP;

    // offset the img4Y_tmp
    int *ini_pos = img4Y_tmp;
    int *img4Y_tmp_padding;
    img4Y_tmp = ini_pos + offset_subpixel;

    //horizontal 1/4, 1/2, 3/4 interpolation
    for ( j = -IMG_SUBPIXEL_PAD_SIZE; j < img->height + IMG_SUBPIXEL_PAD_SIZE; j++ )
    {
        for ( i = -IMG_SUBPIXEL_PAD_SIZE; i < img->width + IMG_SUBPIXEL_PAD_SIZE; i++ )
        {
            jj = j;

            //horizontal 1/2, filter coefficients: -1, 4, -11, 40, 40, -11, 4, -1
            is = 40 * ( imgY[jj*( img->iStride ) + i] + imgY[jj*( img->iStride ) + i + 1] )
                 + 4 * ( imgY[jj*( img->iStride ) + i - 2] + imgY[jj*( img->iStride ) + i + 3] )
                 - 11 * ( imgY[jj*( img->iStride ) + i - 1] + imgY[jj*( img->iStride ) + i + 2] )
                 - ( imgY[jj*( img->iStride ) + i - 3] + imgY[jj*( img->iStride ) + i + 4] );

            //store horizontal 1/2 pixel value
            img4Y_tmp[( ( j + IMG_SUBPIXEL_PAD_SIZE ) * 4 )*subpixel_istride + ( i + IMG_SUBPIXEL_PAD_SIZE ) * 4 + 2] = is;

            //horizontal 1/4, filter coefficients: -1, 4, -10, 57, 18, -6, 3, -1
            is = ( 57 * imgY[jj*( img->iStride ) + i] + 18 * imgY[jj*( img->iStride ) + i + 1]
                   + 4 * imgY[jj*( img->iStride ) + i - 2] + 3 * imgY[jj*( img->iStride ) + i + 3] )
                 - ( 10 * imgY[jj*( img->iStride ) + i - 1] + 6 * imgY[jj*( img->iStride ) + i + 2]
                     + imgY[jj*( img->iStride ) +  i - 3] + imgY[jj*( img->iStride ) + i + 4] );

            //store horizontal 1/4 pixel value
            img4Y_tmp[( ( j + IMG_SUBPIXEL_PAD_SIZE ) * 4 ) * subpixel_istride + ( i + IMG_SUBPIXEL_PAD_SIZE ) * 4 + 1] = is;

            //horizontal 3/4, filter coefficients: -1, 3, -6, 18, 57, -10, 4, -1
            is = ( 18 * imgY[jj*( img->iStride ) + i] + 57 * imgY[jj*( img->iStride ) + i + 1]
                   + 3 * imgY[jj*( img->iStride ) + i - 2] + 4 * imgY[jj*( img->iStride ) + i + 3] )
                 - ( 6 * imgY[jj*( img->iStride ) + i - 1] + 10 * imgY[jj*( img->iStride ) + i + 2]
                     + imgY[jj*( img->iStride ) + i - 3] + imgY[jj*( img->iStride ) + i + 4] );

            //store horizontal 3/4 pixel value
            img4Y_tmp[( ( j + IMG_SUBPIXEL_PAD_SIZE ) * 4 )*subpixel_istride + ( i + IMG_SUBPIXEL_PAD_SIZE ) * 4 + 3] = is;

            //store 1/1 pixel value
            img4Y_tmp[( ( j + IMG_SUBPIXEL_PAD_SIZE ) * 4 )* subpixel_istride + ( i + IMG_SUBPIXEL_PAD_SIZE ) * 4] = imgY[ j*( img->iStride ) + i];
        }
    }

    //vertical 1/4, 1/2, 3/4 interpolation
    for ( i = 0; i < ( img->width + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4; i += 4 )
    {
        for ( j = 0; j < ( img->height + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4; j += 4 )
        {
            maxy = ( img->height + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4 - 4;

            //vertical 1/2, filter coefficients: -1, 4, -11, 40, 40, -11, 4, -1
            is = 40 * ( img4Y_tmp[j* subpixel_istride + i] + img4Y_tmp[MIN( maxy, j + 4 )* subpixel_istride + i] )
                 + 4 * ( img4Y_tmp[MAX( 0, j - 8 )* subpixel_istride + i] + img4Y_tmp[MIN( maxy, j + 12 )* subpixel_istride + i] )
                 - 11 * ( img4Y_tmp[MAX( 0, j - 4 )* subpixel_istride + i] + img4Y_tmp[MIN( maxy, j + 8 )* subpixel_istride + i] )
                 - ( img4Y_tmp[MAX( 0, j - 12 )* subpixel_istride + i] + img4Y_tmp[MIN( maxy, j + 16 )* subpixel_istride + i] );

            img4Y_tmp[( j + 2 ) * subpixel_istride + i] = is; //store vertical 1/2 pixel value

            //vertical 1/4, filter coefficients; -1, 4, -10, 57, 18, -6, 3, -1
            is = ( 57 * img4Y_tmp[j* subpixel_istride + i] + 18 * img4Y_tmp[MIN( maxy, j + 4 )* subpixel_istride + i]
                   + 4 * img4Y_tmp[MAX( 0, j - 8 )* subpixel_istride + i] + 3 * img4Y_tmp[MIN( maxy, j + 12 )* subpixel_istride + i] )
                 - ( 10 * img4Y_tmp[MAX( 0, j - 4 )* subpixel_istride + i] + 6 * img4Y_tmp[MIN( maxy, j + 8 )* subpixel_istride +i]
                     + img4Y_tmp[MAX( 0, j - 12 )* subpixel_istride + i] + img4Y_tmp[MIN( maxy, j + 16 )* subpixel_istride + i] );

            img4Y_tmp[( j + 1 ) * subpixel_istride + i] = is; //store vertical 1/4 pixel value

            //vertical 3/4, filter coefficients: -1, 3, -6, 18, 57, -10, 4, -1
            is = ( 18 * img4Y_tmp[j* subpixel_istride + i] + 57 * img4Y_tmp[MIN( maxy, j + 4 )* subpixel_istride + i]
                   + 3 * img4Y_tmp[MAX( 0, j - 8 )* subpixel_istride +i] + 4 * img4Y_tmp[MIN( maxy, j + 12 )* subpixel_istride +i] )
                 - ( 6 * img4Y_tmp[MAX( 0, j - 4 )* subpixel_istride +i] + 10 * img4Y_tmp[MIN( maxy, j + 8 )* subpixel_istride +i]
                     + img4Y_tmp[MAX( 0, j - 12 )* subpixel_istride +i] + img4Y_tmp[MIN( maxy, j + 16 )* subpixel_istride +i] );

            img4Y_tmp[( j + 3 )* subpixel_istride +i] = is; //store vertical 3/4 pixel value

        }
    }

    // --------------------- padding ---------------------
    img4Y_tmp_padding = img4Y_tmp - subpixel_istride;
    for ( i = 0; i < IMG_SUBPIXEL_PAD_SIZE_FOR_CLIP; i++ )
    {
        memcpy( img4Y_tmp_padding - i*subpixel_istride,
                img4Y_tmp,
                ( img->width + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4 * sizeof( int ) );
    }

    img4Y_tmp_padding = img4Y_tmp + ( img->height + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4 * subpixel_istride;
    for ( i = 0; i < IMG_SUBPIXEL_PAD_SIZE_FOR_CLIP; i++ )
    {
        memcpy( img4Y_tmp_padding + i*subpixel_istride,
                &img4Y_tmp[( ( img->height + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4 - 4 )*subpixel_istride],
                ( img->width + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4 * sizeof( int ) );
    }
    // --------------------- padding ---------------------

    //vertical interpolation, remaining 9 fractional-pel pixels
    //for (i = 0; i < (img->width + 2 * IMG_SUBPIXEL_PAD_SIZE) * 4; i++)
    //{
    //  for (j = 0; j < (img->height + 2 * IMG_SUBPIXEL_PAD_SIZE) * 4; j += 4)
    //  {
    //      // e,i,p; f,j,q; g,k,r
    //      if (i % 4 != 0)//namely (i % 4 == 1)||(i % 4 == 2)||(i % 4 == 3)
    //      {
    //          //e,or f, or g. vertical 1/4 interpolation, filter coefficients: 2, -9, 57, 17, -4, 1
    //          img4Y_tmp[(j + 1)* subpixel_istride + i] = IClip(0, 255,
    //              (int)((2 * img4Y_tmp[IClip(0, (img->height + 2 * IMG_SUBPIXEL_PAD_SIZE) * 4 - 4, j - 8)* subpixel_istride + i] +
    //              57 * img4Y_tmp[IClip(0, (img->height + 2 * IMG_SUBPIXEL_PAD_SIZE) * 4 - 4, j)* subpixel_istride +i] +
    //              17 * img4Y_tmp[IClip(0, (img->height + 2 * IMG_SUBPIXEL_PAD_SIZE) * 4 - 4, j + 4)* subpixel_istride +i] +
    //              1 * img4Y_tmp[IClip(0, (img->height + 2 * IMG_SUBPIXEL_PAD_SIZE) * 4 - 4, j + 12)* subpixel_istride +i] -
    //              9 * img4Y_tmp[IClip(0, (img->height + 2 * IMG_SUBPIXEL_PAD_SIZE) * 4 - 4, j - 4)* subpixel_istride +i] -
    //              4 * img4Y_tmp[IClip(0, (img->height + 2 * IMG_SUBPIXEL_PAD_SIZE) * 4 - 4, j + 8)* subpixel_istride +i] + 2048) / 4096)
    //              );

    //          //i,or j, or k. vertical 1/2 interpolation, filter coefficients: 2, -9, 39, 39, -9, 2
    //          img4Y_tmp[(j + 2)* subpixel_istride + i] = IClip(0, 255,
    //              (int)((2 * (img4Y_tmp[IClip(0, (img->height + 2 * IMG_SUBPIXEL_PAD_SIZE) * 4 - 4, j - 8)* subpixel_istride +i] +
    //              img4Y_tmp[IClip(0, (img->height + 2 * IMG_SUBPIXEL_PAD_SIZE) * 4 - 4, j + 12)* subpixel_istride +i]) +
    //              39 * (img4Y_tmp[IClip(0, (img->height + 2 * IMG_SUBPIXEL_PAD_SIZE) * 4 - 4, j)* subpixel_istride +i] +
    //              img4Y_tmp[IClip(0, (img->height + 2 * IMG_SUBPIXEL_PAD_SIZE) * 4 - 4, j + 4)* subpixel_istride +i]) -
    //              9 * (img4Y_tmp[IClip(0, (img->height + 2 * IMG_SUBPIXEL_PAD_SIZE) * 4 - 4, j - 4)* subpixel_istride +i] +
    //              img4Y_tmp[IClip(0, (img->height + 2 * IMG_SUBPIXEL_PAD_SIZE) * 4 - 4, j + 8)* subpixel_istride +i]) + 2048) / 4096)
    //              );

    //          //p, or q, or r.vertical 3/4 interpolation, filter coefficients: 1, -4, 17, 57, -9, 2
    //          img4Y_tmp[(j + 3)* subpixel_istride + i] = IClip(0, 255,
    //              (int)((1 * img4Y_tmp[IClip(0, (img->height + 2 * IMG_SUBPIXEL_PAD_SIZE) * 4 - 4, j - 8)* subpixel_istride +i] +
    //              17 * img4Y_tmp[IClip(0, (img->height + 2 * IMG_SUBPIXEL_PAD_SIZE) * 4 - 4, j)* subpixel_istride +i] +
    //              57 * img4Y_tmp[IClip(0, (img->height + 2 * IMG_SUBPIXEL_PAD_SIZE) * 4 - 4, j + 4)* subpixel_istride +i] +
    //              2 * img4Y_tmp[IClip(0, (img->height + 2 * IMG_SUBPIXEL_PAD_SIZE) * 4 - 4, j + 12)* subpixel_istride +i] -
    //              4 * img4Y_tmp[IClip(0, (img->height + 2 * IMG_SUBPIXEL_PAD_SIZE) * 4 - 4, j - 4)* subpixel_istride +i] -
    //              9 * img4Y_tmp[IClip(0, (img->height + 2 * IMG_SUBPIXEL_PAD_SIZE) * 4 - 4, j + 8)* subpixel_istride + i] + 2048) / 4096)
    //              );
    //      }
    //  }
    //}
    for ( i = 0; i < ( img->width + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4; i++ )
    {
        for ( j = 0; j < ( img->height + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4; j += 4 )
        {
            // e,i,p; f,j,q; g,k,r
            if ( i % 4 != 0 ) //namely (i % 4 == 1)||(i % 4 == 2)||(i % 4 == 3)
            {
                //e,or f, or g. vertical 1/4 interpolation, filter coefficients: 2, -9, 57, 17, -4, 1
                img4Y_tmp[( j + 1 )* subpixel_istride + i] = IClip( 0, 255,
                        ( int )( ( 2 * img4Y_tmp[( j - 8 )* subpixel_istride + i] +
                                   57 * img4Y_tmp[( j )* subpixel_istride + i] +
                                   17 * img4Y_tmp[( j + 4 )* subpixel_istride + i] +
                                   1 * img4Y_tmp[( j + 12 )* subpixel_istride + i] -
                                   9 * img4Y_tmp[( j - 4 )* subpixel_istride + i] -
                                   4 * img4Y_tmp[( j + 8 )* subpixel_istride + i] + 2048 ) / 4096 )
                                                                  );

                //i,or j, or k. vertical 1/2 interpolation, filter coefficients: 2, -9, 39, 39, -9, 2
                img4Y_tmp[( j + 2 )* subpixel_istride + i] = IClip( 0, 255,
                        ( int )( ( 2 * ( img4Y_tmp[( j - 8 )* subpixel_istride + i] +
                                         img4Y_tmp[( j + 12 )* subpixel_istride + i] ) +
                                   39 * ( img4Y_tmp[( j )* subpixel_istride + i] +
                                          img4Y_tmp[( j + 4 )* subpixel_istride + i] ) -
                                   9 * ( img4Y_tmp[( j - 4 )* subpixel_istride + i] +
                                         img4Y_tmp[( j + 8 )* subpixel_istride + i] ) + 2048 ) / 4096 )
                                                                  );

                //p, or q, or r.vertical 3/4 interpolation, filter coefficients: 1, -4, 17, 57, -9, 2
                img4Y_tmp[( j + 3 )* subpixel_istride + i] = IClip( 0, 255,
                        ( int )( ( 1 * img4Y_tmp[( j - 8 )* subpixel_istride + i] +
                                   17 * img4Y_tmp[( j )* subpixel_istride + i] +
                                   57 * img4Y_tmp[( j + 4 )* subpixel_istride + i] +
                                   2 * img4Y_tmp[( j + 12 )* subpixel_istride + i] -
                                   4 * img4Y_tmp[( j - 4 )* subpixel_istride + i] -
                                   9 * img4Y_tmp[( j + 8 )* subpixel_istride + i] + 2048 ) / 4096 )
                                                                  );
            }
        }
    }

    for ( j = 0; j < ( img->height + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4; j++ )
    {
        for ( i = 0; i < ( img->width + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4; i++ )
        {
            if ( ( ( i % 4 == 0 ) || ( j % 4 == 0 ) ) && ( !( ( i % 4 == 0 ) && ( j % 4 == 0 ) ) ) ) //((i % 4 == 0)||(j % 4 == 0))//round a, b, c; d, h, n
            {
                img4Y_tmp[j* subpixel_istride +i] = IClip( 0, 255, ( int )( img4Y_tmp[j* subpixel_istride +i] + 32 ) / 64 );
            }
        }
    }

    for ( j = 0; j < ( img->height + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4; j++ )
    {
        for ( i = 0; i < ( img->width + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4; i++ )
        {
            PutPel_14( out4Y, j - IMG_SUBPIXEL_PAD_SIZE * 4, i - IMG_SUBPIXEL_PAD_SIZE * 4, ( pel_t )img4Y_tmp[j* subpixel_istride +i] );
        }
    }

    img4Y_tmp = ini_pos;

}

static void UnifiedOneForthPixAdaptive( pel_t * imgY, pel_t * out4Y )
{
    int is;
    int temp;
    int dx, dy;
    int x, y;
    int i, j;// j4;
    int ii, jj;
    int tap, tap_min, tap_max;
    int ipadding_height;
    int ipadding_width;

    int max_pel_value = 255;
    int subpixel_istride, offset_subpixel;
    int *ini_pos;
    int *img4Y_tmp_padding;

    static const int COEF_4tap[3][4] =
    {
        { -6, 56, 15, -1 },
        { -4, 36, 36, -4 },
        { -1, 15, 56, -6 }
    };

    static const int COEF_6tap[3][6] =
    {
        { 2, -9, 57, 17, -4, 1 },
        { 2, -9, 39, 39, -9, 2 },
        { 1, -4, 17, 57, -9, 2 }
    };

    static const int COEF_8tap[3][8] =
    {
        { -1, 4, -10, 57, 19, -7, 3, -1 },
        { -1, 4, -11, 40, 40, -11, 4, -1 },
        { -1, 3, -7, 19, 57, -10, 4, -1 }
    };

    static const int COEF_10tap[3][10] =
    {
        { 1, -2, 4, -10, 57, 19, -7, 3, -1, 0 },
        { 1, -2, 5, -12, 40, 40, -12, 5, -2, 1 },
        { 0, -1, 3, -7, 19, 57, -10, 4, -2, 1 }
    };

    const int *COEF[3];

    if ( img->height >= 1600 ) //for UHD, 2560x1600
    {
        COEF[0] = COEF_4tap[0];
        COEF[1] = COEF_4tap[1];
        COEF[2] = COEF_4tap[2];
        tap = 4;
    }
    else if ( img->height >= 720 ) //for 720p and 1080p
    {
        COEF[0] = COEF_6tap[0];
        COEF[1] = COEF_6tap[1];
        COEF[2] = COEF_6tap[2];
        tap = 6;
    }
    else
    {
        COEF[0] = COEF_10tap[0];
        COEF[1] = COEF_10tap[1];
        COEF[2] = COEF_10tap[2];
        tap = 10;
    }

    tap_min = ( tap >> 1 ) - 1;
    tap_max = ( tap >> 1 ) + 1;

    //    A  a  1  b  B
    //    c  d  e  f
    //    2  h  3  i
    //    j  k  l  m
    //    C           D

    //
#ifdef PADDING_FALG

#else
    ipadding_height = img->height;
    ipadding_width = img->width;
#endif // PADDING_FALG

    subpixel_istride = ( img->width + 2 * ( IMG_SUBPIXEL_PAD_SIZE + IMG_SUBPIXEL_PAD_SIZE_FOR_CLIP ) ) * 4;
    offset_subpixel = subpixel_istride*IMG_SUBPIXEL_PAD_SIZE_FOR_CLIP + IMG_SUBPIXEL_PAD_SIZE_FOR_CLIP;

    // offset the img4Y_tmp
    ini_pos = img4Y_tmp;
    img4Y_tmp = ini_pos + offset_subpixel;

    //full-pixel position: A
    for ( j = -IMG_SUBPIXEL_PAD_SIZE; j < ipadding_height + IMG_SUBPIXEL_PAD_SIZE; j++ )
    {
        for ( i = -IMG_SUBPIXEL_PAD_SIZE; i < ipadding_width + IMG_SUBPIXEL_PAD_SIZE; i++ )
        {
            img4Y_tmp[( ( j + IMG_SUBPIXEL_PAD_SIZE ) * 4 ) * subpixel_istride + ( i + IMG_SUBPIXEL_PAD_SIZE ) * 4] = imgY[j *( img->iStride ) + i];
        }
    }

    //horizontal positions: a,1,b
    for ( j = -IMG_SUBPIXEL_PAD_SIZE; j < ipadding_height + IMG_SUBPIXEL_PAD_SIZE; j++ )
    {
        for ( i = -IMG_SUBPIXEL_PAD_SIZE; i < ipadding_width + IMG_SUBPIXEL_PAD_SIZE; i++ )
        {
            for ( dx = 1; dx < 4; dx++ )
            {
                for ( is = 0, x = -tap_min; x < tap_max; x++ )
                {
                    is += imgY[j*( img->iStride ) + ( i + x )] * COEF[dx - 1][x + tap_min];
                }

                img4Y_tmp[( j + IMG_SUBPIXEL_PAD_SIZE ) * 4 * subpixel_istride + ( i + IMG_SUBPIXEL_PAD_SIZE ) * 4 + dx] = is;
            }
        }
    }

    //vertical positions: c,2,j
    for ( j = -IMG_SUBPIXEL_PAD_SIZE; j < ipadding_height + IMG_SUBPIXEL_PAD_SIZE; j++ )
    {
        for ( i = -IMG_SUBPIXEL_PAD_SIZE; i < ipadding_width + IMG_SUBPIXEL_PAD_SIZE; i++ )
        {
            for ( dy = 1; dy < 4; dy++ )
            {
                for ( is = 0, y = -tap_min; y < tap_max; y++ )
                {
                    is += imgY[( j + y ) *( img->iStride ) + i] * COEF[dy - 1][y + tap_min];
                }

                is = IClip( 0, max_pel_value, ( is + 32 ) / 64 );
                img4Y_tmp[( ( j + IMG_SUBPIXEL_PAD_SIZE ) * 4 + dy )* subpixel_istride + ( i + IMG_SUBPIXEL_PAD_SIZE ) * 4] = is;
            }
        }
    }


    // --------------------- padding ---------------------
    img4Y_tmp_padding = img4Y_tmp - subpixel_istride;
    for ( i = 0; i < IMG_SUBPIXEL_PAD_SIZE_FOR_CLIP; i++ )
    {
        memcpy( img4Y_tmp_padding - i*subpixel_istride,
                img4Y_tmp,
                ( img->width + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4 * sizeof( int ) );
    }

    img4Y_tmp_padding = img4Y_tmp + ( img->height + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4 * subpixel_istride;
    for ( i = 0; i < IMG_SUBPIXEL_PAD_SIZE_FOR_CLIP; i++ )
    {
        memcpy( img4Y_tmp_padding + i*subpixel_istride,
                &img4Y_tmp[( ( img->height + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4 - 4 )*subpixel_istride],
                ( img->width + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4 * sizeof( int ) );
    }
    // --------------------- padding ---------------------


    //vertical positions: d,h,k; e,3,1; f,i,m
    for ( i = 0; i < ( ipadding_width + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4; i += 4 )
    {
        for ( j = 0; j < ( ipadding_height + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4; j += 4 )
        {
            for ( dx = 1; dx < 4; dx++ )
            {
                for ( dy = 1; dy < 4; dy++ )
                {
                    for ( is = 0, y = -tap_min; y < tap_max; y++ )
                    {
                        // the IClip should be deleted
                        //jj = IClip(0, (img->height + 2 * IMG_SUBPIXEL_PAD_SIZE) * 4 - 4, j + 4 * y);
                        jj = ( j + 4 * y );
                        ii = i + dx;

                        is += img4Y_tmp[jj*subpixel_istride + ii] * COEF[dy - 1][y + tap_min];
                    }

                    img4Y_tmp[( j + dy )* subpixel_istride + i + dx] = IClip( 0, max_pel_value, ( is + 2048 ) / 4096 );
                }
            }
        }
    }

    //positions: a,1,b
    for ( j = 0; j < ( ipadding_height + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4; j += 4 )
    {
        for ( i = 0; i < ( ipadding_width + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4; i++ )
        {
            if ( i % 4 != 0 )
            {
                temp = IClip( 0, max_pel_value, ( img4Y_tmp[j* subpixel_istride + i] + 32 ) / 64 );
                img4Y_tmp[j* subpixel_istride + i] = temp;
            }
        }
    }

    for ( j = 0; j < ( ipadding_height + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4; j++ )
    {
        for ( i = 0; i < ( ipadding_width + 2 * IMG_SUBPIXEL_PAD_SIZE ) * 4; i++ )
        {
            PutPel_14( out4Y,
                       j - IMG_SUBPIXEL_PAD_SIZE * 4,
                       i - IMG_SUBPIXEL_PAD_SIZE * 4,
                       ( pel_t )img4Y_tmp[j* subpixel_istride + i] );
        }
    }

    img4Y_tmp = ini_pos;
}

/*
*************************************************************************
* Function:Find SNR for all three components
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

static void find_snr ()
{
    int i, j;
    int diff_y, diff_u, diff_v;
    int impix;
    int uvformat = 4;

    //  Calculate  PSNR for Y, U and V.
    //     Luma.
    impix = img->height * img->width;

    diff_y = 0;
    for ( i = 0; i < img->width; ++i )
    {
        for ( j = 0; j < img->height; ++j )
        {
            diff_y += img->quad[imgY_org[j*( img->width ) + i] - imgY_rec[j*( img->iStride ) + i]];
        }
    }

    //     Chroma.
    diff_u = 0;
    diff_v = 0;

    for ( i = 0; i < img->width_cr; i++ )
    {
        for ( j = 0; j < img->height_cr; j++ )
        {
            diff_u += img->quad[imgU_org[j*( img->width_cr ) + i] - imgU_rec[j*( img->iStrideC ) + i]];
            diff_v += img->quad[imgV_org[j*( img->width_cr ) + i] - imgV_rec[j*( img->iStrideC ) + i]];
        }
    }

    //  Collecting SNR statistics
    if ( diff_y != 0 )
    {
        snr->snr_y = ( float )( 10 * log10( 65025 * ( float )impix / ( float )diff_y ) ); // luma snr for current frame
        snr->snr_u = ( float )( 10 * log10( 65025 * ( float )impix / ( float )( /*4*/uvformat * diff_u ) ) ); // u croma snr for current frame, 1/4 of luma samples,
        snr->snr_v = ( float )( 10 * log10( 65025 * ( float )impix / ( float )( /*4*/uvformat * diff_v ) ) ); // v croma snr for current frame, 1/4 of luma samples,
    }

    if ( img->ip_frm_idx == 0 )
    {
        snr->snr_y1 = ( float )( 10 * log10( 65025 * ( float )impix / ( float )diff_y ) ); // keep luma snr for first frame
        snr->snr_u1 = ( float )( 10 * log10( 65025 * ( float )impix / ( float )( /*4*/uvformat * diff_u ) ) ); // keep croma u snr for first frame
        snr->snr_v1 = ( float )( 10 * log10( 65025 * ( float )impix / ( float )( /*4*/uvformat * diff_v ) ) ); // keep croma v snr for first frame
        snr->snr_ya = snr->snr_y1;
        snr->snr_ua = snr->snr_u1;
        snr->snr_va = snr->snr_v1;
    }
    // B pictures
    else
    {
        snr->snr_ya = ( float )( snr->snr_ya * ( img->ip_frm_idx + Bframe_ctr ) + snr->snr_y ) / ( img->ip_frm_idx + Bframe_ctr + 1 ); // average snr lume for all frames inc. first
        snr->snr_ua = ( float )( snr->snr_ua * ( img->ip_frm_idx + Bframe_ctr ) + snr->snr_u ) / ( img->ip_frm_idx + Bframe_ctr + 1 ); // average snr u croma for all frames inc. first
        snr->snr_va = ( float )( snr->snr_va * ( img->ip_frm_idx + Bframe_ctr ) + snr->snr_v ) / ( img->ip_frm_idx + Bframe_ctr + 1 ); // average snr v croma for all frames inc. first
    }

}

/*
*************************************************************************
* Function:Find distortion for all three components
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void find_distortion ()
{
    int i, j;
    int diff_y, diff_u, diff_v;
    int impix;

    //  Calculate  PSNR for Y, U and V.
    //     Luma.
    impix = img->height * img->width;

    diff_y = 0;
    for ( i = 0; i < img->width; ++i )
    {
        for ( j = 0; j < img->height; ++j )
        {
            diff_y += img->quad[abs( imgY_org[j*( img->width ) + i] - imgY_rec[j*( img->iStride ) + i] )];
        }
    }

    //     Chroma.
    diff_u = 0;
    diff_v = 0;

    for ( i = 0; i < img->width_cr; i++ )
    {
        for ( j = 0; j < img->height_cr; j++ )
        {
            diff_u += img->quad[abs( imgU_org[j*( img->width_cr ) + i] - imgU_rec[j*( img->iStrideC ) + i] )];
            diff_v += img->quad[abs( imgV_org[j*( img->width_cr ) + i] - imgV_rec[j*( img->iStrideC ) + i] )];
        }
    }

    // Calculate real PSNR at find_snr_avg()
    snr->snr_y = ( float ) diff_y;
    snr->snr_u = ( float ) diff_u;
    snr->snr_v = ( float ) diff_v;

}

/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

static void ReportFirstframe( int tmp_time )
{


    FILE *file = fopen( "stat.dat","at" );

    fprintf( file,"\n -------------------- DEBUG_INFO_START -------------------- \n" );

    fprintf ( file,"%3d(I)  %8d %4d %7.4f %7.4f %7.4f  %5d          %3d      %3s\n",
              img->poc, stat->bit_ctr - stat->bit_ctr_n,
              img->frame_qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time,
              intras, "FRM" );

    fclose( file );

    printf ( "%3d(I)  %8d %4d %7.4f %7.4f %7.4f  %5d       %s \n",
             img->poc, stat->bit_ctr - stat->bit_ctr_n,
             img->frame_qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time, "FRM");

    stat->bitr0 = stat->bitr;
    stat->bit_ctr_0 = stat->bit_ctr;

}

/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

static void ReportIntra( int tmp_time )
{
    FILE *file = fopen( "stat.dat","at" );

    fprintf ( file,"%3d(I)  %8d %4d %7.4f %7.4f %7.4f     %5d             \n",
              img->poc, stat->bit_ctr - stat->bit_ctr_n,
              img->frame_qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time);

    fclose( file );

    printf ( "%3d(I)  %8d %4d %7.4f %7.4f %7.4f  %5d %3s\n",
             img->poc, stat->bit_ctr - stat->bit_ctr_n,
             img->frame_qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time, "FRM");

}

/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

static void ReportB( int tmp_time )
{
    FILE *file = fopen( "stat.dat","at" );

    fprintf ( file,"%3d(B)  %8d %4d %7.4f %7.4f %7.4f  %5d        \n",
              img->poc, stat->bit_ctr - stat->bit_ctr_n, img->frame_qp,
              snr->snr_y, snr->snr_u, snr->snr_v, tmp_time );

    fclose( file );

    printf ( "%3d(B)  %8d %4d %7.4f %7.4f %7.4f  %5d       %3s\n",
             img->poc, stat->bit_ctr - stat->bit_ctr_n, img->frame_qp,
             snr->snr_y, snr->snr_u, snr->snr_v, tmp_time, "FRM" );

}

/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

static void ReportP( int tmp_time )
{
    FILE *file = fopen( "stat.dat","at" );
    char typ = ( img->type==I_IMG )?'I':'P';

    fprintf ( file,"%3d(%c)  %8d %4d %7.4f %7.4f %7.4f  %5d        %3d\n",
              img->poc, typ, stat->bit_ctr - stat->bit_ctr_n, img->frame_qp, snr->snr_y,
              snr->snr_u, snr->snr_v, tmp_time,
              intras );

    fclose( file );

    printf ( "%3d(%c)  %8d %4d %7.4f %7.4f %7.4f  %5d       %3s     %3d    \n",
             img->poc, typ, stat->bit_ctr - stat->bit_ctr_n, img->frame_qp, snr->snr_y,
             snr->snr_u, snr->snr_v, tmp_time,  "FRM",intras );

}

/*
*************************************************************************
* Function:Copies contents of a Sourceframe structure into the separate YUV buffers represented by
*    variables imgY_org_frm and imgUV_org_frm.  No other side effects
* Input:  sf the source frame the frame is to be taken from
* Output:
* Return:
* Attention:
*************************************************************************
*/
// image padding if necessary
static void CopyFrameToOrigYUVBuffers()
{
    int x, y;
    pel_t *u_buffer, *v_buffer;
    int input_height_cr = input->height_org / 2;
    int img_height_cr = img->height / 2;

    u_buffer = imgY_org_buffer + input->width_org*input->height_org;
    v_buffer = imgY_org_buffer + input->width_org*input->height_org * 5 / 4;

    // Y
    for ( y = 0; y < input->height_org; y++ )
    {
        for ( x = 0; x < input->width_org; x++ )
        {
            imgY_org[y * ( img->width ) + x] = imgY_org_buffer[y*input->width_org + x];
        }
    }

    // UV
    for ( y = 0; y < input_height_cr; y++ )
    {
        for ( x = 0; x < input->width_org / 2; x++ )
        {
            imgU_org[y * ( img->width_cr ) + x] = u_buffer[y*input->width_org / 2 + x];
            imgV_org[y * ( img->width_cr ) + x] = v_buffer[y*input->width_org / 2 + x];
        }
    }

    // Y's right padding if necessary
    for ( y = 0; y < input->height_org; y++ )
    {
        for ( x = input->width_org; x < img->width; x++ )
        {
            imgY_org[y * ( img->width ) + x] = imgY_org[y * ( img->width ) + x - 1];
        }
    }

    //Y's padding bottom border if necessary
    for ( y = input->height_org; y < img->height; y++ )
    {
        for ( x = 0; x < img->width; x++ )
        {
            imgY_org[y * ( img->width ) + x] = imgY_org[( y - 1 ) * ( img->width ) + x];
        }
    }

    // UV's right padding if necessary
    for ( y = 0; y < input_height_cr; y++ )
    {
        for ( x = input->width_org / 2; x < img->width / 2; x++ )
        {
            imgU_org[y * ( img->width_cr ) + x] = imgU_org[y * ( img->width_cr ) + x - 1];
            imgV_org[y * ( img->width_cr ) + x] = imgV_org[y * ( img->width_cr ) + x - 1];
        }
    }

    // UV's bottom padding bottom if necessary
    for ( y = input_height_cr; y < img_height_cr; y++ )
    {
        for ( x = 0; x < img->width / 2; x++ )
        {
            imgU_org[y * ( img->width_cr ) + x] = imgU_org[( y - 1 ) * ( img->width_cr ) + x];
            imgV_org[y * ( img->width_cr ) + x] = imgV_org[( y - 1 ) * ( img->width_cr ) + x];
        }
    }

}

/*
*************************************************************************
* Function:Reads one new frame from file
* Input: FrameNoInFile: Frame number in the source file
HeaderSize: Number of bytes in the source file to be skipped
xs: horizontal size of frame in pixels, must be divisible by 16
ys: vertical size of frame in pixels, must be divisible by 16
sf: Sourceframe structure to which the frame is written
* Output:
* Return:
* Attention:
*************************************************************************
*/
static void ReadOneFrame ( int FrameNoInFile, int HeaderSize )
{
    int input_width_cr = input->width_org / 2;
    int input_height_cr = input->height_org / 2;
    i32u_t  bytes_y = input->width_org * input->height_org;
    const i32u_t  bytes_uv = input_width_cr * input_height_cr;

#ifdef WIN32
    const __int64 framesize_in_bytes = bytes_y + 2 * bytes_uv;
#else
    const int framesize_in_bytes = bytes_y + 2*bytes_uv;
#endif

    //int stuff_height_cr = (input->img_height-input->stuff_height)/2;
    int off_y = input->width_org*input->height_org;
    int off_uv = input_width_cr * input_height_cr;

    assert( FrameNumberInFile == FrameNoInFile );

    if ( _lseeki64( p_in, framesize_in_bytes * FrameNoInFile + HeaderSize, SEEK_SET ) == -1 )
    {
        error( "ReadOneFrame: cannot fseek to (Header size) in p_in", -1 );
    }

    if ( _read( p_in, imgY_org_buffer, bytes_y ) != ( int )bytes_y )
    {
        printf( "ReadOneFrame: cannot read %d bytes from input file, unexpected EOF?, exiting", bytes_y );
        exit( -1 );
    }

    if ( _read( p_in, imgY_org_buffer + off_y, bytes_uv ) != ( int )bytes_uv )
    {
        printf( "ReadOneFrame: cannot read %d bytes from input file, unexpected EOF?, exiting", bytes_uv );
        exit( -1 );
    }

    if ( _read( p_in, imgY_org_buffer + off_y + off_uv, bytes_uv ) != ( int )bytes_uv )
    {
        printf( "ReadOneFrame: cannot read %d bytes from input file, unexpected EOF?, exiting", bytes_uv );
        exit( -1 );
    }
}

/*
*************************************************************************
* Function:point to frame coding variables
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void put_buffer_frame()
{
    int i, j;

    //initialize ref index 1/4 pixel
    for ( i = 0; i < img->real_ref_num; i++ )
    {
        mref[i] = mref_frm[i];
    }

    //integer pixel for chroma
    for ( i = 0; i < img->real_ref_num; i++ )
    {
        for ( j = 0; j < 2; j++ )
        {
            mcef[i][j] = ref_frm[i][j + 1];
        }
    }

    //integer pixel for luma
    for ( i = 0; i < img->real_ref_num; i++ )
    {
        Refbuf11[i] = &ref_frm[i][0][0];
    }

    // test
    imgY_rec = current_frame[0];
    imgU_rec = current_frame[1];
    imgV_rec = current_frame[2];
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

    uchar_t *tmp_yuv[3];
    uchar_t *tmp_y;

    if ( img->typeb == L2_IMG )
    {
        return;
    }
    if ( img->typeb == L3_IMG )
    {
        tmp_yuv[0] = ref_frm[1][0];
        tmp_yuv[1] = ref_frm[1][1];
        tmp_yuv[2] = ref_frm[1][2];
        ref_frm[1][0] = ref_frm[0][0];
        ref_frm[1][1] = ref_frm[0][1];
        ref_frm[1][2] = ref_frm[0][2];
        ref_frm[0][0] = tmp_yuv[0];
        ref_frm[0][1] = tmp_yuv[1];
        ref_frm[0][2] = tmp_yuv[2];

        tmp_y = mref_frm[1];
        mref_frm[1] = mref_frm[0];
        mref_frm[0] = tmp_y;

        //initial reference index, and for coming interpolation in mref[0]
        for ( i = 0; i < 3; i++ )
        {
            mref[i] = mref_frm[i];
        }
        return;
    }

    tmp_yuv[0] = ref_frm[img->real_ref_num - 1][0];
    tmp_yuv[1] = ref_frm[img->real_ref_num - 1][1];
    tmp_yuv[2] = ref_frm[img->real_ref_num - 1][2];

    for ( i = img->real_ref_num - 1; i > 0; i-- )
    {
        ref_frm[i][0] = ref_frm[i - 1][0];
        ref_frm[i][1] = ref_frm[i - 1][1];
        ref_frm[i][2] = ref_frm[i - 1][2];
    }

    // test
    ref_frm[0][0] = current_frame[0];
    ref_frm[0][1] = current_frame[1];
    ref_frm[0][2] = current_frame[2];
    current_frame[0] = tmp_yuv[0];
    current_frame[1] = tmp_yuv[1];
    current_frame[2] = tmp_yuv[2];
    // test


    tmp_y = mref_frm[img->real_ref_num - 1];

    for ( i = img->real_ref_num - 1; i > 0; i-- )
    {

        mref_frm[i] = mref_frm[i-1];
    }

    mref_frm[0] = tmp_y;

    for ( i = 0; i < img->real_ref_num; i++ )
    {
        mref[i] = mref_frm[i];
    }

    if ( img->type == I_IMG )
    {
        af_intra_cnt = 1;
    }
    else
    {
        af_intra_cnt++;
    }
}

void write_terminating_bit ( CSobj *cs_aec, uchar_t bit )
{
    Env_AEC*  eep_dp;

    //--- write non-slice termination symbol if the macroblock is not the first one in its slice ---
    eep_dp = &( cs_aec->ee_AEC );

    biari_encode_symbol_final( eep_dp, bit );
}