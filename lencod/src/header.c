/*
*************************************************************************************
* File name:
* Function:
*
*************************************************************************************
*/
#include <math.h>
#include <assert.h>
#include <string.h>

#include "header.h"
#include "defines.h"
#include "vlc.h"

pel_t ALPHA_TABLE[64]  =
{
    0, 0, 0, 0, 0, 0, 1, 1,
    1, 1, 1, 2, 2, 2, 3, 3,
    4, 4, 5, 5, 6, 7, 8, 9,
    10,11,12,13,15,16,18,20,
    22,24,26,28,30,33,33,35,
    35,36,37,37,39,39,42,44,
    46,48,50,52,53,54,55,56,
    57,58,59,60,61,62,63,64
} ;

pel_t  BETA_TABLE[64]  =
{
    0, 0, 0, 0, 0, 0, 1, 1,
    1, 1, 1, 1, 1, 2, 2, 2,
    2, 2, 3, 3, 3, 3, 4, 4,
    4, 4, 5, 5, 5, 5, 6, 6,
    6, 7, 7, 7, 8, 8, 8, 9,
    9,10,10,11,11,12,13,14,
    15,16,17,18,19,20,21,22,
    23,23,24,24,25,25,26,27
};

/*
*************************************************************************
* Function:get alpha and beta
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void get_alpha_beta()
{
    input->Alpha = ALPHA_TABLE[Clip3(0, 63, img->frame_qp)];
    input->Beta = BETA_TABLE[Clip3(0, 63, img->frame_qp)];
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
int frametotc( int frame, int dropflag )
{
    int fps, pict, sec, minute, hour, tc;

    fps = ( int )( frame_rate+0.5 );
    pict = frame%fps;
    frame = ( frame-pict )/fps;
    sec = frame%60;
    frame = ( frame-sec )/60;
    minute = frame%60;
    frame = ( frame-minute )/60;
    hour = frame%24;
    tc = ( dropflag<<23 ) | ( hour<<18 ) | ( minute<<12 ) | ( sec<<6 ) | pict;

    return tc;
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
int IPictureHeader( Bitstream *bitstream, int frame )
{
    int len = 0;
    int tc;
    int time_flag;
    int bbv_delay;
    marker_bit=1;
    time_flag = 0;
    img->typeb=0;

    len += u_v( 32, "I picture start code",0x1B3, bitstream );

    bbv_delay = 0xFFFF;

    len+=u_v( 16,"bbv delay", bbv_delay,bitstream );
    len += u_v( 1, "time_flag",0,bitstream );
    if ( time_flag )
    {
        tc = frametotc( tc0+frame,img->dropflag );
        len += u_v( 24, "time_code",tc,bitstream );
    }

    len+=u_1( "marker_bit",1,bitstream );

    len+=u_v( 8,"picture_distance", img->pic_distance, bitstream );

    if( input->low_delay )
    {
        len+=ue_v( "bbv check times", 0,bitstream );
    }

    len+=u_v( 1,"fixed picture qp",input->fixed_picture_qp,bitstream );
    img->frame_qp = input->qp_1st_frm;
#if MULQP
    srand ( ( i32u_t )time( NULL ) );
    img->frame_qp = (rand() % 62) + 1;
#endif
#if DBLK_ABC
    srand ( ( i32u_t )time( NULL ) );
    img->frame_qp = (rand() % (63 - 22 + 1)) + 22;
#endif
    len+=u_v( 6,"I picture QP",img->frame_qp,bitstream );
    len+=u_v( 4,"reserved bits",0,bitstream );
    get_alpha_beta();
#if DBLK_D
    srand ( ( i32u_t )time( NULL ) );
    input->deblk_disable = rand() % 2;
#endif
    len+=u_v( 1,"loop filter disable",input->deblk_disable,bitstream );
    if ( !input->deblk_disable )
    {
        len+=u_v( 8,"alpha",input->Alpha,bitstream );
        len+=u_v( 6,"beta",input->Beta,bitstream );
    }

    return len;
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
int PBPictureHeader( Bitstream *bitstream )
{
    int len = 0;
    int bbv_delay;

    if ( img->type == P_IMG )
    {
        img->Mean_bit += img->SUM_bit;
        img->fourframe_bit = img->Mean_bit;
        img->p_sub_bits = img->p_one_bit + img->p_three_bit;

        if( ( img->ip_frm_idx )%HPGOPSIZE == 1 )
        {
            img->Mean_bit = 0;
            if( img->tag_hp == 1 )
            {
                if( img->p_sub_type_start/( 1.0*img->width/8*img->height/8 ) > 0.55 )
                {
                    img->level_fourframe_bit = img->fourframe_bit;
                    img->level_p_sub_bits = img->p_sub_bits;
                }
                else if ( img->ip_frm_idx == 9 )
                {
                    img->level_fourframe_bit = img->fourframe_bit;
                    img->level_p_sub_bits = img->p_sub_bits;
                }
            }
        }

        if (input->use_p_sub_type && (img->ip_frm_idx - img->i_frm_idx) % HPGOPSIZE == 1)
        {
            img->fourframe_bit = img->level_fourframe_bit ;
            img->p_sub_bits = img->level_p_sub_bits ;
            if ( img->ip_frm_idx == 5 )
            {
                if( img->p_sub_type_start/( 1.0*img->width/8*img->height/8 ) > 0.55 )
                {
                    img->tag_hp=1;
                }
            }
            else if ( img->ip_frm_idx > 5 )
            {
                if( ( img->ip_frm_idx % img->framerate ) < 4 )
                {
                    if( img->p_sub_type_start/( 1.0*img->width/8*img->height/8 ) > 0.55 )
                    {
                        img->tag_hp=1;
                    }
                }
                else if ( img->p_sub_type_start/( 1.0*img->width/8*img->height/8 ) < 0.25 )
                {
                    img->tag_hp=0;
                }
                else if ( img->p_sub_type_start/( 1.0*img->width/8*img->height/8 ) > 0.75 )
                {
                    img->tag_hp=1;
                }
                else if ( img->p_sub_bits / ( 1.0*img->fourframe_bit ) < 0.4 )
                {
                    img->tag_hp=1;
                }
                else
                {
                    img->tag_hp=0;
                }
            }

            if( input->p_sub_non_type_coding )
            {
                img->tag_hp = 1;
            }

            if( ( img->ip_frm_idx )%HPGOPSIZE == 1 )
            {
                if( img->tag_hp==1 )
                {
                    printf( "proportion=%5.3f,extended P coding\n",( img->p_sub_type_start/( 1.0*img->width/8*img->height/8 ) ) );
                }
                else
                {
                    printf( "proportion=%5.3f,not extended P coding\n",( img->p_sub_type_start/( 1.0*img->width/8*img->height/8 ) ) );
                }
            }
        }

        if( input->use_p_sub_type && img->tag_hp )
        {
            if ((img->ip_frm_idx - img->i_frm_idx) % HPGOPSIZE == 0)
            {
                img->typeb=L1_IMG;
            }
            else if ((img->ip_frm_idx - img->i_frm_idx) % HPGOPSIZE == HPGOPSIZE / 2)
            {
                img->typeb=L1_IMG;
            }
            else if ((img->ip_frm_idx - img->i_frm_idx) % HPGOPSIZE == HPGOPSIZE / 2 - 1)
            {
                img->typeb=L2_IMG;
            }
            else if ((img->ip_frm_idx - img->i_frm_idx) % HPGOPSIZE<HPGOPSIZE)
            {
                img->typeb=L2_IMG;
            }
            else
            {
                img->typeb=L1_IMG;
            }
        }
        else
        {
            img->typeb=L1_IMG;
        }
        img->picture_coding_type = P_IMG;
    }
    else
    {
        img->picture_coding_type = B_IMG;
    }

    bbv_delay = 0xFFFF;

    len+=u_v( 24,"start code prefix",1,bitstream );
    len+=u_v( 8, "PB picture start code",0xB6,bitstream );
    len+=u_v( 16,"bbv delay", bbv_delay,bitstream );
    len+=u_v( 2,"picture coding type", img->picture_coding_type,bitstream );
    len+=u_v( 2,"sub picture  type", img->typeb,bitstream );
    len+=u_v( 8,"picture_distance", img->pic_distance, bitstream );

    if( input->low_delay )
    {
        len+=ue_v( "bbv check times", 0,bitstream );
    }

    len+=u_v( 1,"fixed qp",input->fixed_picture_qp,bitstream );

    //rate control
    if( img->type==P_IMG )
    {
        if( input->use_p_sub_type&& img->tag_hp )
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
#if MULQP
        srand ( ( i32u_t )time( NULL ) );
        img->frame_qp = ( rand() % 62 ) + 1;
#endif
#if DBLK_ABC
        srand ( ( i32u_t )time( NULL ) );
        img->frame_qp = ( rand() % ( 63-22+1 ) ) + 22;
#endif
        img->frame_qp = MAX(MIN_QP, MIN(img->frame_qp, MAX_QP));
        len += u_v(6, "P picture QP", img->frame_qp, bitstream);
    }
    else if( img->type==B_IMG )
    {
        img->frame_qp = input->qp_B_frm;
#if MULQP
        srand ( ( i32u_t )time( NULL ) );
        img->frame_qp = (rand() % 62) + 1;
#endif
#if DBLK_ABC
        srand ( ( i32u_t )time( NULL ) );
        img->frame_qp = (rand() % (63 - 22 + 1)) + 22;
#endif
        len+=u_v( 6,"B picture QP",img->frame_qp,bitstream );
    }

    len+=u_v( 1, "no_forward_reference_flag", 0, bitstream );
    len+=u_v( 3,"reserved bits",0,bitstream );

    get_alpha_beta();
#if DBLK_D
    srand ( ( i32u_t )time( NULL ) );
    input->deblk_disable = rand() % 2;
#endif
    len+=u_v( 1,"loop filter disable",input->deblk_disable,bitstream );
    if ( !input->deblk_disable )
    {
        len+=u_v( 8,"alpha",input->Alpha,bitstream );
        len+=u_v( 6,"beta",input->Beta,bitstream );
    }
    return len;
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

int SliceHeader( Bitstream *bitstream, int slice_qp )
{
    int len = 0;

    int mb_row;
    int slice_vertical_position;

    len+=u_v( 24,"start code prefix",1,bitstream );

    slice_vertical_position = img->current_mb_nr/( img->width>>4 );
    if( img->height > 2800 )
    {
        slice_vertical_position_extension = slice_vertical_position >> 7;
        slice_vertical_position = slice_vertical_position % 128;
    }

    len+=u_v( 8, "slice vertical position",( img->current_mb_nr )/( img->width>>4 ),bitstream );
    if( img->height > 2800 )
    {
        len += u_v( 3, "slice vertical position extension",slice_vertical_position_extension,bitstream );
    }

    if( img->height > 2800 )
    {
        mb_row = ( slice_vertical_position_extension << 7 ) +slice_vertical_position;
    }
    else
    {
        mb_row = slice_vertical_position;
    }

    if( !input->fixed_picture_qp )
    {
        len += u_v( 1,"fixed_slice_qp",1,bitstream );
        len += u_v( 6,"slice_qp",slice_qp,bitstream );
        img->frame_qp = slice_qp;
    }

    return len;
}
