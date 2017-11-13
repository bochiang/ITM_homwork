#include "contributors.h"
#include "defines.h"
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include <assert.h>
#include <string.h>

#include "global.h"
#include "header.h"
#include "bitstream.h"
#include "vlc.h"
#include "image.h"
#include "AEC.h"
#include "loopfilter.h"
#include "macroblock.h"
#include "rdopt.h"
#include "../../common/pixel.h"
#ifdef TDRDO
#include "tdrdo.h"
#endif

void init_slice ( int start_mb_addr,Picture *currPic );

/*
*************************************************************************
* Function:This function terminates a slice
* Input:
* Output:
* Return: 0 if OK,                                                         \n
1 in case of error
* Attention:
*************************************************************************
*/
int start_slice( Bitstream *currStream, int mb_nr )
{
    if( mb_nr != 0 )
    {
        Demulate( currStream,current_slice_bytepos ); // Should demulation precede data stuffing ?
    }
#if M38817_DATA_STUFFING
    currStream->byte_buf <<= currStream->bits_to_go;
    if( currStream->bits_to_go == 1 )
    {
        currStream->byte_buf |= ( 0x3 >> 1 );
        currStream->streamBuffer[currStream->byte_pos++] = currStream->byte_buf;
        currStream->streamBuffer[currStream->byte_pos++] = 0x80;
    }
    else
    {
        currStream->byte_buf |= ( 0x3 << ( currStream->bits_to_go - 2 ) );
        currStream->streamBuffer[currStream->byte_pos++] = currStream->byte_buf;
    }
    currStream->bits_to_go = 8;
    currStream->byte_buf = 0;
#else
    if ( currStream->bits_to_go!=8 )
    {

        currStream->byte_buf <<= currStream->bits_to_go;
        currStream->byte_buf |= ( 1 << ( currStream->bits_to_go - 1 ) );

        currStream->streamBuffer[currStream->byte_pos++] = currStream->byte_buf;
        currStream->bits_to_go = 8;
        currStream->byte_buf = 0;
    }
    else
    {
        currStream->streamBuffer[currStream->byte_pos++] = 0x80;
        currStream->bits_to_go = 8;
        currStream->byte_buf = 0;
    }
#endif
    current_slice_bytepos=currStream->byte_pos;

    return 0;
}

/*
*************************************************************************
* Function:This function terminates a picture
* Input:
* Output:
* Return: 0 if OK,                                                         \n
1 in case of error
* Attention:
*************************************************************************
*/
int terminate_picture( Bitstream *currStream )
{
    Demulate( currStream,current_slice_bytepos );

    currStream->byte_buf <<= currStream->bits_to_go;
    currStream->byte_buf |= ( 1 << ( currStream->bits_to_go - 1 ) );

    currStream->streamBuffer[currStream->byte_pos++] = currStream->byte_buf;
    currStream->bits_to_go = 8;
    currStream->byte_buf = 0;

    return 0;
}

