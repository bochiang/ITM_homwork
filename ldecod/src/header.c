#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "memalloc.h"
#include "global.h"
#include "elements.h"
#include "defines.h"
#include "vlc.h"
#include "header.h"
#include "AEC.h"
#include "math.h"

extern StatBits *StatBitsPtr;

/*
*************************************************************************
* Function:sequence header
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void SequenceHeader ( uchar_t *buf,int startcodepos, int length )
{
    memcpy ( currStream->streamBuffer, buf, length );
    currStream->bs_length = length;
    currStream->read_len = currStream->byte_offset = ( startcodepos+1 )*8;

    profile_id                  = u_v  ( 8, "profile_id"  );
    level_id                    = u_v  ( 8, "level_id"    );
    horizontal_size             = u_v  ( 14, "horizontal_size"  );
    vertical_size               = u_v  ( 14, "vertical_size"  );
    chroma_format               = u_v  ( 2, "chroma_format"  );
    sample_precision            = u_v  ( 3, "sample_precision"  );
    aspect_ratio                = u_v  ( 4, "aspect_ratio"  );
    frame_rate_code             = u_v  ( 4, "frame_rate_code"  );
    bit_rate_lower              = u_v  ( 18, "bit_rate_lower"  );
    marker_bit                  = u_v  ( 1,  "marker bit"  );
    bit_rate_upper              = u_v  ( 12, "bit_rate_upper"  );
    low_delay                   = u_v  ( 1, "low_delay"  );
    marker_bit                  = u_v  ( 1, "marker bit"  );
    bbv_buffer_size             = u_v  ( 18,"vbv buffer size" );
    input->abt_enable           = u_v  ( 1, "abt_enable");
    input->if_type              = u_v  ( 1, "if type" );
    u_v( 4,"reseved bits"  );

#if M38817_DATA_STUFFING
    IsIVCStuffingPattern( 0, currStream->bs_length*8 - currStream->byte_offset );
#else
    IsStuffingPattern( 0, currStream->bs_length*8 - currStream->byte_offset ); //ITM
#endif
    CheckHighLevelSyntax( 0 );
}


void video_edit_code_data( char *buf,int startcodepos, int length )
{
    currStream->byte_offset = currStream->read_len = ( startcodepos+1 )*8;
    currStream->bs_length = length;
    memcpy ( currStream->streamBuffer, buf, length );
}
/*
*************************************************************************
* Function:I picture header  //sw
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void I_Picture_Header(char *buf, int startcodepos, int length)
{
    currStream->byte_offset = currStream->read_len = (startcodepos + 1) * 8;
    currStream->bs_length = length;
    memcpy(currStream->streamBuffer, buf, length);

    bbv_delay = u_v(16, "bbv_delay");
    time_flag = u_v(1, "time_flag");
    if (time_flag)
    {
        time_code = u_v(24, "time_code");
    }

    marker_bit = u_v(1, "marker_bit");
    img->pic_distance = u_v(8, "picture_distance");

    if (low_delay)
    {
        ue_v("bbv check times");
    }

    img->fixed_frame_level_qp = u_v(1, "fixed_frame_level_qp");
    img->frame_qp = u_v(6, "frame_qp");

    u_v(4, "reserved bits");

    img->deblk_disable = u_v(1, "loop_filter_disable");
    if (!img->deblk_disable)
    {
        img->Alpha = u_v(8, "alpha");
        img->Beta = u_v(6, "beta");
    }

    img->qp = img->frame_qp;
    img->type = I_IMG;

#if M38817_DATA_STUFFING
    IsIVCStuffingPattern(0, currStream->bs_length * 8 - currStream->byte_offset);
#else
    IsStuffingPattern( 0, currStream->bs_length*8 - currStream->byte_offset );
#endif
    CheckHighLevelSyntax(1);
}

/*
*************************************************************************
* Function:pb picture header
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void PB_Picture_Header(char *buf, int startcodepos, int length)
{
    currStream->byte_offset = currStream->read_len = (startcodepos + 1) * 8;
    currStream->bs_length = length;
    memcpy(currStream->streamBuffer, buf, length);

    bbv_delay = u_v(16, "bbv delay");
    picture_coding_type = u_v(2, "picture_coding_type");
    img->p_subtype = u_v(2, "P picture subtype");
    img->pic_distance = u_v(8, "picture_distance");

    if (low_delay)
    {
        ue_v("bbv check times");
    }

    img->fixed_frame_level_qp = u_v(1, "fixed_frame_level_qp");
    img->frame_qp = u_v(6, "frame_qp");

    img->no_forward_reference = u_v(1, "no_forward_reference_flag");
    u_v(3, "reserved bits");

    img->deblk_disable = u_v(1, "loop_filter_disable");
    if (!img->deblk_disable)
    {
        img->Alpha = u_v(8, "alpha");
        img->Beta = u_v(6, "beta");
    }
    img->qp = img->frame_qp;

    if (picture_coding_type == 1)
    {
        img->type = P_IMG;
    }
    else
    {
        img->type = B_IMG;
    }

#if M38817_DATA_STUFFING
    IsIVCStuffingPattern(0, currStream->bs_length * 8 - currStream->byte_offset);
#else
    IsStuffingPattern( 0, currStream->bs_length*8 - currStream->byte_offset );
#endif
    CheckHighLevelSyntax(2);
    if (img->no_forward_reference)
    {
        bValidSyntax = 0;
        fprintf(stdout, "no_forward_reference_flag should be set to 0 in P frame!\n");
    }
}

/*
*************************************************************************
* Function:user data
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void user_data( char *buf,int startcodepos, int length )
{
    int user_data;
    int i;

    memcpy ( currStream->streamBuffer, buf, length );

    currStream->bs_length = length;
    currStream->read_len = currStream->byte_offset = ( startcodepos+1 )*8;

    for( i=0; i<length-4; i++ )
    {
        user_data = u_v ( 8, "user data" );
    }
}

/*
*************************************************************************
* Function:To calculate the poc values
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void calc_picture_distance( ImgParams *img )
{
    i32u_t MaxPicDistanceLsb = 256;

    if (img->type != B_IMG)
    {
        img->imgtr_prev_P = img->imgtr_last_P;
        img->imgtr_last_P = img->imgtr_next_P;
        img->imgtr_next_P = img->pic_distance;
        img->Bframe_number = 0;
    }

    if (img->type == B_IMG)
    {
        img->Bframe_number++;
    }
}

/*
*************************************************************************
* Function:slice header
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void SliceHeader(uchar_t *buf, int startcodepos, int length)
{
    int mb_row;

    memcpy(currStream->streamBuffer, buf, length);
    currStream->bs_length = length;

    // Actually, this syntax element make no sense, because one slice will terminate according to start coder instead of vertical MB rows.
    currStream->read_len = currStream->byte_offset = (startcodepos)* 8;
    slice_vertical_position = u_v(8, "slice vertical position"); // this is also the start code of the current slice

    if (vertical_size > 2800)
    {
        slice_vertical_position_extension = u_v(3, "slice vertical position extension");
    }

    if (vertical_size > 2800)
    {
        mb_row = (slice_vertical_position_extension << 7) + slice_vertical_position;
    }
    else
    {
        mb_row = slice_vertical_position;
    }

    if (!img->fixed_frame_level_qp)
    {
        img->fixed_slice_qp = u_v(1, "fixed_slice_qp");
        img->slice_qp = u_v(6, "slice_qp");

        img->qp = img->slice_qp;
    }

    CheckHighLevelSyntax(7);
}

/*
*************************************************************************
* Function:Error handling procedure. Print error message to stderr and exit
with supplied code.
* Input:text
Error message
* Output:
* Return:
* Attention:
*************************************************************************
*/