void GetLambda(ImgParams *img, int mb_nr)
{
    int isIntraFrm = (img->type == I_IMG);
    int isBFrm = (img->type == B_IMG);
    double qp = (double)img->frame_qp - SHIFT_QP;

    if (input->successive_Bframe > 0)
    {
        img->lambda = 0.68 * pow(2, qp / 4.0) * (isBFrm ? MAX(2.00, MIN(4.00, (qp / 8.0))) : 1.0);
    }
    else
    {
        if (img->typeb == 2)
        {
            img->lambda = 0.85 * pow(2, qp / 4.0) * MAX(1.4, MIN(2, (1 + (20.0 / (qp))))) * (isBFrm ? 4.0 : 1.0);
        }
        else if ((img->ip_frm_idx - img->i_frm_idx) % HPGOPSIZE == HPGOPSIZE / 2)
        {
            img->lambda = 0.85 * pow(2, qp / 4.0) * 2 * (isBFrm ? 4.0 : 1.0) * 0.8;
        }
        else
        {
            img->lambda = 0.85 * pow(2, qp / 4.0) * (isBFrm ? 4.0 : 1.0);
        }
    }

#ifdef TDRDO
    CurMBQP = img->frame_qp;
    if (input->TRDOLength != 0)
    {
        if (GlobeFrameNumber < input->FrmsToBeEncoded && !isIntraFrm)
        {
            if (input->FrmsToBeEncoded > StepLength&&GlobeFrameNumber%StepLength == 0)
            {
                pOMCPFD = &OMCPDList->FrameDistortionArray[(GlobeFrameNumber - 1) / StepLength];
            }
            else
            {
                pOMCPFD = NULL;
            }
        }
        // Just for LDP
        if (!isIntraFrm)
        {
            if ((input->successive_Bframe && GlobeFrameNumber > StepLength) || (!input->successive_Bframe && GlobeFrameNumber > 0))
            {
                AdjustLcuQPLambdaLDP(pOMCPFD, mb_nr, input->width_org / WORKBLOCKSIZE, &(img->lambda));
                CurMBQP = MAX(MIN_QP, MIN(CurMBQP, MAX_QP));
            }
            if (GlobeFrameNumber%StepLength == 0)
            {
                StoreLCUInf(pRealFD, mb_nr, input->width_org / WORKBLOCKSIZE, CurMBQP, img->lambda, img->type); // stores for key frame
            }
        }
    }
#endif
}

void xGetLambda(ImgParams *img, int mb_nr)
{
    int isIntraFrm = (img->type == I_IMG);
    int isBFrm = (img->type == B_IMG);
    double qp = (double)img->frame_qp - SHIFT_QP;

    if (input->successive_Bframe > 0)
    {
        img->lambda = 0.68 * pow(2, qp / 4.0) * (isBFrm ? MAX(2.00, MIN(4.00, (qp / 8.0))) : 1.0);
    }
    else
    {
        if (img->typeb == 2)
        {
            img->lambda = 0.85 * pow(2, qp / 4.0) * MAX(1.4, MIN(2, (1 + (20.0 / (qp))))) * (isBFrm ? 4.0 : 1.0);
        }
        else if ((img->ip_frm_idx - img->i_frm_idx) % HPGOPSIZE == HPGOPSIZE / 2)
        {
            img->lambda = 0.85 * pow(2, qp / 4.0) * 2 * (isBFrm ? 4.0 : 1.0) * 0.8;
        }
        else
        {
            img->lambda = 0.85 * pow(2, qp / 4.0) * (isBFrm ? 4.0 : 1.0);
        }
    }
}

/*
*************************************************************************
* Function:Encodes one slice
* Input:
* Output:
* Return: the number of coded MBs in the SLice
* Attention:
*************************************************************************
*/
void picture_data( Picture *pic, CSobj *cs_aec )
{
    int mb_pos = 0;
    int slice_nr = 0;
    int slice_qp = img->frame_qp;
    int k;
    int len;
    int old_bit;
    int uhBitSize = 4;
    best_cu_info_t best_info;
    Macroblock  *currMB;
    Boolean end_of_picture = FALSE;

    Env_AEC* eep;
    Bitstream *currStream = cs_aec->bitstream;

    int mb_width = img->PicWidthInMbs;
    int slice_mb = input->slice_row_nr * mb_width;

    while ( end_of_picture == FALSE ) // loop over macro-blocks
    {
        img->writeBSflag = 0;
        img->current_mb_nr = mb_pos;
        currMB = &img->mb_data[img->current_mb_nr];
        init_img_pos( img, mb_pos );
        init_coding_state( img, cs_aec, mb_pos );
        init_mb_params( currMB, uhBitSize, mb_pos );

        //write slice header and do some initializations at the start of one slice
        if ( input->slice_row_nr && ( img->current_mb_nr == 0 || ( img->current_mb_nr > 0 && img->mb_data[img->current_mb_nr].slice_nr != img->mb_data[img->current_mb_nr - 1].slice_nr ) ) )
        {
            old_bit = currStream->byte_pos * 8 + 8 - currStream->bits_to_go;
            start_slice( currStream, mb_pos );
            img->current_slice_start_mb = img->current_mb_nr;
            len = SliceHeader( currStream, slice_qp );

            init_slice( img->current_mb_nr, pic );
            eep = &( cs_aec->ee_AEC );

            if ( currStream->bits_to_go < 8 ) // trailing bits to process
            {
                currStream->byte_buf = ( currStream->byte_buf << currStream->bits_to_go ) | ( 0xff >> ( 8 - currStream->bits_to_go ) );
                stat->bit_use_stuffingBits[img->type] += currStream->bits_to_go;
                currStream->streamBuffer[currStream->byte_pos++] = currStream->byte_buf;
                currStream->bits_to_go = 8;
            }

            arienco_start_encoding( eep, currStream->streamBuffer, &( currStream->byte_pos ) );
            init_contexts( cs_aec );

            stat->bit_slice += len;
            slice_nr++;
            slice_header[img->type] += ( currStream->byte_pos * 8 + 8 - currStream->bits_to_go - old_bit );
        }// slice header end

#if TRACE
        //fprintf(p_trace, "\n*********** POC: %i MB: %i Slice Idx: %i Type %d **********\n", img->tr, img->current_mb_nr, img->current_slice_nr, img->type);
        fprintf(p_trace, "\n*********** MB: %i Slice Idx: %i Type %d **********\n", img->current_mb_nr, img->current_slice_nr, img->type);
#endif
        //set_cu_parameters(img, currMB, mb_pos, uhBitSize - 1);
        // SET LAGRANGE PARAMETERS
        GetLambda(img, img->current_mb_nr);

        EncodeOneCU( img, currMB, cs_aec, &best_info, uhBitSize ); /* uhBitSize == 4 ( macroblock : 16x16 ) */
        revert_large_cu_param( img, mb_pos, &best_info, uhBitSize );

        if ( input->use_p_sub_type && img->type == P_IMG )
        {
            int mb_b4_x = img->mb_b4_x;
            int mb_b4_y = img->mb_b4_y;
            int pu_b4_x, pu_b4_y;
            int mv[2];
            int ref;
            for( k = 0; k < 4; k++ )
            {
                pu_b4_y = mb_b4_y + k / 2;
                pu_b4_x = mb_b4_x + k % 2;
                ref = ( currMB->b8pdir[k] == -1 ? 0 : currMB->b8pdir[k] );
                if ( ( img->type == P_IMG ) && ( ref == 3 ) )
                {
                    ref = 0;
                }
                mv[0] = all_mincost[pu_b4_x][pu_b4_y][ref][4][1];
                mv[1] = all_mincost[pu_b4_x][pu_b4_y][ref][4][2];
                img->p_sub_type_start += ( abs( mv[0] ) + abs( mv[1] ) < 12 );
            }
        }
        img->writeBSflag = 1;
        //write_one_cu(img, currMB, cs_aec, uhBitSize, 1);
        write_cu_tree( img, cs_aec, uhBitSize, mb_pos );

        //the last mb in current slice
        if ( ( ( img->coded_mb_nr + 1 ) % ( slice_mb ) == 0 ) || ( img->coded_mb_nr + 1 == img->total_number_mb ) )
        {
            terminate_slice( cs_aec );
        }

        terminate_pic( img, cs_aec, &end_of_picture );
        update_statistics( currMB, cs_aec );
        mb_pos++;
    }

    old_bit = currStream->byte_pos * 8 + 8 - currStream->bits_to_go;
    terminate_picture( currStream );
    stat->bit_use_stuffingBits[img->type] += ( currStream->byte_pos * 8 + 8 - currStream->bits_to_go - old_bit );

    if( !input->deblk_disable )
    {
        DeblockFrame( img, imgY_rec, imgU_rec, imgV_rec );
    }

    // padding
    image_padding( img, imgY_rec, imgU_rec, imgV_rec, IMG_PAD_SIZE );
}