void error( char *text, int code )
{
    fprintf( stderr, "%s\n", text );
    exit( code );
}

int sign( int a , int b )
{
    int x;

    x=abs( a );

    if ( b>0 )
    {
        return( x );
    }
    else
    {
        return( -x );
    }
}

void CheckHighLevelSyntax(int idx)
{
    //0: sequence_header
    //1: i_picture_header
    //2: pb_picture_header
    //7: slice
    int   DropFrameFlag, TimeCodeHours, TimeCodeMinutes, TimeCodeSeconds, TimeCodePictures;
    int   curr_mb_row;

    switch (idx)
    {
    case 0:
        if (profile_id != 0x20)
        {
            bValidSyntax = 0;
            fprintf(stdout, "\nInvalid Profile_ID: %d!\n", profile_id);
        }

        if (level_id != 0x10 && level_id != 0x20 && level_id != 0x40)
        {
            bValidSyntax = 0;
            fprintf(stdout, "\nInvalid Level_ID: %d!\n", level_id);
        }

        if (sample_precision != 1)
        {
            bValidSyntax = 0;
            fprintf(stdout, "\nsample_precision %d is reserved at the current profile!\n", sample_precision);
        }

        if (aspect_ratio == 0)
        {
            bValidSyntax = 0;
            fprintf(stdout, "\aspect_ratio %d is forbidden at the current profile!\n", aspect_ratio);
        }
        else if (aspect_ratio > 4)
        {
            bValidSyntax = 0;
            fprintf(stdout, "\aspect_ratio %d is reserved at the current profile!\n", aspect_ratio);
        }

        if (frame_rate_code<1 || frame_rate_code>8)
        {
            bValidSyntax = 0;
            fprintf(stdout, "Undefined frame_rate %d in all levels. \n", frame_rate_code);
        }

        if (horizontal_size == 0 || horizontal_size % 2 != 0)
        {
            bValidSyntax = 0;
            fprintf(stdout, "Image Width:%d does not meet the IVC restriction.\n", horizontal_size);
        }

        if (vertical_size == 0 || vertical_size % 2 != 0)
        {
            bValidSyntax = 0;
            fprintf(stdout, "Image Height:%d does not meet the IVC restriction.\n", vertical_size);
        }

        switch (level_id)
        {
        case 0x10:
            if (horizontal_size > 720)
            {
                bValidSyntax = 0;
                fprintf(stdout, "Image Width:%d exceeds level 2.0 restriction.\n", horizontal_size);
            }
            else if (vertical_size > 576)
            {
                bValidSyntax = 0;
                fprintf(stdout, "Image height:%d exceeds level 2.0 and level 2.1's restriction.\n", vertical_size);
            }

            if (chroma_format != 1)
            {
                bValidSyntax = 0;
                fprintf(stdout, "Chroma_format is %d. In level 2.0 only format 4:2:0 is supported.\n", chroma_format);
            }

            if (bbv_buffer_size > 1228800)
            {
                bValidSyntax = 0;
                fprintf(stdout, "Current Bbv_Buffer_Size is %d. Invalid Bbv_Buffer_Size input!\n", bbv_buffer_size);
            }

            break;

        case 0x20:
            if (horizontal_size > 1920)
            {
                bValidSyntax = 0;
                fprintf(stdout, "Image Width:%d exceeds level 4.0's restriction.\n", horizontal_size);
            }
            else if (vertical_size > 1152)
            {
                bValidSyntax = 0;
                fprintf(stdout, "Image Height:%d exceeds level 4.0's restriction.\n", vertical_size);
            }

            if (chroma_format != 1)
            {
                bValidSyntax = 0;
                fprintf(stdout, "Current chroma_format is %d. In level 4.0 only format 4:2:0 is supported.\n", chroma_format);
            }

            if (bbv_buffer_size > 2457600)
            {
                bValidSyntax = 0;
                fprintf(stdout, "Current Bbv_Buffer_Size is %d. Invalid Bbv_Buffer_Size input.\n", bbv_buffer_size);
            }

            break;

        case 0x40:
            if (horizontal_size > 4096)
            {
                bValidSyntax = 0;
                fprintf(stdout, "Image Width:%d exceeds level 6.0 and level 6.0.1's restriction.\n", horizontal_size);
            }
            else if (vertical_size > 2048)
            {
                bValidSyntax = 0;
                fprintf(stdout, "Image Height:%d exceeds level 6.0 and level 6.0.1's restriction.\n", vertical_size);
            }

            if (chroma_format != 1)
            {
                bValidSyntax = 0;
                fprintf(stdout, "Current chroma_format is %d. In level 6.0 and level 6.0.1 only format 4:2:0 is supported.\n", chroma_format);
            }

            if ( bbv_buffer_size > 249954304)
            {
                bValidSyntax = 0;
                fprintf(stdout, "Current Bbv_Buffer_Size is %d. Invalid Bbv_Buffer_Size input.\n", bbv_buffer_size);
            }

            break;

        default:

            bValidSyntax = 0;
            fprintf(stdout, "\n No such level_ID. \n");

        }
        break;

    case 1:
        if (time_flag)
        {
            DropFrameFlag = (time_code && 0x800000) >> 23;
            TimeCodeHours = (time_code && 0x7C0000) >> 18;
            TimeCodeMinutes = (time_code && 0x03F000) >> 12;
            TimeCodeSeconds = (time_code && 0x000FC0) >> 6;
            TimeCodePictures = (time_code && 0x00003F);

            if (TimeCodeHours < 0 || TimeCodeHours > 23)
            {
                bValidSyntax = 0;
                fprintf(stdout, "\nInvalid TimeCodeHours value: %d! TimeCodeHours should be in the range 0 ~ 23!\n", TimeCodeHours);
            }
            if (TimeCodeMinutes < 0 || TimeCodeMinutes > 59)
            {
                bValidSyntax = 0;
                fprintf(stdout, "\nInvalid TimeCodeMinutes value: %d! TimeCodeMinutes should be in the range 0 ~ 59!\n", TimeCodeMinutes);
            }
            if (TimeCodeSeconds < 0 || TimeCodeSeconds > 59)
            {
                bValidSyntax = 0;
                fprintf(stdout, "\nInvalid TimeCodeSeconds value: %d! TimeCodeSeconds should be in the range 0 ~ 59!\n", TimeCodeSeconds);
            }
            if (TimeCodePictures < 0 || TimeCodePictures > 59)
            {
                bValidSyntax = 0;
                fprintf(stdout, "\nInvalid TimeCodePictures value: %d! TimeCodePictures should be in the range 0 ~ 59!\n", TimeCodePictures);
            }
        }
        break;
    case 2:
        break;
    case 3:
        break;
    case 4:
        break;
    case 5:
        break;
    case 6:
        break;
    case 7:
        curr_mb_row = img->current_mb_nr / (img->PicWidthInMbs);
        if (vertical_size < 2800)
        {
            if (curr_mb_row != slice_vertical_position)
            {
                bValidSyntax = 0;
                fprintf(stdout, "\nInvalid Slice_vertical_position %d! Current mb_row is %d!\n", slice_vertical_position, curr_mb_row);
            }
        }
        else
        {
            if (curr_mb_row != ((slice_vertical_position_extension << 7) + slice_vertical_position))
            {
                bValidSyntax = 0;
                fprintf(stdout, "\nInvalid Slice_vertical_position %d! Current mb_row is %d!\n",
                    ((slice_vertical_position_extension << 7) + slice_vertical_position), curr_mb_row);
            }
        }
        break;
    default:
        break;
    }
    assert(bValidSyntax);
}

void CheckBitrate( int bitrate, int second )
{
    switch( level_id )
    {
        case 0x10:
            if( bitrate > 10000000 )
            {
                fprintf( stdout, "The %d second's bitrate: %d. Exceed level %d's maximum bitrate 10000000!", second,bitrate,level_id );
                bValidSyntax = 0;
            }
            break;
        case 0x20:
            if( bitrate > 20000000 )
            {
                fprintf( stdout, "The %d second's bitrate: %d. Exceed level %d's maximum bitrate 20000000!", second,bitrate,level_id );
                bValidSyntax = 0;
            }
            break;
        case 0x40:
            if( bitrate > 200000000 )
            {
                fprintf( stdout, "The %d second's bitrate: %d. Exceed level %d's maximum bitrate 200000000!", second,bitrate,level_id );
                bValidSyntax = 0;
            }
            break;
        default:
            break;
    }
    assert(bValidSyntax);
    return;
}