void Demulate( Bitstream *currStream, int current_slice_bytepos )
{
    int i, j;
    int rawbitsequence=-1;
    int bitvalue, nonzero, bitnum;
    Bitstream *tmpStream = AllocateBitstream();
    if( !( currStream->streamBuffer[current_slice_bytepos]==0 && currStream->streamBuffer[current_slice_bytepos+1]==0 && currStream->streamBuffer[current_slice_bytepos+2]==1 ) )
    {
        printf ( "Fatal bitstream error" );
    }

    bitnum=8;
    tmpStream->bits_to_go=0;
    currStream->streamBuffer[currStream->byte_pos]=currStream->byte_buf<<currStream->bits_to_go;

    for( tmpStream->byte_pos=i=current_slice_bytepos+3 ; i<=currStream->byte_pos; i++ )
    {
        for( j=8; j>( i==currStream->byte_pos? currStream->bits_to_go:0 ); j-- )
        {
            bitvalue=currStream->streamBuffer[i]&( 0x01<<( j-1 ) );
            if( bitnum==2 )
            {
                nonzero = rawbitsequence & 0x003fffff; // check successive 22 zeros.
                if( !nonzero )
                {
                    tmpStream->streamBuffer[tmpStream->byte_pos] = 0x02; // insert '10' after 22 zeros
                    tmpStream->byte_pos++;
                    tmpStream->bits_to_go += 2;
                    rawbitsequence = 0x00000002;
                    bitnum = 8;
                }
            }

            rawbitsequence <<= 1;
            if( bitvalue )
            {
                tmpStream->streamBuffer[tmpStream->byte_pos] |= 1<<( bitnum-1 );
                rawbitsequence |= 1;
            }
            else
            {
                tmpStream->streamBuffer[tmpStream->byte_pos] &= ( ~( 1<<( bitnum-1 ) ) );
            }
            bitnum--;
            tmpStream->bits_to_go++;
            if( bitnum==0 )
            {
                bitnum = 8;
                tmpStream->byte_pos++;
            }
        }

    }
    for( i=current_slice_bytepos+3; i<=tmpStream->byte_pos; i++ )
    {
        currStream->streamBuffer[i]=tmpStream->streamBuffer[i];
    }

    currStream->byte_pos=tmpStream->byte_pos;
    currStream->bits_to_go=8-tmpStream->bits_to_go%8;
    currStream->byte_buf=tmpStream->streamBuffer[tmpStream->byte_pos]>>currStream->bits_to_go;
    FreeBitstream( tmpStream );
}

/*!
************************************************************************
* \brief
*    Initializes the parameters for a new slice and
*     allocates the memory for the coded slice in the Picture structure
*  \par Side effects:
*      Adds slice/partition header symbols to the symbol buffer
*      increments Picture->no_slices, allocates memory for the
*      slice, sets img->currSlice
*  \author
*
************************************************************************
*/
void init_slice ( int start_mb_addr,Picture *currPic )
{
    img->current_mb_nr = start_mb_addr;

    // Allocate new Slice in the current Picture, and set img->currentSlice
    assert ( currPic != NULL );
    currPic->no_slices=1;
    if ( currPic->no_slices >= MAX_SLICE_NUM_IN_PIC )
    {
        error ( "Too many slices per picture, increase MAXLSICESPERPICTURE in global.h.", -1 );
    }
    img->currentSlice = &currPic->slices[currPic->no_slices-1];
    img->currentSlice->start_mb_nr = start_mb_addr;
}

/*!
************************************************************************
* \brief
*    This function terminates a slice (but doesn't write it out),
*    the old terminate_slice (0)
* \return
*    0 if OK,                                                         \n
*    1 in case of error
*  \author
*
*
************************************************************************
*/
int terminate_slice( CSobj *cs_aec )
{
    Bitstream *currStream;
    Env_AEC* eep;

    write_terminating_bit ( cs_aec, 1 );

    currStream = cs_aec->bitstream;
    eep = &( cs_aec->ee_AEC );
    arienco_done_encoding( eep );
    currStream->bits_to_go = eep->Ebits_to_go;
    currStream->byte_buf   = eep->Ebuffer;

    return 0;
}